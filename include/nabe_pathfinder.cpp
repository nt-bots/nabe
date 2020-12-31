#include "nabe_pathfinder.h"

#include "thirdparty/source-sdk-stubs/nav_pathfind.h"

#include "nabe_nav_coordinator.h"
#include "nabe_gamemap.h"

// The code in this file is based on the Source 1 SDK, and is used under the SOURCE 1 SDK LICENSE.
// https://github.com/ValveSoftware/source-sdk-2013

/*
				SOURCE 1 SDK LICENSE

Source SDK Copyright(c) Valve Corp.

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE
CORPORATION ("Valve").  PLEASE READ IT BEFORE DOWNLOADING OR USING
THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE
SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO
THE TERMS OF THIS LICENSE PLEASE DON’T DOWNLOAD OR USE THE SDK.

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

NABE_NavCoordinator* NABE_PathFinder::BuildMapNavCoordinator(const std::string& map_name)
{
	NABE_NavCoordinator* coordinator = GetMapNavCoordinator(map_name, false);

	if (!coordinator) {
		if (m_verbosity) {
			print(Info, "%s::%s Coordinator not cached, preparing to build from nav data...",
				__FUNCTION__, map_name.c_str());
		}
		
		coordinator = new NABE_NavCoordinator(this, map_name, GetMapFolderPath().string().c_str(), GetNavFolderPath().string().c_str());
		if (coordinator->m_loaded == false) {
			delete coordinator;
			return nullptr;
		}

		if (m_verbosity) {
			print(Info, "%s::%s Coordinator ready.", __FUNCTION__, map_name.c_str());
		}

		m_coordinators.push_back(coordinator);
	}

	return coordinator;
}

NABE_NavCoordinator* NABE_PathFinder::GetMapNavCoordinator(const std::string& map_name, const bool build_if_not_exists)
{
	NABE_NavCoordinator* coordinator = nullptr;

	for (auto& p : m_coordinators) {
		if (map_name.compare(p->m_map->map_name) == 0) {
			coordinator = p;
			break;
		}
	}

	if (!coordinator && build_if_not_exists) {
		coordinator = BuildMapNavCoordinator(map_name);
	}

	return coordinator;
}

bool NABE_PathFinder::AddMap(const NABE_GameMap* map)
{
	if (!map) {
		print(Error, "%s: Invalid map", __FUNCTION__);
		return false;
	}
	else if (map->map_size == 0) {
		print(Error, "%s: Couldn't find map: \"%s\"", __FUNCTION__, map->map_name.c_str());
		return false;
	}

	auto coordinator = GetMapNavCoordinator(map->map_name.c_str(), true);
	if (!coordinator) {
		return false;
	}
	return true;
}

bool NABE_PathFinder::Solve(const std::string& map_name, int area_id_from, int area_id_to, std::list<CNavArea*>& out_path)
{
	auto coordinator = GetMapNavCoordinator(map_name, false);
	if (!coordinator) {
		return false;
	}

	auto area_from = coordinator->GetAreaById(area_id_from);
	auto area_to = coordinator->GetAreaById(area_id_to);

	if (!area_from) {
		print(Error, "%s: Failed to find area_id_from %d for \"%s\"",
			__FUNCTION__, area_id_from, map_name.c_str());
		return false;
	}
	else if (!area_to) {
		print(Error, "%s: Failed to find area_id_to %d for \"%s\"",
			__FUNCTION__, area_id_to, map_name.c_str());
		return false;
	}

	CNavArea *from = area_from, *to = area_to, *next = nullptr;

	if (m_verbosity) {
		print(Info, "Solving path: area %d --> area %d", from->GetID(), to->GetID());
	}

	const bool success = NavAreaBuildPath(from, to, nullptr, out_path);

	if (m_verbosity) {
		if (!success) {
			print(Warning, "%s: Failed to solve path.", __FUNCTION__);
		}
		else {
			print(Info, "%s: Found solution (this route visits %d areas total):", __FUNCTION__, out_path.size());

			size_t num = 0;
			constexpr auto print_full_path = false;
			if (print_full_path) {
				for (auto& p : out_path) {
					print(Info, "\t * Path node %d: go through area %d\t(world XYZ coordinates: %.2f %.2f %.2f)",
						num++,
						p->GetID(),
						p->GetCenter().x, p->GetCenter().y, p->GetCenter().z);
				}
			}
		}
	}

	return success;
}

bool NABE_PathFinder::Solve(const std::string& map_name, const Vector& pos_from, const Vector& pos_to, std::list<CNavArea*>& out_path)
{
	auto coordinator = GetMapNavCoordinator(map_name, false);
	if (!coordinator) {
		return false;
	}

	auto area_from = coordinator->GetAreaByPos(pos_from);
	auto area_to = coordinator->GetAreaByPos(pos_to);

	if (!area_from) {
		print(Error, "%s: Failed to find pos_from (%f %f %f) for \"%s\"",
			__FUNCTION__, pos_from.x, pos_from.y, pos_from.z, map_name.c_str());
		return false;
	}
	else if (!area_to) {
		print(Error, "%s: Failed to find pos_to (%f %f %f) for \"%s\"",
			__FUNCTION__, pos_to.x, pos_to.y, pos_to.z, map_name.c_str());
		return false;
	}

	CNavArea* from = area_from, * to = area_to, * next = nullptr;

	if (m_verbosity) {
		print(Info, "Solving path: area %d --> area %d", from->GetID(), to->GetID());
	}
	
	const bool success = NavAreaBuildPath(from, to, nullptr, out_path);

	if (m_verbosity) {
		if (!success) {
			print(Warning, "%s: Failed to solve path.", __FUNCTION__);
		}
		else {
			print(Info, "%s: Found solution (this route visits %d areas total):", __FUNCTION__, out_path.size());
			size_t num = 0;
			constexpr auto print_full_path = false;
			if (print_full_path) {
				for (auto& p : out_path) {
					print(Info, "\t * Path node %d: go through area %d\t(world XYZ coordinates: %.2f %.2f %.2f)",
						num++,
						p->GetID(),
						p->GetCenter().x, p->GetCenter().y, p->GetCenter().z);
				}
			}
		}
	}

	return success;
}
