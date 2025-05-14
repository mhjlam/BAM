//
// LIST.CPP
//
// November 12, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apimem.hpp"
#include "api.hpp"
#include "list.hpp"
#include "mem.hpp"
#include "mono.hpp"
#include "savebase.hpp"

// Default constructor
// Allocate memory enough to hold 'max' number of nodes
List::List(uint16 max, bool resize)
{
	if (max > 0)
	{
		TRACK_MEM("List data");
		gData = AMalloc(max * sizeof(node));
		memset(ADeref(gData), NULL, max * sizeof(node));
	}
	else
	{
		gData = NULL;
	}

	count = 0;
	resizeCount = maxCount = max;
	resizable = resize;
	fDuplicates = TRUE;
	first = NULL;
	last = NULL;
	next = NULL;
}


// Default destructor
// Deallocate the data area
List::~List()
{
	if (count)
	{
		char buffer[60];
		
		sprintf(buffer, "First item in list: %d\n\n", At(first));
		if (pMono) pMono->Out(buffer);

		sprintf(buffer, "<~List> - List not empty: %s\n", name);
		APanic(buffer);
	}

	if (gData) 
	{
		AFree(gData);
	}
}


//----[ List Functions ]---------------------------------------------

bool
List::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&listDataStart, 1, (int)&listDataEnd -
				(int)&listDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&listDataStart, 1, (int)&listDataEnd -
				(int)&listDataStart, pFile);
			break;
	}
	return(TRUE);
}

// Find the value of the first node
void*
List::FirstValue(void)
{
	return NodeValue(First());
}

// Find the value of the last node
void*
List::LastValue(void)
{
	return NodeValue(Last());
}

// Find the value of the next node
// NOTE: this uses the List's next index
void*
List::NextValue(void)
{
	return NodeValue(Next());
}


// Find the first node in the list.
node*
List::First()
{
	node*	firstNode;

	if (!count)
	{
		return NULL;
	}

	firstNode = mNodeAt(first);

	if (firstNode) 
	{
		// store the next node index
		next = firstNode->next; 
	}

	return firstNode;
}


// Find the last node in the list.
node*
List::Last()
{
	if (!count)
	{
		return NULL;
	}
	return mNodeAt(last);
}

// Find the next node using the List's next node variable 
node*
List::Next(void)
{
	node *nextNode;

	if (next == 0) {
		// no next node
		nextNode = NULL;
	}
	else 
	{
		nextNode = GetPNode(next);
	}

	// if a node was found, store the next index
	if (nextNode)
	{
		next = nextNode->next;
	}

	return nextNode;
}

// Find the node that is next (after) the given node or id.
// Returns NULL if there is no next node.
node*
List::Next(node* pNode)
{
	if (pNode->next == 0) return NULL;
	return mNodeAt(pNode->next);
}

node*
List::Next(void* id)
{
	node* pNode = Find(id);
	return Next(pNode);
}



// Find the node that is previous (before) the given node or id.
// Returns NULL if there is no previous node.
node*
List::Prev(node* pNode)
{
	if (pNode->prev == 0) return NULL;
	return mNodeAt(pNode->prev);
}

node*
List::Prev(void* id)
{
	node* pNode = Find(id);
	return Prev(pNode);
}



