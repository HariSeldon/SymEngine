#ifndef BLOCK_MASK_H
#define BLOCK_MASK_H

#include "llvm/Analysis/ScalarEvolution.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"

#include "llvm/Support/raw_ostream.h"

#include <utility>

namespace SymEngine {

// -----------------------------------------------------------------------------
class BlockCondition {
public:
  static BlockCondition createTrueCondition() { return BlockCondition(true); }
  static BlockCondition createFalseCondition() { return BlockCondition(false); }
  static BlockCondition createCondition(const llvm::SCEV *scev,
                                        llvm::CmpInst::Predicate predicate) {
    return BlockCondition(scev, predicate);
  }

private:
  BlockCondition(bool condition) : scev(nullptr), condition(condition) {}
  BlockCondition(const llvm::SCEV *scev, llvm::CmpInst::Predicate predicate)
      : scev(scev), predicate(predicate) {}

public:
  bool isTrue() const { return scev == nullptr && condition; }
  bool isFalse() const { return scev == nullptr && !condition; }
  const llvm::SCEV *getSCEV() const { return scev; }
  llvm::CmpInst::Predicate getPredicate() const { return predicate; }
  void invertPredicate() {
    predicate = llvm::CmpInst::getInversePredicate(predicate);
  }
  void dump() {
    if (scev == nullptr)
      llvm::errs() << "Block condition: " << condition << "\n";
    else {
      llvm::errs() << "Block condition: ";
      scev->dump();
      llvm::errs() << "predicate: " << predicate << "\n";
    }
  }

  static bool getBooleanValue(const llvm::SCEV *scev,
                              llvm::CmpInst::Predicate predicate,
                              llvm::ScalarEvolution *se) {
    switch (predicate) {
    case llvm::CmpInst::FCMP_FALSE:
      return false;
    case llvm::CmpInst::FCMP_OEQ:
      return scev->isZero();
    case llvm::CmpInst::FCMP_OGT:
      return se->isKnownPositive(scev);
    case llvm::CmpInst::FCMP_OGE:
      return se->isKnownNonNegative(scev);
    case llvm::CmpInst::FCMP_OLT:
      return se->isKnownNegative(scev);
    case llvm::CmpInst::FCMP_OLE:
      return se->isKnownNonPositive(scev);
    case llvm::CmpInst::FCMP_ONE:
      return se->isKnownNonZero(scev);
    case llvm::CmpInst::FCMP_ORD:
      return false;
    case llvm::CmpInst::FCMP_UNO:
      return false;
    case llvm::CmpInst::FCMP_UEQ:
      return scev->isZero();
    case llvm::CmpInst::FCMP_UGT:
      return se->isKnownPositive(scev);
    case llvm::CmpInst::FCMP_UGE:
      return se->isKnownNonNegative(scev);
    case llvm::CmpInst::FCMP_ULT:
      return se->isKnownNegative(scev);
    case llvm::CmpInst::FCMP_ULE:
      return se->isKnownNonPositive(scev);
    case llvm::CmpInst::FCMP_UNE:
      return se->isKnownNonZero(scev);
    case llvm::CmpInst::FCMP_TRUE:
      return true;
    case llvm::CmpInst::BAD_FCMP_PREDICATE:
      return false;
    case llvm::CmpInst::ICMP_EQ:
      return scev->isZero();
    case llvm::CmpInst::ICMP_NE:
      return se->isKnownNonZero(scev);
    case llvm::CmpInst::ICMP_UGT:
      return se->isKnownPositive(scev);
    case llvm::CmpInst::ICMP_UGE:
      return se->isKnownNonNegative(scev);
    case llvm::CmpInst::ICMP_ULT:
      return se->isKnownNegative(scev);
    case llvm::CmpInst::ICMP_ULE:
      return se->isKnownNonPositive(scev);
    case llvm::CmpInst::ICMP_SGT:
      return se->isKnownPositive(scev);
    case llvm::CmpInst::ICMP_SGE:
      return se->isKnownPositive(scev);
    case llvm::CmpInst::ICMP_SLT:
      return se->isKnownNegative(scev);
    case llvm::CmpInst::ICMP_SLE:
      return se->isKnownNonPositive(scev);
    case llvm::CmpInst::BAD_ICMP_PREDICATE:
      return false;
    };
    return false;
  }

private:
  const llvm::SCEV *scev;
  llvm::CmpInst::Predicate predicate;
  bool condition;
};

class ControlDependenceAnalysis;

// -----------------------------------------------------------------------------
class BlockMask {
public:
  BlockMask(llvm::Function *function, ControlDependenceAnalysis *cdGraph,
            llvm::ScalarEvolution *scalarEvolution);

public:
  void createMasks();
  void dump();
  std::vector<BlockCondition> getConditions(llvm::BasicBlock *block);

public:
  static bool getBooleanValue(llvm::CmpInst::Predicate predicate);

private:
  BlockCondition getBranchCondition(llvm::BasicBlock *block);

private:
  llvm::Function *function;
  ControlDependenceAnalysis *cdGraph;
  llvm::ScalarEvolution *scalarEvolution;
  std::map<llvm::BasicBlock *, std::vector<BlockCondition>> blocksMask;
};
}

#endif
