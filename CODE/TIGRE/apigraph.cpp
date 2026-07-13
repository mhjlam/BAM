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

// because we have things in here that may be called during an ,
// turn stack checking off



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
AUpdateRect(Rectangle* /*rect*/)
{
	/* Phase 2: full-frame composite every Animate() — dirty rects are gone. */
}


void
AUpdateRect(TClipRect* /*rect*/)
{
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

static void	CopyPixelsNormal(RCopyPixels* p, ASMCopyPixels* pAcp = nullptr);
static void	CopyPixelsExtended(RCopyPixelsEx* p);
static void	CopyPixelsScaledExtended(RCopyPixelsEx* p);
static void	CopyPixelsScaledTRLE(RCopyPixels* p);
static void	CopyPixelsTRLE(RCopyPixels* p);
static void	CopyPixelsSRLE(RCopyPixels* p);
static void	CopyPixelsScaled(RCopyPixels* p, ASMCopyPixels* pAcp = nullptr);

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
	uchar*	pBufData;
	uchar*	pCelData;
	uint32	celIndex;
	uint32	bufIndex;			// index from pBufData to pDst
	int					row;
	int					rWidth;
	int					vbufWidth;
	int					celWidth;
	int					hdir, vdir;			// directions to copy

	//bool					dynamicAcp = false;

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

		//dynamicAcp = true;
		//TRACK_MEM("ACP Structure");
		//pAcp = new ASMCopyPixels;

		//A dynamic memory alloc for such a small structure seems like a lot
		//of unnecessary overhead so I'm putting it on the local stack until
		//somebody gives me a better reason not to. -Kev
		pAcp = &ASMcp;
		memset(pAcp, 0, sizeof(ASMCopyPixels));
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
	pAcp->_doSkip = (p->_flags & CP_NOSKIP) ? false : true;
	pAcp->_srle = false;

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


/* ================================================================
 * OS_CopyPixelLine_C — C implementation of the pixel copy hot path
 * (declared in apigraph.hpp; originally in compat/lib_stubs.cpp)
 * ================================================================ */
void OS_CopyPixelLine_C(ASMCopyPixels* acp)
{
    if (acp->_srle) {
        /* SRLE decompression not implemented — skip scanline. */
        return;
    }

    /* Assembly (osgrph.asm) always advances dst by +1 (inc ecx) and
     * advances src by hdir (+1 or -1).  For R→L (mirrored) sprites,
     * src starts at the rightmost pixel of the row. */
    const uchar* src;
    uchar* dst = acp->_pDstData + acp->_dstOffset;
    int dir = acp->_hdir;   /* +1 = L→R, -1 = R→L (mirrored) */

    if (dir >= 0) {
        src = acp->_pSrcData + acp->_srcOffset;
    } else {
        src = acp->_pSrcData + acp->_srcOffset + acp->_srcWidth - 1;
    }

    for (coord i = 0; i < acp->_srcWidth; i++) {
        uchar pixel = *src;
        src += dir;   /* src walks in direction; dst always walks forward */

        if (acp->_doSkip && pixel == 0xFE) { dst++; continue; }

        if (acp->_clut) {
            if ((uint)pixel >= acp->_clutIdx) {
                uint idx = (uint)pixel - acp->_clutIdx;
                if (idx <= acp->_clutSize) {
                    pixel = acp->_clut[idx];
                }
            }
        }

        *dst = pixel;
        dst++;   /* dst always advances forward */
    }
}


// CopyPixelsTRLE does not implement CLUT translations.

static void
CopyPixelsTRLE (RCopyPixels* p)
{
	ASSERT (0);			// see warning above, jc

	uchar*	pDst;
	int		col;
	uchar		pixel;
	int		celX;
	int		rWidth;

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
	uchar*	pDst;
	int		rWidth;
	uint32	celIndex;
	uint32	bufIndex;			// index from pBufData to pDst
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
		acp._clut = nullptr;
		acp._clutIdx = 0;
		acp._clutSize = 0;
	}
	acp._pSrcData = p->_pCelData + offset;
	acp._pDstData = p->_pBufData;
	acp._srcWidth = rWidth;
	acp._hdir = CP_FORWARD;
	acp._doSkip = (p->_flags & CP_NOSKIP) ? false : true;
	acp._srle = true;

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
	#define	Pixel		uchar
	#define	PixelPtr	uchar*
	const Pixel			_SKIP_BYTE = 0xfe;

	Pixel		pixel;
	PixelPtr	pSrc;
	PixelPtr	pDst;
	int		col;
	int		scale;

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
	int		col;
	int		celX;
	uchar		pixel;
	int		scale;
	uchar*	pDst;
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
	while (true)
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
