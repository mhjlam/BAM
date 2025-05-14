//
// PATHER.CPP
//
// September 21, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	Pather algorithm tester.
//
//----[]-------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pather.hpp"

#ifdef DEBUG_MSGS

void	DisplayPath(PathNode *pData, int32 xSize, int32 ySize);

#endif

#ifndef NOT_USING_TIGRE

// we are using tigre
#include "api.hpp"
#include "memmgr.hpp"
#include "apimem.hpp"

#endif


#define	PATH_FIRST_NODE	254
#define	PATH_LAST_NODE		255

// directions
//
//     9 1 3
//		  \|/
//		 8- -2
//		  /|\
//		12 4 6

// bit oriented
#define	DIR_N	0x1
#define	DIR_E 0x2
#define	DIR_S 0x4
#define	DIR_W 0x8
#define	DIR_NE	(DIR_N | DIR_E)
#define	DIR_NW	(DIR_N | DIR_W)
#define	DIR_SE	(DIR_S | DIR_E)
#define	DIR_SW	(DIR_S | DIR_W)

#define	PATH_USED		10000
#define	PATH_EMPTY		10001
#define	PATH_PRUNED		10002
#define	PATH_BLOCKED	20000
#define	PATH_START		30000

Pather::Pather(void)
{
	// array of offsets, based off of direction for GetFindOffsets.
	int	findOffs[] = {
		DIR_N,  0, -1, -1, -1, 1, -1, -1, 0, 1, 0, -1, 1, 1, 1, 0, 1,
		DIR_E,  1, 0, 1, -1, 1, 1, 0, -1, 0, 1, -1, -1, -1, 1, -1, 0,
		DIR_S,  0, 1, -1, 1, 1, 1, -1, 0, 1, 0, -1, -1, 1, -1, 0, -1,
		DIR_W,  -1, 0, -1, 1, -1, -1, 0, 1, 0, -1, 1, 1, 1, -1, 1, 0,
		DIR_NE, 1, -1, 0, -1, 1, 0, -1, -1, 1, 1, -1, 0, 0, 1, -1, 1,
		DIR_NW, -1, -1, -1, 0, 0, -1, -1, 1, 1, -1, 0, 1, 1, 0, 1, 1,
		DIR_SE, 1, 1, 0, 1, 1, 0, -1, 1, 1, -1, -1, 0, 0, -1, -1, -1,
		DIR_SW, -1, 1, -1, 0, 0, 1, -1, -1, 1, 1, 0, -1, 1, 0, 1, -1
	};

	int	i;
	int	*pOffsets = findOffs;
	int	index;

	gPathData = NULL;
	xSize = ySize = 0;

	// initialize the x and y offsets
	for(i = 0; i < 8; i++)
	{
		index = *pOffsets;
		pOffsets++;
		memcpy(&xyOffsets[index], pOffsets, sizeof(PathOffsets));
		pOffsets += sizeof(PathOffsets)/sizeof(int);
	}
}

Pather::~Pather(void)
{
	Cleanup();
}

void
Pather::Setup(int32 mapXSize, int32 mapYSize, int32 pathArrayMaximum)
{
	// only allow to be setup once
	if (!gPathData ||
		(gPathData && (mapXSize != xSize || mapYSize != ySize)))
	{
		char	mess[100];

		// call this just in case we are reiniting
		Cleanup();
	
		// set the size of the map
		xSize = mapXSize;
		ySize = mapYSize;

		// save the max number of elements to be used in the path array
		// that will be passed with each MakePath call
		pathArrayMax = pathArrayMaximum;

		if (xSize < 1 || ySize < 1)
		{
			sprintf(mess, "Invalid map size (%d, %d)", xSize, ySize);
			APanic(mess);
		}

		pathDataSize = (xSize * ySize) * sizeof(PathNode);
		// allocate twice the amount of memory need so that we can
		// have enough memory to path both directions 
		TRACK_MEM("PatherData");	gPathData = AMalloc(pathDataSize * 2);
	}
}

void
Pather::Cleanup(void)
{
	if (gPathData)
	{
		AFree(gPathData);
		gPathData = NULL;
	}
}

