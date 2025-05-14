//
// MEMMGR.CPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All Rights Reserved.
//
// Notes:
//
// - Hashing is used in two ways to speed up performance.  The first
//   hash is used to determine where to start looking for an empty grip
//   in the grip array.  This is much quicker than starting at one place.
//   The second hash uses the pointer address to determine where it is
//   in the array.  This allows quick lookup in GetGrip.  If it wasn't
//   for possible memory shuffling, or restoring game changing the 
//   pointers, this would be unneeded.  We could instead use the grip
//   array for this purpose.  Please note that the second hash did
//   not help FindOwnerOf.  For some reason, the best way to do that
//   is the old fashioned way.  The profiler doesn't lie.
// 
//----[]-------------------------------------------------------------

#include <limits.h>
#include <string.h>

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "memmgr.hpp"
#include "mono.hpp"
#include	"savemgr.hpp"

//#define NDEBUG

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

// Because save game needs to restore to the save grip numbers,
// we have two grip ranges.  One for systems grips and another
// for all other grips.  Systems grips are ones that are not saved
// or restored.  Examples of these are sound drivers and resouce
// map files.  System grips start at 1 and go to SYSTEM_GRIP_MAX.
// All other grips start at SYSTEM_GRIP_MAX and continue to GRIPS_MAX. 
// To use a systems grip call SysMalloc instead of Malloc.

// include Symantec specific MEM package and redefined new operator
#include "mem.hpp"

// since the new operator depends on the memory 
//	manager it can't be dynamically allocated

MemoryMgr	MemMgr;

bool	MemMgrSave(uint16 state, FILE *fp);

#define	MEM_GRIP_HASH(ptr)	((((uint) ptr & 0xffff) % (GRIPS_MAX-1))+1)
#define	MEM_HASH_HASH(ptr)	((((uint) ptr) % (GRIPS_MAX-1))+1)

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//		MemoryMgr class
//---------------------------------------------------------------------
//---------------------------------------------------------------------


MemoryMgr* pMemMgr = NULL;

// Constructor
MemoryMgr::MemoryMgr()
{
	grip			i;

	// do platform specific memory initialization
	OS_InitMem();

	// initialize Symantec's MEM package
	mem_init();

	// init array as NULLs 
	for (i = 0; i < GRIPS_MAX; i++)
	{
		gripDataArray[i].ptr = NULL; 
		gripDataArray[i].type = EMPTY_GRIP; 
		gripDataArray[i].hashIndex = 0;
	}

	// init the hash array
	memset(gripHashArray, 0, sizeof(gripHashArray));

	lastAllocGrip = 1;
	lastAllocPtr = 0;

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
	// terminate Symantec's MEM package
	mem_term();

	pMemMgr = NULL;
}



// attempt to detect memory corruption, return fragmentation factor
int
MemoryMgr::HeapCheck ()
{
	// call Symantec's MEM package
	mem_check();

	return (AvailMem() / LargestAlloc());
}



// basic alloc of memory, return a grip to allocation
grip
MemoryMgr::Malloc(size_t size,bool retError)
{
	void*		pv = NULL;
	uint32	TOO_LARGE;
	grip		g = 0;

	TOO_LARGE = 8*1024*1024;
	if (size > TOO_LARGE) 
	{
		if(retError)
			return g;
		else
			APanic ("unreasonably large alloc");
	}

	// find first empty grip
//	g = FindEmptyGrip(MEM_GRIP_HASH(ATicks()));
	g = FindEmptyGrip(MEM_GRIP_HASH(szStrTrack));

	pv = MallocAtGrip(size, g, retError);

	if(pv)
	{
		gripDataArray[g].type |= ALLOC_GRIP;
		gripDataArray[g].data.alloc.size = size;
		gripDataArray[g].data.alloc.endPtr = (char *) gripDataArray[g].ptr + size;
	}
	else
	{
		return 0;
	}

	return g;
}


// basic alloc of memory, return a grip to allocation
// (This is almost a copy of Malloc.  This was done for speed.)
grip
MemoryMgr::SysMalloc(size_t size)
{
	void*		pv = NULL;
	uint32	TOO_LARGE;
	grip		g;

	TOO_LARGE = 8*1024*1024;
	if (size > TOO_LARGE) 
	{
		APanic ("unreasonably large alloc");
	}

	// find first empty grip
	g = FindEmptySysGrip();

	MallocAtGrip(size, g);

	gripDataArray[g].type |= ALLOC_GRIP;
	gripDataArray[g].data.alloc.size = size;
	gripDataArray[g].data.alloc.endPtr = (char *) gripDataArray[g].ptr + size;

	return g;
}



// basic alloc of memory, return a grip to allocation
grip
MemoryMgr::Realloc(grip g, size_t size)
{
	void*		pv = NULL;
	uint		hashIndex;

	// must be an alloc'ed grip to realloc
	ASSERT (gripDataArray[g].type & ALLOC_GRIP);

	pv = Deref(g);
	pv = mem_realloc(pv, size);

	// update internals
	gripDataArray[g].data.alloc.size = size;
	gripDataArray[g].ptr = pv;
	gripDataArray[g].data.alloc.endPtr = (char *) pv + size;

	hashIndex = gripDataArray[g].hashIndex;
	gripHashArray[hashIndex].gNum = 0;	
	gripHashArray[hashIndex].ptr = 0;	
	SetHashTable(pv, g);

	return g;
}



