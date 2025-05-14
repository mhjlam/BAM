//
// MEMMGR.HPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// MemoryMgr class definition.
//
// 	Minimal system to implement agreed upon API, built on top of
//		malloc().  Also does basic error checking. (jc)
//
//----[]-------------------------------------------------------------


#ifndef	memmgr_hpp
#define	memmgr_hpp


#include <stddef.h>

#include "tigre.hpp"
#include "objmgr.hpp"
#include "gripsmax.hpp"

const		grip	INVALID_GRIP = 0;


/*	not yet implemented
extern grip		ADupGrip (grip g);
extern size_t	ALargestAlloc (void);
extern bool		AHeapScramble (void);
*/


// for debugging with CheckGrip
enum 
{
	GRIP_ZERO,
	GRIP_OUT_OF_RANGE,
	GRIP_EMPTY,
	GRIP_VALID
};




//---------[Types internal to MemoryMgr]----------------------

typedef uint16	grip_t;

// grip bit flags

#define	EMPTY_GRIP					0x0000
#define	ALLOC_GRIP					0x0001
#define	NON_ALLOC_GRIP				0x0002
#define	NO_SAVE_GRIP				0x0004	// don't save during save game
#define	NO_PURGE_GRIP				0x0008	// don't purge during restore game
#define	TMOV_GRIP					0x0010

// Structure holding Grip data associated with a ptr and not
// a distinct allocation

struct NonAllocGripData
{
	uint16	offset;
	grip		gOwner;	// if this grip points into the space of
							// a different grip, hold the owner grip here
};



// Structure holding Grip data associated with a distinct allocation

struct AllocGripData
{
	size_t	size;
	void*		endPtr;			// precomputed one byte past end of allocation.
};


// structure containing data about Alloc-ed or Non-Alloc-ed grips.
// To improve performance this struct is sized to a power of two (16).
struct GripData 
{
	void*		ptr;
	grip_t	type;
	uint16	hashIndex;
	union {
		AllocGripData		alloc;
		NonAllocGripData	nonalloc;
	} data;
};


struct HashData
{
	grip	gNum;
	void	*ptr;
};


//---------[MemoryMgr]-------------------------------------------


//	MemoryMgr class declaration, currently inherits from nothing
//
class MemoryMgr 
{
	public:
		MemoryMgr (void);
		virtual	~MemoryMgr (void);

		grip		Malloc(size_t size,bool retError=FALSE);
		grip		SysMalloc(size_t size);
		void		Free(grip h);
		grip		Realloc(grip g, size_t size);
		
		grip		AssignGrip(void* p, bool searchArray = TRUE, grip_t flags = NULL);
		void		ReleaseGrip(grip g);

		bool		ChangeGrip(grip oldGrip, grip newGrip);

		void*		Deref(grip g), *DerefDebug(grip g, int line, char *pFile);

		// Same as MemoryMgr::Deref, w/o error checks
		//inline void*	FastDeref(grip g) { return gripDataArray[g].ptr; }

		grip		ForceGrip(grip g, void* dg);

		void		Cycle(void);

		grip		GetGrip(void* p);

		size_t	SizeOfGrip(grip g);

		int		HeapCheck (void);
		void		Dump(uint16 dumpNum = 1, char *title = NULL, bool overWrite = FALSE);
		void		DumpGrip(grip g);
		size_t	LargestAlloc(void);
		size_t	AvailMem(void);
		size_t	UsedMem(void);

		bool		WriteAllocations(FILE *fp);
		bool		ReadAllocations(FILE *fp);
		void		ClearAllocations(void);

		ObjectMgr	objMgr;

		void		ShowGripNames(uint16 start);

		void		SetSaveStatus(grip g, bool saveIt);
		void		SetPurgeStatus(grip g, bool purgeIt);
		bool		SaveGrip(grip gNum, FILE *fp);
		bool		SaveGripNum(grip gNum, FILE *fp);
		bool		RestoreGrip(grip gNum, FILE *fp);
		bool		RestoreGripNum(grip *gNum, FILE *fp);
		bool		RestoreGripOffsets(void);

		// debugging
		int		CheckGrip(grip g);

	protected:

		void*		MallocAtGrip(size_t size, grip g,bool retError=FALSE);

		grip		FindEmptyGrip(grip start = FIRST_STANDARD_GRIP);
		grip		FindEmptySysGrip(void);
		void		MakeEmptyGrip(grip);
		bool		PackMem(void);

		void		SetHashTable(void *pv, grip gNum);

		grip		FindOwnerOf(void* p, ptrdiff_t* pOffset);

		// bit array marking all allocs that have been deref-ed
		int32		inUseBitArray [ (GRIPS_MAX/8)+1 ];	

		// array of pointers, indexed by grip
		GripData	gripDataArray [GRIPS_MAX];
		void		*lastAllocPtr;
		grip		lastAllocGrip;
		grip		lastSysGripFound;

		// hash table for quick lookup of grips given an address
		HashData	gripHashArray[GRIPS_MAX];
};


// since the new operator depends on the memory 
//	manager it can-t be dynamically allocated

extern	MemoryMgr	MemMgr;
extern 	MemoryMgr*	pMemMgr;


#endif	// memmgr_h

