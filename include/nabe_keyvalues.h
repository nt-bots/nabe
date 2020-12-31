#ifndef NABENABE_KEYVALUES_H
#define NABENABE_KEYVALUES_H

#include "thirdparty/KeyValues/src/keyvalues.hpp"

#include <string>
#include <vector>

struct _object;
typedef _object PyObject;

constexpr size_t SKV_BUFFER_SIZE = 44;
struct SectionKeyValue {
	char section[SKV_BUFFER_SIZE]{ 0 };
	char key[SKV_BUFFER_SIZE]{ 0 };
	char value[SKV_BUFFER_SIZE]{ 0 };
};

class NABE_KeyValues : KeyValues
{
	friend class KV::KeyValues; // TODO/HACK: breaking encapsulation here to get things together. should revisit later.
public:
	NABE_KeyValues() = default;
	NABE_KeyValues(const std::string text);
	NABE_KeyValues(const char* text);
	NABE_KeyValues(PyObject* py_keyvalues_obj);

	virtual ~NABE_KeyValues() = default;

	bool InitializeFrom(PyObject* py_keyvalues_obj);

	std::vector<SectionKeyValue>& GetSkvs() { return skvs; }

protected:
	void Recurse(KV::KeyValues* kv, KV::KeyValues* prev = nullptr);

private:
	void InitializeFromText(const char* text);

	std::vector<SectionKeyValue> skvs;
};

#endif // NABENABE_KEYVALUES_H