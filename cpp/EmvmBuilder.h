// EMVM

#pragma once

#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "llvm/Argument.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "EmvmExecutor.h"

namespace emvm {
class EmvmBuilder {
 public:

  class ValuePromise {
   public:
    ValuePromise()
      : value_(nullptr)
      , func_([]() { return nullptr; }) {
    }

    explicit ValuePromise(std::function<llvm::Value*()> func)
      : value_(nullptr)
      , func_(func) {
    }
    ValuePromise(const ValuePromise& other) = default;
    
    llvm::Value* invoke() const {
      if (nullptr == value_) {
	value_ = func_(); 
      }
      return value_;
    }

  private:
    mutable llvm::Value* value_;
    std::function<llvm::Value*()> func_;
  };

  EmvmBuilder();
  explicit EmvmBuilder(const std::string& name);

  EmvmExecutor compile();
  void print();

  llvm::Function* enterFunction(const std::string& name,
				llvm::Type* resultType);
  llvm::Function* enterFunction(const std::string& name,
				llvm::Type* resultType,
				const std::vector<llvm::Type*>& args);
  void exitFunction(ValuePromise ret);

  ValuePromise getArg(size_t index);

  ValuePromise integerLiteral(int64_t i);
  ValuePromise binaryOp(ValuePromise left,
			ValuePromise right,
			const std::string& op);
  ValuePromise select(ValuePromise condition,
		      ValuePromise success,
		      ValuePromise failure);
  ValuePromise call(llvm::Function* function);
  ValuePromise call(llvm::Function* function,
		    const std::vector<ValuePromise>& args);

  llvm::IntegerType* getInt64Type();

 private:
  void checkInFunction() const;
  void checkNotInFunction() const;
  void throwError(const std::string& error) const;

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<> > builder_;
  llvm::IntegerType* int64Type_;
  llvm::Function* func_;
  llvm::BasicBlock* block_;
  std::vector<llvm::Argument*> arguments_;
};
}  // namespace emvm
