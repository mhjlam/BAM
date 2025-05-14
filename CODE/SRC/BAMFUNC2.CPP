// BAMFUNC2.CPP
//
//	Copyright 1995 Tachyon, Inc.
//
// Global functions for BAM and MAPEDIT
//
//	08-10-94
//

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "fontmgr.hpp"
#include "mono.hpp"
#include "resource.hpp"
#include "text.hpp"

//#include "bam.hpp"
#include "bamfunc2.hpp"
#include "alldefs.hpp"

#include <string.h>

void
SetFontColors(int newBack, int newFore1, int newFore2)
{
	SetFontColors((int32)newBack, (int32)newFore1, (int32)newFore2, (int32)newFore1, (int32)newFore2);
}

void
SetFontColors(int32 newBack, int32 newFore1, int32 newFore2, int32 newFore3, int32 newFore4)
{
	pFontMgr->colors[FNT_BACK_COLOR] = (uchar) newBack;
	pFontMgr->colors[FNT_FORE_COLOR] = (uchar) newFore1;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = (uchar) newFore2;
	pFontMgr->colors[FNT_FORE_COLOR + 2] = (uchar) newFore3;
	pFontMgr->colors[FNT_FORE_COLOR + 3] = (uchar) newFore4;
}

void
SetFontColors(int32 newBack, int32 newFore1, int32 newFore2, int32 newFore3,
				  int32 newFore4, int32 newFore5, int32 newFore6)
{
	pFontMgr->colors[FNT_BACK_COLOR] = (uchar) newBack;
	pFontMgr->colors[FNT_FORE_COLOR] = (uchar) newFore1;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = (uchar) newFore2;
	pFontMgr->colors[FNT_FORE_COLOR + 2] = (uchar) newFore3;
	pFontMgr->colors[FNT_FORE_COLOR + 3] = (uchar) newFore4;
	pFontMgr->colors[FNT_FORE_COLOR + 4] = (uchar) newFore5;
	pFontMgr->colors[FNT_FORE_COLOR + 5] = (uchar) newFore6;
}

// copy given rect from Src into DestX/Y
void
CopyCel(CelHeader *pDest, int32 destX,	int32 destY, res_t resType, 
	uint resNum, uint32 celNum, Rectangle *rSrc,	bool fHasSkip,
	grip gNewClut)
{
	CopyCel(pDest, destX, destY, resType, resNum, celNum, rSrc->x1,
		rSrc->y1, rSrc->x2, rSrc->y2, fHasSkip, gNewClut);
}

// copy all of src cel into dest at destX/Y
void
CopyCel(CelHeader *pDest, int32 destX, int32 destY, res_t resType,
	uint resNum, uint32 celNum, bool fHasSkip, grip gNewClut, bool fUseOrgs)
{
	Resource		*pSrcRes;
	CelHeader	*celHeader;
	grip			gSrcRes;

	gSrcRes = ALoadDebug(__FILE__, __LINE__, resType, resNum);

	if (resType == RES_CEL)
	{
		// make sure that the cel num is 1 if this is a cel resource
		celNum = 1;
	}

	pSrcRes = ADerefAs(Resource, gSrcRes);					// pointer to resource itself

	// check to see if we have the cel
	if (celNum > pSrcRes->numHeaders)
	{
		char	mess[100];
		sprintf(mess, "Resource %d only has %d cels.  %d requested", pSrcRes->number,
				pSrcRes->numHeaders, celNum);
		APanic(mess);
	}

	celHeader = pSrcRes->GetHeader(uint16 (celNum));

	CopyCel(pDest, destX, destY, resType, resNum, celNum, 0, 0, celHeader->width - 1, celHeader->height - 1,
		fHasSkip, gNewClut, fUseOrgs);
}

