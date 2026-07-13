//
// MOUSEINT.CPP
//
//	Interrupt based pointer
// (instead of Scrimage based)
//
// November 24, 1993
// WATCOM: October 25, 1994  BKH
// Interrupt: November 18, 1994  JDH
// (c) Copyright 1993,1994 Tachyon, Inc.  All Rights Reserved.
//
// If you want to use the mouse, you MUST do the following:
//
// These must be done in the order shown.
//
//	// the mouse must be created after the graph manager
//	TRACK_MEM("Mouse");	new MouseInt;
//	pMouse->Init(0,0,639,479);
//	pMouse->SetRes(RES_ANIM,9997,1);
//	pMouse->Show();
//
// To shut down the mouse for program exiting, you can...
//
//     - make sure that mouse is hidden
// or  - destruct the mouse before the graph manager is destructed
//
//----[]-------------------------------------------------------------


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "graphmgr.hpp"
#include "modex.hpp"
#include "mouseint.hpp"
#include "resource.hpp"
#include	"savemgr.hpp"
#include "mono.hpp"
#include "writeres.hpp"

#define	MAX_MOUSE_IMAGE_BYTES	2048
#define	MAX_HFENCE_BYTES			320
#define	MAX_VFENCE_BYTES			400



MouseInt	*pMouseInt;

/* MouseHandler — called each game cycle.
 * All mouse and keyboard input arrives via SDL3 events, which are pumped
 * and translated into TIGRE events inside ASDLPumpEvents().
 * SetAbsolutePos() (called from there) handles pointer redraw directly. */
void MouseHandler( int draw_mouse )
{
	(void) draw_mouse;
	ASDLPumpEvents();
}



//----[ Mouse Class ]------------------------------------------------



MouseInt::MouseInt()
{
	pauseCnt = 0;

	// set these to invalid
	resType = RES_LAST;
	resNum = cel = 0;

	// allocate the memory for the mouse image and fence save areas
	gMouseImage = ASysMalloc(MAX_MOUSE_IMAGE_BYTES);
	gSaveH1Bits = ASysMalloc(MAX_HFENCE_BYTES);
	gSaveH2Bits = ASysMalloc(MAX_HFENCE_BYTES);
	gSaveV1Bits = ASysMalloc(MAX_VFENCE_BYTES);
	gSaveV2Bits = ASysMalloc(MAX_VFENCE_BYTES);
	gHFenceImage = ASysMalloc(MAX_HFENCE_BYTES);
	gVFenceImage = ASysMalloc(MAX_HFENCE_BYTES);

	// since our mallocs will never move, let's get the pointers right now
	pMouseImage = ADerefAs(uchar, gMouseImage);
	pSaveH1Bits = ADerefAs(uchar, gSaveH1Bits);
	pSaveH2Bits = ADerefAs(uchar, gSaveH2Bits);
	pSaveV1Bits = ADerefAs(uchar, gSaveV1Bits);
	pSaveV2Bits = ADerefAs(uchar, gSaveV2Bits);
	pHFenceImage = ADerefAs(uchar, gHFenceImage);
	pVFenceImage = ADerefAs(uchar, gVFenceImage);
}


MouseInt::~MouseInt()
{
	if (pGraphMgr && !hideCount)
	{
		// mouse is being shown, so hide it
		Hide();
	}

	AFree(gVFenceImage);
	AFree(gHFenceImage);
	AFree(gMouseImage);
	AFree(gSaveH1Bits);
	AFree(gSaveH2Bits);
	AFree(gSaveV1Bits);
	AFree(gSaveV2Bits);
}


coord
MouseInt::GetX()
{ 
	return x; 
}


coord
MouseInt::GetY()		
{ 
	return y; 
}


// This is mainly for Restore Game.
// Caution should be used when using this,
// because it does not update the Scrimage.
// You should probably use SetPos.
//
void	
MouseInt::SetX(coord setX)
{ 
	x = setX; 
}


