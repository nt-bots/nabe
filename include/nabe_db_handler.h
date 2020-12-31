#ifndef NABENABE_DATABASE_HANDLER_H
#define NABENABE_DATABASE_HANDLER_H

#include "thirdparty/sqlite-amalgamation/sqlite3.h"

#include "print_helpers.h"
#include "nabe_gamemap.h"
#include "nabe_pathfinder.h"

#include <chrono>
#include <list>

// Will wait for this many seconds between pathfinding runs.
static constexpr int LOOP_SLEEP_SECONDS = 1;
static constexpr auto jobs_table_identifier = "nabejobs";
static constexpr auto solutions_table_identifier = "nabesols";

static bool job_table_exists = false;
static bool solution_table_exists = false;
static sqlite3* db = nullptr;

static fs::path _map_folder_path;
static NABE_PathFinder* ptr_pathfinder = nullptr;
static size_t num_jobs_completed_this_loop = 0;

// Retry this many time if the db was busy.
static size_t _max_retries = 0;

// Whether to print some extra informational debug.
static bool _solver_verbosity = false;

// We can't handle too large batches at a time, because it can hang the SourceMod side if the
// SM db handle needs to perform a task that doesn't have multithreaded implementation.
// And this would hang the actual game server, also.
static size_t _max_solves_at_once = 1;

typedef int (*sql_callback)(void* not_used, int argc, char** argv, char** az_col_name);
struct NabePendingSqlJob {
	std::string query;
	std::string table;
	sql_callback callback;
};
std::list<NabePendingSqlJob> pending_sql_queries;
static std::string* current_table;
static std::list<std::pair<NABE_GameMap*, std::pair<int, int>>> pending_paths_to_solve;

static char* _query = nullptr;
static constexpr size_t _query_max_size = 100 * 1024;

static void AddPendingSqlQuery(const char* sql_query, const char* table_name, const sql_callback sqlite_cb)
{
	NabePendingSqlJob query;
	query.query = sql_query;
	query.table = table_name;
	query.callback = sqlite_cb;
	pending_sql_queries.push_back(query);
}

static bool SqlQuery(const char* sql_query, const sql_callback sqlite_cb)
{
	char* error_msg = nullptr;
	if (sqlite3_exec(db, sql_query, sqlite_cb, 0, &error_msg) != SQLITE_OK) {
		print(Error, "SQL error:\t%s", error_msg);
		sqlite3_free(error_msg);
		return false;
	}
	return true;
}

static int callback_job_table_exists(void*, int argc, char** argv, char** az_col_name)
{
	const bool sql_success = (argc == 1 && *argv[0]);
	job_table_exists = (sql_success && (atoi(argv[0]) == 1));
	return sql_success ? SQLITE_OK : SQLITE_ERROR;
}

static int callback_solution_table_exists(void*, int argc, char** argv, char** az_col_name)
{
	const bool sql_success = (argc == 1 && *argv[0]);
	solution_table_exists = (sql_success && (atoi(argv[0]) == 1));
	return sql_success ? SQLITE_OK : SQLITE_ERROR;
}

static int callback_handle_map_jobs(void*, int argc, char** argv, char** az_col_name);

static int callback_get_jobs(void*, int argc, char** argv, char** az_col_name)
{
	if (argc != 5) {
		return SQLITE_ERROR;
	}

	constexpr size_t query_max_size = 1024;
	char query[query_max_size]{ 0 };
	snprintf(query, query_max_size, "SELECT from_area_x, from_area_y, from_area_z, "
		"to_area_x, to_area_y, to_area_z "
		"FROM %s "
		"ORDER BY epoch DESC " // return newest results first, since those are probably most relevant to solve
		"LIMIT %zd;", // see comment about reasoning for limit above
		
		argv[2], _max_solves_at_once);

	AddPendingSqlQuery(query, argv[2], &callback_handle_map_jobs);

	return SQLITE_OK;
}

static bool solution_exists = false;
static int callback_solution_exists(void*, int argc, char** argv, char** az_col_name)
{
	solution_exists = ((argc == 1) && (atoi(argv[0]) == 1));
	return SQLITE_OK;
}

