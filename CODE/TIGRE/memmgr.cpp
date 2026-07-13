//
// MEMMGR.CPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All Rights Reserved.
//
//----[]-------------------------------------------------------------

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "memmgr.hpp"
#include "mono.hpp"
#include "savemgr.hpp"

//#define NDEBUG

bool	MemMgrSave(uint16 state, nlohmann::json& root);

// since the new operator depends on the memory
//	manager it can't be dynamically allocated

MemoryMgr	MemMgr;


MemoryMgr* pMemMgr = nullptr;

// Constructor
MemoryMgr::MemoryMgr()
{
	// do platform specific memory initialization
	OS_InitMem();

	// init array
	for (uint16 i = 0; i < GRIPS_MAX; i++)
	{
		gripDataArray[i].ptr = nullptr;
		gripDataArray[i].type = EMPTY_GRIP;
		gripDataArray[i].generation = 0;
		gripDataArray[i].size = 0;
	}

	lastAllocGrip = grip{1, 0};
	lastAllocPtr = nullptr;
	lastSysGripFound = FIRST_SYSTEM_GRIP;

	// setup exceptional ptr to memmgr
	if (!pMemMgr)
	{
		pMemMgr = this;
		// setup for save game
		AtSave(MemMgrSave);
	}
	pObjMgr = &objMgr;
}



// Destructor
MemoryMgr::~MemoryMgr()
{
	pMemMgr = nullptr;
}



// attempt to detect memory corruption, return fragmentation factor
int
MemoryMgr::HeapCheck ()
{
	return (AvailMem() / LargestAlloc());
}



// basic alloc of memory, return a grip to allocation
grip
MemoryMgr::Malloc(size_t size, bool retError)
{
	grip g = FindEmptyGrip(lastAllocGrip.index);

	void* pv = MallocAtGrip(size, g, retError);
	if (!pv)
		return grip{};

	gripDataArray[g.index].type |= ALLOC_GRIP;
	gripDataArray[g.index].size = size;

	return g;
}


// basic alloc of memory into the system grip range, return a grip to allocation
grip
MemoryMgr::SysMalloc(size_t size)
{
	grip g = FindEmptySysGrip();

	MallocAtGrip(size, g);

	gripDataArray[g.index].type |= ALLOC_GRIP;
	gripDataArray[g.index].size = size;

	return g;
}



// Reallocate a grip's allocation to a new size
grip
MemoryMgr::Realloc(grip g, size_t size)
{
	ASSERT (gripDataArray[g.index].type & ALLOC_GRIP);

	void* pv = Deref(g);
	char* raw = (char*)pv - GRIP_HEADER_SIZE;

	raw = (char*)realloc(raw, GRIP_HEADER_SIZE + size);
	if (!raw)
	{
		AShutDownVideo();
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Realloc: out of memory (%zu bytes)", size);
		exit(1);
	}

	// Update grip header in case the pointer moved
	memcpy(raw, &g, sizeof(grip));

	pv = raw + GRIP_HEADER_SIZE;
	gripDataArray[g.index].ptr = pv;
	gripDataArray[g.index].size = size;

	return g;
}



// Allocate raw memory and install it at the given grip slot.
// The allocation begins with GRIP_HEADER_SIZE bytes reserved for the grip,
// followed by the user data; the returned pointer (stored in gripDataArray) is
// the start of user data.
void*
MemoryMgr::MallocAtGrip(size_t size, grip g, bool retError)
{
	char* raw = (char*)malloc(GRIP_HEADER_SIZE + size);

	lastAllocPtr = nullptr;
	lastAllocGrip = g;

	if (!raw)
	{
		if (retError)
			return nullptr;

		AShutDownVideo();
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Out of Memory: %zu bytes requested", size);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, " %zu total free", AvailMem());
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, " %zu largest block", LargestAlloc());
		Dump(30000, "Out Of Memory", true);
		pResMgr->Dump(30000, "Out Of Memory", true);
		exit(1);
	}

	// Store the grip in the prefix header so GetGrip(p) can recover it without a hash table
	memcpy(raw, &g, sizeof(grip));
	// Zero out remainder of header padding
	if (GRIP_HEADER_SIZE > sizeof(grip))
		memset(raw + sizeof(grip), 0, GRIP_HEADER_SIZE - sizeof(grip));
	// Zero-initialize user data
	memset(raw + GRIP_HEADER_SIZE, 0, size);

	void* pv = raw + GRIP_HEADER_SIZE;
	lastAllocPtr = pv;

	gripDataArray[g.index].ptr = pv;
	gripDataArray[g.index].type |= ALLOC_GRIP;
	gripDataArray[g.index].size = size;

	return pv;
}


