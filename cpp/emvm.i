// EMVM C++/Python interface defintion

%{
#include <string>
#include <vector>
#include "EmvmBuilder.h"
#include "EmvmExecutor.h"
#include "ValuePromise.h"

using GenericArgPtr = std::unique_ptr<std::vector<llvm::GenericValue>>;
%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

%ignore emvm::EmvmBuilder::call(llvm::Function*);

%template(typevector) std::vector<llvm::Type*>;

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

LLVMTYPE(llvm::IntegerType*)

%typemap(in) const std::vector<llvm::GenericValue>& (GenericArgPtr argholder){
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Expected a Python list");
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

%typemap(in) const std::vector<emvm::ValuePromise>& (
    std::vector<emvm::ValuePromise> promises) {
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Expected a Python list");
    return nullptr;
  }

  PyObject* it = PyObject_GetIter($input);
  if (nullptr == it) {
    return nullptr;  // propagate error
  }

  PyObject* val = nullptr;
  while ((val = PyIter_Next(it))) {
    emvm::ValuePromise* promisePtr = nullptr;
    if (nullptr == val) {
      Py_DECREF(it);
      return nullptr;
    }
    int res = SWIG_ConvertPtr(
      val,
      (void**)&promisePtr,
      SWIGTYPE_p_emvm__ValuePromise,
      0);
    if (!SWIG_IsOK(res)) {
      Py_DECREF(val);
      Py_DECREF(it);
      PyErr_SetString(PyExc_TypeError, "Invalid ValuePromise");
      return nullptr;
    }
    assert(promisePtr != nullptr);
    promises.emplace_back(*promisePtr);
  }
  Py_DECREF(it);
  $1 = &promises;
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

%include "ValuePromise.h"
%include "EmvmExecutor.h"
%include "EmvmBuilder.h"
