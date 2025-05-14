//
// TEXT.CPP
//
// February 3, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	Various objects which retrieve or display text
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "mono.hpp"
#include "resmgr.hpp"
#include "savebase.hpp"
#include "text.hpp"


//----------------------------------------------------------
//	Squib
//----------------------------------------------------------

int	squibLanguageNum = LANG_DEFAULT;

void SetLanguage(int newLangNum)
{
	squibLanguageNum = newLangNum;
}

SquibRes::SquibRes(void)
{
	classID = CID_SQUIB;
}

// unlock whatever resource is currently locked by us
void
SquibRes::Cleanup(void)
{
	if(gRes)
	{
		if(pMemMgr->CheckGrip(gRes) == GRIP_VALID)
		{
			Unlock();
		}
	}
}

bool
SquibRes::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&squibDataStart, 1, (int)&squibDataEnd -
				(int)&squibDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&squibDataStart, 1, (int)&squibDataEnd -
				(int)&squibDataStart, pFile);
			break;
	}
	return(TRUE);
}

// Load and lock a squib res, return ptr to a squib. Will remain
//	locked until calling Unlock or destroyed.
// If handErrorBack is TRUE, NULL returned on error
// If handErrorBack is FALSE (default), APanic on error
//
char*
SquibRes::Load(uint resNum, int squibId, bool handErrorBack)
{
	RSquibData*	pData;
	uint			offset = 0;
	char*			sz;
	int			low;
	int			high;
	int			midpoint;

	// tweak resource number for easy localization
	resNum += squibLanguageNum;

	pData = (RSquibData*) AutoRes::Lock(RES_SQUIB, resNum);

	// the id numbers are sorted so that we can do a b-search.
	low = 0;
	high = pData->cSquibs - 1;

	do {
		midpoint = (low + high)/2;

		if (squibId < pData->aSquibHeads[midpoint].id)
		{
			high = midpoint - 1;
		}
		else
		{
			low = midpoint + 1;
		}

	} while(squibId != pData->aSquibHeads[midpoint].id && high >= low);

	if (squibId == pData->aSquibHeads[midpoint].id)
	{
		// found the squib
		offset = pData->aSquibHeads[midpoint].offset;

		sz = (char*) pData + sizeof(RSquibData) + 
					(sizeof(RSquibHead) * (pData->cSquibs - 1));
		sz += offset;
	}
	else
	{
		// didn't find squib id
		if (handErrorBack)
		{
			// they want to handle the error
			sz = NULL;	
		}
		else
		{
			char	sz[40];
			sprintf(sz, "Can't find squib %d - %d", resNum, squibId);
			APanic(sz);
		}
	}


	return sz;
}


#ifdef USING_RESPONDER
	
	Responder class is not in use.

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//		Responder class
//---------------------------------------------------------------------
//---------------------------------------------------------------------

Responder::Responder()
{
	classID = CID_RESPONDER;
	resNum = 0;
	baseSquibId = 0;
}


void
Responder::Setup(uint res, uint squibId)
{
	resNum = res;
	baseSquibId = squibId;
}


//--------------------------------------------------------------------
//	Filters events, adding click type to mouse down events

bool
Responder::HandleMsg(Message* pMsg)
{
	SquibRes	squib;
	char*		sz;

	if (pMsg->type == E_MOUSE_DOWN) 
	{
		if (rect.Contains(pMsg->event.x, pMsg->event.y))
		{
			// verify that Setup has be called
			ASSERT(resNum);

			sz = (char*) squib.Load(resNum, baseSquibId);

			// print msg
			if (pMono) 
			{
				pMono->Out(sz);
				pMono->Out("\n\n");
			}
			return TRUE;
		}
	}
	return FALSE;
}

bool
Responder::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&responderDataStart, 1, (int)&responderDataEnd -
				(int)&responderDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&responderDataStart, 1, (int)&responderDataEnd -
				(int)&responderDataStart, pFile);
			break;
	}
	return(TRUE);
}


//-------[ UtilResponder ]--------------------------------------------

// default constructor
//
UtilResponder::UtilResponder()
{
	classID = CID_UTILRESPONDER;
	type = UT_RESPONDER;
	gScrim = NULL;
}


// default constructor
//
UtilResponder::UtilResponder(uint res, uint squib)
{
	type = UT_RESPONDER;
	responder.Setup(res, squib);
	gScrim = 0;
}


void
UtilResponder::Setup(void* pvGuy)
{
	Guy*	pGuy = (Guy*) pvGuy;

	gContext = pGuy->gContext;	
	gScrim = pGuy->gSelf;

	// set to receive messages
	msgMask = E_MOUSE_DOWN;
	(ADerefAs(Context, gContext))->AddObject(gSelf, CT_MSGS, pGuy->scrim.priority);

	responder.rect = pGuy->scrim.rect;
}


void
UtilResponder::Cleanup()
{
	(ADerefAs(Context, gContext))->CutObject(gSelf, CT_MSGS);
}


bool
UtilResponder::HandleMsg(Message* pMsg)
{
	Guy*	pGuy;

	if (pMsg->type != E_MOUSE_DOWN)
	{
		return FALSE;
	}

	// make sure rectangle is up to date
	pGuy = ADerefAs(Guy, gScrim);
	responder.rect = pGuy->scrim.rect;

	return responder.HandleMsg(pMsg);
}

bool
UtilResponder::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&utilResponderDataStart, 1, (int)&utilResponderDataEnd -
				(int)&utilResponderDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&utilResponderDataStart, 1, (int)&utilResponderDataEnd -
				(int)&utilResponderDataStart, pFile);
			break;
	}
	return(TRUE);
}
#endif	// USING_RESPONDER

