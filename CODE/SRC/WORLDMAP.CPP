//
// WORLDMAP.CPP
//
// April 11, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//
//
//
//----[]-------------------------------------------------------------

#include "alldefs.hpp"
#include "api.hpp"
#include "apigraph.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "mouse.hpp"
#include "tigre.hpp"
#include "viewport.hpp"
#include "maps.hpp"
#include "worldmap.hpp"

extern World	*pWorld;

#define	NEUTRAL_UNIT (255+SIDE0)
#define	RED_UNIT		 (255+SIDE1)
#define	BLUE_UNIT	 (255+SIDE2)


WorldMap::WorldMap(void)
{
	worldRect.Set(255, 44, 318, 163);
	worldMouseRect.Set(264, 50, 306, 154);
}


WorldMap::~WorldMap(void)
{
	if(gContext)
	{
		Context*	pContext;
		pContext = ADerefAs(Context, gContext);
		pContext->CutObject(gSelf, CT_MSGS|CT_SERV);
	}
}


void
WorldMap::Setup(grip gContext, grip gMasterCel)
{
	// Store grip into viewport
	gVPCel = gMasterCel;

	// attach ourselves to context
	SetContext(gContext);

	shadCurPosX = 100; // this weird # will allow MoveShadowCursor to init.
	shadCurPosY = 100;

	Guy	*pGuy;
	pGuy = &shadowCursor;
	pGuy->SetRes(RES_ANIM,102,5);
	MoveShadowCursor(0,0);   	//init. default
	pGuy->SetContext(gContext);
	pGuy->Setup();
	pGuy->SetPri(PRI_INTERFACE + 1);

	worldCursor.Setup(gContext);
}

// Move box cursor
void
WorldMap::MoveCursor(uint32 newTileX, uint32 newTileY)
{
	// adjust ViewPort's upper-left coords to center based coords.
	newTileX += 6;
	newTileY += 4;

	// move cursor WITHOUT it trying to then move the ViewPort
	worldCursor.SetTile(newTileX, newTileY, FALSE);
}


// Set shadow cursor's anim
void
WorldMap::SetShadowAnim(shadowCel_t cel)
{
	shadowCursor.SetCel(cel);
}


// Move map's shadow cursor
void
WorldMap::MoveShadowCursor(uint32 newTileX, uint32 newTileY)
{
	uint32	ptX,ptY;
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);

	if (newTileX != shadCurPosX || newTileY != shadCurPosY)
	{
		shadCurPosX = newTileX;
		shadCurPosY = newTileY;

		ptX = shadCurPosX * UNIT_WIDTH  + pWorldMap->worldRect.x1;
		ptY = shadCurPosY * UNIT_HEIGHT + pWorldMap->worldRect.y1;

		// the position has changed
		shadowCursor.SetPos(ptX,ptY);
	}
}


// Not needed anymore as this functionality has been moved to the
//  low-level worldmap tile draw routine.
//void
//WorldMap::GetMapColors()
//{
//	uint16	x,y, *wptr;
//	Map		*pMap = ADerefAs(Map, pWorld->gMap);
//
//	wptr = *wArray;
//
//	// x,y array indexes are reversed from screen x,y coords.
//
//	//Loop thru tiles and get their color
//	for (x=0; x < WORLD_WIDTH; x++)
//		for (y=0; y < WORLD_HEIGHT; y++)
//		{
//			if (pMap->IsTerrainHidden(x,y))
//				*wptr++ = NOT_SEEN;
//			else
//				*wptr++ = pMap->GetTerrainColor(x,y);
//		}
//}
//
//
//void
//WorldMap::GetUnits()
//{
//	uint16 *wptr;
//
//	wptr = *wArray;
//
//	//Loop thru tiles and overlay unit's color if on that tile
//
////	for (int x=0; x < WORLD_WIDTH; x++)
////		for (int y=0; y < WORLD_HEIGHT; y++)
////			;//*wptr++ = pMap->GetUnitColor(x,y);
//
//
//}
//

// redraw world map
void
WorldMap::Draw(void)
{
	uint32	loopX, loopY;

	//GetMapColors();
	//GetUnits();

	for(loopY = 0; loopY < WORLD_HEIGHT; loopY++)
		for(loopX = 0; loopX < WORLD_WIDTH; loopX++)
			Draw(loopX, loopY, FALSE);

	AUpdateRect(&worldRect);
}

