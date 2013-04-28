#!/usr/bin/env python

"""
builder -- tools for constructing emvm syntax trees.
"""

import operator

import os
import sys

import pyemvm

def _abstract(obj):
    raise NotImplementedError("Called abstract method on %s", obj.__class__)

#
# Types
#

def _vmtype(typ, expected=None):
    expected = expected or VMType
    if not isinstance(typ, expected):
        raise TypeError("Expected VMType")
    return typ

def _unified(t1, t2, cond):
    if not cond:
        raise TypeError(
            "Failed to unify types '%s' and '%s'",
            t1.name(),
            t2.name())
    return t1

def _unified_base(t1, t2, cond):
    return _unified(t1, t2, type(t1) == type(t2) and cond)

class VMType(object):
    def pythonType(self):
        _abstract(self)

    def unify(self, other):
        _abstract(self)

    def name(self):
        _abstract(self)

    def build(self, builder):
        _abstract(self)

class VoidType(VMType):
    _void = None

    def pythonType(self):
        return self.get()  # return our own type as a marker for void

    def unify(self, other):
        return _unified(self, other, other is self.get())

    def name(self):
        return self.__class__

    @classmethod
    def get(cls):
        cls._void = cls._void or cls()
        return cls._void

class BasicType(VMType):
    def __init__(self, basic_type):
        if not isinstance(basic_type, type):
            raise TypeError("Not a basic Python type: %s", basic_type)
        self.basic_type = basic_type

    def pythonType(self):
        return self.basic_type

    def name(self):
        return "BasicType(%s)" % str(self.basic_type).split("'")[1]

    def unify(self, other):
        return _unified_base(
            self,
            other,
            self.pythonType() == other.pythonType())

    def build(self, builder):
        if self.basic_type == int:
            return builder.getInt64Type()
        raise NotImplementedError(
            "Do not know how to build type ",
            self.name())

class FunctionType(VMType):
    def __init__(self, result_type, argument_types):
        self.result_type = _vmtype(result_type, BasicType)
        self.argument_types = map(_vmtype, argument_types)

    def pythonType(self):
        return type(self.pythonType)

    def name(self):
        arg_types = ", ".join(
            [arg_type.name() for arg_type in self.argument_types])
        return "%s -> (%s)" % (self.result_type.name(), arg_types)

    def unify(self):
        return _unified(
            self,
            other,
            type(self) == type(other) and
            self.result_type.unify(other.result_type) and
            len(self.argument_types) == len(other.argument_types) and
            all([self_arg.unify(other_arg)
                 for self_arg, other_arg
                 in zip(self.argument_types, other.argument_types)]))
#
# Symbols
#

def _symbol(obj, expected=None):
    expected = expected or Symbol
    if not isinstance(obj, expected):
        raise TypeError(
            "object of type %s was not an instance of %s" %
            (obj, expected))
    return obj

class Symbol(object):
    def run(self):
        _abstract(self)

    def typeof(self):
        _abstract(self)

    def build(self, builder):
        _abstract(self)

    def __add__(self, other):
        return BinaryOperator(self, other, operator.add)
    def __sub__(self, other):
        return BinaryOperator(self, other, operator.sub)

    def __eq__(self, other):
        return BinaryOperator(self, other, operator.eq)
    def __lt__(self, other):
        return BinaryOperator(self, other, operator.lt)
    def __gt__(self, other):
        return BinaryOperator(self, other, operator.gt)
    def __mul__(self, other):
        return BinaryOperator(self, other, operator.mul)

class Literal(Symbol):
    def __init__(self, val):
        self.val = val

    def run(self):
        return self.val

class IntegerLiteral(Literal):
    def __init__(self, ival):
        if isinstance(ival, long):
            raise TypeError("emvm only supports int type, not long.")
        if not isinstance(ival, int):
            raise TypeError("Not an integer literal: ", ival)
        Literal.__init__(self, ival)

    def typeof(self):
        return BasicType(int)

    def build(self, builder):
        return builder.integerLiteral(self.run())

class BooleanLiteral(Literal):
    def __init__(self, val):
        Literal.__init__(self, bool(val))

    def build(self, builder):
        return builder.integerLiteral(int(self.run()))

class StringLiteral(Literal):
    def __init__(self, val):
        if isinstance(val, unicode):
            raise TypeError("emvm only supports str type, not unicode.")
        if not isinstance(val, str):
            raise TypeError("Not a string literal: ", val)
        Literal.__init__(self, val)

