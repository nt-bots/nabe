#include "nav_parser.h"

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

#include "nabe_keyvalues.h"
#include "print_helpers.h"

bool SignalWasCaughtInPython() { return PyErr_CheckSignals() != 0; }

bool NavParser::Parse(const char* file_path, const char* maps_path, const char* navs_path, NABE_KeyValues& out_keyvalues)
{
	// Import the Python module
	auto py_path = PySys_GetObject("path");
	if (py_path == NULL) {
		print(Error, "Failed to get Python \"path\"");
		return false;
	}

	PyList_Append(py_path, PyUnicode_FromString("scripts"));
	auto module_name = "nav_parser";
	auto py_module = PyImport_ImportModule(module_name);
	if (py_module == NULL || !PyModule_Check(py_module)) {
		print(Error, "Failed to import Python module: %s", module_name);
		PyErr_Print();
		Py_DECREF(py_path);
		return false;
	}

	// Find the Python parser function
	auto function_name = "nav_parse_c_bridge";
	auto py_func = PyObject_GetAttrString(py_module, function_name);
	if (py_func == NULL || !PyFunction_Check(py_func)) {
		print(Error, "Failed to find Python function: %s", function_name);
		PyErr_Print();
		Py_DECREF(py_path);
		return false;
	}

	// Set up arguments for the Python function call
	const auto make_into_pyargs = {
		file_path,
		maps_path,
		navs_path
	};

	auto py_args = PyTuple_New(make_into_pyargs.size());

	int pyarg_i = 0;
	for (auto& p : make_into_pyargs) {
		auto new_py_value = PyUnicode_FromString(p);
		if (PyTuple_SetItem(py_args, pyarg_i++, new_py_value) != 0) {
			Py_DECREF(py_path);
			Py_DECREF(py_module);
			Py_DECREF(py_func);
			Py_XDECREF(py_args); // using XDECREF in case this is NULL
			Py_XDECREF(new_py_value); // using XDECREF in case this is NULL
			print(Error, "Failed to PyTuple_SetItem for args <-> value");
			PyErr_Print();
			return false;
		}
	}

	// Call away!
	auto py_call_res = PyObject_CallObject(py_func, py_args);
	Py_DECREF(py_func);
	Py_DECREF(py_args);
	if (py_call_res == NULL) {
		print(Error, "Python call failed");
		PyErr_Print();
		return false;
	}
	//print(Info, "Returning from Python.");

	bool success = out_keyvalues.InitializeFrom(py_call_res);
	Py_DECREF(py_call_res);
	return success;
}