// step count is returned if there is a path
// zero is returned if you are as close as you can be.
int
Pather::MakePath(char *mapData, int32 startX, int32 startY, int32 *destPX, int32 *destPY, int8 *pathArray)
{
	int	i,j;
	char	*pData;
	PathNode *pPathWork;
	int	workX, workY;
	int	destX, destY;
	int	stepCount;
	int	stepCount2 = 0xffff;
	PathNode *pNode;
	int		tempX, tempY;

	destX = *destPX;
	destY = *destPY;

	if (!gPathData)
	{
		APanic("Pather not inited!");
	}

	// get the pointer to our path work area
	pathData = ADerefAs(PathNode, gPathData);

	// setup the pointer for the second path
	pathData2 = (PathNode *) (((char *) pathData) + pathDataSize);

	// init the pathData
	j = xSize * ySize;
	pData = mapData;
	pPathWork = pathData;
	for (i = 0; i < j; i++, pData++, pPathWork++)
 	{
		if (*pData < 1)
		{
			// we can't walk on this
			pPathWork->value = PATH_BLOCKED;
		}
		else
		{
			pPathWork->value = PATH_EMPTY;
			pPathWork->moveRate = *pData;			
		}
	}

	// copy this so that we can path again
	memcpy(pathData2, pathData, pathDataSize);

	// make a path.  workX and workY are pointers, because the destination
	// may be unreachable.  the reachable dest is put in workX and workY
	workX = destX;
	workY = destY;
	stepCount = MakePath(pathData, startX, startY, &workX, &workY);

#ifdef DEBUG_MSGS
	DisplayPath(pathData, xSize, ySize);
	printf("steps = %d\n", stepCount);
#endif

	if (!stepCount)
	{
		// we are as close as we can be.
#ifdef DEBUG_MSGS
		printf("As close as we can be.  Don't path\n");
#endif
		// terminate the path list
		*pathArray = -1;
		return stepCount;
	}

	// prune the path
	stepCount = PrunePath(pathData, 1, startX, startY, stepCount);

#ifdef DEBUG_MSGS
	DisplayPath(pathData, xSize, ySize);
	printf("steps = %d\n", stepCount);

	if (destX == workX && destY == workY)
	{
		printf("Found a Path!\n");
	}
	else
	{
		printf("Modified Destination! (%d, %d)\n", workX, workY);
	}
#endif

	// check to see if this is a good enough path
	if (stepCount > FindNodeDistance(startX, startY, workX, workY))
	{
		// try to find a better path

		// we already know that we can make it there from here, so
		// it's ok to pass points to startX and startY

		destX = startX;
		destY = startY;
		stepCount2 = MakePath(pathData2, workX, workY, &destX, &destY);

#ifdef DEBUG_MSGS
		printf("Look at backwards path.\n");
		DisplayPath(pathData2, xSize, ySize);
		printf("steps = %d\n", stepCount2);
#endif

		// prune the path
		stepCount2 = PrunePath(pathData2, 1, workX, workY, stepCount2);

#ifdef DEBUG_MSGS
		DisplayPath(pathData2, xSize, ySize);
		printf("steps = %d\n", stepCount2);
#endif

	}

	if (stepCount <= stepCount2)
	{
		// use the first path
#ifdef DEBUG_MSGS
		printf("Use first path.\n");			
#endif

		UnkinkPath(pathData, 1, startX, startY, stepCount);

		// copy up to the max steps into the passed pathArray.
		// start at the one past the one we are standing on.
	  	pathDataWork = pathData;
		pNode = GetPathData(startX, startY);
		i = 0;

		while((pNode->nextX != PATH_LAST_NODE) && (i < pathArrayMax))
		{
			tempX = pNode->nextX;
			tempY = pNode->nextY;
			pNode = GetPathData(tempX, tempY);
			*pathArray = (int8) tempX;
			pathArray++;
			*pathArray = (int8) tempY;
			pathArray++;
	
			i++;
		}
	}
	else
	{
		// use the second path
#ifdef DEBUG_MSGS
		printf("Use second path.\n");
#endif

		UnkinkPath(pathData2, 1, workX, workY, stepCount2);

		// copy up to the max steps into the passed pathArray.
		// start at the one past the one we are standing on.
		// go backwards because this path was made going from 
		// dest to start
		pathDataWork = pathData2;
		pNode = GetPathData(startX, startY);
		i = 0;

		while((pNode->prevX != PATH_FIRST_NODE) && (i < pathArrayMax))
		{
			tempX = pNode->prevX;
			tempY = pNode->prevY;
			pNode = GetPathData(tempX, tempY);
			*pathArray = (int8) tempX;
			pathArray++;
			*pathArray = (int8) tempY;
			pathArray++;
	
			i++;
		}

		stepCount = stepCount2;
	}

	// if we didn't fill the array, add a terminator
	if (i < pathArrayMax)
	{
		*pathArray = -1;
	}

	return stepCount;
}

