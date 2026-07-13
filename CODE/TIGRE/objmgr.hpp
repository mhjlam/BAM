//
// OBJMGR.HPP
//
// November 20, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	ObjectMgr class definition.
//
//		The ObjectMgr maintains a list of objects and handles
//		saving and restoring games.
//
//----[]-------------------------------------------------------------


#ifndef	objmgr_hpp
#define	objmgr_hpp


#include "types.hpp"


//	Base object for all objects
class	ObjectMgr 
{
	public:
		grip		gSelf;

		ObjectMgr(void);
		virtual	~ObjectMgr(void);

		void	SaveWorld(void);
		void	RestoreWorld(void);
};


extern ObjectMgr*	pObjMgr;


#endif

