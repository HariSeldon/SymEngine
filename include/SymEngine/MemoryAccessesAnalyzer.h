#ifndef MEMORY_ACCESSES_ANALYZER_H
#define MEMORY_ACCESSES_ANALYZER_H

#include "llvm/Analysis/ScalarEvolution.h"

#include <vector>

namespace SymEngine {

struct HardwareConfig;

int computeTransactionNumberImpl(const std::vector<int> indices,
                                 const SymEngine::HardwareConfig &hwConfig);
int computeTransactionNumber(const std::vector<const llvm::SCEV *> &scevs,
                             const SymEngine::HardwareConfig &hwConfig,
                             llvm::ScalarEvolution *se);

int computeBankConflictNumberImpl(const std::vector<int> indices,
                                  const SymEngine::HardwareConfig &hwConfig);
int computeBankConflictNumber(const std::vector<const llvm::SCEV *> &scevs,
                              const SymEngine::HardwareConfig &hwConfig,
                              llvm::ScalarEvolution *se);

}

#endif
