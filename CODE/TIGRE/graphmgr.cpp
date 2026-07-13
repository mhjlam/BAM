//
// GRAPHMGR.CPP
//
// November 22, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apigraph.hpp"
#include "modex.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "graphmgr.hpp"
#include "mono.hpp"
#include "mouse.hpp"
#include	"savemgr.hpp"
#include "scrimage.hpp"
#include "vgabuf.hpp"

void	SortScrimsByPri();
extern void MouseHandler( int draw_mouse );


//----[video init]----------------------------------------------------

// Initialize the graphics system (was in xmoddisp.cpp)
int
AInitGraphics(GraphicsMgr* pGM, uint mode)
{
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

	return mode;
}

bool
AInitVideo(uint /*driver*/, uint mode, uint& /*vSeg*/,
				coord& vWide, coord& vHigh, uint& /*bankInc*/, uint& origMode)
{
	uint saveMode = OS_GetScreenMode();

	if(mode == MODEX_320X400)
	{
		SetXMode();
		vWide = 320;
		vHigh = 400;
	}
	else
	{
		char mess[100];
		snprintf(mess, sizeof(mess), "GraphicsMgr::SetVGA(%d) - invalid mode\n", (int) mode);
		APanic(mess);
	}

	// only save the original mode the first time
	if (!origMode)
	{
		origMode = saveMode;
	}

	return true;
}


//----[GraphicsMgr]--------------------------------------------------

bool	GraphMgrSave(uint16 state, nlohmann::json& root);

GraphicsMgr*	pGraphMgr = nullptr;

HealthBarHook g_health_bar_hook     = nullptr;
bool          g_health_bars_enabled = false;
void SetHealthBarsEnabled(bool en) { g_health_bars_enabled = en; }

// Constructor - put us in 640x480x256 mode
GraphicsMgr::GraphicsMgr(uint mode)
{
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
	fMinRectMode = false;
}


// Destructor - put us back into text mode
GraphicsMgr::~GraphicsMgr()
{
	if (pGraphMgr)
	{
		scrimList.Release();

		// only shut down the video if a mode was saved
		if(vidMode)
		{
			OS_ShutDownVideo(vidMode);
			vidMode = 0;
		}

		pGraphMgr = nullptr;
	}
}


void
GraphicsMgr::DrawCel(uint16 /*anim*/, uint16 /*cel*/, coord /*x*/, coord /*y*/, Rectangle* /*clipRect*/)
{
	/* Drawing via VGABuffer was removed during porting — not implemented. */
}


