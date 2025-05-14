//
// APIGRAPH.CPP
//
// November 12, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Graphics manager specific, resolution-independent functions
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "graphmgr.hpp"
#include "os.hpp"
#include "srle.hpp"
#include "trle.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)



// Scrimage functions

void
AAddScrim(Scrimage* pScrim)
{
	pGraphMgr->AddScrimage(pScrim);
}


void
AChangeScrim(Scrimage* pScrim)
{
	pGraphMgr->ChangeScrimage(pScrim);
}


void
ADeleteScrim(Scrimage* pScrim)
{
	pGraphMgr->DeleteScrimage(pScrim);
}



// GraphMgr Property access

coord
AMaxX()
{
	return pGraphMgr->maxX;
}


coord
AMaxY()
{
	return pGraphMgr->maxY;
}


// Graphics Functions

void
AAnimate()
{
	pGraphMgr->Animate();
}


void	
AUpdateRect(Rectangle* rect)
{
	pGraphMgr->UpdateRect(rect);	
}


void	
AUpdateRect(TClipRect* rect)
{
	pGraphMgr->UpdateRect(rect);	
}


//---[ Palette Routines ]-----------------------------------------

void
ASetPalette(Gun* gunsArray, uint startGun, uint endGun)
{
	OS_SetPalette(gunsArray, startGun, endGun);
}


void
AGetPalette(Gun* gunsArray, uint startGun, uint endGun)
{
	OS_GetPalette(gunsArray, startGun, endGun);
}


//---[ CopyPixels Routines ]-----------------------------------------

// The CopyPixels function set consists of 6 parts:  The API entry
// point, ACopyPixels, and 1 private static function for each type
// of copy:
//
//		Normal 		- source cel is uncompressed
//		Extended		- as Normal, but with extended structure
//		Scaled 		- source cel is scaled
//		TRLE   		- source cel is TRLE compressed
//		TRLEScaled	- source cel is TRLE compressed and scaled
//		SRLE			- source cel is SRLE compressed
//
// Note: Copy functions that perform scaling or decompressing will
// 		be run as a pre-process in VGABuffer::Load.  The resulting
//			destination buffer will then be reused as an uncompressed
//			source cel for the final copy, which will always be the
//			CopyPixelsNormal function.

static void	CopyPixelsNormal(RCopyPixels* p, ASMCopyPixels* pAcp = NULL);
static void	CopyPixelsExtended(RCopyPixelsEx* p);
static void	CopyPixelsScaledExtended(RCopyPixelsEx* p);
static void	CopyPixelsScaledTRLE(RCopyPixels* p);
static void	CopyPixelsTRLE(RCopyPixels* p);
static void	CopyPixelsSRLE(RCopyPixels* p);
static void	CopyPixelsScaled(RCopyPixels* p, ASMCopyPixels* pAcp = NULL);

void
ACopyPixels(RCopyPixels* p)
{
	switch (p->_flags & (CP_SCALED | CP_TRLE | CP_SRLE | CP_EXTENDED))
	{
		case CP_SCALED:
			// Source cel is uncompressed & scaled
			CopyPixelsScaled(p);
			break;

		case CP_SCALED | CP_EXTENDED:
			// Source cel is uncompressed & scaled w/ cluts
			CopyPixelsScaledExtended((RCopyPixelsEx*) p);
			break;

		case CP_SCALED | CP_TRLE:
			// Source cel is TRLE compressed & scaled
			CopyPixelsScaledTRLE (p);
			break;

		case CP_TRLE:
			// Source cel is TRLE compressed
			CopyPixelsTRLE(p);
			break;

		case CP_SRLE:
			// Source cel is SRLE compressed
			CopyPixelsSRLE (p);
			break;

		case CP_EXTENDED:
			// Use the extended RCopyPixels structure
			CopyPixelsExtended((RCopyPixelsEx*) p);
			break;

		default:
			// Source cel is uncompressed and unscaled
			CopyPixelsNormal(p);
	}
}


