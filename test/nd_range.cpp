#include "gtest.h"

#include "SymEngine/NDRange.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "llvm/IRReader/IRReader.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;
using namespace SymEngine;

class NDRangeTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    std::string inputFile = "nd_range_test.ll"; 
    SMDiagnostic error;
    LLVMContext &context = getGlobalContext();
    module = parseIRFile(inputFile, error, context);
    if (!module) {
      error.print("NDRangeTest SetUp", errs());
      return;
    }
  }

  virtual ~NDRangeTest() {}

std::unique_ptr<Module> module;

};

TEST_F(NDRangeTest, TestGlobalIds) {
  NDRange *ndRangePass = new NDRange();
  Function *testFunction = module->getFunction("testFunction");
  EXPECT_NE(testFunction, nullptr);

  ndRangePass->runOnFunction(*testFunction);

  const auto globalIds0 = ndRangePass->getGlobalIds(0);
  EXPECT_EQ(globalIds0.size(), 1);
  for (auto x : globalIds0) {
    EXPECT_TRUE(isa<CallInst>(x));
    CallInst *call = dyn_cast<CallInst>(x);
    Function *callee = call->getCalledFunction();
    EXPECT_TRUE(callee->getName() == "get_global_id"); 
  }

  const auto globalIds1 = ndRangePass->getGlobalIds(1);
  EXPECT_EQ(globalIds1.size(), 1);
  for (auto x : globalIds0) {
    EXPECT_TRUE(isa<CallInst>(x));
    CallInst *call = dyn_cast<CallInst>(x);
    Function *callee = call->getCalledFunction();
    EXPECT_TRUE(callee->getName() == "get_global_id"); 
  }

  const auto globalIds2 = ndRangePass->getGlobalIds(2); 
  EXPECT_EQ(globalIds2.size(), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
