//
// MOUSESCR.CPP
//
//	Scrimage based Mouse
//
// November 24, 1993
// WATCOM: October 25, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All Rights Reserved.
//
// If you want to use the mouse, you MUST do the following:
//
// These must be done in the order shown.
//
//	// the mouse must be created after the graph manager
//	TRACK_MEM("Mouse");	new MouseScr;
//	pMouse->Init(0,0,639,479);
//	pMouse->SetRes(RES_ANIM,9997,1);
//	pMouse->Show();
//
// Right before GraphMgr::Animate, the following must be done.
//
// - call Mouse::UpdatePointer()
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
#include "apimem.hpp"
#include "jstream.hpp"
#include "mousescr.hpp"
#include	"savemgr.hpp"


//----[ Mouse Class ]------------------------------------------------



//----[ Mouse Class ]------------------------------------------------



MouseScr::MouseScr()
{
	// set priority to be on top of everything
	SetPri(9999);
}


MouseScr::~MouseScr()
{
	if (!hideCount)
	{
		// mouse is being shown, so hide it
		Hide();
	}
}


coord
MouseScr::GetX()
{ 
	return guy.scrim.x; 
}


coord
MouseScr::GetY()		
{ 
	return guy.scrim.y; 
}


// This is mainly for Restore Game.
// Caution should be used when using this,
// because it does not update the Scrimage.
// You should probably use SetPos.
//
void	
MouseScr::SetX(coord setX)
{ 
	guy.scrim.x = setX; 
}


// This is mainly for Restore Game.
// Caution should be used when using this,
// because it does not update the Scrimage.
// You should probably use SetPos.
//
void
MouseScr::SetY(coord setY)		
{ 
	guy.scrim.y = setY; 
}


int
MouseScr::GetCel()		
{ 
	return guy.scrim.cel; 
}


res_t	
MouseScr::GetResType()
{
	return guy.scrim.resType;
}


uint	
MouseScr::GetResNum()
{
	return guy.scrim.resNum;
}


// this is the routine that actually shows the pointer.
// this is protected on purpose.  If you want to 
// Show the pointer, call Show();
void
MouseScr::ShowPointer()
{
	guy.Show();
}

// this is the routine that actually hides the pointer.
// this is protected on purpose.  If you want to 
// Hide the pointer, call Hide();
void
MouseScr::HidePointer()
{
	guy.Hide();
}


// if the pointer rect intersects the passed rect, protect
// the pointer by Hiding the mouse.  If in a single buffered
// environment, Pause should probably be called before this call.  
// Return values:  true  - pointer protected
//                 false - pointer did not need protection
bool
MouseScr::Protect(bool protect, Rectangle *pRect)
{
	// we really don't want to do this is a scrimage environment

	// take care of unreference warnings
	protect = protect;
	pRect = pRect;

	return false;
}


void
MouseScr::SetRes(res_t type, uint num, uint iCel, uint iScale)
{
	SetScale(iScale);
	guy.SetRes(type,num,iCel);	
}


void
MouseScr::SetScale(uint newScale)
{
	guy.SetScale(newScale);
}


void
MouseScr::SetPri(int32 pri)
{
	guy.SetPri(pri);
}


// this routine moves the pointer to it's new location.
// x and y should be set prior to calling this routine.
void
MouseScr::MovePointer(void)
{
	if (!hideCount)
	{
		guy.SetPos(x,y);
	}
}


void
MouseScr::UpdatePointer()
{
	int16	moveX = 0, moveY = 0;
	coord	oldX, oldY;
	coord	newX, newY;

	if (status & MST_NO_RES)
	{
		// if no resolution is set, end program
		APanic("No mouse resolution set");
	}

	// In an effort to make physical mouse movement the same 
	// regardless of screen resolution, the following rules are
	// used.
	//
	// If x resolution < 320, only use half the movement 
	// If y resolution < 200, only use half the movement 

	if (screenLimits.x2 < 321)
	{
		moveX += mickeysLeftX;

		// save the mickey that gets trunced
		if (moveX & 1)
		{
			mickeysLeftX = 1;
		}
		else
		{
			mickeysLeftX = 0;
		}

 		moveX >>= 1;
	}

	if (screenLimits.y2 < 201)
	{
		moveY += mickeysLeftY;
		
		// save the mickey that gets trunced
		if (moveY & 1)
		{
			mickeysLeftY = 1;
		}
		else
		{
			mickeysLeftY = 0;
		}

 		moveY >>= 1;
	}

	oldX = GetX();
	oldY = GetY();

	// update the coordinates
	SetX(oldX + moveX);
	SetY(oldY + moveY);

	// make sure that the pointer is still within our bounds
	CheckBounds();

	if (!hideCount) 
	{
		newX = GetX();
		newY = GetY();
		if (newX != oldX || newY != oldY)
		{
			// the position has changed
			guy.SetPos(newX,newY);
		}
	}
}


// stop the mouse from moving on the screen, but don't turn off
// interrupts.  If the mouse is paused, save what the coordinates
// should be.  Restore the real coordinates when the mouse is unpaused.

void
MouseScr::Pause(bool pauseIt)
{
	// stub

	// take care of unreferenced warning
	pauseIt = pauseIt;
}

//----[ Saver Routine ]----------------------------------------------

bool
MouseScr::Save(uint16 state, nlohmann::json& root)
{
	switch(state)
	{
		case BEFORE_SAVE:
		case AFTER_SAVE:
		case BEFORE_RESTORE:
		case AFTER_RESTORE:
			break;

		case DURING_SAVE:
		case DURING_RESTORE:
		{
			JsonStream js{root["mousescr"], (state == DURING_SAVE)};

			js.sync(screenLimits.x1, "sl_x1");
			js.sync(screenLimits.y1, "sl_y1");
			js.sync(screenLimits.x2, "sl_x2");
			js.sync(screenLimits.y2, "sl_y2");

			js.sync(mouseLimits.x1, "ml_x1");
			js.sync(mouseLimits.y1, "ml_y1");
			js.sync(mouseLimits.x2, "ml_x2");
			js.sync(mouseLimits.y2, "ml_y2");

			js.sync(guy.scrim.x, "x");
			js.sync(guy.scrim.y, "y");
			js.sync(hideCount, "hideCount");
			js.syncEnum(guy.scrim.resType, "resType");
			js.sync(guy.scrim.resNum, "resNum");
			js.sync(guy.scrim.cel, "cel");
			js.sync(guy.scrim.rect.x1, "rect_x1");
			js.sync(guy.scrim.rect.y1, "rect_y1");
			js.sync(guy.scrim.rect.x2, "rect_x2");
			js.sync(guy.scrim.rect.y2, "rect_y2");

			if (!js.saving)
			{
				SetLimits(&mouseLimits);
				ClearMickeys();
				CheckBounds();
			}
			break;
		}
	}
	return false;
}

