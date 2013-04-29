// EMVM

#include "EmvmBuilder.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/PassManager.h"
#include "llvm/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

namespace emvm {

EmvmBuilder::EmvmBuilder()
  : EmvmBuilder("emvm-default") {
}

EmvmBuilder::EmvmBuilder(const std::string& name)
  : context_(new llvm::LLVMContext())
  , module_(new llvm::Module(name, *context_))
  , builder_(new llvm::IRBuilder<>(*context_))
  , int64Type_(llvm::IntegerType::get(*context_, 64))
  , func_(nullptr)
  , block_(nullptr)
  , arguments_()
  , intFmtStr_(nullptr) {
}

EmvmExecutor EmvmBuilder::compile() {
  return EmvmExecutor(module_.get());
}

void EmvmBuilder::print() {
  llvm::verifyModule(*module_, llvm::PrintMessageAction);
  llvm::PassManager pm;
  pm.add(llvm::createPrintModulePass(&llvm::outs()));
  pm.run(*module_);
}

llvm::Function* EmvmBuilder::enterFunction(
    const std::string& name,
    llvm::Type* resultType) {
  std::vector<llvm::Type*> args;  // empty
  return enterFunction(name, resultType, args);
}

llvm::Function* EmvmBuilder::enterFunction(
    const std::string& name,
    llvm::Type* resultType,
    const std::vector<llvm::Type*>& args) {
  checkNotInFunction();
  llvm::ArrayRef<llvm::Type*> argTypes(args);
  llvm::FunctionType* fnType = llvm::FunctionType::get(
    resultType,
    argTypes,
    /* is vararg = */ false);
  func_ = static_cast<llvm::Function*>(
    module_->getOrInsertFunction(name, fnType));
  block_ = llvm::BasicBlock::Create(*context_, name, func_);
  builder_->SetInsertPoint(block_);
  arguments_.clear();
  arguments_.reserve(argTypes.size());
  for (auto& arg : func_->getArgumentList()) {
    arguments_.push_back(&arg);
  }
  return func_;
}

void EmvmBuilder::exitFunction(ValuePromise ret) {
  checkInFunction();
  builder_->CreateRet(ret.invoke());
  block_ = nullptr;
  func_ = nullptr;
  builder_->ClearInsertionPoint();
}

ValuePromise EmvmBuilder::getArg(size_t index) {
  checkInFunction();
  return ValuePromise([this, index]() -> llvm::Value* {
    return arguments_.at(index);
  });
}

ValuePromise EmvmBuilder::integerLiteral(int64_t i) {
  return ValuePromise([this, i]() -> llvm::Value* {
      return getInt(i);
  });
}

ValuePromise EmvmBuilder::binaryOp(
    ValuePromise leftPromise,
    ValuePromise rightPromise,
    const std::string& op) {
  auto constructor = [this, leftPromise, rightPromise, op]() -> llvm::Value* {
    checkInFunction();

    using Predicate = llvm::CmpInst::Predicate;

    auto left = leftPromise.invoke();
    auto right = rightPromise.invoke();
    llvm::Value* out =
      op == "add" ? builder_->CreateAdd(left, right) :
      op == "sub" ? builder_->CreateSub(left, right) :
      op == "mul" ? builder_->CreateMul(left, right) :
      op == "mod" ? builder_->CreateSRem(left, right) :
      op == "lt"  ? builder_->CreateICmpSLT(left, right) :
      op == "gt"  ? builder_->CreateICmpSGT(left, right) :
      nullptr;
    if (nullptr == out) {
      throwError("Invalid binary operation");
    }
    return out;
  };
  return ValuePromise(constructor);
}

ValuePromise EmvmBuilder::select(
    ValuePromise condition,
    ValuePromise success,
    ValuePromise failure) {
  return ValuePromise([this, condition, success, failure]() -> llvm::Value* {
    checkInFunction();

    auto successBlk = llvm::BasicBlock::Create(*context_, "success", func_);
    auto failureBlk = llvm::BasicBlock::Create(*context_, "failure", func_);
    auto finalBlk = llvm::BasicBlock::Create(*context_, "final", func_);
  
    builder_->CreateCondBr(condition.invoke(), successBlk, failureBlk);
    builder_->SetInsertPoint(successBlk);
    auto successRealized = success.invoke();
    builder_->CreateBr(finalBlk);
    builder_->SetInsertPoint(failureBlk);
    auto failureRealized = failure.invoke();
    builder_->CreateBr(finalBlk);
    builder_->SetInsertPoint(finalBlk);
    auto phi = builder_->CreatePHI(successRealized->getType(), 2);
    phi->addIncoming(successRealized, successBlk);
    phi->addIncoming(failureRealized, failureBlk);
    return phi;
  });
}

ValuePromise EmvmBuilder::call(llvm::Function* function) {
  return call(function, std::vector<ValuePromise>());
}

ValuePromise EmvmBuilder::call(
  llvm::Function* function,
  const std::vector<ValuePromise>& args) {
  return ValuePromise([this, function, args]() -> llvm::Value* {
    checkInFunction();

    std::vector<llvm::Value*> realizedArgs;
    for (auto& promise : args) {
      realizedArgs.emplace_back(promise.invoke());
    }

    checkInFunction();
    return builder_->CreateCall(
      function,
      llvm::ArrayRef<llvm::Value*>(realizedArgs));
  });
}

llvm::Function* EmvmBuilder::countPredicateMatches(llvm::Function* predicate,
						   int64_t* toScan,
						   size_t toScanSize) {
  checkNotInFunction();

  auto zero = getInt(0);
  auto one = getInt(1);

  std::string functionName = (
    std::string("count_predicate_match_") + predicate->getName().str());
  llvm::FunctionType* fnType = llvm::FunctionType::get(
    getInt64Type(),
    /* is vararg = */ false);
  auto func = static_cast<llvm::Function*>(
    module_->getOrInsertFunction(functionName, fnType));

  if (toScanSize <= 0) {
    auto bailBlk = llvm::BasicBlock::Create(*context_, "cpm_bail", func);
    builder_->SetInsertPoint(bailBlk);
    builder_->CreateRet(zero);
    return func;
  }

  auto entryBlk = llvm::BasicBlock::Create(*context_, "cpm_entry", func);
  auto exitBlk  = llvm::BasicBlock::Create(*context_, "cpm_exit",  func);
  auto loopBlk  = llvm::BasicBlock::Create(*context_, "cpm_loop",  func);

  // Handle initialization
  builder_->SetInsertPoint(entryBlk);
  auto memoryLocation = llvm::ConstantExpr::getIntToPtr(
    getInt(reinterpret_cast<uint64_t>(toScan)),
    llvm::PointerType::getUnqual(getInt64Type()));
  builder_->CreateBr(loopBlk);

  // set up loop logic initialization
  builder_->SetInsertPoint(loopBlk);
  auto prevCount = builder_->CreatePHI(getInt64Type(), 2, "prev_count");
  auto loopIndex = builder_->CreatePHI(getInt64Type(), 2, "current_index");
  auto incrementLoopIndex = builder_->CreateAdd(loopIndex, one, "next_index");
  loopIndex->addIncoming(zero, entryBlk);
  loopIndex->addIncoming(incrementLoopIndex, loopBlk);

  // Load next integer to evaluate and bump count, conditioned on predicate
  // result.
  auto currentPointer = builder_->CreateGEP(
    memoryLocation,
    loopIndex,
    "current_pointer");
  auto currentInt = builder_->CreateLoad(currentPointer, "current_value");
  auto predicateResult = builder_->CreateCall(
    predicate,
    currentInt,
    "predicate_result");
  auto predicateResultIsTrue = builder_->CreateICmpNE(predicateResult, zero);
  auto increment = builder_->CreateSelect(predicateResultIsTrue, one, zero);
  auto count = builder_->CreateAdd(prevCount, increment, "count");
  prevCount->addIncoming(zero, entryBlk);
  prevCount->addIncoming(count, loopBlk);

  // Loop condition
  auto atEnd = builder_->CreateICmpSLT(
    incrementLoopIndex,
    getInt(toScanSize),
    "at_end");
  auto loopBr = builder_->CreateCondBr(atEnd, loopBlk, exitBlk);

  // Finish up
  builder_->SetInsertPoint(exitBlk);
  builder_->CreateRet(count);

  return func;
}

llvm::IntegerType* EmvmBuilder::getInt64Type() {
  return int64Type_;
}

llvm::ConstantInt* EmvmBuilder::getInt(int64_t i) {
  return llvm::ConstantInt::get(getInt64Type(), i);
}

void EmvmBuilder::checkInFunction() const {
  if (nullptr == block_) {
    throwError("Builder is not currently in a function.");
  }
}

void EmvmBuilder::checkNotInFunction() const {
  if (nullptr != block_) {
    throwError("Builder is currently in a function.");
  }
}

void EmvmBuilder::throwError(const std::string& error) const {
  throw std::runtime_error(error.c_str());
}

llvm::Function* EmvmBuilder::getPrintf() {
  auto existingPrintfFn = module_->getFunction("printf");
  if (nullptr != existingPrintfFn) {
    return existingPrintfFn;
  }

  std::vector<llvm::Type*> argTypes({llvm::Type::getInt8PtrTy(*context_)});
  auto fnType = llvm::FunctionType::get(
    llvm::IntegerType::get(*context_, 32),
    argTypes,
    /* is vararg = */ true);

  
  auto printfFn = llvm::Function::Create(
    fnType,
    llvm::Function::ExternalLinkage,
    "printf",
    module_.get());
  printfFn->setCallingConv(llvm::CallingConv::C);
  return printfFn;
}

void EmvmBuilder::printIntValue(llvm::Value* value) {
  const std::string fmt = "%lld\n";
  auto fmtStr = llvm::ConstantDataArray::getString(*context_, fmt);
  if (nullptr == intFmtStr_) {
    intFmtStr_ = new llvm::GlobalVariable(
      *module_,
      llvm::ArrayType::get(
        llvm::IntegerType::get(*context_, 8),
	fmt.size() + 1),
      /* is constant = */ true,
      llvm::GlobalValue::PrivateLinkage,
      fmtStr);
  }
  std::vector<llvm::Value*> indices({getInt(0), getInt(0)});
  auto fmtPtr = builder_->CreateGEP(intFmtStr_, indices);
  builder_->CreateCall2(getPrintf(), fmtPtr, value);
}
}  // namespace emvm
