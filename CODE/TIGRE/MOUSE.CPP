//
// MOUSE.CPP
//
//	Interrupt based pointer movement version.
// (instead of Scrimage based)
//
// November 24, 1993
// WATCOM: October 25, 1994  BKH
// Interrupt: November 18, 1994  JDH
// (c) Copyright 1993,1994 Tachyon, Inc.  All Rights Reserved.
//
// This is an abstract class. If you want to use the mouse,
// you should read how to in either mouseint.cpp (interrupt based)
// or mousescr.cpp (scrimage based).  If you want to be able to
// switch back and forth between to two types, follow the instructions
// in mousescr.cpp.  
//
//----[]-------------------------------------------------------------


#include <i86.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "graphmgr.hpp"
#include "mono.hpp"
#include "mouse.hpp"
#include "resource.hpp"
#include	"savemgr.hpp"
#include "writeres.hpp"

#define	MAX_MOUSE_IMAGE_BYTES	2048

bool		MouseSave(uint16 state, FILE *fp);

Mouse*	pMouse = NULL;


//----[ Mouse Interrupt Stuff ]------------------------------------

// The interrupt routine needs to be far, with respect to the calling
// function (main).

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

// we need our own stack
#define	MOUSE_STACK_SIZE	4096
uchar		mouseStack[MOUSE_STACK_SIZE];

StackPtr	oldStackPtr;
StackPtr	newStackPtr;

int32		MIntValue;
int 		MIntMask;


//----[ Mouse Class ]------------------------------------------------

// at exit function
void
ShutDownMouse() 
{
	if (pMouse)
	{
		pMouse->ShutDownMouseDriver();
	}
}

// at exit function
void
Mouse::ShutDownMouseDriver() 
{
	union
	{
		REGS	inRegs;
		REGS  outRegs;
	};

	// Reset the mouse driver, which clears the interrupt routine
	inRegs.x.eax = 0;

	// don't allow any mouse interrupts to happen while we are shutting down
	_disable();
	int386(INT_MOUSE, &inRegs, &outRegs);
	_enable();

	if (outRegs.x.eax == -1)
	{
		APanic ("Mouse shutdown unsuccessful");
	}
}

Mouse::Mouse()
{
	mickeysLeftX = mickeysLeftY = 0;

	// setup exceptional ptr to Mouse
	if (!pMouse)
	{
		pMouse = this;

		// make a far pointer for our interrupt stack
		newStackPtr.selector = FP_SEG(&mouseStack[MOUSE_STACK_SIZE - 4]);
		newStackPtr.pointer = FP_OFF(&mouseStack[MOUSE_STACK_SIZE - 4]);

		// setup for save
		AtSave(MouseSave);
	}

	// protected mode mouse init, installs mouse interrupt function
	union
	{
		REGS	inRegs;
		REGS	outRegs;
	};

	// check to see if the mouse driver exists
	// and reset the driver
	inRegs.x.eax = 0;
	int386(INT_MOUSE, &inRegs, &outRegs);
	if (outRegs.w.ax == 0)
	{
		printf("No mouse driver installed");
		hideCount = 999;
	}
	else
	{
		// initialize the mouse hide Count to hidden
		hideCount = 1;
		
		// init mouse status
		status = MST_NO_RES;

		// install shutdown function
		atexit(ShutDownMouse);

		// don't save during save or purge on a restore
		ASetSaveStatus(pMouse->gSelf, FALSE);
		ASetPurgeStatus(pMouse->gSelf, FALSE);
	}
}

Mouse::~Mouse()
{
	ShutDownMouseDriver();

	pMouse = NULL;
}

// init the mouse.  pass the screen rectangle coordinates.
void
Mouse::Init(coord x1, coord y1, coord x2, coord y2)
{
	// This procedure should be called right after the
	// screen mode (resolution) is set.  You can call this multiple times.

	// screenRect is the current screen resolution

	coord	newX, newY;

	_disable();
	screenLimits.Set(x1,y1,x2,y2);

	rClickDrag.Set(-1, -1, -1, -1);	// default == DISABLED
	fDragMode = FALSE;

	// put the mouse in the middle of the screen
	newX = abs(screenLimits.x2 - screenLimits.x1)/2;
	newY = abs(screenLimits.y2 - screenLimits.y1)/2;

	SetX(newX);
	SetY(newY);
	lastX = newX;
	lastY = newY;

	SetLimits(&screenLimits);

	_enable();

	// make sure that the no resolution bit is clear
	status &= ~MST_NO_RES;

	if (!hideCount)
	{
		// if the pointer is current being shown, let's update it 
		ShowPointer();
	}
}

