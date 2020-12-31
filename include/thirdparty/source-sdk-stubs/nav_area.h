#ifndef _NABENABE_NAV_AREA_H_
#define _NABENABE_NAV_AREA_H_

#include <vector>
#include <string>

#include "thirdparty/source-sdk-stubs/mathlib/vector.h"
#include "thirdparty/source-sdk-stubs/nav.h"
#include "thirdparty/source-sdk-stubs/nav_node.h"
#include "thirdparty/source-sdk-stubs/nav_mesh.h"

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

class CNavArea
{
	friend class CNavMesh;
public:
	CNavArea(CNavNode* nwNode, CNavNode* neNode, CNavNode* seNode, CNavNode* swNode);
	CNavArea(void);
	CNavArea(const Vector& corner, const Vector& otherCorner);
	CNavArea(const Vector& nwCorner, const Vector& neCorner, const Vector& seCorner, const Vector& swCorner);

	virtual ~CNavArea();

	void ConnectTo(CNavArea* area, NavDirType dir);			///< connect this area to given area in given direction
	void Disconnect(CNavArea* area);							///< disconnect this area from given area

	unsigned int GetID(void) const { return m_id; }		///< return this area's unique ID
	static void CompressIDs(void);							///< re-orders area ID's so they are continuous

	void SetAttributes(int bits) { m_attributeFlags = static_cast<unsigned short>(bits); }
	int GetAttributes(void) const { return m_attributeFlags; }

	bool IsBlocked(void) const { return m_isBlocked; }
	bool IsUnderwater(void) const { return m_isUnderwater; }

	bool IsOverlapping(const Vector& pos, float tolerance = 0.0f) const;	///< return true if 'pos' is within 2D extents of area.
	bool IsOverlapping(const CNavArea* area) const;			///< return true if 'area' overlaps our 2D extents
	bool IsOverlappingX(const CNavArea* area) const;			///< return true if 'area' overlaps our X extent
	bool IsOverlappingY(const CNavArea* area) const;			///< return true if 'area' overlaps our Y extent
	float GetZ(const Vector& pos) const;						///< return Z of area at (x,y) of 'pos'
	float GetZ(float x, float y) const;						///< return Z of area at (x,y) of 'pos'
	bool Contains(const Vector& pos) const;					///< return true if given point is on or above this area, but no others
	bool IsCoplanar(const CNavArea* area) const;				///< return true if this area and given area are approximately co-planar
	void GetClosestPointOnArea(const Vector& pos, Vector* close) const;	///< return closest point to 'pos' on this area - returned point in 'close'
	float GetDistanceSquaredToPoint(const Vector& pos) const;	///< return shortest distance between point and this area
	bool IsDegenerate(void) const;							///< return true if this area is badly formed
	bool IsRoughlySquare(void) const;							///< return true if this area is approximately square
	bool IsFlat(void) const;									///< return true if this area is approximately flat

	bool IsEdge(NavDirType dir) const;						///< return true if there are no bi-directional links on the given side

	int GetAdjacentCount(NavDirType dir) const { return static_cast<int>(m_connect[dir].size()); }	///< return number of connected areas in given direction
	CNavArea* GetAdjacentArea(NavDirType dir, int i) const;	/// return the i'th adjacent area in the given direction
	CNavArea* GetRandomAdjacentArea(NavDirType dir) const;

	const NavConnectList* GetAdjacentList(NavDirType dir) const { return &m_connect[dir]; }
	bool IsConnected(const CNavArea* area, NavDirType dir) const;	///< return true if given area is connected in given direction
	//bool IsConnected(const CNavLadder* ladder, CNavLadder::LadderDirectionType dir) const;	///< return true if given ladder is connected in given direction
	float ComputeHeightChange(const CNavArea* area);			///< compute change in height from this area to given area

	NavDirType ComputeDirection(Vector* point) const;			///< return direction from this area to the given point

	//- hiding spots ------------------------------------------------------------------------------------
	const HidingSpotList* GetHidingSpotList(void) const { return &m_hidingSpotList; }

	SpotEncounter* GetSpotEncounter(const CNavArea* from, const CNavArea* to);	///< given the areas we are moving between, return the spots we will encounter
	int GetSpotEncounterCount(void) const { return static_cast<int>(m_spotEncounterList.size()); }

	//- "danger" ----------------------------------------------------------------------------------------
	void IncreaseDanger(int teamID, float amount);			///< increase the danger of this area for the given team
	float GetDanger(int teamID);								///< return the danger of this area (decays over time)

	//- extents -----------------------------------------------------------------------------------------
	float GetSizeX(void) const { return m_extent.hi.x - m_extent.lo.x; }
	float GetSizeY(void) const { return m_extent.hi.y - m_extent.lo.y; }
	const Extent& GetExtent(void) const { return m_extent; }
	const Vector& GetCenter(void) const { return m_center; }
	const Vector& GetCorner(NavCornerType corner) const;
	void SetCorner(NavCornerType corner, const Vector& newPosition);
	void ComputeNormal(Vector* normal, bool alternate = false) const;	///< Computes the area's normal based on m_extent.lo.  If 'alternate' is specified, m_extent.hi is used instead.

	void ResetNodes(void);									///< nodes are going away as part of an incremental nav generation
	bool HasNodes(void) const;
	void AssignNodes(CNavArea* area);							///< assign internal nodes to the given area

