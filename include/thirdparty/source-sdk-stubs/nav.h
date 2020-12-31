#ifndef _NABENABE_NAV_H_
#define _NABENABE_NAV_H_

#include "thirdparty/source-sdk-stubs/mathlib/vector.h"
#include "thirdparty/source-sdk-stubs/nav_spot.h"

#include <list>
#include <utility>

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

// Specifying int type for consistency across formats
typedef unsigned int nabeint;

constexpr auto AREA_ID_NONE = -1337;
static_assert (AREA_ID_NONE < 0);

enum NavDirType :nabeint {
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	NUM_DIRECTIONS
};

struct Ray
{
	Vector from, to;
};

class HidingSpot;
struct SpotOrder
{
	~SpotOrder();

	float t;						///< parametric distance along ray where this spot first has LOS to our path
	union
	{
		HidingSpot* spot = nullptr;	///< the spot to look at
		unsigned int id;			///< spot ID for save/load
	};

	int m_pending_id = AREA_ID_NONE;
};
typedef std::list<SpotOrder> SpotOrderList;

class CNavArea;
union NavConnect
{
	unsigned int id;
	CNavArea* area = nullptr;

	bool operator==(const NavConnect& other) const { return (area == other.area) ? true : false; }
	operator unsigned int() const;
	operator CNavArea* () const { return area; }
};
typedef std::list<NavConnect> NavConnectList;

struct SpotEncounter
{
	NavConnect from;
	NavDirType fromDir;
	NavConnect to;
	NavDirType toDir;
	Ray path;									///< the path segment
	SpotOrderList spotList;						///< list of spots to look at, in order of occurrence

	int m_pending_from_connect = AREA_ID_NONE;
	int m_pending_to_connect = AREA_ID_NONE;
};
typedef std::list<SpotEncounter*> SpotEncounterList;

enum NavTraverseType :nabeint {
	// NOTE: First 4 directions MUST match NavDirType
	// (because the pathfinding does some funky type casting reliant on the ordering)
	GO_NORTH = 0,
	GO_EAST,
	GO_SOUTH,
	GO_WEST,
	GO_LADDER_UP,
	GO_LADDER_DOWN,
	GO_JUMP,

	NUM_TRAVERSE_TYPES
};

struct ApproachInfo
{
	NavConnect here;										///< the approach area
	NavConnect prev;										///< the area just before the approach area on the path
	NavTraverseType prevToHereHow;
	NavConnect next;										///< the area just after the approach area on the path
	NavTraverseType hereToNextHow;
};

#define NAV_MESH_CROUCH			(1 << 0)
#define NAV_MESH_JUMP			(1 << 1)
#define NAV_MESH_PRECISE		(1 << 2)
#define NAV_MESH_NO_JUMP		(1 << 3)
#define NAV_MESH_STOP			(1 << 4)
#define NAV_MESH_RUN			(1 << 5)
#define NAV_MESH_WALK			(1 << 6)
#define NAV_MESH_AVOID			(1 << 7)
#define NAV_MESH_TRANSIENT		(1 << 8)
#define NAV_MESH_DONT_HIDE		(1 << 9)
#define NAV_MESH_STAND			(1 << 10)
#define NAV_MESH_NO_HOSTAGES	(1 << 11)

#define UNDEFINED_PLACE 0 // ie: "no place"
#define ANY_PLACE 0xFFFF

#define MAX_APPROACH_AREAS 16

constexpr nabeint TEAM_JINRAI = 2;
constexpr nabeint TEAM_NSF = 3;

constexpr float HalfHumanWidth = 16.0f;
constexpr float HalfHumanHeight = 36.0f;
constexpr float HumanHeight = 72.0f;

enum NavCornerType
{
	NORTH_WEST = 0,
	NORTH_EAST = 1,
	SOUTH_EAST = 2,
	SOUTH_WEST = 3,

	NUM_CORNERS
};

// Team indices for arrays where only 2 teams are required
// (Jinrai and NSF), so TEAM_SPECTATOR etc. don't need to be allocated.
// Use the teamidx_to_navteamidx(...) and navteamidx_to_teamidx(...)
// helpers to convert between these enums and actual engine team indices.
enum class NavTeamIdx {
	NAV_TEAM_IDX_JINRAI = 0,
	NAV_TEAM_IDX_NSF,