// remove a grip and de-alloc memory
void
MemoryMgr::Free(grip g)
{
	if (g.index >= GRIPS_MAX)
	{
		char szPanic[40];
		snprintf(szPanic, sizeof(szPanic), "MemMgr::Free(g%u) >= GRIPS_MAX!\n", (unsigned)g.index);
		APanic(szPanic);
		return;
	}

	void* pv = Deref(g);
	free((char*)pv - GRIP_HEADER_SIZE);
	MakeEmptyGrip(g);
}



// return the pointer of a grip with error checking
void*
MemoryMgr::DerefDebug(grip g, int line, const char *pFile)
{
	if (g.index == 0)
	{
		char szPanic[100];
		snprintf(szPanic, sizeof(szPanic), "MemMgr::DerefDebug - null grip! %s line %d\n", pFile, line);
		APanic(szPanic);
		return nullptr;
	}

	if (g.index >= GRIPS_MAX)
	{
		char szPanic[256];
		snprintf(szPanic, sizeof(szPanic), "MemMgr::DerefDebug(g%u) index >= GRIPS_MAX(%d)! %s line %d\n",
			(unsigned)g.index, GRIPS_MAX, pFile, line);
		APanic(szPanic);
		return nullptr;
	}

	if (g.generation != gripDataArray[g.index].generation)
	{
		char szPanic[120];
		snprintf(szPanic, sizeof(szPanic),
			"MemMgr::DerefDebug(g%u) stale grip! handle gen=%u, slot gen=%u %s line %d\n",
			(unsigned)g.index, (unsigned)g.generation,
			(unsigned)gripDataArray[g.index].generation, pFile, line);
		APanic(szPanic);
		return nullptr;
	}

	void* p = gripDataArray[g.index].ptr;
	if (p == nullptr)
	{
		char szPanic[100];
		snprintf(szPanic, sizeof(szPanic), "MemMgr::DerefDebug(g%u) - null pointer! %s line %d\n",
			(unsigned)g.index, pFile, line);
		APanic(szPanic);
	}

	return p;
}

// return the pointer of a grip with error checking
void*
MemoryMgr::Deref(grip g)
{
	if (g.index == 0)
	{
		APanic("MemMgr::Deref - null grip!");
		return nullptr;
	}

	if (g.index >= GRIPS_MAX)
	{
		char szPanic[80];
		snprintf(szPanic, sizeof(szPanic), "MemMgr::Deref(g%u) index >= GRIPS_MAX(%d)\n",
			(unsigned)g.index, GRIPS_MAX);
		APanic(szPanic);
		return nullptr;
	}

	if (g.generation != gripDataArray[g.index].generation)
	{
		char szPanic[100];
		snprintf(szPanic, sizeof(szPanic),
			"MemMgr::Deref(g%u) stale grip! handle gen=%u, slot gen=%u\n",
			(unsigned)g.index, (unsigned)g.generation,
			(unsigned)gripDataArray[g.index].generation);
		APanic(szPanic);
		return nullptr;
	}

	void* p = gripDataArray[g.index].ptr;
	if (p == nullptr)
	{
		char szPanic[80];
		snprintf(szPanic, sizeof(szPanic), "MemMgr::Deref(g%u) - null pointer!", (unsigned)g.index);
		APanic(szPanic);
	}

	return p;
}


