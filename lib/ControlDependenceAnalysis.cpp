#include "SymEngine/ControlDependenceAnalysis.h"

#include "SymEngine/Utils.h"

#include "llvm/Analysis/CFG.h"

#include <algorithm>
#include <functional>

#include "llvm/Support/raw_ostream.h"

using namespace SymEngine;
using namespace llvm;

char ControlDependenceAnalysis::ID = 0;
static RegisterPass<ControlDependenceAnalysis> X("dependence-analysis",
                                                 "Control dependence analysis");

ControlDependenceAnalysis::ControlDependenceAnalysis() : FunctionPass(ID) {}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::getAnalysisUsage(AnalysisUsage &au) const {
  au.addRequired<PostDominatorTree>();
  au.setPreservesAll();
}

// -----------------------------------------------------------------------------
bool ControlDependenceAnalysis::runOnFunction(Function &function) {
  pdt = &getAnalysis<PostDominatorTree>();

  forwardGraph.clear();
  backwardGraph.clear();
  s.clear();
  ls.clear();

  findS(function);
  findLs();
  buildForwardGraph(function);
  //transitiveClosure();
  buildBackwardGraph(function);

  return false;
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::findS(Function &function) {
  for (Function::iterator iter = function.begin(), iterEnd = function.end();
       iter != iterEnd; ++iter) {
    BasicBlock *block = iter;

    TerminatorInst *terminator = block->getTerminator();
    int successorsNumber = terminator->getNumSuccessors();
    assert(successorsNumber <= 2 &&
           "Branches with more than 2 successors are not supported");

    if (successorsNumber == 1)
      continue;

    for (int successor = 0; successor < successorsNumber; ++successor) {
      BasicBlock *child = terminator->getSuccessor(successor);
      if (!pdt->dominates(child, block)) {
        s.push_back(std::make_pair(block, child));
        edgesType.push_back(static_cast<bool>(1 - successor));
      }
    }
  }
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::findLs() {
  for (auto edge : s) {
    BasicBlock *block =
        pdt->findNearestCommonDominator(edge.first, edge.second);
    assert(block != nullptr && "Ill-formatted function");
    ls.push_back(block);
  }
  assert(ls.size() == s.size() && "Mismatching S and Ls");
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::buildForwardGraph(Function &function) {
  initGraph(function, forwardGraph);
  unsigned int edgeNumber = s.size();
  for (unsigned int index = 0; index < edgeNumber; ++index) {
    auto edge = s[index];
    bool edgeType = edgesType[index];
    BasicBlock *l = ls[index];

    BasicBlock *a = edge.first;
    BasicBlock *b = edge.second;

    std::vector<BlockBranchPair> children; 
    BasicBlock *aParent = pdt->getNode(a)->getIDom()->getBlock();

    assert((l == aParent || l == a) && "Ill-formatted function");

    // Case 1.
    if (l == aParent) {
      BasicBlock *current = b;
      while (current != l) {
        children.push_back(std::make_pair(current, edgeType));
        current = pdt->getNode(current)->getIDom()->getBlock();
      }
    }

    // Case 2.
    if (l == a) {
      BasicBlock *current = b;
      while (current != aParent) {
        children.push_back(std::make_pair(current, edgeType));
        current = pdt->getNode(current)->getIDom()->getBlock();
      }
    }

    auto &outputVector = forwardGraph[a];
    outputVector.insert(outputVector.end(), children.begin(), children.end());

    auto aIter =
        std::find_if(outputVector.begin(), outputVector.end(),
                  [a](BlockBranchPair &pair) { return a == pair.first; });
    if(aIter != outputVector.end()) {
      outputVector.erase(aIter);
    }
  }
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::initGraph(Function &function, GraphMap &graph) {
  for (auto iter = function.begin(), iterEnd = function.end(); iter != iterEnd;
       ++iter) {
    BasicBlock *block = iter;
    graph[block];
  }
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::transitiveClosure() {
//  for (auto graphIter : forwardGraph) {
//    BlockVector &seeds = graphIter.second;
//    // This implements a traversal of the tree starting from block.
//    BlockSet worklist(seeds.begin(), seeds.end());
//    BlockVector result;
//    while (!worklist.empty()) {
//      BlockSet::iterator iter = worklist.begin();
//      BasicBlock *current = *iter;
//      worklist.erase(iter);
//      result.push_back(current);
//      BlockVector &children = forwardGraph[current];
//
//      for (auto block : children) {
//        if (!isPresent(block, result))
//          worklist.insert(block);
//      }
//    }
//
//    // Update block vector.
//    graphIter.second.assign(result.begin(), result.end());
//  }
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::buildBackwardGraph(Function &function) {
  initGraph(function, backwardGraph); 
  for (auto graphIter : forwardGraph) {
    auto block = graphIter.first;
    auto &children = graphIter.second;

    for (auto child : children) {
      auto childBlock = child.first;
      bool childType = child.second;
      backwardGraph[childBlock].push_back(std::make_pair(block, childType));
    } 
  }
}

// -----------------------------------------------------------------------------
std::vector<BlockBranchPair>
ControlDependenceAnalysis::getControllers(BasicBlock *block) {
  return backwardGraph[block];
}

// -----------------------------------------------------------------------------
void ControlDependenceAnalysis::dump() {
  errs() << "Forward:\n";
  for (auto graphIter : forwardGraph) {
    BasicBlock *block = graphIter.first;
    errs() << block->getName() << ": ";
    auto &children = graphIter.second;
    for (auto child : children) {
      errs() << child.first->getName() << " (" << child.second << ") ";
    }
    errs() << "\n";
  }
  errs() << "Backward:\n";
  for (auto graphIter : backwardGraph) {
    BasicBlock *block = graphIter.first;
    errs() << block->getName() << ": ";
    auto &children = graphIter.second;
    for (auto child : children) {
      errs() << child.first->getName() << " (" << child.second << ") ";
    }
    errs() << "\n";
  }
}
