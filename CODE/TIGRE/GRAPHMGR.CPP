//
// GRAPHMGR.CPP
//
// November 22, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "graphmgr.hpp"
#include "mono.hpp"
#include "mouse.hpp"
#include	"savemgr.hpp"
#include "scrimage.hpp"
#include "vgabuf.hpp"

#define NDEBUG

void	SortScrimsByPri();
extern void MouseHandler( int draw_mouse );



//----[GraphicsMgr]--------------------------------------------------

bool	GraphMgrSave(uint16 state, FILE *fp);

GraphicsMgr*	pGraphMgr = NULL;

// Constructor - put us in 640x480x256 mode
GraphicsMgr::GraphicsMgr(uint mode)
{
	#ifndef NDEBUG
	FILE	*pDummyFile;
	ScrimList	list1;
	ScaleTable	table1;
	TPalette		palette1;

	pDummyFile = fopen("sl1.dat", "wb");
	fwrite(&list1, 1, sizeof(scrimList), pDummyFile);
	fclose(pDummyFile);

	pDummyFile = fopen("st1.dat", "wb");
	fwrite(&table1, 1, sizeof(scaleTable), pDummyFile);
	fclose(pDummyFile);
	
	pDummyFile = fopen("pal1.dat", "wb");
	fwrite(&palette1, 1, sizeof(palette1), pDummyFile);
	fclose(pDummyFile);
	#endif
	
	// setup exceptional ptr to GraphicsMgr
	if (!pGraphMgr)
	{
		// set this to uninitialized so that we can know if we actually
		// saved a mode.
		vidMode = 0;

		pGraphMgr = this;
		// setup for save
		AtSave(GraphMgrSave);
	}
	
	videoBufferSeg = 0;

	// platform specific graphics init
	vgaDriver = AInitGraphics(this, mode);

	scaleTable.Set(130, PercentToScale(50), 340, PercentToScale(100));

	// default is for MaxRect
	fMinRectMode = FALSE;
}


// Destructor - put us back into text mode
GraphicsMgr::~GraphicsMgr()
{
	if (pGraphMgr)
	{
		scrimList.Release();
		drawPool.Release();

		// only shut down the video if a mode was saved
		if(vidMode)
		{
			OS_ShutDownVideo(vidMode);
			vidMode = 0;
		}

		pGraphMgr = NULL;
	}
}


// Draw the specified cel at the specified location.  The clipRect represents
// the portion of the cel to show, relative to the cel's 0,0.  If clipRect
// is NULL, the whole dimensions of the cel should be used.  If 'gRes' is
// an anim, 'cel' identifies the cel to draw.  Otherwise, 'cel' is ignored.
//
void
GraphicsMgr::DrawCel(uint16 anim, uint16 cel, coord x, coord y, Rectangle* clipRect)
{
	uint			clipImageWidth = clipRect->x2 - clipRect->x1;
	uint			clipImageHeight = clipRect->y2 - clipRect->y1;
	VGABuffer*	pVgaBuffer;

	anim = anim;	// Gets rid of unreferenced warning
	cel = cel;

	// First, clip the clipRect by the screen
	Rectangle grRect(x, y, x + clipImageWidth, y + clipImageHeight);

	Clip(&grRect);
	clipRect->x2 = clipRect->x1 + (grRect.x2 - grRect.x1);
	clipRect->y2 = clipRect->y1 + (grRect.y2 - grRect.y1);
	clipImageWidth = clipRect->x2 - clipRect->x1;
	clipImageHeight = clipRect->y2 - clipRect->y1;

	// At this point, clipRect represents the clipped rectangle in cel-relative
	// coordinates again.  Now create a VGABuffer and use it to expand and
	// display the cel.
	TRACK_MEM("VGABuffer");
	pVgaBuffer = new VGABuffer(clipImageWidth, clipImageHeight, x, y);
//	if (anim == DYNAMIC_CEL)
//	{
//		pVgaBuffer->Load(clipRect->x1, clipRect->y1,
//								ALoad(RES_DCEL, cel), 0);
//	}
//	else if (anim)
//	{
//		pVgaBuffer->Load(clipRect->x1, clipRect->y1,
//								ALoad(RES_ANIM, anim), cel);
//	}
//	else
//	{
//		pVgaBuffer->Load(clipRect->x1, clipRect->y1,
//								ALoad(RES_CEL, cel), 0);
//	}

	// Show the cel on the screen & dump the vga VGABuffer.
//	pVgaBuffer->Blit();
	delete pVgaBuffer;
}


