//
//	OSGRAPH.HPP
//
// March 2, 1994
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	DOS Platform specific functions, defines, relating to graphics system
//
//----[]-------------------------------------------------------------


#ifndef	osgraph_hpp
#define	osgraph_hpp

#include	<stddef.h>

#include "graphmgr.hpp"
#include	"types.hpp"



//----[ Graphics ]------------------------------------------------

int		OS_InitGraphics(GraphicsMgr* pGM, uint mode = 0);
//void		OS_Blit(void* pVGABuffer, bitmap_t gBitMapdata);
bitmap_t	OS_CreateBitMap(coord w, coord h);
void		OS_DestroyBitMap(bitmap_t bm);
uchar*	OS_DerefBitMap(bitmap_t bm);


//----[ Copy Pixels ]---------------------------------------------

const int	CP_NORMAL	= 0x00;
const int	CP_SCALED	= 0x01;
const int	CP_RLE	 	= 0x02;
const int	CP_NOSKIP 	= 0x04;

struct	RCopyPixels
{
	int		_flags; 				
	void*		_vpRectFillArea;	// rect of fill area; relative to vbuf area
	int		_vbufWidth;			// width of vbuf area
	coord		_celX; 				// left row of fill area; relative to the res cel rect
	coord		_celY; 				// top row of fill area; relative to the res cel rect
	coord		_celWidth;			// width of res cel data
	uchar*	_pCelData;			// points to first pixel(or ScanTable) of cel

	uchar*	_pBufData;			// points to first pixel of vbuf area
	uchar*	_pDst;					// points to first pixel in fill area

	uint		_scale;
};

// this function executes the tightest loop of building the blit rects
// for vgabuffer. It is broken out as an OS functions to allow platform
// specific optimizations.

void		OS_CopyPixels(RCopyPixels* p, bool turnInterruptsOff = TRUE);


//----[ Misc OS Functions ]---------------------------------------

void 		OS_InitializePlatform();
void		OS_SetPalette(char* gunsArrays, uint startGun, uint endGun);
void		OS_GetPalette(char* gunsArrays, uint startGun, uint endGun);

#endif
