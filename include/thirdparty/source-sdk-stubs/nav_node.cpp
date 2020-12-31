#include "nav_node.h"

#include "thirdparty/source-sdk-stubs/mathlib/vector.h"

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

constexpr NavDirType Opposite[NUM_DIRECTIONS] = { SOUTH, WEST, NORTH, EAST };

CNavNode* CNavNode::m_list = NULL;
unsigned int CNavNode::m_listLength = 0;
unsigned int CNavNode::m_nextID = 1;

CNavNode::CNavNode(const Vector& pos, const Vector& normal, CNavNode* parent)
{
	m_pos = pos;
	m_normal = normal;

	m_id = m_nextID++;

	int i;
	for (i = 0; i < NUM_DIRECTIONS; ++i)
	{
		m_to[i] = NULL;
	}

	m_next = m_list;
	m_list = this;
	m_listLength++;

	m_attributeFlags = 0;
}

void CNavNode::ConnectTo(CNavNode* node, NavDirType dir)
{
	m_to[dir] = node;
}

// Return node at given position.
CNavNode* CNavNode::GetNode(const Vector& pos)
{
	constexpr float generation_step_size = 1.0f;
	constexpr float tolerance = 0.45f * generation_step_size;

	for (CNavNode* node = m_list; node; node = node->m_next)
	{
		float dx = fabsf(node->m_pos.x - pos.x);
		float dy = fabsf(node->m_pos.y - pos.y);
		float dz = fabsf(node->m_pos.z - pos.z);

		if (dx < tolerance && dy < tolerance && dz < tolerance)
			return node;
	}

	return nullptr;
}

// Return true if this node is bidirectionally linked to
// another node in the given direction
bool CNavNode::IsBiLinked(NavDirType dir) const
{
	if (m_to[dir] && m_to[dir]->m_to[Opposite[dir]] == this)
	{
		return true;
	}

	return false;
}

// Return true if this node is the NW corner of a quad of nodes
// that are all bidirectionally linked.
bool CNavNode::IsClosedCell(void) const
{
	if (IsBiLinked(SOUTH) &&
		IsBiLinked(EAST) &&
		m_to[EAST]->IsBiLinked(SOUTH) &&
		m_to[SOUTH]->IsBiLinked(EAST) &&
		m_to[EAST]->m_to[SOUTH] == m_to[SOUTH]->m_to[EAST])
	{
		return true;
	}

	return false;
}