//redraw one tile in world map
void
WorldMap::Draw(uint32 xPos, uint32 yPos, bool fUpdateRect, bool fGroupie)
{
	uint32		i,hc,vc,pos,width, player;
//	uint16		*wptr;
	uint16		color;
	uchar			*pVPCel,*pVPData;
//	CelHeader	celHeader;
	CelHeader	*pCelH;
	grip			gUnit;
	Unit			*pUnit;

	if (pWorld->map.IsTerrainHidden(xPos,yPos))
		return;

	//from the old 'put all in an array first' idea
	//	wptr = &wArray[xPos][yPos];
	//
	//	if (*wptr == NOT_SEEN)
	//		return;


	// is there a unit here? if so, skip getting terrain info
	gUnit = pWorld->unitLib.GetUnitGrip(xPos,yPos);
	if(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
		player = pUnit->player;
		if(player == SIDE3)
			player = SIDE0;
		color = (uint16) (player + 255);
	}
	else
		color = (uint16) pWorld->map.GetTerrainColor(xPos,yPos); //no unit so get terrain info

	// copy cel header
	pVPCel = AGetResData(gVPCel);
	pCelH = (CelHeader*) pVPCel;

	//celHeader = *((CelHeader*) pVPCel);
	pVPData = pVPCel + sizeof(CelHeader);

	vc = worldRect.y1 + yPos * UNIT_HEIGHT;
	hc = worldRect.x1 + xPos * UNIT_WIDTH;

	if(fGroupie)
	{
	 	CopyCel((CelHeader*)pVPCel, hc, vc, RES_ANIM, 102, 5, FALSE);
	}
	else
	{
		if (color > 255)
		{
			if (color == NEUTRAL_UNIT)
				CopyCel((CelHeader*)pVPCel, hc, vc, RES_ANIM, 102, 6, FALSE);
			else
			if (color == RED_UNIT)
				CopyCel((CelHeader*)pVPCel, hc, vc, RES_ANIM, 102, 3, FALSE);
			else
			if (color == BLUE_UNIT)
				CopyCel((CelHeader*)pVPCel, hc, vc, RES_ANIM, 102, 4, FALSE);
		}
		else
		{
			width = pCelH->width;
			pos = hc + width * vc;

			// a real color
			for (i=0; i < UNIT_HEIGHT; i++)
			{
				memset(pVPData + L2R(pos), color, L2R(UNIT_WIDTH));
				pos += width;
			}
		}
	}
	if(fUpdateRect)
		AUpdateRect(&worldRect);
}


//*******************************************************************

void
WorldCursor::Setup(grip gContext)
{
	Context	*pContext;

	msgMask =  E_MOUSE_DOWN | E_MOUSE_UP;
	fIsMouseDown = FALSE;

	// attach ourselves to context
	SetContext(gContext);

	SetRes(RES_ANIM,102,2);	
	SetContext(gContext);
	SetPri(PRI_INTERFACE + 1);
	FindStart();
	Guy::Setup();
	pContext = ADerefAs(Context,gContext);
	pContext->CutObject(gSelf, CT_SERV);
}

void
WorldCursor::Cycle(void)
{
	coord theX = pMouse->GetX();
	coord theY = pMouse->GetY();
	SetPos(theX, theY, TRUE); // setpos does bounds checking
}