// Resolve an animation cycle — two-pass full-frame composite.
// Pass 1: battlefield scrimages (priority < PRI_INTERFACE=10000) → vbuf → pVGAMem.
// Pass 2: UI scrimages (priority >= PRI_INTERFACE) → vbuf_ui → s_ui_texture.
// Both textures are presented by XModeFlipPage().
void
GraphicsMgr::Animate()
{
	if (!scrimList.count)
		return;

	grip		gS;
	Scrimage*	theScrim;
	grip		gRes;
	coord		vcelX, vcelY;
	uint		flags;

	SortScrimsByPri();
	MouseHandler(false);

	// Helper: clip and paint theScrim into buf. Returns false if clipped out.
	auto paint = [&](VGABuffer& buf) -> bool {
		scrimRect.x1 = theScrim->clipRect.x1;
		scrimRect.y1 = theScrim->clipRect.y1;
		scrimRect.x2 = theScrim->clipRect.x2;
		scrimRect.y2 = theScrim->clipRect.y2;
		Clip(&scrimRect);
		if (scrimRect.x1 >= scrimRect.x2 || scrimRect.y1 >= scrimRect.y2)
			return false;
		grInt.x1 = scrimRect.x1;
		grInt.y1 = scrimRect.y1;
		grInt.x2 = scrimRect.x2 - 1;
		grInt.y2 = scrimRect.y2 - 1;
		vcelX = grInt.x1 - theScrim->rect.x1;
		vcelY = grInt.y1 - theScrim->rect.y1;
		flags = theScrim->state & (S_MIRROR | S_FLIP | S_FM_VERT | S_FM_HORZ);
		if (flags & (S_FLIP | S_MIRROR)) {
			if (flags & S_FM_HORZ) vcelX = theScrim->rect.x2 - grInt.x2;
			if (flags & S_FM_VERT) vcelY = theScrim->rect.y2 - grInt.y2;
		}
		gRes = ALoad(theScrim->resType, theScrim->resNum);
		buf.Load(&grInt, vcelX, vcelY, gRes, theScrim->cel, theScrim, flags, false);
		return true;
	};

	// Pass 1: battlefield layer (priority < 10000) → vbuf → pVGAMem.
	vbuf.Construct(320, 400, 0, 0);
	memset(vbuf.GetBuffer(), 0xFE, 320 * 400);
	for (uint i = 1; i <= (uint)scrimList.count; i++) {
		theScrim = scrimList.PtrAt(i);
		if (theScrim->priority >= 10000) continue;
		paint(vbuf);
	}
	if (g_health_bar_hook && g_health_bars_enabled)
		g_health_bar_hook(static_cast<unsigned char*>(vbuf.GetBuffer()), 320, 400);
	vbuf.Blit();

	// Pass 2: UI layer (priority >= 10000) → vbuf_ui → s_ui_texture.
	vbuf_ui.Construct(320, 400, 0, 0);
	memset(vbuf_ui.GetBuffer(), 0xFE, 320 * 400);
	for (uint i = 1; i <= (uint)scrimList.count; i++) {
		theScrim = scrimList.PtrAt(i);
		if (theScrim->priority < 10000) continue;
		paint(vbuf_ui);
	}

	// Merge UI layer into pVGAMem so screen-capture code (FadeTo::ARBlit) sees the
	// full composite. Without this, ARBlit reads only the battlefield layer (pVGAMem
	// after pass 1) and captures 0xFE in the UI area, which FadeTo remaps to a
	// solid grey-pink color. 0xFE in vbuf_ui = transparent: don't overwrite.
	uchar* ui_buf = static_cast<uchar*>(vbuf_ui.GetBuffer());
	for (int i = 0; i < 320 * 400; i++) {
		if (ui_buf[i] != 0xFE)
			pVGAMem[i] = ui_buf[i];
	}

	XModeUploadUILayer(ui_buf);
	XModeFlipPage();

	// Clear dirty flags (no longer gate rendering, but keep state tidy).
	for (gS = ptr_to_grip(scrimList.FirstValue()); gS; gS = ptr_to_grip(scrimList.NextValue()))
	{
		theScrim = ADerefAs(Scrimage, gS);
		theScrim->SetState(S_ADDED | S_CHANGED, false);
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

	pScrim->SetState(S_ADDED, true);
	pScrim->SetState(S_CHANGED, false);
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
		pScrim->SetState(S_CHANGED, true);
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
}


// Create a rectangle that is the size of the anim/cel specified.
Rectangle
GraphicsMgr::GetRect(res_t resType, uint resNum, uint celNum)
{
	uchar*		pData;
	CelHeader	celHeader;

	grip gRes = ALoad(resType, resNum);
	if (!gRes || gRes.index >= GRIPS_MAX) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[GetRect] bad grip %u from ALoad: resType=%d resNum=%u celNum=%u",
			(unsigned)gRes.index, (int)resType, (unsigned)resNum, (unsigned)celNum);
	}
	pData = AGetResData(gRes);
	celHeader = ((CelHeader*) pData)[celNum - 1];
	return Rectangle(0, 0, celHeader.width - 1, celHeader.height - 1, grip{});
}



//----[ Private Functions ]------------------------------------------

// Determine if rectangle pR1 intersects rectangle pR2.
// Returns true if there is an intersection, and puts the intersection
// in r3.  Returns false if no intersection.
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
		return true;
	}
	return false;
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