// copy from X1/Y1 to X2/Y2 of src resource, into dest at DestX/Y
void
CopyCel(CelHeader *pDest, int32 destX, int32 destY, res_t resType, 
	uint resNum, uint32 celNum, uint32 srcX,
	uint32 srcY, uint32 srcX2, uint32 srcY2, bool fHasSkip, grip gNewClut, bool fUseOrgs)
{
	CelHeader	*pSrcCel, *pDestCel, *tempCelHeader;
	uchar			*pSrcData, *pDestData;
	Resource		*pSrcRes;
	RCopyPixelsEx	rcp;
	int			destX2, destY2, orgX, orgY;

	grip			gSrcRes;

	gSrcRes = ALoadDebug(__FILE__, __LINE__, resType, resNum);

	if (resType == RES_CEL)
	{
		// make sure that the cel num is 1 if this is a cel resource
		celNum = 1;
	}

	pSrcRes = ADerefAs(Resource, gSrcRes);					// pointer to resource itself

	// check to see if we have the cel
	if (celNum > pSrcRes->numHeaders)
	{
		char	mess[100];
		sprintf(mess, "Resource %d only has %d cels.  %d requested", pSrcRes->number,
				pSrcRes->numHeaders, celNum);
		APanic(mess);
	}

	tempCelHeader = pSrcRes->GetHeader((uint16) (celNum));
	pSrcData = AGetResData(gSrcRes) + (sizeof(CelHeader) * pSrcRes->numHeaders) + tempCelHeader->offset;

	orgX = tempCelHeader->orgX;
	orgY = tempCelHeader->orgY;

	// first - compensate for trim-skip side effect with neg orgs
	if(orgX < 0)
	{
		destX -= orgX;
	}
	if(orgY < 0)
	{
		destY -= orgY;
	}

	if(fUseOrgs)
	{
		if(orgX > 0)
			destX -= orgX;

		if(orgY > 0)
			destY -= orgY;
	}

	pSrcCel = tempCelHeader;
	//	pMono->Out("orgX=%d orgY=%d\n", pSrcCel->orgX, pSrcCel->orgY);

	pDestCel = pDest;
	pDestData = (uchar *)pDest + sizeof(CelHeader);
	
	// some boundary checking
	destX2 = destX + (srcX2 - srcX);
	if(destX2 >= pDest->width)
	{
		destX2 = pDest->width - 1;
	}

	destY2 = destY + (srcY2 - srcY);

	if(destY2 >= pDest->height)
	{
		destY2 = pDest->height - 1;
	}

	Rectangle	rFillArea(destX, destY, destX2, destY2);

	// fill copyPixels structure
	rcp._vpRectFillArea = &rFillArea;
	rcp._vbufWidth = pDest->width;
	rcp._celX = srcX;
	rcp._celY = srcY;
	rcp._celWidth = pSrcCel->width;
	rcp._pCelData = pSrcData;	
	rcp._pBufData = pDestData;	
	rcp._pDst = rcp._pBufData + ((rFillArea.y1 * pDest->width) + rFillArea.x1);
	rcp._scale = 256;
	rcp._pScrim = NULL;

	// determine copy pixels transfer mode.
	rcp._flags = 0;
	if (pSrcRes->compType & CMP_TRLE)	rcp._flags |= CP_TRLE;
	if (pSrcRes->compType & CMP_SRLE)	rcp._flags |= CP_SRLE;
//	if (scale != 256)						rcp._flags |= CP_SCALED;
	if (!fHasSkip)							rcp._flags |= CP_NOSKIP;

	// pass CLUT info
	rcp._flags |= CP_EXTENDED;
	if (gNewClut)
	{
		rcp._clut = ADerefAs(uchar ,gNewClut);
	}
	else
	{
		rcp._clut = NULL;
	}
	rcp._clutIdx = CLUT_TEAM_COLORS;
	rcp._clutSize = CLUT_SIZE;

	// fill buffer with platform optimized routine
	ACopyPixels(&rcp);
//	pMono->Out("CopyCel(): orgX=%d orgY=%d\n", tempCelHeader->orgX, tempCelHeader->orgY);
}
