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
#include "ValuePromise.h"

namespace emvm {
class EmvmBuilder {
 public:
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

  // Function applications
  llvm::Function* countPredicateMatches(llvm::Function* predicate,
					int64_t* toScan,
					size_t toScanSize);

 private:
  llvm::ConstantInt* getInt(int64_t i);
  llvm::Value* toInt64(llvm::Value* otherInt);
  llvm::Value* toBool(llvm::Value* otherInt);
  void checkInFunction() const;
  void checkNotInFunction() const;
  void throwError(const std::string& error) const;

  // Handy debugging functions for adding print statements to generated
  // LLVM IR code.
  llvm::Function* getPrintf();
  void printIntValue(llvm::Value* value);

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<> > builder_;
  llvm::IntegerType* int64Type_;
  llvm::Function* func_;
  llvm::BasicBlock* block_;
  std::vector<llvm::Argument*> arguments_;
  llvm::GlobalVariable* intFmtStr_;
};
}  // namespace emvm
