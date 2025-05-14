//
// HEAP.CPP
//
// April 11, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
// Heap class definition.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "heap.hpp"
#include "memmgr.hpp"

// if less than this is left, merge into the free node.
// in other words, if we don't have enough memory for an AllocNode
// and a 4 byte allocation, don't bother leaving this block around.
#define	MIN_FREE_BLOCK_SIZE	(sizeof(AllocNode) + 4)


Heap::Heap()
{
	allocAddr = 0;
	Cleanup();
}


Heap::~Heap()
{
	Cleanup();	
}


bool
Heap::Setup(size_t memSize)
{
	bool	success = FALSE;

	if (pMemMgr) 
	{
		// just in case we already have memory, call cleanup
		Cleanup();

		// get a whole bunch of memory
		AllocNode *aNode = (AllocNode *) malloc(memSize);

		if (aNode)
		{
			totalMemPool = memSize;

			aNode->actualSize = memSize;
			aNode->size = 0;						// zero size means unused
			aNode->prev = aNode->next = NULL;
			aNode->physicalPrev = aNode->physicalNext = NULL;
			aNode->locked = 0;
			aNode->gripNum = 0;

			firstFree = lastFree = aNode;			
			firstUsed = lastUsed = NULL;
			availBlocks = 1;
			availMem = totalMemPool;
			success = TRUE;
		}
	}
}

void
Heap::Cleanup(void)
{
	if (allocAddr)
	{
		free(allocAddr);
		allocAddr = 0;
	}

	availBlocks = availMem = usedBlocks = 0;
}


// to lock memory, pass TRUE.
// to unlock memory, pass FALSE.
// lock is stored as a counter, so that the lock can be multiple
// levels deep.
void
Heap::Lock(grip gNum, bool lockIt)
{
	ASSERT(gNum > 0 && gNum < GRIPS_MAX);

	AllocNode	*memNode = (AllocNode *) pMemMgr->gripDataArray[gNum].ptr;

	if (memNode == NULL)
	{
		char mess[100];
		sprintf(mess, "Null grip lock failure. (%d)", gNum);
		APanic(mess);
	}

	if (lockIt)
	{
		// lock the memory
		memNode->locked++;
	}
	else
	{
		// unlock the memory.
		if (memNode->locked)
		{
			memNode->locked--;
		}
	}
}

// if no memory available, null returned.
void*
Heap::MallocAtGrip(size_t size, grip g, alloc_strat strategy)
{
	uint32	actualSize;
	char		mess[100];
	bool		triedToPack = FALSE;

	if (pMemMgr->gripDataArray[g].ptr)
	{
		sprintf(mess, "Grip already allocated. (%d)", g);
		APanic(mess);
	}

	AllocNode	*usedNode = NULL;
	AllocNode	*sourceNode = NULL;

	// add on the size for the AllocNode struct that will be at the
	// front of the allocation.  Also align on a 4 byte boundry.
	actualSize = (((size +  sizeof(AllocNode)) + 3) >> 2) << 2;

	while(TRUE)
	{
		switch(strategy)
		{
			case ALLOC_LOW:
				sourceNode = firstFree;
				while(sourceNode)
				{
					if (sourceNode->actualSize >= actualSize)
					{
						break;
					}

					sourceNode = sourceNode->next;
				}
				break;

			case ALLOC_HIGH:
				sourceNode = lastFree;
				while(sourceNode)
				{
					if (sourceNode->actualSize >= actualSize)
					{
						break;
					}

					sourceNode = sourceNode->prev;
				}
				break;

			default:
				sprintf(mess, "Invalid strategy %d", strategy);
				APanic(mess);
		}

		if (sourceNode || triedToPack)
		{
			// we found a node and/or packed
			break;
		}
		else
		{
			// we couldn't find a node big enough.  try a pack for this size
			Pack(size);
			triedToPack = TRUE;
		}
	}

	if (sourceNode)
	{
		// add the used node.
		usedNode = AddUsedNode(size, actualSize, strategy, sourceNode);
	}
	else
	{
		// no memory was found
		APanic("Out of Memory.  Sorry.");
	}

	if (usedNode)
	{
		usedNode->gripNum = g;
		pMemMgr->gripDataArray[g].ptr = usedNode;	
	}

	return	usedNode;
}