// Return the size of the grip
size_t
MemoryMgr::SizeOfGrip(grip g)
{
	if (gripDataArray[g.index].type & ALLOC_GRIP)
		return gripDataArray[g.index].size;
	return 0;
}


// Recover the grip that owns pointer p by reading the grip header stored
// GRIP_HEADER_SIZE bytes before p.  Validates the recovered grip before
// returning it; returns INVALID_GRIP for pointers not from MallocAtGrip.
grip
MemoryMgr::GetGrip(void* p)
{
	if (p == nullptr)
	{
		APanic("Getting grip of null pointer");
	}

	grip g;
	memcpy(&g, (char*)p - GRIP_HEADER_SIZE, sizeof(grip));

	// Validate: index in range, slot non-empty, ptr matches, generation matches
	if (g.index == 0 || g.index >= GRIPS_MAX)
		return INVALID_GRIP;
	if (gripDataArray[g.index].ptr != p)
		return INVALID_GRIP;
	if (gripDataArray[g.index].generation != g.generation)
		return INVALID_GRIP;

	return g;
}


//---------------------------------------------------------------
// Protected:
//---------------------------------------------------------------

// Find the first empty standard grip, starting from 'start'
grip
MemoryMgr::FindEmptyGrip(uint16 start)
{
	if (start < FIRST_STANDARD_GRIP)
		start = FIRST_STANDARD_GRIP;

	// forward pass from start
	for (uint16 i = start; i < STANDARD_GRIP_MAX; i++)
	{
		if (gripDataArray[i].ptr == nullptr)
			return grip{i, gripDataArray[i].generation};
	}

	// wrap-around pass
	for (uint16 i = FIRST_STANDARD_GRIP; i < start; i++)
	{
		if (gripDataArray[i].ptr == nullptr)
			return grip{i, gripDataArray[i].generation};
	}

	Dump(7, "FindEmptyGrip() failed!", true);
	APanic("Out of Grips");
	return grip{0, 0}; // unreachable
}


// Find the first empty system grip (slots 1..SYSTEM_GRIP_MAX-1)
grip
MemoryMgr::FindEmptySysGrip()
{
	for (uint16 i = lastSysGripFound; i < SYSTEM_GRIP_MAX; i++)
	{
		if (gripDataArray[i].ptr == nullptr)
		{
			lastSysGripFound = (uint16)(i + 1);
			return grip{i, gripDataArray[i].generation};
		}
	}

	for (uint16 i = FIRST_SYSTEM_GRIP; i < SYSTEM_GRIP_MAX; i++)
	{
		if (gripDataArray[i].ptr == nullptr)
		{
			lastSysGripFound = (uint16)(i + 1);
			return grip{i, gripDataArray[i].generation};
		}
	}

	APanic("Out of System Grips");
	return grip{0, 0}; // unreachable
}


// Mark a grip slot empty and advance its generation counter
void
MemoryMgr::MakeEmptyGrip(grip g)
{
	gripDataArray[g.index].ptr = nullptr;
	gripDataArray[g.index].type = EMPTY_GRIP;
	gripDataArray[g.index].size = 0;
	gripDataArray[g.index].generation++;
}


// Allocate a grip slot for an embedded TMovable (no malloc).
// Used when TMovable::TMovable() detects it is not the outermost object allocation.
grip
MemoryMgr::CreateEmbeddedGrip(void* p)
{
	grip g = FindEmptyGrip(lastAllocGrip.index);
	gripDataArray[g.index].ptr = p;
	gripDataArray[g.index].type = NON_ALLOC_GRIP;
	gripDataArray[g.index].size = 0;
	return g;
}


// Release a NON_ALLOC grip slot.  No-op for ALLOC grips (freed via operator delete).
void
MemoryMgr::ReleaseEmbeddedGrip(grip g)
{
	if (g.index && g.index < GRIPS_MAX &&
	    (gripDataArray[g.index].type & NON_ALLOC_GRIP))
	{
		MakeEmptyGrip(g);
	}
}


