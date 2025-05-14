//
// EVENTMGR.CPP
//
// November 23, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include <time.h>

#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "eventmgr.hpp"
#include "mouse.hpp"
#include "periodic.hpp"
#include	"savemgr.hpp"

#include "mono.hpp"
#include "debug.hpp"

#define NDEBUG

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)
extern Debugger ResMgrDebug;

//
// MDB - Added locking of all interrupt level code.
//

/* lock a given region of memory */
int lock_region(void* address, int size)
{
union REGS regs;

regs.w.ax=0x600;
regs.w.cx=(unsigned long) address&0xffff;
regs.w.bx=(unsigned long) address>>16;
regs.w.di=size&0xffff;
regs.w.si=(unsigned long) size>>16;
int386(0x31, &regs, &regs);
return regs.x.cflag&1;
}


/* unlock a region of memory */
int unlock_region(void* address, int size)
{
union REGS regs;
regs.w.ax=0x601;
regs.w.cx=(unsigned long) address&0xffff;
regs.w.bx=(unsigned long) address>>16;
regs.w.di=size&0xffff;
regs.w.si=(unsigned long) size>>16;
int386(0x31, &regs, &regs);
return regs.x.cflag&1;
}

//
// MDB - END
//

//
// MDB - Start address for all the eventmgr routines.
//

void EventMgrStart( void );
void EventMgrEnd( void );

void EventMgrStart( void )
{
}

//
// MDB - END
//

//
// MDB - Create a stack for the keyboard interrupt event calls.
//

// we need our own stack
#define	KEYBOARD_STACK_SIZE		2048
uchar		keyboardStack[KEYBOARD_STACK_SIZE];

StackPtr	oldKeyboardStackPtr;
StackPtr	newKeyboardStackPtr;

uchar		K_scanCode;
evt_t		K_type;
bool		K_keyAlreadyPressed = FALSE;
//
// MDB - END
//

// These allow us to trick BIOS into thinking there's nothing for it to do
uint16* const 	pKeyBufHead = (uint16*) 0x41a;
uint16* const 	pKeyBufTail = (uint16*) 0x41c;

EventMgr*	pEventMgr = NULL;

bool	EventMgrSave(uint16 state, FILE *fp);
//static bool		KeyForBios(unsigned char scanCode);


//								 012345678901234567890123456789012345678901234567890123456789
// Scancode to ASCII tables
char*	asciiTable      = "_ 1234567890-=__qwertyuiop[]__asdfghjkl;'`_\\zxcvbnm,./ ";
char*	asciiShiftTable = "_ !@#$%^&*()_+__QWERTYUIOP{}__ASDFGHJKL:\"~_|ZXCVBNM<>? ";


// if the corresponding scan code is still down, return TRUE
bool
EventMgr::KeyIsDown(uchar scanCode)
{
	// clear press/release bit
	scanCode &= 0x7f;

	return  scanKeys[scanCode] ? TRUE : FALSE;	
}


// Check if this is a key we want to give to BIOS
//static bool
//KeyForBios(unsigned char scanCode) 
//{
//	// clear press/release bit
//	scanCode &= 0x7f;
//
//	switch (scanCode)
//	{
//		// let bios maintain modifiers keys
//		case K_BREAK:
//		case K_RIGHT_SHIFT:
//		case K_LEFT_SHIFT:
//		case K_ALT:
//		case K_CTRL:
//			return TRUE;
//
//		// everything else is ours
//		default:
//			return FALSE;
//	}
//}


//----[ EventMgr ]---------------------------------------------------

