//
// APIGRAPH.HPP
//
// November 12, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Definitions for API calls specific to the graphics manager.
//
//----[]-------------------------------------------------------------


#ifndef	apigraph_hpp
#define	apigraph_hpp


#include "graphmgr.hpp"
#include "palette.hpp"
#include "scrimage.hpp"
#include "types.hpp"


const	uchar		SKIP_BYTE = 254;
#define			MAX_H_RES	640			// BUGBUG: should this be 320 in XMode?


//---[ CopyPixels ]--------------------------------------------------

#define	CP_FORWARD	1
#define	CP_BACKWARD	(-1)

const int	CP_NORMAL	= 0x00;
const int	CP_SCALED	= 0x01;
const int	CP_TRLE	 	= 0x02;
const int	CP_NOSKIP 	= 0x04;
const int	CP_SRLE	 	= 0x08;
const int	CP_VREVERSE	= 0x10;
const int	CP_HREVERSE	= 0x20;
const int	CP_EXTENDED = 0x80;

// This structure is used to pass information to ACopyPixels
struct	RCopyPixels
{
	int			_flags; 				
	void*			_vpRectFillArea;	// rect of fill area; relative to vbuf area
	int			_vbufWidth;			// width of vbuf area
	coord			_celX; 				// left row of fill area; relative to the res cel rect
	coord			_celY; 				// top row of fill area; relative to the res cel rect
	coord			_celWidth;			// width of res cel data
	uchar*		_pCelData;			// points to first pixel(or ScanTable) of cel

	uchar*		_pBufData;			// points to first pixel of vbuf area
	uchar*		_pDst;				// points to first pixel in fill area

	Scrimage*	_pScrim;				// points to scrimage being drawn
	uint			_scale;				// scaling factor (if different from pScrim)
};

// This structure is an extended version of RCopyPixels and is used to
// pass scrimage-related information without requiring a scrimage
struct	RCopyPixelsEx : RCopyPixels
{
	uchar*		_clut;
	uint			_clutIdx;
	uint			_clutSize;
};

// This structure is used to pass information to the assembly CopyPixels
struct	ASMCopyPixels
{
	uchar*		_pSrcData;			// points to source buffer
	uchar*		_pDstData;			// points to destination buffer
	uchar*		_clut;				// points to CLUT to use (or NULL if no CLUT)
	uint			_clutIdx;			// starting palette register of CLUT
	uint			_clutSize;			// number of entries in CLUT
	coord			_srcWidth;			// width of source buffer
	uint			_srcOffset;			// offset to first pixel in row to copy
	uint			_dstOffset;			// offset to first pixel in row to copy
	int			_hdir;				// horz direction (1 = forward, -1 = backward)
	bool			_doSkip;				// TRUE if skip should be respected
	bool			_srle;				// TRUE if source is SRLE compressed
};


// OS-Abstracted graphics functions

#ifdef OS_DOS
// In Assembly in DOS/TIGRE
extern "C"
{
#endif
	int 	OS_GetScreenMode();
	void	OS_SetPalette(Gun* gunsArray, uint startGun, uint endGun);
	void	OS_GetPalette(Gun* gunsArray, uint startGun, uint endGun);
#ifdef OS_DOS
}
#endif


void	OS_CopyPixelLine_C(ASMCopyPixels* acp);

// This allows us to use either the optimized-for-platform or portable
// versions of OS_CopyPixelLine()
//

#ifdef OS_DOS
	extern "C"
	{
		// DOS has optimized version in assembly
		void	OS_CopyPixelLine_ASM(ASMCopyPixels* acp);
	}
	#define	OS_CopyPixelLine	OS_CopyPixelLine_ASM
#else
	// other platforms will use portable C version
	#define	OS_CopyPixelLine	OS_CopyPixelLine_C
#endif


// Scrimage functions
void		AAddScrim(Scrimage* pScrim);
void		AChangeScrim(Scrimage* pScrim);
void		ADeleteScrim(Scrimage* pScrim);

// GraphMgr Property access
coord		AMaxX();
coord		AMaxY();

// Graphics functions
void		AAnimate();
bitmap_t	ACreateBitMap(coord w, coord h);
void		ADestroyBitMap(bitmap_t bm);
uchar*	ADerefBitMap(bitmap_t bm);
void		AUpdateRect(Rectangle* rect);
void		AUpdateRect(TClipRect* rect);
void		ACopyPixels(RCopyPixels* p);
void		AShutDownVideo();
void		ASetPalette(Gun* gunsArray, uint startGun, uint endGun);
void		AGetPalette(Gun* gunsArray, uint startGun, uint endGun);


// These functions are defined differently based on display device, driver
//	or resolutions. For XMode, they are defined in XModDisp.cpp. For
// Super vga, high res, they are defined in svgaDisp.cpp
//
int		AInitGraphics(GraphicsMgr* pGM, uint mode = 0);
int		ADetectVGA();
bool		AInitVideo(uint driver, uint mode, uint& vSeg,
							coord& vWide, coord& vHigh, uint& bankInc, uint& oldMode);

#ifdef OS_DOS
// In Assembly in DOS/TIGRE (modex.asm)
extern "C"
{
#endif
void		ABlit(uint driver, uchar* pData, coord x, coord y,
					uint bufWidth, uint bufHeight, uint bufSpan, uint vSeg);
void		ARBlit(uint driver, uchar* pData, coord x, coord y,
					uint bufWidth, uint bufHeight, uint vSeg);
#ifdef OS_DOS
}
#endif


#endif

