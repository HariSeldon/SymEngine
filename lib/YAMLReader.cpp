#include "SymEngine/YAMLReader.h"

#include "SymEngine/Utils.h"

using namespace SymEngine;
using yaml::Input;

// -----------------------------------------------------------------------------
HardwareConfig readHardwareConfig(const std::string &fileName) {
  HardwareConfig hwConfig;
  std::string fileContent = readFile(fileName);

  Input yin(fileContent);
  yin >> hwConfig;

  if (yin.error()) {
    errs() << "Error reading the hardware configuration file.\n";
    exit(1);
  }
  return hwConfig;
}

// -----------------------------------------------------------------------------
KernelArgumentsVector readKernelArguments(const std::string &fileName) {
  KernelArgumentsVector kernelArgs;
  std::string fileContent = readFile(fileName);
  Input yin(fileContent);
  yin >> kernelArgs;

  if (yin.error()) {
    errs() << "Error reading the kernel arguments input file.\n";
    exit(1);
  }

  return kernelArgs;
}

// -----------------------------------------------------------------------------
OpenCLConfig readOpenCLConfig(const std::string &fileName) {
  OpenCLConfig openclConfig;
  std::string fileContent = readFile(fileName);

  Input yin(fileContent);
  yin >> openclConfig;

  if (yin.error()) {
    errs() << "Error reading the opencl config input file.\n";
    exit(1);
  }

  return openclConfig;
}