// Add a node to the used list and remove what is appropriate
// from the free list.  actualSize should include the size
// of the AllocNode itself.  sourceNode is a pointer to a free node.
// The pointer to the used node is returned. 
AllocNode*
Heap::AddUsedNode(size_t size, uint32 actualSize, uint32 strategy, AllocNode *sourceNode)
{
	uint32	sizeLeft;

	AllocNode	*highNode;
	AllocNode	*lowNode;
	AllocNode	*freeNode;
	AllocNode	*usedNode = NULL;

	if (sourceNode)
	{
		if (actualSize < sizeof(AllocNode))
		{
			APanic("Alloc size less than size of AllocNode");	
		}

		sizeLeft = sourceNode->actualSize - actualSize;
		if (sizeLeft < MIN_FREE_BLOCK_SIZE)
		{
			// not enough left for an AllocNode and a small allocation.
			// let this node have the extra bytes.

			// since we are using up the whole node, decrease the availBlocks
			availBlocks--;

			usedNode = sourceNode;
			
			usedNode->size = size;

			// physical prev and next will be the same

			// remove from the free list

			if (usedNode->prev != NULL)
			{
				(usedNode->prev)->next = usedNode->next;
			}
			if (usedNode->next != NULL)
			{
				usedNode->next->prev = usedNode->prev;
			}

			if (usedNode == firstFree)
			{
				firstFree = usedNode->next;
			}
			if (usedNode == lastFree)
			{
				lastFree = usedNode->prev;
			}

			// now put into the used list

			// since free nodes can only have used nodes on
			// both sides (physically), we can link in.
			usedNode->prev = usedNode->physicalPrev;		
			if (usedNode->prev)
			{
				usedNode->prev->next = usedNode;
			}

			usedNode->next = usedNode->physicalNext;		
			if (usedNode->next)
			{
				usedNode->next->prev = usedNode;
			}

			if (firstUsed == NULL ||
				 usedNode < firstUsed)
			{
				firstUsed = usedNode;
			}

			if (lastUsed == NULL ||
				(lastUsed != NULL && usedNode > lastUsed))
			{
				lastUsed = usedNode;
			}
		}
		else
		{
			// we have enough to split this node.

			// split the node.

			switch(strategy)
			{
				case ALLOC_LOW:
					usedNode = lowNode = sourceNode;
					freeNode = highNode = (AllocNode *) ((char *) sourceNode + actualSize);
					break;

				case ALLOC_HIGH:
					freeNode = lowNode = sourceNode;
					usedNode = highNode = (AllocNode *) ((char *) sourceNode + sizeLeft);
					break;
			}

			// copy the existing node structure
			memcpy(highNode, lowNode, sizeof(AllocNode));

			freeNode->actualSize = sizeLeft;
			usedNode->size = size;
			usedNode->actualSize = actualSize;

			// fix up the free list
			if (sourceNode->prev)
			{
				sourceNode->prev->next = freeNode;
			}
			if (sourceNode->next)
			{
				sourceNode->next->prev = freeNode;
			}

			if (sourceNode == firstFree)
			{
				firstFree = freeNode;
			}
			if (sourceNode == lastFree)
			{
				lastFree = freeNode;
			}

			// fix up this used node and add to
			// the used list

			// since free nodes can only have used nodes on
			// both sides (physically), we can link in.

			usedNode->prev = sourceNode->physicalPrev;		
			if (usedNode->prev)
			{
				usedNode->prev->next = usedNode;
			}

			usedNode->next = sourceNode->physicalNext;		
			if (usedNode->next)
			{
				usedNode->next->prev = usedNode;
			}

			if (firstUsed == NULL ||
				 usedNode < firstUsed)
			{
				firstUsed = usedNode;
			}

			if (lastUsed == NULL ||
				(lastUsed != NULL && usedNode > lastUsed))
			{
				lastUsed = usedNode;
			}

			// fix the physical links
			highNode->physicalPrev = lowNode;

			if (highNode->physicalNext)
			{
				highNode->physicalNext->physicalPrev = highNode;	
			}

			lowNode->physicalNext = highNode;
		}

		usedNode->locked = 0;
		usedBlocks++;

		availMem -= usedNode->actualSize;
	}

	return	usedNode;
}

