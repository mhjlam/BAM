//
// MEMMGR.HPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// MemoryMgr class definition.
//
//----[]-------------------------------------------------------------


#ifndef	memmgr_hpp
#define	memmgr_hpp


#include <stddef.h>
#include <cstddef>

#include "tigre.hpp"
#include "objmgr.hpp"
#include "gripsmax.hpp"

constexpr grip INVALID_GRIP{};  // {index=0, generation=0}


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
#define	EMPTY_GRIP		0x0000
#define	ALLOC_GRIP		0x0001
#define	NON_ALLOC_GRIP	0x0002   // grip for embedded TMovable members (no malloc)

// Each allocation is prefixed with a grip (4 bytes), padded to max alignment
// so the returned pointer is always suitably aligned.
// GetGrip(p) reads p[-GRIP_HEADER_SIZE] to recover the grip without a hash table.
static constexpr size_t GRIP_HEADER_SIZE = alignof(std::max_align_t);
static_assert(GRIP_HEADER_SIZE >= sizeof(grip), "header too small for grip");


// Structure containing data about an allocated grip.
// Sized to a power of two (16 bytes) for cache performance.
struct GripData
{
	void*		ptr;
	grip_t	type;
	uint16	generation;  // incremented in MakeEmptyGrip; packed into grip on alloc
	size_t	size;        // size of the allocation
};


//---------[MemoryMgr]-------------------------------------------

class MemoryMgr
{
	public:
		MemoryMgr (void);
		virtual	~MemoryMgr (void);

		grip		Malloc(size_t size, bool retError = false);
		grip		SysMalloc(size_t size);
		void		Free(grip h);
		grip		Realloc(grip g, size_t size);

		void*		Deref(grip g), *DerefDebug(grip g, int line, const char *pFile);

		// Non-panicking check: true if grip is invalid/freed (safe to call on freed grips)
		inline bool IsGripFree(grip g) const {
			return !g.index || g.index >= GRIPS_MAX || gripDataArray[g.index].ptr == nullptr;
		}

		// True if grip index/generation match a live slot (generation-aware, non-panicking)
		inline bool IsGripValid(grip g) const {
			return g.index > 0 && g.index < GRIPS_MAX &&
			       gripDataArray[g.index].ptr != nullptr &&
			       gripDataArray[g.index].generation == g.generation;
		}

		// Return the grip most recently allocated (used by TMovable constructor for gSelf)
		inline grip LastAllocGrip() const { return lastAllocGrip; }

		// Allocate a NON_ALLOC grip slot pointing at an embedded TMovable (no malloc).
		grip		CreateEmbeddedGrip(void* p);
		// Release a NON_ALLOC grip slot (no-op for ALLOC grips).
		void		ReleaseEmbeddedGrip(grip g);

		grip		GetGrip(void* p);

		size_t	SizeOfGrip(grip g);

		int		HeapCheck (void);
		void		Dump(uint16 dumpNum = 1, const char *title = nullptr, bool overWrite = false);
		void		DumpGrip(grip g);
		size_t	LargestAlloc(void);
		size_t	AvailMem(void);
		size_t	UsedMem(void);

		void		ClearAllocations(void);

		ObjectMgr	objMgr;

		// debugging
		int		CheckGrip(grip g);

	protected:

		void*		MallocAtGrip(size_t size, grip g, bool retError = false);

		grip		FindEmptyGrip(uint16 start = FIRST_STANDARD_GRIP);
		grip		FindEmptySysGrip(void);
		void		MakeEmptyGrip(grip);

		// array of pointers, indexed by grip
		GripData	gripDataArray [GRIPS_MAX];
		void		*lastAllocPtr;
		grip		lastAllocGrip;
		uint16	lastSysGripFound;
};


// since the new operator depends on the memory
//	manager it can't be dynamically allocated

extern	MemoryMgr	MemMgr;
extern 	MemoryMgr*	pMemMgr;


#endif	// memmgr_h
