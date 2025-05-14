//
// VGABUF.CPP
//
// December 28, 1993
// Brian K. Hughes
// WATCOM: September 28, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include	"apires.hpp"
#include "palette.hpp"
#include "resource.hpp"
#include "trle.hpp"
#include "srle.hpp"
#include "vgabuf.hpp"
#include "writeres.hpp"


// Constructor - create buffer
VGABuffer::VGABuffer(uint w, uint h, coord newX, coord newY)
{
	offscreenBMap = 0;
	Construct(w, h, newX, newY);
	SetName("VGABuffer");
}

VGABuffer::VGABuffer(void)
{
	x = y = 0;
	width = height = 0;
	bufWidth = bufHeight = 0;
	size = 0;
	offscreenBMap = 0;
	SetName("VGABuffer");
}


// create buffer
void
VGABuffer::Construct(uint w, uint h, coord newX, coord newY)
{
	// OPTIMATIZATION, if the offscreenBMap already exist and is 
	// big enough, there's not need to delete and re-allocate.
	//

	// set properties for new buffer
	x = newX;
	y = newY;
	width = w;
	height = h;
	size = width * height;

	// this code can start the buffer off big, rather than resize as needed
	if(!offscreenBMap)
	{
		#ifdef OS_MAC
			offscreenBMap = ACreateBitMap(320, 498);
			bufWidth  = 320;
			bufHeight = 498;
		#else
			offscreenBMap = ACreateBitMap(320, 400);
			bufWidth  = 320;
			bufHeight = 400;
		#endif
	}

	bool	fFits=FALSE;

	if( w <= bufWidth && h <= bufHeight )
		fFits = TRUE;

	if (!fFits)
	{
		ReleaseBuffer();

		// create new off screen bit map, 
		offscreenBMap = ACreateBitMap(w, h);
		bufWidth  = w;
		bufHeight = h;
	}
}


// Destructor - destroy buffer
VGABuffer::~VGABuffer()
{
	ReleaseBuffer();
}


// release the buffer
void
VGABuffer::ReleaseBuffer(void)
{
	if (offscreenBMap)
	{
		ADestroyBitMap(offscreenBMap);
		offscreenBMap = 0;
	}
}

uchar*
VGABuffer::GetBuffer()
{
	return ADerefBitMap(offscreenBMap);
}


// Set the origin point
void
VGABuffer::SetOrigin(coord newX, coord newY)
{
	x = newX;
	y = newY;
}


// Return the pixel of a scrimage at a given global coordinate. Uses the
// Load method to be sure to incorporate scaling.
//
uchar
VGABuffer::GetPixel(Rectangle* pR, coord celXOff, coord celYOff, grip gRes, uint cel, Scrimage* pScrim)
{
	uchar* 		pResData;

	pResData = GetBuffer();
	
	// default to skip
	*pResData = SKIP_BYTE;

	// load pixel data, not resolving skip
	Load(pR, celXOff, celYOff, gRes, cel, pScrim, FALSE);

	//return first pixel of data
	return 		*pResData;

}



