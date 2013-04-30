#!/usr/bin/env python

import unittest

import numpy as np

import builder as br

@br.function(int, int)
def sum(a, b):
    return a + b

@br.function(int)
def factorial(n):
    return br.If(n > 1, n * factorial(n - 1), 1)

@br.function(int)
def is_odd(n):
    return n % 2 != 0

class BuilderTestCase(unittest.TestCase):
    def testSymbol(self):
        s = br.Symbol()
        self.assertRaises(NotImplementedError, s.run)

    def testIntegers(self):
        value = br.emvm_run(br.IntegerLiteral(12) + br.IntegerLiteral(13))
        self.assertEquals(25, value, "addition failed")
        self.assertFalse(
            br.emvm_run(br.IntegerLiteral(1) == br.IntegerLiteral(2)),
            "Equality comparison failed")
        self.assertTrue(
            br.emvm_run(br.IntegerLiteral(-10) == br.IntegerLiteral(-10)),
            "Equality comparison failed")

    def testCall(self):
        self.assertEquals(
            5,
            br.emvm_run(
                br.Call(sum, [br.IntegerLiteral(2), br.IntegerLiteral(3)])),
            "Function call failed")

    def testFactorial(self):
        self.assertEquals(
            24,
            br.emvm_run(
                br.Call(factorial, [br.IntegerLiteral(4)])),
            "Incorrect factorial call")

    def testBuild(self):
        builder = br.emvm_build(sum)
        self.assertEquals(
            42,
            builder.compile().run("sum", [10, 32]),
            "Incorrect compiled sum result")

    def testBuildFactorial(self):
        builder = br.emvm_build(factorial)
        self.assertEquals(
            3628800,
            builder.compile().run("factorial", [10]),
            "Incorrect compiled factorial result")

    def testCountPredicate(self):
        arr = np.array([0, 1, 3, 5, 10], dtype='int64')
        self.assertEquals(
            3,
            br.emvm_count_predicate(is_odd, arr),
            "Incorrect odd count")

if __name__ == "__main__":
    unittest.main()
