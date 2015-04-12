#ifndef YAML_READER_H
#define YAML_READER_H

#include "SymEngine/HardwareConfig.h"

#include "llvm/Support/YAMLTraits.h"

using namespace llvm;
using yaml::MappingTraits;
using yaml::SequenceTraits;

// -----------------------------------------------------------------------------

namespace SymEngine {

struct KernelArguments {
  std::string kernelName;
  std::vector<int> argValues;
};

typedef std::vector<SymEngine::KernelArguments> KernelArgumentsVector;

}

LLVM_YAML_IS_SEQUENCE_VECTOR(SymEngine::KernelArguments)

// -----------------------------------------------------------------------------

namespace SymEngine {

struct NDRangeStruct {
  std::vector<int> localSize;
  std::vector<int> numberOfGroups;
};

struct WarpStruct {
  std::vector<int> group;
  int warpIndex;
};

struct OpenCLConfig {
  NDRangeStruct ndRange;
  WarpStruct warp;
};

}

namespace llvm {
namespace yaml {

template <> struct MappingTraits<SymEngine::KernelArguments> {
  static void mapping(yaml::IO &io, SymEngine::KernelArguments &kernelArgs) {
    io.mapRequired("kernelName", kernelArgs.kernelName);
    io.mapRequired("args", kernelArgs.argValues);
  }
};

template <> struct MappingTraits<SymEngine::HardwareConfig> {
  static void mapping(yaml::IO &io, SymEngine::HardwareConfig &hwConfig) {
    io.mapRequired("local_memory_bank_number", hwConfig.banksNumber);
    io.mapRequired("local_memory_bank_width", hwConfig.bankWidth);
    io.mapRequired("warp_size", hwConfig.warpSize);
    io.mapRequired("cache_line_size", hwConfig.cacheLineSize);
  }
};

template <> struct MappingTraits<SymEngine::NDRangeStruct> {
  static void mapping(yaml::IO &io, SymEngine::NDRangeStruct &ndRange) {
    io.mapRequired("localSize", ndRange.localSize);
    io.mapRequired("numberOfGroups", ndRange.numberOfGroups);
  }
};

template <> struct MappingTraits<SymEngine::WarpStruct> {
  static void mapping(yaml::IO &io, SymEngine::WarpStruct &warp) {
    io.mapRequired("group", warp.group);
    io.mapRequired("warpIndex", warp.warpIndex);
  }
};

template <> struct MappingTraits<SymEngine::OpenCLConfig> {
  static void mapping(yaml::IO &io, SymEngine::OpenCLConfig &openclConfig) {
    io.mapRequired("ndRange", openclConfig.ndRange);
    io.mapRequired("warp", openclConfig.warp);
  }
};

// Sequence of ints.
template <> struct SequenceTraits<std::vector<int>> {
  static size_t size(yaml::IO &, std::vector<int> &seq) { return seq.size(); }
  static int &element(yaml::IO &, std::vector<int> &seq, size_t index) {
    if (index >= seq.size())
      seq.resize(index + 1);
    return seq[index];
  }

  static const bool flow = true;
};
}
}

SymEngine::HardwareConfig readHardwareConfig(const std::string &fileName);
SymEngine::KernelArgumentsVector readKernelArguments(const std::string &fileName);
SymEngine::OpenCLConfig readOpenCLConfig(const std::string &fileName);

#endif
