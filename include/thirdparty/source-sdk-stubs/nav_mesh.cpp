#include "nav_mesh.h"

#include "thirdparty/source-sdk-stubs/nav.h"
#include "thirdparty/source-sdk-stubs/nav_area.h"

#include <algorithm>

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

// The singleton for accessing the navigation mesh
CNavMesh* TheNavMesh = nullptr;

HidingSpotList TheHidingSpotList;
unsigned int HidingSpot::m_nextID = 1;

CNavMesh::CNavMesh(void)
{
	m_grid = nullptr;
	m_gridCellSize = 300.0f;

	Reset();
}

CNavMesh::~CNavMesh()
{
	for (auto& area : *m_grid) {
		delete area;
	}
	m_grid->clear();
	m_grid = nullptr;
}

void CNavMesh::Reset(void)
{
	DestroyNavigationMesh();

	m_isFromCurrentMap = true;
}

void CNavMesh::DestroyNavigationMesh(bool incremental)
{
	if (!incremental)
	{
		// destroy all areas
		CNavArea::m_isReset = true;

		// remove each element of the list and delete them
		for (auto& area : TheNavAreaList) {
			delete area;
		}
		TheNavAreaList.clear();

		CNavArea::m_isReset = false;

		// destroy ladder representations
		//DestroyLadders();
	}
	else
	{
		for (auto& area : TheNavAreaList) {
			area->ResetNodes();
		}
	}

	// destroy all hiding spots
	DestroyHidingSpots();

	// destroy navigation nodes created during map generation
	CNavNode* node, * next;
	for (node = CNavNode::m_list; node; node = next)
	{
		next = node->m_next;
		delete node;
	}
	CNavNode::m_list = nullptr;
	CNavNode::m_listLength = 0;
	CNavNode::m_nextID = 1;

	if (!incremental)
	{
		// destroy the grid
		for (auto& area : *m_grid) {
			delete area;
		}
		m_grid->clear();
		m_grid = nullptr;

		m_gridSizeX = 0;
		m_gridSizeY = 0;
	}

	// clear the hash table
	for (int i = 0; i < HASH_TABLE_SIZE; ++i)
	{
		m_hashTable[i] = nullptr;
	}

	if (!incremental)
	{
		m_areaCount = 0;
	}

	if (!incremental)
	{
		// Reset the next area and ladder IDs to 1
		CNavArea::CompressIDs();
		//CNavLadder::CompressIDs();
	}

	if (!incremental)
	{
		m_isLoaded = false;
	}
}

void CNavMesh::AllocateGrid(float minX, float maxX, float minY, float maxY)
{
	if (m_grid)
	{
		// destroy the grid
		if (m_grid)
			delete[] m_grid;

		m_grid = NULL;
	}

	m_minX = minX;
	m_minY = minY;

	m_gridSizeX = (int)((maxX - minX) / m_gridCellSize) + 1;
	m_gridSizeY = (int)((maxY - minY) / m_gridCellSize) + 1;

	m_grid = new NavAreaList[m_gridSizeX * m_gridSizeY];
}

void CNavMesh::AddNavArea(CNavArea* area)
{
	if (!m_grid)
	{
		// If we somehow have no grid (manually creating a nav area without loading or generating a mesh), don't crash
		AllocateGrid(0, 0, 0, 0);
	}

	// add to grid
	const Extent& extent = area->GetExtent();

	int loX = WorldToGridX(extent.lo.x);
	int loY = WorldToGridY(extent.lo.y);
	int hiX = WorldToGridX(extent.hi.x);
	int hiY = WorldToGridY(extent.hi.y);

	for (int y = loY; y <= hiY; ++y)
	{
		for (int x = loX; x <= hiX; ++x)
		{
			m_grid[x + y * m_gridSizeX].push_back(area);
		}
	}

	// add to hash table
	int key = ComputeHashKey(area->GetID());

	if (m_hashTable[key])
	{
		// add to head of list in this slot
		area->m_prevHash = nullptr;
		area->m_nextHash = m_hashTable[key];
		m_hashTable[key]->m_prevHash = area;
		m_hashTable[key] = area;
	}
	else
	{
		// first entry in this slot
		m_hashTable[key] = area;
		area->m_nextHash = nullptr;
		area->m_prevHash = nullptr;
	}

	//if (area->GetAttributes() & NAV_MESH_TRANSIENT)
	//{
	//	m_transientAreas.AddToTail(area);
	//}

	++m_areaCount;
}

