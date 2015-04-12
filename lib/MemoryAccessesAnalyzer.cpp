#include "SymEngine/MemoryAccessesAnalyzer.h"

#include "SymEngine/HardwareConfig.h"
#include "SymEngine/OCLEnv.h"

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

using namespace llvm;
using namespace SymEngine;

//------------------------------------------------------------------------------
const SCEVUnknown *getUnknownSCEV(const SCEV *scev) {
  if (auto unknown = dyn_cast<SCEVUnknown>(scev)) {
    return unknown;
  }

  if (auto add = dyn_cast<SCEVAddExpr>(scev)) {
    if (add->getNumOperands() != 2)
      return nullptr;

    if (isa<SCEVConstant>(add->getOperand(0)) &&
        isa<SCEVUnknown>(add->getOperand(1))) {
      return dyn_cast<SCEVUnknown>(add->getOperand(1));
    }

    if (isa<SCEVConstant>(add->getOperand(1)) &&
        isa<SCEVUnknown>(add->getOperand(0))) {
      return dyn_cast<SCEVUnknown>(add->getOperand(0));
    }

    return nullptr;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
bool verifyUnknown(const SCEV *scev, const SCEV *unknown) {
  if (const SCEVAddExpr *add = dyn_cast<SCEVAddExpr>(scev)) {
    return std::find(add->op_begin(), add->op_end(), unknown) != add->op_end();
  }
  return false;
}

bool verifyUnknown(const std::vector<const SCEV *> &scevs,
                   const SCEV *unknown) {
  return std::accumulate(scevs.begin(), scevs.end(), true,
                         [unknown](bool result, const SCEV *scev) {
    return result & verifyUnknown(scev, unknown);
  });
}

//------------------------------------------------------------------------------
std::vector<int> getMemoryOffsets(std::vector<const SCEV *> scevs,
                                  const SCEV *unknown, ScalarEvolution *se) {
  std::vector<const SCEV *> offsets;
  std::transform(scevs.begin(), scevs.end(), std::back_inserter(offsets),
                 [se, unknown](const SCEV *scev) {
    return se->getMinusSCEV(scev, unknown);
  });

  assert(offsets.size() == scevs.size() && "Wrong number of offsets");

  std::vector<int> indices;
  std::transform(offsets.begin(), offsets.end(), std::back_inserter(indices),
                 [](const SCEV *scev) {
    if (const SCEVAddRecExpr *AddRecSCEV = dyn_cast<SCEVAddRecExpr>(scev)) {
      scev = AddRecSCEV->getStart();
    }
    if (const SCEVConstant *ConstSCEV = dyn_cast<SCEVConstant>(scev)) {
      const ConstantInt *value = ConstSCEV->getValue();
      return static_cast<int>(value->getValue().roundToDouble());
    } else {
      // The SCEV is not constant. I don't know which element is accessed.
      return OCLEnv::UNKNOWN_MEMORY_LOCATION;
    }
  });

  assert(indices.size() == scevs.size() && "Wrong number of indices");
  return indices;
}

//------------------------------------------------------------------------------
std::vector<int> getRelativeAddresses(const std::vector<const SCEV *> &scevs,
                                      const HardwareConfig hwConfig,
                                      ScalarEvolution *scalarEvolution) {
  const SCEV *first = scevs[0];
  const SCEV *unknown = getUnknownSCEV(first);
  assert(unknown != nullptr);

  verifyUnknown(scevs, unknown);

  std::vector<int> indices = getMemoryOffsets(scevs, unknown, scalarEvolution);

  // Check that all memory locations are known.
  auto unknownMemoryLocationPosition = std::find(
      indices.begin(), indices.end(), OCLEnv::UNKNOWN_MEMORY_LOCATION);
  assert(unknownMemoryLocationPosition == indices.end());

  return indices;
}

//------------------------------------------------------------------------------
int SymEngine::computeBankConflictNumberImpl(std::vector<int> indices,
                                  const HardwareConfig &hwConfig) {

  std::vector<int> rows;
  std::vector<int> columns;

  rows.reserve(indices.size());
  columns.reserve(indices.size());

  const int banksNumber = hwConfig.banksNumber; 

  std::transform(indices.begin(), indices.end(), std::back_inserter(columns),
                 [banksNumber](int index) { return index % banksNumber; });
  const int LOCAL_MEMORY_WIDTH = hwConfig.banksNumber * hwConfig.bankWidth;
  std::transform(
      indices.begin(), indices.end(), std::back_inserter(rows),
      [LOCAL_MEMORY_WIDTH](int x) { return x / LOCAL_MEMORY_WIDTH; });

  assert(rows.size() == columns.size() && "Rows and Columns don't match");

  std::map<int, std::vector<int>> localMemory;

  for (int index = 0; index < hwConfig.warpSize; ++index) {
    int row = rows[index];
    int column = columns[index];

    localMemory[column].push_back(row);
  }

  int conflictNumber = 0;

  for (auto &iter : localMemory) {
    auto &currentRows = iter.second;
    std::sort(currentRows.begin(), currentRows.end());

    auto uniqueEnd = std::unique(currentRows.begin(), currentRows.end());

    int uniqueRows = std::distance(currentRows.begin(), uniqueEnd);
    --uniqueRows;

    conflictNumber = std::max(conflictNumber, uniqueRows);
  }

  return conflictNumber;
}

//------------------------------------------------------------------------------
int SymEngine::computeBankConflictNumber(const std::vector<const SCEV *> &scevs,
                              const HardwareConfig &hwConfig,
                              ScalarEvolution *scalarEvolution) {
  auto indices = getRelativeAddresses(scevs, hwConfig, scalarEvolution);
  return SymEngine::computeBankConflictNumberImpl(indices, hwConfig);
}

//------------------------------------------------------------------------------
int SymEngine::computeTransactionNumberImpl(std::vector<int> indices,
                                 const HardwareConfig &hwConfig) {
  int cacheLineSize = hwConfig.cacheLineSize;

  std::transform(indices.begin(), indices.end(), indices.begin(),
                 [cacheLineSize](int x) { return x / cacheLineSize; });

  std::sort(indices.begin(), indices.end());
  auto uniqueEnd = std::unique(indices.begin(), indices.end());
  int uniqueCacheLines = std::distance(indices.begin(), uniqueEnd);

  return uniqueCacheLines;
}

//------------------------------------------------------------------------------
int SymEngine::computeTransactionNumber(const std::vector<const SCEV *> &scevs,
                             const HardwareConfig &hwConfig,
                             ScalarEvolution *scalarEvolution) {
  auto indices = getRelativeAddresses(scevs, hwConfig, scalarEvolution);
  return SymEngine::computeTransactionNumberImpl(indices, hwConfig);
}