// basic alloc of memory, return a grip to allocation
void*
MemoryMgr::MallocAtGrip(size_t size, grip g,bool retError)
{
	void*		pv = NULL;
	char 		buffer[100];

	do 
	{
		if (size < 16)
		{
			// debugger break space
			int i = 0;
			i++;
		}

		// currently using the Symantec MEM package version of C stdlib malloc()
		if (!szStrTrack)
		{
			sprintf(buffer, "Grip: %d", g);
			TRACK_MEM(buffer);
		}
		lastAllocPtr =	pv = mem_malloc(size);
		lastAllocGrip = g;

		// if error message for unfreed ptr indicate next line, the 
		// allocation that caused the error was not tracked.
		if (pv == NULL) 
		{
			if (!PackMem()) 
			{
				if (!APurge(size / 1024 + 1))
				{
					//first we must try one last time after the purge
					//because we may have purged the amount we really needed
					//but not as much as we ask for.
					//NOTE: Purge deletes OLDEST first not contigious blocks
					lastAllocPtr =	pv = mem_malloc(size);
					lastAllocGrip = g;

					if(pv)
						break;

					if(retError)
					  return pv;	

					AShutDownVideo();

					printf("\nOut of Memory, \n\t%d bytes", (int) size);
					printf("\n %d total free", (int) AvailMem());
					printf("\n %d largest block", (int) LargestAlloc());
					printf("\n %allocated from %s, line %d -> %s", szFileTrack, lineTrack, szStrTrack);

					sprintf(buffer, "Out Of Memory Error Dump (%d requested)", (int) size);
					Dump(30000, buffer, TRUE);

					pResMgr->Dump(30000, buffer, TRUE);
					APrintUnfreedPtrs(FALSE);
					exit(1);
				}
			}
		}
	} while (pv == NULL);

	// fill in that grip's data 
	gripDataArray[g].ptr = pv;
	gripDataArray[g].type |= ALLOC_GRIP;
	gripDataArray[g].data.alloc.size = size;
	gripDataArray[g].data.alloc.endPtr = (char *) gripDataArray[g].ptr + size;

	SetHashTable(pv, g);

	return pv;
}

// set the hash table value
void
MemoryMgr::SetHashTable(void *pv, grip gNum)
{
	// save the grip number in the hash array
//	uint hashIndex = ((uint) pv & 0xffff) % GRIPS_MAX;
	uint 	hashIndex = MEM_HASH_HASH(pv);
	uint	gHash;

	while(TRUE)
	{
		gHash = gripHashArray[hashIndex].gNum;

		if (gHash && gHash != gNum)
		{
			// index Already used
			hashIndex++;
			if (hashIndex >= GRIPS_MAX)
			{
			hashIndex = 1;
			}
		}
		else
		{
		  gripHashArray[hashIndex].gNum = gNum;	
		  gripHashArray[hashIndex].ptr = pv;	
		  gripDataArray[gNum].hashIndex = (uint16) hashIndex; 
		  break;
		}
	}
}

// remove a grip and de-alloc memory
void
MemoryMgr::Free(grip g)
{
	void*	pv;

	// debug version error checking for valid grip
	if(g >= GRIPS_MAX)
	{
		char	szPanic[40];

		sprintf(szPanic, "MemMgr::Free(g%d) >= GRIPS_MAX!\n", g);
		APanic(szPanic);
		return;
	}

	// get "master pointer"
	pv = Deref(g);

	// currently using the Symantec MEM package version of C stdlib free()
	mem_free(pv);

	MakeEmptyGrip(g);
}



// return the pointer of a grip, mark the grip as "in use",
// with some error checking
void*
MemoryMgr::DerefDebug(grip g, int line, char *pFile)
{
	void	*p;

	// debug version error checking for valid grip
	if(g >= GRIPS_MAX)
	{
		char	szPanic[80];

		sprintf(szPanic, "MemMgr::DerefDebug(g%d) g >= GRIPS_MAX(%d)! %s line %d\n",
			g, GRIPS_MAX, pFile, line);
		APanic(szPanic);
		return(NULL);
	}

	p = gripDataArray[g].ptr;

	if (p == NULL)
	{
		char	szPanic[80];
		if (g == NULL)
		{
			// gripDataArray[0].ptr is alway 0, so error check this only if p == 0
			sprintf(szPanic, "MemMgr::DerefDebug(g%d) - null grip! %s line %d\n",
				g, pFile, line);
			APanic(szPanic);
		}
		else
		{
			sprintf(szPanic, "MemMgr::DerefDebug(g%d) - null pointer! %s line %d\n",
				g, pFile, line);
			APanic(szPanic);
		}
	}

	return p;
}

// return the pointer of a grip, mark the grip as "in use",
// with some error checking
void*
MemoryMgr::Deref(grip g)
{
	void*			p;

	// debug version error checking for valid grip
	if(g >= GRIPS_MAX)
	{
		char	szPanic[80];

		sprintf(szPanic, "MemMgr::Deref(g%d) g >= GRIPS_MAX(%d)\n", g, GRIPS_MAX);
		APanic(szPanic);
		return(NULL);
	}

	p = gripDataArray[g].ptr;

	if (p == NULL)
	{
		char	szPanic[80];
		if (g == NULL)
		{
			// gripDataArray[0].ptr is alway 0, so error check this only if p == 0
			sprintf(szPanic, "MemMgr::Deref(g%d) - null grip!", g);
			APanic(szPanic);
		}
		else
		{
			sprintf(szPanic, "MemMgr::Deref(g%d) - null pointer!", g); 
			APanic(szPanic);
		}
	}

	return p;
}


