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
#include "jstream.hpp"

#include "mono.hpp"
#include "debug.hpp"

#ifndef NDEBUG
#define NDEBUG
#endif

extern Debugger ResMgrDebug;


EventMgr*	pEventMgr = nullptr;

bool	EventMgrSave(uint16 state, nlohmann::json& root);


//								 012345678901234567890123456789012345678901234567890123456789
// Scancode to ASCII tables
const char*	asciiTable      = "_ 1234567890-=__qwertyuiop[]__asdfghjkl;'`_\\zxcvbnm,./ ";
const char*	asciiShiftTable = "_ !@#$%^&*()_+__QWERTYUIOP{}__ASDFGHJKL:\"~_|ZXCVBNM<>? ";


// if the corresponding scan code is still down, return true
bool
EventMgr::KeyIsDown(uchar scanCode)
{
	// clear press/release bit
	scanCode &= 0x7f;

	return  scanKeys[scanCode] ? true : false;	
}



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
	AutoUpdateTicks(true);

	// init queue indeces & sizes
	iEHead = iETail = 0;
	iNHead = iNTail = 0;
	eqSize = esize;
	nqSize = nsize;
	pQEvents = ADerefAs(Message, ASysMalloc(eqSize * sizeof(Message)));
	pQNotices = ADerefAs(Message, ASysMalloc(nqSize * sizeof(Message)));
	feedEvents = false;
	inInterrupt = false;

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

	// init our scan keys
	memset(scanKeys, 0, sizeof(scanKeys));
}



// Destructor
EventMgr::~EventMgr()
{
	AFree(AGetGrip(pQEvents));
	AFree(AGetGrip(pQNotices));

	pEventMgr = nullptr;
}


// should the eventmgr auto update ticks?
void
EventMgr::AutoUpdateTicks(bool updateIt)
{
	autoUpdateTicks = updateIt;
}


// Publish notices and events to pContextMgr.
// If feedEvents is true, only the top event will be posted, otherwise
// all events will be posted.  In either case, all notices will be
// posted.
//
void
EventMgr::PublishNext()
{
	Object*		pObj = nullptr;
	Message		msg;
	Message*		pMsg;
	Periodic*	pPer;
	bool			fSendToAll = false;

	// Update the "clock" which we will use for the rest of this cycle.
	// only update if we are suppossed to.
	if (autoUpdateTicks)
	{
		UpdateTicks();
	}

	// Process events, but only if there is at least one event
	if (iETail != iEHead)
	{
		do
		{
			pMsg = nullptr;

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
			pMsg = nullptr;

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


// Returns SDL modifier state (shift, ctrl, alt) as TIGRE modifier bitmask.
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
		return true;
	}
	return false;
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
	if (pMsg == nullptr)
	{
		return 0;
	}

	InitEvent(pMsg, type, value);

	// Ignore certain key combinations
	if (type == E_KEY_DOWN)
	{
		m = pMsg->event.modifiers;
		if (value == K_DEL && m & MOD_CTRL && m & MOD_ALT)
		{
			return 0;
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
	if (pMsg == nullptr)
	{
		return false;
	}

	InitNotice(pMsg, type);
	pMsg->notice.gDest	= gDest;
	pMsg->notice.gSource	= gSource;
	pMsg->notice.param	= param;

	return true;
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
	return ret;
}


// clear queue of notices directed to deleted grip
uint
EventMgr::FlushNotices(grip gDest)
{
	// can happen during shutdown
	if (this == nullptr)
	{
		return 0;
	}

	unsigned	i;
	Message*	pMsg;
	uint		ret = 0;

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
	return ret;
}


//----[ Protected Methods ]------------------------------------------

void
EventMgr::InitEvent(Message* pMsg, evt_t type, int32 value)
{
	memset(pMsg, 0, sizeof(Message));

	pMsg->type = MSG_EVENT;
	pMsg->event.type = type;
	pMsg->event.value = value;
	pMsg->event.modifiers = pEventMgr->GetModifiers();

	// using ANSI clock() causes a crash if called during an 
	pMsg->event.ticks = ATicks();

	// give mouse loc
	//pMsg->event.x = 0;
	//pMsg->event.y = 0;
}


void
EventMgr::InitNotice(Message* pMsg, notice_t type)
{
	memset (pMsg, 0, sizeof(Message));

	pMsg->type = MSG_NOTICE;
	pMsg->notice.type = type;
}


void
EventMgr::AdvanceEQueue()
{
	// check for wrap
	if (++iEHead >= eqSize)
	{
		iEHead = 0; 
	}
}


void
EventMgr::AdvanceNQueue()
{
	// check for wrap
	if (++iNHead >= nqSize)
	{
		iNHead = 0; 
	}
}


// Get the current tail of the event queue which can be filled with a newly
//	posted event, advance to the new tail.
//
Message*
EventMgr::ETail()
{
	int		i;

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
//
// MDB - End.
//

		// Event is queue full
		return 0;
	}
	return &pQEvents[i];
}


// Get the current tail of the notice queue which can be filled with a newly
//	posted notice, advance to the new tail.
//
Message*
EventMgr::NTail()
{
	int	i;

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
//
// MDB - End.
//

		// Event is queue full
		return 0;
	}
	return &pQNotices[i];
}



/* Post a key event and update the scanKeys[] state table.
 * Called from ASDLPumpEvents() to feed SDL keyboard events into the game. */
void
EventMgr::PostScanKey(uchar scanCode, bool down)
{
	scanCode &= 0x7F;
	if (!scanCode) return;

	if (down)
	{
		if (scanKeys[scanCode]) return;   /* suppress auto-repeat */
		scanKeys[scanCode] = 1;
		PostEvent(E_KEY_DOWN, (int32) scanCode);
	}
	else
	{
		scanKeys[scanCode] = 0;
		PostEvent(E_KEY_UP, (int32) scanCode);
	}
}


//----[ Saver Routine ]----------------------------------------------
bool
EventMgrSave(uint16 state, nlohmann::json& root)
{
	return pEventMgr->Save(state, root);
}

bool
EventMgr::Save(uint16 state, nlohmann::json& root)
{
	switch (state)
	{
		case BEFORE_SAVE:
			PauseTicks();
			break;

		case DURING_SAVE:
		case DURING_RESTORE:
		{
			JsonStream js{root["eventmgr"], (state == DURING_SAVE)};
			ticks_t ticks = ATicks();
			js.sync(ticks, "ticks");
			if (!js.saving)
			{
				iEHead = iETail = 0;
				iNHead = iNTail = 0;
				clock.SetTicks(ticks);
			}
			break;
		}

		case AFTER_SAVE:
			ResumeTicks();
			FlushEvents();
			break;

		case BEFORE_RESTORE:
			PauseTicks();
			break;

		case AFTER_RESTORE:
			ResumeTicks();
			break;
	}
	return false;
}


//

