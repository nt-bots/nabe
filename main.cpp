/*	Some of the code in this project is based on the Source 1 engine, and is used under the SOURCE 1 SDK LICENSE.
	Source 1 SDK repository: https://github.com/ValveSoftware/source-sdk-2013
	Please see the SOURCE 1 SDK license terms below. */

/*
				SOURCE 1 SDK LICENSE

Source SDK Copyright(c) Valve Corp.

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE
CORPORATION ("Valve").  PLEASE READ IT BEFORE DOWNLOADING OR USING
THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE
SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO
THE TERMS OF THIS LICENSE PLEASE DON'T DOWNLOAD OR USE THE SDK.

  You may, free of charge, download and use the SDK to develop a modified Valve game
running on the Source engine.  You may distribute your modified Valve game in source and
object code form, but only for free. Terms of use for Valve games are found in the Steam
Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/

  You may copy, modify, and distribute the SDK and any modifications you make to the
SDK in source and object code form, but only for free.  Any distribution of this SDK must
include this LICENSE file and thirdpartylegalnotices.txt.

  Any distribution of the SDK or a substantial portion of the SDK must include the above
copyright notice and the following:

	DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY
	OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED
	"AS IS".  VALVE AND ITS SUPPLIERS DISCLAIM ALL
	WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS
	OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED
	WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT,
	TITLE AND FITNESS FOR A PARTICULAR PURPOSE.

	LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR
	ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
	INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER
	(INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF
	BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
	BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
	ARISING OUT OF THE USE OF OR INABILITY TO USE THE
	ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.


If you would like to use the SDK for a commercial purpose, please contact Valve at
sourceengine@valvesoftware.com.
*/

#if(1)
#ifdef _WIN32
#define NOMINMAX
#endif
#endif

#include "thirdparty/sqlite-amalgamation/sqlite3.h"
#include "thirdparty/source-sdk-stubs/nav_area.h"
#include "thirdparty/LeksysINI/iniparser.hpp"

#include "interrupt_handler.h"
#include "print_helpers.h"
#include "nabe_db_handler.h"
#include "nabe_pathfinder.h"
#include "python_auto_initializer.h"
#include "nabe_gamemap.h"

#ifdef _WIN32
#include <windows.h>
#else // Linux
#include <fcntl.h> // For O_* constants
#include <sys/stat.h> // For mode constants
#include <semaphore.h>
#include <errno.h>
constexpr auto semaphore_name = "/nabe_singleton";
static_assert(semaphore_name[0] == '/');

#include <stdio.h>
#endif

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <limits.h> // PATH_MAX
#endif

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#include <iostream>
#include <string>

#ifdef _WIN32
//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}
#endif

#ifdef __linux__
static sem_t* _semaphore;
#endif
bool EnsureAppSingleton()
{
#ifdef _WIN32
	CreateMutexA(0, FALSE, "Local\\$myprogram$");
	const auto error = GetLastError();
	if (error != NULL) {
		if (error == ERROR_ALREADY_EXISTS) {
			print(Error, "Only one instance of this program may run at the same time.");
		}
		else {
			print(Error, "Unexpected error occurred whilst creating an app mutex: \"%s\"",
				GetLastErrorAsString().c_str());
		}
	}
	return error == NULL;
#else
	_semaphore = sem_open(semaphore_name, O_CREAT | O_EXCL, 0, 0);
	if (_semaphore == SEM_FAILED) {
		if (errno == EEXIST) {
			print(Error, "Only one instance of this program may run at the same time.");
		}
		else {
			print(Error, "Semaphore open failed (errno %d)", errno);
		}
	}
	return _semaphore != SEM_FAILED;
#endif
}