int
SortAscendingPri(const void* pElem1, const void* pElem2)
{
	Scrimage*	pS1;
	Scrimage*	pS2;
	node	*pNode1, *pNode2;

	pNode1 = (node *)pElem1;
	pNode2 = (node *)pElem2;

	pS1 = ADerefAs(Scrimage, ptr_to_grip(pNode1->value));
	pS2 = ADerefAs(Scrimage, ptr_to_grip(pNode2->value));

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
	ASSERT(false);
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
			fwrite(&scaleTableDataStart, 1, (uintptr_t)&scaleTableDataEnd -
				(uintptr_t)&scaleTableDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&scaleTableDataStart, 1, (uintptr_t)&scaleTableDataEnd -
				(uintptr_t)&scaleTableDataStart, pFile);
			break;

		default:
			return(false);
	}
	return(true);
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
	return grip{};
}


//----[ Saver Routine ]----------------------------------------------

bool
GraphMgrSave(uint16 state, nlohmann::json& root)
{
	return pGraphMgr->Save(state, root);
}

bool
GraphicsMgr::Save(uint16 state, nlohmann::json& /*root*/)
{
	uint16		i;
	Scrimage*	theScrim;
	TPalette		fadePal;

	switch(state)
	{
		case BEFORE_SAVE:
			vbuf.ReleaseBuffer();
			vbuf_ui.ReleaseBuffer();
			break;

		case DURING_SAVE:
		case DURING_RESTORE:
			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			scrimList.Release();
			vbuf.ReleaseBuffer();
			vbuf_ui.ReleaseBuffer();
			break;

		case AFTER_RESTORE:
			for (i = 1; i <= scrimList.count; i++)
			{
				theScrim = scrimList.PtrAt(i);
				theScrim->SetState(S_CHANGED, true);
				theScrim->Save(AFTER_RESTORE);
			}

			fadePal.FadeToBlack();
			Animate();
			saveRestorePalette.GetColors(saveRestorePalette.guns, 0, 255);
			saveRestorePalette.FadeUp();
			break;
	}

	return false;
}


//----[ GList ]------------------------------------------------------


// Default constructor
// Allocate memory enough to hold 'max' number of nodes
//
GList::GList(uint16 max, bool resize) : SysList(max, resize)
{
	fResortList = false;
}

bool
GList::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&GListDataStart, 1, (uintptr_t)&GListDataEnd -
				(uintptr_t)&GListDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&GListDataStart, 1, (uintptr_t)&GListDataEnd -
				(uintptr_t)&GListDataStart, pFile);
			break;

		default:
			return(false);
	}
	return(true);
}

// Add a node to the list, given an id, a key, a position, and an
// optional target id.
// If the third parameter is passed it will be:
//		L_FRONT 	= node should be inserted at the front of the list
//		L_END		= node should be appended to the list (default)
//		L_AFTER	= node should be inserted after the target node
// Returns nullptr if the list is out of room and not resizable,
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
			snprintf(buffer, sizeof(buffer), "<List> - List is full: %s\n", name);
			APanic(buffer);
		}
		else
		{
			List::Resize(maxCount + 10);
		}
	}

	node* dgCurNode = mNodeAt(++count);
	dgCurNode->value = grip_to_ptr(((Object*) id)->gSelf);
	dgCurNode->index = count;
	dgCurNode->key = key;
	node* dgNode;

	switch (posn)
	{
		case L_FRONT:
			dgCurNode->next = first;
			dgCurNode->prev = 0;
			if (first)
			{
				dgNode = mNodeAt(first);
				dgNode->prev = count;
			}
			first = count;
			break;

		case L_END:
			dgCurNode->next = 0;
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

	fResortList = true;
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
// nullptr if the node isn't found
node*
GList::Find(void* id)
{
	grip gId = ((Object*) id)->gSelf;
	return List::Find(grip_to_ptr(gId));
}


// Move the node identified by 'id' to a new position, specified
// by 'posn' and optionally relative to the id 'target'.
// The second parameter determines the new position:
//		L_FRONT 	= node should be moved to the front of the list
//		L_END		= node should be moved to the list
//		L_AFTER	= node should be moved to after the target node
// Returns true if successful, false otherwise
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
	return true;
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
	return ADerefAs(Scrimage, ptr_to_grip(GList::At(index)));
}


