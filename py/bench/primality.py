#!/usr/bin/env python

import os
import sys
sys.path.append(os.path.dirname(os.path.dirname(__file__)))

import builder as br

from matplotlib import pyplot
import numpy as np

import time

@br.function(int, int)
def is_prime_recursive(n, k):
    return br.If(k > 1, (n % k != 0) & is_prime_recursive(n, k - 1), 1)

@br.function(int)
def is_prime(n):
    return br.If(n > 1, is_prime_recursive(n, n / 2), 0)

def prime_count_py(arr):
    count = 0
    for n in arr:
        if n < 2:
            continue
        incr = 1
        for i in xrange(2, n / 2 + 1):
            if n % i == 0:
                incr = 0
                break
        count += incr
    return count
        
def stopwatch(func):
    start = time.time()
    print "output: ", func()
    return time.time() - start

def main(argv):
    emvm_results = []
    py_results = []
    sizes = (1, 10, 100, 1000, 10000, 100000, 1000000)
    for size in sizes:
        arr = np.array(np.random.random_integers(1, 100, size), dtype='int64')
        print "array size:", size
        emvm_result = stopwatch(
            lambda: br.emvm_count_predicate(
                is_prime,
                arr,
                [is_prime_recursive]))
        emvm_results.append(emvm_result)
        print "emvm:", emvm_result
        py_result = stopwatch(lambda: prime_count_py(arr))
        py_results.append(py_result)
        print "py:", py_result
        print ""
    pyplot.plot(sizes, emvm_results, label="EMVM")
    pyplot.plot(sizes, py_results, label="Python")
    pyplot.title("Performance comparison: EMVM vs. Python")
    pyplot.xlabel("Input size")
    pyplot.ylabel("Runtime (seconds, lower is better)")
    pyplot.legend()
    pyplot.savefig("emvm_vs_py_results.pdf")
    pyplot.savefig("emvm_vs_py_results.png")
    pyplot.show()
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv))
