//
// LIST.HPP
//
// November 12, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// List class definition.
//
//		The List class maintains a group of pointers or grips.
// 	(See doco below)
//
//----[]-------------------------------------------------------------


#ifndef	list_hpp
#define	list_hpp


#include "tigre.hpp"


// Return a node* to the node at the 1-relative, physical index 'x'. 
// Also used in GraphMgr.
#define	mNodeAt(x)	(&((node*) pMemMgr->Deref(gData))[x - 1])


enum
{
	L_NONE,
	L_FRONT,
	L_END,
	L_AFTER
};


class List : public Object
{
	public:

		List(uint16 max = 10, bool resize = FALSE);
		~List();
		virtual	bool		Save(uint16 state, FILE *pFile = NULL);

		virtual	void* 	FirstValue(void);
		virtual 	void* 	LastValue(void);
		virtual	void*		NextValue(void);

		virtual	node* 	First();
		virtual	node* 	Last();

		virtual	node* 	Next(node* pNode);
		virtual	node*		Next(void* id);
		virtual	node*		Next(void);

		virtual	node* 	Prev(node* pNode);
		virtual	node*		Prev(void* id);

		virtual	uint16 	Add(void* id, int32 key = 0, uint16 posn = L_END, void* target = 0);

		virtual	uint16	Delete(node* pNode);
		virtual	uint16 	Delete(void* id);

		virtual	node*		Find(void* id);

		virtual	bool 		Move(void* id, uint16 posn, void* pTarget = 0);

		virtual	node* 	GetNode(uint index);
		virtual	node*		GetPNode(uint index);
		virtual	void		Verify(char* msg = "");
		virtual	void		Release();
		virtual	void*		NodeValue(node* pNode);
		virtual	void*		At(uint index);
		virtual	bool		Resize(uint num);

		// these are public, in general use
		char		listDataStart;
		uint16	count;
		bool		fDuplicates;
		bool		resizable;
		
		// is public because context save function need to get this
		grip		gData;

		//	used by sort routines in graphmgr
		uint		first;
		uint		last;
		uint		next;

//	protected:
		
		uint16	maxCount;
		uint		resizeCount;
		char		listDataEnd;
};

class SysList : public List
{
	public:

		SysList(uint16 max = 10, bool resize = FALSE);
		void * operator new(size_t size);
};

#endif


/*

//----[]-------------------------------------------------------------
//
// A List is made up of nodes, each of which contains a
//	pointer to the node data, an index of the node, indeces
// of the next and previous nodes, and a key value.  Note that
// the next and prev properties are uint16 indeces, not pointers.
// This facilitates direct access instead of pointer dereferencing.
//
// Nodes have a physical position, described by the index and
// relative to the beginning of the allocated space, and a
// logical position, relative to the beginning of the chain.
//
//				+--------------+
//				| index = 1		|
//			 B	|  next = 2		|
//				|	prev = 3		|
//				+--------------+
//				| index = 2		|
//			 C	|	next = 4		|
//				|	prev = 1		|
//				+--------------+
//				| index = 3		|
//			 A	|	next = 1		|
//				|	prev = 0		|
//				+--------------+
//				| index = 4		|
//			 D	|	next = 0		|
//				|	prev = 2		|
//				+--------------+
//
// The GetNode function always returns the logical node. Given a
// 1-relative index of 2 this function would return a node pointer to "B".
//
// The mNodeAt macro always returns the physical node.  Given a
// 1-relative index of 3 this macro would return a node pointer to "A".
//
// New nodes are always added to the physical end of the list, and to
// the logical end as well unless another position is specified.  When
// a node is deleted, the (physically) last node in the list is moved
// into the newly-created hole, thereby maintaining a packed list.
//
// Note that Lists should contain grips on object rather than pointers
// to objects, to prevent movable allocations from becoming "stranded".
//
//----[]-------------------------------------------------------------
*/

