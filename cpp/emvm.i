// EMVM C++/Python interface defintion

%{
#include <string>
#include <vector>
#include "EmvmBuilder.h"
#include "EmvmExecutor.h"

using GenericArgPtr = std::unique_ptr<std::vector<llvm::GenericValue>>;
%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

%template(typevector) std::vector<llvm::Type*>;
%typemap(in) size_t {
  if (!PyInt_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Expected integer");
    return nullptr;
  }
  $1 = PyInt_AsLong($input);
  if (-1 == $1 && PyErr_Occurred()) {
    return nullptr;
  }
}

%define LLVMGENERIC(typename,llvm_typename)
%typemap(out) typename {
  $result = SWIG_NewPointerObj(
    SWIG_as_voidptr($1),
    llvm_typename,
    0 |  0 );
}

%typemap(in) typename {
  if (SWIG_ConvertPtr($input, &$1,llvm_typename, 0 |  0 ) == -1) {
    return nullptr;
  }
}
%enddef

%define LLVMTYPE(typename)
LLVMGENERIC(typename, SWIGTYPE_p_llvm__Type)
%enddef

%define LLVMVALUE(typename)
LLVMGENERIC(typename, SWIGTYPE_p_llvm__Value)
%enddef

LLVMTYPE(llvm::IntegerType*)
LLVMVALUE(llvm::ConstantInt*)
LLVMVALUE(llvm::Argument*)

%typemap(in) const std::vector<llvm::GenericValue>& (GenericArgPtr argholder){
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Expected a Python string");
    return nullptr;
  }

  PyObject* it = PyObject_GetIter($input);
  PyObject* val = nullptr;

  if (nullptr == it) {
    return nullptr;  // propagate error
  }

  argholder = GenericArgPtr(new std::vector<llvm::GenericValue>());
  while ((val = PyIter_Next(it))) {
    if (nullptr == val) {
      Py_DECREF(it);
      return nullptr;
    }

    llvm::GenericValue generic;
    try {
      if (PyInt_Check(val)) {
	int64_t longVal = PyInt_AsLong(val);
	if (-1 == longVal && PyErr_Occurred()) {
	  Py_DECREF(val);
	  Py_DECREF(it);
	  return nullptr;
	}
	generic.IntVal = llvm::APInt(64, longVal, /* is signed = */ true);
      } else {
	PyErr_SetString(
          PyExc_TypeError,
          "Cannot convert type to llvm::GenericValue");
        return nullptr;
      }
    } catch (const std::exception& exn) {
      Py_DECREF(val);
      Py_DECREF(it);
      throw exn;
    }
    Py_DECREF(val);
    argholder->emplace_back(std::move(generic));
  }
  Py_DECREF(it);
  $1 = argholder.get();
}

%typemap(out) llvm::GenericValue {
  $result = PyInt_FromLong(*$1.IntVal.getRawData());
}

%exception {
  try {
    $function
  } catch (const std::exception& exn) {
    PyErr_SetString(PyExc_Exception, exn.what());
    return nullptr;
  }
}

%init {
  emvm::EmvmExecutor::initTargets();
}

%include "EmvmExecutor.h"
%include "EmvmBuilder.h"