// This is mainly for Restore Game.
// Caution should be used when using this,
// because it does not update the Scrimage.
// You should probably use SetPos.
//
void
MouseInt::SetY(coord setY)		
{ 
	y = setY; 
}


int
MouseInt::GetCel()		
{ 
	return cel; 
}


res_t	
MouseInt::GetResType()
{
	return resType;
}


uint	
MouseInt::GetResNum()
{
	return resNum;
}


// this is the routine that actually shows the pointer.
// this is protected on purpose.  If you want to 
// Show the pointer, call Show();
void
MouseInt::ShowPointer()
{
	MovePointer();
}

// this is the routine that actually hides the pointer.
// this is protected on purpose.  If you want to 
// Hide the pointer, call Hide();
void
MouseInt::HidePointer()
{
	RestoreUnderBits();
}


// if the pointer rect intersects the passed rect, protect
// the pointer by Hiding the mouse.  If in a single buffered
// environment, Pause should probably be called before this call.  
// Return values:  true  - pointer protected
//                 false - pointer did not need protection
bool
MouseInt::Protect(bool protect, Rectangle *pRect)
{
	bool	fProtected = false;

	if (protect)
	{
		if(!pRect)
			return(false);

		if(AMin(lastRect.x2, pRect->x2) >= AMax(pRect->x1, lastRect.x1) &&
			AMin(lastRect.y2, pRect->y2) >= AMax(pRect->y1, lastRect.y1))
		{
			// we need to protect the mouse
			Hide();
			fProtected = true;
		}

		if(!fProtected)
		{
			int	x1, y1, x2, y2;
			
			x1 = AMin(rCurrentFence.x1, rCurrentFence.x2);
			x2 = AMax(rCurrentFence.x1, rCurrentFence.x2);
			y1 = AMin(rCurrentFence.y1, rCurrentFence.y2);
			y2 = AMax(rCurrentFence.y1, rCurrentFence.y2);
			
			if(AMin(x2, pRect->x2) >= AMax(pRect->x1, x1) &&
				AMin(y2, pRect->y2) >= AMax(pRect->y1, y1))
			{
				// we might need to protect the fence
				Hide();
				fProtected = true;
			}
		}
		return(fProtected);
	}
	else
	{
		Show();	
	}

	return false;
}

void
MouseInt::SetRes(res_t type, uint num, uint iCel, uint iScale)
{
	Resource		*pRes;
	grip 			gRes;
	char			mess[100];
	CelHeader	*celHeader;
	uchar*		pResData;
	uint32		width;	
	uint32		height;
	uint			numCels;
	res_t		oldType;
	uint		oldNum;
	int		tempOrgX, tempOrgY, tempX2, tempY2;

	if (type != resType ||
		 num != resNum ||
		 cel != iCel ||
		 scale != iScale)
	{
		// Extract information about the current resource
		oldType = resType;
		oldNum = resNum;

		resType = type;
		resNum = num;
		cel = iCel;
		scale = iScale;

		gRes = ALoad(resType, resNum);

		// Adjust the use counts - this will purge the old resource, if
		// no one else is using it
		Object::SetResCount(oldType, oldNum, resType, resNum);

		pRes = ADerefAs(Resource, gRes);
		numCels = pRes->numHeaders;
		iCel = cel - 1;

		// check for bogus cel number
		if (iCel >= numCels)
		{
			snprintf(mess, sizeof(mess), "Bogus Cel Num (Anim %d) cel num %d, of %d cels", resNum, iCel+1, numCels);
			APanic(mess);
		}

		pResData = AGetResData(gRes);
		
		celHeader = &((CelHeader*) pResData)[iCel];

		// take origin point into account
		tempOrgX = ScaleCoord(celHeader->orgX, scale);
		tempOrgY = ScaleCoord(celHeader->orgY, scale);

		// find the rect (based off of 0,0)
		tempX2 = ScaleCoord(celHeader->width, scale) - 1;
		tempY2 = ScaleCoord(celHeader->height,scale) - 1;

		// save these computed values for speed purposes
		width = ScaleCoord(celHeader->width, scale);
		height = ScaleCoord(celHeader->height, scale);

		if ((width * height) > MAX_MOUSE_IMAGE_BYTES)
		{
			snprintf(mess, sizeof(mess), "Mouse Anim(%d, %d) too large", resNum, cel);
			APanic(mess);
		}

		VGABuffer	vbuf;

		vbuf.offscreenBMap = (bitmap_t) gMouseImage;
		vbuf.width = width;
		vbuf.bufWidth = width;

		orgX = tempOrgX;
		orgY = tempOrgY;

		rect.x1 = 0;
		rect.y1 = 0;
		rect.x2 = tempX2;
		rect.y2 = tempY2;
		rect.owner = gSelf;

		vbuf.Load(&rect, 0, 0, gRes, cel, nullptr, 0, true, scale);
		// clear this so that the VGABuffer destructor doesn't free it
		vbuf.offscreenBMap = grip{};

		MovePointer();
	}
}

