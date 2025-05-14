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
#include "mouseint.hpp"
#include "resource.hpp"
#include	"savemgr.hpp"
#include "mono.hpp"
#include "writeres.hpp"

#define	MAX_MOUSE_IMAGE_BYTES	2048
#define	MAX_HFENCE_BYTES			320
#define	MAX_VFENCE_BYTES			400


//
// MDB - Define this to use old mouse interrupt driver.
//

//#define	OLD_MOUSE_INTERRUPT_HANDLER		1

//
// MDB - END
//


//----[ Mouse Interrupt Routine ]------------------------------------



//
// MDB - Start #ifdef
//

#ifdef OLD_MOUSE_INTERRUPT_HANDLER

static void	(_loadds __interrupt far* functionPtr) (int, int, int, int);

// The interrupt routine needs to be far, with respect to the calling
// function (main).

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

int16		MIntMoveX, MIntMoveY;

MouseInt	*pMouseInt;

static void _loadds __interrupt far MouseInterrupt (int mask, int curposy, int state, int curposx)
{
	if (pMouse && pGraphMgr)
	{
		pMouseInt = (MouseInt *) pMouse;

		#pragma aux MouseInterrupt parm [eax edx ebx ecx]
		// if we are in this routine, pMouse better be set!!!

		state = state;		// Gets rid of unreferenced warning
		curposx = curposx;
		curposy = curposy;

		// copy this to a global before we change the stack and lose it.
		MIntMask = mask;

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
		
		// get the mickeys.
		// the mickeys in si and di (when this procedure is called)
		// are cumulative.  What we want is the change in mickeys, so
		// let's call in routine to get those.
		#pragma aux GET_MICKEYS =	\
			"mov	ax,0xb"				\
			"int	0x33"					\
			"mov	MIntMoveX,cx"			\
			"mov	MIntMoveY,dx";
		GET_MICKEYS();

		if (!(pMouseInt->status & MST_NO_RES))
		{
			// the mouse resolution has been set.
			// update the position.

			// In an effort to make physical mouse movement the same 
			// regardless of screen resolution, the following rules are
			// used.
			//
			// If x resolution < 320, only use half the movement 
			// If y resolution < 200, only use half the movement 

			if (pMouseInt->screenLimits.x2 < 321)
			{
				MIntMoveX += pMouseInt->mickeysLeftX;
				
				// save the mickey that gets trunced
				if (MIntMoveX & 1)
				{
					pMouseInt->mickeysLeftX = 1;
				}
				else
				{
					pMouseInt->mickeysLeftX = 0;
				}

 				MIntMoveX >>= 1;
			}

			if (pMouseInt->screenLimits.y2 < 201)
			{
				MIntMoveY += pMouseInt->mickeysLeftY;
				
				// save the mickey that gets trunced
				if (MIntMoveY & 1)
				{
					pMouseInt->mickeysLeftY = 1;
				}
				else
				{
					pMouseInt->mickeysLeftY = 0;
				}

 				MIntMoveY >>= 1;
			}

			if (pMouseInt->pauseCnt)
			{
				// we are paused.  save the new coords...but don't update yet
				pMouseInt->pauseX += MIntMoveX; 
				pMouseInt->pauseY += MIntMoveY; 

				// make sure that the mouse is within the bounds of the limits
				pMouseInt->pauseX = AMax(pMouseInt->mouseLimits.x1, pMouseInt->pauseX);
				pMouseInt->pauseX = AMin(pMouseInt->mouseLimits.x2, pMouseInt->pauseX);
				pMouseInt->pauseY = AMax(pMouseInt->mouseLimits.y1, pMouseInt->pauseY);
				pMouseInt->pauseY = AMin(pMouseInt->mouseLimits.y2, pMouseInt->pauseY);
			}
			else
			{
				// not paused
				pMouseInt->lastX = pMouseInt->GetX();
				pMouseInt->lastY = pMouseInt->GetY();

				// update the coordinates
				pMouseInt->SetX(pMouseInt->lastX + MIntMoveX);
				pMouseInt->SetY(pMouseInt->lastY + MIntMoveY);

				// make sure that the pointer is still within our bounds
				pMouseInt->CheckBounds();
		
				// the position has changed
				if (!pMouseInt->hideCount)
				{
					// if currently fencing
					if(pMouseInt->fDragMode)
					{
						// update coords of the fence
						pMouseInt->rCurrentFence.x2 = AMin(pMouseInt->GetX(), pMouseInt->rClickDrag.x2);
						pMouseInt->rCurrentFence.y2 = AMin(pMouseInt->GetY(), pMouseInt->rClickDrag.y2);
					}

					pMouseInt->RestoreUnderBits();
					pMouseInt->DrawPointer();
				}
			}
		}

		if (AEvtMgr())
		{
			switch (MIntMask & BTNS_UP_DOWN)
			{
				case MSM_BTN_DOWN_R:
					MIntValue = RIGHT_BTN;
					APostEvent(E_MOUSE_DOWN, MIntValue, FALSE);
					break;

				case MSM_BTN_DOWN_L:
					if(pMouseInt->rClickDrag.Contains(pMouseInt->GetX(), pMouseInt->GetY()))
					{
						pMouseInt->EnableDragging(TRUE);
					}
					MIntValue = LEFT_BTN;
					APostEvent(E_MOUSE_DOWN, MIntValue, FALSE);
					break;

				case MSM_BTN_UP_R:
					MIntValue = RIGHT_BTN;
					APostEvent(E_MOUSE_UP, MIntValue, FALSE);
					break;

				case MSM_BTN_UP_L:
					MIntValue = LEFT_BTN;
					APostEvent(E_MOUSE_UP, MIntValue, FALSE);
					if(pMouseInt->fDragMode)
						pMouseInt->EnableDragging(FALSE);
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



#else // New Mouse Interrupt Handler



//
// MDB - Added locking of all interrupt level code.
//

int lock_region(void* address, int size);
int unlock_region(void* address, int size);

//
// MDB - END
//

//
// MDB - Start address for all the mouse routines.
//

void MouseIntStart( void );
void MouseIntEnd( void );

void MouseIntStart( void )
{
}

//
// MDB - END
//

static void	(_loadds far* functionPtr) (int, int, int, int);

// The interrupt routine needs to be far, with respect to the calling
// function (main).

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

static int		mMask;
static int16	mMoveX;
static int16	mMoveY;

//#define		MouseMoveNoEvent	0
//#define		MouseMoveEvent		1
//#define		MouseButtonEvent	2
#define		MouseMoveEvent		0
#define		MouseButtonEvent	1

struct MouseQueue {
	int		event;
	int		data1;
	int		data2;
};

// Size is determined by an average of 30 mouse events a second and allowing
// for five seconds worth.  Total byte count = 150 * 3 * sizeof(int) = 1800 bytes.
#define		MouseQueueCount		150
#define		MouseQueueEnd		149
static MouseQueue	mMouseQueue[MouseQueueCount];

static int	mHead;
static int	mTail;
static int	mEmpty;



static void InitMouseEvent( void )
{

	_disable();
	mHead = 0;
	mTail = 0;
	mEmpty = TRUE;
	_enable();

}

static void AddMouseEvent( int event, int data1, int data2 )
{

	_disable();
	if ( ( mHead != mTail ) || mEmpty ) {
		mMouseQueue[mTail].event = event;
		mMouseQueue[mTail].data1 = data1;
		mMouseQueue[mTail].data2 = data2;
		if ( mTail < MouseQueueEnd ) {
			mTail++;
		}
		else {
			mTail = 0;
		}
		mEmpty = FALSE;
	}
	_enable();

}

static int GetNextMouseEvent( int* event, int* data1, int* data2 )
{

	int		status = FALSE;

	_disable();
	if ( mEmpty == FALSE ) {
		(*event) = mMouseQueue[mHead].event;
		(*data1) = mMouseQueue[mHead].data1;
		(*data2) = mMouseQueue[mHead].data2;
		if ( mHead < MouseQueueEnd ) {
			mHead++;
		}
		else {
			mHead = 0;
		}
		if ( mHead == mTail ) {
			mEmpty = TRUE;
		}
		status = TRUE;
	}
	_enable();

	return( status );

}



MouseInt	*pMouseInt;

static void _loadds far MouseInterrupt (int mask, int curposy, int state, int curposx)
{

	// Don't need these variables.
	(void) curposy;
	(void) curposx;
	(void) state;

	if ( pMouse ) {

		#pragma aux MouseInterrupt parm [eax edx ebx ecx]
		// if we are in this routine, pMouse better be set!!!

		// copy this to a global before we change the stack and lose it.
		mMask = mask;

		// setup our own stack
		#pragma aux SET_NEW_STACK =					\
			"push	ebp" \
			"push	es" \
			"push	ds" \
			"pop	es"						\
			"mov	word ptr oldStackPtr + 4,ss"		\
			"mov	dword ptr oldStackPtr,esp"		\
			"lss	esp, newStackPtr"		\
	      	"mov	ebp,esp"; 
		SET_NEW_STACK();
		
		// get the mickeys.
		// the mickeys in si and di (when this procedure is called)
		// are cumulative.  What we want is the change in mickeys, so
		// let's call in routine to get those.
		#pragma aux GET_MICKEYS =	\
			"mov	ecx, 0" \
			"mov	edx, 0" \
			"mov	ax,0xb"				\
			"int	0x33"					\
			"mov	word ptr mMoveX,cx"			\
			"mov	word ptr mMoveY,dx";
		GET_MICKEYS();

		// Determine if this was a move event and add the event if so.
		if ( mMask & MSM_MOVE ) {
			AddMouseEvent( MouseMoveEvent, mMoveX, mMoveY );
		}

		// Determine if this was a button event and add the event if so.
		if ( mMask & BTNS_UP_DOWN ) {
			AddMouseEvent( MouseButtonEvent, mMask, 0 );
		}

		#pragma aux RESET_STACK =		\
			"lss	esp, oldStackPtr" \
			"pop	es"	\
			"pop	ebp";
		RESET_STACK();
	}
}



void MouseHandler( int draw_mouse )
{

	// For posible later use.
	(void) draw_mouse;


	if (pMouse && pGraphMgr) {

		int		MIntMoveX = 0;
		int		MIntMoveY = 0;
		int		MIntMask = 0;
		int		event;
		int		data1;
		int		data2;
		int		fDrawMouse = FALSE;

		pMouseInt = (MouseInt *) pMouse;

		// Service several events before drawing the mouse.
		for ( int i = 0; i < 5; i++ ) {

			MIntMoveX = 0;
			MIntMoveY = 0;
			MIntMask = 0;

			// Get the next mouse event if one exists otherwise just return.
			if ( GetNextMouseEvent( &event, &data1, &data2 ) ) {
				if ( event == MouseMoveEvent ) {
					MIntMoveX = data1;
					MIntMoveY = data2;
				}
				else {
					MIntMask = data1;
				}
			}
			else {
				//return;
				break;
			}


			if (!(pMouseInt->status & MST_NO_RES))
			{
				// the mouse resolution has been set.
				// update the position.

				// In an effort to make physical mouse movement the same 
				// regardless of screen resolution, the following rules are
				// used.
				//
				// If x resolution < 320, only use half the movement 
				// If y resolution < 200, only use half the movement 

				if (pMouseInt->screenLimits.x2 < 321)
				{
					MIntMoveX += pMouseInt->mickeysLeftX;
					
					// save the mickey that gets trunced
					if (MIntMoveX & 1)
					{
						pMouseInt->mickeysLeftX = 1;
					}
					else
					{
						pMouseInt->mickeysLeftX = 0;
					}

 					MIntMoveX >>= 1;
				}

				if (pMouseInt->screenLimits.y2 < 201)
				{
					MIntMoveY += pMouseInt->mickeysLeftY;
					
					// save the mickey that gets trunced
					if (MIntMoveY & 1)
					{
						pMouseInt->mickeysLeftY = 1;
					}
					else
					{
						pMouseInt->mickeysLeftY = 0;
					}

 					MIntMoveY >>= 1;
				}

				if (pMouseInt->pauseCnt)
				{
					// we are paused.  save the new coords...but don't update yet
					pMouseInt->pauseX += MIntMoveX; 
					pMouseInt->pauseY += MIntMoveY; 

					// make sure that the mouse is within the bounds of the limits
					pMouseInt->pauseX = AMax(pMouseInt->mouseLimits.x1, pMouseInt->pauseX);
					pMouseInt->pauseX = AMin(pMouseInt->mouseLimits.x2, pMouseInt->pauseX);
					pMouseInt->pauseY = AMax(pMouseInt->mouseLimits.y1, pMouseInt->pauseY);
					pMouseInt->pauseY = AMin(pMouseInt->mouseLimits.y2, pMouseInt->pauseY);
				}
				else
				{
					// not paused
					pMouseInt->lastX = pMouseInt->GetX();
					pMouseInt->lastY = pMouseInt->GetY();

					// update the coordinates
					pMouseInt->SetX(pMouseInt->lastX + MIntMoveX);
					pMouseInt->SetY(pMouseInt->lastY + MIntMoveY);

					// make sure that the pointer is still within our bounds
					pMouseInt->CheckBounds();
			
					// the position has changed
					if (!pMouseInt->hideCount)
					{
						// if currently fencing
						if(pMouseInt->fDragMode)
						{
							// update coords of the fence
							pMouseInt->rCurrentFence.x2 = AMin(pMouseInt->GetX(), pMouseInt->rClickDrag.x2);
							pMouseInt->rCurrentFence.y2 = AMin(pMouseInt->GetY(), pMouseInt->rClickDrag.y2);
						}

						// Enable drawing of the mouse.
						fDrawMouse = TRUE;

					}
				}
			}

			if (AEvtMgr())
			{
				switch (MIntMask & BTNS_UP_DOWN)
				{
					case MSM_BTN_DOWN_R:
						MIntValue = RIGHT_BTN;
						APostEvent(E_MOUSE_DOWN, MIntValue, FALSE);
						break;

					case MSM_BTN_DOWN_L:
						if(pMouseInt->rClickDrag.Contains(pMouseInt->GetX(), pMouseInt->GetY()))
						{
							pMouseInt->EnableDragging(TRUE);
						}
						MIntValue = LEFT_BTN;
						APostEvent(E_MOUSE_DOWN, MIntValue, FALSE);
						break;

					case MSM_BTN_UP_R:
						MIntValue = RIGHT_BTN;
						APostEvent(E_MOUSE_UP, MIntValue, FALSE);
						break;

					case MSM_BTN_UP_L:
						MIntValue = LEFT_BTN;
						APostEvent(E_MOUSE_UP, MIntValue, FALSE);
						if(pMouseInt->fDragMode)
							pMouseInt->EnableDragging(FALSE);
						break;

					default:
						// this should never happen
						//ASSERT(FALSE);
						break;
				}
			}

		}

		// Now draw the mouse in the new position if changed.
		if ( fDrawMouse ) {
			pMouseInt->RestoreUnderBits();
			pMouseInt->DrawPointer();
		}

	}
}

//
// MDB - End address for all the mouse routines.
//

void MouseIntEnd( void )
{
}

//
// MDB - END
//

#endif // MDB - New Mouse Interrupt Handler

//
// MDB - END #ifdef
//



//----[ Mouse Class ]------------------------------------------------



MouseInt::MouseInt()
{
	pauseCnt = 0;

	union
	{
		REGS	inRegs;
		REGS	outRegs;
	};
	SREGS	sRegs;

	// set these to invalid
	resType = RES_LAST;
	resNum = cel = 0;

	// allocate the memory for the mouse image and the save area
	TRACK_MEM("MouseImage");	gMouseImage = ASysMalloc(MAX_MOUSE_IMAGE_BYTES);
	TRACK_MEM("MouseSBits");	gSaveBits = ASysMalloc(MAX_MOUSE_IMAGE_BYTES);
	TRACK_MEM("MouseSH1Bits");	gSaveH1Bits = ASysMalloc(MAX_HFENCE_BYTES);
	TRACK_MEM("MouseSH2Bits");	gSaveH2Bits = ASysMalloc(MAX_HFENCE_BYTES);
	TRACK_MEM("MouseSV1Bits");	gSaveV1Bits = ASysMalloc(MAX_VFENCE_BYTES);
	TRACK_MEM("MouseSV2Bits");	gSaveV2Bits = ASysMalloc(MAX_VFENCE_BYTES);
	TRACK_MEM("MouseDBits");	gDrawBits = ASysMalloc(MAX_MOUSE_IMAGE_BYTES);
	TRACK_MEM("MouseHFenceImage");	gHFenceImage = ASysMalloc(MAX_HFENCE_BYTES);
	TRACK_MEM("MouseVFenceImage");	gVFenceImage = ASysMalloc(MAX_HFENCE_BYTES);

	// since our mallocs will never move, let's get the pointers right now
	pMouseImage = ADerefAs(uchar, gMouseImage);
	pSaveBits = ADerefAs(uchar, gSaveBits);
	pSaveH1Bits = ADerefAs(uchar, gSaveH1Bits);
	pSaveH2Bits = ADerefAs(uchar, gSaveH2Bits);
	pSaveV1Bits = ADerefAs(uchar, gSaveV1Bits);
	pSaveV2Bits = ADerefAs(uchar, gSaveV2Bits);
	pDrawBits = ADerefAs(uchar, gDrawBits);
	pHFenceImage = ADerefAs(uchar, gHFenceImage);
	pVFenceImage = ADerefAs(uchar, gVFenceImage);

	saveBitsSaved = FALSE;

	//
	// MDB - Lock the memory down.
	//

#ifndef OLD_MOUSE_INTERRUPT_HANDLER

	#define	MOUSE_STACK_SIZE	1024
	extern	uchar		mouseStack[MOUSE_STACK_SIZE];

	extern	StackPtr	oldStackPtr;
	extern	StackPtr	newStackPtr;

	// Lock all the mouse interrupt routines.
	lock_region( ( void * ) MouseIntStart,
		(char * ) MouseIntEnd - (char *) MouseIntStart );

	// Lock the global variables.
	lock_region( &mouseStack[0], MOUSE_STACK_SIZE );
	lock_region( &pMouse, sizeof( pMouse ) );
	lock_region( &oldStackPtr, sizeof( oldStackPtr ) );
	lock_region( &newStackPtr, sizeof( newStackPtr ) );
	lock_region( &mMask, sizeof( mMask ) );
	lock_region( &mMoveX, sizeof( mMoveX ) );
	lock_region( &mMoveY, sizeof( mMoveY ) );
	lock_region( &mMouseQueue[0], sizeof( mMouseQueue[0] ) * MouseQueueCount );
	lock_region( &mHead, sizeof( mHead ) );
	lock_region( &mTail, sizeof( mTail ) );
	lock_region( &mEmpty, sizeof( mEmpty ) );

	// Initialize the mouse event queue.
	InitMouseEvent();

#endif

	//
	// MDB - END
	//

	// Setup the interrupt routine
	memset(&sRegs, NULL, sizeof(SREGS));
	memset(&inRegs, NULL, sizeof(REGS));

	inRegs.x.eax = 0x000C;
	inRegs.x.ecx = BTNS_UP_DOWN | MSM_MOVE;
	functionPtr = MouseInterrupt;
	inRegs.x.edx = FP_OFF(functionPtr);
	sRegs.es = FP_SEG(functionPtr);
	int386x(INT_MOUSE, &inRegs, &outRegs, &sRegs);

	// mouse_init will show the driver pointer.
	// Since we are going to display the pointer ourselves,
	// let's hide the mouse driver's pointer

	#pragma aux MOUSE_INIT =	\
		"mov	ax, 2"				\
		"int	33h";
}


MouseInt::~MouseInt()
{
	if (pGraphMgr && !hideCount)
	{
		// mouse is being shown, so hide it
		Hide();
	}

	//
	// MDB - UnLock the memory.
	//

#ifndef OLD_MOUSE_INTERRUPT_HANDLER

	#define	MOUSE_STACK_SIZE	1024
	extern	uchar		mouseStack[MOUSE_STACK_SIZE];

	extern	StackPtr	oldStackPtr;
	extern	StackPtr	newStackPtr;


	// Lock all the mouse interrupt routines.
	unlock_region( ( void * ) MouseIntStart,
		(char * ) MouseIntEnd - (char *) MouseIntStart );

	// Lock the global variables.
	unlock_region( &mouseStack[0], MOUSE_STACK_SIZE );
	unlock_region( &pMouse, sizeof( pMouse ) );
	unlock_region( &oldStackPtr, sizeof( oldStackPtr ) );
	unlock_region( &newStackPtr, sizeof( newStackPtr ) );
	unlock_region( &mMask, sizeof( mMask ) );
	unlock_region( &mMoveX, sizeof( mMoveX ) );
	unlock_region( &mMoveY, sizeof( mMoveY ) );
	unlock_region( &mMouseQueue[0], sizeof( mMouseQueue[0] ) * MouseQueueCount );
	unlock_region( &mHead, sizeof( mHead ) );
	unlock_region( &mTail, sizeof( mTail ) );
	unlock_region( &mEmpty, sizeof( mEmpty ) );

#endif

	//
	// MDB - END
	//

	saveBitsSaved = FALSE;
	AFree(gVFenceImage);
	AFree(gHFenceImage);
	AFree(gMouseImage);
	AFree(gSaveBits);
	AFree(gSaveH1Bits);
	AFree(gSaveH2Bits);
	AFree(gSaveV1Bits);
	AFree(gSaveV2Bits);
	AFree(gDrawBits);
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
// Return values:  TRUE  - pointer protected
//                 FALSE - pointer did not need protection
bool
MouseInt::Protect(bool protect, Rectangle *pRect)
{
	bool	fProtected = FALSE;

	if (protect)
	{
		if(!pRect)
			return(FALSE);

		if(AMin(lastRect.x2, pRect->x2) >= AMax(pRect->x1, lastRect.x1) &&
			AMin(lastRect.y2, pRect->y2) >= AMax(pRect->y1, lastRect.y1))
		{
			// we need to protect the mouse
			Hide();
			fProtected = TRUE;
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
				fProtected = TRUE;
			}
		}
		return(fProtected);
	}
	else
	{
		Show();	
	}

	return FALSE;
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
			sprintf(mess, "Bogus Cel Num (Anim %d) cel num %d, of %d cels", resNum, iCel+1, numCels);
			APanic(mess);
		}

		pResData = AGetResData(gRes);
		
		celHeader = &((CelHeader*) pResData)[iCel];

		// take origin point into account
//		_disable();	// BUGBUG - removed by Van cuz of serial ISR conflict
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
			sprintf(mess, "Mouse Anim(%d, %d) too large", resNum, cel);
			APanic(mess);
		}

		VGABuffer	vbuf;

		vbuf.offscreenBMap = (bitmap_t) gMouseImage;
		vbuf.width = width;
		vbuf.bufWidth = width;

//		_disable();	// BUGBUG - proposed new location of _disable, by Van
		orgX = tempOrgX;
		orgY = tempOrgY;

		rect.x1 = 0;
		rect.y1 = 0;
		rect.x2 = tempX2;
		rect.y2 = tempY2;
		rect.owner = gSelf;

		vbuf.Load(&rect, 0, 0, gRes, cel, NULL, 0, TRUE, scale);
		// clear this so that the VGABuffer destructor doesn't free it
		vbuf.offscreenBMap = 0;
		_enable();

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
		sprintf(mess, "Bogus Cel Num (Anim %d) cel num %d, of %d cels",
			resNum, celNum, numCels);
		APanic(mess);
	}

	pResData = AGetResData(gRes);
		
	celHeader = &((CelHeader*) pResData)[celNum];

	// take origin point into account
	//		_disable();	// BUGBUG - removed by Van cuz of serial ISR conflict
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
		sprintf(mess, "Mouse Anim(%d, %d) too large", resNum, celNum);
		APanic(mess);
	}

	vbuf.offscreenBMap = (bitmap_t) gHFenceImage;
	vbuf.width = width;
	vbuf.bufWidth = width;

//		_disable();	// BUGBUG - proposed new location of _disable, by Van
	orgX = tempOrgX;
	orgY = tempOrgY;

	rFence.Set(0, 0, celHeader->width - 1, celHeader->height - 1);
	rFence.owner = gSelf;

	vbuf.Load(&rFence, 0, 0, gRes, celNum + 1, NULL, 0, TRUE, 256);
	// clear this so that the VGABuffer destructor doesn't free it
	vbuf.offscreenBMap = 0;

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
		sprintf(mess, "Bogus Cel Num (Anim %d) cel num %d, of %d cels",
			resNum, celNum, numCels);
		APanic(mess);
	}

	pResData = AGetResData(gRes);
		
	celHeader = &((CelHeader*) pResData)[celNum];

	// take origin point into account
	//		_disable();	// BUGBUG - removed by Van cuz of serial ISR conflict
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
		sprintf(mess, "Mouse Anim(%d, %d) too large", resNum, celNum);
		APanic(mess);
	}

	vbuf.offscreenBMap = (bitmap_t) gVFenceImage;
	vbuf.width = width;
	vbuf.bufWidth = width;

	orgX = tempOrgX;
	orgY = tempOrgY;

	rFence.Set(0, 0, celHeader->width - 1, celHeader->height - 1);
	rFence.owner = gSelf;

	vbuf.Load(&rFence, 0, 0, gRes, celNum + 1, NULL, 0, TRUE, 256);
	// clear this so that the VGABuffer destructor doesn't free it
	vbuf.offscreenBMap = 0;
	_enable();
}

