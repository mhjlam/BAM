//
// SCRIMAGE.CPP
//
// November 22, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "graphmgr.hpp"
#include "mono.hpp"
#include "resource.hpp"
#include "scrimage.hpp"
#include "savebase.hpp"

enum 
{
	_STAND 		= 0,
	FACE_EAST 	= 1,
	FACE_WEST 	= 2,
	FACE_SOUTH 	= 3,
	FACE_NORTH 	= 4,
	FACE_SE	 	= 5,
	FACE_SW	 	= 6,
	FACE_NE	 	= 7,
	FACE_NW	 	= 8
};



// Constructor - set properties
Scrimage::Scrimage()
{
	classID = CID_SCRIM;
	x = 0;
	y = 0;
	resType = RES_CEL;	// 0
	resNum = 0;
	cel = 0;
	scale = 256;
	state = 0;
	SetRect(0, 0, 0, 0);
	priority = 0;
	hasSkip = FALSE;
	numCels = 0;

	clut = NULL;
	gClut = NULL;
	clutIdx = 0;
	clutSize = 0;

	lastX = 0;
	lastY = 0;
	SetLastRect(0, 0, 0, 0);
	orgX = orgY = 0;
	SetName("Scrimage");
}


// Destructor
Scrimage::~Scrimage()
{
	FlushDynamicCel();
}

// Set the rectangle of the scrimage to the coords specified.
void
Scrimage::SetRect(coord x1, coord y1, coord x2, coord y2)
{
	rect.Set(x1, y1, x2, y2);
	rect.owner = gSelf;
	if (rect.IsEmpty())
	{
		clipRect.Set(x1, y1, x2, y2);
	}
	else
	{
		clipRect.Set(x1, y1, x2 + 1, y2 + 1);
	}
	clipRect.owner = gSelf;
}


// Set the rectangle to the dimensions of the cel at the origin
void
Scrimage::SetRect()
{
	CelHeader	celHeader;

	celHeader = GetCelHeader();

	// take origin point into account
	orgX = ScaleCoord(celHeader.orgX, scale);
	orgY = ScaleCoord(celHeader.orgY, scale);

	// set rect
	if (CheckState(S_FLIP))
	{
		// We're flipped, so rotate around the origin
		if (CheckState(S_FM_HORZ))
		{
			rect.x2 = x + orgX;
			rect.y1 = y - orgY;
			rect.x1 = rect.x2 - ScaleCoord(celHeader.width, scale) + 1;
			rect.y2 = rect.y1 + ScaleCoord(celHeader.height,scale) - 1;
		}
		else
		{
			rect.x1 = x - orgX;
			rect.y2 = y + orgY;
			rect.x2 = rect.x1 + ScaleCoord(celHeader.width, scale) - 1;
			rect.y1 = rect.y2 - ScaleCoord(celHeader.height,scale) + 1;
		}
	}
	else
	{
		rect.x1 = x - orgX;
		rect.y1 = y - orgY;
		rect.x2 = x + ScaleCoord(celHeader.width, scale) - orgX - 1;
		rect.y2 = y + ScaleCoord(celHeader.height,scale) - orgY - 1;
	}
	rect.owner = gSelf;

	if (rect.IsEmpty())
	{
		clipRect.Set(rect.x1, rect.y1, rect.x2, rect.y2);
	}
	else
	{
		clipRect.Set(rect.x1, rect.y1, rect.x2 + 1, rect.y2 + 1);
	}
	clipRect.owner = gSelf;
}


// Set the last rectangle of the scrimage to the coords specified.
void
Scrimage::SetLastRect(coord x1, coord y1, coord x2, coord y2)
{
	Rectangle r(x1, y1, x2, y2, gSelf);
	lastRect.x1 = r.x1;
	lastRect.y1 = r.y1;
	lastRect.x2 = r.x2;
	lastRect.y2 = r.y2;
	lastRect.owner = r.owner;
}


// Set the clip rect to the coordinates specified
void
Scrimage::SetClipRect(coord x1, coord y1, coord x2, coord y2)
{
	clipRect.Set(x1, y1, x2, y2);
	clipRect.owner = gSelf;
}