// assigns a grip to already existing allocation
grip
MemoryMgr::AssignGrip(void* p, bool searchArray, grip_t flags)
{
	grip			g;
	grip			gOwner;
	ptrdiff_t	offset;
	grip			start;

	if (lastAllocPtr == p)
	{
		// if the last alloc was p, we already know the grip.
		g = lastAllocGrip;
	}
	else
	{
		if (searchArray)
		{
			g = GetGrip(p);
		}
		else
		{
			g = INVALID_GRIP;
		}
	}


	if (g == INVALID_GRIP) 
	{
		// Hash the pointer to get a relatively random spread across
		// our grip array
		start = MEM_GRIP_HASH(p);

		// no grip already allocated to this pointer, get first empty
		g = FindEmptyGrip(start);

		// fill in that grip's data 
		GripData		*pGripData;

		pGripData = &gripDataArray[g];
		pGripData->ptr = p;
		pGripData->type |= NON_ALLOC_GRIP;

		// any additional flags they want set?
		if(flags)
			pGripData->type |= flags;


		// find owning grip and offset
		gOwner = FindOwnerOf(p, &offset);
		// verify that it will fit into dataArray struct
		ASSERT(offset < USHRT_MAX);

		pGripData->data.nonalloc.gOwner = gOwner;
		pGripData->data.nonalloc.offset = (uint16) offset;

		SetHashTable(p, g);

		if (!gOwner && pGripData->type & NON_ALLOC_GRIP)
		{
			// There was no owner and this is a non-alloc grip.
			// This must be an automatic on the stack.  for the
			// purpose of save game, don't save this and don't
			// purge on restore.
			SetSaveStatus(g, FALSE);
			SetPurgeStatus(g, FALSE);
		}
	}

	return (g);
}


// undoes assign of grip to already existing allocation
void
MemoryMgr::ReleaseGrip(grip g)
{
	if (g == 0) return;

	// only release non alloc grips.  
	// alloc grips will get released through Free().
	if (gripDataArray[g].type & NON_ALLOC_GRIP)
	{
		MakeEmptyGrip(g);
	}
}


// change from the old grip number to the new grip number.
// return TRUE if error.
bool
MemoryMgr::ChangeGrip(grip oldGrip, grip newGrip)
{
	bool	error = FALSE;
	uint	hashIndex;

	// run in a while loop so that we can break

	while (TRUE)
	{
		if (oldGrip == newGrip)
		{
			// these are the same.  no need to do anything
			break;
		}

		if (gripDataArray[newGrip].type != EMPTY_GRIP)
		{
			// this grip is being used.  we can't do this.
			error = TRUE;
			break;
		}

		// transfer grip info
		gripDataArray[newGrip] = gripDataArray[oldGrip];

		// empty out the old grip
		MakeEmptyGrip(oldGrip);

		// set hash info
		hashIndex = gripDataArray[newGrip].hashIndex;
		gripHashArray[hashIndex].gNum = newGrip;
		gripHashArray[hashIndex].ptr = gripDataArray[newGrip].ptr;

		break;
	}

	return error;
}

//Return the size of the grip
size_t
MemoryMgr::SizeOfGrip(grip g)
{
	if (gripDataArray[g].type & ALLOC_GRIP)
	{
		return gripDataArray[g].data.alloc.size;
	}

	return 0;
}


// Return the grip assigned to an exact ptr (not an owning alloc), if present
grip
MemoryMgr::GetGrip(void* p)
{
	int	i;
	grip	start;

	if (p == NULL) 
	{
		APanic("Getting grip of null pointer");
	}

	// Hash the pointer to get a starting grip
	start = MEM_HASH_HASH(p);

	HashData	*pHashArray;

	// scan the hash table.
	for (i = start, pHashArray = &gripHashArray[start]; i < GRIPS_MAX; i++, pHashArray++)
	{
		if (pHashArray->ptr == p)
		{
			i = pHashArray->gNum;
			// must match an allocated grip
			switch (gripDataArray[i].type & ALLOC_GRIP)
			{
				case ALLOC_GRIP:
					return (grip) i;

				default:
					// non-alloc or empty grip
					return INVALID_GRIP;
			}
		}
	}

	// we must not have found it.  start from the beginning again
	for (i = 1, pHashArray = &gripHashArray[1]; i < start; i++, pHashArray++)
	{
		if (pHashArray->ptr == p)
		{
			i = pHashArray->gNum;
			// must match an allocated grip
			switch (gripDataArray[i].type & ALLOC_GRIP)
			{
				case ALLOC_GRIP:
					return (grip) i;

				default:
					// non-alloc or empty grip
					return INVALID_GRIP;
			}
		}
	}

//	GripData		*pGArray;
//
//	// scan array of pointer
//	for (i = start, pGArray = &gripDataArray[start]; i < GRIPS_MAX; i++, pGArray++)
//	{
//		if (pGArray->ptr == p) 
//		{
//			// must match an allocated grip
//			switch (pGArray->type & ALLOC_GRIP)
//			{
//				case ALLOC_GRIP:
//					return (grip) i;
//
//				default:
//					// non-alloc or empty grip
//					return INVALID_GRIP;
//			}
//		}
//	}
//
//	// we must not have found it.  start from the beginning again
//	for (i = 1, pGArray = &gripDataArray[1]; i < start; i++, pGArray++)
//	{
//		if (pGArray->ptr == p) 
//		{
//			// must match an allocated grip
//			switch (pGArray->type & ALLOC_GRIP)
//			{
//				case ALLOC_GRIP:
//					return (grip) i;
//
//				default:
//					// non-alloc or empty grip
//					return INVALID_GRIP;
//			}
//		}
//	}

	// didn't find an assigned grip
	return INVALID_GRIP;
}


		
void
MemoryMgr::Cycle()
{
	// When fully implemented MemoryMgr::Cycle() may cause
	// packing of memory, will always unlock derefenced ptrs, causing
	// them to be invalid.

	// Also note that this is NOT a redefinition of Object::Cycle()
}


//---------------------------------------------------------------
// Protected:
//---------------------------------------------------------------

