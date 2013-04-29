// EMVM

#include "ValuePromise.h"

namespace emvm {

ValuePromise::ValuePromise()
  : value_(nullptr)
  , func_([]() { return nullptr; }) {
}

ValuePromise::ValuePromise(std::function<llvm::Value*()> func)
  : value_(nullptr)
  , func_(func) {
}    

llvm::Value* ValuePromise::invoke() const {
  if (nullptr == value_) {
    value_ = func_(); 
  }
  return value_;
}
}  // namespace emvm
