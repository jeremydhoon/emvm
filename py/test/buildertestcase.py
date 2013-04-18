#!/usr/bin/env python

import unittest

import builder as br

@br.function(int, int)
def sum(a, b):
    return a + b

@br.function(int)
def factorial(n):
    return br.If(
        n > br.IntegerLiteral(1),
        n * br.Call(factorial, [n - br.IntegerLiteral(1)]),
        br.IntegerLiteral(1))

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

if __name__ == "__main__":
    unittest.main()
