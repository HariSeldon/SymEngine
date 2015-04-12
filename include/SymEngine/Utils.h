#ifndef UTILS_H
#define UTILS_H

#include <set>
#include <string>
#include <vector>

namespace llvm {
class BasicBlock;
class Instruction;
class Function;
class Loop;
class LoopInfo;
class ScalarEvolution;
}

typedef std::vector<llvm::Instruction *> InstVector;
typedef std::set<llvm::Instruction *> InstSet;
typedef std::vector<llvm::BasicBlock *> BlockVector;
typedef std::set<llvm::BasicBlock *> BlockSet;
typedef std::pair<llvm::BasicBlock *, bool> BlockBranchPair;

template <class T>
bool isPresent(const T *value, const std::vector<T *> &vector);

template <class type> void dumpVector(const std::vector<type *> &toDump);

llvm::Function *getOpenCLFunctionByName(std::string calleeName,
                                        llvm::Function *caller);

bool isOpenCLIntCast(llvm::Instruction *inst);

std::string readFile(const std::string &filePath);

#endif