void CNavMesh::RemoveNavArea(CNavArea* area)
{
	// add to grid
	const Extent& extent = area->GetExtent();

	int loX = WorldToGridX(extent.lo.x);
	int loY = WorldToGridY(extent.lo.y);
	int hiX = WorldToGridX(extent.hi.x);
	int hiY = WorldToGridY(extent.hi.y);

	for (int y = loY; y <= hiY; ++y)
	{
		for (int x = loX; x <= hiX; ++x)
		{
			m_grid[x + y * m_gridSizeX].remove(area);
		}
	}

	// remove from hash table
	int key = ComputeHashKey(area->GetID());

	if (area->m_prevHash)
	{
		area->m_prevHash->m_nextHash = area->m_nextHash;
	}
	else
	{
		// area was at start of list
		m_hashTable[key] = area->m_nextHash;

		if (m_hashTable[key])
		{
			m_hashTable[key]->m_prevHash = NULL;
		}
	}

	if (area->m_nextHash)
	{
		area->m_nextHash->m_prevHash = area->m_prevHash;
	}

	//if (area->GetAttributes() & NAV_MESH_TRANSIENT)
	//{
	//	BuildTransientAreaList();
	//}

	--m_areaCount;
}

#if(0)
void CNavMesh::BuildTransientAreaList(void)
{
	m_transientAreas.RemoveAll();

	FOR_EACH_LL(TheNavAreaList, it)
	{
		CNavArea* area = TheNavAreaList[it];

		if (area->GetAttributes() & NAV_MESH_TRANSIENT)
		{
			m_transientAreas.AddToTail(area);
		}
	}
}
#endif

CNavArea* CNavMesh::GetNavArea(const Vector& pos, float beneathLimit) const
{
	if (m_grid == nullptr) {
		return nullptr;
	}

	// get list in cell that contains position
	int x = WorldToGridX(pos.x);
	int y = WorldToGridY(pos.y);
	NavAreaList* list = &m_grid[x + y * m_gridSizeX];


	// search cell list to find correct area
	CNavArea* use = NULL;
	float useZ = -99999999.9f;
	Vector testPos = pos + Vector(0, 0, 5);

	for (auto& area : *list) {
		// check if position is within 2D boundaries of this area
		if (area->IsOverlapping(testPos))
		{
			// project position onto area to get Z
			float z = area->GetZ(testPos);

			// if area is above us, skip it
			if (z > testPos.z)
				continue;

			// if area is too far below us, skip it
			if (z < pos.z - beneathLimit)
				continue;

			// if area is higher than the one we have, use this instead
			if (z > useZ)
			{
				use = area;
				useZ = z;
			}
		}
	}

	return use;
}

void CNavMesh::GridToWorld(int gridX, int gridY, Vector* pos) const
{
	gridX = std::clamp(gridX, 0, m_gridSizeX - 1);
	gridY = std::clamp(gridY, 0, m_gridSizeY - 1);

	pos->x = m_minX + gridX * m_gridCellSize;
	pos->y = m_minY + gridY * m_gridCellSize;
}

