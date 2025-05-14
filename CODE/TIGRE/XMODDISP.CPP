//
// XModDisp.CPP
//
// November 12, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// X Mode implementation of Graphics functions.
//
//----[]-------------------------------------------------------------


#include <dos.h>
#include <i86.h>

#include "api.hpp"
#include "apigraph.hpp"
#include "XModDisp.hpp"
#include "graphmgr.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


// Initialize the graphics system
int
AInitGraphics(GraphicsMgr* pGM, uint mode)
{
	if (ADetectVGA())
	{
		// we have a vga card
		pGM->vgaDriver = mode;

		if (AInitVideo(pGM->vgaDriver, mode, pGM->videoBufferSeg,
						pGM->maxX, pGM->maxY, pGM->vgaBankNoIncrement,
						pGM->vidMode) == 0)
		{
			// video mode didn't set
			APanic("Can't init video.");
		}

		--pGM->maxX;
		--pGM->maxY;

		// install shutdown function
//		atexit(AShutDownVideo);
	}
	else
	{
		APanic("VGA card not found.");
	}

	return mode;
}



//---[ Access to Assembly Routines ]---------------------------------

// Determine if we have a VGA card.
// If no VGA card is detected, 0 is returned.

int
ADetectVGA()
{
	#define VIDEO_STATE_INQUIRE 0x1c00

	uint16	foundVGA;

	union PassRegs
	{
		REGS			inRegs, outRegs;
		PassRegs ()
		{
			memset (this, 0, sizeof (*this));
		}
	} passRegs;

	passRegs.inRegs.x.eax = VIDEO_STATE_INQUIRE;
	passRegs.inRegs.x.ecx = 1;
	int386(INT_VIDEO, &passRegs.inRegs, &passRegs.outRegs);

	if (passRegs.outRegs.h.al == 0x1c)
	{
		// a VGA card was detected
		foundVGA = TRUE;
	}
	else
	{
		// no vga card detected
		foundVGA = FALSE;
	}

	return foundVGA;
}

bool
AInitVideo(uint driver, uint mode, uint& vSeg,
				coord& vWide, coord& vHigh, uint& bankInc, uint& origMode)
{
	uint	saveMode;

	// get rid of compiler warnings
	driver = driver;
	vSeg = vSeg;
	bankInc = bankInc;

	// Grab the current mode before we go to the new mode
	saveMode = OS_GetScreenMode();

	if(mode == MODEX_320X400)
	{	
		SetXMode();
		vWide = 320;
		vHigh = 400;
	}
	else
	{	
		char	mess[100];
		sprintf(mess, "GraphicsMgr::SetVGA(%d) - invalid mode\n", (int) mode);
		APanic(mess);
	}

	// only save the original mode the first time
	if (!origMode)
	{
		origMode = saveMode;						//Set video mode property value
	}

	return (TRUE);		//Return TRUE if mode was accepted
}


//// blit vgabuf data to screen
//void
//ABlit(uint driver, uchar* pData, coord x, coord y,
//		uint bufWidth, uint bufHeight, uint vSeg)
//{
//	uint16	imageRow, imageCol, videoRow, videoCol, planeNum, planeLoop;
////	uchar		*pVGAMem = (uchar *) 0xa0000;
//	uchar		*pVidBuff, *pImageBuff;
//
//	// get rid of warnings
//	driver = driver;
//	vSeg = vSeg;
//
//	// Copy bitmap to screen
//	
////  	pMono->Out("  Blit(): X=%d, Y=%d, W=%d, H=%d\n", (int)x, (int)y, (int)width, (int)height);
//   videoCol = (uint16) (x / 4);
//	videoRow = (uint16) y;
//	pVidBuff = pVGAMem + (videoRow * 80) + videoCol;
//
//	pImageBuff = pData;
//	for (imageRow = 0; imageRow < bufHeight; imageRow++, pVidBuff += 80, pImageBuff += bufWidth)
//	{
//		// X mode - copy image one bank at a time
//		for(planeNum = (uint16) (x & 3), planeLoop = 0; planeLoop < 4; planeLoop++, planeNum++)
//		{
//			// set the bit corresponding to the plane the pixel is in.
//			SetXModeWritePlane(planeNum);
//
//			videoCol = 0;
//			if(planeNum > 3)
//				videoCol = 1;
//			
//			for(imageCol = planeLoop; imageCol < bufWidth; videoCol++, imageCol += 4)
//				*(pVidBuff + videoCol) = *(pImageBuff + imageCol);
//		}
//	}
//}
//
//// blit screen to vgabuf data
//void
//ARBlit(uint driver, uchar* pData, coord x, coord y,
//		uint bufWidth, uint bufHeight, uint vSeg)
//{
//	uint16	imageRow, imageCol, videoRow, videoCol, planeNum, planeLoop;
////	uchar		*pVGAMem = (uchar *) 0xa0000;
//	uchar		*pVidBuff, *pImageBuff;
//
//	// get rid of warnings
//	driver = driver;
//	vSeg = vSeg;
//
//	// Copy bitmap to screen
//	
////  	pMono->Out("  Blit(): X=%d, Y=%d, W=%d, H=%d\n", (int)x, (int)y, (int)width, (int)height);
//   videoCol = (uint16) (x / 4);
//	videoRow = (uint16) y;
//	pVidBuff = pVGAMem + (videoRow * 80) + videoCol;
//
//	pImageBuff = pData;
//	for (imageRow = 0; imageRow < bufHeight; imageRow++, pVidBuff += 80, pImageBuff += bufWidth)
//	{
//		// X mode - copy image one bank at a time
//		for(planeNum = (uint16) (x & 3), planeLoop = 0; planeLoop < 4; planeLoop++, planeNum++)
//		{
//			// set the bit corresponding to the plane the pixel is in.
//			SetXModeReadPlane(planeNum);
//
//			videoCol = 0;
//			if(planeNum > 3)
//				videoCol = 1;
//			
//			for(imageCol = planeLoop; imageCol < bufWidth; videoCol++, imageCol += 4)
//				*(pImageBuff + imageCol) =	*(pVidBuff + videoCol);
//		}
//	}
//}
