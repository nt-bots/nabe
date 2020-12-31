#ifndef _NABENABE_NABE_NAV_COORDINATOR_H
#define _NABENABE_NABE_NAV_COORDINATOR_H

#include "thirdparty/source-sdk-stubs/nav.h"

#include "nabe_area.h"
#include "nabe_gamemap.h"

#include <vector>
#include <string>

class NABE_Area;
class NABE_PathFinder;

class NABE_NavCoordinator
{
	friend class NABE_PathFinder;
public:
	NABE_NavCoordinator(NABE_PathFinder* owner, const std::string& map_name, const char* maps_path, const char* navs_path);
	~NABE_NavCoordinator();

private:
	bool LoadMapNavData();
	size_t GetBspSize(const std::string& map_name);

	bool SetApproachInfo_ThisAreaId(NABE_Area* target, const size_t area_n, const int id = AREA_ID_NONE);
	bool SetApproachInfo_PrevAreaId(NABE_Area* target, const size_t area_n, const int id = AREA_ID_NONE);
	bool SetApproachInfo_NextAreaId(NABE_Area* target, const size_t area_n, const int id = AREA_ID_NONE);

	bool SetSpotEncounter_FromArea(SpotEncounter* target, const int id = 0);
	bool SetSpotEncounter_ToArea(SpotEncounter* target, const int id = 0);

	NABE_Area* GetAreaById(const int id);
	NABE_Area* GetAreaByPos(const Vector& pos);

private:
	NABE_PathFinder* m_owner;

	std::vector<NABE_Area*> m_areas;

	std::list<std::pair<NABE_Area*, int>> m_pending_area_connections_north;
	std::list<std::pair<NABE_Area*, int>> m_pending_area_connections_east;
	std::list<std::pair<NABE_Area*, int>> m_pending_area_connections_south;
	std::list<std::pair<NABE_Area*, int>> m_pending_area_connections_west;

	std::list<std::pair<NABE_Area*, SpotEncounter*>> m_pending_encounter_spots;

	NABE_GameMap* m_map;
	bool m_loaded;

	const char* m_maps_path;
	const char* m_navs_path;
};

#endif // _NABENABE_NABE_NAV_COORDINATOR_H