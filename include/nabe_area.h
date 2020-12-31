#ifndef _NABENABE_NABE_AREA_H
#define _NABENABE_NABE_AREA_H

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

class NABE_Area : public CNavArea
{
public:
	friend class NABE_NavCoordinator;

	NABE_Area(CNavNode* nwNode, CNavNode* neNode, CNavNode* seNode, CNavNode* swNode)
		: CNavArea(nwNode, neNode, seNode, swNode)
	{
		Initialize();
	}

	NABE_Area(void) : CNavArea()
	{
		Initialize();
	}

	NABE_Area(const Vector& corner, const Vector& otherCorner)
		: CNavArea(corner, otherCorner)
	{
		Initialize();
	}

	NABE_Area(const Vector& nwCorner, const Vector& neCorner, const Vector& seCorner, const Vector& swCorner)
		: CNavArea(nwCorner, neCorner, seCorner, swCorner)
	{
		Initialize();
	}

	void CalculateCenter()
	{
		if (m_center != Vector(0, 0, 0)) {
			print(Error, "Non-zeroed center already");
		}
		m_center.x = (m_extent.lo.x + m_extent.hi.x) / 2.0f;
		m_center.y = (m_extent.lo.y + m_extent.hi.y) / 2.0f;
		m_center.z = (m_extent.lo.z + m_extent.hi.z) / 2.0f;
	}

	virtual ~NABE_Area() { }

private:
	void Initialize()
	{
		constexpr size_t num_elems_pending_appr_this = sizeof(m_pending_approaches_this_area_id) / sizeof(m_pending_approaches_this_area_id[0]);
		constexpr size_t num_elems_pending_appr_prev = sizeof(m_pending_approaches_prev_area_id) / sizeof(m_pending_approaches_prev_area_id[0]);
		constexpr size_t num_elems_pending_appr_next = sizeof(m_pending_approaches_next_area_id) / sizeof(m_pending_approaches_next_area_id[0]);

		std::fill_n(m_pending_approaches_this_area_id, num_elems_pending_appr_this, AREA_ID_NONE);
		std::fill_n(m_pending_approaches_prev_area_id, num_elems_pending_appr_prev, AREA_ID_NONE);
		std::fill_n(m_pending_approaches_next_area_id, num_elems_pending_appr_next, AREA_ID_NONE);

		for (auto& p : m_pending_approaches_this_area_id) {
			if (p != AREA_ID_NONE) {
				print(Error, "%s: Failed to initialize m_pending_approaches_this_area_id", __FUNCTION__);
			}
		}
		for (auto& p : m_pending_approaches_prev_area_id) {
			if (p != AREA_ID_NONE) {
				print(Error, "%s: Failed to initialize m_pending_approaches_prev_area_id", __FUNCTION__);
			}
		}
		for (auto& p : m_pending_approaches_next_area_id) {
			if (p != AREA_ID_NONE) {
				print(Error, "%s: Failed to initialize m_pending_approaches_next_area_id", __FUNCTION__);
			}
		}
	}

private:
	int m_pending_approaches_this_area_id[MAX_APPROACH_AREAS];
	int m_pending_approaches_prev_area_id[MAX_APPROACH_AREAS];
	int m_pending_approaches_next_area_id[MAX_APPROACH_AREAS];
};

#endif // _NABENABE_NABE_AREA_H
