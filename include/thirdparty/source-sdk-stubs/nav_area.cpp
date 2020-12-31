#include "thirdparty/source-sdk-stubs/nav_area.h"

#include "thirdparty/source-sdk-stubs/nav_node.h"

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

unsigned int CNavArea::m_nextID = 1;
NavAreaList TheNavAreaList;

unsigned int CNavArea::m_masterMarker = 1;
CNavArea* CNavArea::m_openList = nullptr;

bool CNavArea::m_isReset = false;

HidingSpot::HidingSpot()
{
	m_pos = Vector(0, 0, 0);
	m_id = m_nextID++;
	m_flags = 0;
	m_area = nullptr;

	TheHidingSpotList.push_back(this);
}

CNavArea::CNavArea(CNavNode* nwNode, CNavNode* neNode, CNavNode* seNode, CNavNode* swNode)
{
	Initialize();

	m_extent.lo = *nwNode->GetPosition();
	m_extent.hi = *seNode->GetPosition();

	m_center.x = (m_extent.lo.x + m_extent.hi.x) / 2.0f;
	m_center.y = (m_extent.lo.y + m_extent.hi.y) / 2.0f;
	m_center.z = (m_extent.lo.z + m_extent.hi.z) / 2.0f;

	m_neZ = neNode->GetPosition()->z;
	m_swZ = swNode->GetPosition()->z;

	m_node[NORTH_WEST] = nwNode;
	m_node[NORTH_EAST] = neNode;
	m_node[SOUTH_EAST] = seNode;
	m_node[SOUTH_WEST] = swNode;

	// mark internal nodes as part of this area
	AssignNodes(this);
}

CNavArea::CNavArea(void)
{
	Initialize();
}

CNavArea::CNavArea(const Vector& corner, const Vector& otherCorner)
{
	Initialize();

	if (corner.x < otherCorner.x)
	{
		m_extent.lo.x = corner.x;
		m_extent.hi.x = otherCorner.x;
	}
	else
	{
		m_extent.hi.x = corner.x;
		m_extent.lo.x = otherCorner.x;
	}

	if (corner.y < otherCorner.y)
	{
		m_extent.lo.y = corner.y;
		m_extent.hi.y = otherCorner.y;
	}
	else
	{
		m_extent.hi.y = corner.y;
		m_extent.lo.y = otherCorner.y;
	}

	m_extent.lo.z = corner.z;
	m_extent.hi.z = corner.z;

	m_center.x = (m_extent.lo.x + m_extent.hi.x) / 2.0f;
	m_center.y = (m_extent.lo.y + m_extent.hi.y) / 2.0f;
	m_center.z = (m_extent.lo.z + m_extent.hi.z) / 2.0f;

	m_neZ = corner.z;
	m_swZ = otherCorner.z;
}

CNavArea::CNavArea(const Vector& nwCorner, const Vector& neCorner, const Vector& seCorner, const Vector& swCorner)
{
	Initialize();

	m_extent.lo = nwCorner;
	m_extent.hi = seCorner;

	m_center.x = (m_extent.lo.x + m_extent.hi.x) / 2.0f;
	m_center.y = (m_extent.lo.y + m_extent.hi.y) / 2.0f;
	m_center.z = (m_extent.lo.z + m_extent.hi.z) / 2.0f;

	m_neZ = neCorner.z;
	m_swZ = swCorner.z;
}

void CNavArea::Initialize(void)
{
	m_marker = 0;
	m_parent = NULL;
	m_parentHow = GO_NORTH;
	m_attributeFlags = 0;
	m_isBlocked = false;
	m_isUnderwater = false;

	ResetNodes();

	m_approachCount = 0;

	// set an ID for splitting and other interactive editing - loads will overwrite this
	m_id = m_nextID++;

	m_prevHash = nullptr;
	m_nextHash = nullptr;

	m_isBattlefront = false;

	for (int i = 0; i < NUM_DIRECTIONS; ++i)
	{
		m_connect[i].clear();
	}
}

CNavArea::~CNavArea()
{
	for (auto& encounter_spot : m_spotEncounterList) {
		delete encounter_spot;
	}
	m_spotEncounterList.clear();

	// if we are resetting the system, don't bother cleaning up - all areas are being destroyed
	if (m_isReset)
		return;

	// tell the other areas we are going away
	for (auto& area : TheNavAreaList) {
		if (area == this) {
			continue;
		}
		area->OnDestroyNotify(this);
	}

	// unhook from ladders
	//{
	//	FOR_EACH_LL(TheNavMesh->GetLadders(), it)
	//	{
	//		CNavLadder* ladder = TheNavMesh->GetLadders()[it];
	//
	//		ladder->OnDestroyNotify(this);
	//	}
	//}

	// remove the area from the grid
	TheNavMesh->RemoveNavArea(this);
}