int
Pather::FindNodeDistance(int x, int y, int destX, int destY)
{
	// find the shortest distance between the two points.
	// take walking diagnal into account
	int workX = abs(destX - x);
	int workY = abs(destY - y);
 	int destDist;

	if (workX >= workY)
	{
		destDist = workX;
	}
	else
	{
		destDist = workY;
	}

	return destDist;
}

PathNode*
Pather::GetPathData(int x, int y)
{
	return (pathDataWork + x + (y * xSize));
}

// always add at the end of the linked list
void
Pather::AddPathData(int x, int y, int value, int prevX, int prevY)
{
	PathNode	*curNode = pathDataWork + x + (y * xSize);

	curNode->value = (uint16) value;
	curNode->prevX = (uint8) prevX;
	curNode->prevY = (uint8) prevY;
	curNode->nextX = PATH_LAST_NODE;

	// if this isn't the first node, set the next node in the prev node
	if (prevX != PATH_FIRST_NODE)
	{
		PathNode	*prevNode = pathDataWork + prevX + (prevY * xSize);
		prevNode->nextX = (uint8) x;
		prevNode->nextY = (uint8) y;
	}
}

// this always deletes from the end of a linked list
void
Pather::DelPathData(int x, int y)
{
	PathNode	*curNode = pathDataWork + x + (y * xSize);

	curNode->value = PATH_PRUNED;

	// if this isn't the first node, set the last node in the prev node
	if (curNode->prevX != PATH_FIRST_NODE)
	{
		(pathDataWork + curNode->prevX + (curNode->prevY * xSize))->nextX = PATH_LAST_NODE;
	}
}

// reconstruct a path, moving backwards from the last node.
// prev node will have the proper values already
void
Pather::StepBackPathData(int x, int y, int value)
{
	PathNode	*curNode = pathDataWork + x + (y * xSize);

	curNode->value = (uint16) value;

	// if this isn't the first node, set the next node in the prev node
	if (curNode->prevX != PATH_FIRST_NODE)
	{
		PathNode	*prevNode = pathDataWork + curNode->prevX + (curNode->prevY * xSize);
		prevNode->nextX = (uint8) x;
		prevNode->nextY = (uint8) y;
	}
}

// the first step is 1
// Spiral out in a logical order, attempting to find the closest offsets to
// the destination direction for each step. 
void
Pather::GetFindOffsets(int *xOffset, int *yOffset, int direction, int step)
{
	int	*pOffs;

	pOffs = &xyOffsets[direction].offsets[(step - 1) << 1];

	*xOffset = *pOffs;
	pOffs++;
	*yOffset = *pOffs;
}

