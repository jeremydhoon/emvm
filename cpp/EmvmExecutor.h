// EMVM

#pragma once


#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITMemoryManager.h"

namespace emvm {

class EmvmBuilder;

class EmvmExecutor {
 public:
  typedef std::vector<llvm::GenericValue> ArgVector;

  llvm::GenericValue run(const std::string& funcName, const ArgVector& args);
  llvm::GenericValue run(llvm::Function* func, const ArgVector& args);
  llvm::GenericValue run(llvm::Function* func);

  static void initTargets();

 private:
  EmvmExecutor(llvm::Module* module);

  llvm::Module* module_;
  std::string error_;
  llvm::ExecutionEngine* jit_;

  friend class EmvmBuilder;
};
}  // namespace emvm
