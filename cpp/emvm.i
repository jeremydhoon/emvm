// EMVM C++/Python interface defintion

%{
#include <string>
#include <vector>

#include "numpy/arrayobject.h"

#include "EmvmBuilder.h"
#include "EmvmExecutor.h"
#include "ValuePromise.h"

using GenericArgPtr = std::unique_ptr<std::vector<llvm::GenericValue>>;
%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

%ignore emvm::EmvmBuilder::call(llvm::Function*);
%rename("runFunction") emvm::EmvmExecutor::run(llvm::Function* func, const std::vector<llvm::GenericValue>& args);
%ignore emvm::EmvmExecutor::run(llvm::Function*);

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

// Numpy handling
%typemap(in) (llvm::Function* predicate, int64_t* toScan, size_t toScanSize) {
  const char* errStr = "Expected a tuple (function, ndarray)";
  if (!PyTuple_Check($input)) {
    PyErr_SetString(PyExc_TypeError, errStr);
    return nullptr;
  }

  auto numArgs = PyTuple_GET_SIZE($input);
  if (numArgs != 2) {
    PyErr_SetString(PyExc_TypeError, errStr);
    return nullptr;
  }

  PyObject* funcPtr = PyTuple_GET_ITEM($input, 0);
  PyObject* arr = PyTuple_GET_ITEM($input, 1);

  llvm::Function* func = nullptr;
  auto result = SWIG_ConvertPtr(
    funcPtr,
    reinterpret_cast<void**>(&func),
    SWIGTYPE_p_llvm__Function,
    0);
  if (!SWIG_IsOK(result)) {
    PyErr_SetString(PyExc_TypeError, errStr);
    return nullptr;
  }

  if (nullptr == arr || !PyArray_Check(arr)) {
    PyErr_SetString(PyExc_TypeError, errStr);
    return nullptr;
  }
  
  if (PyArray_NDIM(arr) != 1) {
    PyErr_SetString(PyExc_TypeError, "Expected a one-dimensional array");
    return nullptr;
  }
  auto descr = PyArray_DESCR(arr);
  if (nullptr == descr || descr->elsize != sizeof(int64_t)) {
    PyErr_SetString(PyExc_TypeError, "Array has the wrong type.");
    return nullptr;
  }

  $1 = func;
  $2 = static_cast<int64_t*>(PyArray_DATA(arr));
  $3 = PyArray_DIMS(arr)[0];
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
  import_array();
}

%include "ValuePromise.h"
%include "EmvmExecutor.h"
%include "EmvmBuilder.h"