void
MouseInt::SetFenceRes(res_t horizType, uint rHorizNum, uint horizCel,
	res_t vertType, uint rVertNum, uint vertCel)
{
	Resource		*pRes;
	grip 			gRes;
	CelHeader	*celHeader;
	char			mess[100];
	uchar*		pResData;
	uint32		width;	
	uint32		height;
	uint			numCels, celNum;
	int			tempOrgX, tempOrgY, tempX2, tempY2;
	VGABuffer	vbuf;
	Rectangle	rFence;

	gRes = ALoad(horizType, rHorizNum);
	celNum = horizCel - 1;

	// Adjust the use counts - this will purge the old resource, if
	// no one else is using it
//	Object::SetResCount(oldType, oldNum, resType, resNum);

  	pRes = ADerefAs(Resource, gRes);
  	numCels = pRes->numHeaders;

	// check for bogus cel number
	if (celNum >= numCels)
	{
		snprintf(mess, sizeof(mess), "Bogus Cel Num (Anim %d) cel num %d, of %d cels",
			resNum, celNum, numCels);
		APanic(mess);
	}

	pResData = AGetResData(gRes);
		
	celHeader = &((CelHeader*) pResData)[celNum];

	// take origin point into account
	tempOrgX = ScaleCoord(celHeader->orgX, 256);
	tempOrgY = ScaleCoord(celHeader->orgY, 256);

	// find the rect (based off of 0,0)
	tempX2 = ScaleCoord(celHeader->width, 256) - 1;
	tempY2 = ScaleCoord(celHeader->height, 256) - 1;

	// save these computed values for speed purposes
	width = ScaleCoord(celHeader->width, 256);
	height = ScaleCoord(celHeader->height, 256);

	if ((width * height) > MAX_HFENCE_BYTES)
	{
		snprintf(mess, sizeof(mess), "Mouse Anim(%d, %d) too large", resNum, celNum);
		APanic(mess);
	}

	vbuf.offscreenBMap = (bitmap_t) gHFenceImage;
	vbuf.width = width;
	vbuf.bufWidth = width;

	orgX = tempOrgX;
	orgY = tempOrgY;

	rFence.Set(0, 0, celHeader->width - 1, celHeader->height - 1);
	rFence.owner = gSelf;

	vbuf.Load(&rFence, 0, 0, gRes, celNum + 1, nullptr, 0, true, 256);
	// clear this so that the VGABuffer destructor doesn't free it
	vbuf.offscreenBMap = grip{};

	gRes = ALoad(vertType, rVertNum);
	celNum = vertCel - 1;

	// Adjust the use counts - this will purge the old resource, if
	// no one else is using it
//	Object::SetResCount(oldType, oldNum, resType, resNum);

  	pRes = ADerefAs(Resource, gRes);
  	numCels = pRes->numHeaders;

	// check for bogus cel number
	if (celNum >= numCels)
	{
		snprintf(mess, sizeof(mess), "Bogus Cel Num (Anim %d) cel num %d, of %d cels",
			resNum, celNum, numCels);
		APanic(mess);
	}

	pResData = AGetResData(gRes);
		
	celHeader = &((CelHeader*) pResData)[celNum];

	// take origin point into account
	tempOrgX = ScaleCoord(celHeader->orgX, 256);
	tempOrgY = ScaleCoord(celHeader->orgY, 256);

	// find the rect (based off of 0,0)
	tempX2 = ScaleCoord(celHeader->width, 256) - 1;
	tempY2 = ScaleCoord(celHeader->height, 256) - 1;

	// save these computed values for speed purposes
	width = ScaleCoord(celHeader->width, 256);
	height = ScaleCoord(celHeader->height, 256);

	if ((width * height) > MAX_VFENCE_BYTES)
	{
		snprintf(mess, sizeof(mess), "Mouse Anim(%d, %d) too large", resNum, celNum);
		APanic(mess);
	}

	vbuf.offscreenBMap = (bitmap_t) gVFenceImage;
	vbuf.width = width;
	vbuf.bufWidth = width;

	orgX = tempOrgX;
	orgY = tempOrgY;

	rFence.Set(0, 0, celHeader->width - 1, celHeader->height - 1);
	rFence.owner = gSelf;

	vbuf.Load(&rFence, 0, 0, gRes, celNum + 1, nullptr, 0, true, 256);
	// clear this so that the VGABuffer destructor doesn't free it
	vbuf.offscreenBMap = grip{};
}

