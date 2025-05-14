//
// KEYLIST.HPP
//
// November 23, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// KeyList class definition.
//
//		The KeyList is a List that allows manipulations by key value.
//
//----[]-------------------------------------------------------------


#ifndef	keylist_hpp
#define	keylist_hpp

#include "types.hpp"
#include "list.hpp"


enum 
{
	SORT_DESCENDING,
	SORT_ASCENDING
};


class KeyList : public List
{
	public:

		KeyList(uint16 max = 10, bool resize = FALSE);

		virtual	node* 	FirstKey(int32 key);
		virtual	node* 	LastKey(int32 key);

		virtual	node* 	NextKey(node* pNode, int32 key);
		virtual	node*		NextKey(void* id, int32 key);

		virtual	node* 	PrevKey(node* pNode, int32 key);
		virtual	node*		PrevKey(void* id, int32 key);

		virtual	uint16 	DeleteKey(int32 key);
		virtual	uint16 	CountKey(int32 key);

		virtual	void		SortByKey(int sortDirection);
		virtual	bool		Save(uint16 state, FILE *pFile = NULL);

		char		keyListDataStart;
		bool		fNeedsResort;
		char		keyListDataEnd;
};


#endif

