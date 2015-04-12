#include "gtest.h"

#include "SymEngine/YAMLReader.h"

using namespace SymEngine;

TEST(OCLEnvTest, HardwareConfigTest) {
  HardwareConfig hwConfig = readHardwareConfig("test_hw_config.yaml"); 
  EXPECT_EQ(hwConfig.banksNumber, 42);
  EXPECT_EQ(hwConfig.bankWidth, 43);
  EXPECT_EQ(hwConfig.cacheLineSize, 44);
  EXPECT_EQ(hwConfig.warpSize, 45);
}

TEST(OCLEnvTest, KernelArgumentsTest) {
  KernelArgumentsVector argVector = readKernelArguments("test_kernel_arg.yaml");
  EXPECT_EQ(argVector.size(), 3);

  KernelArguments firstKernel = argVector[0];
  EXPECT_TRUE(firstKernel.kernelName == "testKernel0");
  EXPECT_EQ(firstKernel.argValues.size(), 0);

  KernelArguments secondKernel = argVector[1];
  EXPECT_TRUE(secondKernel.kernelName == "testKernel1");
  EXPECT_EQ(secondKernel.argValues.size(), 1);
  EXPECT_EQ(secondKernel.argValues[0], 123);

  KernelArguments thirdKernel = argVector[2];
  EXPECT_TRUE(thirdKernel.kernelName == "testKernel2");
  EXPECT_EQ(thirdKernel.argValues.size(), 2);
  EXPECT_EQ(thirdKernel.argValues[0], 321);
  EXPECT_EQ(thirdKernel.argValues[1], 654);
}

TEST(OCLEnvTest, OpenCLConfig) {
  OpenCLConfig openclConfig = readOpenCLConfig("test_config_opencl.yaml");
  NDRangeStruct ndRange = openclConfig.ndRange; 
  WarpStruct warp = openclConfig.warp;

  EXPECT_EQ(ndRange.localSize.size(), 3);
  EXPECT_EQ(ndRange.localSize[0], 1024);
  EXPECT_EQ(ndRange.localSize[1], 1025);
  EXPECT_EQ(ndRange.localSize[2], 1026);
   
  EXPECT_EQ(ndRange.numberOfGroups.size(), 3);
  EXPECT_EQ(ndRange.numberOfGroups[0], 1027);
  EXPECT_EQ(ndRange.numberOfGroups[1], 1028);
  EXPECT_EQ(ndRange.numberOfGroups[2], 1029);
   
  EXPECT_EQ(warp.group.size(), 3); 
  EXPECT_EQ(warp.group[0], 7); 
  EXPECT_EQ(warp.group[1], 8); 
  EXPECT_EQ(warp.group[2], 9); 
  EXPECT_EQ(warp.warpIndex, 42); 
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