	static void MakeNewMarker(void) { ++m_masterMarker; if (m_masterMarker == 0) m_masterMarker = 1; }
	void Mark(void) { m_marker = m_masterMarker; }
	bool IsMarked(void) const { return (m_marker == m_masterMarker) ? true : false; }

	void SetParent(CNavArea* parent, NavTraverseType how = NUM_TRAVERSE_TYPES) { m_parent = parent; m_parentHow = how; }
	CNavArea* GetParent(void) const { return m_parent; }
	NavTraverseType GetParentHow(void) const { return m_parentHow; }

	bool IsOpen(void) const { return (m_openMarker == m_masterMarker) ? true : false; }									///< true if on "open list"
	static bool IsOpenListEmpty(void) { return (m_openList) ? false : true; }
	void AddToOpenList(void);									///< add to open list in decreasing value order
	void UpdateOnOpenList(void);								///< a smaller value has been found, update this area on the open list
	
	void RemoveFromOpenList(void)
	{
		if (m_prevOpen)
			m_prevOpen->m_nextOpen = m_nextOpen;
		else
			m_openList = m_nextOpen;
		if (m_nextOpen)
			m_nextOpen->m_prevOpen = m_prevOpen;
		m_openMarker = 0; // zero is an invalid marker
	}
	
	static CNavArea* PopOpenList(void)						///< remove and return the first element of the open list
	{
		if (!m_openList)
			return nullptr;
		auto area = m_openList;
		area->RemoveFromOpenList(); // disconnect from list
		return area;
	}

	bool IsClosed(void) const { return (IsMarked() && !IsOpen()); }					///< true if on "closed list"
	void AddToClosedList(void) { Mark(); }						///< add to the closed list
	void RemoveFromClosedList(void) { } // // since "closed" is defined as visited (marked) and not on open list, do nothing

	static void ClearSearchLists(void)						///< clears the open and closed lists for a new search
	{
		// effectively clears all open list pointers and closed flags
		CNavArea::MakeNewMarker();
		m_openList = nullptr;
	}

	void SetTotalCost(float value) { m_totalCost = value; }
	float GetTotalCost(void) const { return m_totalCost; }

	void SetCostSoFar(float value) { m_costSoFar = value; }
	float GetCostSoFar(void) const { return m_costSoFar; }

private:
	//friend class CNavMesh;
	//friend class CNavLadder;
	void Initialize(); // to keep constructors consistent

protected:
	static bool m_isReset; // if true, don't bother cleaning up in destructor since everything is going away

	/*
	extent.lo
		nw           ne
		 +-----------+
		 | +-->x     |
		 | |         |
		 | v         |
		 | y         |
		 |           |
		 +-----------+
		sw           se
					extent.hi
	*/

	static nabeint m_nextID; // used to allocate unique IDs
	nabeint m_id; // unique area ID
	Extent m_extent; //  extents of area in world coords (NOTE: lo.z is not necessarily the minimum Z, but corresponds to Z at point (lo.x, lo.y), etc
	Vector m_center;  // centroid of area
	unsigned short m_attributeFlags;  // set of attribute bit flags (see NavAttributeType)
	bool m_isBlocked; // if true, some part of the world is preventing movement through this nav area
	bool m_isUnderwater; // true if the center of the area is underwater

	/// height of the implicit corners
	float m_neZ;
	float m_swZ;

	// hiding spots
	HidingSpotList m_hidingSpotList;

	// encounter spots
	SpotEncounterList m_spotEncounterList;
	bool IsHidingSpotCollision(const Vector& pos) const; // returns true if an existing hiding spot is too close to given position

	// approach areas
	struct ApproachInfo
	{
		NavConnect here;										// the approach area
		NavConnect prev;										// the area just before the approach area on the path
		NavTraverseType prevToHereHow;
		NavConnect next;										// the area just after the approach area on the path
		NavTraverseType hereToNextHow;
	};
	const ApproachInfo* GetApproachInfo(int i) const { return &m_approach[i]; }
	int GetApproachInfoCount(void) const { return m_approachCount; }
	ApproachInfo m_approach[MAX_APPROACH_AREAS];
	unsigned char m_approachCount;

	bool m_isBattlefront;

	// pathfinding algorithm
	static unsigned int m_masterMarker; 
	unsigned int m_marker; // used to flag the area as visited
	CNavArea* m_parent; // the area just prior to this on in the search path
	NavTraverseType m_parentHow; // how we get from parent to us
	float m_totalCost; // the distance so far plus an estimate of the distance left
	float m_costSoFar; // distance travelled so far

	static CNavArea* m_openList;
	CNavArea* m_nextOpen, * m_prevOpen; // only valid if m_openMarker == m_masterMarker
	unsigned int m_openMarker; // if this equals the current marker value, we are on the open list

	// connections to adjacent areas
	NavConnectList m_connect[NUM_DIRECTIONS]; // a list of adjacent areas for each direction

	std::list<CNavArea*> m_overlapList;				///< list of areas that overlap this area

	CNavNode* m_node[NUM_CORNERS]; // nav nodes at each corner of the area

	void OnDestroyNotify(CNavArea* dead);						///< invoked when given area is going away

	CNavArea* m_prevHash, * m_nextHash;							///< for hash table in CNavMesh
};

struct CNavLadder
{
	float m_length;
};

#endif // _NABENABE_NAV_AREA_H_