static int sqlite_busy_handler(void* data, int num)
{
	static auto num_tries = _max_retries;
	num_tries -= num;

	if (num_tries <= 0) {
		num_tries = _max_retries;
		print(Warning, "DB was busy. Gave up after %d retries.", _max_retries);
		return 0;
	}
	else {
		constexpr auto milliseconds_in_one_second = 1000;
		constexpr auto busy_sleep_seconds = 1;
		sqlite3_sleep(busy_sleep_seconds * milliseconds_in_one_second);
	}

	//print(Info, "DB was busy, trying again. Previous attempts: %d", num);
	return static_cast<int>(_max_retries);
}

// Return Unix epoch
size_t GetEpoch()
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock().now().time_since_epoch()).count();
}

class NABE_DatabaseHandler
{
public:
	NABE_DatabaseHandler(NABE_PathFinder* pathfinder, const char* db_path, const char* map_folder_path,
		size_t max_retries, bool solver_verbosity, size_t max_solves_at_once,
		bool create_db_if_not_exists = false)
	{
		if (_query != nullptr) {
			print(Error, "%s: Query variable is leaking memory", __FUNCTION__);
			return;
		}
		else {
			_query = new char[_query_max_size];
		}

		ptr_pathfinder = pathfinder;
		_map_folder_path = map_folder_path;
		_max_retries = max_retries;
		_solver_verbosity = solver_verbosity;
		_max_solves_at_once = max_solves_at_once;

		if (!fs::exists(fs::path(db_path)) && !create_db_if_not_exists) {
			print(Error, "%s:  Database path doesn't exist: \"%s\"", __FUNCTION__, db_path);
			return;
		}

		print(Info, "Opening SQLite database location: \"%s\"", db_path);
		if (sqlite3_open(db_path, &db) != SQLITE_OK) {
			print(Error, "%s: Can't open database: %s", __FUNCTION__, sqlite3_errmsg(db));
			sqlite3_close(db);
			return;
		}

		sqlite3_busy_handler(db, &sqlite_busy_handler, NULL);

		print(Info, "Cleaning up any dirty navigation data from database...");
		sqlite3_exec(db, "VACUUM", 0, 0, 0);
	}

	~NABE_DatabaseHandler()
	{
		print(Info, "Cleaning up any dirty navigation data from database...");
		sqlite3_exec(db, "VACUUM", 0, 0, 0);

		print(Info, "Closing database connection...");
		sqlite3_close(db);

		delete[] _query;
		_query = nullptr;
	}

	void HandlePendingQueries()
	{
		for (auto& p : pending_sql_queries) {
			current_table = &p.table;
			SqlQuery(p.query.c_str(), p.callback);
		}
		pending_sql_queries.clear();
	}

	// Wait, so that we don't needlessly spend cycles when there's no work.
	void Sleep()
	{
		// Only sleep if there haven't been any pathfinding jobs for us recently.
		if (num_jobs_completed_this_loop == 0) {
			constexpr auto milliseconds_in_one_second = 1000;
			sqlite3_sleep(LOOP_SLEEP_SECONDS * milliseconds_in_one_second);
		}
	}

	bool AddMap(const NABE_GameMap* map)
	{
		if (!db) {
			print(Error, "%s: Database is not open.", __FUNCTION__);
			return false;
		}

		// Jobs table
		{
			constexpr auto schema =
				"CREATE TABLE IF NOT EXISTS %s_%zd_%s(\n"
				"\tepoch INTEGER NOT NULL,\n"
				"\tfrom_area_x REAL NOT NULL,\n"
				"\tfrom_area_y REAL NOT NULL,\n"
				"\tfrom_area_z REAL NOT NULL,\n"
				"\tto_area_x REAL NOT NULL,\n"
				"\tto_area_y REAL NOT NULL,\n"
				"\tto_area_z REAL NOT NULL,\n"
				"\tUNIQUE (from_area_x, from_area_y, from_area_z, "
				"to_area_x, to_area_y, to_area_z)\n);";

			constexpr size_t query_max_size = 1024;
			char query[query_max_size]{ 0 };
			snprintf(query, query_max_size, schema,
				jobs_table_identifier,
				map->map_size,
				map->map_name.c_str());

			SqlQuery(query, NULL);

			if (!JobTableExists(map)) {
				print(Error, "%s: Failed to create job table", __FUNCTION__);
				return false;
			}
			else {
				if (_solver_verbosity) {
					print(Info, "%s: Job table exists for: %s", __FUNCTION__, map->map_name.c_str());
				}
			}
		}

		// Solution tables
		{
			constexpr auto schema =
				"CREATE TABLE IF NOT EXISTS %s_%zd_%s(\n"
				"\tepoch INTEGER NOT NULL,\n"
				"\tfrom_area_x REAL NOT NULL,\n"
				"\tfrom_area_y REAL NOT NULL,\n"
				"\tfrom_area_z REAL NOT NULL,\n"
				"\tto_area_x REAL NOT NULL,\n"
				"\tto_area_y REAL NOT NULL,\n"
				"\tto_area_z REAL NOT NULL,\n"
				"\tstep_num INTEGER NOT NULL,\n"
				"\tpass_area_x REAL NOT NULL,\n"
				"\tpass_area_y REAL NOT NULL,\n"
				"\tpass_area_z REAL NOT NULL,\n"
				"\tUNIQUE(from_area_x, from_area_y, from_area_z, "
				"to_area_x, to_area_y, to_area_z, step_num)\n);";

			constexpr size_t query_max_size = 1024;
			char query[query_max_size]{ 0 };
			snprintf(query, query_max_size, schema,
				solutions_table_identifier,
				map->map_size,
				map->map_name.c_str());

			SqlQuery(query, NULL);

			if (!SolutionsTableExists(map)) {
				print(Error, "%s: Failed to create solution table", __FUNCTION__);
				return false;
			}
			else {
				if (_solver_verbosity) {
					print(Info, "%s: Solutions table exists for: %s", __FUNCTION__, map->map_name.c_str());
				}
			}
		}

		return true;
	}