// Resolve an animation cycle.  This is our meat 'n' potatas, guys!
void
GraphicsMgr::Animate()
{
	if (!scrimList.count)
	{
		drawPool.Release();
		return;
	}

	Rectangle	vbufRect;
	grip			gS;
	Scrimage*	theScrim;
	Scrimage*	picScrim;
	grip 			gRes;
	TClipRect*	pGr1;

	coord			vcelX;
	coord			vcelY;
 	uint			theCel;
	bool 			fIgnoreSkip;
	uint			flags;
	bool			mouseHidden;

	// DEBUG!  Can we do this elsewhere?
	SortScrimsByPri();
	picScrim = scrimList.PtrAt(1);

	// Gather all rectangles from the Scrimages who matter.
	//
	for (gS = (grip) scrimList.FirstValue();
			gS != NULL;
			gS = (grip) scrimList.NextValue())
	{
		theScrim = ADerefAs(Scrimage, gS);

		if (theScrim->CheckState(S_ADDED | S_CHANGED))
		{
			// if not empty
			if (!theScrim->clipRect.IsEmpty())
			{
				// Add rect to list of rects needing update
				drawPool.Add(&theScrim->clipRect);
			}
		}

		if (theScrim->CheckState(S_CHANGED))
		{
			// if not empty
			if (!theScrim->lastRect.IsEmpty())
			{
				// Add rect to list of rects needing update
				drawPool.Add(&theScrim->lastRect);
			}
		}
	}


	MouseHandler(FALSE);

	// Now the finalDrawPool list contains all invalid rectangles.  Next, we go
	// through these rects looking for intersections and split them up.
	if (drawPool.Count())
	{
		drawPool.UniteRects();
	}


	// The finalDrawPool list now contains the smallest number of rectangles to
	// cover all changes in Scrimages.  Furthermore, none of the rectangles
	// intersect.  Now we can create a VGABuffer for each rectangle and
	// load it with cel data from all Scrimages that intersect with that
	// rectangle.

	for (pGr1 = drawPool.FirstValue();
			pGr1 != NULL;
			pGr1 = drawPool.NextValue())
	{
		Clip(pGr1);

		// If the rectangle is completely clipped, get rid of it
		if (pGr1->x1 != pGr1->x2 && pGr1->y1 != pGr1->y2)
		{
			vbuf.Construct(pGr1->Wide(), pGr1->High(), pGr1->x1, pGr1->y1);

			// for first load, ignore skip
			fIgnoreSkip = TRUE;

			for (gS = (grip) scrimList.FirstValue();
					gS != NULL;
					gS = (grip) scrimList.NextValue())
			{
				flags = 0;

				theScrim = ADerefAs(Scrimage, gS);

				scrimRect.x1 = theScrim->clipRect.x1;
				scrimRect.y1 = theScrim->clipRect.y1;
				scrimRect.x2 = theScrim->clipRect.x2;
				scrimRect.y2 = theScrim->clipRect.y2;

				flags |= (theScrim->state & (S_MIRROR | S_FLIP | S_FM_VERT | S_FM_HORZ));

				//does this scrim rect intersect this group rect
				if (IntersectRect(pGr1, &scrimRect, &grInt))
				{
					//set starting copy pt. to offset into
					//Src cel by amt. of Intersect cel
					vcelX = grInt.x1 - theScrim->rect.x1;
					vcelY = grInt.y1 - theScrim->rect.y1;

					//if flipping or mirroring then set starting copy pt to
					//"back-off" from "end" of Src cel by amount of Intersect cel
					if (flags & (S_FLIP | S_MIRROR))
					{
						if (flags & S_FM_HORZ)
						{
							vcelX = theScrim->rect.x2 - grInt.x2;
						}
						if (flags & S_FM_VERT)
						{
							vcelY = theScrim->rect.y2 - grInt.y2;
						}
					}

					//reset reference pt. (0,0) to be at start of
					//Intersect rect instead of Group rect for vbuf.Load()
					grInt.x1 -= pGr1->x1;
					grInt.y1 -= pGr1->y1;
					grInt.x2 -= pGr1->x1;
					grInt.y2 -= pGr1->y1;

					gRes = ALoad(theScrim->resType, theScrim->resNum);
					theCel = theScrim->cel;
					vbuf.Load(&grInt, vcelX, vcelY, gRes, theCel, theScrim, flags, fIgnoreSkip);
					fIgnoreSkip = FALSE;
					MouseHandler(FALSE);
				}
			}

			// mouse stuff expects an inclusive rect.  so make the adjustment
			pGr1->x2 -= 1;
			pGr1->y2 -= 1;

			// stop the mouse from moving visually
			pMouse->Pause(TRUE);
			// check to see if the rectangle conflicts.
			// if interrupt mouse, this will hide the mouse
			mouseHidden = pMouse->Protect(TRUE, pGr1);

			vbuf.Blit();

			if (mouseHidden)
			{
				// the mouse was hidden.  restore it 
				pMouse->Protect(FALSE);
			}

			// let the mouse visually update again
			pMouse->Pause(FALSE);
		}
	}

	// Clear the lists.  Since all rectangles on these lists are new
	//	allocations, we delete them all and then release the nodes.
   // Note that all rectangles remaining on the pdrawPool will
   // also be on the drawPool list, so the pdrawPool can simply
   // be released.
	//
	drawPool.Release();


	// Lastly, turn off all the S_ADDED and S_CHANGED bits of
	// all the scrimages and set their lastRects.
	//
	for (gS = (grip) scrimList.FirstValue();
			gS != NULL;
			gS = (grip) scrimList.NextValue())
	{
		theScrim = ADerefAs(Scrimage, gS);

		theScrim->SetState(S_ADDED | S_CHANGED, FALSE);
		theScrim->lastRect.Copy(&theScrim->clipRect);
	}
}