// Set a state bit.
// Returns the old value of bits changed
bool
Scrimage::SetState(uint16 flag, bool on)
{
	bool oldState = (state & flag) ? TRUE : FALSE;

	if (on)
	{
		state |= flag;
	}
	else
	{
		state &= ~flag;
	}

	return oldState;
}


// Return the value of a state bit.
bool
Scrimage::CheckState(uint16 flag)
{
	return (state & flag) ? TRUE : FALSE;
}


// read cel header for defaults, used by Picture::Load()
CelHeader
Scrimage::GetCelHeader()
{
	Resource* 	pRes;
	grip 			gRes;

	gRes = ALoad(resType, resNum);
	pRes = ADerefAs(Resource, gRes);
	numCels = pRes->numHeaders;
	ASSERT(pRes->numHeaders >= cel);

	return *(pRes->GetHeader(resType == RES_CEL ? 1 : cel));
}


// check for dynamic cel, if its there, flush it
void
Scrimage::FlushDynamicCel()
{
	if (resNum & RES_DYNAMIC)
	{
		grip g = AIsLoaded(resType, resNum);
		if (g)
		{
			AFlush(g);
		}
	}
}


// Set up a color look-up table (CLUT) for this guy to use.
// Substitution will be done during CopyPixels
//
void
Scrimage::SetClut(uint cIdx, uint cSize, grip clutGrip)
{
	if (clutGrip)
	{
		// Setting a clut
		gClut = clutGrip;
		clut = ADerefAs(uchar, gClut);
		clutIdx = cIdx;
		if (cSize + cIdx > 255)
		{
			cSize = 256 - cIdx;
		}
		clutSize = cSize;
	}
	else
	{
		// Getting rid of a clut
		gClut = NULL;
		clut = NULL;
		clutIdx = 0;
		clutSize = 0;
	}
	SetState(S_CHANGED, TRUE);
}


bool
Scrimage::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&scrimageDataStart, 1, (int)&scrimageDataEnd -
				(int)&scrimageDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&scrimageDataStart, 1, (int)&scrimageDataEnd -
				(int)&scrimageDataStart, pFile);
			break;

		case AFTER_RESTORE:
			// fixup the clut
			if (gClut)
			{
				clut = ADerefAs(uchar, gClut);
			}
			break;
	}
	return(TRUE);
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
//		Guy class
//---------------------------------------------------------------------
//---------------------------------------------------------------------


// Constructor - set properties
Guy::Guy()
{
	classID = CID_GUY;
	xMoveMax = 6;
	yMoveMax = 3;
  	fAutoScale = FALSE;
	fIsActive = FALSE;
	scrim.gOwner = gSelf;
	SetName("Guy");

	baseAnim = 0;
}



// Destructor
Guy::~Guy()
{
	Cleanup();
}


// 
grip
Guy::GetScrimage()
{
	return scrim.gSelf;
}


// Hide the Guy.
void
Guy::Hide()
{
	ADeleteScrim(&scrim);
	SetState(S_HIDDEN, TRUE);
}


// Show the Guy.
void
Guy::Show()
{
	AAddScrim(&scrim);
	SetState(S_HIDDEN, FALSE);
}


// Set a state bit.
// Returns the old value of bits changed
// identical to Scrimage::SetState()
bool
Guy::SetState(uint16 flag, bool on)
{
	bool oldState = (scrim.state & flag) ? TRUE : FALSE;

	if (on)
	{
		scrim.state |= flag;
	}
	else
	{
		scrim.state &= ~flag;
	}

	return oldState;
}


// Return the value of a state bit.
// identical to Scrimage::CheckState()
bool
Guy::CheckState(uint16 flag)
{
	return (scrim.state & flag) ? TRUE : FALSE;
}


void
Guy::SetPos(coord L, coord T)
{
	scrim.x = L;
	scrim.y = T;
	
	// check if we need to change scaling
	if (fAutoScale)
	{
		scrim.scale = pGraphMgr->scaleTable.aScales[scrim.y];
	}

	SetRect();
	SetState(S_CHANGED, TRUE);
}