void
Mouse::SetDraggableRect(int x1, int y1, int x2, int y2)
{
	rClickDrag.Set(x1, y1, x2, y2);
}

void
Mouse::SetLimits(Rectangle *mouseRect)
{
	// mouseRect is the box within the screen that the mouse
	//    should stay restricted to.  This is usually the same as screenRect

	// set the new mouse limits
	_disable();
	mouseLimits.Copy(mouseRect);
	_enable();

	// make sure that the pointer is still within our bounds
	SetPos(GetX(),GetY());
}

// Check to see if the x,y are in bounds.
// If x or y change, TRUE is returned. 
bool
Mouse::CheckBounds()
{
	coord	newX,newY;
	coord oldX,oldY;

	newX = oldX = GetX();
	newY = oldY = GetY();

	// make sure that the mouse is within the bounds of the limits
	newX = AMax(mouseLimits.x1, newX);
	newX = AMin(mouseLimits.x2, newX);
	newY = AMax(mouseLimits.y1, newY);
	newY = AMin(mouseLimits.y2, newY);

	if (oldX != newX)
	{
		SetX(newX);
	}

	if (oldY != newY)
	{
		SetY(newY);
	}

	// If x or y changed, TRUE is returned. 
	return (bool) !(oldX == newX && oldY == newY);
}

void
Mouse::GetPos(Message* pMsg)
{
	pMsg->event.x = GetX();
	pMsg->event.y = GetY();
}

void
Mouse::SetPos(coord setX, coord setY)
{
	// clear the motion (mickeys) counters
	ClearMickeys();

	_disable();

	lastX = GetX();
	lastY = GetY();

	SetX(setX);
	SetY(setY);

	_enable();

	// make sure that the pointer is still within our bounds
	if (CheckBounds())
	{
		// the position has changed
		MovePointer();
	}
}

void
Mouse::Show()
{
	// if hideCount > 0, then the mouse is hidden
	if (hideCount) 
	{
		hideCount--;

		if (!hideCount)
		{
			// time to show the pointer
			if (status & MST_NO_RES) 
			{
				// if no resolution is set, end program
				APanic("No mouse resolution set");
			}

			ShowPointer();
		}
	}
}

void
Mouse::Hide()
{
	_disable();
	if (!hideCount)
	{
		// hiding for the first time
		HidePointer();
	}

	// don't let hideCount wrap
	if (hideCount < 0xffff)
	{
		// hidden 1 more deep
		hideCount++;
	}
	_enable();
}


// Force the hide count to a new value.
// The previous hide count is returned (for possible restore).

int
Mouse::ForceShow(int16 newCount)
{
	int	prevHideCount = hideCount;

	if (newCount < 0)
	{
		APanic("Trying to Force a negative value");	
	}

	// force hide count to specified value
	hideCount = newCount;

	if (!prevHideCount && hideCount)
	{
		// we were shown, but now we should be hidden
		_disable();
		HidePointer();
		_enable();
	}
	else
	{
		if (prevHideCount && !hideCount)
		{
			// we were hidden, but now we should be shown
			ShowPointer();
		}
	}

	// return previous count so that it can be restored
	return prevHideCount;
}


//----[ Saver Routine ]----------------------------------------------

bool
MouseSave(uint16 state, FILE *fp)
{
	return	pMouse->Save(state, fp);
}

// Clear the motion (mickeys) counters by reading them again
void
Mouse::ClearMickeys()
{
	union
	{
		REGS	inRegs;
		REGS	outRegs;
	};

	inRegs.x.eax = 0x000b;
	int386(INT_MOUSE, &inRegs, &outRegs);

	pMouse->mickeysLeftX = 0;
	pMouse->mickeysLeftY = 0;
}

int
Mouse::GetFenceStartX(void)
{
	return(rCurrentFence.x1);
}

int
Mouse::GetFenceStartY(void)
{
	return(rCurrentFence.y1);
}

void
Mouse::SetFence(int X1, int Y1, int X2, int Y2)
{
	// note: this function unused in DOS
	#ifdef OS_MAC
	// insert Mac code here
	#endif
}

void
Mouse::DrawFence(void)
{
	#ifdef OS_MAC
	// insert Mac code here
	#endif
}

void
Mouse::EraseFence(void)
{
	#ifdef OS_MAC
	// insert Mac code here
	#endif
}