// find the direction that it takes to go from begin to end
// if begin and end are the same, 0 is returned
int
Pather::FindDirection(int beginX, int beginY, int endX, int endY)
{
	int	direction = 0;	
	int	workX, workY;
	int	workDiffX, workDiffY;

	workX = endX - beginX;
	workY = endY - beginY;

	// in order give our path a straight line look,
	// don't go diagnal unless the two differences are equal
	
	workDiffX = abs(workX);
	workDiffY = abs(workY);
	if (workDiffX && workDiffY)
	{
		// both x and y are non-zero
		if (workDiffX > workDiffY)
		{
			if (workDiffX%workDiffY)
			{
				workY = 0;
			}
		}
		else
		{
			if (workDiffY > workDiffX)
			{
				if (workDiffY%workDiffX)
				{
					workX = 0;
				}
			}
		}
	}

	if (workX)
	{
		if (workX > 0)
		{
			direction |= DIR_E;
		}
		else
		{
			direction |= DIR_W;
		}
	}
	if (workY)
	{
		if (workY > 0)
		{
			direction |= DIR_S;
		}
		else
		{
			direction |= DIR_N;
		}
	}
	return direction;
}

int
Pather::PrunePath(PathNode *pathData, int level, int startX, int startY, int stepCount)
{
	PathNode *pNode;
	PathNode *stepNode;
	int		tempX, tempY;
	int		searchCount;
	int		searchCountMax = level * 8;
	int		xOff, yOff;
	uint16	bestStep;
	int		bestStepX, bestStepY;
	int		stepX, stepY;
	int		oldX, oldY;
	int		minOff = level * -1;
	int		maxOff = level;
	int		curDir;
	uint16	nextStep;
	uint16	curStep;

	// set the pathDataWork member variable.  we will now be working on this
	pathDataWork = pathData;

	tempX = startX;
	tempY = startY;
	pNode = GetPathData(tempX, tempY);
	curStep = 0;

	while(pNode->nextX != PATH_LAST_NODE)
	{
		searchCount = 0;
		xOff = yOff = level;
		curDir = 0;
		oldX = tempX;
		oldY = tempY;
		bestStepX = pNode->nextX;
 		bestStepY = pNode->nextY;
		nextStep = bestStep = GetPathData(pNode->nextX, pNode->nextY)->value;

		// start at the lower right corner and check all positions in
		// this level in a circular manner.
		while(searchCount < searchCountMax)
		{
	
			searchCount++;
			
			stepX = tempX + xOff; 
			stepY = tempY + yOff; 

			// only check if this is inbounds
			if (stepX >= 0 &&
			 	stepX < xSize &&
			 	stepY >= 0 &&
			 	stepY < ySize)
			{
				// this is in bounds	
				stepNode = GetPathData(stepX, stepY);

				if (stepNode->value < PATH_EMPTY &&
					 stepNode->value > bestStep)
				{
//					// this one was used and is further along.
//					// check to see if we can get to this one.
//					for (i = 1; i < level; i++)
//					{
//						
//					}

					// we can use this
					bestStep = stepNode->value;
					bestStepX = stepX;
					bestStepY = stepY;
				}
			}

			// setup the offsets
			switch(curDir)
			{
				case 0:
					// right to left, bottom row
					xOff--;
					if (xOff == minOff)
					{
						curDir++;
					}
					break;

				case 1:
					// bottom to top, left col
					yOff--;
					if (yOff == minOff)
					{
						curDir++;
					}
					break;

				case 2:
					// left to right, top row
					xOff++;
					if (xOff == maxOff)
					{
						curDir++;
					}
					break;

				case 3:
					// top to bottom, right col
					yOff++;
			}
		}

		// NOTE...THIS ONLY WORKS ONLY ON A LEVEL 1

		// see if we found a better step
		if (nextStep != bestStep)
		{
			// we have a better step
			stepNode = pNode;

			do
			{
				stepNode = GetPathData(stepNode->nextX, stepNode->nextY);
				stepNode->value = PATH_PRUNED;
				stepCount--;
			} while (!(stepNode->nextX == bestStepX && stepNode->nextY == bestStepY));

			pNode->nextX = (uint8) bestStepX;
			pNode->nextY = (uint8) bestStepY;
		}

		tempX = pNode->nextX;
		tempY = pNode->nextY;
		pNode = GetPathData(tempX, tempY);

		pNode->prevX = (uint8) oldX;
		pNode->prevY = (uint8) oldY;
		curStep++;
		pNode->value = curStep;
	}

	return stepCount;
}