void
Guy::SetRes(res_t type, uint num, uint cel)
{
	res_t		oldType;
	uint		oldNum;

	// Extract information about the current resource
	oldType = scrim.resType;
	oldNum = scrim.resNum;

	// Load the new resource
	ALoad(type, num);

	// Adjust the use counts - this will purge the old resource, if
	// no one else is using it
	Object::SetResCount(oldType, oldNum, type, num);

	// Set the scrimage's properties
	scrim.resType = type;
	scrim.resNum = num;
	SetCel(cel);

	// Adjust the rectangle, if necessary, and tell graphmgr we need to update
	SetRect();
	SetState(S_CHANGED, TRUE);
}


void
Guy::SetCel(uint celNum)
{
	if (!celNum)
	{
		APanic("Invalid cel number: 0");
	}

	// if setting to last without knowing it ahead of time
	if (celNum == LAST_CEL) 
	{
		// load anim header, safely
		scrim.cel = 1;
		scrim.GetCelHeader();

		// set to last cel
		celNum = scrim.numCels;
	}

	if (scrim.cel != celNum) 
	{
		// only if changing
		scrim.cel = celNum;
		SetState(S_CHANGED, TRUE);
	}
	SetRect();
}


void
Guy::SetPri(int32 pri)
{
	node*		pNode;
	Object*	puResp;

	if (pri != scrim.priority)
	{
		pGraphMgr->scrimList.fResortList = TRUE;

		scrim.priority = pri;

		for (	pNode = lUtils.FirstKey((int32) UT_RESPONDER); 
				pNode != NULL; 
				pNode = lUtils.NextKey(pNode, (int32) UT_RESPONDER)
			)
		{
			puResp = ADerefAs(Object, (grip) pNode->value);
			puResp->ResetMsgPri(pri);
		}

		SetState(S_CHANGED, TRUE);
	}
}


void
Guy::SetScale(uint newScale)
{
	if (scrim.scale != newScale)
	{
		scrim.scale = newScale;
		SetRect();
		SetState(S_CHANGED, TRUE);
	}
}



void
Guy::OffsetPos(coord xOff, coord yOff)
{
	SetPos(scrim.x + xOff, scrim.y + yOff);
	SetRect();
	SetState(S_CHANGED, TRUE);
}


//-------------------------------------------------------------
// update guy's rectangle
//
void
Guy::SetRect()
{
	scrim.SetRect();	
}


void
Guy::SetClipRect(coord x1, coord y1, coord x2, coord y2)
{
	if (x1 | y1 | x2 | y2)
	{
		scrim.SetClipRect(x1, y1, x2, y2);
	}
	else
	{
		scrim.SetClipRect(scrim.rect.x1,
								scrim.rect.y1,
								scrim.rect.x2,
								scrim.rect.y2);
	}
	SetState(S_CHANGED, TRUE);
}


// Set up a color look-up table (CLUT) for this guy to use.
// Substitution will be done during CopyPixels
//
void
Guy::SetClut(uint cIdx, uint cSize, grip clutGrip)
{
	scrim.SetClut(cIdx, cSize, clutGrip);
}


// Flip the Guy around the origin point.  'orient' will be either
// S_FM_VERT or S_FM_HORZ, and the optional boolean indicates whether
// the flip should be done or undone (default is TRUE, do the flip).
//
void
Guy::Flip(uint16 orient, bool doflip)
{
	if (orient == S_FM_VERT || orient == S_FM_HORZ || orient == S_FM_VERT | S_FM_HORZ)
	{
		if (doflip)
		{
			if (scrim.CheckState(S_MIRROR))
			{
				// Flip and Mirror are mutually exclusive
				scrim.SetState(S_MIRROR | S_FM_VERT | S_FM_HORZ, FALSE);
			}
			scrim.SetState((uint16) (S_FLIP | orient), TRUE);
		}
		else
		{
			scrim.SetState(orient, FALSE);
			if (!scrim.CheckState(S_FM_VERT | S_FM_HORZ))
			{
				scrim.SetState(S_FLIP, FALSE);
			}
		}
		scrim.SetState(S_CHANGED, TRUE);
		SetRect();
	}
	else
	{
		APanic("Incorrect parameter passed to Flip\n");
	}
}


