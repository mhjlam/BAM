//
// APIEVT.HPP
//
// November 12, 1993
// WATCOM: October 4, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Definitions for API calls specific to the event manager.
//
//----[]-------------------------------------------------------------


#ifndef	apievt_hpp
#define	apievt_hpp


#include "eventmgr.hpp"
#include "tigre.hpp"
#include "types.hpp"


bool		AEvtMgr();

bool		APostEvent(evt_t type, int32 value, bool turnInterruptsOff = true);
bool		APostNotice(notice_t type, grip gDest = grip{}, void* param = nullptr, grip gSource = grip{});
bool		ASendNotice(notice_t type, grip gDest = grip{}, grip gSource = grip{}, void* param = nullptr);
uint		AFlushEvents(evt_t mask = E_KEY_DOWN | E_KEY_UP | E_MOUSE_DOWN | E_MOUSE_UP);
uint		AFlushNotices(grip gDest);
void		APublishNext();
char		AScanToASCII(Event e);
uint16	AGetEventMods();
void		AAutoUpdateTicks(bool updateIt);


#endif