bool
WorldCursor::HandleMsg(Message* pMsg)
{
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	uint32	tileX, tileY;

 	if (pWorldMap->worldRect.Contains(pMsg->event.x, pMsg->event.y))
 	{
		switch (pMsg->type)
		{
		case MSG_EVENT:
			switch (pMsg->event.type)
			{
				case E_MOUSE_DOWN:

					if(pWorld->worldEnder && pWorld->worldEnder != SIDE0)
					{
						// game is about to end - don't allow any input
						return(TRUE);
					}

					if (!fIsMouseDown)
					{
						if(pMsg->event.value == RIGHT_BTN)
						{
							if(pWorld->vPort.currentVerb)
							{
								// clear the current verb
								pWorld->DeselectAllVerbButtons();
								pWorld->vPort.SetCurrentVerb(NO_ACTION);
								break;
							}
							else
							{
								GetTilePos(pMsg->event.x, pMsg->event.y, &tileX, &tileY, FALSE);
								pWorld->vPort.DoObviousVerbProcess(tileX, tileY);
								break;
							}
						}
						else if(pWorld->vPort.currentVerb && pMsg->event.value == LEFT_BTN)
						{
							// select target - dont move viewport or WorldMap cursor
							GetTilePos(pMsg->event.x, pMsg->event.y, &tileX, &tileY, FALSE);

							if(pWorld->statBoxCurrMode != TERRAIN)
							{
//								pWorld->vPort.MoveCursor(tileX, tileY);
							}
							else
							{
								pWorld->vPort.MoveCursor(tileX, tileY, TRUE);
								pWorld->vPort.ProcessVerb();
								pWorld->vPort.RestoreSourceAsTarget();
								break;
							}
						}

						fIsMouseDown = TRUE;
						saveMouseRect.Copy(&(pMouse->mouseLimits));

						// Lock boundaries on mouse
						pMouse->SetLimits(&(pWorldMap->worldMouseRect)); //calls bounds check also

						// Turn on service to make guy draggable
						if (gContext)
							(ADerefAs(Context, gContext))->AddObject(gSelf, CT_SERV);
					}

					return TRUE;
					break;

				case E_MOUSE_UP:
					if(fIsMouseDown)
					{
						fIsMouseDown = FALSE;
		
						// Turn off service to make guy undraggable
						if (gContext)
							(ADerefAs(Context, gContext))->CutObject(gSelf, CT_SERV);

						// Unlock boundaries on mouse
						pMouse->SetLimits(&saveMouseRect);
				
						SetPos(pMsg->event.x, pMsg->event.y, TRUE);
					}
					else if(pWorld->vPort.fFenceMode)
					{
						// someone clicked inside viewport and dragged out here -
						// clean up fencing.
						pWorld->vPort.EndFencing(pMsg->event.x, pMsg->event.y);
					}
					return TRUE;
					break;
				}
			}
	}

	return FALSE;
}


void
WorldCursor::FindStart()
{
	uint32 tX=0,tY=0;

	// set to some illegal tile number and let SetPos() init them
	tilePosX = 100;
	tilePosY = 100;


	// need to scan tile map for portal here



	// adjust ViewPort's upper-left coords to center based coords.
	tX += 5;
	tY += 3;

	SetTile(tX,tY,TRUE);
}


// For converting mouse clicks to tile coords
void
WorldCursor::GetTilePos(uint32 newX, uint32 newY, uint32 *pTileX, uint32 *pTileY, bool fCheckBounds)
{
	uint32 tileX,tileY;
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);

	tileX = ((newX) - pWorldMap->worldRect.x1) / UNIT_WIDTH;
	tileY = ((newY) - pWorldMap->worldRect.y1) / UNIT_HEIGHT;

	// find legal tile
	if(fCheckBounds)
		CheckBounds(&tileX,&tileY);

	*pTileX = tileX;
	*pTileY = tileY;
}


// For taking mouse clicks
void
WorldCursor::SetPos(coord newX, coord newY, bool syncVP)
{
	uint32 tileX,tileY;
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);

	tileX = ((newX) - pWorldMap->worldRect.x1) / UNIT_WIDTH;
	tileY = ((newY) - pWorldMap->worldRect.y1) / UNIT_HEIGHT;

	// find legal tile
	CheckBounds(&tileX,&tileY);

	SetTile(tileX,tileY,syncVP);
}


void
WorldCursor::SetTile(uint32 tileX, uint32 tileY, bool syncVP)
{
	uint32	ptX,ptY;
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);

	if (tileX != tilePosX || tileY != tilePosY)
	{
		tilePosX = tileX;
		tilePosY = tileY;

		ptX = tileX * UNIT_WIDTH  + pWorldMap->worldRect.x1;
		ptY = tileY * UNIT_HEIGHT + pWorldMap->worldRect.y1 + 1;

		// the position has changed
		Guy::SetPos(ptX,ptY);

		//Position region map to match
		if (syncVP)
		{
			pWorld->vPort.MoveView(tileX-6,tileY-4);
		}
	}
}


void
WorldCursor::CheckBounds(uint32 *tileX, uint32 *tileY)
{
	// make sure the tile cursor is centered on is away from the edge of screen
	if (*tileX < 5) 
		 *tileX = 5;	
	else 
		if (*tileX > 26)
			 *tileX = 26;

	if (*tileY < 2) 
		 *tileY = 2;	
	else 
		if (*tileY > 36)
			 *tileY = 36;
}