// remove a used node from the used list
// and add it to the free list.
void
Heap::DelUsedNode(AllocNode *memNode)
{
	int32			availBlocksAdded = 0;
	AllocNode	*searchNode;

	if (memNode)
	{
		usedBlocks--;
		availMem += memNode->actualSize;
		availBlocksAdded = 1;
		memNode->gripNum = 0;
		memNode->locked = 0;

		// remove from the used list

		if (firstUsed == memNode)
		{
			firstUsed = memNode->next;
		}

		if (lastUsed == memNode)
		{
			lastUsed = memNode->prev;
		}

		if (memNode->next)
		{
			memNode->next->prev = memNode->prev;
		}

		if (memNode->prev)
		{
			memNode->prev->next = memNode->next;
		}

		// add to free list
		if (memNode->physicalPrev &&
			 memNode->physicalPrev->size == 0)
		{
			// join with this free node
			memNode->physicalPrev->actualSize += memNode->actualSize;
			memNode->physicalPrev->physicalNext = memNode->physicalNext;
			if (memNode->physicalNext)
			{
				memNode->physicalNext->physicalPrev = memNode->physicalPrev;
			}
			memNode = memNode->physicalPrev;

			// we have one less avail block then we would have had
			availBlocksAdded--;
		}

		if (memNode->physicalNext &&
			 memNode->physicalNext->size == 0)
		{
			// join with this free node
			memNode->physicalNext->actualSize += memNode->actualSize;
			memNode->physicalNext->physicalPrev = memNode->physicalPrev;

			if (memNode->physicalNext->prev == memNode)
			{
				// if this is true, we already merged the other side.
				// use our current node's prev
				memNode->physicalNext->prev = memNode->prev;
			}

			memcpy(memNode, memNode->physicalNext, sizeof(AllocNode));

			if (memNode->physicalNext)
			{
				memNode->physicalNext->physicalPrev = memNode;	
			}

			if (memNode->prev)
			{
				memNode->prev->next = memNode;
			}

			if (memNode->next)
			{
				memNode->next->prev = memNode;
			}

			// we have one less avail block then we would have had
			availBlocksAdded--;
		}

		if (memNode->size)
		{
			// since we still have a size, we didn't merge with anyone.
			// no neighboring free nodes.
			// make this a free node and add to the free list
			memNode->size = 0;

			// since this node stands by itself, the physical links
			// remain the same.

			// To find the prev and next, we could use the physical
			// prev and next's, but since there should be far more allocated
			// nodes than free nodes, it should be faster to search
			// through the free list and find the place to insert it.

			if (firstFree == NULL)
			{
				// since this is NULL, the free list must be empty.
				memNode->next = memNode->prev = NULL;
			}
			else
			{
				if (memNode < firstFree)
				{
					// this is now the first in the list
					memNode->prev = NULL;
					memNode->next = firstFree;
					firstFree->prev = memNode;
				}
				else
				{
					if (memNode > lastFree)
					{
						// this is now the last in the list
						lastFree->next = memNode;
						memNode->prev = lastFree;
						memNode->next = NULL;
					}
					else
					{
						// it must be between two nodes
						searchNode = firstFree->next;
						while(memNode > searchNode)
						{
							if (searchNode->next)
							{
								searchNode = searchNode->next;
							}
							else
							{
								// the next one is NULL.  break here
								break;
							}
						}

						// searchNode now points to the node after memNode.
						// insert here.
						memNode->prev = searchNode->prev;
						memNode->prev->next = memNode;

						memNode->next = searchNode;
						searchNode->prev = memNode;
					}
				}
			}
		}
				
		if (memNode->prev == NULL)
		{
			// if we have no prev, we must be the first in the list
			firstFree = memNode;
		}

		if (memNode->next == NULL)
		{
			// if we have no next, we must be the last in the list
			lastFree = memNode;
		}

		// update the availBlocks variable
		availBlocks += availBlocksAdded;
	}
}

