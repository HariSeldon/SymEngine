#ifndef NDRANGE_H
#define NDRANGE_H

#include "SymEngine/Utils.h"

#include "llvm/Pass.h"

#include <map>

namespace llvm {
class Function;
}

namespace SymEngine {

class NDRange : public llvm::FunctionPass {
  void operator=(const NDRange &);
  NDRange(const NDRange &);

public:
  static char ID;
  NDRange();

  virtual bool runOnFunction(llvm::Function &function);
  virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;

public:
  InstVector getTids();
  InstVector getSizes();
  InstVector getTids(int direction);
  InstVector getSizes(int direction);

  bool isTid(llvm::Instruction *inst);
  bool isTidInDirection(llvm::Instruction *inst, int direction);
  std::string getType(llvm::Instruction *inst) const;
  bool isCoordinate(llvm::Instruction *inst) const;
  bool isSize(llvm::Instruction *inst) const;

  int getDirection(llvm::Instruction *inst) const;

  bool isGlobal(llvm::Instruction *inst) const;
  bool isLocal(llvm::Instruction *inst) const;
  bool isGlobalSize(llvm::Instruction *inst) const;
  bool isLocalSize(llvm::Instruction *inst) const;
  bool isGroupId(llvm::Instruction *inst) const;
  bool isGroupsNum(llvm::Instruction *inst) const;

  bool isGlobal(llvm::Instruction *inst, int direction) const;
  bool isLocal(llvm::Instruction *inst, int direction) const;
  bool isGlobalSize(llvm::Instruction *inst, int direction) const;
  bool isLocalSize(llvm::Instruction *inst, int direction) const;
  bool isGroupId(llvm::Instruction *inst, int direction) const;
  bool isGroupsNum(llvm::Instruction *inst, int dimension) const;

  void dump();

  // These getters are for testing only.
  const InstVector &getGlobalIds(int direction) const;
  const InstVector &getLocalIds(int direction) const;

public:
  static std::string GET_GLOBAL_ID;
  static std::string GET_LOCAL_ID;
  static std::string GET_GLOBAL_SIZE;
  static std::string GET_LOCAL_SIZE;
  static std::string GET_GROUP_ID;
  static std::string GET_GROUPS_NUMBER;
  static constexpr int DIRECTION_NUMBER = 3;

private:
  void init();
  bool isPresentInDirection(llvm::Instruction *inst,
                            const std::string &functionName,
                            int direction) const;
  void findOpenCLFunctionCallsByNameAllDirs(std::string calleeName,
                                            llvm::Function *caller);

private:
  std::vector<std::map<std::string, InstVector>> oclInsts;
};

// Non-member functions.
void findOpenCLFunctionCallsByName(std::string calleeName,
                                   llvm::Function *caller, int dimension,
                                   InstVector &target);
void findOpenCLFunctionCalls(llvm::Function *callee, llvm::Function *caller,
                             int dimension, InstVector &target);
}

#endif