/**
 * Assign internal nodes to the given area
 * NOTE: "internal" nodes do not include the east or south border nodes
 */
void CNavArea::AssignNodes(CNavArea* area)
{
	CNavNode* horizLast = m_node[NORTH_EAST];

	for (CNavNode* vertNode = m_node[NORTH_WEST]; vertNode != m_node[SOUTH_WEST]; vertNode = vertNode->GetConnectedNode(SOUTH))
	{
		for (CNavNode* horizNode = vertNode; horizNode != horizLast; horizNode = horizNode->GetConnectedNode(EAST))
		{
			horizNode->AssignArea(area);
		}

		horizLast = horizLast->GetConnectedNode(SOUTH);
	}
}

/**
 * This is invoked at the start of an incremental nav generation on pre-existing areas.
 */
void CNavArea::ResetNodes(void)
{
	for (int i = 0; i < NUM_CORNERS; ++i)
	{
		m_node[i] = NULL;
	}
}

void CNavArea::OnDestroyNotify(CNavArea* dead)
{
	NavConnect con;
	con.area = dead;
	for (int d = 0; d < NUM_DIRECTIONS; ++d) {
		m_connect[d].remove(con);
	}

	//m_overlapList.FindAndRemove(dead);
}

void CNavArea::CompressIDs(void)
{
	m_nextID = 1;

	for (auto& area : TheNavAreaList) {
		area->m_id = m_nextID++;

		// remove and re-add the area from the nav mesh to update the hashed ID
		TheNavMesh->RemoveNavArea(area);
		TheNavMesh->AddNavArea(area);
	}
}

bool CNavArea::IsOverlapping(const Vector& pos, float tolerance) const
{
	if (pos.x + tolerance >= m_extent.lo.x && pos.x - tolerance <= m_extent.hi.x &&
		pos.y + tolerance >= m_extent.lo.y && pos.y - tolerance <= m_extent.hi.y)
		return true;

	return false;
}

float CNavArea::GetZ(const Vector& pos) const
{
	float dx = m_extent.hi.x - m_extent.lo.x;
	float dy = m_extent.hi.y - m_extent.lo.y;

	// guard against division by zero due to degenerate areas
	if (dx == 0.0f || dy == 0.0f)
		return m_neZ;

	float u = (pos.x - m_extent.lo.x) / dx;
	float v = (pos.y - m_extent.lo.y) / dy;

	// clamp Z values to (x,y) volume
	if (u < 0.0f)
		u = 0.0f;
	else if (u > 1.0f)
		u = 1.0f;

	if (v < 0.0f)
		v = 0.0f;
	else if (v > 1.0f)
		v = 1.0f;

	float northZ = m_extent.lo.z + u * (m_neZ - m_extent.lo.z);
	float southZ = m_swZ + u * (m_extent.hi.z - m_swZ);

	return northZ + v * (southZ - northZ);
}

float CNavArea::GetZ(float x, float y) const
{
	Vector pos(x, y, 0.0f);
	return GetZ(pos);
}

void CNavArea::GetClosestPointOnArea(const Vector& pos, Vector* close) const
{
	const Extent& extent = GetExtent();

	if (pos.x < extent.lo.x)
	{
		if (pos.y < extent.lo.y)
		{
			// position is north-west of area
			*close = extent.lo;
		}
		else if (pos.y > extent.hi.y)
		{
			// position is south-west of area
			close->x = extent.lo.x;
			close->y = extent.hi.y;
		}
		else
		{
			// position is west of area
			close->x = extent.lo.x;
			close->y = pos.y;
		}
	}
	else if (pos.x > extent.hi.x)
	{
		if (pos.y < extent.lo.y)
		{
			// position is north-east of area
			close->x = extent.hi.x;
			close->y = extent.lo.y;
		}
		else if (pos.y > extent.hi.y)
		{
			// position is south-east of area
			*close = extent.hi;
		}
		else
		{
			// position is east of area
			close->x = extent.hi.x;
			close->y = pos.y;
		}
	}
	else if (pos.y < extent.lo.y)
	{
		// position is north of area
		close->x = pos.x;
		close->y = extent.lo.y;
	}
	else if (pos.y > extent.hi.y)
	{
		// position is south of area
		close->x = pos.x;
		close->y = extent.hi.y;
	}
	else
	{
		// position is inside of area - it is the 'closest point' to itself
		*close = pos;
	}

	close->z = GetZ(*close);
}

