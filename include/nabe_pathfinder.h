#ifndef _NABENABE_PATHFINDER_H
#define _NABENABE_PATHFINDER_H

#include "thirdparty/source-sdk-stubs/nav_area.h"

#include "nabe_filesystem.h"

#include <vector>
#include <string>
#include <list>

// The code in this file is based on the Source 1 SDK, and is used under the SOURCE 1 SDK LICENSE.
// https://github.com/ValveSoftware/source-sdk-2013

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

class NABE_NavCoordinator;
struct NABE_GameMap;

class NABE_PathFinder
{
public:
	NABE_PathFinder(const fs::path& map_folder_path, const fs::path& nav_folder_path, bool verbosity)
		: m_verbosity(verbosity),
		m_map_folder(map_folder_path),
		m_nav_folder(nav_folder_path)
	{
	}

	NABE_PathFinder(const std::string& map_folder_path, const std::string& nav_folder_path, bool verbosity)
		: m_verbosity(verbosity),
		m_map_folder(fs::path(map_folder_path)),
		m_nav_folder(fs::path(nav_folder_path))
	{
	}

	NABE_PathFinder(const char* map_folder_path, const char* nav_folder_path, bool verbosity)
		: m_verbosity(verbosity),
		m_map_folder(fs::path(map_folder_path)),
		m_nav_folder(fs::path(nav_folder_path))
	{
	}

	bool AddMap(const NABE_GameMap* map);
	bool Solve(const std::string& map_name, int area_id_from, int area_id_to, std::list<CNavArea*>& out_path);
	bool Solve(const std::string& map_name, const Vector& pos_from, const Vector& pos_to, std::list<CNavArea*>& out_path);

	const fs::path& GetMapFolderPath() const { return m_map_folder; }
	const fs::path& GetNavFolderPath() const { return m_nav_folder; }

private:
	NABE_NavCoordinator* GetMapNavCoordinator(const std::string& map_name, const bool build_if_not_exists);
	NABE_NavCoordinator* BuildMapNavCoordinator(const std::string& map_name);

private:
	fs::path m_map_folder;
	fs::path m_nav_folder;
	std::vector<NABE_NavCoordinator*> m_coordinators;
	bool m_verbosity;
};

#endif // _NABENABE_PATHFINDER_H
