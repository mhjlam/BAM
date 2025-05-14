//
// APIEVT.CPP
//
// November 12, 1993
// WATCOM: October 4, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Portable event manager specific functions
//
//----[]-------------------------------------------------------------



#include "apievt.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


// Signal whether the manager has been created
bool
AEvtMgr()
{
	return pEventMgr ? TRUE : FALSE;
}


bool
APostEvent(evt_t type, int32 value, bool turnInterruptsOff)
{
	bool	result;

	if (turnInterruptsOff)
	{
		pEventMgr->inInterrupt = FALSE;	
	}
	else
	{
		pEventMgr->inInterrupt = TRUE;	
	}

	result = pEventMgr->PostEvent(type, value);

	// make sure this is reset
	pEventMgr->inInterrupt = FALSE;	

	return result;
}


bool
APostNotice(notice_t type, grip gDest, void* param, grip gSource)
{
	return pEventMgr->PostNotice(type, gDest, param, gSource);
}


bool
ASendNotice(notice_t type, grip gDest, grip gSource, void* param)
{
	return pEventMgr->SendNotice(type, gDest, gSource, param);
}


uint
AFlushEvents(evt_t mask)
{
	return pEventMgr->FlushEvents(mask);
}


uint
AFlushNotices(grip gDest)
{
	return pEventMgr->FlushNotices(gDest);
}


void
APublishNext()
{
	pEventMgr->PublishNext();
}

void		
AAutoUpdateTicks(bool updateIt)
{
	pEventMgr->AutoUpdateTicks(updateIt);
}