// Mirror the Guy within his cel rect.  'orient' will be either
// S_FM_VERT or S_FM_HORZ, and the optional boolean indicates whether
// the mirror should be done or undone (default is TRUE, do the mirror).
//
void
Guy::Mirror(uint16 orient, bool domirror)
{
	if (orient == S_FM_VERT || orient == S_FM_HORZ || orient == S_FM_VERT | S_FM_HORZ)
	{
		if (domirror)
		{
			if (scrim.CheckState(S_FLIP))
			{
				// Flip and Mirror are mutually exclusive
				scrim.SetState(S_FLIP | S_FM_VERT | S_FM_HORZ, FALSE);
			}
			scrim.SetState((uint16) (S_MIRROR | orient), TRUE);
		}
		else
		{
			scrim.SetState(orient, FALSE);
			if (!scrim.CheckState(S_FM_VERT | S_FM_HORZ))
			{
				scrim.SetState(S_MIRROR, FALSE);
			}
		}
		scrim.SetState(S_CHANGED, TRUE);
		SetRect();
	}
	else
	{
		APanic("Incorrect parameter passed to Mirror\n");
	}
}


// Set object's Message mask, subscribe to Message stream
void
Guy::Setup()
{
	Context*	pContext;

	// error check
	if (!gContext)
	{
		APanic ("No context assigned to Object");
	}

	fIsActive = TRUE;

	pContext = ADerefAs(Context, gContext);
	pContext->AddObject(gSelf, CT_MSGS|CT_SERV|CT_ROST);
}




// Clear object's event mask, cancel subscription to Event stream
void
Guy::Cleanup()
{
	if (gContext)
	{
		fIsActive = FALSE;
		Context*	pContext;
		pContext = ADerefAs(Context, gContext);
		pContext->CutObject(gSelf, CT_MSGS|CT_SERV|CT_ROST);
	}
}


// choose stander to face a point
void
Guy::FaceTo(coord xGoal, coord yGoal)
{
	ASSERT(baseAnim);

	ChooseWalker(scrim.x - xGoal, scrim.y - yGoal);
	ChooseStander();
}


// choose stander anim based on current anim
void
Guy::ChooseStander()
{
	int	facing;
	int	diff;

	ASSERT(baseAnim != 0);

	// get current direction for stander
	diff = scrim.resNum % 50;

	switch (diff)
	{
		case FACE_NE:
		case -1:
			facing = -1;
			break;

		case FACE_SW:
		case FACE_WEST:
		case 9:
			facing = 9;
			break;

		case FACE_SE:
		case FACE_SOUTH:
		case FACE_EAST:
		case 0:
			facing = 0;
			break;

		case FACE_NORTH:
		case FACE_NW:
		case -2:
			facing = -2;
			break;

		default:
			facing = 0;
			pMono->Out("\nunknown stander from %d", scrim.resNum);
			break;
	}

	SetRes(RES_ANIM, baseAnim + facing, 1);
}


// choose walker anim based on the direction of cur x,y to
// point offset by xChange,yChange (which are signed)
void
Guy::ChooseWalker(coord xChange, coord yChange)
{
	uint		newAnim;
	int		facing = _STAND;
	coord		xDir;
	coord		yDir;

	ASSERT(baseAnim != 0);

	newAnim = baseAnim;
	facing = _STAND;

	// check which directions to move in
	xDir = (xChange > 0) ? 1 : -1;
	yDir = (yChange > 0) ? 1 : -1;

	// going east/west/north/south
	if ((xDir ==  1) && (yDir ==  1))
	{
		facing = FACE_SE;
	}
	if ((xDir == -1) && (yDir ==  1))
	{
		facing = FACE_SW;
	}
	if ((xDir ==  1) && (yDir == -1))
	{
		facing = FACE_NE;
	}
	if ((xDir == -1) && (yDir == -1))
	{
		facing = FACE_NW;
	}

	// get angle, vertical, horizontal, or diagonal

	xChange = abs(xChange) * 2;
	yChange = abs(yChange) * 2;

	/****************** not using north or south walkers
	if (xChange < yChange/2)
	{
		// angle is vertical
		if ((facing == FACE_NE) || (facing == FACE_NW))
		{
			facing = FACE_NORTH;
		}
		else if ((facing == FACE_SE) || (facing == FACE_SW))
		{
			facing = FACE_SOUTH;
		}
	}
	else 
	******************/

	/****************** not using east or west walkers
	if (yChange < xChange/2)
	{
		// angle is horizontal
		if ((facing == FACE_NE) || (facing == FACE_SE))
		{
			facing = FACE_EAST;
		}
		else if ((facing == FACE_SW) || (facing == FACE_NW))
		{
			facing = FACE_WEST;
		}
	}
	******************/

	SetRes(RES_ANIM, baseAnim + facing, 1);
}