// create a new grip, updating memory manager as needed
grip
MemoryMgr::FindEmptyGrip(grip start)
{
	grip	 		i;
	int			purgeCount = 0;
	bool			found = FALSE;
	GripData		*pGArray;

	// index 0 is invalid.
	// also, make sure we don't get into the system grip range
	if (start < FIRST_STANDARD_GRIP)
	{
		start = FIRST_STANDARD_GRIP;
	}

	while (!found)
	{
		// find first empty grip 
		for (i = start, pGArray = &gripDataArray[start]; i < STANDARD_GRIP_MAX; i++, pGArray++)
		{
			if (pGArray->ptr == NULL) 
			{
				found = TRUE;
				break;
			}
		}

		// if not yet found...
		if (!found && i >= STANDARD_GRIP_MAX) 
		{
			// continue search by wrapping back to zero
			for (i = FIRST_STANDARD_GRIP, pGArray = &gripDataArray[FIRST_STANDARD_GRIP]; i < start; i++, pGArray++)
			{
				if (pGArray->ptr == NULL) 
				{
					found = TRUE;
					break;
				}
			}
		}

		// array overflow error check
		if (!found)
		{
			if (i >= start) 
			{
				if (++purgeCount < 10)
				{
					// attemp to free up some grips
					APurge(300);
				}
				else 
				{
					// after tenth try, we ain't gonna do it
					Dump(7, "FindEmptyGrip() failed!", TRUE);
					APanic ("Out of Grips");
				}
			}
			else
			{
				// found empty grip, break loop
				found = TRUE;
				continue;
			}
		}
	};

	return i;
}


// create a new grip, updating memory manager as needed
// (This is almost a copy of FindEmptySysGrip.  This was done for speed.)
grip
MemoryMgr::FindEmptySysGrip()
{
	grip	 		i;
	int			purgeCount = 0;

	while (TRUE)
	{
		// find first empty grip 
		for (i = lastSysGripFound; i < SYSTEM_GRIP_MAX; i++)
		{
			if (gripDataArray[i].ptr == NULL) 
			{
				lastSysGripFound = (grip) (i + 1);
				break;
			}
		}

		// if not yet found...
		if (i >= SYSTEM_GRIP_MAX) 
		{
			// continue search by wrapping back to zero
			for (i = FIRST_SYSTEM_GRIP; i < SYSTEM_GRIP_MAX; i++)
			{
				if (gripDataArray[i].ptr == NULL) 
				{
					lastSysGripFound = (grip) (i + 1);
					break;
				}
			}
		}

		// array overflow error check
		if (i >= SYSTEM_GRIP_MAX) 
		{
			if (++purgeCount < 10)
			{
				// attemp to free up some grips
				APurge(300);
			}
			else 
			{
				// after tenth try, we ain't gonna do it
				APanic ("Out of System Grips");
			}
		}
		else
		{
			// found empty grip, break loop
			break;
		}
	};

	return i;
}


// remove a grip from the memory manager, does not free memory
void
MemoryMgr::MakeEmptyGrip(grip g)
{
	uint	hashIndex;

//	#ifndef NDEBUG
//		void*		p;
//
//		p = gripDataArray[g].ptr;
//		if (p == NULL) 
//		{
//			APanic ("Deleting a null grip");
//		}
//
//		// debug stuff.
//		// check to see if this alloc grip has any nonallocs hanging
//		// around.  This should never happen.  If it does, it
//		// probably means that someone overroad gSelf of a nonalloc.
//		// The '=' operator is the most common way to do this.
//		if (gripDataArray[g].type & ALLOC_GRIP)
//		{
//			grip	i;
//			char	mess[100];
//			for (i = 1; i < GRIPS_MAX; i++)
//			{
//				if ((gripDataArray[i].type & NON_ALLOC_GRIP) &&
//				  	gripDataArray[i].data.nonalloc.gOwner == g)
//				{
//					Dump(30000, "NonAlloc Error Dump", TRUE);
//					sprintf(mess, "NonAlloc %d still owned by Alloc %d\nProbably '=' problem.", i, g);
//					APanic(mess);
//				}
//			}
//		}
//
//	#endif

	// clear the hash first

	hashIndex = gripDataArray[g].hashIndex;
	gripHashArray[hashIndex].gNum = 0;
	gripHashArray[hashIndex].ptr = 0;
	gripDataArray[g].hashIndex = 0;

	// clear spot in grip a
	gripDataArray[g].ptr = NULL;
	gripDataArray[g].type = EMPTY_GRIP;
}






// this method searches for the allocated grip which contains the
//	non-allocated grip, returns the owning grip and the offset
// into the owner of the non-alloc'ed grip.