// Constructor
EventMgr::EventMgr(uint nsize, uint esize)
{
	int		i;

	// setup exceptional ptr to EventMgr
	if (!pEventMgr)
	{
		pEventMgr = this;
		// setup for save
		AtSave(EventMgrSave);
	}
	SetName("EventMgr");

	// init "clock"
	UpdateTicks();
	AutoUpdateTicks(TRUE);

	real_ptr_bios_key_status = AGetBiosKey();

	// init queue indeces & sizes
	iEHead = iETail = 0;
	iNHead = iNTail = 0;
	eqSize = esize;
	nqSize = nsize;
	TRACK_MEM("Event Buf");		pQEvents = ADerefAs(Message, ASysMalloc(eqSize * sizeof(Message)));
	TRACK_MEM("Notice Buf");	pQNotices = ADerefAs(Message, ASysMalloc(nqSize * sizeof(Message)));
	feedEvents = FALSE;
	inInterrupt = FALSE;

	// Initialize event queue
	for (i = 0; i < eqSize; i++)
	{
		InitEvent(&pQEvents[i]);
	}

	// Initialize notice queue
	for (i = 0; i < nqSize; i++)
	{
		InitEvent(&pQNotices[i]);
	}

	// init our scan keys (This MUST be done before keyboard vector is set)
	memset(scanKeys, 0, sizeof(scanKeys));

	ASetKeyboardVector();

	//
	// MDB - Create the stack.
	//

	// make a far pointer for our interrupt stack
	newKeyboardStackPtr.selector = FP_SEG(&keyboardStack[KEYBOARD_STACK_SIZE - 4]);
	newKeyboardStackPtr.pointer = FP_OFF(&keyboardStack[KEYBOARD_STACK_SIZE - 4]);

	//
	// MDB - END
	//

	//
	// MDB - Lock the memory down.
	//

	// Lock all the mouse interrupt routines.
	lock_region( ( void * ) EventMgrStart,
		(char * ) EventMgrEnd - (char *) EventMgrStart );

	// Lock the class and global variables.
	lock_region( this, sizeof( EventMgr ) );
	lock_region( &pEventMgr, sizeof( pEventMgr ) );
	lock_region( (void *) &pKeyBufHead, sizeof( pKeyBufHead ) );
	lock_region( (void *) &pKeyBufTail, sizeof( pKeyBufTail ) );
	lock_region( (void *) &asciiTable, sizeof( asciiTable ) );
	lock_region( (void *) &asciiShiftTable, sizeof( asciiShiftTable ) );
	lock_region( &keyboardStack[0], KEYBOARD_STACK_SIZE );
	lock_region( &oldKeyboardStackPtr, sizeof( oldKeyboardStackPtr ) );
	lock_region( &newKeyboardStackPtr, sizeof( newKeyboardStackPtr ) );
	lock_region( &K_scanCode, sizeof( K_scanCode ) );
	lock_region( &K_type, sizeof( K_type ) );

	//
	// MDB - END
	//

}



// Destructor
EventMgr::~EventMgr()
{
	ASetKeyboardVector(FALSE);
	AFree(AGetGrip(pQEvents));
	AFree(AGetGrip(pQNotices));

	//
	// MDB - Lock the memory down.
	//

	// Lock all the mouse interrupt routines.
	unlock_region( ( void * ) EventMgrStart,
		(char * ) EventMgrEnd - (char *) EventMgrStart );

	// Lock the class and global variables.
	unlock_region( this, sizeof( EventMgr ) );
	unlock_region( &pEventMgr, sizeof( pEventMgr ) );
	unlock_region( (void *) &pKeyBufHead, sizeof( pKeyBufHead ) );
	unlock_region( (void *) &pKeyBufTail, sizeof( pKeyBufTail ) );
	unlock_region( (void *) &asciiTable, sizeof( asciiTable ) );
	unlock_region( (void *) &asciiShiftTable, sizeof( asciiShiftTable ) );
	unlock_region( &keyboardStack[0], KEYBOARD_STACK_SIZE );
	unlock_region( &oldKeyboardStackPtr, sizeof( oldKeyboardStackPtr ) );
	unlock_region( &newKeyboardStackPtr, sizeof( newKeyboardStackPtr ) );
	unlock_region( &K_scanCode, sizeof( K_scanCode ) );
	unlock_region( &K_type, sizeof( K_type ) );

	//
	// MDB - END
	//

	pEventMgr = NULL;
}


// should the eventmgr auto update ticks?
void
EventMgr::AutoUpdateTicks(bool updateIt)
{
	autoUpdateTicks = updateIt;
}


