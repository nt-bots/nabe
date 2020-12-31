#ifndef _NABENABE_NAV_MESH_H_
#define _NABENABE_NAV_MESH_H_

#include "thirdparty/source-sdk-stubs/nav.h"
#include "thirdparty/source-sdk-stubs/nav_area.h"
#include "thirdparty/source-sdk-stubs/mathlib/vector.h"

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

class CNavArea;

/**
 * The CNavMesh is the global interface to the Navigation Mesh.
 * @todo Make this an abstract base class interface, and derive mod-specific implementations.
 */
class CNavMesh
{
public:
	CNavMesh(void);
	virtual ~CNavMesh();

	virtual HidingSpot* CreateHidingSpot(void) const;					///< Hiding Spot factory

	virtual void Reset(void);											///< destroy Navigation Mesh data and revert to initial state
	bool IsLoaded(void) const { return m_isLoaded; }				///< return true if a Navigation Mesh has been loaded

	bool IsFromCurrentMap(void) const { return m_isFromCurrentMap; }	///< return true if the Navigation Mesh was last edited with the current map version

	unsigned int GetNavAreaCount(void) const { return m_areaCount; }	///< return total number of nav areas

	CNavArea* GetNavArea(const Vector& pos, float beneathLimt = 120.0f) const;	///< given a position, return the nav area that IsOverlapping and is *immediately* beneath it
	CNavArea* GetNavAreaByID(unsigned int id) const;
	CNavArea* GetNearestNavArea(const Vector& pos, bool anyZ = false, float maxDist = 10000.0f, bool checkLOS = false) const;

	const char* GetFilename(void) const;								///< return the filename for this map's "nav" file

	/**
	 * Apply the functor to all navigation areas.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllAreas(Functor& func);

private:
	friend class CNavArea;
	friend class CNavNode;

	NavAreaList* m_grid;
	float m_gridCellSize;										///< the width/height of a grid cell for spatially partitioning nav areas for fast access
	int m_gridSizeX;
	int m_gridSizeY;
	float m_minX;
	float m_minY;
	unsigned int m_areaCount;									///< total number of nav areas

	bool m_isLoaded;											///< true if a Navigation Mesh has been loaded
	bool m_isFromCurrentMap;									///< true if the Navigation Mesh was last saved with the current map

	enum { HASH_TABLE_SIZE = 256 };
	CNavArea* m_hashTable[HASH_TABLE_SIZE];					///< hash table to optimize lookup by ID
	int ComputeHashKey(unsigned int id) const;				///< returns a hash key for the given nav area ID

	int WorldToGridX(float wx) const;							///< given X component, return grid index
	int WorldToGridY(float wy) const;							///< given Y component, return grid index
	void AllocateGrid(float minX, float maxX, float minY, float maxY);	///< clear and reset the grid to the given extents
	void GridToWorld(int gridX, int gridY, Vector* pos) const;

	void AddNavArea(CNavArea* area);							///< add an area to the grid
	void RemoveNavArea(CNavArea* area);						///< remove an area from the grid

	void DestroyNavigationMesh(bool incremental = false);		///< free all resources of the mesh and reset it to empty state
	void DestroyHidingSpots(void);
};

// the global singleton interface
extern CNavMesh* TheNavMesh;

inline int CNavMesh::ComputeHashKey(unsigned int id) const
{
	return id & 0xFF;
}

inline int CNavMesh::WorldToGridX(float wx) const
{
	int x = (int)((wx - m_minX) / m_gridCellSize);

	if (x < 0)
		x = 0;
	else if (x >= m_gridSizeX)
		x = m_gridSizeX - 1;

	return x;
}

inline int CNavMesh::WorldToGridY(float wy) const
{
	int y = (int)((wy - m_minY) / m_gridCellSize);

	if (y < 0)
		y = 0;
	else if (y >= m_gridSizeY)
		y = m_gridSizeY - 1;

	return y;
}

#endif // _NABENABE_NAV_MESH_H_
