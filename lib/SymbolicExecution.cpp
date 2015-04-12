#include "SymEngine/SymbolicExecution.h"

#include "SymEngine/BlockMask.h"
#include "SymEngine/ControlDependenceAnalysis.h"
#include "SymEngine/NDRange.h"
#include "SymEngine/OCLEnv.h"
#include "SymEngine/SubscriptAnalysis.h"
#include "SymEngine/Utils.h"
#include "SymEngine/Warp.h"

#include "llvm/Analysis/ScalarEvolution.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"

#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

using namespace llvm;
using namespace SymEngine;

static cl::opt<std::string>
    kernelName("symbolic-kernel-name", cl::init(""), cl::Hidden,
               cl::desc("Name of the kernel to analyze"));

static cl::opt<bool> loopMultiplier(
    "symbolic-loop-multiplier", cl::init(false), cl::Hidden,
    cl::desc("Control whether output is multiplied by loop trip count"));

char SymbolicExecution::ID = 0;
static RegisterPass<SymbolicExecution>
    X("symbolic-execution",
      "Perform symbolic execution tracing memory accesses");

using yaml::MappingTraits;
using yaml::SequenceTraits;
using yaml::IO;
using yaml::Output;

// Support functions.
//------------------------------------------------------------------------------
void addTransactionMetadata(Instruction *inst, int transactionNumber);
void addConflictMetadata(Instruction *inst, int conflictNumber);

namespace llvm {
namespace yaml {

//------------------------------------------------------------------------------
template <> struct MappingTraits<SymbolicExecution> {
  static void mapping(IO &io, SymbolicExecution &exe) {
    io.mapRequired("load_transactions", exe.loadTransactions);
    io.mapRequired("store_transactions", exe.storeTransactions);

    io.mapRequired("load_bank_conflicts", exe.loadBankConflicts);
    io.mapRequired("store_bank_conflicts", exe.storeBankConflicts);
  }
};

//------------------------------------------------------------------------------
// Sequence of ints.
template <> struct SequenceTraits<std::vector<int>> {
  static size_t size(IO &, std::vector<int> &seq) { return seq.size(); }
  static int &element(IO &, std::vector<int> &seq, size_t index) {
    if (index >= seq.size())
      seq.resize(index + 1);
    return seq[index];
  }

  static const bool flow = true;
};
}
}

// =============================================================================
SymbolicExecution::SymbolicExecution()
    : FunctionPass(ID), subscriptAnalysis(nullptr) {}

SymbolicExecution::~SymbolicExecution() {
  if (subscriptAnalysis != nullptr)
    delete subscriptAnalysis;
}

//------------------------------------------------------------------------------
bool SymbolicExecution::runOnFunction(Function &function) {
  if (function.getName() != kernelName)
    return false;

  loopInfo = &getAnalysis<LoopInfo>();
  scalarEvolution = &getAnalysis<ScalarEvolution>();
  ndr = &getAnalysis<NDRange>();
  cdGraph = &getAnalysis<ControlDependenceAnalysis>();

  BlockMask blockMask(&function, cdGraph, scalarEvolution);
  blockMask.createMasks();

  OCLEnv ocl(function, ndr);
  subscriptAnalysis = new SubscriptAnalysis(scalarEvolution, std::move(ocl),
                                            std::move(blockMask));

  initBuffers();
  visit(function);
  dump();

  return false;
}

//------------------------------------------------------------------------------
void SymbolicExecution::initBuffers() {
  loadTransactions.clear();
  storeTransactions.clear();

  loadBankConflicts.clear();
  storeBankConflicts.clear();
}

//------------------------------------------------------------------------------
void SymbolicExecution::getAnalysisUsage(AnalysisUsage &au) const {
  au.addRequired<ScalarEvolution>();
  au.addRequired<NDRange>();
  au.addRequired<LoopInfo>();
  au.addRequired<ControlDependenceAnalysis>();
  au.setPreservesAll();
}

//------------------------------------------------------------------------------
int SymbolicExecution::visitLocalMemoryInst(Instruction *inst, Value *pointer,
                                            std::vector<int> &resultVector) {
  int transactionNumber =
      subscriptAnalysis->getBankConflictNumber(inst, pointer);
  resultVector.push_back(transactionNumber);
  return transactionNumber;
}