bool
Guy::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&guyDataStart, 1, (int)&guyDataEnd -
				(int)&guyDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&guyDataStart, 1, (int)&guyDataEnd -
				(int)&guyDataStart, pFile);
			break;
	}
	return(TRUE);
}


//-------------------------------------------------------------
//-------------------------------------------------------------
//		Utilizer Methods
//-------------------------------------------------------------
//-------------------------------------------------------------

Utilizer::Utilizer(void)
{
	classID = CID_UTILIZER;
}

// Destructor
Utilizer::~Utilizer()
{
	node 		*pNode;
	Object	*pObject;

	for (pNode = lUtils.First(); pNode != NULL; pNode = lUtils.Next())
	{
		// free memory for the object
		pObject = ADerefAs(Object, (grip) pNode->value);
		delete pObject;
	}

	lUtils.Release();
}

bool
Utilizer::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&utilizerDataStart, 1, (int)&utilizerDataEnd -
				(int)&utilizerDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&utilizerDataStart, 1, (int)&utilizerDataEnd -
				(int)&utilizerDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
Utilizer::ScriptAborting()
{
	// force cues, which will be flushed by script::abort
	AbortAllUtils();	

	Periodic::ScriptAborting();
}


//-------------------------------------------------------------
//	Setup() adds a dynamically created utility to list

void
Utilizer::SetupUtil(Utility* pUtil)
{
	pUtil->gOwner = gSelf;

	// call Utility::Setup() before adding to list in case
	// the utility wants to removed utils of the same type
	pUtil->Setup(this);

	// add with key
	lUtils.Add((void*) (pUtil->gSelf), pUtil->type);
}


//-------------------------------------------------------------
//	Cleanup() removes from list and delete dynamic utility instance

void
Utilizer::CleanupUtil(grip gUtil)
{
	Utility*	pUtil;

	pUtil = ADerefAs(Utility, gUtil);

	// if utility has any special cleanup
	pUtil->Cleanup();

	// remove from list
	lUtils.Delete((void*) gUtil);

	// cue successor
	if (pUtil->gSuccessor)
	{
		APostNotice(N_CUE, pUtil->gSuccessor);
	}

	// delete dynamic instance
	pUtil = ADerefAs(Utility, gUtil);
	delete pUtil;
}



//-------------------------------------------------------------
//	CleanupUtils() cleans up all utils of a given type

void
Utilizer::CleanupUtils(util_t utilType)
{
	uint		i;
	uint		nextNode = 0;
	node*		pCurrent;

	for (i = 1; i <= lUtils.count; i = nextNode)
	{
		pCurrent = lUtils.GetPNode(i);
		if (pCurrent->key == utilType)
		{
			CleanupUtil((grip) pCurrent->value);
			// keep same index since one has been removed
			nextNode = i;
		}
		else
		{
			nextNode = i + 1;
		}
	}
}



//-------------------------------------------------------------
//	CleanupAllUtils() cleans up all utils

void
Utilizer::CleanupAllUtils()
{
	uint		i;
	uint		nextNode = 0;
	node*		pCurrent;

	for (i = 1; i <= lUtils.count; i = nextNode)
	{
		pCurrent = lUtils.GetPNode(i);
		CleanupUtil((grip) pCurrent->value);

		// keep same index since one has been removed
		nextNode = i;
	}
}


//-------------------------------------------------------------
//	Cleanup without cueing

void
Utilizer::AbortUtil(grip gUtil)
{
	Utility*	pUtil;

	pUtil = ADerefAs(Utility, gUtil);

	// don't cue!
	pUtil->gSuccessor = NULL;

	// if utility has any special cleanup
	pUtil->Cleanup();

	// remove from list
	lUtils.Delete((void*) gUtil);

	// delete dynamic instance
	pUtil = ADerefAs(Utility, gUtil);
	delete pUtil;
}