	MAX_NAV_TEAMS
};

constexpr NavTeamIdx teamidx_to_navteamidx(const int team_idx)
{
	return team_idx == TEAM_JINRAI ? NavTeamIdx::NAV_TEAM_IDX_JINRAI : NavTeamIdx::NAV_TEAM_IDX_NSF;
}

constexpr int navteamidx_to_teamidx(const NavTeamIdx team_idx)
{
	return team_idx == NavTeamIdx::NAV_TEAM_IDX_JINRAI ? TEAM_JINRAI : TEAM_NSF;
}

struct Extent
{
	Vector lo, hi;

	void Init(void)
	{
		lo.Init();
		hi.Init();
	}

	float SizeX(void) const { return hi.x - lo.x; }
	float SizeY(void) const { return hi.y - lo.y; }
	float SizeZ(void) const { return hi.z - lo.z; }
	float Area(void) const { return SizeX() * SizeY(); }

	// Increase bounds to contain the given point
	void Encompass(const Vector& pos)
	{
		for (nabeint i = 0; i < 3; ++i)
		{
			if (pos[i] < lo[i])
			{
				lo[i] = pos[i];
			}
			else if (pos[i] > hi[i])
			{
				hi[i] = pos[i];
			}
		}
	}

	/// return true if 'pos' is inside of this extent
	bool Contains(const Vector& pos) const
	{
		return (pos.x >= lo.x && pos.x <= hi.x &&
			pos.y >= lo.y && pos.y <= hi.y &&
			pos.z >= lo.z && pos.z <= hi.z);
	}
};

class HidingSpot
{
	friend class NABE_NavCoordinator;
public:
	HidingSpot(void);

	enum
	{
		IN_COVER = 0x01,							///< in a corner with good hard cover nearby
		GOOD_SNIPER_SPOT = 0x02,							///< had at least one decent sniping corridor
		IDEAL_SNIPER_SPOT = 0x04,							///< can see either very far, or a large area, or both
		EXPOSED = 0x08							///< spot in the open, usually on a ledge or cliff
	};

	bool HasGoodCover(void) const { return (m_flags & IN_COVER) ? true : false; }	///< return true if hiding spot in in cover
	bool IsGoodSniperSpot(void) const { return (m_flags & GOOD_SNIPER_SPOT) ? true : false; }
	bool IsIdealSniperSpot(void) const { return (m_flags & IDEAL_SNIPER_SPOT) ? true : false; }
	bool IsExposed(void) const { return (m_flags & EXPOSED) ? true : false; }

	int GetFlags(void) const { return m_flags; }

	const Vector& GetPosition(void) const { return m_pos; }	///< get the position of the hiding spot
	unsigned int GetID(void) const { return m_id; }
	const CNavArea* GetArea(void) const { return m_area; }	///< return nav area this hiding spot is within

	void Mark(void) { m_marker = m_masterMarker; }
	bool IsMarked(void) const { return (m_marker == m_masterMarker) ? true : false; }
	static void ChangeMasterMarker(void) { ++m_masterMarker; }

private:
	friend class CNavMesh;

	Vector m_pos;											///< world coordinates of the spot
	unsigned int m_id;										///< this spot's unique ID
	unsigned int m_marker;									///< this spot's unique marker
	CNavArea* m_area;										///< the nav area containing this hiding spot

	unsigned char m_flags;									///< bit flags

	static unsigned int m_nextID;							///< used when allocating spot ID's
	static unsigned int m_masterMarker;						///< used to mark spots
};
typedef std::list<HidingSpot*> HidingSpotList;
extern HidingSpotList TheHidingSpotList;

HidingSpot* GetHidingSpotByID(unsigned int id);

enum NavErrorType
{
	NAV_OK,
	NAV_CANT_ACCESS_FILE,
	NAV_INVALID_FILE,
	NAV_BAD_FILE_VERSION,
	NAV_FILE_OUT_OF_DATE,
	NAV_CORRUPT_DATA,
};

typedef std::list <CNavArea*> NavAreaList;
extern NavAreaList TheNavAreaList;

#endif // _NABENABE_NAV_H_