grip
MemoryMgr::FindOwnerOf(void* p, ptrdiff_t* pOffset)
{
	grip	i;
	grip	result = INVALID_GRIP;
	GripData	*pGArray;
	uint	start;

	if (p == NULL) 
	{
		APanic("Attempt to FindOwner of null pointer");
	}

	*pOffset = 0;

	// scan array of pointer

	start = lastAllocGrip;

//	// scary optization
//	int 	*pTemp;
//	int	numSearches;
//
//	// make sure that the pointer is on a 4 byte boundry
//	pTemp = (int *) ((int) p & ~3);
//
//	numSearches = ((int) pTemp >> 2);
//
//	int	beforeVal = BEFOREVAL;
//
//	// search backwards to find the before value of the allocation
//	#pragma aux FIND_BEFOREVAL =	\
//		"pushfd" 						\
//		"push		eax" 					\
//		"push		ecx" 					\
//		"push 	edi" 					\
//		"std" 							\
//		"mov		ecx,numSearches"	\
//		"mov		eax,beforeVal"		\
//		"mov		edi,pTemp"			\
//		"repne	scasd"				\
//		"mov		pTemp,edi" 			\
//		"pop	 	edi" 					\
//		"pop		ecx" 					\
//		"pop		eax" 					\
//      "popfd"; 
//	FIND_BEFOREVAL();
//
//	if (pTemp == 0)
//	{
//		// we didn't find it.
//	}
//	else
//	{
//		// the next dword should be the address to the alloc
//		pTemp++;
//		result = GetGrip(pTemp);
//
//		if (result != INVALID_GRIP)
//		{
//			// we found it!
//			*pOffset = (char *) p - (char *) pTemp;
//		}
//	}
	
//	grip	index;
//	int	tempPtr;
//	start = MEM_HASH_HASH(p);
//
//
//	if (gripDataArray[lastAllocGrip].ptr > p)
//	{
//		// an allocation must have happened in a constructor.
//		// use the hash to find a starting location.
//		// back up the memory address to increase the
//		// chances of finding it sooner
//		tempPtr = (int) p - 0x8000;
//		if (tempPtr < 0)
//		{
//			tempPtr = 0;
//		}
//		start = MEM_HASH_HASH(tempPtr);
//	}
//	else
//	{
//		// since in most cases, we are looking for the last alloc grip
//		// that was created, we will use that as a starting place 
//		start = gripDataArray[lastAllocGrip].hashIndex;	
//	}
//
//
//	for (i = start; i < GRIPS_MAX; i++)
//	{
//		index = gripHashArray[i].gNum;
//		pGArray = &gripDataArray[index];
//		if ((pGArray->type & ALLOC_GRIP) &&
//			 pGArray->ptr < p && 
//			 pGArray->data.alloc.endPtr > p)
// 		{
//			// we found it!
//			*pOffset = (char *) p - (char *) pGArray->ptr;
//			result = index;
//			break;
//		}
//	}
//
//	if (result == INVALID_GRIP)
//	{
//		for (i = 1; i < start; i++)
//		{
//			index = gripHashArray[i].gNum;
//			pGArray = &gripDataArray[index];
//			if ((pGArray->type & ALLOC_GRIP) &&
//			 	pGArray->ptr < p && 
//			 	pGArray->data.alloc.endPtr > p)
// 			{
//				// we found it!
//				*pOffset = (char *) p - (char *) pGArray->ptr;
//				result = index;
//				break;
//			}
//		}
//	}
	
	for (i = start, pGArray = &gripDataArray[start]; i < GRIPS_MAX; i++, pGArray++)
	{
		if (pGArray->type & ALLOC_GRIP)
		{
			if (pGArray->ptr < p && 
				 pGArray->data.alloc.endPtr > p)
 			{
				// we found it!
				*pOffset = (char *) p - (char *) pGArray->ptr;
				result = i;
				break;
			}
		}
	}

	if (result == INVALID_GRIP)
	{
		for (i = 1, pGArray = &gripDataArray[1]; i < start; i++, pGArray++)
		{
		if (pGArray->type & ALLOC_GRIP)
			{
				if (pGArray->ptr < p && 
				 	 pGArray->data.alloc.endPtr > p)
 				{
					// we found it!
					*pOffset = (char *) p - (char *) pGArray->ptr;
					result = i;
					break;
				}
			}
		}
	}

	if (result != INVALID_GRIP)
	{
		// since we usually find owners in groups of non-allocs, 
 		// and the lastAlloc may have been changed by a constructor
		// allocating memory, save the result to be where we
		// start next time.  This speeds things up.
		lastAllocGrip = result;
		lastAllocPtr = pGArray->ptr;
	}

	return result;
}



//	Pack allocations, return false if no packing can be done.
// Currently stubbed.
bool
MemoryMgr::PackMem()
{
	return FALSE;
}


void
MemoryMgr::ClearAllocations()
{
	uint32	i;

	// clear all standard grips

	// clear nonallocs first (to avoid error detection)
	for (i = FIRST_STANDARD_GRIP; i < STANDARD_GRIP_MAX; i++)
	{
		// only purge grips with the NO_PURGE_GRIP bit clear
		if	(!(gripDataArray[i].type & NO_PURGE_GRIP))
		{
			if (gripDataArray[i].type & NON_ALLOC_GRIP) 
			{
				MakeEmptyGrip(i);
			}
		}
	}

	// clear alloc grips
	for (i = FIRST_STANDARD_GRIP; i < STANDARD_GRIP_MAX; i++)
	{
		// only purge grips with the NO_PURGE_GRIP bit clear
		if	(!(gripDataArray[i].type & NO_PURGE_GRIP))
		{
			if (gripDataArray[i].type & ALLOC_GRIP) 
			{
				Free((grip) i);
			}
		}
	}
}

bool
MemoryMgr::WriteAllocations(FILE *fp)
{
	uint32	i;
	bool		error = FALSE;
	grip		gripNum;

	// save all allocations that we are responsible for
	for (i = FIRST_STANDARD_GRIP; i < STANDARD_GRIP_MAX; i++)
	{
		if (gripDataArray[i].type != EMPTY_GRIP)
		{
			if (!(gripDataArray[i].type & NO_SAVE_GRIP))
			{
				// save this one
				gripNum = (grip) i;
				if (fwrite(&gripNum, 1, sizeof(grip), fp) != sizeof(grip))
				{
					// problem. let's exit
					error = TRUE;
					break;
				}

				if (SaveGrip((grip) i, fp) != 0)
				{
					// problem
					error = TRUE;
					break;
				}
			}
		}
	}

	// terminate the grip list
	gripNum = 0xffff;
	if (fwrite(&gripNum, 1, sizeof(grip), fp) != sizeof(grip))
	{
		// problem
		error = TRUE;
	}

	return error;
}


bool
MemoryMgr::ReadAllocations(FILE *fp)
{
	bool		error = FALSE;
	size_t	bytesRead;
	grip		gripNum;

	// restore saved allocations
	while (TRUE)
	{
		bytesRead = fread(&gripNum, 1, sizeof(grip), fp);
		if (bytesRead != sizeof(grip))
		{
			// we had a problem.  time to leave
			error = TRUE;
			break;
		}
		else
		{
			if (gripNum == 0xffff)
			{
				// we are through
				break;
			}
			else
			{
				// restore the grip
				if (RestoreGrip(gripNum, fp) != 0)
				{
					// problem;
					error = TRUE;
					break;
				}
			}
		}
	}

	return error;
}