// Add an Scrimage object to the new list.
void
GraphicsMgr::AddScrimage(Scrimage* pScrim)
{
	// If already in the scrim list, ignore
	if (scrimList.Find(pScrim))
	{
		return;
	}

	pScrim->SetState(S_ADDED, TRUE);
	pScrim->SetState(S_CHANGED, FALSE);
	scrimList.Add(pScrim);
}


// Change a Scrimage object.
void
GraphicsMgr::ChangeScrimage(Scrimage* pScrim)
{
	// If not in either roster, this is an add
	if (!scrimList.Find(pScrim))
	{
		AddScrimage(pScrim);
	}
	else
	{
		pScrim->SetState(S_CHANGED, TRUE);
	}
}


// Delete a Scrimage object.
void
GraphicsMgr::DeleteScrimage(Scrimage* pScrim)
{
	// If not in either roster, ignore
	if (!scrimList.Find(pScrim))
	{
		//pMono->Out("<GraphicsMgr::DeleteScrimage> Scrimage not in list\n");
		return;
	}

	scrimList.Delete(pScrim);
	TRACK_MEM("DeleteScrimage");
	if (!pScrim->lastRect.IsEmpty())
	{
		// This scrim has been drawn. 
		// Add the drawn rect to list of rects needing update
		drawPool.Add(&pScrim->lastRect);
	}
}


// Tell the graphmgr to update everything within this rectangle
// on the next call to Animate.
void
GraphicsMgr::UpdateRect(Rectangle *pRect)
{
	drawPool.Add(pRect);
}


// Tell the graphmgr to update everything within this rectangle
// on the next call to Animate.
void
GraphicsMgr::UpdateRect(TClipRect *pRect)
{
	drawPool.Add(pRect);
}