// Publish notices and events to pContextMgr.
// If feedEvents is TRUE, only the top event will be posted, otherwise
// all events will be posted.  In either case, all notices will be
// posted.
//
void
EventMgr::PublishNext()
{
	Object*		pObj = NULL;
	Message		msg;
	Message*		pMsg;
	Periodic*	pPer;
	bool			fSendToAll = FALSE;

	// Update the "clock" which we will use for the rest of this cycle.
	// only update if we are suppossed to.
	if (autoUpdateTicks)
	{
		UpdateTicks();
	}

	AQueueUpdate();

	// Process events, but only if there is at least one event
	if (iETail != iEHead)
	{
		do
		{
			pMsg = NULL;

			// Get top event, adjust iEHead and iETail
			pMsg = &pQEvents[iEHead];
			msg = *pMsg;

			// Reset popped event
			InitEvent(pMsg);

//			if (msg.event.type == E_KEY_DOWN)
//			{
//				pMono->Out("Key = %x CTRL %d ALT %d SHIFT %d %d\n",
//						msg.event.value, scanKeys[K_CTRL], scanKeys[K_ALT], scanKeys[K_LEFT_SHIFT], scanKeys[K_RIGHT_SHIFT]);
//			}

			AdvanceEQueue();
			if (msg.event.type != E_NULL && msg.type != MSG_ABORTED)
			{
				#ifdef	OS_DOS
				// check for control alt delete
				if (msg.event.type == E_KEY_DOWN) {
					if (msg.event.value == K_DEL) {
						if (msg.event.modifiers & MOD_CTRL) {
							if (msg.event.modifiers & MOD_ALT) {
						
								if (pContextMgr) 
								{
									pContextMgr->Quit();
								} 
								else 
								{
									exit(0);
								}
							}
						}
					}
				}
				#endif

				// Send this notice to all event suscribers
				if (msg.event.type == E_NULL) 
				{
					InitEvent(&msg);
				}
				else if (pContextMgr->InMask(&msg))
				{
					// Pass message on to all active contexts if the manager wants them
					pContextMgr->HandleMsg(&msg);
				}
			}

		}	while (msg.type == MSG_ABORTED || msg.type == MSG_NULL || (feedEvents && iEHead != iETail));
	}


	// Process notices, but only if there is at least one notice
	if (iNTail != iNHead)
	{
		do
		{
			pMsg = NULL;

			// Get top notice, adjust iNHead and iNTail
			pMsg = &pQNotices[iNHead];
			msg = *pMsg;

			// Reset popped notice
			InitNotice(pMsg);

			AdvanceNQueue();
			if (msg.type != MSG_NULL && msg.type != MSG_ABORTED)
			{
				if (msg.notice.type == N_NULL)
				{
					InitNotice(&msg);
				}
				else if (msg.notice.gDest)
				{
					// Pass notice to the periodic specified
					pPer = ADerefAs(Periodic, msg.notice.gDest);
					pPer->HandleMsg(&msg);
				}
			}

		}	while (msg.type == MSG_ABORTED || msg.type == MSG_NULL || iNHead != iNTail);
	}
}


// Lock into a loop until a message an event is available
//
void
EventMgr::WaitForEvent(Message* pMsg)
{
	Message	msg;

	msg.type = MSG_NULL;
	while (msg.type == MSG_NULL)
	{
		if (iEHead != iETail)
		{
			// Get top event, adjust iEHead and iETail
			memcpy(&msg, &pQEvents[iEHead], sizeof(Message));

			// Reset popped event
			InitEvent(&pQEvents[iEHead]);

			AdvanceEQueue();
			if (msg.event.type == E_NULL || msg.type == MSG_ABORTED)
			{
				msg.type = MSG_NULL;
			}
		}
		pMouse->UpdatePointer();
		AAnimate();
	}
	if (pMsg)
	{
		memcpy(pMsg, &msg, sizeof(Message));
	}
}


void
EventMgr::Where(coord* pX, coord* pY)
{
	Message	msg;

	if (pMouse)
	{
		pMouse->GetPos(&msg);
	}

	// Mouse support not yet implemented
	*pX = msg.event.x;
	*pY = msg.event.y;
}


//	Instead of calling bios ... bioskey(_KEYBRD_SHIFTSTATUS) ...
// look at bios variable
uint16
EventMgr::GetModifiers()
{
	return AGetEventMods();
}


