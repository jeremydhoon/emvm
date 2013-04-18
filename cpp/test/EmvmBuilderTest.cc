// EMVM

#include "EmvmBuilder.h"

#include <iostream>
#include <string>
#include <vector>

using emvm::EmvmBuilder;

void testBuilder() {
  EmvmBuilder builder("test-builder");

  {
    builder.enterFunction("testReturn", builder.getInt64Type());
    auto op = builder.binaryOp(
      builder.integerLiteral(10),
      builder.integerLiteral(20),
      "add");
    builder.exitFunction(op);
  }
  {
    builder.enterFunction(
      "addFunc",
      builder.getInt64Type(),
      std::vector<llvm::Type*>(2, builder.getInt64Type()));
    auto op = builder.binaryOp(
      builder.getArg(0),
      builder.getArg(1),
      "add");
    builder.exitFunction(op);
  }
  {
    builder.enterFunction(
      "min",
      builder.getInt64Type(),
      std::vector<llvm::Type*>(2, builder.getInt64Type()));
    auto left = builder.getArg(0);
    auto right = builder.getArg(1);
    auto condition = builder.binaryOp(left, right, "lt");
    auto min = builder.select(condition, left, right);
    builder.exitFunction(min);
  }

  builder.print();

  emvm::EmvmExecutor::initTargets();
  auto exec = builder.compile();
  llvm::GenericValue ten, twenty;
  ten.IntVal = llvm::APInt(64, 10, /* is signed = */ true);
  twenty.IntVal = llvm::APInt(64, 20, /* is signed = */ true);
  auto result = exec.run("addFunc", {ten, twenty});
  std::cout << result.IntVal.toString(10, true) << std::endl;
}

int main(int argc, char* argv[]) {
  testBuilder();
  return 0;
}