static void
CopyPixelsNormal (RCopyPixels* p, ASMCopyPixels* pAcp)
{
	ASMCopyPixels		ASMcp;
	register uchar*	pBufData;
	register uchar*	pCelData;
	register uint32	celIndex;
	register uint32	bufIndex;			// index from pBufData to pDst
	int					row;
	int					rWidth;
	int					vbufWidth;
	int					celWidth;
	int					hdir, vdir;			// directions to copy

	//bool					dynamicAcp = FALSE;

	Rectangle*	pR = (Rectangle*) p->_vpRectFillArea;

	rWidth = pR->Wide();

	// use register/local variables
	pBufData = p->_pBufData;
	pCelData = p->_pCelData;
	vbufWidth = p->_vbufWidth;
	celWidth = p->_celWidth;
	hdir = (p->_flags & CP_HREVERSE) ? CP_BACKWARD : CP_FORWARD;
	vdir = (p->_flags & CP_VREVERSE) ? CP_BACKWARD : CP_FORWARD;

	celIndex = p->_celY * p->_celWidth + p->_celX;
	if (vdir == -1)
	{
		// Set the buffer index to the last line
		bufIndex = pR->y2 * vbufWidth + pR->x1;
	}
	else
	{
		// Set the buffer index to the first line
		bufIndex = pR->y1 * vbufWidth + pR->x1;
	}

	// Set up the ASMCopyPixels structure
	if (!pAcp)
	{
		// We were not passed an ACP structure, so we have to create one.
		// Set a flag so we can delete it later.

		//dynamicAcp = TRUE;
		//TRACK_MEM("ACP Structure");
		//pAcp = new ASMCopyPixels;

		//A dynamic memory alloc for such a small structure seems like a lot
		//of unnecessary overhead so I'm putting it on the local stack until
		//somebody gives me a better reason not to. -Kev
		pAcp = &ASMcp;
		memset(pAcp, NULL, sizeof(ASMCopyPixels));
	}
	if (p->_pScrim)
	{

		pAcp->_clut = p->_pScrim->clut;
		pAcp->_clutIdx = p->_pScrim->clutIdx;
		pAcp->_clutSize = p->_pScrim->clutSize;
	}
	pAcp->_pSrcData = pCelData;
	pAcp->_pDstData = pBufData;
	pAcp->_srcWidth = rWidth;
	pAcp->_hdir = hdir;
	pAcp->_doSkip = (p->_flags & CP_NOSKIP) ? FALSE : TRUE;
	pAcp->_srle = FALSE;

	for (row = pR->y1; row <= pR->y2; row++)
	{
		pAcp->_srcOffset = celIndex;
		pAcp->_dstOffset = bufIndex;
		OS_CopyPixelLine(pAcp);
		bufIndex += vbufWidth * vdir;
		celIndex += celWidth;
	}

	// If we created an ACP structure, delete it
	//if (dynamicAcp)
	//{
	//	delete pAcp;
	//}
}

static void
CopyPixelsExtended(RCopyPixelsEx* p)
{
	ASMCopyPixels		acp;

	acp._clut = p->_clut;
	acp._clutIdx = p->_clutIdx;
	acp._clutSize = p->_clutSize;
	CopyPixelsNormal(p, &acp);
}


static void
CopyPixelsScaledExtended(RCopyPixelsEx* p)
{
	ASMCopyPixels		acp;

	acp._clut = p->_clut;
	acp._clutIdx = p->_clutIdx;
	acp._clutSize = p->_clutSize;
	CopyPixelsScaled(p, &acp);
}


void	OS_CopyPixelsLine(ASMCopyPixels* pAcp)
{
	#ifdef DOUBLE_PIX
		register uint16		pixel;
		register uint16*	pSrc = (uint16*)(pAcp->_pSrcData);
		register uint16*	pDst = (uint16*)(pAcp->_pDstData);
		const uint16 _SKIP_BYTE = 0xfefe;
	#else
		register uchar		pixel;
		register uchar*	pSrc = pAcp->_pSrcData;
		register uchar*	pDst = pAcp->_pDstData;
		const uchar _SKIP_BYTE = 0xfe;
	#endif

	// generic C version
	register int		i;
	register int		index;
	int					col;
	
	// BUGBUG: This function should but doesn't implement SRLE expansion
	if (pAcp->_srle)	APanic ("CopyPixels doesn't handle SRLE");
	

	if (pAcp->_doSkip)
	{

		col = (pAcp->_hdir == 1) ? 0 : pAcp->_srcWidth-1;

		for (i=0; i<pAcp->_srcWidth; i++)
		{
			if (pSrc[i + pAcp->_srcOffset] != _SKIP_BYTE)
			{
				pixel = pSrc[i + pAcp->_srcOffset];
			
				// is there a clut?
				if (pAcp->_clut)
				{
					index = (pSrc[i + pAcp->_srcOffset]) & 0x00FF;
					// in clut range?
					if (index > pAcp->_clutIdx)
					{
						if (index - pAcp->_clutIdx <= pAcp->_clutSize)
						{
							pixel = pAcp->_clut[index - pAcp->_clutIdx];
							#ifdef DOUBLE_PIX
								pixel = pixel + (pixel << 8);
							#endif
						}
					}
				}
				pDst[col + pAcp->_dstOffset] = pixel;
			}
			col += pAcp->_hdir;
		}
	}
	else
	{
		// don't resolve skip, copy all

		col = (pAcp->_hdir == 1) ? 0 : pAcp->_srcWidth-1;

		for (i=0; i<pAcp->_srcWidth; i++)
		{
			pixel = pSrc[i + pAcp->_srcOffset];
		
			// is there a clut?
			if (pAcp->_clut)
			{
				index = (pSrc[i + pAcp->_srcOffset]) & 0x00FF;
				// in clut range?
				if (index > pAcp->_clutIdx)
				{
					if (index - pAcp->_clutIdx <= pAcp->_clutSize)
					{
						pixel = pAcp->_clut[index - pAcp->_clutIdx];
						#ifdef DOUBLE_PIX
							pixel = pixel + (pixel << 8);
						#endif
					}
				}
			}
			pDst[col + pAcp->_dstOffset] = pixel;
			col += pAcp->_hdir;
		}
	}
}