class CompoundSymbol(Symbol):
    pass

class BinaryOperator(CompoundSymbol):
    def __init__(self, left, right, op):
        self.left = _symbol(left)
        self.right = _symbol(right)
        self.t = self.left.typeof().unify(self.right.typeof())
        self.op = op

    def typeof(self):
        return self.t

    def run(self):
        return self.op(self.left.run(), self.right.run())

    def build(self, builder):
        return builder.binaryOp(
            self.left.build(builder),
            self.right.build(builder),
            self.op.__name__)

class If(CompoundSymbol):
    def __init__(self, cond, success, failure):
        self.cond = _symbol(cond)
        self.success = success
        self.failure = failure
        self.t = self.success.typeof().unify(self.failure.typeof())

    def typeof(self):
        return self.t

    def run(self):
        if self.cond.run():
            return self.success.run()
        return self.failure.run()

    def build(self, builder):
        return builder.select(
            self.cond.build(builder),
            self.success.build(builder),
            self.failure.build(builder))

class Argument(Symbol):
    def __init__(self, typeof, index):
        self.bound = False
        self.value = None
        # for convenience, allow conversion from Python basic types
        # to BasicTypes
        self.t = BasicType(typeof) if isinstance(typeof, type) else typeof
        self.index = index

    def typeof(self):
        return self.t

    def bind(self, arg_value):
        assert self.typeof().unify(arg_value.typeof())
        self.bound = True
        self.value = arg_value.run()

    def unbind(self):
        self.value = None
        self.bound = False

    def run(self):
        assert self.bound
        return self.value

    def build(self, builder):
        return builder.getArg(self.index)

class Function(Symbol):
    def __init__(self, arguments, body):
        self.arguments = self._validateargs(arguments)
        self.body = _symbol(body)

    def _validateargs(self, arguments):
        return [_symbol(arg, Argument) for arg in arguments]

    def _getbody(self):
        return self.body

    def _bind(self, arg_values):
        for arg_value, arg in zip(arg_values, self.arguments):
            arg.bind(arg_value)

    def call(self, arg_values):
        assert len(arg_values) == len(self.arguments)
        self._bind(arg_values)
        return self._getbody().run()

    def typeof(self):
        return FunctionType(
            self._getbody().typeof(),
            [arg.typeof() for arg in self.arguments])

    def run(self):
        pass

class LazyFunction(Function):
    def __init__(self, arguments, fn, result_type, name):
        self.body = None
        self.evaluating = False
        self.arguments = self._validateargs(arguments)
        self.fn = fn
        self.result_type = _vmtype(result_type)
        self.name = name
        self.compiled_function = None

    def typeof(self):
        return FunctionType(
            self.result_type,
            [arg.typeof() for arg in self.arguments])

    def _getbody(self):
        if self.body is None:
            if self.evaluating:
                raise RuntimeError("Infinite recursion")
            self.evaluating = True
            self.body = self.fn(*self.arguments)
            self.evaluating = False
        return self.body

    def build(self, builder):
        if self.compiled_function is None:
            argtypes = [arg.typeof().build(builder) for arg in self.arguments]
            self.compiled_function = builder.enterFunction(
                self.name,
                self.result_type.build(builder),
                argtypes)
            builder.exitFunction(self._getbody().build(builder))
        return self.compiled_function
            

class Call(CompoundSymbol):
    def __init__(self, fn, argument_values):
        self.fn = _symbol(fn, Function)
        self.argument_values = map(_symbol, argument_values)
        for arg_value, arg in zip(self.argument_values, self.fn.arguments):
            arg_value.typeof().unify(arg.typeof())

    def typeof(self):
        return self.fn.typeof().result_type

    def run(self):
        return self.fn.call(self.argument_values)

    def build(self, builder):
        fn = self.fn.build(builder)
        args = [arg.build(builder) for arg in self.argument_values]
        return builder.call(self.fn.build(builder), args)

def emvm_run(symbol):
    return _symbol(symbol).run()

def emvm_build(symbol):
    builder = pyemvm.EmvmBuilder()
    _symbol(symbol).build(builder)
    return builder

def function(*arguments):
    """ Decorator a function to denote the AST subtree it returns as an emvm
    function."""
    vm_arguments = [Argument(typ, ix) for ix,typ in enumerate(arguments)]
    def decorator(fn):
        return LazyFunction(vm_arguments, fn, BasicType(int), fn.func_name)
    return decorator

