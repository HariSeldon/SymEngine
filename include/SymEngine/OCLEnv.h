#ifndef OCL_ENV_H
#define OCL_ENV_H

#include "SymEngine/HardwareConfig.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace llvm {
class Function;
class Value; 
}

namespace SymEngine {

class NDRange;
class NDRangeSpace;
class Warp;

class OCLEnv {

public:
  static const std::string KERNEL_ARGUMENTS_FILE_NAME;
  static const std::string HARDWARE_CONFIG_FILE_NAME;
  static const std::string OPENCL_CONFIG_FILE_NAME;
  static const int UNKNOWN_MEMORY_LOCATION;
  static const unsigned int LOCAL_AS;

public:
  OCLEnv(llvm::Function &function, const NDRange *ndRange);
  OCLEnv(llvm::Function &function, const NDRange *ndRange,
         SymEngine::HardwareConfig hwConfig);
  OCLEnv(OCLEnv &&oclEnv);

public:
  const NDRange *getNDRange() const;
  const NDRangeSpace *getNDRangeSpace() const;
  int resolveValue(llvm::Value *) const;

  const SymEngine::HardwareConfig getHWConfig() const;
  void setHWConfig(SymEngine::HardwareConfig hwConfig);

  std::vector<Warp> getWarps() const;
    
private:
  void setupHWConfig();
  void setupKernelArgs(llvm::Function &function);
  void setupOpenCLConfig();

private:
  // ndRange is not a owning ptr.
  const NDRange *ndRange;
  std::vector<Warp> warps;
  std::unique_ptr<NDRangeSpace> ndRangeSpace;
  std::map<llvm::Value *, int> argumentMap;

  SymEngine::HardwareConfig hwConfig; 
};

}

#endif