// CopyPixelsTRLE does not use OS_COpyPixelLine. Therefore, it will not function properly, on
// the MAC in the DOUBLE_PIX mode, nor will it perform CLUT translations.  Probably all 
// copypixels should rely on OS_CopyPixelLine.  This also applys to functions CopyPixelsTRLE,
// and CopyPixelsScaledTRLE. ALso, there appears to be no ScaledSRLE copy pixels.

static void
CopyPixelsTRLE (RCopyPixels* p)
{
	ASSERT (0);			// see warning above, jc

	register uchar*	pDst;
	register int		col;
	register uchar		pixel;
	register int		celX;
	register int		rWidth;

	Rectangle*			pR = (Rectangle*) p->_vpRectFillArea;
	int					row;
	uint32				stackCheck = 0xfadefade;
	uchar					line[MAX_H_RES];
	uint					maxExpand;
	ScanOffsetTable	*pTable;

	rWidth = pR->Wide();

	// use register variables
	pDst = p->_pDst;
	celX = p->_celX;

	// get cel data ptr
	pTable = (ScanOffsetTable*) (p->_pCelData);

	// don't expand any more than needed
	maxExpand = AMin(p->_celWidth, (celX + rWidth));

	// expand each full scan line, copy needed segment into blit buffer
	for (row = pR->y1; row <= pR->y2; row++)
	{
		ExpandRLE(line, ScanAddress(pTable, row + p->_celY - pR->y1), maxExpand);

		if (p->_flags & CP_NOSKIP)
		{
			memcpy(pDst, line + celX, rWidth);
		} 
		else 
		{
			for (col = 0; col < rWidth; col++)
			{
				if ((pixel = line[celX + col]) != SKIP_BYTE)
				{
					pDst[col] = pixel;
				}
			}
		}
		pDst += p->_vbufWidth;
	}
		
	// verify that ExpandRLE() didn't write beyond is output buffer
	ASSERT (stackCheck == 0xfadefade);
}

static void
CopyPixelsSRLE (RCopyPixels* p)
{
	register uchar*	pDst;
	register int		rWidth;
	register uint32	celIndex;
	register uint32	bufIndex;			// index from pBufData to pDst
	ASMCopyPixels		acp;

	Rectangle*			pR = (Rectangle*) p->_vpRectFillArea;
	int					row;
	uint32				stackCheck = 0xfadefade;
	uint					offset = 0;
	SRLEline*			pSRLEData;


	rWidth = pR->Wide();

	// use register variables
	pDst = p->_pDst;

	for (row = 0; row < p->_celY; row++)
	{
		offset += ((SRLEline*)(p->_pCelData + offset))->runLength + 3;
	}

	celIndex = p->_celX;
	bufIndex = pR->y1 * p->_vbufWidth + pR->x1;
	
	// Setup the ASMCopyPixels structure
	if (p->_pScrim)
	{

		acp._clut = p->_pScrim->clut;
		acp._clutIdx = p->_pScrim->clutIdx;
		acp._clutSize = p->_pScrim->clutSize;
	}
	else
	{
		acp._clut = NULL;
		acp._clutIdx = 0;
		acp._clutSize = 0;
	}
	acp._pSrcData = p->_pCelData + offset;
	acp._pDstData = p->_pBufData;
	acp._srcWidth = rWidth;
	acp._hdir = CP_FORWARD;
	acp._doSkip = (p->_flags & CP_NOSKIP) ? FALSE : TRUE;
	acp._srle = TRUE;

	// expand each full scan line, copy needed segment into blit buffer
	for (row = pR->y1; row <= pR->y2; row++)
	{
		acp._srcOffset = celIndex;
		acp._dstOffset = bufIndex;
		OS_CopyPixelLine(&acp);
		pSRLEData = (SRLEline*)(p->_pCelData + offset);
		offset += pSRLEData->runLength + 3;
		bufIndex += p->_vbufWidth;
	}
		
	// verify that SRLE didn't write beyond is output buffer
	ASSERT (stackCheck == 0xfadefade);
}

