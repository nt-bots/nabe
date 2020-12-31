#include "nabe_keyvalues.h"

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

#include "nabe_filesystem.h"
#include "print_helpers.h"

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include <iosfwd>
#include <ios>
#include <stdlib.h>
#include <iostream>

//#include <xiosbase>

NABE_KeyValues::NABE_KeyValues(const std::string text)
{
	InitializeFromText(text.c_str());
}

NABE_KeyValues::NABE_KeyValues(const char* text)
{
	InitializeFromText(text);
}

NABE_KeyValues::NABE_KeyValues(PyObject* dict)
{
	InitializeFrom(dict);
}

bool NABE_KeyValues::InitializeFrom(PyObject* dict)
{
	if (!Py_IsInitialized()) {
		print(Error, "Python environment must be initialized to construct KeyValues from PyObject*");
		return false;
	}
	else if (!dict) {
		print(Error, "PyObject* was nullptr");
		return false;
	}
	if (!PyDict_Check(dict)) {
		print(Error, "PyObject* was not a dict object or an instance of a subtype of the dict type.");
		return false;
	}

	//print(Info, "PyObject* dict size: %d", PyDict_Size(dict));

	auto dict_repr = PyObject_Repr(dict);
	if (dict_repr == NULL) {
		print(Error, "Failed to get PyObject* dict representation.");
		return false;
	}
	InitializeFromText(PyUnicode_AsUTF8(dict_repr));
	Py_DECREF(dict_repr);
	return true;
}

// Returns a random filename by reference, ending in ".kv".
// Expects length of at least 4.
// "out" will be a nullptr on failure.
void GetRandomKvTempFilename(char* out, size_t len)
{
	if (len < 4) {
		out = nullptr;
		return;
	}
	// A bunch of valid characters to use in a filename.
	const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
	// The file extension we want.
	const std::string ext = ".kv";
	// Randomly populate the filename char array.
	size_t i = 0;
	for (i = 0; i < len - ext.size() - 1; ++i) {
		// Something vaguely pseudorandom is good enough here.
		srand(static_cast<unsigned int>((std::time(0) + i) / (1 + i)));
		out[i] = chars[rand() % chars.size()];
	}
	// Complete the filename with the extension.
	out[i++] = '.';
	out[i++] = 'k';
	out[i++] = 'v';
	out[i] = '\0';
}

// Recurse a single key section of a multimap
void NABE_KeyValues::Recurse(KV::KeyValues* kv, KV::KeyValues* prev)
{
	if (!kv) {
		return;
	}

	int i = 0;
	for (auto it = kv->begin(); it != kv->end(); ++it) {

		std::string section{ kv->getKey() };
		std::string key{ it.it->first };
		std::string value{ kv->getKeyValue(key) };
		if (value.length() > 0) {
			value = kv->getKeyValue(key, i++);
		}

#if(0)
		print(Info, "--------------------------------------------");
		print(Info, "Section: \"%s\" | Key: \"%s\" | Value: \"%s\"", section.c_str(), key.c_str(), value.c_str());
		print(Info, "--------------------------------------------");
#endif

		const auto sizes = {
			section.size(),
			key.size(),
			value.size(),
		};
		for (auto& size : sizes) {
			if (size >= SKV_BUFFER_SIZE) {
				print(Error, "%s: SKV size overflow (%d >= %d", __FUNCTION__, size, SKV_BUFFER_SIZE);
				print(RawText, "Section: \"%s\" (size: %d)\nKey: \"%s\" (size: %d)\nValue: \"%s\" (size: %d)\n",
					section.c_str(), section.size(),
					key.c_str(), key.size(),
					value.c_str(), value.size()
				);
				return;
			}
		}

		SectionKeyValue skv;
#ifdef _WIN32
		if (strncpy_s(skv.section, section.c_str(), SKV_BUFFER_SIZE - 1) != 0)
#else
		if (!strncpy(skv.section, section.c_str(), SKV_BUFFER_SIZE - 1))
#endif
		{
			print(Error, "%s: Failed to copy KeyValues \"section\" string.", __FUNCTION__);
			return;
		}

#ifdef _WIN32
		if (strncpy_s(skv.key, key.c_str(), SKV_BUFFER_SIZE - 1) != 0)
#else
		if (!strncpy(skv.key, key.c_str(), SKV_BUFFER_SIZE - 1))
#endif
		{
			print(Error, "%s: Failed to copy KeyValues \"key\" string.", __FUNCTION__);
			return;
		}

#ifdef _WIN32
		if (strncpy_s(skv.value, value.c_str(), SKV_BUFFER_SIZE - 1) != 0)
#else
		if (!strncpy(skv.value, value.c_str(), SKV_BUFFER_SIZE - 1))
#endif
		{
			print(Error, "%s: Failed to copy KeyValues \"value\" string.", __FUNCTION__);
			return;
		}

		skvs.push_back(skv);

		if (it.it->second.get() && it.it->second.get() != kv) {
			Recurse(it.it->second.get(), kv);
		}
	}
}

