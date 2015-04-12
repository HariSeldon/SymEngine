#ifndef CONTROL_DEPENCE_ANALYSIS_H
#define CONTROL_DEPENCE_ANALYSIS_H

// Implementation based on Ferrante et al. "Program Dependecene Graph and Its
// Use in Optimization". page 324
// The notation of the variables is taken from the paper.

#include "SymEngine/Utils.h"

#include "llvm/Pass.h"

#include "llvm/Analysis/PostDominators.h"

#include <map>
#include <vector>

namespace SymEngine {

class ControlDependenceAnalysis : public llvm::FunctionPass {
public:
  static char ID;
  ControlDependenceAnalysis();

public:
  virtual bool runOnFunction(llvm::Function &function);
  virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;

  std::vector<BlockBranchPair> getControllers(llvm::BasicBlock *block); 
  void dump();

private:
  typedef std::map<llvm::BasicBlock *, std::vector<BlockBranchPair>> GraphMap;

private:
  void findS(llvm::Function &function);
  void findLs();
  void buildForwardGraph(llvm::Function &function);
  void initGraph(llvm::Function &function, GraphMap& graph);
  void buildBackwardGraph(llvm::Function &function);
  void transitiveClosure();

private:
  llvm::PostDominatorTree *pdt;
  GraphMap forwardGraph;
  GraphMap backwardGraph;
  std::vector<std::pair<llvm::BasicBlock *, llvm::BasicBlock *> > s;
  std::vector<bool> edgesType; 
  BlockVector ls;
};

}

#endif