// Return the ASCII character that corresponds with the scancode passed.
char
EventMgr::ScanToASCII(Event e)
{
	char ret;

	if (e.modifiers & MOD_SHIFT)
	{
		ret = *(asciiShiftTable + e.value);
	}
	else
	{
		ret = *(asciiTable + e.value);
	}
	return ret;
}

//	Attempt to post event 
bool
EventMgr::PostEvent(evt_t type, int32 value)
{
	Message*	pMsg;

	pMsg = PostEventXY(type, value, 0, 0);
	if (pMsg)
	{
		// Get mouse position
		if (pMouse)
		{
			pMouse->GetPos(pMsg);
		}
		return TRUE;
	}
	return FALSE;
}


//	Attempt to post event, but use the x and y locations passed in
// to set the event's x and y instead of Polling the mouse with
// GetPos(). This is used on the mac side to prevent inaccurate 
// mouse up and down events. 
Message*
EventMgr::PostEventXY(evt_t type, int32 value, coord x, coord y)
{
	Message*	pMsg;
	uint16	m;

	pMsg = ETail();

	// if Queue is full, don't post
	if (pMsg == NULL)
	{
		return NULL;
	}

	InitEvent(pMsg, type, value);

	// Ignore certain key combinations
	if (type == E_KEY_DOWN)
	{
		m = pMsg->event.modifiers;
		if (value == K_DEL && m & MOD_CTRL && m & MOD_ALT)
		{
			return NULL;
		}
	}
	
	// use explicit location
	pMsg->event.x = x;
	pMsg->event.y = y;

	return pMsg;
}


bool
EventMgr::PostNotice(notice_t type, grip gDest, void* param, grip gSource)
{
	Message*	pMsg;

	pMsg = NTail();

	// if Queue is full, don't post
	if (pMsg == NULL)
	{
		return FALSE;
	}

	InitNotice(pMsg, type);
	pMsg->notice.gDest	= gDest;
	pMsg->notice.gSource	= gSource;
	pMsg->notice.param	= param;

	return TRUE;
}


bool
EventMgr::SendNotice(notice_t type, grip gDest, grip gSource, void* param)
{
	Message		msg;
	Periodic*	pPer;

	// Send requires a specific destination object, otherwise is should be posted
	ASSERT (gDest);

	InitNotice(&msg, type);
	msg.notice.gDest		= gDest;
	msg.notice.gSource	= gSource;
	msg.notice.param		= param;

	pPer = ADerefAs(Periodic, gDest);

	return (pPer->HandleMsg(&msg));
}


// clear queue of notices directed to deleted grip
uint
EventMgr::FlushEvents(evt_t mask)
{
	unsigned	i;
	Message*	pMsg;
	uint		ret = 0;

	_disable();
	pMsg = pQEvents;

	for (i = 0; i < eqSize; i++, pMsg++)
	{
		if (pMsg->event.type & mask)
		{
			ret++;

			// set unwanted message	to a type that will be ignored. 
			pMsg->type = MSG_ABORTED;
			pMsg->event.value = 0;
		}
	}
	EnableInterrupts();
	return ret;
}


// clear queue of notices directed to deleted grip
uint
EventMgr::FlushNotices(grip gDest)
{
	// can happen during shutdown
	if (this == NULL)
	{
		return 0;
	}

	unsigned	i;
	Message*	pMsg;
	uint		ret = 0;

	_disable();
	pMsg = pQNotices;

	for (i = 0; i < nqSize; i++, pMsg++)
	{
		if (pMsg->notice.gDest == gDest)
		{
			ret++;

			// set unwanted message	to a type that will be ignored. 
			pMsg->type = MSG_ABORTED;
			pMsg->event.value = 0;
		}
	}
	EnableInterrupts();
	return ret;
}


//----[ Protected Methods ]------------------------------------------

void
EventMgr::InitEvent(Message* pMsg, evt_t type, int32 value)
{
	_disable();
	memset(pMsg, 0, sizeof(Message));

	pMsg->type = MSG_EVENT;
	pMsg->event.type = type;
	pMsg->event.value = value;
	pMsg->event.modifiers = pEventMgr->GetModifiers();

	// using ANSI clock() causes a crash if called during an interrupt
	pMsg->event.ticks = ATicks();

	// give mouse loc
	//pMsg->event.x = 0;
	//pMsg->event.y = 0;
	EnableInterrupts();
}


