#include "SymEngine/SubscriptAnalysis.h"

#include "SymEngine/MemoryAccessesAnalyzer.h"
#include "SymEngine/NDRange.h"
#include "SymEngine/NDRangePoint.h"
#include "SymEngine/OCLEnv.h"
#include "SymEngine/Warp.h"

#include "llvm/IR/Instructions.h"

#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <functional>
#include <iterator>

using namespace llvm;
using namespace SymEngine;

int getTypeWidth(const Type *type);

//------------------------------------------------------------------------------
SubscriptAnalysis::SubscriptAnalysis(ScalarEvolution *scalarEvolution,
                                     OCLEnv &&ocl, BlockMask &&blockMask)
    : scalarEvolution(scalarEvolution), ocl(std::move(ocl)),
      blockMask(std::move(blockMask)) {}

//------------------------------------------------------------------------------
const SCEV *getSCEV(Value *value, ScalarEvolution *scalarEvolution) {
  if (!isa<GetElementPtrInst>(value)) {
    return nullptr;
  }

  if (!scalarEvolution->isSCEVable(value->getType())) {
    return nullptr;
  }

  return scalarEvolution->getSCEV(value);
}

//------------------------------------------------------------------------------
int SubscriptAnalysis::getBankConflictNumber(Instruction *inst, Value *value) {
  auto scev = getSCEV(value, scalarEvolution);
  if (scev == nullptr)
    return -1;

  auto warps = ocl.getWarps();
  std::vector<int> conflictNumber;

  for (auto currentWarp : warps) {
    std::vector<const SCEV *> addressesVector =
        analyzeSubscript(inst, scev, currentWarp);
    if (addressesVector.empty())
      continue;

    int currentBankConflicts = computeBankConflictNumber(
        addressesVector, ocl.getHWConfig(), scalarEvolution);
    conflictNumber.push_back(currentBankConflicts);
  }

  int result = std::accumulate(conflictNumber.begin(), conflictNumber.end(), 0,
                               std::plus<int>());
  return result;
}

//------------------------------------------------------------------------------
int SubscriptAnalysis::getBankConflictNumberInLoop(Instruction *inst,
                                                   Value *value,
                                                   const SCEV *tripCount) {
  return getBankConflictNumber(inst, value) * resolveTripCount(tripCount);
}

//------------------------------------------------------------------------------
int SubscriptAnalysis::getTransactionNumber(Instruction *inst, Value *value) {
  auto scev = getSCEV(value, scalarEvolution);
  if (scev == nullptr)
    return -1;

  auto warps = ocl.getWarps();
  std::vector<int> transactionNumber;

  for (auto currentWarp : warps) {
    std::vector<const SCEV *> addressesVector =
        analyzeSubscript(inst, scev, currentWarp);
    if (addressesVector.empty())
      continue;

    int currentTransactionNumber = computeTransactionNumber(
        addressesVector, ocl.getHWConfig(), scalarEvolution);

    transactionNumber.push_back(currentTransactionNumber);
  }

  // Decide how to accumulate the results.
  int result = std::accumulate(transactionNumber.begin(),
                               transactionNumber.end(), 0, std::plus<int>());
  return result;
}

//------------------------------------------------------------------------------
int SubscriptAnalysis::getTransactionNumberInLoop(Instruction *inst,
                                                  Value *value,
                                                  const SCEV *tripCount) {
  return getTransactionNumber(inst, value) * resolveTripCount(tripCount);
}

//------------------------------------------------------------------------------
int SubscriptAnalysis::resolveTripCount(const SCEV *tripCount) {
  int DEFAULT_LOOP_TRIP_COUNT = 1024;
  NDRangePoint pointZero;
  SCEVMap processed;
  const SCEV *resolvedCount = replaceInExpr(tripCount, pointZero, processed);
  if (resolvedCount == nullptr) {
    errs() << "WARNING: loop trip count cannot be resolved, defaulting to "
           << DEFAULT_LOOP_TRIP_COUNT << "\n";
    return DEFAULT_LOOP_TRIP_COUNT;
  }

  if (const SCEVConstant *constSCEV = dyn_cast<SCEVConstant>(resolvedCount)) {
    const ConstantInt *value = constSCEV->getValue();
    return value->getValue().roundToDouble();
  } else {
    errs() << "WARNING: loop trip count cannot be resolved, defaulting to "
           << DEFAULT_LOOP_TRIP_COUNT << "\n";
    return DEFAULT_LOOP_TRIP_COUNT;
  }

  return 0;
}