void
MouseInt::SetScale(uint newScale)
{
	if (scale != newScale)
	{
		SetRes(resType, resNum, cel, newScale);
	}
}


void
MouseInt::SetPri(int32 /*pri*/)
{
}


void
MouseInt::UpdatePointer(void)
{
	// this is just a stub for tigre.
}

/* Set the cursor to an absolute game-space position and redraw it.
 * Unlike Mouse::SetPos(), this always redraws when the position changes. */
void
MouseInt::SetAbsolutePos(coord newX, coord newY)
{
	if (!pGraphMgr) return;
	if (status & MST_NO_RES) return;

	coord prevX = GetX();
	coord prevY = GetY();
	SetX(newX);
	SetY(newY);
	CheckBounds();   /* clamp to mouseLimits */

	if (GetX() != prevX || GetY() != prevY)
	{
		lastX = prevX;
		lastY = prevY;
		if (!hideCount)
		{
			RestoreUnderBits();
			DrawPointer();
		}
	}
}

// clip the last rect to be within the screen coords.
// also set clipRow and clipCol (used when drawing).
void
MouseInt::ClipLastRect(void)
{
	if (lastRect.x1 < screenLimits.x1)
	{
		clipCol = screenLimits.x1 - lastRect.x1;
		lastRect.x1 = screenLimits.x1;
	}
	else
	{
		clipCol = 0;	
	}

	if (lastRect.y1 < screenLimits.y1)
	{
		clipRow = screenLimits.y1 - lastRect.y1;
		lastRect.y1 = screenLimits.y1;
	}
	else
	{
		clipRow = 0;	
	}

	lastRect.x2 = AMax(AMin(lastRect.x2, screenLimits.x2), lastRect.x1);
	lastRect.y2 = AMax(AMin(lastRect.y2, screenLimits.y2), lastRect.y1);
}

// this routine moves the pointer to it's new location.
// x and y should be set prior to calling this routine.
void
MouseInt::MovePointer(void)
{
	if (!hideCount)
	{

		RestoreUnderBits();
		DrawPointer();

	}
}