int
Pather::MakePath(PathNode *pathData, int startX, int startY, int *destPX, int *destPY)
{
   int	workX, workY;
	int	workDestX, workDestY;
	int	xOff, yOff;
	int	oldX, oldY;
	int	workDir;
	int	tempX,tempY;
	uint16	pathChar;
	bool	pruneable;
	PathNode *curPathNode;
	int	origDist;
	int	closestX, closestY;
	int	closestDist;
	int	closestSteps;
	int	tempDist;
	int	i;
	bool	dontMove = FALSE;
	int	tempCnt;
	PathNode *pNode;
	int	stepCount;

	// set the pathDataWork member variable.  we will now be working on this
	pathDataWork = pathData;

	workDestX = *destPX;
	workDestY = *destPY;

	closestX = startX;
	closestY = startY;
	origDist = closestDist = FindNodeDistance(startX, startY, workDestX, workDestY);
	closestSteps = 10000;

#ifdef DEBUG_MSGS
	printf("Start X = %d, Y = %d\n", startX, startY);
	printf("Dest  X = %d, Y = %d\n", workDestX, workDestY);
	printf("Distance = %d\n", closestDist);

	printf("Begin Path\n");
#endif

	// do the path
	workX = startX;
	workY = startY;
	AddPathData(workX, workY, PATH_START, PATH_FIRST_NODE, PATH_FIRST_NODE);
#ifdef DEBUG_MSGS
	printf("%3d,%3d\n", workX, workY);
#endif
	stepCount = 0;

	while(TRUE)
	{
		oldX = workX;
		oldY = workY;
		workDir = FindDirection(oldX, oldY, workDestX, workDestY);

		if (dontMove == TRUE || workDir == 0)
		{
#ifdef DEBUG_MSGS
			printf("End Path\n");
#endif
			break;
		}

		curPathNode = GetPathData(oldX, oldY);

		pruneable = FALSE;

		int tempXOff, tempYOff;
		int tempMoveValue = 0;
		int moveValue;

		for(i = 1; i < 9; i++)
		{
			GetFindOffsets(&xOff, &yOff, workDir, i);
			tempX = workX + xOff;
			tempY = workY + yOff;

			// only check if this is inbounds
			if (tempX >= 0 &&
			 	tempX < xSize &&
			 	tempY >= 0 &&
			 	tempY < ySize)
			{
				// this is in bounds	
				pNode = GetPathData(tempX, tempY);
				pathChar = pNode->value;
				moveValue = pNode->moveRate;
				if (pathChar < PATH_EMPTY ||
					 pathChar == PATH_START)
				{
					pruneable = TRUE;
				}
				else
				{
					if (pathChar == PATH_EMPTY)
					{
						// we found one.
						// we take the best step of the first 3.
						if (moveValue > tempMoveValue)
 						{
							tempXOff = xOff;
							tempYOff = yOff;
							tempMoveValue = moveValue;
						}
					}
				}
			}

			if (i > 2 && tempMoveValue)
			{
				// we have found one
				break;
			}
		}

		if (i == 9)
		{
			if (pruneable == FALSE)
			{
				// we didn't find a way to move
				dontMove = TRUE;
			}
			else
			{
				// since we didn't find anything, back up onto the
				// the one that we were on before, and prune this one

				// this may be as close as we can get.
				// save some info just in case.
				tempDist = FindNodeDistance(workX, workY, workDestX, workDestY);
				if (tempDist < closestDist ||
					(tempDist == closestDist &&
					 stepCount < closestSteps))
				{
					closestX = workX;
					closestY = workY;
					closestDist = tempDist;
					closestSteps = stepCount;
				}

				DelPathData(workX, workY);
				workX = curPathNode->prevX;
				workY = curPathNode->prevY;
				stepCount--;
			}
		}
		else
		{
			workX += tempXOff;			
			workY += tempYOff;
			stepCount++;
			if (GetPathData(workX, workY)->value != PATH_START)
			{
				AddPathData(workX, workY, stepCount, oldX, oldY);
			}
		}

#ifdef DEBUG_MSGS
		if (dontMove == FALSE)
		{
			printf("%3d,%3d  step %d\n", workX, workY, stepCount);
		}
#endif
	}

	if (workDestX != workX || workDestY != workY)
	{
		// we couldn't find a path

		// if we are closer or as close as we would be if
		// we pathed, don't path
		if (origDist <= FindNodeDistance(closestX, closestY, workDestX, workDestY))
		{
			// we are as close as we can be
			stepCount = 0;
		}
		else
		{
			// let the caller know our new destination
			*destPX = closestX;
			*destPY = closestY;

			// reconstruct the path that was closest
			tempX = closestX;
			tempY = closestY;
			stepCount = tempCnt = closestSteps;
			pNode = GetPathData(tempX, tempY);
			while(pNode->value != PATH_START)
			{
				StepBackPathData(tempX, tempY, tempCnt);
				tempCnt--;
				tempX = pNode->prevX;
				tempY = pNode->prevY;
				pNode = GetPathData(tempX, tempY);
			}
		}
	}

	return stepCount;
}

