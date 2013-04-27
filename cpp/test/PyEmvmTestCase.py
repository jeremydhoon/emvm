#!/usr/bin/env python

import os
import unittest
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pyemvm

class PyEmvmTestCase(unittest.TestCase):
    def testNoArgFunction(self):
        b = pyemvm.EmvmBuilder()
        itype = b.getInt64Type()
        b.enterFunction("add3", itype, [itype]*3)
        addfirst = b.binaryOp(b.getArg(0), b.getArg(1), "add")
        addsecond = b.binaryOp(b.getArg(2), addfirst, "add")
        b.exitFunction(addsecond)
        executor = b.compile()
        self.assertEquals(
            18,
            executor.run("add3", [3, 6, 9]),
            "Failed to compiute result")

if __name__ == "__main__":
    unittest.main()