// Set the bit that determines whether the memory manager
// should save this grip during a save game.  FALSE should
// be passed for grips that you don't want the memory
// manager to save.  This may mean that the your individual
// application will take care of the saving itself.  An 
// example of this is resources.  The default state of
// grips is to be saved.  
// All grips associated with the passed grip will have their
// status changed. (ALLOCs and NON_ALLOCs)
void		
MemoryMgr::SetSaveStatus(grip g, bool saveIt)
{
	grip	i;

 	if (gripDataArray[g].type & NON_ALLOC_GRIP)
	{
		// save the grip
		i = g;

		// find the owner and use that
		g = gripDataArray[g].data.nonalloc.gOwner;

		if (!g)
		{
			// we have no owner.  this means that we are constructing.
			// since we have no owner, just change the bit accordingly
			// and don't search through the grip list.  This condition will
			// during initial allocations because non-allocs are created
			// before the allocs.
			if (saveIt)
			{
				// save this grip.  make sure that the bit is clear
				gripDataArray[i].type &= ~NO_SAVE_GRIP;
			}
			else
			{
				// don't save this grip
				gripDataArray[i].type |= NO_SAVE_GRIP;
			} 
		}
	}

	// only work if we have a valid alloc grip
	if (g)
	{
		for (i = 1; i < GRIPS_MAX; i++)
		{
			if (((gripDataArray[i].type & ALLOC_GRIP) && (i == g)) ||
			 	((gripDataArray[i].type & NON_ALLOC_GRIP) && (gripDataArray[i].data.nonalloc.gOwner == g)))

			{
				// change this one
		
				if (saveIt)
				{
					// save this grip.  make sure that the bit is clear
					gripDataArray[i].type &= ~NO_SAVE_GRIP;
				}
				else
				{
					// don't save this grip
					gripDataArray[i].type |= NO_SAVE_GRIP;
				}					  
			}
		}
	}
}

// set the purge status of a grip.  this flage will be
// looked at on a restore game.  If the NO_PURGE_GRIP
// bit is set, the grip will be purged.  The default
// is that the grip be purged.
// All grips associated with the passed grip will have their
// status changed. (ALLOCs and NON_ALLOCs)
void		
MemoryMgr::SetPurgeStatus(grip g, bool purgeIt)
{
	grip	i;

 	if (gripDataArray[g].type & NON_ALLOC_GRIP)
	{
		// save the grip
		i = g;

		// find the owner and use that
		g = gripDataArray[g].data.nonalloc.gOwner;

		if (!g)
		{
			// we have no owner.  this means that we are constructing.
			// since we have no owner, just change the bit accordingly
			// and don't search through the grip list.  This condition will
			// during initial allocations because non-allocs are created
			// before the allocs.

			if (purgeIt)
			{
				// save this grip.  make sure that the bit is clear
				gripDataArray[i].type &= ~NO_PURGE_GRIP;
			}
			else
			{
				// don't save this grip
				gripDataArray[i].type |= NO_PURGE_GRIP;
			}
		}
	}

	// only work if we have a valid alloc grip
	if (g)
	{
		for (i = 1; i < GRIPS_MAX; i++)
		{
			if (((gripDataArray[i].type & ALLOC_GRIP) && (i == g)) ||
			 	((gripDataArray[i].type & NON_ALLOC_GRIP) && (gripDataArray[i].data.nonalloc.gOwner == g)))

			{
				// change this one
		
				if (purgeIt)
				{
					// save this grip.  make sure that the bit is clear
					gripDataArray[i].type &= ~NO_PURGE_GRIP;
				}
				else
				{
					// don't save this grip
					gripDataArray[i].type |= NO_PURGE_GRIP;
				}
			}
		}
	}
}

// Write grip number and grip information to a file
bool
MemoryMgr::SaveGripNum(grip gNum, FILE *fp)
{
	bool	error = FALSE;

	if (fwrite(&gNum, 1, sizeof(grip), fp) != sizeof(grip))
	{
		// problem. let's exit
		error = TRUE;
	}
	else
	{
		if (SaveGrip(gNum, fp) != 0)
		{
			// problem
			error = TRUE;
		}
	}

	return error;
}

// Write grip information to a file
bool
MemoryMgr::SaveGrip(grip gNum, FILE *fp)
{
	bool	error = FALSE;
	struct mh *dl;
	size_t	writeSize;

	#ifdef MEM_DEBUG
		bool	debugging = TRUE;
	#else
		bool	debugging = FALSE;
	#endif

	// run in a while loop so that we can break on error
	while (TRUE)
	{
		// write the debugging flag
		if (fwrite(&debugging, 1, sizeof(bool), fp) != sizeof(bool))
		{
			// write error
			error = TRUE;
			break;
		}

		// if debugging, write debug information

		if (debugging)
		{
			dl = (struct mh *) ((char *) gripDataArray[gNum].ptr - sizeof(struct mh));

			if (fwrite(dl, 1, sizeof(struct mh), fp) != sizeof(struct mh))
			{
				// write error
				error = TRUE;
				break;
			}
		}

		// write grip 

		if (fwrite(&gripDataArray[gNum], 1, sizeof(GripData), fp) != sizeof(GripData))
		{
			// write error
			error = TRUE;
			break;
		}

		// what comes next depends on the grip_t
		switch (gripDataArray[gNum].type & (ALLOC_GRIP | NON_ALLOC_GRIP))
		{
			default:
			case EMPTY_GRIP:
				// nothing more
				break;

			case ALLOC_GRIP:
				// write out the allocation
				writeSize = gripDataArray[gNum].data.alloc.size;
				if (fwrite(pMemMgr->Deref(gNum), 1, writeSize, fp) != writeSize)
				{
					// write error
					error = TRUE;
				}

				break;

			case NON_ALLOC_GRIP:
				break;
		}

		break;
	}

 	return error;
}