	void GetJobs()
	{
		num_jobs_completed_this_loop = 0;

		constexpr size_t query_max_size = 1024;
		char query[query_max_size]{ 0 };
		snprintf(query, query_max_size, "SELECT * FROM sqlite_master WHERE type='table' AND name LIKE '%s_%%';",
			jobs_table_identifier);

		SqlQuery(query, &callback_get_jobs);
	}

	void Debug_AddJob(const NABE_GameMap* map, const Vector& pos_from, const Vector& pos_to)
	{
		constexpr auto schema = "INSERT INTO %s_%zd_%s "
			"(epoch, from_area_x, from_area_y, from_area_z, to_area_x, to_area_y, to_area_z) "
			"VALUES (%zd, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f);";

		constexpr size_t query_max_size = 1024;
		char query[query_max_size]{ 0 };
		snprintf(query, query_max_size, schema,
			jobs_table_identifier,
			map->map_size,
			map->map_name.c_str(),
			GetEpoch(),
			pos_from.x, pos_from.y, pos_from.z,
			pos_to.x, pos_to.y, pos_to.z);

		SqlQuery(query, NULL);
	}

	static bool RequestSolve(const char* table_name, const Vector& pos_from, const Vector& pos_to, std::list<CNavArea*>& solution)
	{
		if (!ptr_pathfinder) {
			print(Error, "%s: Pathfinder pointer is null", __FUNCTION__);
			return false;
		}

		// Extract map name from the table name
		std::string map_name_buffer{ table_name };
		auto id_ext_pos = map_name_buffer.find(jobs_table_identifier);
		if (id_ext_pos != std::string::npos) {
			map_name_buffer.replace(id_ext_pos, strlen(jobs_table_identifier) + 1, "");
		}
		auto filesize_end_pos = map_name_buffer.find("_");
		if (filesize_end_pos != std::string::npos) {
			map_name_buffer.replace(0, filesize_end_pos + 1, "");
		}
	
		return ptr_pathfinder->Solve(map_name_buffer, pos_from, pos_to, solution);
	}

private:
	bool JobTableExists(const NABE_GameMap* map)
	{
		if (!map) {
			print(Error, "%s: !map", __FUNCTION__);
			return false;
		}

		constexpr size_t query_max_size = 1024;
		char query[query_max_size]{ 0 };
		snprintf(query, query_max_size, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name ='%s_%zd_%s';",
			jobs_table_identifier,
			map->map_size,
			map->map_name.c_str());

		SqlQuery(query, &callback_job_table_exists);

		return job_table_exists;
	}

	bool SolutionsTableExists(const NABE_GameMap* map)
	{
		if (!map) {
			print(Error, "%s: !map", __FUNCTION__);
			return false;
		}

		constexpr size_t query_max_size = 1024;
		char query[query_max_size]{ 0 };
		snprintf(query, query_max_size, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name ='%s_%zd_%s';",
			solutions_table_identifier,
			map->map_size,
			map->map_name.c_str());

		SqlQuery(query, &callback_solution_table_exists);

		return solution_table_exists;
	}
};

