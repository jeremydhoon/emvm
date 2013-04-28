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
  , arguments_() {
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

void EmvmBuilder::exitFunction(llvm::Value* ret) {
  checkInFunction();
  builder_->CreateRet(ret);
  block_ = nullptr;
  func_ = nullptr;
  builder_->ClearInsertionPoint();
}

llvm::Argument* EmvmBuilder::getArg(size_t index) {
  checkInFunction();
  return arguments_.at(index);
}

llvm::ConstantInt* EmvmBuilder::integerLiteral(int64_t i) {
  return llvm::ConstantInt::getSigned(int64Type_, i);
}

llvm::Value* EmvmBuilder::binaryOp(
    llvm::Value* left,
    llvm::Value* right,
    const std::string& op) {
  checkInFunction();

  using Predicate = llvm::CmpInst::Predicate;

  llvm::Value* out =
    op == "add" ? builder_->CreateAdd(left, right) :
    op == "sub" ? builder_->CreateSub(left, right) :
    op == "mul" ? builder_->CreateMul(left, right) :
    op == "lt"  ? builder_->CreateICmp(Predicate::ICMP_SLT, left, right) :
    op == "gt"  ? builder_->CreateICmp(Predicate::ICMP_SGT, left, right) :
    nullptr;
  if (nullptr == out) {
    throwError("Invalid binary operation");
  }
  return out;
}

llvm::Value* EmvmBuilder::select(
    llvm::Value* condition,
    llvm::Value* success,
    llvm::Value* failure) {
  checkInFunction();

  auto successBlk = llvm::BasicBlock::Create(*context_, "success", func_);
  auto failureBlk = llvm::BasicBlock::Create(*context_, "failure", func_);
  auto finalBlk = llvm::BasicBlock::Create(*context_, "final", func_);
  
  builder_->CreateCondBr(condition, successBlk, failureBlk);
  builder_->SetInsertPoint(successBlk);
  builder_->CreateBr(finalBlk);
  builder_->SetInsertPoint(failureBlk);
  builder_->CreateBr(finalBlk);
  builder_->SetInsertPoint(finalBlk);
  auto phi = builder_->CreatePHI(success->getType(), 2);
  phi->addIncoming(success, successBlk);
  phi->addIncoming(failure, failureBlk);
  return phi;
}

llvm::CallInst* EmvmBuilder::call(llvm::Value* function) {
  return call(function, std::vector<llvm::Value*>());
}

llvm::CallInst* EmvmBuilder::call(
  llvm::Value* function,
  const std::vector<llvm::Value*>& args) {

  checkInFunction();
  return builder_->CreateCall(
    function,
    llvm::ArrayRef<llvm::Value*>(args));
}

llvm::IntegerType* EmvmBuilder::getInt64Type() {
  return int64Type_;
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
}  // namespace emvm