// Read grip number and grip information from a file.
// The grip number restored is put into gNum.
bool
MemoryMgr::RestoreGripNum(grip *gNum, FILE *fp)
{
	bool	error = FALSE;

	if (fread(gNum, 1, sizeof(grip), fp) != sizeof(grip))
	{
		// problem. let's exit
		error = TRUE;
	}
	else
	{
		if (RestoreGrip(*gNum, fp) != 0)
		{
			// problem
			error = TRUE;
		}
	}

	return error;
}

// Read grip information from a file
bool
MemoryMgr::RestoreGrip(grip gNum, FILE *fp)
{
	bool			error = FALSE;
	bool			savedDebugInfo;
	struct mh 	debugInfo;
	size_t		readSize;
	void*			vp;
	char			errorMess[100];

	#ifdef MEM_DEBUG
		bool	debugging = TRUE;
	#else
		bool	debugging = FALSE;
	#endif

	// run in a while loop so that we can break on error
	while (TRUE)
	{
		// make sure that this grip is not being used!
		if (gripDataArray[gNum].ptr != NULL)
		{
			sprintf(errorMess, "Restore grip still in use - gnum: %d", gNum);
			Dump(29999, errorMess, TRUE);
			APanic(errorMess);
		}

		// read debugging flag
		if (fread(&savedDebugInfo, 1, sizeof(bool), fp) != sizeof(bool))
		{
			// read error
			error = TRUE;
			break;
		}

		// if the debugging flag is set and we are debugging,
		// read the info in.
		// if the debugging flag is set and we are not debugging,
		// skip past the debug info.

		if (savedDebugInfo)
		{
			// get debug info
			if (fread(&debugInfo, 1, sizeof(struct mh), fp) != sizeof(struct mh))
			{
				// read error
				error = TRUE;
				break;
			}

			if (debugging)
			{
				lineTrack   = debugInfo.Mline;
				szFileTrack = (char *) debugInfo.Mfile;
				szStrTrack	= debugInfo.Mstr;
			}
		}
		else
		{
			// no info saved
			if (debugging)
			{
				// we are debugging with no prior info to use.
				// give TrackMem something;
				TRACK_MEM("Restore Game");
			}
		}

		// read the grip
		if (fread(&gripDataArray[gNum], 1, sizeof(GripData), fp) != sizeof(GripData))
		{
			// read error
			error = TRUE;
			break;
		}

		// what comes next depends on the grip_t
		switch (gripDataArray[gNum].type & (ALLOC_GRIP | NON_ALLOC_GRIP))
		{
			default:
				sprintf(errorMess, "Bad grip type on Restore - gnum: %d", gNum);
				APanic(errorMess);
				break;

			case ALLOC_GRIP:
				// read in the allocation
				readSize = gripDataArray[gNum].data.alloc.size;

				vp = MallocAtGrip(readSize, gNum);
				if (fread(vp, 1, readSize, fp) != readSize)
				{
					// read error
					MakeEmptyGrip(gNum);
					error = TRUE;
					break;
				}
				break;

			case NON_ALLOC_GRIP:
			case EMPTY_GRIP:
				break;
		}

		if(gripDataArray[gNum].type & TMOV_GRIP)
		{
			pMono->Out("MemMgr::RestoreGrip(g%d) CID%d need vtable pointer fixup!\n",
				gNum, (int)(((TMovable *)vp)->classID));
			// BUGBUG! We still need this logic.
		}

		break;
	}

 	return error;
}

// Fix Grip Offset information for NON_ALLOCs.
// This should be done after all grips have been Restored.

bool
MemoryMgr::RestoreGripOffsets()
{
	bool			error = FALSE;
 	grip			i;

	// now, all the grips should be here.  fix all the ptrs
	// for the NON_ALLOC grips.  we couldn't do this as we
	// loaded the grips because the ALLOC grip owner could be a higher
	// grip number and therefore wouldn't be loaded yet.
	// this will fix all NON_ALLOC's, not just the ones
	// that were loaded.  That should be fine.

	for (i = 1; i < GRIPS_MAX; i++)
	{
		// if this is a NO_ALLOC_GRIP and not on the stack (gOwner == 0),
		// fix the pointer
		if (gripDataArray[i].type & NON_ALLOC_GRIP &&
			 gripDataArray[i].data.nonalloc.gOwner != 0)
		{
			if (gripDataArray[gripDataArray[i].data.nonalloc.gOwner].type & ALLOC_GRIP)
			{
				gripDataArray[i].ptr = (char *) gripDataArray[gripDataArray[i].data.nonalloc.gOwner].ptr
												+ gripDataArray[i].data.nonalloc.offset;
				SetHashTable(gripDataArray[i].ptr, i);
			}
			else
			{
				// this restore is all screwed up...return an error and exit
				error = TRUE;
				break;
			}

		}
	}

 	return error;
}