// Create a rectangle that is the size of the anim/cel specified.
Rectangle*
GraphicsMgr::GetRect(res_t resType, uint resNum, uint celNum)
{
	uchar*		pData;
	CelHeader	celHeader;

	pData = AGetResData(ALoad(resType, resNum));
	celHeader = ((CelHeader*) pData)[celNum - 1];
	TRACK_MEM("GetRect");
	return new Rectangle(0, 0, celHeader.width - 1, celHeader.height - 1, 0);
}



//----[ Private Functions ]------------------------------------------

// Determine if rectangle pR1 intersects rectangle pR2.
// Returns TRUE if there is an intersection, and puts the intersection
// in r3.  Returns FALSE if no intersection.
//
bool
GraphicsMgr::IntersectRect(TClipRect* pR1, TClipRect* pR2, Rectangle* pR3)
{
	coord x1;
	coord y1;
	coord x2;
	coord y2;

	x1 = AMax(pR1->x1, pR2->x1);
	x2 = AMin(pR2->x2, pR1->x2);
	y1 = AMax(pR1->y1, pR2->y1);
	y2 = AMin(pR2->y2, pR1->y2);

	if (x2 > x1 && y2 > y1)
	{
		pR3->x1 = x1;
		pR3->y1 = y1;
		// we are making an inclusive rectangle
		pR3->x2 = x2 - 1;
		pR3->y2 = y2 - 1;
		return TRUE;
	}
	return FALSE;
}


// Clip the given rectangle 'r1' to the rectangle 'r2'.
// If 'r2' is not passed, the screen coordinates are used.
//
void
GraphicsMgr::Clip(Rectangle* r1, Rectangle* pR2)
{
	coord rx1;
	coord ry1;
	coord rx2;
	coord ry2;

	rx1 = pR2 ? pR2->x1 : 0;
	ry1 = pR2 ? pR2->y1 : 0;
	rx2 = pR2 ? pR2->x2 : maxX;
	ry2 = pR2 ? pR2->y2 : maxY;

	r1->x1 = AMax(r1->x1, rx1);
	r1->y1 = AMax(r1->y1, ry1);
	r1->x2 = AMax(AMin(r1->x2, rx2), r1->x1);
	r1->y2 = AMax(AMin(r1->y2, ry2), r1->y1);
}


// Clip the given rectangle 'r1' to the rectangle 'r2'.
// If 'r2' is not passed, the screen coordinates are used.
//
void
GraphicsMgr::Clip(TClipRect* r1, TClipRect* pR2)
{
	coord rx1;
	coord ry1;
	coord rx2;
	coord ry2;

	rx1 = pR2 ? pR2->x1 : 0;
	ry1 = pR2 ? pR2->y1 : 0;
	rx2 = pR2 ? pR2->x2 : maxX + 1;
	ry2 = pR2 ? pR2->y2 : maxY + 1;

	r1->x1 = AMax(r1->x1, rx1);
	r1->y1 = AMax(r1->y1, ry1);
	r1->x2 = AMax(AMin(r1->x2, rx2), r1->x1);
	r1->y2 = AMax(AMin(r1->y2, ry2), r1->y1);
}


// DEBUG!  This routine sorts the pScrimList by ascending priority.
// This should be done elsewhere.

#ifdef OS_MAC
	// some odd difference in compiler's requires the following on the mac
	// and prohibits it on the pc!
	extern "C"	int SortAscendingPri(const void* pElem1, const void* pElem2);
#endif

