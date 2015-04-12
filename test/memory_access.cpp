#include "gtest.h"

#include "SymEngine/HardwareConfig.h"
#include "SymEngine/MemoryAccessesAnalyzer.h"

#include <numeric>

using namespace llvm;
using namespace SymEngine;

class MemoryAccessTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    hwConfig.banksNumber = 32;
    hwConfig.bankWidth = 4;
    hwConfig.warpSize = 8;
    hwConfig.cacheLineSize = 32;
  }

HardwareConfig hwConfig;
};

TEST_F(MemoryAccessTest, CoalescedTest) {
  std::vector<int> coalescedAccesses(8);
  std::iota(coalescedAccesses.begin(), coalescedAccesses.end(), 0);
  std::transform(coalescedAccesses.begin(), coalescedAccesses.end(),
                 coalescedAccesses.begin(), [](int x) { return x * 4; });

  int transactions = computeTransactionNumberImpl(coalescedAccesses,
                                                  hwConfig);  
  EXPECT_EQ(transactions, 1);
}

TEST_F(MemoryAccessTest, OneTransaction) {
  std::vector<int> coalescedAccesses(8, 7);
  int transactions = computeTransactionNumberImpl(coalescedAccesses,
                                                  hwConfig);  
  EXPECT_EQ(transactions, 1);
}

TEST_F(MemoryAccessTest, TwoTransactions) {
  std::vector<int> coalescedAccesses = {0, 8, 16, 24, 32, 40, 48, 56};
  int transactions = computeTransactionNumberImpl(coalescedAccesses,
                                                  hwConfig);  
  EXPECT_EQ(transactions, 2);
}

TEST_F(MemoryAccessTest, ThreeTransactions) {
  std::vector<int> coalescedAccesses = {0, 12, 24, 36, 48, 60, 72, 84};
  int transactions = computeTransactionNumberImpl(coalescedAccesses,
                                                  hwConfig);  
  EXPECT_EQ(transactions, 3);
}

TEST_F(MemoryAccessTest, EigthTransactions) {
  std::vector<int> coalescedAccesses(8);
  std::iota(coalescedAccesses.begin(), coalescedAccesses.end(), 0);
  std::transform(coalescedAccesses.begin(), coalescedAccesses.end(),
                 coalescedAccesses.begin(), [](int x) { return x * 32; });
  int transactions = computeTransactionNumberImpl(coalescedAccesses,
                                                  hwConfig);  
  EXPECT_EQ(transactions, 8);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
