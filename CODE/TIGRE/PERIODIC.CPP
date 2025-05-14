//
// PERIODIC.CPP
//
// December 09, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apievt.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "periodic.hpp"
#include "savebase.hpp"

// Constructor used by all Periodics
Periodic::Periodic()
{
	gScript = 0;
	gContext = 0;
	gClock = 0;

	SetInterval(1);
	SetName("Periodic");

}


// Destructor used by all Periodics
Periodic::~Periodic()
{
	// clear queue of notices directed to deleted grip
	AFlushNotices(gSelf);
}

bool
Periodic::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&periodicDataStart, 1, (int)&periodicDataEnd -
				(int)&periodicDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&periodicDataStart, 1, (int)&periodicDataEnd -
				(int)&periodicDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

//now we have a choice of clocks to run from
void
Periodic::SetClock(grip gNewClock)
{
	gClock = gNewClock;
}


// 
void
Periodic::SetInterval(ticks_t delay)
{
	TClock	*pClock;

	if (!delay)
	{
		delay = 1;
	}

	interval = delay;

	if (AEvtMgr())
	{
		if(gClock)
		{
			pClock = ADerefAs(TClock,gClock);
			timer = pClock->GetTicks() + interval;
		}
		else
		{
			timer = ATicks() + interval;
		}
	}
	else
	{
		timer = 0;
	}
}



//----------------------------------------------------------------

// Test if periodic is ready to be serviced
//
void
Periodic::Cycle()
{
	uint32	ticks;
	TClock	*pClock;

	if (timer)
	{
		if(gClock)
		{
			pClock = ADerefAs(TClock,gClock);
			ticks = pClock->GetTicks();
		}
		else
		{
			ticks = ATicks();
		}

		if (timer <= ticks)
		{
			Service();
			SetInterval(interval);
		}
	}
	else 
	{
		Service();
		SetInterval(interval);
	}
}


// Default service for all Periodics
void
Periodic::Service()
{
}


// assign a script to this periodic and set it in motion
void
Periodic::Set_Script(void* pv, grip gSuccessor)
{
	Script*	pScript;

	if (gContext == 0) {
		APanic("Periodic must have context before taking script");
	}

	// if already has script, abort it
	if (gScript)
	{
		pScript = ADerefAs(Script, gScript);
		pScript->Abort();
	}

	pScript = (Script*) pv;
	gScript = pScript->gSelf;

	pScript->Setup(gContext, this, gSuccessor);
}


void
Periodic::ScriptAborting()
{
	// hook lets periodic know when its script is aborted
	gScript = 0;
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
//		Script
//---------------------------------------------------------------------
//---------------------------------------------------------------------


// Constructor used by all Scripts
Script::Script()
{
	gContext = 0;
	gSuccessor = 0;
	gOwner = 0;
	SetName("Script");
	fIsActive = FALSE;

	// the default notice sent when script completes is n_cue, 
	// but may be overwritten
	completionNoticeType = N_CUE;
}


// Destructor used by all Scripts
Script::~Script()
{
}


// handle cue message
void
Script::Cue()
{
	APanic("Script should redefine this method");	
}


// Default handle event for all Objects, does nothing,
// and advises the EventMgr that its done nothing (returns FALSE)
//
bool
Script::HandleMsg(Message* pMsg)
{
	if ((pMsg->type & MSG_NOTICE) && (pMsg->notice.type == N_CUE))
	{
		// no sub-script, cue self
		Cue();
		return TRUE;
	}
	return FALSE;
}


// Start script in motion
void
Script::Setup(grip gCntxt, Periodic *pOwner, grip gSendToWhenDone)
{
	Context*	pContext;

	// if already active, abort it (clearing cues) before re-starting
	if (fIsActive)
	{
		Abort();
	}

	fIsActive = TRUE;

	SetContext(gCntxt);
	pContext = ADerefAs(Context, gContext);
	pContext->AddObject(gSelf, CT_MSGS|CT_SERV);

	if (pOwner) 
	{
		gOwner = pOwner->gSelf;
	}
	else
	{
		gOwner = 0;
	}

	gSuccessor = gSendToWhenDone;
	
	state = 0;
	scriptTimer = 0;

	APostNotice(N_CUE, gSelf);
}


// end script activity, remove from service and event streams
void
Script::Cleanup()
{
	Abort();

	if (gSuccessor)
	{
		APostNotice(completionNoticeType, gSuccessor);
	}
}


// end script activity, don't cue
void
Script::Abort()
{
	Context*	pContext;

	ASSERT(gContext);

	// tell owner that the script is aborting (Utilizer uses this
	// to cleanup utilities that might otherwise cue owner)
	if (gOwner)
	{
		Periodic* pOwner = ADerefAs(Periodic, gOwner);
		pOwner->ScriptAborting();
	}

	pContext = ADerefAs(Context, gContext);
	pContext->CutObject(gSelf, CT_MSGS|CT_SERV);

	gContext = 0;

	AFlushNotices(gSelf); // so no one will cue a cancelled script

	fIsActive = FALSE;
}


// use Servicer to check script interval timer
void
Script::Service()
{
	ticks_t	ticks;
	TClock	*pClock;

	if (scriptTimer)
	{
		if(gClock)
		{
			pClock = ADerefAs(TClock,gClock);
			ticks = pClock->GetTicks();
		}
		else
		{
			ticks = ATicks();
		}

		if (scriptTimer <= ticks)
		{
			// turn off 
			scriptTimer = 0;

			// timer has expired, cue self
			APostNotice(N_CUE, gSelf);
		}
	}
}


void
Script::CountDown(ticks_t	delay)
{
	TClock	*pClock;

	if(gClock)
	{
		pClock = ADerefAs(TClock,gClock);
		scriptTimer = pClock->GetTicks() + delay;
	}
	else
	{
		scriptTimer = ATicks() + delay;
	}
}
