#ifndef _NABENABE_PATHFIND_H_
#define _NABENABE_PATHFIND_H_

#include "thirdparty/source-sdk-stubs/nav.h"
#include "thirdparty/source-sdk-stubs/nav_area.h"

#include <unordered_set>

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

/**
 * Used when building a path to determine the kind of path to build
 */
enum RouteType
{
	FASTEST_ROUTE,
	SAFEST_ROUTE,
};

float CostFunctor(CNavArea* area, CNavArea* fromArea)
{
	if (fromArea == nullptr)
	{
		// first area in path, no cost
		return 0.0f;
	}
	else
	{
		// compute distance travelled along path so far
		float dist;

#if(0)
		if (ladder)
			dist = ladder->m_length;
		else
			dist = (area->GetCenter() - fromArea->GetCenter()).Length();
#else
		dist = (area->GetCenter() - fromArea->GetCenter()).Length();
#endif

		float cost = dist + fromArea->GetCostSoFar();

		// if this is a "crouch" area, add penalty
		if (area->GetAttributes() & NAV_MESH_CROUCH)
		{
			const float crouchPenalty = 20.0f;		// 10
			cost += crouchPenalty * dist;
		}

		// if this is a "jump" area, add penalty
		if (area->GetAttributes() & NAV_MESH_JUMP)
		{
			const float jumpPenalty = 5.0f;
			cost += jumpPenalty * dist;
		}

		return cost;
	}
}

// Remove possibly non-contiguous duplicates without changing list order.
void RemoveDuplicates(NavAreaList& list)
{
	std::unordered_set<CNavArea*> s;
	list.remove_if([&](CNavArea* n) { return (s.find(n) == s.end()) ? (s.insert(n), false) : true; });
}

/**
 * Find path from startArea to goalArea via an A* search, using supplied cost heuristic.
 * If cost functor returns -1 for an area, that area is considered a dead end.
 * This doesn't actually build a path, but the path is defined by following parent
 * pointers back from goalArea to startArea.
 * If 'closestArea' is non-NULL, the closest area to the goal is returned (useful if the path fails).
 * If 'goalArea' is NULL, will compute a path as close as possible to 'goalPos'.
 * If 'goalPos' is NULL, will use the center of 'goalArea' as the goal position.
 * Returns true if a path exists.
 * If path exists, returns the path by reference in pathList.
 */
