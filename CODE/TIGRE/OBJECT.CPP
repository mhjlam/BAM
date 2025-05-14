//
// OBJECT.CPP
//
// November 20, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All Rights Reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "object.hpp"
#include "savebase.hpp"
#include "tigre.hpp"


//----------------------------------------------------------------------
// TMovable Class
//
// Base Class for Movable things
//----------------------------------------------------------------------

// Constructor
TMovable::TMovable()
{
	gSelf = AAssignGrip(this, FALSE, TMOV_GRIP);
	classID = CID_TMOV;
}


// Destructor
TMovable::~TMovable()
{
	// release grip 
	AReleaseGrip(gSelf);
}

bool
TMovable::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&tMovableDataStart, 1, (int)&tMovableDataEnd -
				(int)&tMovableDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&tMovableDataStart, 1, (int)&tMovableDataEnd -
				(int)&tMovableDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

// Copy from the source pointer into this.
// Size cannot be determined with this, so it
// must be passed.
//
void
TMovable::Copy(void* source, size_t sizeToCopy)
{
	grip myGrip = gSelf;

	memcpy(this, source, sizeToCopy);
	gSelf = myGrip;
}


// This is an example of the Copy function that should
// be placed in any class that you want to use it.
// NOTE: sizeof is determined at compile time, so this MUST
//       be placed in each class that you want to use it
//       so that the proper size will be passed.
//
void
TMovable::Copy(void* source)
{
	Copy(source, sizeof(*this));	
}


//----------------------------------------------------------------------
// Object Class
//----------------------------------------------------------------------

// Constructor used by all objects
Object::Object()
{
	classID = CID_OBJECT;
	gContext = 0;
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN;
	SetName("Object");
	choreoId = 0;
}


// Destructor used by all objects
Object::~Object()
{
	// if installed to a context, remove from context
	if (gContext)
	{
		(ADerefAs(Context, gContext))->CutObject(gSelf, CT_ALL);
		gContext = 0;
	}
}

bool
Object::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&objectDataStart, 1, (int)&objectDataEnd -
				(int)&objectDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&objectDataStart, 1, (int)&objectDataEnd -
				(int)&objectDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

// Default handle event for all Objects, does nothing,
// and advises the EventMgr that its done nothing (returns FALSE)
//
bool
Object::HandleMsg(Message* pMsg)
{
	pMsg = pMsg;	// Gets rid of unreferenced warning

	return FALSE;
}


// Default for getting grip to an objects scrimage. If we call this 
// here, it means there is no scrimage.
//
grip
Object::GetScrimage()
{
	return 0;
}


// Default for attaching a resource to this object.  Will be handled
// differently by all objects that make use of external resources.
//
void
Object::SetResCount(res_t oldRType, uint oldResNum, res_t rType, uint resNum)
{
	grip			gRes;
	Resource*	pRes;
	
	// only do work if something has changed
	if (oldRType != rType ||
		 oldResNum != resNum)
	{
		// something changed.  update things

		gRes = AIsLoaded(oldRType, oldResNum);
		if (gRes)
		{
			pRes = ADerefAs(Resource, gRes);
			pRes->SetUseCount(pRes->GetUseCount() - 1);
		}

		gRes = AIsLoaded(rType, resNum);
		if (gRes)
		{
			pRes = ADerefAs(Resource, gRes);
			pRes->SetUseCount(pRes->GetUseCount() + 1);
		}
	}
}


// Set object's Message mask, subscribe to Message stream
void
Object::SetContext(grip gC)
{
	// ASSERT that the context isn't set or isn't changing
	ASSERT ((gContext == 0) || (gContext == gC));

	gContext = gC;
}


// Set object's Message mask, subscribe to Message stream
void
Object::ResetMsgPri(int32 pri)
{
	Context*	pContext;

	// error check
	if (!gContext)
	{
		APanic("No context assigned to Object");
	}

	pContext = ADerefAs(Context, gContext);

	pContext->ResetMsgPri(gSelf, pri);
}


void
Object::SetName(char* n)
{
	memset(name, NULL, OBJ_NAME_SIZE);
	strncpy(name, n, OBJ_NAME_SIZE - 1);
}


bool
Object::InMask(Message* pMsg)
{
	bool	ret = FALSE;

	if (pMsg->type == MSG_NOTICE ||
		 (pMsg->type == MSG_EVENT && (msgMask & pMsg->event.type))
		)
	{
		ret = TRUE;
	}
	return ret;
}