// this will take the diagonal movements out in places
// were a straight line also works.  Note - this only looks
// at the next two nodes, so it only takes out small kinks.
void
Pather::UnkinkPath(PathNode *pathData, int level, int startX, int startY, int stepCount)
{
	// eliminate warning
	level = level;

	if (stepCount < 2)
	{
		// nothing to unkink
		return;
	}

	PathNode *pNode;
	PathNode *pNextNode;
	PathNode *pTestNode;
	int		curX, curY;
	int		nextX, nextY;
	int		workDir;

	// set the pathDataWork member variable.  we will now be working on this
	pathDataWork = pathData;

	curX = startX;
	curY = startY;
	pNode = GetPathData(curX, curY);
	pNextNode = GetPathData(pNode->nextX, pNode->nextY);

	while(pNextNode->nextX != PATH_LAST_NODE)
	{
		// find the direction that it takes to get to
		// the node to the node after the next node
		workDir = FindDirection(curX, curY, pNextNode->nextX, pNextNode->nextY);

		nextX = nextY = 0;
		// we only unkink these directions
		switch(workDir)
		{
			case DIR_N:
				nextY = -1;
				break;

			case DIR_S:
				nextY = 1;
				break;

			case DIR_E:
				nextX = 1;
				break;

			case DIR_W:
				nextX = -1;
				break;
		}

		// if nextX or nextY are non-zero, we should are going N,S,E, or W
		if (nextX || nextY)
		{
			// get the coordinates
			nextX += curX;
			nextY += curY;

			if (nextX != pNode->nextX ||
				 nextY != pNode->nextY)
			{
				// we aren't already using this node.  check it.
				pTestNode = GetPathData(nextX, nextY);

				if (pTestNode->value != PATH_BLOCKED &&
					 pTestNode->moveRate >= pNextNode->moveRate)
				{
					// we can walk here and it's a better looking place.
					// make this the next node.
					pNode->nextX = (uint8) nextX;
					pNode->nextY = (uint8) nextY;
					pTestNode->prevX = (uint8) curX;
					pTestNode->prevY = (uint8) curY;
					pTestNode->nextX = pNextNode->nextX;
					pTestNode->nextY = pNextNode->nextY;
					pTestNode->value = pNextNode->value;
					pNextNode->value = PATH_PRUNED;
					pNextNode = pTestNode;
				}
			}
		}

		curX = pNode->nextX;
		curY = pNode->nextY;
		pNode = pNextNode;
		pNextNode = GetPathData(pNode->nextX, pNode->nextY);
	}
}

#ifdef DEBUG_MSGS

void
DisplayPath(PathNode *pData, int32 xSize, int32 ySize)
{
	int	i,j;

	printf("    ");
	for (i = 0; i < xSize; i++)
	{
		printf("%d", i%10);	
	}
	printf("\n\n");

	if (pData && xSize > 0 && ySize > 0)
	{
		for (j = 0; j < ySize; j++)
		{
			printf("%03d ", j);	
			for (i = 0; i < xSize; i++)
			{
				switch(pData->value)
				{
					case PATH_BLOCKED:
						printf("1");
						break;

					case PATH_PRUNED:
//						printf("p");
						printf(" ");
						break;

					case PATH_START:
						printf("S");
						break;

					case PATH_EMPTY:
						printf(" ");
						break;

					// path used is now the path step count
//					case PATH_USED:
					default:
						printf("*");
						break;

				}
				pData++;
			}
			printf("\n");
		}
	}
}