uint32
Heap::LargestAvailBlock(void)
{
	uint32		largestBlock = 0;
	
	AllocNode	*memNode = firstFree;

	while(memNode)
	{
		if (memNode->actualSize > largestBlock)
		{
			largestBlock = memNode->actualSize;
		}

		if (memNode->next != NULL &&
			 memNode >= memNode->next)
		{
			APanic("Available List Corrupt!");
		}
		memNode = memNode->next;
	}

	return largestBlock;
}

uint32
Heap::AvailMem(uint32 *numBlocks)
{
	if (numBlocks)
	{
		*numBlocks = availBlocks;
	}

	return availMem;
}

uint32
Heap::TestAvailMem(uint32 *numBlocks)
{
	uint32	availMemory = 0;
	uint32	availNextBlocks = 0;
	uint32	availPrevBlocks = 0;
	
	AllocNode	*memNode = firstFree;

	// walk the next links
	while(memNode)
	{
		availNextBlocks++;
	
		availMemory += memNode->actualSize;

		if (memNode->next != NULL &&
			 memNode >= memNode->next)
		{
			APanic("Available List Next Corrupt!");
		}
		memNode = memNode->next;
	}

	memNode = lastFree;

	// walk the prev links
	while(memNode)
	{
		availPrevBlocks++;
		if (memNode->prev != NULL &&
			 memNode <= memNode->prev)
		{
			APanic("Available List Prev Corrupt!");
		}
		memNode = memNode->prev;
	}

	if (availNextBlocks != availPrevBlocks)
	{
		APanic("Available Prev and Next blocks not the same!");
	}

	if (availBlocks != availNextBlocks)
	{
		APanic("Available blocks different from current count");	
	}

	if (availMem != availMemory)
	{
		APanic("Available memory different from current count");	
	}

	if (numBlocks)
	{
		*numBlocks = availNextBlocks;
	}

	return availMem;
}

uint32
Heap::UsedMem(uint32 *numBlocks)
{
	if (numBlocks)
	{
		*numBlocks = usedBlocks;	
	}

	return (totalMemPool - availMem);	
}

uint32
Heap::TestUsedMem(uint32 *numBlocks)
{
	uint32	usedMem = 0;
	uint32	usedNextBlocks = 0;
	uint32	usedPrevBlocks = 0;
	
	AllocNode	*memNode = firstUsed;

	// walk the next links
	while(memNode)
	{
		usedNextBlocks++;

		usedMem += memNode->actualSize;

		if (memNode->next != NULL &&
			 memNode >= memNode->next)
		{
			APanic("Used List Next Corrupt!");
		}
		memNode = memNode->next;
	}

	memNode = lastUsed;

	// walk the prev links
	while(memNode)
	{
		usedPrevBlocks++;
		if (memNode->prev != NULL &&
			 memNode <= memNode->prev)
		{
			APanic("Used List Prev Corrupt!");
		}
		memNode = memNode->prev;
	}

	if (usedNextBlocks != usedPrevBlocks)
	{
		APanic("Used Prev and Next blocks not the same!");
	}

	if (usedBlocks != usedNextBlocks)
	{
		APanic("Used blocks different from current count");	
	}

	if ((totalMemPool - availMem) != usedMem)
	{
		APanic("Used memory different from current count");	
	}

	if (numBlocks)
	{
		*numBlocks = usedNextBlocks;	
	}

	return usedMem;
}

uint32
Heap::TotalMem(uint32 *numBlocks)
{
	if (numBlocks)
	{
		*numBlocks = availBlocks + usedBlocks;
	}

	return totalMemPool;
}

