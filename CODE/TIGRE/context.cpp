//
// CONTEXT.CPP
//
// December 09, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "list.hpp"
#include	"savemgr.hpp"
#include "scrimage.hpp"


//----[ Context ]----------------------------------------------------

const	uint32	CONTEXT_KEY = 0xabbacddc;

ContextMgr* pContextMgr = nullptr;

// Constructor
Context::Context(void)
{
	key = CONTEXT_KEY;

	fIsActive = false;

	lServiceables.resizable = true;
	lReceivers.resizable = true;
	lRoster.resizable = true;

	lServiceables.fDuplicates = false;
	lReceivers.fDuplicates = false;
	lRoster.fDuplicates = false;

	// the owning context of a context is itsself
	gContext = gSelf;
	gModerator = grip{};
	mode = M_MODELESS;

	// add context to ContextMgr
	ASSERT (pContextMgr);
	pContextMgr->Add(gSelf);
	SetName("Context");
}


// Destructor
Context::~Context(void)
{
	// verify that this object is a context
	ASSERT (key == CONTEXT_KEY);

	gContext = grip{};

	ClearLists();
	
	// remove context from ContextMgr
	ASSERT (pContextMgr);
	pContextMgr->Cut(gSelf);
}



//-------------------------------------------------------------------
// Add object to context
//
void
Context::AddObject(grip gObj, uint16 flags, int32 pri)
{
	// verify that this object is a context
	ASSERT (key == CONTEXT_KEY);

	if (flags & CT_MSGS)		AddReceiver(gObj, pri);
	if (flags & CT_SERV)		AddServiceable(gObj);
	if (flags & CT_ROST)
	{
		grip		gScrim;
		Object*	pObj;

		pObj = ADerefAs(Object, gObj);
		pObj->gContext = gSelf;

		gScrim = pObj->GetScrimage();
		if (gScrim)
		{
			AddScrimage(gScrim);
		} else {
			//APanic("Object added to roster for no reason");
			ASSERT(false);
		}
	}
}



// cut object from context
void
Context::CutObject(grip gObj, uint16 flags)
{
	// verify that this object is a context
	ASSERT (key == CONTEXT_KEY);

	if (flags & CT_MSGS)		CutReceiver(gObj);
	if (flags & CT_SERV)		CutServiceable(gObj);
	if (flags & CT_ROST)
	{
		grip		gScrim;
		Object*	pObj;

		pObj = ADerefAs(Object, gObj);

		gScrim = pObj->GetScrimage();
		if (gScrim)
		{
			CutScrimage(gScrim);
		}
	}
}


//-------------------------------------------------------------------

// Subscribe to Services
void
Context::AddServiceable(grip gPeriodic)
{
	lServiceables.Add(grip_to_ptr(gPeriodic), 0, L_FRONT);
}



// Cancel subscription to Services
void
Context::CutServiceable(grip gPeriodic)
{
	if (lServiceables.Find(grip_to_ptr(gPeriodic))) {
		lServiceables.Delete(grip_to_ptr(gPeriodic));
	}
}


// Subscribe to Messages
void
Context::AddReceiver(grip gPeriodic, int32 pri)
{
	lReceivers.Add(grip_to_ptr(gPeriodic), pri, L_FRONT);

	// flag sort as needed, don't do it now, cuz we might be
	// in the middle of iterating the lRecievers list.
	lReceivers.fNeedsResort = true;
}



// Change priority of subscriber to Messages
void
Context::ResetMsgPri(grip gPeriodic, int32 pri)
{
	node*	pNode;

	// verify that the element isn't added to the list twice
	pNode = lReceivers.Find(grip_to_ptr(gPeriodic));

	if (!pNode)	APanic("Bogus reset of priority");

	// change key to new pri, and re-sort
	pNode->key = pri;

	// flag sort as needed, don't do it now, cuz we might be
	// in the middle of iterating the lRecievers list.
	lReceivers.fNeedsResort = true;
}



// Cancel subscription to Messages
void
Context::CutReceiver(grip gPeriodic)
{
	if (lReceivers.Find(grip_to_ptr(gPeriodic)))
	{
		lReceivers.Delete(grip_to_ptr(gPeriodic));
	}
}



// Add to roster/serviceables
void
Context::AddScrimage(grip gScrim)
{
	lRoster.Add(grip_to_ptr(gScrim));
	
	if (fIsActive)
	{
		AAddScrim(ADerefAs(Scrimage, gScrim));
	}
}



// Cancel subscription to Services
void
Context::CutScrimage(grip gScrim)
{
	if (lRoster.Find(grip_to_ptr(gScrim)))
	{
		lRoster.Delete(grip_to_ptr(gScrim));
	}

	ADeleteScrim(ADerefAs(Scrimage, gScrim));
}



//-------------------------------------------------------------------
// Pass cycle() on to serviceables
//
void
Context::Cycle()
{
	Periodic*	pPer;
	grip			g;

	if (!fIsActive) 
	{
		return;
	}

	// pass message on to all recievers
	for (g = ptr_to_grip(lServiceables.FirstValue());
			g;
			g = ptr_to_grip(lServiceables.NextValue()))
	{
		pPer = ADerefAs(Periodic, g);
		pPer->Cycle();
	}
}




