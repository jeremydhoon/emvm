// EMVM

#include "EmvmBuilder.h"
#include "ValuePromise.h"

#include <iostream>
#include <string>
#include <vector>

using emvm::EmvmBuilder;
using emvm::ValuePromise;

void testBuilder() {
  EmvmBuilder builder("test-builder");
  auto i64 = builder.getInt64Type();

  {
    builder.enterFunction("testReturn", i64);
    auto op = builder.binaryOp(
      builder.integerLiteral(10),
      builder.integerLiteral(20),
      "add");
    builder.exitFunction(op);
  }
  {
    builder.enterFunction(
      "addFunc",
      i64,
      std::vector<llvm::Type*>(2, i64));
    auto op = builder.binaryOp(
      builder.getArg(0),
      builder.getArg(1),
      "add");
    builder.exitFunction(op);
  }
  {
    builder.enterFunction(
      "min",
      i64,
      std::vector<llvm::Type*>(2, i64));
    auto left = builder.getArg(0);
    auto right = builder.getArg(1);
    auto condition = builder.binaryOp(left, right, "lt");
    auto min = builder.select(condition, left, right);
    builder.exitFunction(min);
  }
  {
    auto factorial = builder.enterFunction(
      "factorial",
      i64,
      std::vector<llvm::Type*>({ i64 }));
    auto n = builder.getArg(0);
    auto recurse = builder.select(
      builder.binaryOp(n, builder.integerLiteral(1), "gt"),
      builder.binaryOp(
        n,
	builder.call(
          factorial,
          std::vector<ValuePromise>({
	    builder.binaryOp(n, builder.integerLiteral(1), "sub")})),
	"mul"),
      builder.integerLiteral(1));
    builder.exitFunction(recurse);
  }

  auto isOdd = builder.enterFunction("isOdd", i64, { i64 });
  builder.exitFunction(
    builder.binaryOp(builder.getArg(0), builder.integerLiteral(2), "mod"));
  std::vector<int64_t> numbers({1, 3, 5, 10, 12, 15});
  auto countOdd = builder.countPredicateMatches(
    isOdd,
    &numbers.front(),
    numbers.size());

  builder.print();

  emvm::EmvmExecutor::initTargets();
  auto exec = builder.compile();
  llvm::GenericValue ten, twenty;
  ten.IntVal = llvm::APInt(64, 10, /* is signed = */ true);
  twenty.IntVal = llvm::APInt(64, 20, /* is signed = */ true);
  auto result = exec.run("addFunc", {ten, twenty});
  std::cout << "Addition: " << result.IntVal.toString(10, true) << std::endl;

  auto factorialResult = exec.run("factorial", {ten});
  std::cout << "Factorial: " << factorialResult.IntVal.toString(10, true)
	    << std::endl;

  auto countResult = exec.run(countOdd);
  std::cout << "Odd count: " << countResult.IntVal.toString(10, true)
	    << std::endl;
}

int main(int argc, char* argv[]) {
  testBuilder();
  return 0;
}
