//
// PATHER.HPP
//
// September 21, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	Pather class
//
//----[]-------------------------------------------------------------

#ifndef PATHER_HPP
#define PATHER_HPP

typedef char 				int8;
typedef unsigned char 	uint8;

//***************************** NOTE *********************************
//
// These are defined in tigre.  If you are compiling not using
// tigre, define NOT_USING_TIGRE (in the makefile).
//
//********************************************************************

#ifdef NOT_USING_TIGRE

typedef unsigned int 	uint32;
typedef int					int32;
typedef unsigned short 	uint16;

typedef	unsigned char	bool;
#define	TRUE		1
#define	FALSE		0

typedef	void*				grip;

class	MemMgrStub
{
	public:
		void*	Malloc(int size);
		void	Free(grip ptr);
};

#define	AMalloc(size)		pMemMgr->Malloc(size)
#define	AFree(ptr)			pMemMgr->Free(ptr)
#define	TRACK_MEM(x)		

extern	MemMgrStub	*pMemMgr;

#define	ADerefAs(t,x)			((t *)(x))

void		_Panic(char* msg, char* fileName, int lineNum);
#define	APanic(msg)	( (_Panic(msg,__FILE__,__LINE__)))

#define ASSERT(e)	((void)((e) || (_Panic(#e,__FILE__,__LINE__),1)))

#else
#include "tigre.hpp"
#endif

//************************ END TIGRE STUFF****************************


#pragma SC align 1

typedef struct
{
	uint8		prevX, prevY;			
	uint8		nextX, nextY; 
	uint16	value;  
	int8		moveRate;	
} PathNode;

#pragma SC align 

typedef struct
{
	int offsets [8 * 2];	
} PathOffsets;

class Pather
{
	public:
		Pather(void);
		~Pather(void);

		void	Setup(int32 mapXSize, int32 mapYSize, int32 pathArrayMaximum);
		void	Cleanup(void);
		int	MakePath(char *mapData, int32 startX, int32 startY, int32 *destPX, int32 *destPY, int8 *pathArray);

	protected:
		grip		gPathData;
		PathNode	*pathData;
		PathNode	*pathData2;
		PathNode	*pathDataWork;
		int32		xSize, ySize;
		int32		pathDataSize;
		int32		pathArrayMax;

		// this is the setup to handle the largest value of the directions,
		// which will be used to index into this array
		PathOffsets	xyOffsets[13];

		int	FindNodeDistance(int x, int y, int destX, int destY);
		void	GetFindOffsets(int *xOffset, int *yOffset, int direction, int step);
		int	FindDirection(int beginX, int beginY, int endX, int endY);
		int	PrunePath(PathNode *pathData, int level, int startX, int startY, int stepCount);
		void	UnkinkPath(PathNode *pathData, int level, int startX, int startY, int stepCount);
		int	MakePath(PathNode *pathData, int startX, int startY, int *destPX, int *destPY);

		PathNode*	GetPathData(int x, int y);
		void			AddPathData(int x, int y, int value, int prevX, int prevY);
		void			DelPathData(int x, int y);
		void			StepBackPathData(int x, int y, int value);
};

#endif