//-------------------------------------------------------------
//	AbortUtils() cleans up all utils of a given type

void
Utilizer::AbortUtils(util_t utilType)
{
	uint		i;
	uint		nextNode = 0;
	node*		pCurrent;

	for (i = 1; i <= lUtils.count; i = nextNode)
	{
		pCurrent = lUtils.GetPNode(i);
		if (pCurrent->key == utilType)
		{
			AbortUtil((grip) pCurrent->value);
			// keep same index since one has been removed
			nextNode = i;
		}
		else
		{
			nextNode = i + 1;
		}
	}
}


//-------------------------------------------------------------
//	AbortAllUtils() cleans up all utils, without cueing

void
Utilizer::AbortAllUtils()
{
	uint		i;
	uint		nextNode = 0;
	node*		pCurrent;

	for (i = 1; i <= lUtils.count; i = nextNode)
	{
		pCurrent = lUtils.GetPNode(i);
		AbortUtil((grip) pCurrent->value);

		// keep same index since one has been removed
		nextNode = i;
	}
}


//-------------------------------------------------------------
//	Service() services all utils

void
Utilizer::Service()
{
	// WARNING!!!!!!
	// Do not make any reference to a utility after the Cycle()
	// has been called.  The Cycle() might delete itself.
	
	grip		gUtil;
	
	// Service all utilities
	for (gUtil = (grip) lUtils.FirstValue();
		 gUtil != NULL;
		 gUtil = (grip) lUtils.NextValue())
	{
		((Utility *) ADeref(gUtil))->Cycle();
	}
}



//-------------------------------------------------------------
//-------------------------------------------------------------
//		Utilities 
//-------------------------------------------------------------
//-------------------------------------------------------------

Utility::Utility()
{
	classID = CID_UTILITY;
	SetName("Utility");
	gOwner = gSuccessor = 0;
}


Utility::~Utility()
{
}

bool
Utility::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&utilityDataStart, 1, (int)&utilityDataEnd -
				(int)&utilityDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&utilityDataStart, 1, (int)&utilityDataEnd -
				(int)&utilityDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
Utility::Service()
{
}


//	the Owning object calls Utility::Setup() to start it up
void
Utility::Setup(void* pOwner)
{
	pOwner = pOwner;	// Gets rid of unreferenced warning

	APanic("Utility::Setup() should be redefined");
}


void
Utility::Cleanup()
{
}



//-------[ TLineTo ]--------------------------------------------

#define	SCALE_FACTOR	256

// default constructor
//
TLineTo::TLineTo()
{
	classID = CID_TLINETO;
	type = UT_LINE_TO;

	xGoal = yGoal = 0;
	gSuccessor = 0;
	SetName("TLineTo");
}


// constructor used from script state
//
TLineTo::TLineTo(coord x, coord y, grip gNext, ticks_t ticksInt)
{
	type = UT_LINE_TO;

	xGoal = x;
	yGoal = y;
	gSuccessor = gNext;
	SetInterval(ticksInt);
	SetName("TLineTo");
}


TLineTo::~TLineTo()
{
}


// Scrimage is calling to say this, "this is where I am. You know where I'm
//		going, so start up".
//
void
TLineTo::Setup(void* pvGuy)
{
	int32		minorChange;
	int32		majorChange;
	Guy*		pGuy = (Guy*) pvGuy;

	// TLineTo pre-empts any prior existing TLineTo utils
	pGuy->CleanupUtils(UT_LINE_TO);

	// where is the Guy starting from?
	xStart = pGuy->scrim.x;
	yStart = pGuy->scrim.y;

	// how far to get where i need to go next?
	majorChange = xGoal - xStart;
	minorChange = yGoal - yStart;

	// check which directions to move in
	xDir = (majorChange > 0) ? 1 : -1;
	yDir = (minorChange > 0) ? 1 : -1;

	// select correct facing, if will move
	if (majorChange || minorChange)
	{
		//ChooseAnim(pGuy, xChange, yChange);
		ChooseAnim(pGuy, majorChange, minorChange);
	}

	fMajorIsHorz = TRUE;

	// swap if major axis is veritical
	if (abs(minorChange) > abs(majorChange))
	{
		coord temp = minorChange;
		minorChange = majorChange;
		majorChange = temp;
		fMajorIsHorz = FALSE;
	}

	// scale up, calc fraction
	if (majorChange)
	{
		fraction = abs((minorChange * SCALE_FACTOR) / majorChange);
	} else {
		fraction = 0;
	}

	fractionSum = 0;
}