static int callback_handle_map_jobs(void*, int argc, char** argv, char** az_col_name)
{
	const Vector pos_from(
		static_cast<float>(atof(argv[0])),
		static_cast<float>(atof(argv[1])),
		static_cast<float>(atof(argv[2]))
	);

	const Vector pos_to(
		static_cast<float>(atof(argv[3])),
		static_cast<float>(atof(argv[4])),
		static_cast<float>(atof(argv[5]))
	);

	if (!pos_from.IsValid() || !pos_to.IsValid()) {
		print(Error, "%s: Invalid position vectors(s)", __FUNCTION__);
		return SQLITE_ERROR;
	}

	if (_solver_verbosity) {
		print(Info, "%s: %s: Area (%.1f %.1f %.1f) --> (%.1f %.1f %.1f)",
			__FUNCTION__, current_table->c_str(),
			pos_from.x, pos_from.y, pos_from.z,
			pos_to.x, pos_to.y, pos_to.z);
	}

	std::string solutions_table = *current_table;
	auto id_ext_pos = solutions_table.find(jobs_table_identifier);
	if (id_ext_pos != std::string::npos) {
		solutions_table.replace(id_ext_pos, strlen(jobs_table_identifier), solutions_table_identifier);
	}

	// Check if solution already exists.
	snprintf(_query, _query_max_size, "SELECT EXISTS(SELECT * FROM %s WHERE "
		"from_area_x = %.1f AND from_area_y = %.1f AND from_area_z = %.1f AND "
		"to_area_x = %.1f AND to_area_y = %.1f AND to_area_z = %.1f);",
		solutions_table.c_str(),
		pos_from.x, pos_from.y, pos_from.z,
		pos_to.x, pos_to.y, pos_to.z);
	SqlQuery(_query, &callback_solution_exists);

	// No solution cached in database; need to solve this.
	if (!solution_exists) {
		std::list<CNavArea*> solution;
		if (NABE_DatabaseHandler::RequestSolve(current_table->c_str(), pos_from, pos_to, solution)) {
			snprintf(_query, _query_max_size, "INSERT INTO %s", solutions_table.c_str());

			size_t i = 0;
			size_t num_passes = solution.size();
			constexpr size_t append_max_size = 1024;
			char append[append_max_size]{ 0 };
			auto epoch = GetEpoch();
			for (auto& p : solution) {
				if (i == 0) {
					snprintf(append, append_max_size, " SELECT %zd AS 'epoch', %.1f AS 'from_area_x', %.1f AS 'from_area_y', %.1f AS 'from_area_z', "
						"%.1f AS 'to_area_x', %.1f AS 'to_area_y', %.1f AS 'to_area_z', %zd AS 'step_num', "
						"%.1f AS 'pass_area_x', %.1f AS 'pass_area_y', %.1f AS 'pass_area_z'",
						epoch,
						pos_from.x, pos_from.y, pos_from.z,
						pos_to.x, pos_to.y, pos_to.z,
						i,
						p->GetCenter().x, p->GetCenter().y, p->GetCenter().z);
				}
				else {
					snprintf(append, append_max_size, " UNION ALL SELECT %zd, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %zd, %.1f, %.1f, %.1f",
						epoch,
						pos_from.x, pos_from.y, pos_from.z,
						pos_to.x, pos_to.y, pos_to.z,
						i,
						p->GetCenter().x, p->GetCenter().y, p->GetCenter().z);
				}
				snprintf(_query, _query_max_size, "%s%s", _query, append);
				++i;
			}
			snprintf(_query, _query_max_size, "%s%c", _query, ';');

			NabePendingSqlJob job;
			job.query = std::string(_query);

			SqlQuery(job.query.c_str(), NULL);
			++num_jobs_completed_this_loop;
		}
	}

	// This job is completed, so we can delete the row.
	snprintf(_query, _query_max_size, "DELETE FROM %s WHERE from_area_x = %.1f AND from_area_y = %.1f AND from_area_z = %.1f AND "
		"to_area_x = %.1f AND to_area_y = %.1f AND to_area_z = %.1f;",
		current_table->c_str(),
		pos_from.x, pos_from.y, pos_from.z,
		pos_to.x, pos_to.y, pos_to.z);
	SqlQuery(_query, NULL);

	return SQLITE_OK;
}

#endif // NABENABE_DATABASE_HANDLER_H