// Add a node to the list, given an id, a key, a position, and an
// optional target id.
// If the third parameter is passed it will be:
//		L_FRONT 	= node should be inserted at the front of the list
//		L_END		= node should be appended to the list (default)
//		L_AFTER	= node should be inserted after the target node
// Returns NULL if the list is out of room and not resizable,
// the new count of the list otherwise
uint16
List::Add(void* id, int32 key, uint16 posn, void* target)
{
	// check is the list allow duplicate entries
	if ((!fDuplicates) && (Find(id)))
	{
		return (count);
	}

	// check for need to resize list
	if (count >= maxCount)
	{
		if (!resizable)
		{
			char buffer[40];
			sprintf(buffer, "<List> - List is full: %s\n", name);
			APanic(buffer);
		}
		else
		{
			Resize(maxCount + resizeCount);
		}
	}

	node* dgCurNode = mNodeAt(++count);
	dgCurNode->value = id;
	dgCurNode->index = count;
	dgCurNode->key = key;
	node* dgNode;

	switch (posn)
	{
		case L_FRONT:
			dgCurNode->next = first;
			dgCurNode->prev = NULL;
			if (first)
			{
				dgNode = mNodeAt(first);
				dgNode->prev = count;
			}
			first = count;
			break;

		case L_END:
			dgCurNode->next = NULL;
			dgCurNode->prev = last;
			if (last)
			{
				dgNode = mNodeAt(last);
				dgNode->next = dgCurNode->index;
			}
			last = count;
			break;

		case L_AFTER:
			dgNode = Find(target);
			if (!target)
			{
				APanic("<List::Add> No target specified for L_AFTER");
			}

			if (!dgNode)
			{
				APanic("<List::Add> Specified target not found");
			}
			dgCurNode->next = dgNode->next;
			dgCurNode->prev = dgNode->index;
			dgNode->next = dgCurNode->index;
			if (dgCurNode->next)
			{
				dgNode = mNodeAt(dgCurNode->next);
				dgNode->prev = dgCurNode->index;
			}

			// If we're adding after the last element, we're now the last element
			if (!dgCurNode->next)
			{
				last = dgCurNode->index;
			}
			break;
	}
	if (!first)
	{
		first = dgCurNode->index;
	}
	if (!last)
	{
		last = dgCurNode->index;
	}
	return count;
}


// Delete a node from the list, given an id or a node*.
// The last node is moved to fill a hole left by a delete
// Returns NULL if the delete failed, the new count otherwise.
uint16
List::Delete(node* pNode)
{
	uint	idx;

	if (!pNode)
	{
		APanic("<List::Delete> Specified node is NULL");
	}

	idx = pNode->index;
	node* dgNextNode = NULL;
	node* dgPrevNode = NULL;

	// First, bridge the gap over the deleted node
	if (pNode->prev)
	{
		dgPrevNode = mNodeAt(pNode->prev);
		dgPrevNode->next = pNode->next;
	}
	if (pNode->next)
	{
		dgNextNode = mNodeAt(pNode->next);
		dgNextNode->prev = pNode->prev;
	}

	// Second, fix up the first or last indeces if necessary
	if (first == idx)
	{
		first = pNode->next;
	}
	if (last == idx)
	{
		last = pNode->prev;
	}

	// Now, check to see if the next node that we are going to look
	// at is the one that we are deleting
	if (pNode->index == next)
	{
		// the next node will now be the next node of the one that we are
		// deleting
		next = pNode->next;
	}

	--count;

	if (idx <= count)
	{
		// Third, move the last node to the new "hole"
		node* dgEndNode = mNodeAt(count + 1);
		memcpy(pNode, dgEndNode, sizeof(node));

		// Last, adjust the pointers to the moved node
		if (pNode->next)
		{
			dgNextNode = mNodeAt(pNode->next);
			dgNextNode->prev = idx;
		}
		if (pNode->prev)
		{
			dgPrevNode = mNodeAt(pNode->prev);
			dgPrevNode->next = idx;
		}
		pNode->index = idx;
		if (last == count + 1)
		{
			last = idx;
		}
		if (first == count + 1)
		{
			first = idx;
		}
	}
	return count;
}

uint16
List::Delete(void* id)
{
	node*	pNode = Find(id);
	if (!pNode)
	{
		return count;
	}
	return Delete(pNode);
}



// Return the node identified by the id or
// NULL if the node isn't found
node*
List::Find(void* id)
{
	uint16 	i;
	node*	 	dgCurNode;

	for (i = 1; i <= count; i++)
	{
		dgCurNode = mNodeAt(i);
		if (dgCurNode->value == id)
		{
			return dgCurNode;
		}
	}
	return FALSE;
}