// incrementing in a line, following Bresenham's algorythm
//
void
TLineTo::Service()
{
	Guy*	pGuy;
	coord		xDist = 0;
	coord		yDist = 0;
	bool		fArrived = FALSE;

	if (!gOwner)
	{
		return;
	}
	pGuy = ADerefAs(Guy, gOwner);

	// calc distances to travel on each axis
	if (fMajorIsHorz)
	{
		while ((xDist < pGuy->xMoveMax) && (yDist <= pGuy->yMoveMax))
		{
			// check for arrival at goal
			if ((pGuy->scrim.x+(xDist*xDir)) == xGoal)
			{
				fArrived = TRUE;
				break;
			}

			// bump along major axis
			xDist++;

			// check minor axis
			fractionSum += fraction;
			while (fractionSum > SCALE_FACTOR)
			{
				// fraction accumalated to whole, bump along minor axis
				yDist++;
				fractionSum -= SCALE_FACTOR;
			}
		}
	}
	else 
	{
		while ((yDist < pGuy->yMoveMax) && (xDist <= pGuy->xMoveMax))
		{
			// check for arrival at goal
			if ((pGuy->scrim.y+(yDist*yDir)) == yGoal)
			{
				fArrived = TRUE;
				break;
			}

			// bump along major axis
			yDist++;

			// check minor axis
			fractionSum += fraction;
			while (fractionSum > SCALE_FACTOR)
			{
				// fraction accumalated to whole, bump along minor axis
				xDist++;
				fractionSum -= SCALE_FACTOR;
			}
		}
	}

	// multipy distance by direction
	xDist *= xDir;
	yDist *= yDir;

	// if walking, animate forward
	AdvanceCel(pGuy);

	pGuy->OffsetPos(xDist, yDist);

	// check if we have arrived at our goal
	if (fArrived)
	{
		// select stander
		ChooseAnim(pGuy, 0, 0);

		pGuy->CleanupUtil(gSelf);
	}
}


void
TLineTo::AdvanceCel(Guy* pGuy)
{
	pGuy = pGuy;	// Gets rid of unreferenced warning

	// stubbed, is used by WalkTo
}


void
TLineTo::ChooseAnim(Guy* pGuy, coord xChange, coord yChange)
{
	pGuy = pGuy;			// Gets rid of unreferenced warning
	xChange = xChange;
	yChange = yChange;

	// stubbed, is used by WalkTo
}

bool
TLineTo::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&tLineToDataStart, 1, (int)&tLineToDataEnd -
				(int)&tLineToDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&tLineToDataStart, 1, (int)&tLineToDataEnd -
				(int)&tLineToDataStart, pFile);
			break;
	}
	return(TRUE);
}


//-------[ WalkTo ]--------------------------------------------


// default constructor
//
WalkTo::WalkTo()
{
	classID = CID_WALKTO;
	type = UT_LINE_TO;

	xGoal = yGoal = 0;
	gSuccessor = 0;
	SetName("WalkTo");
}


// constructor used from script state
//
WalkTo::WalkTo(coord x, coord y, grip gNext, ticks_t ticksInt)
{
	type = UT_LINE_TO;

	xGoal = x;
	yGoal = y;
	gSuccessor = gNext;
	SetInterval(ticksInt);
	SetName("WalkTo");
}



void
WalkTo::AdvanceCel(Guy* pGuy)
{
	uint	newCelNum;

	newCelNum = pGuy->scrim.cel + 1;
	if (newCelNum > pGuy->scrim.numCels) 
	{
		newCelNum = 1;
	}

	pGuy->SetCel(newCelNum);
}