bool NavAreaBuildPath(CNavArea* startArea, CNavArea* goalArea, const Vector* goalPos, NavAreaList& pathList)
{
	if (startArea == NULL)
		return false;

	if (goalArea != NULL && goalArea->IsBlocked())
		goalArea = NULL;

	if (goalArea == NULL && goalPos == NULL)
		return false;

	startArea->SetParent(NULL);

	// if we are already in the goal area, build trivial path
	if (startArea == goalArea)
	{
		goalArea->SetParent(NULL);
		pathList.push_back(goalArea);
		return true;
	}

	// determine actual goal position
	Vector actualGoalPos = (goalPos) ? *goalPos : goalArea->GetCenter();

	// start search
	CNavArea::ClearSearchLists();

	// compute estimate of path length
	/// @todo Cost might work as "manhattan distance"
	startArea->SetTotalCost((startArea->GetCenter() - actualGoalPos).Length());

	float initCost = CostFunctor(startArea, NULL /*, NULL*/);
	if (initCost < 0.0f)
		return false;
	startArea->SetCostSoFar(initCost);

	startArea->AddToOpenList();

	// keep track of the area we visit that is closest to the goal
	float closestAreaDist = startArea->GetTotalCost();

	// do A* search
	while (!CNavArea::IsOpenListEmpty())
	{
		// get next area to check
		CNavArea* area = CNavArea::PopOpenList();
		if (!area) {
			print(Error, "%s: Area == nullptr", __FUNCTION__);
			return false;
		}

		// don't consider blocked areas
		if (area->IsBlocked())
			continue;

		// check if we have found the goal area or position
		if (area == goalArea || (goalArea == NULL && goalPos && area->Contains(*goalPos)))
		{
			pathList.push_back(area);
			for (CNavArea* parent = area->GetParent(); parent != nullptr; parent = parent->GetParent()) {
				pathList.push_back(parent);
			}
			pathList.reverse();

			auto testList = pathList;
			const size_t size_before_removing_duplicates = testList.size();
			RemoveDuplicates(testList);
			const size_t size_after_removing_duplicates = testList.size();
			if (size_before_removing_duplicates != size_after_removing_duplicates) {
				print(Warning, "%s: Circular paths detected!", __FUNCTION__);
			}

			return true;
		}

		// search adjacent areas
		bool searchFloor = true;

		for (int dir = NORTH; dir != NUM_DIRECTIONS; ++dir) {
			const NavConnectList* floorList = area->GetAdjacentList((NavDirType)dir);

			for (auto floorIter = floorList->begin(); floorIter != floorList->end(); ++floorIter)
			{
				CNavArea* newArea = nullptr;
				NavTraverseType how;
				//const CNavLadder* ladder = NULL;

				//
				// Get next adjacent area - either on floor or via ladder
				//
				if (searchFloor)
				{
					// if exhausted adjacent connections in current direction, begin checking next direction
					if (floorIter == floorList->end())// floorList->InvalidIndex())
					{
						if (++dir == NavDirType::NUM_DIRECTIONS)
						{
							// checked all directions on floor - check ladders next
							searchFloor = false;

							//ladderList = area->GetLadderList(CNavLadder::LADDER_UP);
							//ladderIter = ladderList->Head();
							//ladderTopDir = AHEAD;
						}
						else
						{
							// start next direction
							floorList = area->GetAdjacentList((NavDirType)dir);
							floorIter = floorList->begin();
						}

						continue;
					}

					newArea = floorIter->area; //  floorList->Element(floorIter).area;
					how = static_cast<NavTraverseType>(dir);
				}
#if(0)
				else	// search ladders
				{
					if (ladderIter == ladderList->InvalidIndex())
					{
						if (!ladderUp)
						{
							// checked both ladder directions - done
							break;
						}
						else
						{
							// check down ladders
							ladderUp = false;
							ladderList = area->GetLadderList(CNavLadder::LADDER_DOWN);
							ladderIter = ladderList->Head();
						}
						continue;
					}

					if (ladderUp)
					{
						ladder = ladderList->Element(ladderIter).ladder;

						// do not use BEHIND connection, as its very hard to get to when going up a ladder
						if (ladderTopDir == AHEAD)
							newArea = ladder->m_topForwardArea;
						else if (ladderTopDir == LEFT)
							newArea = ladder->m_topLeftArea;
						else if (ladderTopDir == RIGHT)
							newArea = ladder->m_topRightArea;
						else
						{
							ladderIter = ladderList->Next(ladderIter);
							ladderTopDir = AHEAD;
							continue;
						}

						how = GO_LADDER_UP;
						++ladderTopDir;
					}
					else
					{
						newArea = ladderList->Element(ladderIter).ladder->m_bottomArea;
						how = GO_LADDER_DOWN;
						ladder = ladderList->Element(ladderIter).ladder;
						ladderIter = ladderList->Next(ladderIter);
					}

					if (newArea == NULL)
						continue;
				}
#endif

				if (newArea == nullptr) {
					print(Error, "%s: newArea was nullptr", __FUNCTION__);
					break;
				}

				// don't backtrack
				if (newArea == area)
					continue;

				// don't consider blocked areas
				if (newArea->IsBlocked())
					continue;

				float newCostSoFar = CostFunctor(newArea, area/*, ladder*/);

				// check if cost functor says this area is a dead-end
				if (newCostSoFar < 0.0f)
					continue;

				if ((newArea->IsOpen() || newArea->IsClosed()) && newArea->GetCostSoFar() <= newCostSoFar)
				{
					// this is a worse path - skip it
					continue;
				}
				else
				{
					// compute estimate of distance left to go
					float newCostRemaining = (newArea->GetCenter() - actualGoalPos).Length();

					// track closest area to goal in case path fails
					if (newCostRemaining < closestAreaDist)
					{
						closestAreaDist = newCostRemaining;
					}

					newArea->SetParent(area, how);
					newArea->SetCostSoFar(newCostSoFar);
					newArea->SetTotalCost(newCostSoFar + newCostRemaining);

					if (newArea->IsClosed())
						newArea->RemoveFromClosedList();

					if (newArea->IsOpen())
					{
						// area already on open list, update the list order to keep costs sorted
						newArea->UpdateOnOpenList();
					}
					else
					{
						newArea->AddToOpenList();
					}
				}
			}
		}
		

		//bool ladderUp = true;
		//const NavLadderConnectList* ladderList = NULL;
		//int ladderIter = NavLadderConnectList::InvalidIndex();
		//enum { AHEAD = 0, LEFT, RIGHT, BEHIND, NUM_TOP_DIRECTIONS };
		//int ladderTopDir = AHEAD;

		

		// we have searched this area
		area->AddToClosedList();
	}

	return false;
}

#endif // _NABENABE_PATHFIND_H_
