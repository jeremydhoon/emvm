// EMVM

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "llvm/Argument.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "EmvmExecutor.h"

namespace emvm {
class EmvmBuilder {
 public:
  EmvmBuilder();
  explicit EmvmBuilder(const std::string& name);

  EmvmExecutor compile();
  void print();

  void enterFunction(const std::string& name, llvm::Type* resultType);
  void enterFunction(const std::string& name,
                     llvm::Type* resultType,
                     const std::vector<llvm::Type*>& args);
  void exitFunction(llvm::Value* ret);

  llvm::Argument* getArg(size_t index);

  llvm::ConstantInt* integerLiteral(int64_t i);
  llvm::Value* binaryOp(llvm::Value* left,
                        llvm::Value* right,
                        const std::string& op);
  llvm::Value* select(llvm::Value* condition,
                      llvm::Value* success,
                      llvm::Value* failure);

  llvm::IntegerType* getInt64Type();

 private:
  void checkInFunction() const;
  void checkNotInFunction() const;
  void throwError(const std::string& error) const;

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  llvm::IntegerType* int64Type_;
  llvm::Function* func_;
  llvm::BasicBlock* block_;
  std::vector<llvm::Argument*> arguments_;
};
}  // namespace emvm