//------------------------------------------------------------------------------
int
SymbolicExecution::visitLocalMemoryInstInLoop(Instruction *inst, Value *pointer,
                                              std::vector<int> &resultVector,
                                              Loop *loop) {
  if(!loopMultiplier)
    return visitLocalMemoryInst(inst, pointer, resultVector);

  const SCEV *tripCount = scalarEvolution->getBackedgeTakenCount(loop);
  int transactionNumber =
      subscriptAnalysis->getBankConflictNumberInLoop(inst, pointer, tripCount);
  resultVector.push_back(transactionNumber);
  return transactionNumber;
}

//------------------------------------------------------------------------------
int SymbolicExecution::visitMemoryInst(Instruction *inst, Value *pointer,
                                       std::vector<int> &resultVector) {
  int transactionNumber =
      subscriptAnalysis->getTransactionNumber(inst, pointer);
  resultVector.push_back(transactionNumber);
  return transactionNumber;
}

//------------------------------------------------------------------------------
int SymbolicExecution::visitMemoryInstInLoop(Instruction *inst, Value *pointer,
                                             std::vector<int> &resultVector,
                                             Loop *loop) {
  if(!loopMultiplier)
    return visitMemoryInst(inst, pointer, resultVector);

  const SCEV *tripCount = scalarEvolution->getBackedgeTakenCount(loop);
  int transactionNumber =
      subscriptAnalysis->getTransactionNumberInLoop(inst, pointer, tripCount);
  resultVector.push_back(transactionNumber);
  return transactionNumber;
}

//------------------------------------------------------------------------------
void SymbolicExecution::visitPointer(Instruction *inst, Value *pointer,
                                     std::vector<int> &bankConflicts,
                                     std::vector<int> &transactions) {
  if (const GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(pointer)) {
    if (gep->getPointerAddressSpace() == OCLEnv::LOCAL_AS) {
      // Local memory instruction.
      int conflictNumber = 0;
      if (Loop *loop = loopInfo->getLoopFor(inst->getParent())) {
        conflictNumber =
            visitLocalMemoryInstInLoop(inst, pointer, bankConflicts, loop);
      } else {
        conflictNumber = visitLocalMemoryInst(inst, pointer, bankConflicts);
      }
      addConflictMetadata(inst, conflictNumber);
    } else {
      // Global memory instruction.
      int transactionNumber = 0;
      if (Loop *loop = loopInfo->getLoopFor(inst->getParent())) {
        transactionNumber =
            visitMemoryInstInLoop(inst, pointer, transactions, loop);
      } else {
        transactionNumber = visitMemoryInst(inst, pointer, transactions);
      }
      addTransactionMetadata(inst, transactionNumber);
    }
  }
}

//------------------------------------------------------------------------------
void SymbolicExecution::visitStoreInst(StoreInst &storeInst) {
  Instruction *inst = &storeInst;
  Value *pointer = storeInst.getOperand(1);
  visitPointer(inst, pointer, storeBankConflicts, storeTransactions);
}

//------------------------------------------------------------------------------
void SymbolicExecution::visitLoadInst(LoadInst &loadInst) {
  Instruction *inst = &loadInst;
  Value *pointer = loadInst.getOperand(0);
  visitPointer(inst, pointer, loadBankConflicts, loadTransactions);
}

//------------------------------------------------------------------------------
void SymbolicExecution::dump() {
  Output yout(llvm::outs());
  yout << *this;
}

//------------------------------------------------------------------------------
void addMetadata(Instruction *inst, int value, const std::string &mdName) {
  auto constant =
      ConstantInt::get(IntegerType::get(inst->getContext(), 32), value);
  auto md = ConstantAsMetadata::get(constant);
  inst->setMetadata(mdName, MDNode::get(inst->getContext(), md));
}

//------------------------------------------------------------------------------
void addTransactionMetadata(Instruction *inst, int transactionNumber) {
  addMetadata(inst, transactionNumber, "transaction_number");
}

//------------------------------------------------------------------------------
void addConflictMetadata(Instruction *inst, int conflictNumber) {
  addMetadata(inst, conflictNumber, "conflict_number");
}
