#ifndef NABENABE_PYTHON_AUTO_INITIALIZER_H
#define NABENABE_PYTHON_AUTO_INITIALIZER_H

// Helper for initializing and finalizing the Python environment
// automatically, based on this object instance's lifetime.
class PythonAutoInitializer {
public:
	PythonAutoInitializer();
	~PythonAutoInitializer();

	bool IsPythonReady() const;
};

#endif // NABENABE_PYTHON_AUTO_INITIALIZER_H