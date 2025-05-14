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
#include "mousescr.hpp"
#include	"savemgr.hpp"


//----[ Mouse Interrupt Routine ]------------------------------------

static void	(_loadds far* functionPtr) (int, int, int, int);

// The interrupt routine needs to be far, with respect to the calling
// function (main).

#pragma off (check_stack)

static void _loadds far MouseInterrupt (int mask, int curposy, int state, int curposx)
{
	if (pMouse)
	{
		#pragma aux MouseInterrupt parm [eax edx ebx ecx]

		state = state;		// Gets rid of unreferenced warning
		curposx = curposx;
		curposy = curposy;

		// copy this to a global before we change the stack and lose it.
		MIntMask = mask;

		// We rely on PostEvent to poll the mouse. This may lead 
		// to slight inaccurracies.

		// setup our own stack
		#pragma aux SET_NEW_STACK =					\
			"push	ebp" \
			"push es" \
			"push ds" \
			"pop	es"						\
			"mov	word ptr oldStackPtr + 4,ss"		\
			"mov	dword ptr oldStackPtr,esp"		\
			"lss	esp, newStackPtr"		\
      	"mov  ebp,esp"; 
		SET_NEW_STACK();

		if (AEvtMgr())
		{
			// default (until proven 'right')
			MIntValue = LEFT_BTN;

			switch (MIntMask & BTNS_UP_DOWN)
			{
				case MSM_BTN_DOWN_R:
					MIntValue = RIGHT_BTN;
					// Fall thru...

				case MSM_BTN_DOWN_L:
					APostEvent(E_MOUSE_DOWN, MIntValue, FALSE);
					break;

				case MSM_BTN_UP_R:
					MIntValue = RIGHT_BTN;
					// Fall thru...

				case MSM_BTN_UP_L:
					APostEvent(E_MOUSE_UP, MIntValue, FALSE);
					break;

				default:
					// this should never happen
					//ASSERT(FALSE);
					break;
			}
		}

		#pragma aux RESET_STACK =		\
			"lss	esp, oldStackPtr" \
			"pop	es"	\
			"pop	ebp";
		RESET_STACK();
	}
}



//----[ Mouse Class ]------------------------------------------------



MouseScr::MouseScr()
{
	union
	{
		REGS	inRegs;
		REGS	outRegs;
	};
	SREGS	sRegs;

	// Setup the interrupt routine
	memset(&sRegs, NULL, sizeof(SREGS));
	memset(&inRegs, NULL, sizeof(REGS));

	inRegs.x.eax = 0x000C;
	inRegs.x.ecx = BTNS_UP_DOWN;
	functionPtr = MouseInterrupt;
	inRegs.x.edx = FP_OFF(functionPtr);
	sRegs.es = FP_SEG(functionPtr);
	int386x(INT_MOUSE, &inRegs, &outRegs, &sRegs);

	// mouse_init will show the driver pointer.
	// Since we are going to display the pointer using
	// animate, let's hide the mouse driver's pointer

	#pragma aux MOUSE_INIT =	\
		"mov	ax, 2"				\
		"int	33h";

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
// Return values:  TRUE  - pointer protected
//                 FALSE - pointer did not need protection
bool
MouseScr::Protect(bool protect, Rectangle *pRect)
{
	// we really don't want to do this is a scrimage environment

	// take care of unreference warnings
	protect = protect;
	pRect = pRect;

	return FALSE;
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
	int16	moveX,moveY;
	coord	oldX, oldY;
	coord	newX, newY;
	REGS	inRegs, outRegs;

	if (status & MST_NO_RES) 
	{
		// if no resolution is set, end program
		APanic("No mouse resolution set");
	}

	// get the motion counters
	memset(&inRegs, NULL, sizeof(REGS));
	memset(&outRegs, NULL, sizeof(REGS));
	inRegs.x.eax = 0x000b;
	int386(INT_MOUSE, &inRegs, &outRegs);
	moveX = (int16) outRegs.x.ecx;
	moveY = (int16) outRegs.x.edx;

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
MouseScr::Save(uint16 state, FILE *fp)
{
	bool		error = FALSE;
	grip		gMgr = 0;
	MouseScr	*pTempMgr;
	uint16	i;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
			// write out the mouse class
			if (fwrite(pMouse, 1, sizeof(MouseScr), fp) != sizeof(MouseScr))
			{
				// some sort of error
				error = TRUE;
				break;
			}

			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			break;

		case DURING_RESTORE:
			// read old mouse into temp
			TRACK_MEM("TmpMouseMgr");	gMgr = AMalloc(sizeof(MouseScr));
			pTempMgr = ADerefAs(MouseScr, gMgr);

			// run in a while loop so that we can break on an error
			// and clean up properly
			while (TRUE)
			{
				// read the graph manager
				if (fread(pTempMgr, 1, sizeof(MouseScr), fp) != sizeof(MouseScr))
				{
					// some sort of error
					error = TRUE;
					break;
				}

				screenLimits.Copy(&pTempMgr->screenLimits);
				SetLimits(&pTempMgr->mouseLimits);

				SetX(pTempMgr->guy.scrim.x);
				SetY(pTempMgr->guy.scrim.y);

				// clear the motion (mickeys) counters
				ClearMickeys();

				// make sure that the pointer is still within our bounds
				CheckBounds();

				if (hideCount > pTempMgr->hideCount)
				{
					// hide this some more
					for (i = pTempMgr->hideCount; i < hideCount; i++)
					{
						Hide();
					}
				}
				else
				{
					if (hideCount < pTempMgr->hideCount)
					{
						// show this some more
						for (i = hideCount; i < pTempMgr->hideCount; i++)
						{
							Show();
						}
					}
				}

				// set the anim
				guy.scrim.resType = pTempMgr->guy.scrim.resType;
				guy.scrim.resNum = pTempMgr->guy.scrim.resNum;
				guy.scrim.cel = pTempMgr->guy.scrim.cel;
				guy.scrim.rect.Copy(&pTempMgr->guy.scrim.rect);

				break;
			}

			// if we allocated memory, free it
			if (gMgr)
			{
				AFree(gMgr);
			}

			// since graphmgr will be setting the update flag on all scrimages,
			// we don't need to here.

			break;

		case AFTER_RESTORE:
			break;
	}
	return error;
}

