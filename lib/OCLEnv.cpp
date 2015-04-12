#include "SymEngine/OCLEnv.h"

#include "SymEngine/NDRange.h"
#include "SymEngine/NDRangeSpace.h"
#include "SymEngine/Utils.h"
#include "SymEngine/YAMLReader.h"
#include "SymEngine/Warp.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"

#include "llvm/Support/CommandLine.h"

#include <algorithm>

using namespace llvm;
using namespace SymEngine;

cl::opt<int> localSizeX("localSizeX", cl::init(0), cl::Hidden,
                        cl::desc("localSizeX for symbolic execution"));
cl::opt<int> localSizeY("localSizeY", cl::init(0), cl::Hidden,
                        cl::desc("localSizeY for symbolic execution"));
cl::opt<int> localSizeZ("localSizeZ", cl::init(0), cl::Hidden,
                        cl::desc("localSizeZ for symbolic execution"));

cl::opt<int>
    numberOfGroupsX("numberOfGroupsX", cl::init(0), cl::Hidden,
                    cl::desc("numberOfGroupsX for symbolic execution"));
cl::opt<int>
    numberOfGroupsY("numberOfGroupsY", cl::init(0), cl::Hidden,
                    cl::desc("numberOfGroupsY for symbolic execution"));
cl::opt<int>
    numberOfGroupsZ("numberOfGroupsZ", cl::init(0), cl::Hidden,
                    cl::desc("numberOfGroupsZ for symbolic execution"));

cl::opt<bool> fullSimulation("full-simulation", cl::init(false),
                              cl::Hidden,
                              cl::desc("Enable simulation of all warps"));

// -----------------------------------------------------------------------------
const std::string OCLEnv::KERNEL_ARGUMENTS_FILE_NAME = "kernel_arg_config.yaml";
const std::string OCLEnv::HARDWARE_CONFIG_FILE_NAME = "hardware_config.yaml";
const std::string OCLEnv::OPENCL_CONFIG_FILE_NAME = "opencl_config.yaml";
const int OCLEnv::UNKNOWN_MEMORY_LOCATION = -1;
const unsigned int OCLEnv::LOCAL_AS = 3;

// -----------------------------------------------------------------------------
OCLEnv::OCLEnv(Function &function, const NDRange *ndRange) : ndRange(ndRange) {
  setupHWConfig();
  setupKernelArgs(function);
  setupOpenCLConfig();
}

// -----------------------------------------------------------------------------
OCLEnv::OCLEnv(Function &function, const NDRange *ndRange,
               HardwareConfig hwConfig)
    : ndRange(ndRange), hwConfig(hwConfig) {
  setupKernelArgs(function);
  setupOpenCLConfig();
}

// -----------------------------------------------------------------------------
OCLEnv::OCLEnv(OCLEnv &&oclEnv) {
  std::swap(this->ndRange, oclEnv.ndRange);
  std::swap(this->warps, oclEnv.warps);
  std::swap(this->ndRangeSpace, oclEnv.ndRangeSpace);
  std::swap(this->argumentMap, oclEnv.argumentMap);
  std::swap(this->hwConfig, oclEnv.hwConfig);
}

// -----------------------------------------------------------------------------
void OCLEnv::setupHWConfig() {
  hwConfig = readHardwareConfig(HARDWARE_CONFIG_FILE_NAME);
}

// -----------------------------------------------------------------------------
void OCLEnv::setupKernelArgs(Function &function) {
  KernelArgumentsVector kernelArgs =
      readKernelArguments(KERNEL_ARGUMENTS_FILE_NAME);

  std::string kernelName = function.getName();

  auto args = std::find_if(kernelArgs.begin(), kernelArgs.end(),
                           [kernelName](const KernelArguments &x) {
    return x.kernelName == kernelName;
  });

  if (args == kernelArgs.end()) {
    errs() << "Argument values not present for kernel: " << kernelName << "\n";
    exit(1);
  }

  auto &argValues = args->argValues;
  int argCounter = 0;
  // Go through the function arguements and setup the map.
  for (Function::arg_iterator iter = function.arg_begin(),
                              iterEnd = function.arg_end();
       iter != iterEnd; ++iter) {
    llvm::Value *argument = iter;
    llvm::Type *type = argument->getType();
    // Only set the value of the argument if it is an integer.
    if (type->isIntegerTy()) {
      int argValue = argValues[argCounter++];
      argumentMap.insert(std::make_pair(argument, argValue));
    }
  }
}

// -----------------------------------------------------------------------------
void OCLEnv::setupOpenCLConfig() {
  OpenCLConfig openclConfig = readOpenCLConfig(OPENCL_CONFIG_FILE_NAME);

  // If the user does not specify a size paramter, then use the default from the
  // configuration file.
  if (localSizeX == 0)
    localSizeX = openclConfig.ndRange.localSize[0];
  if (localSizeY == 0)
    localSizeY = openclConfig.ndRange.localSize[1];
  if (localSizeZ == 0)
    localSizeZ = openclConfig.ndRange.localSize[2];

  if (numberOfGroupsX == 0)
    numberOfGroupsX = openclConfig.ndRange.numberOfGroups[0];
  if (numberOfGroupsY == 0)
    numberOfGroupsY = openclConfig.ndRange.numberOfGroups[1];
  if (numberOfGroupsZ == 0)
    numberOfGroupsZ = openclConfig.ndRange.numberOfGroups[2];

  ndRangeSpace = std::unique_ptr<NDRangeSpace>(
      new NDRangeSpace(localSizeX, localSizeY, localSizeZ, numberOfGroupsX,
                       numberOfGroupsY, numberOfGroupsZ));

  WarpFactory warpFactory(ndRangeSpace.get(), hwConfig.warpSize);
  if (fullSimulation)
    warps = warpFactory.createAllWarpsInGroup(openclConfig.warp.group[0],
                                              openclConfig.warp.group[1],
                                              openclConfig.warp.group[2]);
  else
    warps.push_back(warpFactory.createWarp(
        openclConfig.warp.group[0], openclConfig.warp.group[1],
        openclConfig.warp.group[2], openclConfig.warp.warpIndex));
}

// -----------------------------------------------------------------------------
const NDRange *OCLEnv::getNDRange() const { return ndRange; }

// -----------------------------------------------------------------------------
const NDRangeSpace *OCLEnv::getNDRangeSpace() const {
  return ndRangeSpace.get();
}

// -----------------------------------------------------------------------------
const HardwareConfig OCLEnv::getHWConfig() const {
  return hwConfig;
}

// -----------------------------------------------------------------------------
std::vector<Warp> OCLEnv::getWarps() const {
  return warps;
}

// -----------------------------------------------------------------------------
int OCLEnv::resolveValue(llvm::Value *value) const {
  std::map<llvm::Value *, int>::const_iterator iter = argumentMap.find(value);
  assert(iter != argumentMap.end() && "Argument is not in argument map!");
  return iter->second;
}
