#include "print_helpers.h"

#include <cstdarg>
#include <cstring>
#include <iostream>
#include <string>

#include "NabeConfig.h" // Generated to "binarypath/include" by CMake.

// This grabs the CMake generated version from the builds header above.
#define GET_VERSION_STRING() CMAKEVAR_TOSTR1(nabe_VERSION)
// Get the repo commit hash of this release.
#define GET_COMMIT_STRING() CMAKEVAR_TOSTR1(NABE_COMMIT)

#define CMAKEVAR_TOSTR1(X) CMAKEVAR_TOSTR2(X)
#define CMAKEVAR_TOSTR2(X) #X

void show_ascii_logo()
{
    constexpr const char* version = GET_VERSION_STRING();
    constexpr const char* commit_hash = GET_COMMIT_STRING();

	// Whitespace formatting
	constexpr size_t v_pos = 21; // position index where to place the 'V', after the \t, on logo line 3
	char spaces[v_pos + 1]{ 0 };
	// Pad third line after version by this many spaces, so the 'V' slots into the right place
    for (size_t i = 0; i < v_pos - strlen(version) - 1; ++i) {
		if (i >= sizeof(spaces)) {
			break;
		}
		spaces[i] = ' ';
	}

	std::cout << "\t                    __" << std::endl;
	std::cout << "\t{ N A B E }      __/  \\ /`" << std::endl;
    std::cout << "\t " << "v." << version << spaces <<
		// Omit the logo 'V' if there's no room to display it
        (strlen(spaces) + strlen(version) < v_pos ? "V" : "") << std::endl;
	std::cout << "  -- Navigate A-->B Externally --" << std::endl << std::endl;
	std::cout << "(Commit: " << commit_hash << ")" << std::endl;
}

void print(LogLevel loglvl, const char* to_be_formatted, ...)
{
	if (loglvl == LogLevel::NewLine) {
		std::cout << std::endl;
		return;
	}

	constexpr bool warnings_are_errors = true;
	if (warnings_are_errors && loglvl == LogLevel::Warning) {
		loglvl = LogLevel::Error;
	}

	std::string formatted_msg = "";

    va_list args;
	va_start(args, to_be_formatted);

    // We do vsnprintf twice, and it may mutate the state of args,
    // so need an extra copy of it.
    va_list args_copy;
    va_copy(args_copy, args);

    size_t len = static_cast<size_t>(vsnprintf(NULL, 0, to_be_formatted, args) + 1);

	if (len > 0)
	{
		formatted_msg.resize(len);
        vsnprintf(&formatted_msg[0], len, to_be_formatted, args_copy);
	}
    va_end(args);
    va_end(args_copy);

	// Helpers for prepending log messages and errors with specific character symbols, depending on error level.
	struct LogLevelPrefix {
		const char* info = "* ";
		const char* error = "!! ";
		const char* warning = "?? ";
	};
	static LogLevelPrefix prefix;

	switch (loglvl) {
	case LogLevel::Info:
		std::cout << prefix.info << formatted_msg << std::endl;
		break;
	case LogLevel::Error:
		std::cerr << prefix.error << formatted_msg << std::endl;
		break;
	case LogLevel::Warning:
		std::cout << prefix.warning << formatted_msg << std::endl;
		break;
	default:
		std::cout << formatted_msg;
		break;
	}
}