void NABE_KeyValues::InitializeFromText(const char* text)
{
	if (!text || *text == '\0') {
		print(Error, "%s: Failed to read input.", __FUNCTION__);
		return;
	}

	fs::path temp_file_path;
	char random_filename[20]{ 0 };
	std::ofstream ofs;

	// Temp file creation
	{
		// Get the system temp file path, and make our subfolder there
		fs::path temp_dir{ fs::temp_directory_path() };
		if (!fs::is_directory(temp_dir)) {
			print(Error, "%s: Failed to find temp file directory.", __FUNCTION__);
			return;
		}
		temp_dir /= "nabenabe_kvs";
		if (!fs::is_directory(temp_dir)) {
			if (!fs::create_directory(temp_dir) || !fs::is_directory(temp_dir)) {
				print(Error, "%s: Failed to create folder(s) in temp directory.", __FUNCTION__);
				return;
			}
		}

		// Generate a new unique temp filename to use
		bool have_unique_filename = false;
		size_t num_unique_filename_tries = 0;
		constexpr size_t max_unique_filename_tries = 1000;
		while (!have_unique_filename) {
			if (++num_unique_filename_tries >= max_unique_filename_tries) {
				print(Error, "%s: Failed to find a unique temp filename after %d tries.", __FUNCTION__, num_unique_filename_tries);
				return;
			}

			GetRandomKvTempFilename(random_filename, sizeof(random_filename));
			if (!random_filename) {
				print(Error, "%s: Failed to get random filename.", __FUNCTION__);
				return;
			}

			bool filename_clashed = false;
			for (auto& p : fs::directory_iterator(temp_dir)) {
#if(0)
				std::cout << "Comparing " <<
					p.path().filename() <<
					" vs " <<
					random_filename << "\n";
#endif

				auto iterated_string = (p.path().filename().string());
				auto filename_string = std::string(random_filename);

				if (iterated_string == filename_string) {
					filename_clashed = true;
					break;
				}
			}
			have_unique_filename = !filename_clashed;
		}
		// This should never happen.
		if (!have_unique_filename) {
			print(Error, "%s: have_unique_filename returned false. This is a bug, please report it.", __FUNCTION__);
			return;
		}

		temp_file_path = temp_dir;
		temp_file_path /= random_filename;

		// Make the new temp file, and open it for writing
		if (!fs::exists(temp_file_path)) {
			const auto mode = (std::ios::out | std::ios::trunc);
			ofs.open(temp_file_path, mode);
			if (!ofs.is_open()) {
				print(Error, "%s: Failed to open temp file: \"%s\"", __FUNCTION__, random_filename);
				return;
			}
		}
	}

	// Temp file is ready for writing now
	{
		// Write kv to file
		ofs << text;
		// Close the file
		ofs.flush();
		ofs.close();

#ifdef _WIN32
		// Windows thinks it's a good idea to return this as wchar_t,
		// so have to kludge some kind of type conversion here ¯\_(ツ)_/¯
		auto path_wide = temp_file_path.c_str();
		char path_c_str[260]{ 0 };
		if (sprintf_s(path_c_str, "%ws", path_wide) <= 0) {
			print(Error, "%s: Failed to get filepath as C string.", __FUNCTION__);
			return;
		}
#else
		auto path_c_str = temp_file_path.c_str();
		if (!path_c_str || (*path_c_str) == '\0') {
			print(Error, "%s: Failed to get filepath as C string.", __FUNCTION__);
			return;
		}
#endif
		auto kv = KeyValues::parseKVFile(path_c_str);
		Recurse(&kv);
	}

	// Temp file cleanup
	{
		if (ofs.is_open()) {
			print(Error, "%s: Failed to close previously opened temporary file: \"%s\"", __FUNCTION__, random_filename);
			return;
		}
		// And then remove it
		else if (!fs::remove(temp_file_path)) {
			// For whatever reasons this can fail, at least on Windows, so trying to remove the hard way.
			print(Error, "%s: Failed to remove temporary file: \"%s\"", __FUNCTION__, random_filename);
			return;
		}
		else if (fs::exists(temp_file_path)) {
			print(Error, "%s: Called remove on temporary file, but it still exists: \"%s\"", __FUNCTION__, random_filename);
			return;
		}
	}
}