bool
Context::HandleMsg(Message* pMsg)
{
	Object*	pObj;
	grip		g;

	// verify that this object is a context
	ASSERT (key == CONTEXT_KEY);

	if (!fIsActive) 
	{
		return false;
	}

	if (lReceivers.fNeedsResort)
	{
		lReceivers.SortByKey(SORT_DESCENDING);
	}

	// Pass message on to all receivers
	// If the message is an event that is in the receiver's mask or if
	// the message is a notice, send it.  Otherwise, the receiver isn't
	// interested.
	for (g = ptr_to_grip(lReceivers.FirstValue());
			g;
			g = ptr_to_grip(lReceivers.NextValue()))
	{
		pObj = ADerefAs(Object, g);
		if (pObj->InMask(pMsg) && pObj->HandleMsg(pMsg))
		{
			// Message claimed, stop passing
			return true;
		}
	}
	return false;
}



void
Context::Activate(bool fOn)
{
	// verify that this object is a context
	ASSERT (key == CONTEXT_KEY);

	// only if changing
	if (fIsActive != fOn) 
	{
		fIsActive = fOn;

		if (fOn)
		{
			AddRoster();
		}
		else
		{
			CutRoster();
		}

	}
}



void
Context::ClearLists()
{
	Activate(false);

	lServiceables.Release();
	lReceivers.Release();
	lRoster.Release();
}



//--------------------------------------------------------------
// Install roster to graphmgr
//
void
Context::AddRoster()
{
	grip			g;
	Scrimage*	pScrim;

	for (g = ptr_to_grip(lRoster.FirstValue());
			g;
			g = ptr_to_grip(lRoster.NextValue()))
	{
		pScrim = ADerefAs(Scrimage, g);
		AAddScrim(pScrim);
	}
}


//--------------------------------------------------------------
// Remove roster from graphmgr
//
void
Context::CutRoster()
{
	grip			g;
	Scrimage*	pScrim;

	for (g = ptr_to_grip(lRoster.FirstValue());
			g;
			g = ptr_to_grip(lRoster.NextValue()))
	{
		pScrim = ADerefAs(Scrimage, g);
		ADeleteScrim(pScrim);
	}
}




//---------------------------------------------------------------------
//---------------------------------------------------------------------
//		ContextMgr class
//---------------------------------------------------------------------
//---------------------------------------------------------------------

bool	ContextMgrSave(uint16 state, nlohmann::json& root);

// Constructor
ContextMgr::ContextMgr(void)
{
	// setup exceptional ptr to ContextMgr
	if (!pContextMgr)
	{
		pContextMgr = this;
		// setup for save
		AtSave(ContextMgrSave);
	}

	fQuitting = false;

	lContexts.resizable = true;
	lContexts.fDuplicates = false;

	SetName("ContextMgr");
}


// Destructor
ContextMgr::~ContextMgr(void)
{
	grip		g;
	Context*	pContext;

	for (g = ptr_to_grip(lContexts.FirstValue());
			g;
			g = ptr_to_grip(lContexts.NextValue()))
	{
		pContext = ADerefAs(Context, g);
		delete pContext;
	}

	lContexts.Release();
}



//-------------------------------------------------------------
// Check for special global event, for screen capturing, etc

HandleMsgProc	ppGlobalMsg = nullptr;


//-------------------------------------------------------------
// Pass msgs to all active contexts
bool 
ContextMgr::HandleMsg(Message* pMsg)
{
	uint16	i;
	Context*	pContext;

	if ((!ppGlobalMsg) ||
		 ((ppGlobalMsg) && (ppGlobalMsg(pMsg) == false))
		)
	{
		for (i = 1; i <= lContexts.count; i++)
		{
			pContext = ADerefAs(Context, ptr_to_grip(lContexts.At(i)));
	  		if (pContext->fIsActive)
			{
	  			if (pContext->InMask(pMsg))
				{
					if (pContext->HandleMsg(pMsg))
					{
						return true;
					}
				}

				// if is an active modal context, do not service other contexts
				if (pContext->mode == M_MODAL)
				{
					break;
				}
			}
		}
	}

	// context manager is also able to receive messages
	if ((pMsg->type == MSG_NOTICE) && (pMsg->notice.type == N_QUIT))
	{
		Quit();
	}

	return false;
}



// Pass cycle to all active contexts
void 
ContextMgr::Cycle(void)
{
	uint16	i;
	Context*	pContext;

	for (i = 1; i <= lContexts.count; i++)
	{
		pContext = ADerefAs(Context, ptr_to_grip(lContexts.At(i)));
		if (pContext->fIsActive)
		{
			pContext->Cycle();

			// if is a modal context, do not service other contexts
			if (pContext->mode == M_MODAL)
			{
				break;
			}
		}
	}
}



void
ContextMgr::Add(grip gContext)
{
	lContexts.Add(grip_to_ptr(gContext), 0, L_FRONT);
}




void
ContextMgr::Cut(grip gContext)
{
	// verify that the element is there
	if (lContexts.Find(grip_to_ptr(gContext)))
	{
		lContexts.Delete(grip_to_ptr(gContext));
	}
}



void
ContextMgr::Quit()
{
	fQuitting = true;
}

bool
ContextMgrSave(uint16 state, nlohmann::json& /*root*/)
{
	switch (state)
	{
		case BEFORE_RESTORE:
			pContextMgr->lContexts.Release();
			break;

		default:
			break;
	}
	return false;
}



