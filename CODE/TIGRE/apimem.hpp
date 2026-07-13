//
// APIMEM.HPP
//
// November 12, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// API calls specific to the memory manager and memory functions.
//
//----[]-------------------------------------------------------------


#ifndef	apimem_hpp
#define	apimem_hpp


#include "memmgr.hpp"
#include "types.hpp"


bool		AMemMgr();

grip		AMalloc(size_t size);
grip		ASysMalloc(size_t size);
grip		ARealloc(grip g, size_t size);
void		AFree(grip g);

bool		AIsOnStack(void* p);
size_t	ALargestAlloc();
size_t	AAvailMem();
#define	ADeref(g)		(pMemMgr->DerefDebug(g, __LINE__, __FILE__))
#define	ADerefAs(t, g)	((t*) ADeref(g))

grip		AGetGrip(void* p);
size_t	ASizeOfGrip(grip g);

void		APrintUnfreedPtrs(bool printThem);


// Delete an instance of Object referenced by grip (note that Object::~Object()
//		is virtual so any subclass will work also)
#define	ADelete(g)		(delete (Object *)(pMemMgr->Deref(g)))


#endif
