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
#include "json.hpp"



#define	EVENT_Q_MAX			32



//---------[EventMgr]-------------------------------------------

#define	MESSAGE_Q_SIZE		64

//	EventMgr class declaration
//
class EventMgr : public Manager, public Object
{
	public:
		using Manager::operator new;
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

		EventMgr(uint nsize = MESSAGE_Q_SIZE, uint esize = MESSAGE_Q_SIZE);
		virtual		~EventMgr();

		void			PublishNext();
		void			WaitForEvent(Message* pMsg = nullptr);
						
		void			Where(coord* x, coord* y);
		uint16		GetModifiers();
		char			ScanToASCII(Event e);

		bool			PostEvent(evt_t type, int32 value);
		Message*		PostEventXY(evt_t type, int32 value, coord x, coord y);

		bool			PostNotice(notice_t type, grip gDest = grip{}, void* param = nullptr, grip gSource = grip{});
		bool			SendNotice(notice_t type, grip gDest = grip{}, grip gSource = grip{}, void* param = nullptr);

		uint			FlushEvents(evt_t mask = E_KEY_DOWN | E_KEY_UP | E_MOUSE_DOWN | E_MOUSE_UP);
		uint			FlushNotices(grip gDest);

		bool			Save(uint16 state, nlohmann::json& root);
		bool			KeyIsDown(uchar scanCode);
		void			PostScanKey(uchar scanCode, bool down);

		void			AutoUpdateTicks(bool updateIt);

		TClock		clock;

	protected:

		void			AdvanceEQueue();
		void			AdvanceNQueue();
		Message*		ETail();
		Message*		NTail();
		void			InitEvent(Message* pMsg, evt_t type = E_NULL, int32 value = 0);
		void			InitNotice(Message* pMsg, notice_t type = N_NULL);

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
#define	ASetGameSpeed(spd)	(pEventMgr->clock.SetSpeed(spd))


#endif