static void
CopyPixelsScaled (RCopyPixels* p, ASMCopyPixels* pAcp)
{
	// provide a "Pixel" type to make code more compatible
	#ifdef DOUBLE_PIX
		#define	Pixel		uint16
		#define	PixelPtr	uint16*
		const Pixel			_SKIP_BYTE = 0xfefe;
		APanic ("This function is untested on mac");
	#else
		#define	Pixel		uchar
		#define	PixelPtr	uchar*
		const Pixel			_SKIP_BYTE = 0xfe;
	#endif

	register Pixel		pixel;
	register PixelPtr	pSrc;
	register PixelPtr	pDst;
	register int		col;
	register int		scale;

	Rectangle*	pR = (Rectangle*) p->_vpRectFillArea;
	int			rWidth;
	int			row;
	int			endCol;
	uint32		celIndex;
	uint32		bufIndex;			// index from pBufData to pDst
	uchar*		pBufData;
	uchar*		pCelData;

	// since each pixel must be scaled, and since background cels
	// usually won't be scaled, no attempt is made to
	// optimize for CP_NOSKIP

	rWidth = pR->Wide();

	// use register variables
	scale = p->_scale;
	pBufData = p->_pBufData;
	pCelData = p->_pCelData;

	endCol = p->_celX + rWidth;
	bufIndex = pR->y1 * p->_vbufWidth + pR->x1;

	for (row = pR->y1; row <= pR->y2; row++)
	{	
		// scale Y coord to find correct row
		celIndex = (UnScaleCoord(row + p->_celY - pR->y1, scale) * p->_celWidth); 

		pDst = (PixelPtr) (pBufData + bufIndex);
		pSrc = (PixelPtr) (pCelData + celIndex);

		for (col = p->_celX; col < endCol; col++)
		{
			if ((pixel = pSrc[ UnScaleCoord(col, scale) ]) != SKIP_BYTE)
			{
				if(pAcp)
				{
					//we have some clut info
					if(pixel >= pAcp->_clutIdx && pixel < (pAcp->_clutIdx + pAcp->_clutSize))
						*pDst = pAcp->_clut[pixel - pAcp->_clutIdx];
					else
						*pDst = pixel;
				}
				else
				{
					*pDst = pixel;
				}
			}
			pDst++;
		}
		bufIndex += p->_vbufWidth;
	}
}

static void
CopyPixelsScaledTRLE (RCopyPixels* p)
{
	register int		col;
	register int		celX;
	register uchar		pixel;
	register int		scale;
	register uchar*	pDst;
	uchar*	pBufData;
	uchar*	pCelData;

	ASSERT (0);			// see warning above, jc

	Rectangle*			pR = (Rectangle*) p->_vpRectFillArea;
	int					rWidth;
	int					row;
	int					endCol;
	uint32				stackCheck = 0xfadefade;
	uchar					line[MAX_H_RES];
	uint					maxExpand;
	ScanOffsetTable	*pTable;

	// since each pixel must be scaled, and since background cels
	// usually won't be scaled, no attempt is made to
	// optimize for CP_NOSKIP

	rWidth = pR->Wide();

	// use register variables
	pDst = p->_pDst;
	pBufData = p->_pBufData;
	pCelData = p->_pCelData;
	scale = p->_scale;
	celX = p->_celX;

	// get cel data ptr
	pTable = (ScanOffsetTable*) (pCelData);

	// don't expand any more than needed
	maxExpand = p->_celWidth;
	maxExpand = AMin(p->_celWidth, UnScaleCoord(celX + rWidth, scale));

	// Expand first line
	ExpandRLE(line, 
			ScanAddress(pTable, UnScaleCoord(p->_celY, scale)),
			maxExpand);

	// init loop iter
	row = pR->y1;
	endCol = celX + rWidth;

	// expand each full scan line, copy needed segment into blit buffer
	while (TRUE)
	{
		for (col = celX; col < endCol; col++)
		{
			if ((pixel = line[ UnScaleCoord(celX+col, scale) ]) != SKIP_BYTE)
			{
				pDst[col] = pixel;
			}
		}

		// are we done?
		if (++row > pR->y2)
		{
			break;
		}

		ExpandRLE(line, 
			ScanAddress(pTable, UnScaleCoord(p->_celY + row - pR->y1, scale)), 
			maxExpand);

		pDst += p->_vbufWidth;
	}
		
	// verify that ExpandRLE() didn't write beyond is output buffer
	ASSERT (stackCheck == 0xfadefade);
}


void
AShutDownVideo()
{
	if (pGraphMgr)
	{
		delete pGraphMgr;
	}
}