//------------------------------------------------------------------------------
std::vector<const SCEV *> SubscriptAnalysis::analyzeSubscript(Instruction *inst,
                                                              const SCEV *scev,
                                                              Warp &warp) {
  std::vector<const SCEV *> resultVector;
  resultVector.reserve(ocl.getHWConfig().warpSize);

  // Iterate over threads in a warp.
  for (auto iter = warp.begin(), iterEnd = warp.end(); iter != iterEnd;
       ++iter) {
    NDRangePoint point = *iter;

    if (!isOperationExecuted(point, inst))
      continue;

    SCEVMap processed;
    const SCEV *expr = replaceInExpr(scev, point, processed);
    resultVector.push_back(expr);
  }

  return resultVector;
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEV *expr,
                                             const NDRangePoint &point,
                                             SCEVMap &processed) {
  SCEVMap::iterator iter = processed.find(expr);
  if (iter != processed.end()) {
    return processed[expr];
  }

  const SCEV *result = nullptr;

  // FIXME: This is ugly.
  if (const SCEVCommutativeExpr *tmp = dyn_cast<SCEVCommutativeExpr>(expr))
    result = replaceInExpr(tmp, point, processed);
  if (const SCEVConstant *tmp = dyn_cast<SCEVConstant>(expr))
    result = replaceInExpr(tmp, point, processed);
  if (const SCEVUnknown *tmp = dyn_cast<SCEVUnknown>(expr))
    result = replaceInExpr(tmp, point, processed);
  if (const SCEVUDivExpr *tmp = dyn_cast<SCEVUDivExpr>(expr))
    result = replaceInExpr(tmp, point, processed);
  if (const SCEVAddRecExpr *tmp = dyn_cast<SCEVAddRecExpr>(expr))
    result = replaceInExpr(tmp, point, processed);
  if (const SCEVCastExpr *tmp = dyn_cast<SCEVCastExpr>(expr))
    result = replaceInExpr(tmp, point, processed);

  processed[expr] = result;

  return result;
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEVAddRecExpr *expr,
                                             const NDRangePoint &point,
                                             SCEVMap &processed) {
  const SCEV *start = expr->getStart();
  // Check that the step is independent of the TID. TODO.
  return replaceInExpr(start, point, processed);
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEVCommutativeExpr *expr,
                                             const NDRangePoint &point,
                                             SCEVMap &processed) {
  SmallVector<const SCEV *, 8> operands;
  for (auto iter = expr->op_begin(), iterEnd = expr->op_end(); iter != iterEnd;
       ++iter) {
    const SCEV *NewOperand = replaceInExpr(*iter, point, processed);
    if (isa<SCEVCouldNotCompute>(NewOperand))
      return scalarEvolution->getCouldNotCompute();
    operands.push_back(NewOperand);
  }
  const SCEV *result = nullptr;

  if (isa<SCEVAddExpr>(expr))
    result = scalarEvolution->getAddExpr(operands);
  if (isa<SCEVMulExpr>(expr))
    result = scalarEvolution->getMulExpr(operands);
  if (isa<SCEVSMaxExpr>(expr))
    result = scalarEvolution->getSMaxExpr(operands);
  if (isa<SCEVUMaxExpr>(expr))
    result = scalarEvolution->getUMaxExpr(operands);

  return result;
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEVConstant *expr,
                                             const NDRangePoint &, SCEVMap &) {
  return expr;
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEVUnknown *expr,
                                             const NDRangePoint &point,
                                             SCEVMap &processed) {
  Value *value = expr->getValue();
  // Implement actual replacement.
  if (Instruction *instruction = dyn_cast<Instruction>(value)) {

    // Manage binary operations.
    if (BinaryOperator *BinOp = dyn_cast<BinaryOperator>(instruction)) {
      // Modulo.
      if (BinOp->getOpcode() == Instruction::URem) {
        const SCEV *Arg = scalarEvolution->getSCEV(BinOp->getOperand(0));
        const SCEV *Modulo = scalarEvolution->getSCEV(BinOp->getOperand(1));
        const SCEV *Result = scalarEvolution->getMinusSCEV(
            Arg, scalarEvolution->getMulExpr(
                     scalarEvolution->getUDivExpr(Arg, Modulo), Modulo));
        return replaceInExpr(Result, point, processed);
      }

      // Signed division.
      if (BinOp->getOpcode() == Instruction::SDiv) {
        const SCEV *First = scalarEvolution->getSCEV(BinOp->getOperand(0));
        const SCEV *Second = scalarEvolution->getSCEV(BinOp->getOperand(1));
        const SCEV *Div = scalarEvolution->getUDivExpr(First, Second);
        return replaceInExpr(Div, point, processed);
      }

      // All the rest.
      return scalarEvolution->getCouldNotCompute();
    }

    // Manage casts.
    if (isOpenCLIntCast(instruction)) {
      CallInst *Call = dyn_cast<CallInst>(instruction);
      const SCEV *ArgSCEV = scalarEvolution->getSCEV(Call->getArgOperand(0));
      return replaceInExpr(ArgSCEV, point, processed);
    }

    // Manage phi nodes.
    if (PHINode *phi = dyn_cast<PHINode>(value))
      return replaceInPhi(phi, point, processed);

    return resolveInstruction(instruction, point);
  }

  // If the value is a function argument query OCL.
  if (isa<Argument>(value) && value->getType()->isIntegerTy())
    return scalarEvolution->getConstant(APInt(32, ocl.resolveValue(value)));

  return expr;
}

