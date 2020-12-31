#ifndef _NABENABE_NAV_NODE_H_
#define _NABENABE_NAV_NODE_H_

#include "thirdparty/source-sdk-stubs/mathlib/vector.h"
#include "thirdparty/source-sdk-stubs/nav.h"

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

class CNavNode
{
public:
	CNavNode(const Vector& pos, const Vector& normal, CNavNode* parent = NULL);

	static CNavNode* GetNode(const Vector& pos); // return navigation node at the position, or NULL if none exists

	CNavNode* GetConnectedNode(NavDirType dir) const;				///< get navigation node connected in given direction, or NULL if cant go that way
	const Vector* GetPosition(void) const;
	const Vector* GetNormal(void) const { return &m_normal; }
	unsigned int GetID(void) const { return m_id; }

	static CNavNode* GetFirst(void) { return m_list; }
	static unsigned int GetListLength(void) { return m_listLength; }
	CNavNode* GetNext(void) { return m_next; }

	void ConnectTo(CNavNode* node, NavDirType dir);				///< create a connection FROM this node TO the given node, in the given direction
	CNavNode* GetParent(void) const;

	void MarkAsVisited(NavDirType dir);							///< mark the given direction as having been visited
	bool HasVisited(NavDirType dir);								///< return TRUE if the given direction has already been searched
	bool IsBiLinked(NavDirType dir) const;						///< node is bidirectionally linked to another node in the given direction
	bool IsClosedCell(void) const;								///< node is the NW corner of a bi-linked quad of nodes

	void AssignArea(CNavArea* area);								///< assign the given area to this node
	CNavArea* GetArea(void) const;								///< return associated area

	void SetAttributes(unsigned char bits) { m_attributeFlags = bits; }
	unsigned char GetAttributes(void) const { return m_attributeFlags; }

private:
	friend class CNavMesh;

	Vector m_pos;													///< position of this node in the world
	Vector m_normal;												///< surface normal at this location
	CNavNode* m_to[NUM_DIRECTIONS];								///< links to north, south, east, and west. NULL if no link
	unsigned int m_id;												///< unique ID of this node
	unsigned char m_attributeFlags;									///< set of attribute bit flags (see NavAttributeType)

	static CNavNode* m_list;										///< the master list of all nodes for this map
	static unsigned int m_listLength;
	static unsigned int m_nextID;

	unsigned char m_visited;										///< flags for automatic node generation. If direction bit is clear, that direction hasn't been explored yet.
	CNavNode* m_next;												///< next link in master list
	CNavNode* m_parent;												///< the node prior to this in the search, which we pop back to when this node's search is done (a stack)
	bool m_isCovered;												///< true when this node is "covered" by a CNavArea
	CNavArea* m_area;												///< the area this node is contained within
};

inline CNavNode* CNavNode::GetConnectedNode(NavDirType dir) const
{
	return m_to[dir];
}

inline const Vector* CNavNode::GetPosition(void) const
{
	return &m_pos;
}

inline CNavNode* CNavNode::GetParent(void) const
{
	return m_parent;
}

inline void CNavNode::MarkAsVisited(NavDirType dir)
{
	m_visited |= (1 << dir);
}

inline bool CNavNode::HasVisited(NavDirType dir)
{
	if (m_visited & (1 << dir)) {
		return true;
	}
	return false;
}

inline void CNavNode::AssignArea(CNavArea* area)
{
	m_area = area;
}

inline CNavArea* CNavNode::GetArea(void) const
{
	return m_area;
}

#endif // _NABENABE_NAV_NODE_H_