//	print out information of all grips, debugging only
//
void
MemoryMgr::Dump(uint16 dumpNum, char *title, bool overWrite)
{
	#ifndef NDEBUG

	int	i;
	int	nGrips = 0;
	FILE*	pFile;
	char	dumpName[60];
	struct mh *dl;
	char	*dumpTitle;

	sprintf(dumpName, "mem%05d.dmp", dumpNum);
	if (overWrite)
	{
		// trunc any existing file
		pFile = fOpenPrefs(dumpName, "wt");
	}
	else
	{
		// append to any existing file
		pFile = fOpenPrefs(dumpName, "at");
	}
	if (!pFile)
	{
		APanic ("can't open memmgr.dmp");
	}
	ASSERT (pFile);

	if (title == NULL)
	{
		dumpTitle = "";
	}
	else
	{
		dumpTitle = title;
	}

	fprintf (pFile, "\n\nMemory manager dump: %s\n", dumpTitle);

	fprintf(pFile, "\n %d Total Bytes Used", (int) UsedMem());
	fprintf(pFile, "\n %d Total Bytes Free", (int) AvailMem());
	fprintf(pFile, "\n %d Largest Block\n", (int) LargestAlloc());

	// init array as NULLs 
	for (i = 1; i < GRIPS_MAX; i++)
	{

		if (i == FIRST_SYSTEM_GRIP)
 		{
			fprintf (pFile, "\n\n******** SYSTEM GRIPS ********\n"); 
		}
		else
		{
			if (i == FIRST_STANDARD_GRIP)
 			{
				fprintf (pFile, "\n\n******** STANDARD GRIPS ********\n"); 
			}
		}

		switch (gripDataArray[i].type & (NON_ALLOC_GRIP | ALLOC_GRIP))
		{
			default:
			case EMPTY_GRIP:
				break;

			case ALLOC_GRIP:
				nGrips++;

				// show the name.
				dl = (struct mh *) ((char *) gripDataArray[i].ptr - sizeof(struct mh));
				if (dl->Mstr)
				{
					// we have a name
					sprintf(dumpName, "- NAME: %s", dl->Mstr);
				}
				else
				{
					dumpName[0] = 0;	
				}

				// if this is a resource, show locked status
				if (strstr(dumpName, "Res:"))
				{
					if (((Resource *) gripDataArray[i].ptr)->attr & RA_LOCKED)
					{
						strcat(dumpName, " (Locked)"); 
					}
					else
					{
						strcat(dumpName, " (Not Locked)"); 
					}
				}

				fprintf (pFile, "\n grip: %d  Allocation %u bytes, at %x %s", 
					i,
					gripDataArray[i].data.alloc.size,
					gripDataArray[i].ptr,
					dumpName);

				break;

			case NON_ALLOC_GRIP:
				nGrips++;
				fprintf (pFile, "\n grip: %d  Non-Allocation ptr %x owned by grip %hu, at offset %lu", 
					i,
					gripDataArray[i].ptr,
					gripDataArray[i].data.nonalloc.gOwner,
					gripDataArray[i].data.nonalloc.offset);
				break;
		}
	}

	fprintf (pFile, "\n\nEnd dump: %d unfreed grips\n\n", nGrips);
	fclose(pFile);

	#else
	dumpNum = dumpNum;
	title = title;
	overWrite = overWrite;
	#endif  // NDEBUG
}


//	print out information of all grips, debugging only
//
void
MemoryMgr::DumpGrip(grip gSpec)
{
	#ifndef NDEBUG

	if (pMono)
	{
		switch (gripDataArray[gSpec].type & (ALLOC_GRIP | NON_ALLOC_GRIP))
		{
//			default:
			case EMPTY_GRIP:
				pMono->Out ("\n Empty grip %d", (int) gSpec);
				break;


			case ALLOC_GRIP:
				pMono->Out ("\n Allocation %u bytes, at %x", 
					gripDataArray[gSpec].data.alloc.size,
					gripDataArray[gSpec].ptr);
				break;

			case NON_ALLOC_GRIP:
				pMono->Out ("\n Non-Allocation ptr %x owned by grip %h, at offset %l", 
					gripDataArray[gSpec].ptr,
					gripDataArray[gSpec].data.nonalloc.gOwner,
					gripDataArray[gSpec].data.nonalloc.offset);
				break;
		}
	}
	#else
	gSpec = gSpec;
	#endif  // NDEBUG
}



// Return the largest allocation possible
//
size_t
MemoryMgr::LargestAlloc()
{
	return ALargestAlloc();
}


// Return amount of memory available
//
size_t
MemoryMgr::AvailMem()
{
	return AAvailMem();
}


// Return amount of used memory
//
size_t
MemoryMgr::UsedMem()
{
	grip		i;
	size_t	memUsed = 0;

	for (i = 1; i < GRIPS_MAX; i++)
	{
		if (gripDataArray[i].type & ALLOC_GRIP)
		{
			memUsed += gripDataArray[i].data.alloc.size;
		}
	}

	return memUsed;
}


// debugging functions
//
int
MemoryMgr::CheckGrip(grip g)
{
	if (g == 0)					return GRIP_ZERO;
	if (g >= GRIPS_MAX)		return GRIP_OUT_OF_RANGE;

	void*	p;

	p = gripDataArray[g].ptr;

	if (p == NULL)				return GRIP_EMPTY;

//	mem_checkptr(p);

	return GRIP_VALID;
}


void
MemoryMgr::ShowGripNames(uint16 start)
{
	Object*	pO;
	uint16	i;
	uint16	gs;
	char		theName[40];
	FILE*	fp;

	fp = fOpenPrefs("gripname.dmp", "rt");
	ASSERT (fp);

  	fprintf (fp, "\n\nDump of grip names:\n");

	for (i = start; i < GRIPS_MAX; i++)
	{
		if (gripDataArray[i].type == EMPTY_GRIP)
			break;

		pO = (Object*) gripDataArray[i].ptr;
		gs = 0;
		memcpy(&gs, ((uchar*) pO) + 4, 1);
		if (gs == i)
		{
			if (pO->name)
			{
				strcpy(theName, pO->name);
			}
			else
			{
				strcpy(theName, "             ");
			}
		}
		else
		{
			strcpy(theName, "------");
		}
		fprintf (fp, "%d: %s\n", i, theName);
	}
	fprintf (fp, "\nEnd dump\n\n");
}

bool
MemMgrSave(uint16 state, FILE *fp)
{
	bool		error = FALSE;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
			// save all allocations that we are responsible for
			error = pMemMgr->WriteAllocations(fp);
			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			// clear all standard grips
			pMemMgr->ClearAllocations();
			break;

		case DURING_RESTORE:
			// restore saved allocations
			error = pMemMgr->ReadAllocations(fp);
			break;

		case AFTER_RESTORE:
			break;
	}

	return error;
}