CNavArea* CNavMesh::GetNearestNavArea(const Vector& pos, bool anyZ, float maxDist, bool checkLOS) const
{
	if (m_grid == nullptr) {
		return nullptr;
	}

	CNavArea* close = nullptr;
	float closeDistSq = maxDist * maxDist;

	// quick check
	close = GetNavArea(pos);
	if (close)
	{
		return close;
	}

	// ensure source position is well behaved
	Vector source;
	source.x = pos.x;
	source.y = pos.y;

	source.z += HalfHumanHeight;

	// find closest nav area
	CNavArea::MakeNewMarker();

	// get list in cell that contains position
	int originX = WorldToGridX(pos.x);
	int originY = WorldToGridY(pos.y);

	int shiftLimit = static_cast<int>(maxDist / m_gridCellSize);

	//
	// Search in increasing rings out from origin, starting with cell
	// that contains the given position.
	// Once we find a close area, we must check one more step out in
	// case our position is just against the edge of the cell boundary
	// and an area in an adjacent cell is actually closer.
	// 
	for (int shift = 0; shift <= shiftLimit; ++shift)
	{
		for (int x = originX - shift; x <= originX + shift; ++x)
		{
			if (x < 0 || x >= m_gridSizeX)
				continue;

			for (int y = originY - shift; y <= originY + shift; ++y)
			{
				if (y < 0 || y >= m_gridSizeY)
					continue;

				// only check these areas if we're on the outer edge of our spiral
				if (x > originX - shift &&
					x < originX + shift &&
					y > originY - shift &&
					y < originY + shift)
					continue;

				NavAreaList* list = &m_grid[x + y * m_gridSizeX];

				// find closest area in this cell
				for (auto& area : *list) {
					// skip if we've already visited this area
					if (area->IsMarked())
						continue;

					area->Mark();

					Vector areaPos;
					area->GetClosestPointOnArea(source, &areaPos);

					float distSq = (areaPos - source).LengthSqr();

					// keep the closest area
					if (distSq < closeDistSq)
					{
#if(0)
						// check LOS to area
						// REMOVED: If we do this for !anyZ, it's likely we wont have LOS and will enumerate every area in the mesh
						// It is still good to do this in some isolated cases, however
						if (checkLOS)
						{
							trace_t result;
							UTIL_TraceLine(source, areaPos + Vector(0, 0, HalfHumanHeight), MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result);
							if (result.fraction != 1.0f)
							{
								continue;
							}
						}
#endif

						closeDistSq = distSq;
						close = area;

						// look one more step outwards
						shiftLimit = shift + 1;
					}
				}
			}
		}
	}

	return close;
}

CNavArea* CNavMesh::GetNavAreaByID(unsigned int id) const
{
	if (id == 0)
		return nullptr;

	int key = ComputeHashKey(id);

	for (CNavArea* area = m_hashTable[key]; area; area = area->m_nextHash)
	{
		if (area->GetID() == id)
		{
			return area;
		}
	}

	return nullptr;
}

#if(0)
CNavLadder* CNavMesh::GetLadderByID(unsigned int id) const
{
	if (id == 0)
		return NULL;

	FOR_EACH_LL(m_ladderList, it)
	{
		CNavLadder* ladder = m_ladderList[it];
		if (ladder->GetID() == id)
		{
			return ladder;
		}
	}

	return NULL;
}
#endif

HidingSpot* CNavMesh::CreateHidingSpot(void) const
{
	return new HidingSpot;
}

void CNavMesh::DestroyHidingSpots(void)
{
	// remove all hiding spot references from the nav areas
	for (auto& area : TheNavAreaList) {
		area->m_hidingSpotList.clear();
	}

	HidingSpot::m_nextID = 0;

	// free all the HidingSpots
	for (auto& hiding_spot : TheHidingSpotList) {
		delete hiding_spot;
	}
	TheHidingSpotList.clear();
}

template < typename Functor >
bool CNavMesh::ForAllAreas(Functor& func)
{
	for (auto& area : TheNavAreaList) {
		if (func(area) == false) {
			return false;
		}
	}
	return true;
}