float CNavArea::GetDistanceSquaredToPoint(const Vector& pos) const
{
	const Extent& extent = GetExtent();

	if (pos.x < extent.lo.x)
	{
		if (pos.y < extent.lo.y)
		{
			// position is north-west of area
			return (extent.lo - pos).LengthSqr();
		}
		else if (pos.y > extent.hi.y)
		{
			// position is south-west of area
			Vector d;
			d.x = extent.lo.x - pos.x;
			d.y = extent.hi.y - pos.y;
			d.z = m_swZ - pos.z;
			return d.LengthSqr();
		}
		else
		{
			// position is west of area
			float d = extent.lo.x - pos.x;
			return d * d;
		}
	}
	else if (pos.x > extent.hi.x)
	{
		if (pos.y < extent.lo.y)
		{
			// position is north-east of area
			Vector d;
			d.x = extent.hi.x - pos.x;
			d.y = extent.lo.y - pos.y;
			d.z = m_neZ - pos.z;
			return d.LengthSqr();
		}
		else if (pos.y > extent.hi.y)
		{
			// position is south-east of area
			return (extent.hi - pos).LengthSqr();
		}
		else
		{
			// position is east of area
			float d = pos.z - extent.hi.x;
			return d * d;
		}
	}
	else if (pos.y < extent.lo.y)
	{
		// position is north of area
		float d = extent.lo.y - pos.y;
		return d * d;
	}
	else if (pos.y > extent.hi.y)
	{
		// position is south of area
		float d = pos.y - extent.hi.y;
		return d * d;
	}
	else
	{
		// position is inside of 2D extent of area - find delta Z
		float z = GetZ(pos);
		float d = z - pos.z;
		return d * d;
	}
}

CNavArea* CNavArea::GetRandomAdjacentArea(NavDirType dir) const
{
	CNavArea* res = nullptr;
	int count = static_cast<int>(m_connect[dir].size());
	if (count > 0) {
		int which = rand() % count;

		int i = 0;
		for (auto& nav_connect : m_connect[dir]) {
			if (i == which) {
				res = nav_connect.area;
				break;
			}
			++i;
		}
	}
	return res;
}

void CNavArea::ConnectTo(CNavArea* area, NavDirType dir)
{
	// don't allow self-referential connections
	if (area == this)
		return;

	// check if already connected
	for (auto& connect : m_connect[dir]) {
		if (connect == area) {
			return;
		}
	}

	NavConnect con;
	con.area = area;
	m_connect[dir].push_back(con);
}

void CNavArea::Disconnect(CNavArea* area)
{
	NavConnect connect;
	connect.area = area;

	for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
	{
		m_connect[dir].remove(connect);
	}
}

bool CNavArea::IsConnected(const CNavArea* area, NavDirType dir) const
{
	// we are connected to ourself
	if (area == this)
		return true;

	if (dir == NUM_DIRECTIONS)
	{
		// search all directions
		for (int d = 0; d < NUM_DIRECTIONS; ++d)
		{
			for (auto& connection : m_connect[d]) {
				if (area == connection) {
					return true;
				}
			}
		}

		// check ladder connections
		//FOR_EACH_LL(m_ladder[CNavLadder::LADDER_UP], it)
		//{
		//	CNavLadder* ladder = m_ladder[CNavLadder::LADDER_UP][it].ladder;
		//
		//	if (ladder->m_topBehindArea == area ||
		//		ladder->m_topForwardArea == area ||
		//		ladder->m_topLeftArea == area ||
		//		ladder->m_topRightArea == area)
		//		return true;
		//}

		//FOR_EACH_LL(m_ladder[CNavLadder::LADDER_DOWN], dit)
		//{
		//	CNavLadder* ladder = m_ladder[CNavLadder::LADDER_DOWN][dit].ladder;
		//
		//	if (ladder->m_bottomArea == area)
		//		return true;
		//}
	}
	else
	{
		// check specific direction
		for (auto& connection : m_connect[dir]) {
			if (area == connection) {
				return true;
			}
		}
	}

	return false;
}

float CNavArea::ComputeHeightChange(const CNavArea* area)
{
	float ourZ = GetZ(GetCenter());
	float areaZ = area->GetZ(area->GetCenter());

	return areaZ - ourZ;
}

bool CNavArea::IsRoughlySquare(void) const
{
	if (GetSizeY() == 0) {
		return false;
	}

	float aspect = GetSizeX() / GetSizeY();

	constexpr float maxAspect = 3.01f;
	constexpr float minAspect = 1.0f / maxAspect;
	if (aspect < minAspect || aspect > maxAspect)
		return false;

	return true;
}

