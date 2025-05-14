//
// HEAP.HPP
//
// April 11, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
// Heap class definition.
//
//----[]-------------------------------------------------------------

#ifndef	heap_hpp
#define	heap_hpp

#include "tigre.hpp"

// allocation strategies
enum alloc_strat
{
	ALLOC_LOW = 1,
	ALLOC_HIGH
};

#pragma pack(4)
// the AllocNode struct.  This is at the beginning of every memory
// allocation.
struct AllocNode
{
	// the prev and next of the linked list that this node belongs to.
	AllocNode	*prev, *next;

	// the prev and next based on physical memory. (used for packing)
	AllocNode	*physicalPrev, *physicalNext;

	uint32	size;				// size that was requested for allocation  
	uint32	actualSize;		// actual size after allocation alignment
	grip		gripNum;			// grip index in the grip array
	int32		locked;			// if non-zero, it is locked
};
#pragma pack()

class Heap 
{
	public:
		size_t	totalMemPool;
		uint32	availBlocks;
		uint32	usedBlocks;
		uint32	availMem;

		void		*allocAddr;

		AllocNode	*firstFree, *lastFree;
		AllocNode	*firstUsed, *lastUsed;
	
					Heap();
					~Heap();

		bool		Setup(size_t memSize);
		void		Cleanup(void);
		void		Pack(uint32 requestedSize = 0);

		size_t	LargestAvailBlock(void);
		size_t	AvailMem(uint32 *numBlocks = NULL);
		size_t	UsedMem(uint32 *numBlocks = NULL);
		size_t	TotalMem(uint32 *numBlocks = NULL);

		size_t	TestAvailMem(uint32 *numBlocks = NULL);
		size_t	TestUsedMem(uint32 *numBlocks = NULL);
		size_t	TestTotalMem(uint32 *numBlocks = NULL);

		void		Lock(grip gNum, bool lockIt);

 		void		CheckMem(void);
		void		Dump(void);

		void*		MallocAtGrip(size_t size, grip g, alloc_strat strategy);

		AllocNode*	AddUsedNode(size_t size, uint32 actualSize,  uint32 strategy, AllocNode *sourceNode);
		void			DelUsedNode(AllocNode *memNode);
};

#endif	// heap_hpp
