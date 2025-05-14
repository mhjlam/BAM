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

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

// Memory tracking
int		lineTrack;
char*		szFileTrack;
char*		szStrTrack;


// Signal whether the manager has been created
bool
AMemMgr()
{
	return pMemMgr ? TRUE : FALSE;
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


// void*
// ADeref(grip g)
// {
// 	return pMemMgr->Deref(g);
// }


bool
ASaveGripNum(grip gNum, FILE *fp)
{
	return pMemMgr->SaveGripNum(gNum, fp);
}


bool
ARestoreGripNum(grip *gNum, FILE *fp)
{
	return pMemMgr->RestoreGripNum(gNum, fp);
}


bool
AChangeGrip(grip oldGrip, grip newGrip)
{
	return pMemMgr->ChangeGrip(oldGrip, newGrip);
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


grip
AAssignGrip(void* p, bool searchArray, grip_t flags)
{
	return pMemMgr->AssignGrip(p, searchArray, flags);
}


void
AReleaseGrip(grip g)
{
	pMemMgr->ReleaseGrip(g);
}

// set the global variable that determines if the unfreed pointers
// should be displayed when the program is exiting.
void
APrintUnfreedPtrs(bool printThem)
{
	fPrintUnfreedPtrs = printThem;
}

void
ASetSaveStatus(grip g, bool saveIt)
{
	pMemMgr->SetSaveStatus(g, saveIt);
}


void
ASetPurgeStatus(grip g, bool purgeIt)
{
	pMemMgr->SetPurgeStatus(g, purgeIt);
}


void
AMemCycle()
{
	pMemMgr->Cycle();
}