void
MouseInt::SetScale(uint newScale)
{
	if (scale != newScale)
	{
		SetRes(resType, resNum, cel, newScale);
	}
}


// just a stub
void
MouseInt::SetPri(int32 pri)
{
	// take care of unreferenced warning
	pri = pri;
}


void
MouseInt::UpdatePointer(void)
{
	// this is just a stub for tigre.	
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
		_disable();

		RestoreUnderBits();
		DrawPointer();

		_enable();
	}
}

// save the bits at x,y
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

	lastRect.x1 = x - orgX;
	lastRect.y1 = y - orgY;
	lastRect.x2 = lastRect.x1 + rect.Wide() - 1;
	lastRect.y2 = lastRect.y1 + rect.High() - 1;

	ClipLastRect();
//	lastRect.Clip(&screenLimits);

	ARBlit(pGraphMgr->vgaDriver, pSaveBits, lastRect.x1, lastRect.y1,
		lastRect.Wide(), lastRect.High(), pGraphMgr->videoBufferSeg);

	saveBitsSaved = TRUE;
}

// restore the bits at the lastX, lastY
void
MouseInt::RestoreUnderBits(void)
{
	if (saveBitsSaved)
	{
		// restore the bits back to the screen
		ABlit(pGraphMgr->vgaDriver, pSaveBits,
				lastRect.x1, lastRect.y1, lastRect.Wide(), lastRect.High(),
				lastRect.Wide(), pGraphMgr->videoBufferSeg);

		if(fDragMode)
		{
			HideFence();
		}

		saveBitsSaved = FALSE;
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
			EnableDragging(FALSE);
		}

		// enable drag mode
		fDragMode = TRUE;
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
			fDragMode = FALSE;
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

// draw the pointer at it's x,y
void
MouseInt::DrawPointer(void)
{
	uint32	celIndex;
	uint32	bufIndex;			
	int		vbufWidth;
	int		celWidth;
	int		rWidth;
	int		rHigh;
	int		row;
	int		tmpFenceX,tmpFenceY;
	ASMCopyPixels		acp;

	SaveUnderBits();

	rWidth = lastRect.Wide();
	rHigh = lastRect.High();

	// copy the back ground bits
	memcpy(pDrawBits, pSaveBits, rWidth * rHigh);

	celIndex = clipRow * rect.Wide() + clipCol;
	bufIndex = 0;
	vbufWidth = rWidth;
	celWidth = rect.Wide();

	acp._clut = NULL;
	acp._clutIdx = 0;
	acp._clutSize = 0;
	acp._pSrcData = pMouseImage;
	acp._pDstData = pDrawBits;
	acp._srcWidth = rWidth;
	acp._hdir = CP_FORWARD;
	acp._doSkip = TRUE;
	acp._srle = FALSE;

	// now copy the mouse image into the draw bits, checking for skip 
	for (row = 0; row < rHigh; row++)
	{
		acp._srcOffset = celIndex;
		acp._dstOffset = bufIndex;
		OS_CopyPixelLine(&acp);
		bufIndex += vbufWidth;
		celIndex += celWidth;
	}

	// now blit the results back into screen mem
	ABlit(pGraphMgr->vgaDriver, pDrawBits,
			lastRect.x1, lastRect.y1, rWidth, rHigh, rWidth,
			pGraphMgr->videoBufferSeg);

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

		// draw the drag fences
		ABlit(pGraphMgr->vgaDriver, pHFenceImage, fenceX1, fenceY1,
			tmpFenceX, 1, tmpFenceX, pGraphMgr->videoBufferSeg);
		ABlit(pGraphMgr->vgaDriver, pHFenceImage, fenceX1, fenceY2,
			tmpFenceX, 1, tmpFenceX, pGraphMgr->videoBufferSeg);
		ABlit(pGraphMgr->vgaDriver, pVFenceImage, fenceX1, fenceY1,
			1, tmpFenceY, 1, pGraphMgr->videoBufferSeg);
		ABlit(pGraphMgr->vgaDriver, pVFenceImage, fenceX2, fenceY1,
			1, tmpFenceY, 1, pGraphMgr->videoBufferSeg);

//		pMono->Out("(%d,%d %d,%d)\n", fenceX1, fenceY1, fenceX2, fenceY2);
	}
}

