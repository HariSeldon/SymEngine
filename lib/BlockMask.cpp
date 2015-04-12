#include "SymEngine/BlockMask.h"

#include "SymEngine/ControlDependenceAnalysis.h"

#include <deque>

using namespace llvm;
using namespace SymEngine;

// -----------------------------------------------------------------------------
BlockMask::BlockMask(Function *function, 
                     ControlDependenceAnalysis *cdGraph,
                     ScalarEvolution *scalarEvolution)
    : function(function), cdGraph(cdGraph), scalarEvolution(scalarEvolution) {}

// -----------------------------------------------------------------------------
void BlockMask::createMasks() {
  // Initialize the map.
  for (auto iter = function->begin(), iterEnd = function->end(); iter != iterEnd;
       ++iter) {
    BasicBlock *block = iter;
    blocksMask[block];
  }

  for (auto iter = function->begin(), iterEnd = function->end(); iter != iterEnd;
       ++iter) {
    BasicBlock *block = iter;
    auto &conditions = blocksMask[block];

    // Perform an up-ward traversal of cd graph to determine the masks of the
    // controllers.
    std::vector<BlockBranchPair> controllers = cdGraph->getControllers(block);
    
    std::set<BlockBranchPair> toVisit(controllers.begin(), controllers.end());
  
    while(!toVisit.empty()) {
      BlockBranchPair controllerPair = *toVisit.begin();
      toVisit.erase(controllerPair);
      BasicBlock *controller = controllerPair.first;
      bool swap = !controllerPair.second; 
      
      BlockCondition condition = getBranchCondition(controller); 
      if(swap)
        condition.invertPredicate();
      
      conditions.push_back(condition);

      controllers = cdGraph->getControllers(controller);
      toVisit.insert(controllers.begin(), controllers.end()); 
    }
  }
}

// -----------------------------------------------------------------------------
void BlockMask::dump() {
  for (auto iter : blocksMask) {
    errs() << "------------------"  << iter.first->getName() << "\n"; 
    auto conditionVector = iter.second;
    for (auto &&condition : conditionVector) {
      condition.dump();
    }
    errs() << "------------------\n"; 
  }
}

// -----------------------------------------------------------------------------
BranchInst *getBranch(llvm::BasicBlock *block) {
  TerminatorInst *terminator = block->getTerminator();
  BranchInst *branch = dyn_cast<BranchInst>(terminator);
  assert(branch != nullptr && "Ill-formatted block");
  return branch;
}

// -----------------------------------------------------------------------------
BlockCondition BlockMask::getBranchCondition(llvm::BasicBlock *block) {
  BranchInst *branch = getBranch(block);

  if (branch->isUnconditional())
    return BlockCondition::createTrueCondition();

  Value *condition = branch->getCondition();

  if (CmpInst *cmpInst = dyn_cast<CmpInst>(condition)) {
    Value *firstOperand = cmpInst->getOperand(0);
    Value *secondOperand = cmpInst->getOperand(1);

    const SCEV *firstSCEV = scalarEvolution->getSCEV(firstOperand);
    const SCEV *secondSCEV = scalarEvolution->getSCEV(secondOperand);

    const SCEV *diff = scalarEvolution->getMinusSCEV(firstSCEV, secondSCEV);

    return BlockCondition::createCondition(diff, cmpInst->getPredicate());

  } else {
    return BlockCondition::createTrueCondition();
  }
}

// -----------------------------------------------------------------------------
std::vector<BlockCondition> BlockMask::getConditions(llvm::BasicBlock *block) {
  return blocksMask[block];
}