bool CNavArea::IsOverlapping(const CNavArea* area) const
{
	if (area->m_extent.lo.x < m_extent.hi.x && area->m_extent.hi.x > m_extent.lo.x &&
		area->m_extent.lo.y < m_extent.hi.y && area->m_extent.hi.y > m_extent.lo.y)
		return true;

	return false;
}

bool CNavArea::IsOverlappingX(const CNavArea* area) const
{
	if (area->m_extent.lo.x < m_extent.hi.x && area->m_extent.hi.x > m_extent.lo.x)
		return true;

	return false;
}

bool CNavArea::IsOverlappingY(const CNavArea* area) const
{
	if (area->m_extent.lo.y < m_extent.hi.y && area->m_extent.hi.y > m_extent.lo.y)
		return true;

	return false;
}

bool CNavArea::Contains(const Vector& pos) const
{
	// check 2D overlap
	if (!IsOverlapping(pos))
		return false;

	// the point overlaps us, check that it is above us, but not above any areas that overlap us
	float myZ = GetZ(pos);

	// if the nav area is above the given position, fail
	if (myZ > pos.z)
		return false;

	
	for (auto& p : m_overlapList) {
		const CNavArea* area = p;

		// skip self
		if (area == this)
			continue;

		// check 2D overlap
		if (!area->IsOverlapping(pos))
			continue;

		float theirZ = area->GetZ(pos);
		if (theirZ > pos.z)
		{
			// they are above the point
			continue;
		}

		if (theirZ > myZ)
		{
			// we are below an area that is beneath the given position
			return false;
		}
	}

	return true;
}

void CNavArea::AddToOpenList(void)
{
	// mark as being on open list for quick check
	m_openMarker = m_masterMarker;

	// if list is empty, add and return
	if (m_openList == nullptr)
	{
		m_openList = this;
		this->m_prevOpen = nullptr;
		this->m_nextOpen = nullptr;
		return;
	}

	// insert self in ascending cost order
	CNavArea* area, * last = nullptr;
	for (area = m_openList; area; area = area->m_nextOpen)
	{
		if (this->GetTotalCost() < area->GetTotalCost())
			break;

		last = area;
	}

	if (area)
	{
		// insert before this area
		this->m_prevOpen = area->m_prevOpen;
		if (this->m_prevOpen)
			this->m_prevOpen->m_nextOpen = this;
		else
			m_openList = this;

		this->m_nextOpen = area;
		area->m_prevOpen = this;
	}
	else
	{
		// append to end of list
		last->m_nextOpen = this;

		this->m_prevOpen = last;
		this->m_nextOpen = nullptr;
	}
}

void CNavArea::UpdateOnOpenList(void)
{
	// since value can only decrease, bubble this area up from current spot
	while ((m_prevOpen) && this->GetTotalCost() < m_prevOpen->GetTotalCost())
	{
		// swap position with predecessor
		auto other = m_prevOpen;
		auto before = other->m_prevOpen;
		auto after = this->m_nextOpen;

		this->m_nextOpen = other;
		this->m_prevOpen = before;

		other->m_prevOpen = this;
		other->m_nextOpen = after;

		if (before)
			before->m_nextOpen = this;
		else
			m_openList = this;

		if (after)
			after->m_prevOpen = other;
	}
}

void CNavArea::SetCorner(NavCornerType corner, const Vector& newPosition)
{
	switch (corner)
	{
	case NORTH_WEST:
		m_extent.lo = newPosition;
		break;

	case NORTH_EAST:
		m_extent.hi.x = newPosition.x;
		m_extent.lo.y = newPosition.y;
		m_neZ = newPosition.z;
		break;

	case SOUTH_WEST:
		m_extent.lo.x = newPosition.x;
		m_extent.hi.y = newPosition.y;
		m_swZ = newPosition.z;
		break;

	case SOUTH_EAST:
		m_extent.hi = newPosition;
		break;

	default:
	{
		Vector oldPosition = GetCenter();
		Vector delta = newPosition - oldPosition;
		m_extent.lo += delta;
		m_extent.hi += delta;
		m_neZ += delta.z;
		m_swZ += delta.z;
	}
	}

	m_center.x = (m_extent.lo.x + m_extent.hi.x) / 2.0f;
	m_center.y = (m_extent.lo.y + m_extent.hi.y) / 2.0f;
	m_center.z = (m_extent.lo.z + m_extent.hi.z) / 2.0f;
}