// Move the node identified by 'id' to a new position, specified
// by 'posn' and optionally relative to the id 'target'.
// The second parameter determines the new position:
//		L_FRONT 	= node should be moved to the front of the list
//		L_END		= node should be moved to the list
//		L_AFTER	= node should be moved to after the target node
// Returns TRUE if successful, FALSE otherwise
bool
List::Move(void* id, uint16 posn, void* target)
{
	int32 key = Find(id)->key;
	Delete(id);

	switch (posn)
	{
		case L_FRONT:
			Add(id, key, L_FRONT);
			break;

		case L_END:
			Add(id, key, L_END);
			break;

		case L_AFTER:
			if (!target)
			{
				APanic("<List::Move> No target specified for L_AFTER");
			}

			Add(id, key, L_AFTER, target);
			break;
	}
	return TRUE;
}


// Given a logical 1-relative index, return a pointer to the node at that index
node*
List::GetNode(uint index)
{
	uint		i;
	uint16	testCount;
	node*	 	dgCurNode;
	char 		szPanicBuffer[40];

	if (index > count)
	{
		sprintf(szPanicBuffer, "<List::GetNode> Index out of range (%d:1-%d)",
					index, count);
		APanic(szPanicBuffer);
	}

	i = first;
	testCount = 1;
	do
	{
		dgCurNode = mNodeAt(i);
		if (testCount++ == index)
		{
			break;
		}
		i = dgCurNode->next;
		if (!i)
		{
			sprintf(szPanicBuffer, "<List::GetNode, %s> Index missing: %d", name, index + 1);
			APanic(szPanicBuffer);
		}
	} while (TRUE);

	return dgCurNode;
}


// Given a physical 1-relative index, return a pointer to the
// node at that index.
node*
List::GetPNode(uint index)
{
	return mNodeAt(index);
}


// Verify that the list chain is intact.
// If an error is found, an optional message may be displayed.
void
List::Verify(char* msg)
{
	uint		i;
	uint16	c = 0;
	node*		dgCurNode;

	i = first;
	if (!i)
	{
		return;
	}

	do
	{
		dgCurNode = mNodeAt(i);
		++c;
		i = dgCurNode->next;
	} while (i);

	if (c != count)
	{
		if (strlen(msg))
		{
			printf(msg);
		}
		char	szPanicBuffer[40];
		sprintf(szPanicBuffer, "<List::Verify> Verify failed.  Found %d nodes of %d\n",
					c, count);
		APanic(szPanicBuffer);
	}
}


// Release all nodes so that the list can be deleted
void
List::Release()
{
	node* theNode;

	while (count)
	{
		theNode = GetPNode(1);
		Delete(theNode->value);
	}
}


// Return the value of a node.
void*
List::NodeValue(node* pNode)
{
	if (pNode)
	{
		return pNode->value;
	}
	else 
	{
		// no node
		return NULL;	
	}
}


// Shortcut routine to return the value at a 1-relative index.
void*
List::At(uint index)
{
	node* pNode = GetNode(index);
	return NodeValue(pNode);
}



//----[ Private Functions ]------------------------------------------

// Resize the list to 'num' capacity
// WARNING: MemMgr::Realloc() can move a deref'ed ptr
bool
List::Resize(uint num)
{
	if (num < count)
	{
		APanic("<List::Resize> List cannot be shrunk");
	}
	else
	{
		if (pMono)
			pMono->Out("Resizing list\n");

		TRACK_MEM("List buf resize");
		ARealloc(gData, num * sizeof(node));
		maxCount = (uint16) num;
	}
	return TRUE;
}


//-------------------------------------------------------------------------
//
// SysList 
//
// A list class that uses system grips.
//
//-------------------------------------------------------------------------

// Default constructor

// Allocate memory enough to hold 'max' number of nodes
SysList::SysList(uint16 max, bool resize) : List(0)
{
	TRACK_MEM("SysList data");
	gData = ASysMalloc(max * sizeof(node));
	memset(ADeref(gData), NULL, max * sizeof(node));
	count = 0;
	resizeCount = maxCount = max;
	resizable = resize;
	fDuplicates = TRUE;
	first = NULL;
	last = NULL;
	next = NULL;
}

void * 
SysList:: operator new(size_t size)
{
	// allocate this using a system grip
	return newSys(size);	
}