//------------------------------------------------------------------------------
const SCEV *
SubscriptAnalysis::resolveInstruction(llvm::Instruction *instruction,
                                      const NDRangePoint &point) {
  const NDRange *ndr = ocl.getNDRange();
  const NDRangeSpace *ndrSpace = ocl.getNDRangeSpace();
  std::string type = ndr->getType(instruction);

  // Check if the instruction is a coordinate querying the NDRangePoint class.
  if (ndr->isCoordinate(instruction)) {
    int direction = ndr->getDirection(instruction);
    int coordinate = point.getCoordinate(type, direction);
    return scalarEvolution->getConstant(APInt(32, coordinate));
  }

  // Check if the instruction is a size querying the NDRange class.
  if (ndr->isSize(instruction)) {
    int direction = ndr->getDirection(instruction);
    int size = ndrSpace->getSize(type, direction);
    return scalarEvolution->getConstant(APInt(32, size));
  }

  // If the instruction is neither a coordinate nor a size return
  // CouldNotCompute.
  return scalarEvolution->getCouldNotCompute();
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEVUDivExpr *expr,
                                             const NDRangePoint &point,
                                             SCEVMap &processed) {
  const SCEV *newLHS = replaceInExpr(expr->getLHS(), point, processed);
  if (isa<SCEVCouldNotCompute>(newLHS))
    return scalarEvolution->getCouldNotCompute();
  const SCEV *newRHS = replaceInExpr(expr->getRHS(), point, processed);
  if (isa<SCEVCouldNotCompute>(newRHS))
    return scalarEvolution->getCouldNotCompute();

  return scalarEvolution->getUDivExpr(newLHS, newRHS);
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInExpr(const SCEVCastExpr *expr,
                                             const NDRangePoint &point,
                                             SCEVMap &processed) {
  return replaceInExpr(expr->getOperand(), point, processed);
}

//------------------------------------------------------------------------------
const SCEV *SubscriptAnalysis::replaceInPhi(PHINode *Phi,
                                            const NDRangePoint &point,
                                            SCEVMap &processed) {
  // FIXME: Pick the first argument of the phi node.
  Value *param = Phi->getIncomingValue(0);
  assert(scalarEvolution->isSCEVable(param->getType()) &&
         "PhiNode argument non-SCEVable");

  const SCEV *scev = scalarEvolution->getSCEV(param);

  processed[scalarEvolution->getSCEV(Phi)] = scev;

  return replaceInExpr(scev, point, processed);
}

//------------------------------------------------------------------------------
bool SubscriptAnalysis::isOperationExecuted(const NDRangePoint &point,
                                            Instruction *inst) {
  BasicBlock *block = inst->getParent();
  std::vector<BlockCondition> conditions = blockMask.getConditions(block);

  bool result = true;
  for (auto &condition : conditions) {
    SCEVMap processed;
    const SCEV *resolvedCondition =
        replaceInExpr(condition.getSCEV(), point, processed);
    bool tmpResult = BlockCondition::getBooleanValue(
        resolvedCondition, condition.getPredicate(), scalarEvolution);
    result &= tmpResult;
  }
  return result;
}

//------------------------------------------------------------------------------
int getTypeWidth(const Type *type) {
  assert(type->isPointerTy() && "Type is not a pointer");
  const Type *pointedType = type->getPointerElementType();
  int result = pointedType->getPrimitiveSizeInBits();
  if (result == 0) {
    return 32;
  }
  return result / 8;
}
