//
// APIMEM.CPP
//
// November 12, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// API calls specific to the memory manager and memory functions
//
//----[]-------------------------------------------------------------


#include "apimem.hpp"
#include "apigraph.hpp"		// GetSP()
#include "mem.hpp"
#include "types.hpp"


// Signal whether the manager has been created
bool
AMemMgr()
{
	return pMemMgr ? true : false;
}


grip
AMalloc(size_t size)
{
	return pMemMgr->Malloc(size);
}


grip
ASysMalloc(size_t size)
{
	return pMemMgr->SysMalloc(size);
}


grip
ARealloc(grip g, size_t size)
{
	return pMemMgr->Realloc(g, size);
}


void
AFree(grip g)
{
	pMemMgr->Free(g);
}


grip
AGetGrip(void* p)
{
	return pMemMgr->GetGrip(p);
}


size_t
ASizeOfGrip(grip g)
{
	return pMemMgr->SizeOfGrip(g);
}


void
APrintUnfreedPtrs(bool /*printThem*/)
{
	// no-op: mem debug package removed
}