// Return total memory by walking the physical links.
// totalMemPool has the value allocated.  This routine is
// actually used to check if things are ok.  If you
// want to know the total blocks, pass the address of a uint32.
uint32
Heap::TestTotalMem(uint32 *numBlocks)
{
	uint32		totalMem;
	AllocNode	*memNode;
	uint32		diff;
	uint32 		totalNextBlocks;
	uint32 		totalPrevBlocks;
	
	// find the first node
	if (firstUsed && firstFree)
	{
		if (firstUsed < firstFree)
		{
			memNode = firstUsed;
		}
		else
		{
			memNode = firstFree;
		}
	}
	else
	{
		if (firstUsed)
		{
			memNode = firstUsed;
		}
		else
		{
			memNode = firstFree;
		}
	}

	// walk the list using next
	totalMem = totalNextBlocks = 0;
	while(memNode)
	{
		totalNextBlocks++;
		if (memNode->physicalNext != NULL)
		{
			// check the size
			diff = (char*) memNode->physicalNext -  (char *) memNode;
			if (diff != memNode->actualSize)
			{
				APanic("Physical next size corrupt");
			}
		}

		if (memNode->physicalNext != NULL &&
			 memNode >= memNode->physicalNext)
		{
			APanic("Physical list next corrupt");
		}

		totalMem += memNode->actualSize;
		memNode = memNode->physicalNext;
	}

	// find the last node
	if (lastUsed && lastFree)
	{
		if (lastUsed > lastFree)
		{
			memNode = lastUsed;
		}
		else
		{
			memNode = lastFree;
		}
	}
	else
	{
		if (lastUsed)
		{
			memNode = lastUsed;
		}
		else
		{
			memNode = lastFree;
		}
	}

	// now check using prev

	totalPrevBlocks = 0;
	while(memNode)
	{
		totalPrevBlocks++;
		if (memNode->physicalNext != NULL)
		{
			diff = (char*) memNode->physicalNext -  (char *) memNode;
			if (diff != memNode->actualSize)
			{
				APanic("Physical prev size corrupt");
			}
		}

		if (memNode->physicalPrev != NULL &&
			 memNode <= memNode->physicalPrev)
		{
			APanic("Physical list prev corrupt");
		}

		memNode = memNode->physicalPrev;
	}

	if (totalNextBlocks != totalPrevBlocks)
	{
		APanic("Total Prev and Next blocks not the same!");
	}

	if (numBlocks)
	{
		*numBlocks = totalNextBlocks;
	}

	return totalMem;
}

// run various checks to make sure that everything is ok.
void
Heap::CheckMem(void)
{
	uint32	availMemory;
	uint32	usedMem;
	uint32	totalMem;
	uint32	numAvailBlocks;
	uint32	usedBlocks;
	uint32 	totalBlocks;

	char		mess[200];

	// walk the next and prev ptrs in the free list
	availMemory = TestAvailMem(&numAvailBlocks); 

	// walk the next and prev ptrs in the used list
	usedMem = TestUsedMem(&usedBlocks);

	if ((availMemory + usedMem) != totalMemPool)
	{
		sprintf(mess,"Total MemPool (%d) doesn't equal Avail (%d) + Used (%d)", totalMemPool, availMemory, usedMem);
		APanic(mess);
	}

	// check the physical links

	// walk to next and prev ptrs in the physical list
	totalMem = TestTotalMem(&totalBlocks);

	if (totalBlocks != (numAvailBlocks + usedBlocks))
	{
		APanic("Physical total block error");
	}

	if ((availMemory + usedMem) != totalMem)
	{
		sprintf(mess,"Total Physical Mem (%d) doesn't equal Avail (%d) + Used (%d)", totalMem, availMemory, usedMem);
		APanic(mess);
	}
}