int
SortAscendingPri(const void* pElem1, const void* pElem2)
{
	Scrimage*	pS1;
	Scrimage*	pS2;
	node	*pNode1, *pNode2;

	pNode1 = (node *)pElem1;
	pNode2 = (node *)pElem2;

	pS1 = ADerefAs(Scrimage, (grip)(pNode1->value));
	pS2 = ADerefAs(Scrimage, (grip)(pNode2->value));

	if(pS1->priority > pS2->priority)	return(1);
	if(pS1->priority < pS2->priority)	return(-1);
	if(pS1->priority == pS2->priority)	
	{
		// we should never switch order if pri's are the same,
		// so return the order according to the order prior to sort
		if (pNode1->index > pNode2->index)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	// shouldn't get here
	ASSERT(FALSE);
	return 0;
}


void
SortScrimsByPri()
{
	node*		pNode;
	uint16	i, count;
	GList		*pList;

	pList = &(pGraphMgr->scrimList);

	// don't sort unless we know we need to
	if (!pList->fResortList)
	{
		return;
	}

	count = pList->count;

	if (count > 1)
	{
		// get * of first physical node in table of nodes
		pNode = pList->GetPNode(1);

		OS_qsort((void*) pNode, count, sizeof(node), SortAscendingPri);

		// set logical order to match phyiscal order
		pNode = pList->GetPNode(1);

		for (i = 1; i <= count; i++)
		{
			pNode[i - 1].prev = i - 1;
			pNode[i - 1].next = i + 1;
			pNode[i - 1].index = i;
		}
		pNode[count - 1].next = 0;

		pList->first = 1;
		pList->last = count;
	}
}


//----[ ScaleTable Class ]-------------------------------------------

ScaleTable::ScaleTable(void)
{
	Fill(PercentToScale(100));
}

bool
ScaleTable::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&scaleTableDataStart, 1, (int)&scaleTableDataEnd -
				(int)&scaleTableDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&scaleTableDataStart, 1, (int)&scaleTableDataEnd -
				(int)&scaleTableDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

void
ScaleTable::Fill(uint16 scale)
{
	unsigned	i;

	for (i = 0; i < MAX_SCALE_Y; i++)
	{
		aScales[i] = scale;
	}

}


void
ScaleTable::Set(coord top, uint16 topScale, coord bottom, uint16 bottomScale)
{
	unsigned	i;

	// check for Duh code
	ASSERT (top >= 0);
	ASSERT (bottom < 641);
	ASSERT (bottom >= top);

	// set top area scaling
	for (i = 0; i < top; i++)
	{
		aScales[i] = topScale;
	}


	// scale the y coords between top and bottom
	long	height = bottom - top;
	long	partialH;
	long	hRatio;
	long	scaleDif = bottomScale - topScale;
	long	curScale;

	for (i = top; i < bottom; i++)
	{
		// get ratio of i between top and bottom
		partialH = bottom - i;

		hRatio = (partialH * 256) / height;

		// apply ratio to scale difference
		curScale = scaleDif * hRatio;
		curScale /= 256;

		// set the current index as base minus ratio
		aScales[i] = (uint16) (bottomScale - curScale);
	}

	// set bottom area scaling
	for (i = bottom; i < MAX_SCALE_Y; i++)
	{
		aScales[i] = bottomScale;
	}
}


// return grip to guy who has the highest priority of non-skip pixel
//	at a given point
//
grip
GraphicsMgr::FindTopGuy(coord x, coord y)
{
	Scrimage*	pScrim;
	coord			celX, celY;
	uchar			pixel;
	unsigned		i;
	uint			theCel;
	grip			gRes;
	const	uchar	SKIP_BYTE = 254;

	SortScrimsByPri();

	// Scan scrimages from top pri down
	//
	for (i = scrimList.count; i > 0; i--)
	{
		pScrim = scrimList.PtrAt(i);
		
		// don't consider mouse cursor
		if (pMouse && pScrim->gOwner == pMouse->gSelf)
		{
			continue;
		}

		if (pScrim->rect.Contains(x, y))
		{
			//// find offset into cel of global coord
		  	//celX = x - pScrim->rect.x1;
			//celY = y - pScrim->rect.y1;

			// set one pixel rectangle
			grInt.Set(x,y,x,y);

			// create vgabuf of a one pixel rectangle
			vbuf.Construct(1, 1, x, y);

			// find offset of blit rect within scrim rect
			celX = grInt.x1 - pScrim->rect.x1;
			celY = grInt.y1 - pScrim->rect.y1;

			ASSERT (celX >= 0);
			ASSERT (celY >= 0);

			// offset blit rect to 0,0
			grInt.x1 -= x;
			grInt.y1 -= y;
			grInt.x2 -= x;
			grInt.y2 -= y;

			gRes = ALoad(pScrim->resType, pScrim->resNum);
			theCel = pScrim->cel;
			pixel = vbuf.GetPixel(&grInt, celX, celY, gRes, theCel, pScrim);

			if (pixel != SKIP_BYTE)
			{
				// found top non-skip scrim, return its guy
				return pScrim->gOwner;
			}
		}
	}

	// found nobody
	return 0;
}


//----[ Saver Routine ]----------------------------------------------

bool
GraphMgrSave(uint16 state, FILE *fp)
{
	return	pGraphMgr->Save(state, fp);
}

bool
GraphicsMgr::Save(uint16 state, FILE *fp)
{
	bool			error = FALSE;
	grip			gMgr = 0;
	GraphicsMgr	*pTempMgr;
	grip			gOldListData;
	grip			gListData;
	uint16		i;
	Scrimage*	theScrim;
	TPalette		fadePal;

	switch(state)
	{
		case BEFORE_SAVE:
			// get rid of this buffer so that it won't be saved
			vbuf.ReleaseBuffer();
			break;

		case DURING_SAVE:
			// save current palette
			saveRestorePalette.GetColors(saveRestorePalette.guns, 0, 255);

			// write out graphmgr
			if (fwrite(pGraphMgr, 1, sizeof(GraphicsMgr), fp) != sizeof(GraphicsMgr))
			{
				// some sort of error
				error = TRUE;
				break;
			}

			// write out the scrim list data
			if (ASaveGripNum(scrimList.gData, fp))
			{
				// some sort of error
				error = TRUE;
				break;
			}

			scaleTable.Save(DURING_SAVE, fp);

			scrimList.Save(DURING_SAVE, fp);

			saveRestorePalette.Save(DURING_SAVE, fp);

			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			// clear the draw list. it's unneeded because all scrims will be updated.
			drawPool.Release();

			// this scrim list is history
			scrimList.Release();

			// get rid of this buffer
			vbuf.ReleaseBuffer();
			break;

		case DURING_RESTORE:
			// read old graphmgr into temp
			TRACK_MEM("TmpGraphMgr");	gMgr = AMalloc(sizeof(GraphicsMgr));
			pTempMgr = ADerefAs(GraphicsMgr, gMgr);

			// run in a while loop so that we can break on an error
			// and clean up properly
			while (TRUE)
			{
				// read the graph manager
				if (fread(pTempMgr, 1, sizeof(GraphicsMgr), fp) != sizeof(GraphicsMgr))
				{
					// some sort of error
					error = TRUE;
					break;
				}

				// read the scrim list data 
				gOldListData = scrimList.gData;
				AFree(gOldListData);
				ARestoreGripNum(&gListData, fp);
				scrimList.gData = gListData;

				scaleTable.Save(DURING_RESTORE, fp);
				
				scrimList.Save(DURING_RESTORE, fp);

				saveRestorePalette.Save(DURING_RESTORE, fp);

				break;
			}

			// if we allocated memory, free it
			if (gMgr)
			{
				AFree(gMgr);
			}

			break;

		case AFTER_RESTORE:
			// set all scrims to changed
			for (i = 1; i <= scrimList.count; i++)
			{
				theScrim = scrimList.PtrAt(i);
				theScrim->SetState(S_CHANGED, TRUE);
				theScrim->Save(AFTER_RESTORE);
			}

			// fade to black
			fadePal.FadeToBlack();

			// animate
			Animate();

			// fade palette back to normal
			saveRestorePalette.FadeUp();
			break;
	}

	return error;
}


//----[ GList ]------------------------------------------------------


// Default constructor
// Allocate memory enough to hold 'max' number of nodes
//
GList::GList(uint16 max, bool resize) : SysList(max, resize)
{
	fResortList = FALSE;
}

bool
GList::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&GListDataStart, 1, (int)&GListDataEnd -
				(int)&GListDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&GListDataStart, 1, (int)&GListDataEnd -
				(int)&GListDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

// Add a node to the list, given an id, a key, a position, and an
// optional target id.
// If the third parameter is passed it will be:
//		L_FRONT 	= node should be inserted at the front of the list
//		L_END		= node should be appended to the list (default)
//		L_AFTER	= node should be inserted after the target node
// Returns NULL if the list is out of room and not resizable,
// the new count of the list otherwise
//
uint16
GList::Add(void* id, int32 key, uint16 posn, void* target)
{
	// check is the list allow duplicate entries
	if ((!fDuplicates) && (Find(id)))
	{
		return (count);
	}

	// check for need to resize list
	if (count >= maxCount)
	{
		if (!resizable)
		{
			char buffer[40];
			sprintf(buffer, "<List> - List is full: %s\n", name);
			APanic(buffer);
		}
		else
		{
			List::Resize(maxCount + 10);
		}
	}

	node* dgCurNode = mNodeAt(++count);
	dgCurNode->value = (void*) ((Object*) id)->gSelf;
	dgCurNode->index = count;
	dgCurNode->key = key;
	node* dgNode;

	switch (posn)
	{
		case L_FRONT:
			dgCurNode->next = first;
			dgCurNode->prev = NULL;
			if (first)
			{
				dgNode = mNodeAt(first);
				dgNode->prev = count;
			}
			first = count;
			break;

		case L_END:
			dgCurNode->next = NULL;
			dgCurNode->prev = last;
			if (last)
			{
				dgNode = mNodeAt(last);
				dgNode->next = dgCurNode->index;
			}
			last = count;
			break;

		case L_AFTER:
			dgNode = Find(target);
			if (!target)
			{
				APanic("<GList::Add> No target specified for L_AFTER");
			}

			if (!dgNode)
			{
				APanic("<GList::Add> Specified target not found");
			}
			dgCurNode->next = dgNode->next;
			dgCurNode->prev = dgNode->index;
			dgNode->next = dgCurNode->index;
			if (dgCurNode->next)
			{
				dgNode = mNodeAt(dgCurNode->next);
				dgNode->prev = dgCurNode->index;
			}

			// If we're adding after the last element, we're now the last element
			if (!dgCurNode->next)
			{
				last = dgCurNode->index;
			}
			break;
	}
	if (!first)
	{
		first = dgCurNode->index;
	}
	if (!last)
	{
		last = dgCurNode->index;
	}

	fResortList = TRUE;
	return count;
}


// Delete a node from the list, given an id or a node*.
//uint16
//GList::Delete(void* id)
//{
//	node*	pNode = Find(id);
//	if (!pNode)
//	{
//		return count;
//	}
//	return List::Delete(pNode);
//}


// Return the node identified by the id or
// NULL if the node isn't found
node*
GList::Find(void* id)
{
	grip gId = ((Object*) id)->gSelf;
	return List::Find((void*) gId);
}


// Move the node identified by 'id' to a new position, specified
// by 'posn' and optionally relative to the id 'target'.
// The second parameter determines the new position:
//		L_FRONT 	= node should be moved to the front of the list
//		L_END		= node should be moved to the list
//		L_AFTER	= node should be moved to after the target node
// Returns TRUE if successful, FALSE otherwise
bool
GList::Move(void* id, uint16 posn, void* target)
{
	int32 key = Find(id)->key;
	Delete(id);

	switch (posn)
	{
		case L_FRONT:
			Add(id, key, L_FRONT);
			break;

		case L_END:
			Add(id, key, L_END);
			break;

		case L_AFTER:
			if (!target)
			{
				APanic("<GList::Move> No target specified for L_AFTER");
			}

			Add(id, key, L_AFTER, target);
			break;
	}
	return TRUE;
}


// Release all nodes so that the list can be deleted
void
GList::Release()
{
	node* 	theNode;

	while (count)
	{
		theNode = GetPNode(1);
		Delete(theNode);
	}
}



//----[ ScrimList Class ]--------------------------------------------

Scrimage*
ScrimList::PtrAt(uint index)
{
	return ADerefAs(Scrimage, (grip) GList::At(index));
}



//----[TRectPool]----------------------------------------------------


TRectPool::TRectPool()
{
	Release();
}


void
TRectPool::Release()
{
	int	i;

	for (i=0; i<MAX_GRECTS; i++)
	{
		aRects[i].Construct();
	}

	count = 0;
}

int 
TRectPool::Add(Rectangle* r)
{
	int	i;
	
	i = GetAvailIndex ();

	aRects[i].Construct(r);
	pGraphMgr->Clip(&aRects[i]);

	count++;

	// if overflowing, reduce rects
	if (count == MAX_GRECTS)
	{
		ReduceRects();
	}

	return count;
}

int 
TRectPool::Add(TClipRect* r)
{
	int	i;
	
	i = GetAvailIndex ();

	aRects[i].Construct(r);
	pGraphMgr->Clip(&aRects[i]);

	count++;

	// if overflowing, reduce rects
	if (count == MAX_GRECTS)
	{
		ReduceRects();
	}

	return count;
}


int 
TRectPool::Count()
{
	return count;
}


TClipRect*
TRectPool::FirstValue()
{
	iter = 0;

	return NextValue();
}


TClipRect*
TRectPool::NextValue()
{
	int	i;

	for (i=iter; i<MAX_GRECTS; i++)
	{
		// check if is in used
		if (aRects[i].y2)
		{
			iter = i+1;
			return &aRects[i];
		}
	}

	// if none in use
	iter = MAX_GRECTS;
	return NULL;
}


int
TRectPool::GetAvailIndex ()
{
	int	i;

	for (i=0; i<MAX_GRECTS; i++)
	{
		if (aRects[i].y2 == 0)
		{
			return i;
		}
	}
	assert (0);
	return 0;
}


// Unite all graph rects whose bounding rect's area is smaller than
//	150% of the sum of the areas of the parts
//
int
TRectPool::UniteRects()
{
	int			i;
	int			j;
	int			bounds;
	long			sumArea;

	if (count < 2)
	{
		// takes two to tangle
		return count;
	}

	// nested loop to compare rects
	//
	for (i=0; i<MAX_GRECTS-1; i++)
	{
		// check if rect is in use
		if (aRects[i].y2 == 0)	continue;

		for (j=i+1; j<MAX_GRECTS; j++)
		{
			// check if rect is in use
			if (aRects[j].y2 == 0)	continue;

			// If the area of the bounding rect is < 150% of the sum of the
			// rects' areas, unite the rects. That is, if there is a lot
			// of overlap, unite the rects.

			bounds = aRects[i].AreaBounds(&aRects[j]);
			sumArea = aRects[i].area + aRects[j].area;

			if ( (bounds * 2) <= (sumArea * 3))
			{
				// unite i and j
				aRects[i].Unite(&aRects[j]);

				// empty j
				aRects[j].Construct();
				count--;

				if (count < 2)
				{
					// take 2 to tangle
					return count;
				}

				// break this loop...
				j = MAX_GRECTS+1;

				// ...and repeat i with the new larger rect.
				i--;
			}
		}
	}

	return count;
}


// Before the pool overflows ReduceRects() guarantees a reduction
// in the number of rects.
//
int
TRectPool::ReduceRects()
{
	// try uniting first
	if (UniteRects() == MAX_GRECTS)
	{
		// ok, that didn't work. Lets find the two biggest, merge them.
		
		// give programmers warning that TRectPool should be bigger
		if (pMono) pMono->Out("TRectPool::ReduceRects");

		int 	i;
		int 	iLargeArea1 = 1;
		int 	iLargeArea2 = 0;
		long	largeArea1 = 0;
		long	largeArea2 = 0;

		// find two largest
		//
		for (i=0; i<MAX_GRECTS; i++)
		{
			// check if rect is in use
			if (aRects[i].y2 == 0)	continue;
 			
			if (aRects[i].area > largeArea1)
			{
				iLargeArea2 = iLargeArea1;
				iLargeArea1 = i;

				largeArea1 = aRects[iLargeArea1].area;
				largeArea2 = aRects[iLargeArea2].area;
			}
		}

		// unite two largest
		aRects[iLargeArea1].Unite(&aRects[iLargeArea2]);

		// empty #2
		aRects[iLargeArea2].Construct();
		count--;

		// the two more call to Add() will bring us back here,
		// but UniteRects will be more effective.
	}
	return count;
}


