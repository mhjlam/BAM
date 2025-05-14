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


// Memory tracking
extern int		lineTrack;
extern char*	szFileTrack;
extern char*	szStrTrack;

#ifdef NDEBUG
	#define	TRACK_MEM(s)
#else
	#define	TRACK_MEM(s)	lineTrack   = __LINE__; \
									szFileTrack = __FILE__;	\
									szStrTrack	= s;
#endif

#define	INT_DPMI		0x31		// DPMI Interrupt

struct DPMIMemInfo
{
	uint32	largestBlock;
	uint32	maxUnlockedPage;
	uint32	maxLockedPage;
	uint32	totalLinearAddress;
	uint32	totalUnlockedPages;
	uint32	freePages;
	uint32	totalPhysicalPages;
	uint32	freeLinearAddr;
	uint32	sizePagingFilePartition;
	char		reserved[12];
};


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

bool		ASaveGripNum(grip gNum, FILE *fp);
bool		ARestoreGripNum(grip *gNum, FILE *fp);
bool		AChangeGrip(grip oldGrip, grip newGrip);
grip		AGetGrip(void* p);
size_t	ASizeOfGrip(grip g);
grip		AAssignGrip(void* p, bool searchArray = TRUE, grip_t flags = NULL);
void		AReleaseGrip(grip g);

void		APrintUnfreedPtrs(bool printThem);

void		ASetSaveStatus(grip g, bool saveIt);
void		ASetPurgeStatus(grip g, bool purgeIt);

void		AMemCycle();


// Delete an instance of Object referenced by grip (note that Object::~Object() 
//		is virtual so any subclass will work also)
#define	ADelete(g)		(delete (Object *)(pMemMgr->Deref(g)))


#endif


