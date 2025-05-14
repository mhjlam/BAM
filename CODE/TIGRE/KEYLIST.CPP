//
// KEYLIST.CPP
//
// November 23, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "keylist.hpp"
#include "savebase.hpp"

// constructor - call super's constructor
KeyList::KeyList(uint16 max, bool resize) : List(max, resize)
{
	fNeedsResort = FALSE;
}

bool
KeyList::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&keyListDataStart, 1, (int)&keyListDataEnd -
				(int)&keyListDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&keyListDataStart, 1, (int)&keyListDataEnd -
				(int)&keyListDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

// Find first node that matches key.
// Returns NULL if no match is found.
node*
KeyList::FirstKey(int32 key)
{
	uint		i;
	node*		pCurrent;

	for (i = first; i; i = pCurrent->next)
	{
		pCurrent = GetPNode(i);
		if (pCurrent->key == key)
		{
			return pCurrent;
		}
	}
	return NULL;
}


// Find last node that matches key.
// Returns NULL if no match is found.
node*
KeyList::LastKey(int32 key)
{
	uint		i;
	node*		pCurrent;

	for (i = last; i; i = pCurrent->prev)
	{
		pCurrent = GetPNode(i);
		if (pCurrent->key == key)
		{
			return pCurrent;
		}
	}
	return NULL;
}


// Find next node that matches key.
// Returns NULL if no match is found.
node*
KeyList::NextKey(node* pNode, int32 key)
{
	uint		i;
	node*		pCurrent;

	if (pNode->next)
	{
		pCurrent = GetPNode(pNode->next);
		for (i = pCurrent->index; i; i = pCurrent->next)
		{
			pCurrent = GetPNode(i);
			if (pCurrent->key == key)
			{
				return pCurrent;
			}
		}
	}
	return NULL;
}

node*
KeyList::NextKey(void* id, int32 key)
{
	node* pNode = Find(id);
	return NextKey(pNode, key);
}


// Find previous node that matches key
node*
KeyList::PrevKey(node* pNode, int32 key)
{
	uint		i;
	node*		pCurrent;

	if (pNode->prev)
	{
		pCurrent = GetPNode(pNode->prev);
		for (i = pCurrent->index; i; i = pCurrent->prev)
		{
			pCurrent = GetPNode(i);
			if (pCurrent->key == key)
			{
				return pCurrent;
			}
		}
	}
	return NULL;
}

node*
KeyList::PrevKey(void* id, int32 key)
{
	node* pNode = Find(id);
	return PrevKey(pNode, key);
}


// Delete all nodes whose keys match.
// Returns new count.
uint16
KeyList::DeleteKey(int32 key)
{
	uint		i;
	uint		nextNode = 0;
	node*		pCurrent;

	for (i = 1; i <= count; i = nextNode)
	{
		pCurrent = GetPNode(i);
		if (pCurrent->key == key)
		{
			Delete(pCurrent->value);
			nextNode = i;
		}
		else
		{
			nextNode = i + 1;
		}
	}
	return count;
}


// Return count of all nodes whose keys match
uint16
KeyList::CountKey(int32 key)
{
	uint16	i;
	uint16	theCount = 0;

	for (i = 1; i <= count; i++)
	{
		if (GetPNode(i)->key == key)
		{
			++theCount;
		}
	}
	return theCount;
}


//---------------------------------------------------------
// qsort() the elements of List by key

#ifdef OS_MAC
	// some odd difference in compiler's requires the following on the mac
	// and prohibits it on the pc!
	extern "C"	int	SortDescendingKey(const void* pElem1, const void* pElem2);
	extern "C"	int	SortAscendingKey(const void* pElem1, const void* pElem2);
#endif

int
SortDescendingKey(const void* pElem1, const void* pElem2)
{
	node	node1 = *(node*) pElem1;
	node	node2 = *(node*) pElem2;

	//
	if (node1.key == node2.key)
	{
		if (node1.index == node1.index)
		{
			return 0;
		}

		// we should never switch order if keys are the same,
		// so return the order according to the order prior to sort
		//
		if (node1.index > node1.index)
		{
			return 1;
		}
		if (node1.index < node1.index)
		{
			return -1;
		}
	}

	// higher key goes later in list
	if (node1.key >  node2.key)
	{
		return -1;
	}

	// lower key goes earlier in list
	if (node1.key <  node2.key)
	{
		return 1;
	}

	// shouldn't get here
	ASSERT(FALSE);
	return 0;
}


//---------------------------------------------------------
// qsort() the elements of List by key

int
SortAscendingKey(const void* pElem1, const void* pElem2)
{
	node	node1 = *(node*) pElem1;
	node	node2 = *(node*) pElem2;

	//	WARNING! Compiler bug avoidance!
	// 
	// The == test is done first because the Symantec compiler
	// was treating the greater-than test as greater-or-equal
	//

	if (node1.key == node2.key)
	{
		// qsort may compare node to itself, if so return 0
		if (node1.index == node2.index)
		{
			return 0;
		}

		// we should never switch order if keys are the same,
		// so return the order according to the order prior to sort
		if (node1.index > node2.index)
		{
			return 1;
		}
		if (node1.index < node2.index)
		{
			return -1;
		}
	}

	// higher key goes later in list
	if (node1.key >  node2.key)
	{
		return 1;
	}

	// lower key goes earlier in list
	if (node1.key <  node2.key)
	{
		return -1;
	}

	// shouldn't get here
	ASSERT(FALSE);
	return 0;
}


void
KeyList::SortByKey(int sortType)
{
	node*		pNode;
	uint16	i;

	Verify("list is broke");

	if (count > 1)
	{
		pNode = GetPNode(1);

		switch (sortType)
		{
			case SORT_DESCENDING:
				// sort physical nodes by key
				OS_qsort((void*) pNode, count, sizeof(node), SortDescendingKey);
				break;

			case SORT_ASCENDING:
				// sort physical nodes by key
				OS_qsort((void*) pNode, count, sizeof(node), SortAscendingKey);
				break;

			default:
				break;
		}

		// set logical order to match phyiscal order
		pNode = GetPNode(1);

		for (i = 1; i <= count; i++)
		{
			pNode[i - 1].prev = i - 1;
			pNode[i - 1].next = i + 1;
			pNode[i - 1].index = i;
		}
		pNode[count - 1].next = 0;

		fNeedsResort = FALSE;

		first = 1;
		last = count;
	}
}