// Free all standard-range grips (scenario transition / restore)
void
MemoryMgr::ClearAllocations()
{
	for (uint16 i = FIRST_STANDARD_GRIP; i < STANDARD_GRIP_MAX; i++)
	{
		if (gripDataArray[i].type & ALLOC_GRIP)
		{
			Free(grip{i, gripDataArray[i].generation});
		}
	}
}


//	print out information of all grips, debugging only
void
MemoryMgr::Dump(uint16 dumpNum, const char *title, bool overWrite)
{
	#ifndef NDEBUG

	uint16	i;
	int	nGrips = 0;
	SDL_IOStream* pFile;
	char	dumpName[60];
	char	lineBuf[256];
	const char	*dumpTitle;

	snprintf(dumpName, sizeof(dumpName), "mem%05d.dmp", dumpNum);
	pFile = fOpenPrefs(dumpName, overWrite ? "w" : "a");
	if (!pFile)
	{
		APanic ("can't open memmgr.dmp");
	}
	ASSERT (pFile);

	dumpTitle = title ? title : "";

#define WLINE(fmt, ...) do { snprintf(lineBuf, sizeof(lineBuf), fmt, ##__VA_ARGS__); SDL_WriteIO(pFile, lineBuf, strlen(lineBuf)); } while(0)

	WLINE("\n\nMemory manager dump: %s\n", dumpTitle);

	WLINE("\n %zu Total Bytes Used", UsedMem());
	WLINE("\n %zu Total Bytes Free", AvailMem());
	WLINE("\n %zu Largest Block\n", LargestAlloc());

	for (i = 1; i < GRIPS_MAX; i++)
	{
		if (i == FIRST_SYSTEM_GRIP)
			WLINE("\n\n******** SYSTEM GRIPS ********\n");
		else if (i == FIRST_STANDARD_GRIP)
			WLINE("\n\n******** STANDARD GRIPS ********\n");

		if (gripDataArray[i].type & ALLOC_GRIP)
		{
			nGrips++;
			WLINE("\n grip: %u  Allocation %zu bytes, at %p",
				(unsigned)i,
				gripDataArray[i].size,
				gripDataArray[i].ptr);
		}
	}

	WLINE("\n\nEnd dump: %d unfreed grips\n\n", nGrips);
#undef WLINE
	SDL_CloseIO(pFile);

	#else
	dumpNum = dumpNum;
	title = title;
	overWrite = overWrite;
	#endif  // NDEBUG
}


//	print information about a single grip
void
MemoryMgr::DumpGrip(grip gSpec)
{
	#ifndef NDEBUG

	if (pMono)
	{
		if (gripDataArray[gSpec.index].type & ALLOC_GRIP)
		{
			pMono->Out("\n Allocation %zu bytes, at %p",
				gripDataArray[gSpec.index].size,
				gripDataArray[gSpec.index].ptr);
		}
		else
		{
			pMono->Out("\n Empty grip %u", (unsigned)gSpec.index);
		}
	}
	#else
	gSpec = gSpec;
	#endif  // NDEBUG
}


// Return the largest allocation possible
size_t
MemoryMgr::LargestAlloc()
{
	return ALargestAlloc();
}


// Return amount of memory available
size_t
MemoryMgr::AvailMem()
{
	return AAvailMem();
}


// Return amount of used memory
size_t
MemoryMgr::UsedMem()
{
	size_t memUsed = 0;

	for (uint16 i = 1; i < GRIPS_MAX; i++)
	{
		if (gripDataArray[i].type & ALLOC_GRIP)
			memUsed += gripDataArray[i].size;
	}

	return memUsed;
}


// debugging
int
MemoryMgr::CheckGrip(grip g)
{
	if (!g.index)              return GRIP_ZERO;
	if (g.index >= GRIPS_MAX)  return GRIP_OUT_OF_RANGE;

	if (gripDataArray[g.index].ptr == nullptr)  return GRIP_EMPTY;

	return GRIP_VALID;
}


bool
MemMgrSave(uint16 state, nlohmann::json& /*root*/)
{
	if (state == BEFORE_RESTORE) {
		pMemMgr->ClearAllocations();
	}
	return false;
}