void
EventMgr::InitNotice(Message* pMsg, notice_t type)
{
	_disable();
	memset (pMsg, 0, sizeof(Message));

	pMsg->type = MSG_NOTICE;
	pMsg->notice.type = type;
	EnableInterrupts();
}


void
EventMgr::AdvanceEQueue()
{
	_disable();
	// check for wrap
	if (++iEHead >= eqSize)
	{
		iEHead = 0; 
	}
	EnableInterrupts();
}


void
EventMgr::AdvanceNQueue()
{
	_disable();
	// check for wrap
	if (++iNHead >= nqSize)
	{
		iNHead = 0; 
	}
	EnableInterrupts();
}


// Get the current tail of the event queue which can be filled with a newly
//	posted event, advance to the new tail.
//
Message*
EventMgr::ETail()
{
	int		i;

	_disable();
	// save for return
	i = iETail;

	// check for wrap
	if (++iETail >= eqSize)
	{
		iETail = 0; 
	}

	// full queue error check
	if (iETail == iEHead)
	{ 
		// restore tail
		iETail = i;

//
// MDB - Added code to enable interrupts since diabled above.
//
		EnableInterrupts();
//
// MDB - End.
//

		// Event is queue full
		return NULL;
	}
	EnableInterrupts();
	return &pQEvents[i];
}


// Get the current tail of the notice queue which can be filled with a newly
//	posted notice, advance to the new tail.
//
Message*
EventMgr::NTail()
{
	int	i;

	_disable();
	// save for return
	i = iNTail;

	// check for wrap
	if (++iNTail >= nqSize)
	{
		iNTail = 0; 
	}

	// full queue error check
	if (iNTail == iNHead)
	{ 
		// restore tail
		iNTail = i;

//
// MDB - Added code to enable interrupts since diabled above.
//
		EnableInterrupts();
//
// MDB - End.
//

		// Event is queue full
		return NULL;
	}
	EnableInterrupts();
	return &pQNotices[i];
}


void
EventMgr::EnableInterrupts()
{
	if (!inInterrupt)
	{
		_enable();
	}
}


#ifdef OS_DOS

#include <conio.h>

int	ki_debugging = FALSE;
uchar	ki_lastChar[2] = {0,0};