void
WalkTo::ChooseAnim(Guy* pGuy, coord xChange, coord yChange)
{
	uint	newAnim = pGuy->baseAnim;

	// check for stander
	if ((!xChange) && (!yChange))
	{
		pGuy->ChooseStander();
		return;
	}

	ASSERT(pGuy->baseAnim != 0);

	pGuy->ChooseWalker(xChange, yChange);
}

bool
WalkTo::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&walkToDataStart, 1, (int)&walkToDataEnd -
				(int)&walkToDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&walkToDataStart, 1, (int)&walkToDataEnd -
				(int)&walkToDataStart, pFile);
			break;
	}
	return(TRUE);
}


//-------[ Animator ]--------------------------------------------


#define	SCALE_FACTOR	256

// default constructor
Animator::Animator()
{
	classID = CID_ANIMATOR;
	SetName("Animator");

	type = UT_ANIMATOR;

	goingForward = TRUE;
	gSuccessor = 0;
	SetName("Animator");
}


// constructor used from script state
//
Animator::Animator(animator_t spin, grip gNext, ticks_t ticksInt)
{
	SetName("Animator");

	type = UT_ANIMATOR;

	goingForward = TRUE;
	gSuccessor = 0;

	// check for invalid gSucessor
	if (gNext) 
	{
		switch (spin)
		{
			case AN_OSCILLATE:
			case AN_FORWARD:
			case AN_BACKWARD:
			case AN_RANDOM:
				APanic ("this type of Animator does NOT use a sucessor");
				break;
		}
	}

	anType = spin;

	gSuccessor = gNext;
	SetInterval(ticksInt);
	SetName("Animator");
}


Animator::~Animator()
{
}


// Scrim
//
void
Animator::Setup(void* pvGuy)
{
	// Animator pre-empts any prior existing Animator utils
	((Guy*) pvGuy)->CleanupUtils(UT_ANIMATOR);
}


void
Animator::Service()
{
	Guy*		pGuy;
	coord		xDist = 0;
	coord		yDist = 0;
	bool		fArrived = FALSE;
	uint		newCelNum;

	pGuy = ADerefAs(Guy, gOwner);

 	newCelNum = pGuy->scrim.cel;

	switch (anType)
	{
		case AN_NONE:
			break;

//		case AN_RANDOM:
//			newCelNum = ARandom(pGuy->scrim.numCels) + 1;
//			break;

		case AN_TO_END:
			newCelNum = pGuy->scrim.cel + 1;
			if (newCelNum >= pGuy->scrim.numCels) 
			{
				newCelNum = pGuy->scrim.numCels;
				pGuy->CleanupUtil(gSelf);
			}
			break;

		case AN_TO_START:
			newCelNum = pGuy->scrim.cel - 1;
			if (newCelNum <= 1)
			{
				newCelNum = 1;
				pGuy->CleanupUtil(gSelf);
			}
			break;

		case AN_FORWARD:
			newCelNum = pGuy->scrim.cel + 1;
			if (newCelNum > pGuy->scrim.numCels) 
			{
				newCelNum = 1;
			}
			break;

		case AN_BACKWARD:
			newCelNum = pGuy->scrim.cel - 1;
			if (newCelNum < 1) 
			{
				newCelNum = pGuy->scrim.numCels;
			}
			break;

		case AN_OSCILLATE:
			if (goingForward)
			{
				newCelNum = pGuy->scrim.cel + 1;
				if (newCelNum >= pGuy->scrim.numCels) 
				{
					newCelNum = pGuy->scrim.numCels;
					goingForward = FALSE;
				}
			}
			else 
			{
				newCelNum = pGuy->scrim.cel - 1;
				if (newCelNum < 1)
				{
					newCelNum = 1;
					goingForward = TRUE;
				}
			}
			break;

		case AN_LAST:
		default:
			APanic ("Unknown Animator type");
	}

	pGuy->SetCel(newCelNum);
}

bool
Animator::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&animatorDataStart, 1, (int)&animatorDataEnd -
				(int)&animatorDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&animatorDataStart, 1, (int)&animatorDataEnd -
				(int)&animatorDataStart, pFile);
			break;
	}
	return(TRUE);
}
