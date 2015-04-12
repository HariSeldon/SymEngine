#ifndef SUBSCRIPT_ANALYSIS_H
#define SUBSCRIPT_ANALYSIS_H

#include "SymEngine/BlockMask.h"
#include "SymEngine/OCLEnv.h"
#include "SymEngine/Utils.h"

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

namespace SymEngine {

class NDRangePoint;
class OCLEnv;

class SubscriptAnalysis {
public:
  SubscriptAnalysis(llvm::ScalarEvolution *se, OCLEnv &&ocl, BlockMask &&blockMask);

public:
  int getBankConflictNumber(llvm::Instruction *inst, llvm::Value *value);
  int getBankConflictNumberInLoop(llvm::Instruction *inst, llvm::Value *value,
                                  const llvm::SCEV *tripCount);
  int getTransactionNumber(llvm::Instruction *inst, llvm::Value *value);
  int getTransactionNumberInLoop(llvm::Instruction *inst, llvm::Value *value,
                                 const llvm::SCEV *tripCount);

private:
  llvm::ScalarEvolution *scalarEvolution;
  const OCLEnv ocl;
  BlockMask blockMask;
  typedef std::map<const llvm::SCEV *, const llvm::SCEV *> SCEVMap;

private:
  std::vector<const llvm::SCEV *> analyzeSubscript(llvm::Instruction *inst,
                                                   const llvm::SCEV *scev,
                                                   Warp &warp);

  // Thread-dependet replacing methods.
  const llvm::SCEV *replaceInExpr(const llvm::SCEV *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInExpr(const llvm::SCEVAddRecExpr *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInExpr(const llvm::SCEVCommutativeExpr *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInExpr(const llvm::SCEVConstant *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInExpr(const llvm::SCEVUnknown *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInExpr(const llvm::SCEVUDivExpr *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInExpr(const llvm::SCEVCastExpr *expr,
                                  const NDRangePoint &point,
                                  SCEVMap &processed);
  const llvm::SCEV *replaceInPhi(llvm::PHINode *Phi, const NDRangePoint &point,
                                 SCEVMap &processed);

  const llvm::SCEV *resolveInstruction(llvm::Instruction *instruction,
                                       const NDRangePoint &point);

  int resolveTripCount(const llvm::SCEV *tripCount);
  
  bool isOperationExecuted(const NDRangePoint &point, llvm::Instruction *inst);

};

}

#endif