// NEW KEYBOARD ROUTINE
//
//static void __interrupt __far
//EventMgr::KeyInterrupt()
//{
//	uchar				scanCode;
//	evt_t				type;
//	bool				keyAlreadyPressed;
//	bool				callPrevInt;
//	uchar				port61Val;
//	uchar				oldPort61Val;
//	uchar				port64Val;
//	bool				keyToProcess = TRUE;
//
//	pEventMgr->inInterrupt = TRUE;
//
//	while(keyToProcess)
//	{
//		keyAlreadyPressed = FALSE;
//		callPrevInt = TRUE;
//
//		// read keyboard data port
//		scanCode = (uchar) inp(0x60);
//
//		if (ki_lastChar[0] == 0xe0)
//		{
//			// extended key code
//			if ((scanCode & 0x7f) == 0x2a)
//			{
//				// this is half of print screen.
//				// (E0 2A E0 37 - down, E0 B7 E0 AA - up)
//				// ignore this char. let the other char trigger it
//				scanCode = 0;
//			}
//
//			ki_lastChar[0] = scanCode;
//		}
//		else
//		{
//			if (ki_lastChar[0] == 0xe1)
//			{
//				// moving towards a pause sequence.
//				// (E1 1D 45 - down, E1 9D C5 - up)
//				if ((ki_lastChar[1] & 0x7f) == 0x1d)
//				{
//					// we have the second key.
//					// this must be the third
//
//					// we fake out the rest of our code into thinking this is one
// 					// key
//					if (scanCode & 0x80)
//					{
//						// Pause Up
//						scanCode = (K_PAUSE | 0x80);
//					}
//					else
//					{
//						// Pause Down
//						scanCode = K_PAUSE;
//					}
//
//					ki_lastChar[0] = scanCode;
//					ki_lastChar[1] = 0;
//				}
//				else
//				{
//					// save this second key
//					ki_lastChar[1] = scanCode;
//					// don't process this
//					scanCode = 0;
//				}
//			}
//			else
//			{
//				ki_lastChar[0] = scanCode;
//			}
//		}
//
//
//		if (scanCode == 0xe0 ||
//		 	scanCode == 0xe1)
//		{
//			// this is an extended code
//			// ignore it.
//			scanCode = 0;
//		}
//
//		// check if key is pressed or released & adjust scanCode
//		if (scanCode & 0x80)
//		{
//			type = E_KEY_UP;
//			scanCode &= 0x7f;
//
//			// this key is no longer being pressed
//			pEventMgr->scanKeys[scanCode] = 0;
//		}
//		else
//		{
//			type = E_KEY_DOWN;
//
//			// if this key is already pressed, or is invalid, don't Post
//			if (pEventMgr->scanKeys[scanCode] || !scanCode)
//			{
//				// we allow backspace to repeat
//				if (scanCode != K_BACK_SPACE)
//				{
//					keyAlreadyPressed = TRUE;
//				}
//			}
//			else
//			{
//				// key is now being pressed
//				pEventMgr->scanKeys[scanCode] = 1;
//			}
//		}
//
//		if	(scanCode == K_RIGHT_SHIFT ||
//		 	scanCode == K_LEFT_SHIFT ||
//		 	scanCode == K_ALT ||
//		 	scanCode == K_CTRL)
//		{
//			// this is a modifier.
//			// ignore it.
//			keyAlreadyPressed = TRUE;
//		}
//
//		if (ki_debugging)
//		{
//			// We are debugging and want CTRL-BRK to put us in the 
//			// debugger.
//			if (callPrevInt)
//			{
//				prev_int_09();
//			}
//
//			// if we actually have a key, do some things.
//			// (modifiers, etc call this interrupt, but don't add keys)
//			if (*pKeyBufTail != *pKeyBufHead)
//			{
//				// don't allow key repeat
//				if (!keyAlreadyPressed)
//				{
//					pEventMgr->PostEvent(type, (int32) scanCode);
//				}
//
//				// trick BIOS into thinking there's nothing for it to do
//				*pKeyBufTail = *pKeyBufHead;
//			}
//		}
//		else
//		{
//			// don't allow key repeat
//			if (!keyAlreadyPressed)
//			{
//				pEventMgr->PostEvent(type, (int32) scanCode);
//			}
//		}
//
//		// check for another key
//		port64Val = (uchar) inp(0x64);
//		if ((port64Val & 0x02) == 0)
//		{
//			keyToProcess = FALSE;
//		}
//	}
//
//	pEventMgr->inInterrupt = FALSE;
//
//	// reset keyboard flag
//
//	oldPort61Val = (uchar) inp(0x61);
//	port61Val = (uchar) (oldPort61Val | 0x80);
//	outp(0x61, port61Val);
//	outp(0x61, oldPort61Val);
//
//	// signal end of interrupt to interrupt controller
//	outp(0x20, 0x20);
//}
//


// OLD KEYBOARD ROUTINE

/*
//
// MDB - Removed old keyboard interrupt routine.
//
static void __interrupt __far
EventMgr::KeyInterrupt()
{
	uchar				scanCode;
	evt_t				type;
	bool				keyAlreadyPressed = FALSE;
	bool				callPrevInt = TRUE;

	pEventMgr->inInterrupt = TRUE;

	// read keyboard data port
	scanCode = (uchar) inp(0x60);

	if (scanCode == 0xe0)
	{
		// this is an extended code.
		// since we don't care about the difference
		// between the grey keys and the normal keys,
		// get the next scancode
		scanCode = (uchar) inp(0x60);
	}

	// check if key is pressed or released & adjust scanCode
	if (scanCode & 0x80)
	{
		type = E_KEY_UP;
		scanCode &= 0x7f;

		// this key is no longer being pressed
		pEventMgr->scanKeys[scanCode] = 0;
	}
	else
	{
		type = E_KEY_DOWN;

		if (pEventMgr->scanKeys[scanCode])
		{
			keyAlreadyPressed = TRUE;
		}
		else
		{
			// key is now being pressed
			pEventMgr->scanKeys[scanCode] = 1;
		}
	}

	// check to see if we should pass this on to the previous keyboard int.
	if (pEventMgr->GetModifiers() & MOD_CTRL)
	{
		if (scanCode == K_C)
		{
			// nope.  ctrl C doesn't get through
			callPrevInt = FALSE;
		}
	}

	if (callPrevInt)
	{
		prev_int_09();
	}

	// if we actually have a key, do some things.
	// (modifiers, etc call this interrupt, but don't add keys)
	if (*pKeyBufTail != *pKeyBufHead)
	{
		// don't allow key repeat
		if (!keyAlreadyPressed)
		{
			pEventMgr->PostEvent(type, (int32) scanCode);
		}

		// trick BIOS into thinking there's nothing for it to do
		*pKeyBufTail = *pKeyBufHead;
	}

	pEventMgr->inInterrupt = FALSE;
}
//
// MDB - End.
//
*/