void
Heap::Dump(void)
{
	AllocNode	*memNode;

	uint32	diff;
	uint32	availMemory;
	uint32	usedMem;
	uint32	totalMem;
	uint32	numAvailBlocks;
	uint32	usedBlocks;
	uint32 	totalBlocks;
	uint32	largestBlock;
	char		mess[200];

	printf("Start Mem Dump\n");

	// walk the next and prev ptrs in the free list
	availMemory = AvailMem(&numAvailBlocks); 

	// walk the next and prev ptrs in the used list
	usedMem = UsedMem(&usedBlocks);

	largestBlock = LargestAvailBlock();

	printf("Total %d  Avail %d  Used %d  Avail Blocks %d  Largest %d\n", totalMemPool, 
	availMemory, usedMem, numAvailBlocks, largestBlock);
	printf("Size Used %d  Used Blocks %d", usedMem, usedBlocks);

	// check the physical links

	// walk to next and prev ptrs in the physical list
	totalMem = TotalMem(&totalBlocks);

	printf("  Total blocks %d\n", totalBlocks);

	if (totalBlocks != (numAvailBlocks + usedBlocks))
	{
		APanic("Physical total block error");
	}

	if ((availMemory + usedMem) != totalMem)
	{
		sprintf(mess,"Total Physical Mem (%d) doesn't equal Avail (%d) + Used (%d)", totalMem, availMemory, usedMem);
		APanic(mess);
	}

	// find the first node
	if (firstUsed && firstFree)
	{
		if (firstUsed < firstFree)
		{
			memNode = firstUsed;
		}
		else
		{
			memNode = firstFree;
		}
	}
	else
	{
		if (firstUsed)
		{
			memNode = firstUsed;
		}
		else
		{
			memNode = firstFree;
		}
	}

	// walk the list using next
	while(memNode)
	{
		if (memNode->physicalNext != NULL)
		{
			// check the size
			diff = (char*) memNode->physicalNext -  (char *) memNode;
			if (diff != memNode->actualSize)
			{
				APanic("Physical next size corrupt");
			}
		}

		if (memNode->physicalNext != NULL &&
			 memNode >= memNode->physicalNext)
		{
			APanic("Physical list next corrupt");
		}

		totalMem += memNode->actualSize;

		if (memNode->size)
		{
			strcpy(mess, "Used");
		}
		else
		{
			strcpy(mess, "Free");
		}
		printf("Address: %d  Size: %d  Type: %s  Grip: %d  Lock %d\n", memNode, memNode->actualSize, mess, memNode->gripNum, memNode->locked);

		memNode = memNode->physicalNext;
	}

	printf("End Mem Dump\n");
}

// Pack Memory!
// For each free block, we move entire blocks to fill them as much as
// possible.  Then the used blocks are "shifted" into the remaining
// free space until a free or locked block is encountered.  If the
// block is a free block, the two free blocks are merged and the
// moving process is repeated.  If the block is locked, the next free
// block is found and the moving process is repeated.  When we
// reach the last free block, we stop.
// To pack only for a required size, pass the size.

