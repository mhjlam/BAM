//
// OBJMGR.CPP
//
// November 20, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All Rights Reserved.
//
//----[]-------------------------------------------------------------


#include "objmgr.hpp"


void	exit_DestroyManagers(void);

ObjectMgr*	pObjMgr = NULL;

// Constructor 
ObjectMgr::ObjectMgr(void)
{
	// setup exceptional ptr to ObjectMgr
	if (!pObjMgr)
	{
		pObjMgr = this;
	}
}


// Destructor 
ObjectMgr::~ObjectMgr(void)
{
 	pObjMgr = NULL;
}




// 
void
ObjectMgr::SaveWorld(void)
{
	// free up resources
//	if (AResMgr())
//	{
//		delete pResMgr;
//	}

//	pMemMgr->WriteAllocations("tigre.sav");

	// restore resource manager
//	new ResourceMgr;
}


// 
void
ObjectMgr::RestoreWorld(void)
{
	// free up resources
//	if (AResMgr())
//	{
//		delete pResMgr;
//	}

//	pMemMgr->ReadAllocations("tigre.sav");

	// restore resource manager
//	new ResourceMgr;
}




