//
// EVENTMGR.HPP
//
// November 23, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// EventMgr class definition.
//
//		EventMgr accepts subscribers to the event stream.
//		Events may be hardware caused (mouse, keyboard) or 
//		defined and posted by the programmer.
//
//----[]-------------------------------------------------------------


#ifndef	eventmgr_hpp
#define	eventmgr_hpp


#include "tigre.hpp"
#include "clock.hpp"
#include "manager.hpp"
#include "keybrd.hpp"



// convert ANSI clock_t to ticks_t
#define	CLOCKS_TO_TICKS(c)	((ticks_t) (c / (CLOCKS_PER_SEC/TICKS_PER_SEC)))

#define	EVENT_Q_MAX			32



//---------[EventMgr]-------------------------------------------

#define	MESSAGE_Q_SIZE		64

//	EventMgr class declaration
//
class EventMgr : public Manager, public Object
{
	public:
		bool			feedEvents;					// Should events be handled 1/cycle?
		Message*		pQEvents;
		uint			iEHead;
		uint			iETail;
		uint			eqSize;
		Message*		pQNotices;
		uint			iNHead;
		uint			iNTail;
		uint			nqSize;
		bool			inInterrupt;
		uchar*		real_ptr_bios_key_status;

		EventMgr(uint nsize = MESSAGE_Q_SIZE, uint esize = MESSAGE_Q_SIZE);
		virtual		~EventMgr();

		void			PublishNext();
		void			WaitForEvent(Message* pMsg = NULL);
						
		void			Where(coord* x, coord* y);
		uint16		GetModifiers();
		char			ScanToASCII(Event e);

		bool			PostEvent(evt_t type, int32 value);
		Message*		PostEventXY(evt_t type, int32 value, coord x, coord y);

		bool			PostNotice(notice_t type, grip gDest = 0, void* param = NULL, grip gSource = 0);
		bool			SendNotice(notice_t type, grip gDest = 0, grip gSource = 0, void* param = NULL);

		uint			FlushEvents(evt_t mask = E_KEY_DOWN | E_KEY_UP | E_MOUSE_DOWN | E_MOUSE_UP);
		uint			FlushNotices(grip gDest);

		bool			Save(uint16 state, FILE *fp = NULL);
		bool			KeyIsDown(uchar scanCode);

		void			AutoUpdateTicks(bool updateIt);

		static void __interrupt __far KeyInterrupt();
		static void (__interrupt __far *prev_int_09)();

		TClock		clock;

	protected:

		void			AdvanceEQueue();
		void			AdvanceNQueue();
		Message*		ETail();
		Message*		NTail();
		void			InitEvent(Message* pMsg, evt_t type = E_NULL, int32 value = 0);
		void			InitNotice(Message* pMsg, notice_t type = N_NULL);
		void			EnableInterrupts();

		uchar			scanKeys[128];
		bool			autoUpdateTicks;
};

extern EventMgr* pEventMgr;


// The old tick functions now map to EventMgr's TClock instance

#define	ATicks()			(pEventMgr->clock.GetTicks())
#define	UpdateTicks()	(pEventMgr->clock.Cycle())
#define	PauseTicks()	(pEventMgr->clock.Pause())
#define	ResumeTicks()	(pEventMgr->clock.Resume())
#define	ForceResumeTicks()	(pEventMgr->clock.ForceResume())
#define	ASetTicks(t)		(pEventMgr->clock.SetTicks(t))


#endif