void	
Heap::Pack(uint32 requestedSize)
{
	// Pack memory towards the top.

	AllocNode	*freeMergeNode;
	AllocNode	*tempFirstUsed;
	AllocNode	*tempLastFree;
	AllocNode	*prevUsedNode;
	AllocNode	*oldFreeNode;
	AllocNode	*freeNode;
	AllocNode	*usedNode;
	AllocNode	*prevNode;
	AllocNode	*newNode;

	AllocNode	*memNode;
	uint32		ptrOffset;

	if (availBlocks < 2 ||
		 firstUsed == NULL)
	{
		// we either have one block or no blocks.
		// or..we could have no used blocks.
		// no need to pack.
		return;		
	}

	if (requestedSize)
	{
		// add on the size for the AllocNode struct that we be at the
		// front of the allocation.  Also align on a 4 byte boundry.
		requestedSize = (((requestedSize +  sizeof(AllocNode)) + 3) >> 2) << 2;
	}

	// try and move blocks from below to fill holes above
	
	tempLastFree = NULL;
	freeNode = lastFree;
	usedNode = firstUsed;

	while(freeNode && (freeNode > usedNode) && (availBlocks > 1))
	{
		while(usedNode && (usedNode < freeNode))
		{
			// search from the bottom to find used nodes to fill the hole
			if (!usedNode->locked &&
			 	usedNode->actualSize <= freeNode->actualSize)
			{
				// move this node

				prevUsedNode = usedNode->prev;

				// alloc a new used node
				newNode = AddUsedNode(usedNode->size, usedNode->actualSize, ALLOC_HIGH, freeNode);

				// copy from the old to the new
				memcpy(((char *) newNode) + sizeof(AllocNode), ((char *) usedNode) + sizeof(AllocNode), usedNode->size);

				// fix up the grip pointer
				newNode->gripNum = usedNode->gripNum;
				pMemMgr->gripDataArray[newNode->gripNum].ptr = newNode;	

				// delete the old used node
				DelUsedNode(usedNode);

				// fix the free point that we are working with
				oldFreeNode = freeNode;
				if (tempLastFree)
				{
					freeNode = tempLastFree->prev;
				}
				else
				{
					freeNode = lastFree;
				}

				if (freeNode != oldFreeNode)
				{
					// our free node has changed.
					// use the first usedNode again
					prevUsedNode = NULL;
				}

				if (prevUsedNode)
				{
					usedNode = prevUsedNode->next;
				}
				else
				{
					usedNode = firstUsed;
				}

				if (freeNode == firstFree)
				{
					// we are at the first node.  stop.
					break;
				}
			}
			else
			{
				// too big.  look at the next one
				usedNode = usedNode->next;
			}
		}

		if (freeNode == firstFree)
		{
			// we are at the first node.  stop.
			break;
		}

		// we have moved as much as we can into this block.
		// shift things up until we reach another free node or
		// locked block.

		// scan backwards looking for a free node or locked node.
		// if a locked node is found, do no shifting.
		// if a free node is found, fix all the pointers and move
		// the entire area at once.

		memNode = freeNode->physicalPrev;
		while(!memNode->locked && memNode->size)
		{
			memNode = memNode->physicalPrev;
		}

		// since free node should not be locked, we can just check the locked
		// flag.
		if (!memNode->locked)
		{
			// node is a free node.

			// fixup pointers.
			ptrOffset = freeNode->actualSize;

			freeMergeNode = memNode;

			// merge these two free nodes
			memNode->actualSize += ptrOffset;
			memNode->next = freeNode->next;
			if (memNode->next)
			{
				memNode->next->prev = memNode;
			}

			if (freeNode == lastFree)
			{
				lastFree = memNode;
			}

			// point to the first used node
			tempFirstUsed = usedNode = memNode->physicalNext;

			// since all of these nodes will be used nodes (until
			// we reach the free node), the pointers will be the same
			// as the physical.
			while(usedNode != freeNode)
			{
				newNode = (AllocNode *) (((char *) usedNode) + ptrOffset);
				prevNode = (AllocNode *) (((char *) usedNode->prev) + ptrOffset);
				
				// make sure that the first and last pointers are current
				if (usedNode == firstUsed)
				{
					firstUsed = newNode;
				}
				if (usedNode == lastUsed)
				{
					lastUsed = newNode;
				}

				if (usedNode->prev)
				{
					usedNode->prev->next = newNode;

					// if this isn't the first used node, fix the prev ptr
					if (usedNode != tempFirstUsed)
					{
						usedNode->prev = prevNode;
					}
				}

				if (usedNode->physicalPrev)
				{
					usedNode->physicalPrev->physicalNext = newNode;

					// if this isn't the first used node, fix the prev ptr
					if (usedNode != tempFirstUsed)
					{
						usedNode->physicalPrev = prevNode;
					}
				}

				// fix the grip array
				pMemMgr->gripDataArray[usedNode->gripNum].ptr = newNode;	

				usedNode = usedNode->physicalNext;
			}

			// now go back and fix the last used node
			// and remove the free block

			usedNode = freeNode->physicalPrev;

  			// newNode is still valid, so don't recompute it.

			if (usedNode->next)
			{
				usedNode->next->prev = newNode;
			}

			usedNode->physicalNext = freeNode->physicalNext;
			if (usedNode->physicalNext)
			{
				usedNode->physicalNext->physicalPrev = newNode;
			}

			availBlocks--;

			// finally we will "shift" the entire block

			memmove(((char *) tempFirstUsed) + ptrOffset, tempFirstUsed, ((char*) freeNode) - ((char*) tempFirstUsed)); 

			// setup for the next loop.
			// find the free node after the one that we want to look at.
			tempLastFree = freeMergeNode->next;
		}
		else
		{
			// setup for the next loop.
			// find the free node after the one that we want to look at.
			
			if (tempLastFree)
			{
				tempLastFree = tempLastFree->prev;
			}
			else
			{
				tempLastFree = lastFree;
			}
		}

		// find the first free node to look at
		if (tempLastFree)
		{
			freeNode = tempLastFree->prev;
		}
		else
		{
			freeNode = lastFree;
		}

		usedNode = firstUsed;

		if (requestedSize)
		{
			// check to see if we have the requested size now.
			if (LargestAvailBlock() >= requestedSize)
			{
				// we are done.
				break;
			}
		}
	}

	// for now, run a check mem after each pack.
	CheckMem();
}

