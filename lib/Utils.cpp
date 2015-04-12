#include "SymEngine/Utils.h"

#include "llvm/Analysis/LoopInfo.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/raw_ostream.h"

#include <fstream>

using namespace llvm;

//------------------------------------------------------------------------------
bool isInLoop(const Instruction *inst, LoopInfo *loopInfo) {
  const BasicBlock *block = inst->getParent();
  return loopInfo->getLoopFor(block) != nullptr;
}

//------------------------------------------------------------------------------
bool isInLoop(const BasicBlock *block, LoopInfo *loopInfo) {
  return loopInfo->getLoopFor(block) != nullptr;
}

//------------------------------------------------------------------------------
template <class T>
bool isPresent(const T *value, const std::vector<T *> &values) {
  auto result = std::find(values.begin(), values.end(), value);
  return result != values.end();
}

// Explict template instantiation.
template bool isPresent(const Instruction *value, const InstVector &values);
template bool isPresent(const BasicBlock *block, const BlockVector &values);

//------------------------------------------------------------------------------
template <class type> void dumpVector(const std::vector<type *> &toDump) {
  errs() << "Size: " << toDump.size() << "\n";
  for (auto element : toDump)
    element->dump();
}

template void dumpVector(const InstVector &toDump);

//------------------------------------------------------------------------------
Function *getOpenCLFunctionByName(std::string calleeName, Function *caller) {
  Module &module = *caller->getParent();
  Function *callee = module.getFunction(calleeName);

  if (callee == nullptr)
    return nullptr;

  assert(callee->arg_size() == 1 && "Wrong OpenCL function");
  return callee;
}

//------------------------------------------------------------------------------
bool isOpenCLIntCast(Instruction *inst) {
  if (CallInst *call = dyn_cast<CallInst>(inst)) {
    Function *callee = call->getCalledFunction();
    std::string name = callee->getName();
    bool begin = (name[0] == '_' && name[1] == 'Z');
    bool value = ((name.find("as_uint") != std::string::npos) ||
                  (name.find("as_int") != std::string::npos));
    return begin && value;
  }
  return false;
}

// -----------------------------------------------------------------------------
std::string readStream(std::ifstream &fileStream) {
  std::string text;

  // Get file size.
  fileStream.seekg(0, std::ios::end);
  unsigned long int fileSize = fileStream.tellg();

  // Allocate buffer.
  text.resize(fileSize + 1);

  // Rewind.
  fileStream.seekg(0, std::ios::beg);

  // Read.
  fileStream.read(&text[0], fileSize);
  fileStream.close();

  text[fileSize] = '\0';
  return text;
}

// -----------------------------------------------------------------------------
std::string readFile(const std::string &filePath) {
  std::ifstream fileStream(filePath.c_str());
  if (fileStream.is_open()) {
    return readStream(fileStream);
  } else {
    throw std::runtime_error("Cannot open: " + filePath);
  }
}