// Load the buffer from a cel or anim.  'gRes' will be the
// resource (either cel or anim) and 'cel' will be the cel number
// (for anims) or 0 (for cel).
//
//	r respresent the intersection of the scrimage rect and the blit rect.
//	celX, and celY represent the offset from the top/left of the scrimage 
//	rect to the intersection rect
//
void
VGABuffer::Load(Rectangle* 	pR,
						coord			celX,
						coord			celY,
						grip			gRes,
						uint			cel,
						Scrimage*	pScrim,
						uint			flags,
						bool			fIgnoreSkip,
						uint			scale)
{
	RCopyPixelsEx	rcp;
	Rectangle	r;
	Resource*	pRes = ADerefAs(Resource, gRes);
	grip			gTmpBuffer = 0;

	// Verify that the cel number is valid
	ASSERT(cel <= pRes->numHeaders);

	// Get rectangle of cel
	GetCelRect(gRes, cel, &r);

	//default clut info to NULL
	rcp._clut = NULL;
	rcp._clutIdx = 0;
	rcp._clutSize = 0;

	// Determine copy pixels transfer mode.
	rcp._flags = 0;
	rcp._pScrim = pScrim;
	if (pScrim)
	{
		rcp._scale = pScrim->scale;
	}
	else
	{
		rcp._scale = scale;
	}
	if (pRes->compType & CMP_TRLE)	rcp._flags |= CP_TRLE;
	if (pRes->compType & CMP_SRLE)	rcp._flags |= CP_SRLE;
	if (rcp._scale != 256)				rcp._flags |= CP_SCALED;
	if (fIgnoreSkip)						rcp._flags |= CP_NOSKIP;

	// If the orientation is something other than normal and the source
	// cel is compressed or scaled, first copy it to a temp buffer so
	// we have a normal cel to work with. This is so because these methods
	// don't support mirroring and flipping.
	if (flags && (rcp._flags & (CP_TRLE | CP_SRLE | CP_SCALED)))
	{
		// Create the buffer & fill with skip
		TRACK_MEM("VgaLoadTmp"); gTmpBuffer = AMalloc(r.Wide() * r.High());
		memset(ADerefAs(uchar, gTmpBuffer), CI_SKIP, r.Wide() * r.High());

		rcp._vpRectFillArea = &r;
		rcp._vbufWidth = r.Wide();
		rcp._celX = 0;
		rcp._celY = 0;
		rcp._celWidth = r.Wide();
		rcp._pCelData = GetResUnitData(gRes, cel);
		rcp._pBufData = ADerefAs(uchar, gTmpBuffer);
		rcp._pDst = rcp._pBufData;
		ACopyPixels(&rcp);
		//BUG BUG -we are assuming that any decompression can happen together
		//with any scaling -currently not always true
		rcp._flags &= ~(CP_TRLE | CP_SRLE | CP_SCALED);
		rcp._scale = 256;
	}

	//NOTE that at this point we may still need to do scaling and/or decomp
	//on a normal (non-flipped or non-mirrored) cel -consequently clut info
	//may be ignored if the needed copypixel routine doesn't support this
	//(currently that's all the decomp methods and poss. some others)

	//So, since "normal" mode will properly apply cluts we need only
	//worry about turning on cluts for normal scaled cels
	//-should also do this for any decomp methods that support this (none yet)
	if ((rcp._flags & CP_SCALED) && pScrim && pScrim->gClut)
	{
		// pass CLUT info
		rcp._flags |= CP_EXTENDED;
		rcp._clut = ADerefAs(uchar ,pScrim->gClut);
		rcp._clutIdx = pScrim->clutIdx;
		rcp._clutSize = pScrim->clutSize;
	}

	// Fill copyPixels structure
	if (flags & S_FM_VERT)	rcp._flags |= CP_VREVERSE;
	if (flags & S_FM_HORZ)	rcp._flags |= CP_HREVERSE;
	rcp._vpRectFillArea = pR;
	rcp._vbufWidth = bufWidth;
	rcp._celX = celX;
	rcp._celY = celY;
	rcp._celWidth = r.Wide();
	if (gTmpBuffer)
	{
		// Use the temp buffer we created before
		rcp._pCelData = ADerefAs(uchar, gTmpBuffer);
	}
	else
	{
		// Use the cel source data
		rcp._pCelData = GetResUnitData(gRes, cel);
	}
	rcp._pBufData = GetBuffer();
	rcp._pDst = rcp._pBufData + ((pR->y1 * bufWidth) + pR->x1);

	// fill buffer with platform optimized routine
	ACopyPixels(&rcp);

	if (gTmpBuffer)
	{
		AFree(gTmpBuffer);
	}
}



// Blit the buffer to visual
// 
void
VGABuffer::Blit(bitmap_t gBitMapData)
{
	uchar*	pBuf;

	#ifdef OS_DOS
		gBitMapData = gBitMapData;
		pBuf = ADerefAs(uchar, offscreenBMap);

		ABlit(pGraphMgr->vgaDriver, pBuf,
				x, y, width, height, bufWidth, pGraphMgr->videoBufferSeg);
	#endif
	#ifdef OS_MAC
		#include "macutils.hpp"
		//so MAC doesn't use offscreenBMap ? Why?
		OS_Blit(this, gBitMapData, bufWidth, bufHeight);
	#endif

}


// Return a pointer to the data of the specified resource's specified cel
//
uchar*
VGABuffer::GetResUnitData(grip gRes, uint celNum)
{
	CelHeader*	ch;
	Resource*	pRes;
	
	pRes = ADerefAs(Resource, gRes);
	ch = pRes->GetHeader(celNum);
	return pRes->Data() + sizeof(CelHeader) * pRes->numHeaders + ch->offset;
}


// Get the rectangle for a cel
//
void
VGABuffer::GetCelRect(grip gRes, uint celNum, Rectangle* pR)
{
	CelHeader*	ch = ADerefAs(Resource, gRes)->GetHeader(celNum);

	pR->Set(0, 0, ch->width - 1, ch->height - 1);
}
