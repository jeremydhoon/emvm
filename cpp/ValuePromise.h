// EMVM

#pragma once

#include <functional>

namespace llvm {
class Value;
}

namespace emvm {

class ValuePromise {
 public:
  ValuePromise();
  ValuePromise(std::function<llvm::Value*()> func);
  ValuePromise(const ValuePromise& other) = default;

  llvm::Value* invoke() const;

 private:
  mutable llvm::Value* value_;
  std::function<llvm::Value*()> func_;
};
}  // namespace emvm
