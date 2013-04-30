// EMVM

#include "EmvmExecutor.h"

#include <stdexcept>

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/Module.h"
#include "llvm/Support/TargetSelect.h"

namespace emvm {

using ArgVector = std::vector<llvm::GenericValue>;

EmvmExecutor::EmvmExecutor(llvm::Module* module)
  : module_(module)
  , jit_(llvm::ExecutionEngine::createJIT(module_, &error_)) {
  if (nullptr == jit_) {
    throw std::runtime_error("Failed to create JIT: " + error_);
  }
}

llvm::GenericValue EmvmExecutor::run(
    const std::string& funcName,
    const ArgVector& args) {
  auto func = static_cast<llvm::Function*>(module_->getFunction(funcName));
  return run(func, args);
}

llvm::GenericValue EmvmExecutor::run(
    llvm::Function* func,
    const ArgVector& args) {
  if (nullptr == func) {
    throw std::runtime_error("Function not found.");
  }
  return jit_->runFunction(func, args);
}

llvm::GenericValue EmvmExecutor::run(llvm::Function* func) {
  ArgVector args;
  return run(func, args);
}

void EmvmExecutor::initTargets() {
  llvm::InitializeNativeTarget();
}
}  // namespace llvm