//
// MDB - Fixed keyboard interrupt routine.
//

static void __interrupt __far
EventMgr::KeyInterrupt()
{
	bool				callPrevInt = TRUE;

	pEventMgr->inInterrupt = TRUE;

	K_keyAlreadyPressed = FALSE;

	// read keyboard data port
	K_scanCode = (uchar) inp(0x60);

	// Check if pause key e1 or normal extended scan code and ignore them.
#ifdef NDEBUG
	if ( ( K_scanCode != 0xe0 ) && ( K_scanCode != 0xe1 ) ) {
#else
	// This allows Crtl-Break to occur.
	// MUST BE SWITCHED BACK FOR RELEASE VERSION !!!
	if ( K_scanCode != 0xe1 ) {
#endif

		// check if key is pressed or released & adjust K_scanCode
		if (K_scanCode & 0x80)
		{
			K_type = E_KEY_UP;
			K_scanCode &= 0x7f;

			// this key is no longer being pressed
			pEventMgr->scanKeys[K_scanCode] = 0;
		}
		else
		{
			K_type = E_KEY_DOWN;

			if (pEventMgr->scanKeys[K_scanCode])
			{
				K_keyAlreadyPressed = TRUE;
			}
			else
			{
				// key is now being pressed
				pEventMgr->scanKeys[K_scanCode] = 1;
			}
		}

		// check to see if we should pass this on to the previous keyboard int.
		if (pEventMgr->GetModifiers() & MOD_CTRL)
		{
			if (K_scanCode == K_C)
			{
				// nope.  ctrl C doesn't get through
				// This also does Crtl-Break since loses E0 scan code
				// and becomes just normal Crtl-C.
				callPrevInt = FALSE;
			}
		}

		if (callPrevInt)
		{
			prev_int_09();
		}

		// setup our own stack
		#pragma aux SET_NEW_KEYBOARD_STACK =					\
			"push	ebp" \
			"push	es" \
			"push	ds" \
			"pop	es"						\
			"mov	word ptr oldKeyboardStackPtr + 4,ss"		\
			"mov	dword ptr oldKeyboardStackPtr,esp"		\
			"lss	esp, newKeyboardStackPtr";
		SET_NEW_KEYBOARD_STACK();

		// if we actually have a key, do some things.
		// (modifiers, etc call this interrupt, but don't add keys)
		if (*pKeyBufTail != *pKeyBufHead)
		{
			// don't allow key repeat
			if (!K_keyAlreadyPressed)
			{
				pEventMgr->PostEvent(K_type, (int32) K_scanCode);
			}

			// trick BIOS into thinking there's nothing for it to do
			*pKeyBufTail = *pKeyBufHead;
		}

		#pragma aux RESET_KEYBOARD_STACK =		\
			"lss	esp, oldKeyboardStackPtr" \
			"pop	es"	\
			"pop	ebp";
		RESET_KEYBOARD_STACK();

	}

	pEventMgr->inInterrupt = FALSE;
	outp( 0x20, 0x20 );

}

//
// MDB - End.
//

static void (__interrupt __far *EventMgr::prev_int_09) ();

#endif



//----[ Saver Routine ]----------------------------------------------
bool
EventMgrSave(uint16 state, FILE *fp)
{
	return pEventMgr->Save(state, fp);
}

bool
EventMgr::Save(uint16 state, FILE *fp)
{
	bool		error = FALSE;
	ticks_t	oldTicks;
	grip		gMgr = 0;
	EventMgr	*pTempMgr;

	switch (state)
	{
		case BEFORE_SAVE:
			PauseTicks();
			break;

		case DURING_SAVE:
			_disable();				// stop messages

			// write out event mgr
			if (fwrite(pEventMgr, 1, sizeof(EventMgr), fp) != sizeof(EventMgr))
			{
				// some sort of error
				error = TRUE;
				EnableInterrupts();			// messages can start again
				break;
			}

			// write out events
			if (fwrite(pQEvents, 1, (eqSize * sizeof(Message)), fp) != (eqSize * sizeof(Message)))
			{
				// some sort of error
				error = TRUE;
				EnableInterrupts();			// messages can start again
				break;
			}

			// write out notices
			if (fwrite(pQNotices, 1, (nqSize * sizeof(Message)), fp) != (nqSize * sizeof(Message)))
			{
				// some sort of error
				error = TRUE;
				EnableInterrupts();			// messages can start again
				break;
			}

			EnableInterrupts();			// messages can start again

			// save ticks
			oldTicks = ATicks();
			if (fwrite(&oldTicks, 1, sizeof(ticks_t), fp) != sizeof(ticks_t))
			{
				// some sort of error
				error = TRUE;
				break;
			}
			break;

		case AFTER_SAVE:
			ResumeTicks();
			FlushEvents();
			break;

		case BEFORE_RESTORE:
			PauseTicks();
			break;

		case DURING_RESTORE:
			// read old manager into temp
			TRACK_MEM("TmpEvtMgr");	gMgr = AMalloc(sizeof(EventMgr));
			pTempMgr = ADerefAs(EventMgr, gMgr);

			// run in a while loop so that we can break on an error
			// and clean up properly
			while (TRUE)
			{
				_disable();				// stop messages

				// read in event mgr
				if (fread(pTempMgr, 1, sizeof(EventMgr), fp) != sizeof(EventMgr))
				{
					// some sort of error
					error = TRUE;
					EnableInterrupts();			// messages can start again
					break;
				}

				// make sure the buffers are the same size
				if (eqSize != pTempMgr->eqSize)
				{
					pQEvents = ADerefAs(Message, pMemMgr->Realloc(AGetGrip(pQEvents), pTempMgr->eqSize * sizeof(Message)));	
					eqSize = pTempMgr->eqSize;
				}

				if (nqSize != pTempMgr->nqSize)
				{
					pQNotices = ADerefAs(Message, pMemMgr->Realloc(AGetGrip(pQNotices), pTempMgr->nqSize * sizeof(Message)));	
					nqSize = pTempMgr->nqSize;
				}

				// read in events
				if (fread(pQEvents, 1, (eqSize * sizeof(Message)), fp) != (eqSize * sizeof(Message)))
				{
					// some sort of error
					error = TRUE;
					EnableInterrupts();			// messages can start again
					break;
				}

				// read in notices
				if (fread(pQNotices, 1, (nqSize * sizeof(Message)), fp) != (nqSize * sizeof(Message)))
				{
					// some sort of error
					error = TRUE;
					EnableInterrupts();			// messages can start again
					break;
				}

				feedEvents = pTempMgr->feedEvents;
				iEHead = pTempMgr->iEHead;
				iETail = pTempMgr->iETail;
				iNHead = pTempMgr->iNHead;
				iNTail = pTempMgr->iNTail;
				inInterrupt = pTempMgr->inInterrupt;

				EnableInterrupts();			// messages can start again

				// read ticks
				if (fread(&oldTicks, 1, sizeof(ticks_t), fp) != sizeof(ticks_t))
				{
					// some sort of error
					error = TRUE;
					break;
				}
				clock.SetTicks(oldTicks);

				break;
			}

			// if we allocated memory, free it
			if (gMgr)
			{
				AFree(gMgr);
			}

			break;

		case AFTER_RESTORE:
			ResumeTicks();
			break;
	}
	return error;
}


//
// MDB - End address for all the eventmgr routines.
//

void EventMgrEnd( void )
{
}

//
// MDB - END
//

