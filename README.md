EMVM
====

Goals
-----

How will your children write programs? They'll need to write code that...
- Performant
- Expressive
- Portable
- Transmissible
- Extensively customizable
- Able to interact easily with the system (rather than providing only the
  lowest common denominator)

Other Work
----------

Three approaches:
1. Just JIT everything and hope that it's fast
   * V8 approach to Javascript
   * PyPy
   * HHVM
   * Advantages:
     - Easy to write correct code, but sometimes unintuitive to write
       performant code.
     - Results in transmissible, portable code (no need to pass around
       native code).
   * Disadvantages:
     - Sometimes unintuitive to write performant code
     - JIT-ing can be computationally expensive
     - Difficult to achieve the level of performance of other approaches
2. Provide a fast subset or an API with certain performance guarantees
   * asm.js
   * Dart optional typing
   * Typescript
   * Ruby inline-C
   * Cython/cytpes
   * Advantages:
     - Can achieve faster performance than pure JIT'ing
     - No need to pass around a binary (except possibly some native code
       for the module that runs the subset)
     - Often compatible with JIT'ing as a fallback solution
   * Disadvantages:
     - Requires deliberate effort on the part of the programmer
     - Not necessarily as fast as native code
3. Expose a foreign function interface that makes it easy to call into a
   performant system.
   * Google Native Client
   * CPython extensions/Python FFI/SWIG/boost::python
   * Advantages:
     - Offers the absolute best performance
     - Works with any piece of C or C++ out there
   * Disadvantages:
     - Requires distributing a binary
       - Not portable across different platforms or architectures
       - Requires that the binary come from a trusted source
     - Bridging the dynamic runtime/native gap is generally a lot of work
       - Have to interact with dynamic runtime internals
       - Semantics of C types may not match those of the dynamic runtime.
     - Does not play well with JIT (example: PyPy has to reify PyObjects).

Design
------

- Simplified AST construction embedded in Python
- AST representation is transmissible
- Compiles to LLVM
- Initial limitations:
  * no memory allocation
  * no foreign function calls
- Use case: arithmetic circuits, quick computation
- All arguments and return types should be losslessly represented in Python