// Entry point
int main(int argc, char** argv)
{
	int return_value = 0;

	show_ascii_logo();

	enum {
		ARG_APP = 0,
		ARG_CONFIG_FILE_PATH,

		NUM_RECOGNIZED_ARGS
	};

	if (argc > NUM_RECOGNIZED_ARGS) {
		char my_path[PATH_MAX];
#ifdef _WIN32
		if (GetModuleFileNameA(NULL, my_path, sizeof(my_path)) == 0) {
			print(Error, "%s: Failed to get the app name (%s)", __FUNCTION__, GetLastErrorAsString().c_str());
			return 1;
		}
#else // Linux
		if (readlink("/proc/self/exe", my_path, PATH_MAX) == -1) {
			print(Error, "%s: Failed to get the app name", __FUNCTION__);
			return 1;
		}
#endif

		print(Info, "Unrecognized argument count (%d).", argc);
		print(Info, "Usage: %s (optional config file path)", my_path);
		print(Info, "If no optional config file path is provided, config.ini will be looked for in the current working dir.");
		return 1;
	}

	print(Info, "Initializing...");

	if (!EnsureAppSingleton()) {
		return 1;
	}

	{
		char config_file_path[PATH_MAX]{ 0 };

		if (argc > 1) {
#ifdef _WIN32
			if (strncpy_s(config_file_path, argv[ARG_CONFIG_FILE_PATH], sizeof(config_file_path) - 1) != 0)
#else
			if (!strncpy(config_file_path, argv[ARG_CONFIG_FILE_PATH], sizeof(config_file_path) - 1))
#endif
			{
				print(Error, "%s: Failed to get config file path.", __FUNCTION__);
				return_value = 1;
				goto semaphore_cleanup;
			}
		}
		else {
			if (!GetCurrentDir(config_file_path, sizeof(config_file_path))) {
				print(Error, "%s Failed to get current working dir.", __FUNCTION__);
				return_value = 1;
				goto semaphore_cleanup;
			}
			constexpr auto config_file = "config.ini";

#ifdef _WIN32
			if (strncat_s(config_file_path, "\\", sizeof(config_file_path) - 1) != 0)
#else
			if (!strncat(config_file_path, "/", sizeof(config_file_path) - 1))
#endif
			{
				print(Error, "%s: Failed to concatenate to config path (1).", __FUNCTION__);
				return_value = 1;
				goto semaphore_cleanup;
			}

#ifdef _WIN32
			if (strncat_s(config_file_path, config_file, sizeof(config_file_path) - 1) != 0)
#else
			if (!strncat(config_file_path, config_file, sizeof(config_file_path) - 1))
#endif
			{
				print(Error, "%s: Failed to concatenate to config path (2).", __FUNCTION__);
				return_value = 1;
				goto semaphore_cleanup;
			}
		}

		INI::File ft;
		if (!ft.Load(config_file_path)) {
			print(Error, "%s: Failed to read config file from: \"%s\"", __FUNCTION__, config_file_path);
			return_value = 1;
			goto semaphore_cleanup;
		}

		const auto db_type = ft.GetSection("database")->GetValue("type").AsString();
		if (db_type.compare("sqlite3") != 0) {
			print(Error, "%s: Unsupported database type: \"%s\"", __FUNCTION__, db_type.c_str());
			return_value = 1;
			goto semaphore_cleanup;
		}

		const auto db_location = ft.GetSection("database")->GetValue("location").AsString();
		const auto max_retries = ft.GetSection("database")->GetValue("locked_max_retries").AsInt();
		const auto max_solves_at_once = ft.GetSection("database")->GetValue("max_solves_at_one_time").AsInt();
		const auto maps_folder_path = ft.GetSection("gameserver")->GetValue("maps_folder_path").AsString();
		const auto navs_folder_path = ft.GetSection("solver")->GetValue("navs_folder_path").AsString();
		const auto supported_maps = ft.GetSection("solver")->GetValue("supported_maps_list").AsArray();
		const auto solver_verbosity = ft.GetSection("solver")->GetValue("verbose_debug").AsBool();

		std::vector<NABE_GameMap*> maps;
		for (int i = 0; i < supported_maps.Size(); ++i) {
			auto map_name = supported_maps.GetValue(i).AsString();
			if (map_name.empty()) {
				for (auto& p : maps) {
					delete p;
				}
				print(Error, "%s: Received empty map name in config file's solver::supported_maps_list", __FUNCTION__);
				return_value = 1;
				goto semaphore_cleanup;
			}
			maps.push_back(new NABE_GameMap(maps_folder_path, map_name));
		}

		NABE_PathFinder pathfinder(maps_folder_path, navs_folder_path, solver_verbosity);

		NABE_DatabaseHandler db_handler(&pathfinder, db_location.c_str(), maps_folder_path.c_str(), max_retries, solver_verbosity, max_solves_at_once);

		PythonAutoInitializer pai;
		if (!pai.IsPythonReady()) {
			return_value = 1;
			goto semaphore_cleanup;
		}

		enable_interrupt_handler();

		print(Info, "Loading navigation data into memory. This may take a few moments...");
		size_t num_processed = 0;
		const size_t total_num_to_process = maps.size();
		for (auto& map : maps) {
			print(RawText, "** Processing navigation data for: \"%s\"...", map->map_name.c_str());
			if (solver_verbosity) {
				print(NewLine);
			}

			if (!db_handler.AddMap(map)) {
				break;
			}
			else if (!pathfinder.AddMap(map)) {
				break;
			}
			print(RawText, " Completed. (%d/%d)\n", ++num_processed, total_num_to_process);
		}

		if (num_processed != total_num_to_process) {
			print(Error, "%s: Failed to initialize nav data.", __FUNCTION__);
			return_value = 1;
			goto semaphore_cleanup;
		}
		else {
			print(Info, "Initialization complete. Now actively listening for navigation jobs.");
			print(Info, "Use system interrupt (Ctrl+C) to shut down.");
			while (!was_interrupted()) {
				db_handler.GetJobs();
				db_handler.HandlePendingQueries();
				db_handler.Sleep();
			}
		}
	}

	semaphore_cleanup: // TODO: refactor this jump out
	print(Info, "Shutting down.");
#ifdef __linux__
	if (_semaphore != SEM_FAILED) {
		if (sem_close(_semaphore) != 0) {
			print(Error, "%s: Failed to close semaphore (errno %d)", __FUNCTION__, errno);
		}
	}
	if (sem_unlink(semaphore_name) != 0) {
		if (_semaphore != SEM_FAILED) {
			print(Error, "%s: Failed to unlink semaphore (errno %d)", __FUNCTION__, errno);
		}
	}
#endif

	return return_value;
}