#endif


// NOTE: I have left his old GetFindOffsets code here in case we ever
// need to do level 2 or higher again.  It is good code and should be left
// here.
//
//// the first step is 1
//// Find the offsets.  for each level, there are 8 * level possible offsets.
//// Spiral out in a logical order, attempting to find the closest offsets to
//// the destination direction for each step. 
//void
//Pather::GetFindOffsets(int *xOffset, int *yOffset, int direction, int level, int step)
//{
//	int	xOff, yOff;
//
//	// find initial offsets for this level
//
//	if (direction & DIR_W)
//	{
//		xOff = -1;
//	}
//	else
//	{
//		if (direction & DIR_E)
//		{
//			xOff = 1;
//		}
//		else
//		{
//			xOff = 0;
//		}	
//	}
//
//	if (direction & DIR_N)
//	{
//		yOff = -1;
//	}
//	else
//	{
//		if (direction & DIR_S)
//		{
//			yOff = 1;
//		}
//		else
//		{
//			yOff = 0;
//		}
//	}
//
//	xOff *= level;
//	yOff *= level;
//
//	int searchCountMax = level * 8;
//
//	switch(direction)
//	{
//		case DIR_NW:
//			if (step & 1) 
//			{
//				// odd
//				if (step > (searchCountMax/2))
//				{
//					// east side		
//					xOff += (level * 2);
//					yOff += (step - (searchCountMax/2))/2;
//				}
//				else
//				{
//					// north side
//					xOff += step/2;
//					// yOff unchanged
//				}
//			}
//			else
//			{
//				// even
//				if (step > (searchCountMax/2))
//				{
//					// south side		
//					xOff += (step - (searchCountMax/2))/2;
//					yOff += (level * 2);
//				}
//				else
//				{
//					// west side
//					// xOff unchanged
//					yOff += step/2;
//				}
//			}
//			break;
//		
//		case DIR_NE:
//			if (step & 1) 
//			{
//				// odd
//				if (step > (searchCountMax/2))
//				{
//					// west side		
//					xOff -= (level * 2);
//					yOff += (step - (searchCountMax/2))/2;
//				}
//				else
//				{
//					// north side
//					xOff -= step/2;
//					// yOff unchanged
//				}
//			}
//			else
//			{
//				// even
//				if (step > (searchCountMax/2))
//				{
//					// south side		
//					xOff -= (step - (searchCountMax/2))/2;
//					yOff += (level * 2);
//				}
//				else
//				{
//					// east side
//					// xOff unchanged
//					yOff += step/2;
//				}
//			}
//			break;
//		
//		case DIR_SE:
//			
//			if (step & 1) 
//			{
//				// odd
//				if (step > (searchCountMax/2))
//				{
//					// west side		
//					xOff -= (level * 2);
//					yOff -= (step - (searchCountMax/2))/2;
//				}
//				else
//				{
//					// south side
//					xOff -= step/2;
//					// yOff unchanged
//				}
//			}
//			else
//			{
//				// even
//				if (step > (searchCountMax/2))
//				{
//					// north side		
//					xOff -= (step - (searchCountMax/2))/2;
//					yOff -= (level * 2);
//				}
//				else
//				{
//					// east side
//					// xOff unchanged
//					yOff -= step/2;
//				}
//			}
//			break;
//
//		case DIR_SW:
//			
//			if (step & 1) 
//			{
//				// odd
//				if (step > (searchCountMax/2))
//				{
//					// east side		
//					xOff += (level * 2);
//					yOff -= (step - (searchCountMax/2))/2;
//				}
//				else
//				{
//					// south side
//					xOff += step/2;
//					// yOff unchanged
//				}
//			}
//			else
//			{
//				// even
//				if (step > (searchCountMax/2))
//				{
//					// north side		
//					xOff += (step - (searchCountMax/2))/2;
//					yOff -= (level * 2);
//				}
//				else
//				{
//					// west side
//					// xOff unchanged
//					yOff -= step/2;
//				}
//			}
//			break;
//
//		case DIR_N:
//
//			if (step < (searchCountMax/3))
//			{
//				// north side
//
//				// yOff unchanged
//				if (step & 1)
//				{
//					// right side
//					xOff += step/2;
//				}
//				else
//				{
//					// left side
//					xOff -= step/2;
//				}
//			
//			}
//			else
//			{
//				if (step < ((searchCountMax/4) * 3))
//				{
//					yOff += step/2 - level;
//					if (step & 1)
//					{
//						// east side
//						xOff += level;
//					}
//					else
//					{
//						// west side
//						xOff -= level;
//					}
//			
//				}
//				else
//				{
//					// south side
//					yOff += (level * 2);
//
//					if (step & 1)
//					{
//						// right side
//						xOff += (abs(searchCountMax - step) + 1)/2;
//					}
//					else
//					{
//						// left side
//						xOff -= abs(searchCountMax - step)/2;
//					}
//				}
//			}
//			break;
//
//		case DIR_S:
//
//			if (step < (searchCountMax/3))
//			{
//				// south side
//
//				// yOff unchanged
//				if (step & 1)
//				{
//					// left side
//					xOff -= step/2;
//				}
//				else
//				{
//					// right side
//					xOff += step/2;
//				}
//			
//			}
//			else
//			{
//				if (step < ((searchCountMax/4) * 3))
//				{
//					yOff -= step/2 - level;
//					if (step & 1)
//					{
//						// west side
//						xOff -= level;
//					}
//					else
//					{
//						// east side
//						xOff += level;
//					}
//			
//				}
//				else
//				{
//					// north side
//					yOff -= (level * 2);
//
//					if (step & 1)
//					{
//						// left side
//						xOff -= (abs(searchCountMax - step) + 1)/2;
//					}
//					else
//					{
//						// right side
//						xOff += abs(searchCountMax - step)/2;
//					}
//				}
//			}
//			break;
//
//		case DIR_E:
//
//			if (step < (searchCountMax/3))
//			{
//				// east side
//
//				// xOff unchanged
//				if (step & 1)
//				{
//					// bottom side
//					yOff += step/2;
//				}
//				else
//				{
//					// top side
//					yOff -= step/2;
//				}
//			
//			}
//			else
//			{
//				if (step < ((searchCountMax/4) * 3))
//				{
//					xOff -= step/2 - level;
//					if (step & 1)
//					{
//						// south side
//						yOff += level;
//					}
//					else
//					{
//						// north side
//						yOff -= level;
//					}
//			
//				}
//				else
//				{
//					// west side
//					xOff -= (level * 2);
//
//					if (step & 1)
//					{
//						// bottom side
//						yOff += (abs(searchCountMax - step) + 1)/2;
//					}
//					else
//					{
//						// top side
//						yOff -= abs(searchCountMax - step)/2;
//					}
//				}
//			}
//			break;
//
//		case DIR_W:
//
//			if (step < (searchCountMax/3))
//			{
//				// west side
//
//				// xOff unchanged
//				if (step & 1)
//				{
//					// top side
//					yOff -= step/2;
//				}
//				else
//				{
//					// bottom side
//					yOff += step/2;
//				}
//			
//			}
//			else
//			{
//				if (step < ((searchCountMax/4) * 3))
//				{
//					xOff += step/2 - level;
//					if (step & 1)
//					{
//						// north side
//						yOff -= level;
//					}
//					else
//					{
//						// south side
//						yOff += level;
//					}
//			
//				}
//				else
//				{
//					// east side
//					xOff += (level * 2);
//
//					if (step & 1)
//					{
//						// top side
//						yOff -= (abs(searchCountMax - step) + 1)/2;
//					}
//					else
//					{
//						// bottom side
//						yOff += abs(searchCountMax - step)/2;
//					}
//				}
//			}
//			break;
//	}
//
//	*xOffset = xOff;
//	*yOffset = yOff;
//}