// Update lastRect for Protect() and save fence background pixels.
// Cursor pixels are no longer saved here — the cursor is composited
// directly over the RGB24 buffer in XModeFlipPage().
void
MouseInt::SaveUnderBits(void)
{
	if(fDragMode)
	{
		int x1, y1, x2, y2, temp;

		x1 = rCurrentFence.x1;
		x2 = rCurrentFence.x2;
		y1 = rCurrentFence.y1;
		y2 = rCurrentFence.y2;

		if(x2 < x1)
		{
			temp = x1;
			x1 = x2;
			x2 = temp;
		}
		if(y2 < y1)
		{
			temp = y1;
			y1 = y2;
			y2 = temp;
		}

		rLastFenceRect.Set(x1, y1, x2, y2);

		ARBlit(pGraphMgr->vgaDriver, pSaveH1Bits, rLastFenceRect.x1, rLastFenceRect.y1,
			rLastFenceRect.Wide(), 1, pGraphMgr->videoBufferSeg);
		ARBlit(pGraphMgr->vgaDriver, pSaveH2Bits, rLastFenceRect.x1, rLastFenceRect.y2,
			rLastFenceRect.Wide(), 1, pGraphMgr->videoBufferSeg);
		ARBlit(pGraphMgr->vgaDriver, pSaveV1Bits, rLastFenceRect.x1, rLastFenceRect.y1,
			1, rLastFenceRect.High(), pGraphMgr->videoBufferSeg);
		ARBlit(pGraphMgr->vgaDriver, pSaveV2Bits, rLastFenceRect.x2, rLastFenceRect.y1,
			1, rLastFenceRect.High(), pGraphMgr->videoBufferSeg);
	}

	/* Update lastRect so Protect() can test cursor vs. dirty rects. */
	lastRect.x1 = x - orgX;
	lastRect.y1 = y - orgY;
	lastRect.x2 = lastRect.x1 + rect.Wide() - 1;
	lastRect.y2 = lastRect.y1 + rect.High() - 1;
	ClipLastRect();
}

// Restore fence background pixels.
// Cursor restore is no longer needed — cursor is composited in XModeFlipPage().
void
MouseInt::RestoreUnderBits(void)
{
	if (fDragMode)
	{
		HideFence();
	}
}

void
MouseInt::EnableDragging(bool fNewState)
{
	if(fNewState)
	{
		if(fDragMode)
		{
			// umm.. already dragging?  Clean up first.
			EnableDragging(false);
		}

		// enable drag mode
		fDragMode = true;
//		pMono->Out("MOUSE:DRAG ENABLED @%d,%d!\n", GetX(), GetY());
		rCurrentFence.x1 = GetX();
		rCurrentFence.y1 = GetY();
		rCurrentFence.x2 = rCurrentFence.x1;
		rCurrentFence.y2 = rCurrentFence.y1;
		rLastFenceRect.Set(-1, -1, -1, -1);
	}
	else
	{
		if(fDragMode)
		{
			// disable drag mode
			HideFence();
			fDragMode = false;
//			pMono->Out("MOUSE:DRAG DISABLED @%d,%d!\n", GetX(), GetY());
		}
		// else already disabled - nothing to do
	}
}

