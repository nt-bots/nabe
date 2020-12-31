#include "python_auto_initializer.h"

// For all # variants of formats (s#, y#, etc.), the type of the length
// argument (int or Py_ssize_t) is controlled by defining the macro
// PY_SSIZE_T_CLEAN before including Python.h. If the macro was defined,
// length is a Py_ssize_t rather than an int. This behavior will change
// in a future Python version to only support Py_ssize_t and drop int
// support. It is best to always define PY_SSIZE_T_CLEAN.
// https://docs.python.org/3/c-api/arg.html#arg-parsing
#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif

// Use debug libraries for debug build?
#if(0)
#include <Python.h>
#else
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#endif

#include "print_helpers.h"

PythonAutoInitializer::PythonAutoInitializer()
{
	if (!IsPythonReady()) {
		print(Info, "Initializing the Python environment.");
		Py_Initialize();
		if (!IsPythonReady()) {
			print(Error, "Failed to initialize the embedded Python interpreter.");
		}
	}
}

PythonAutoInitializer::~PythonAutoInitializer()
{
	if (IsPythonReady()) {
		print(Info, "Cleaning up the Python environment.");
		Py_Finalize();
		if (IsPythonReady()) {
			print(Error, "Failed to clean up the embedded Python interpreter.");
		}
	}
}

bool PythonAutoInitializer::IsPythonReady() const
{
	return Py_IsInitialized() != 0;
}