// stop the mouse from moving on the screen, but don't turn off
// interrupts.  If the mouse is paused, save what the coordinates
// should be.  Restore the real coordinates when the mouse is unpaused.

void
MouseInt::Pause(bool pauseIt)
{
	_disable();

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

	_enable();
}

//----[ Saver Routine ]----------------------------------------------

bool
MouseInt::Save(uint16 state, FILE *fp)
{
	bool		error = FALSE;
	grip		gMgr = 0;
	MouseInt	*pTempMgr;
	uint16	i;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
			// write out the mouse class
			if (fwrite(pMouse, 1, sizeof(MouseInt), fp) != sizeof(MouseInt))
			{
				// some sort of error
				error = TRUE;
				break;
			}

			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			Hide();
			break;

		case DURING_RESTORE:
			// read old mouse into temp
			TRACK_MEM("TmpMouseMgr");	gMgr = AMalloc(sizeof(MouseInt));
			pTempMgr = ADerefAs(MouseInt, gMgr);

			// run in a while loop so that we can break on an error
			// and clean up properly
			while (TRUE)
			{
				// read the manager
				if (fread(pTempMgr, 1, sizeof(MouseInt), fp) != sizeof(MouseInt))
				{
					// some sort of error
					error = TRUE;
					break;
				}

				screenLimits.Copy(&pTempMgr->screenLimits);
				SetLimits(&pTempMgr->mouseLimits);

				SetX(pTempMgr->x);
				SetY(pTempMgr->y);

				// clear the motion (mickeys) counters
				ClearMickeys();

				// make sure that the pointer is still within our bounds
				CheckBounds();

				// set the anim
				_disable();

				resType = pTempMgr->resType;
				resNum = pTempMgr->resNum;
				cel = pTempMgr->cel;
				rect.Copy(&pTempMgr->rect);

				saveBitsSaved = FALSE;
				orgX = pTempMgr->orgX;
				orgY = pTempMgr->orgY;
				scale = pTempMgr->scale;

 				pauseCnt = pTempMgr->pauseCnt;
				pauseX = pTempMgr->pauseX;
				pauseY = pTempMgr->pauseY;

				_enable();

				if (hideCount > pTempMgr->hideCount)
				{
					// show this some more
					for (i = pTempMgr->hideCount; i < hideCount; i++)
					{
						Show();
					}
				}
				else
				{
					if (hideCount < pTempMgr->hideCount)
					{
						// hide this some more
						for (i = hideCount; i < pTempMgr->hideCount; i++)
						{
							Hide();
						}
					}
				}

				// update the pointer 
				MovePointer();

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



