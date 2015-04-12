#ifndef SYMBOLIC_EXECUTION_H
#define SYMBOLIC_EXECUTION_H

#include "SymEngine/NDRangeSpace.h"

#include "llvm/Pass.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include "llvm/IR/InstVisitor.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
class Function;
class StoreInst;
class LoadInst;
}

namespace SymEngine {
class NDRange;
class OCLEnv;
class SubscriptAnalysis;
class ControlDependenceAnalysis;
}

/// Collect information about the kernel function.
namespace SymEngine {
class SymbolicExecution : public llvm::FunctionPass,
                          public llvm::InstVisitor<SymbolicExecution> {

  friend class llvm::InstVisitor<SymbolicExecution>;

public:
  static char ID;
  SymbolicExecution();
  ~SymbolicExecution();

  virtual bool runOnFunction(llvm::Function &F);
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

public:
  // These result vectors will contain 1 entry per memory operation.
  std::vector<int> loadTransactions;
  std::vector<int> storeTransactions;

  std::vector<int> loadBankConflicts;
  std::vector<int> storeBankConflicts;

private:
  void memoryAccessAnalysis(llvm::BasicBlock &block,
                            std::vector<int> &loadTrans,
                            std::vector<int> &storeTrans);
  void init();
  void initBuffers();
  void initOCLSpace();
  void visitLoadInst(llvm::LoadInst &loadInst);
  void visitStoreInst(llvm::StoreInst &storeInst);
  int visitMemoryInst(llvm::Instruction *inst, llvm::Value *pointer,
                      std::vector<int> &resultVector);
  int visitMemoryInstInLoop(llvm::Instruction *inst, llvm::Value *pointer,
                            std::vector<int> &resultVector, llvm::Loop *loop);
  int visitLocalMemoryInst(llvm::Instruction *inst, llvm::Value *pointer,
                           std::vector<int> &resultVector);
  int visitLocalMemoryInstInLoop(llvm::Instruction *inst, llvm::Value *pointer,
                                 std::vector<int> &resultVector,
                                 llvm::Loop *loop);
  void visitPointer(llvm::Instruction *inst, llvm::Value *pointer,
                    std::vector<int> &bankConflicts,
                    std::vector<int> &transactions);
  void dump();

private:
  llvm::ScalarEvolution *scalarEvolution;
  SymEngine::SubscriptAnalysis *subscriptAnalysis;
  SymEngine::NDRange *ndr;
  SymEngine::ControlDependenceAnalysis *cdGraph;
  llvm::LoopInfo *loopInfo;
};
}

#endif