void
MouseInt::HideFence(void)
{
	int x1 = rLastFenceRect.x1, y1 = rLastFenceRect.y1,
		x2 = rLastFenceRect.x2,	y2 = rLastFenceRect.y2, temp;

	if(x1 == -1)
	{
		// nevermind, no previous data to restore
		return;
	}

	if(x2 < x1)
	{
		temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if(y2 < y1)
	{
		temp = y1;
		y1 = y2;
		y2 = temp;
	}
	ABlit(pGraphMgr->vgaDriver, pSaveH1Bits,
		rLastFenceRect.x1, rLastFenceRect.y1, rLastFenceRect.Wide(),
		1, rLastFenceRect.Wide(), pGraphMgr->videoBufferSeg);
	ABlit(pGraphMgr->vgaDriver, pSaveH2Bits,
		rLastFenceRect.x1, rLastFenceRect.y2, rLastFenceRect.Wide(),
		1, rLastFenceRect.Wide(), pGraphMgr->videoBufferSeg);
	ABlit(pGraphMgr->vgaDriver, pSaveV1Bits,
		rLastFenceRect.x1, rLastFenceRect.y1, 1,
		rLastFenceRect.High(), 1, pGraphMgr->videoBufferSeg);
	ABlit(pGraphMgr->vgaDriver, pSaveV2Bits,
		rLastFenceRect.x2, rLastFenceRect.y1, 1,
		rLastFenceRect.High(), 1, pGraphMgr->videoBufferSeg);
}

// Update cursor state and draw the drag fence if active.
// Cursor pixels are composited over the RGB24 buffer in XModeFlipPage()
// rather than being drawn into the 8-bit framebuffer here.
void
MouseInt::DrawPointer(void)
{
	int		tmpFenceX,tmpFenceY;

	SaveUnderBits();

	/* Cursor is now composited in XModeFlipPage(); nothing to blit here. */

	if(fDragMode)
	{
		int	fenceX1, fenceX2, fenceY1, fenceY2, temp;

		fenceX1 = rCurrentFence.x1;
		fenceX2 = rCurrentFence.x2;
		fenceY1 = rCurrentFence.y1;
		fenceY2 = rCurrentFence.y2;

		// in case of negative delta, swap
		if(fenceX2 < fenceX1)
		{
			temp = fenceX1;
			fenceX1 = fenceX2;
			fenceX2 = temp;
		}

		if(fenceY2 < fenceY1)
		{
			temp = fenceY1;
			fenceY1 = fenceY2;
			fenceY2 = temp;
		}

		if(fenceX2 > rClickDrag.x2)
			fenceX2 = rClickDrag.x2;
		if(fenceY2 > rClickDrag.y2)
			fenceY2 = rClickDrag.y2;

		tmpFenceX = fenceX2 - fenceX1 + 1;
		tmpFenceY = fenceY2 - fenceY1 + 1;

		/* Fence is composited in XModeUploadUILayer(); nothing to blit here. */
		(void)tmpFenceX; (void)tmpFenceY;

//		pMono->Out("(%d,%d %d,%d)\n", fenceX1, fenceY1, fenceX2, fenceY2);
	}
}

// stop the mouse from moving on the screen, but don't turn off
// interrupts.  If the mouse is paused, save what the coordinates
// should be.  Restore the real coordinates when the mouse is unpaused.

void
MouseInt::Pause(bool pauseIt)
{

	if (pauseIt)
	{
		if (!pauseCnt)
		{
			// pausing for the first time
			pauseX = x;
			pauseY = y;
		}

		pauseCnt++;
	}
	else
	{
		// unpause
		if (pauseCnt)
		{
			// we are paused
			pauseCnt--;

			if (!pauseCnt)
			{
				// we are fully unpaused
				if (pauseX != x || pauseY != y)
				{
					// the mouse was moved
					SetX(pauseX);
					SetY(pauseY);

					if (!hideCount)
					{
						RestoreUnderBits();
						DrawPointer();
					}
				}
			}
		}
	}

}

/* Return cursor image data for compositing in XModeFlipPage().
 * ppData: 8-bit indexed pixel buffer, row-major, width×height bytes.
 * Pixel value 0xFE is transparent (SKIP_COLOR).
 * hotX/hotY: hotspot offset from cursor top-left in game pixels. */
bool
MouseInt::GetCursorForComposite(const uchar** ppData, int* w, int* h,
                                int* hotX, int* hotY)
{
	if (status & MST_NO_RES)
		return false;
	*ppData = pMouseImage;
	*w      = rect.Wide();
	*h      = rect.High();
	*hotX   = orgX;
	*hotY   = orgY;
	return true;
}

/* Return fence image strips for compositing in XModeUploadUILayer().
 * ppHData: horizontal strip (MAX_HFENCE_BYTES=320 bytes, one pixel per col).
 * ppVData: vertical strip (MAX_VFENCE_BYTES=400 bytes, one pixel per row).
 * Returns false if not in drag mode or fence resource not set. */
bool
MouseInt::GetFenceForComposite(const uchar** ppHData, const uchar** ppVData)
{
	if (!fDragMode || !pHFenceImage || !pVFenceImage)
		return false;
	*ppHData = pHFenceImage;
	*ppVData = pVFenceImage;
	return true;
}

//----[ Saver Routine ]----------------------------------------------

bool
MouseInt::Save(uint16 state, nlohmann::json& root)
{
	switch(state)
	{
		default:
			break;
	}
	return false;
}



