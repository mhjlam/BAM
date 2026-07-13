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

void*
TMovable::operator new(size_t size)
{
	grip g = pMemMgr->Malloc(size);
	return pMemMgr->Deref(g);
}

void
TMovable::operator delete(void* p) noexcept
{
	if (!p) return;
	grip g = pMemMgr->GetGrip(p);
	if (g != INVALID_GRIP)
		pMemMgr->Free(g);
}


// Constructor
TMovable::TMovable()
{
	grip g = pMemMgr->LastAllocGrip();
	// If the most recent allocation is this object, claim that grip.
	// Otherwise (embedded value member inside a larger allocation), create a
	// lightweight NON_ALLOC grip slot pointing at this with no malloc.
	if (pMemMgr->IsGripValid(g) && pMemMgr->Deref(g) == (void*)this)
		gSelf = g;
	else
		gSelf = pMemMgr->CreateEmbeddedGrip(this);
	classID = CID_TMOV;
}


// Destructor
TMovable::~TMovable()
{
	// Release NON_ALLOC grip slot if this was an embedded object.
	// No-op if gSelf is an ALLOC grip (freed by operator delete).
	pMemMgr->ReleaseEmbeddedGrip(gSelf);
}

bool
TMovable::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&tMovableDataStart, 1, (uintptr_t)&tMovableDataEnd -
				(uintptr_t)&tMovableDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&tMovableDataStart, 1, (uintptr_t)&tMovableDataEnd -
				(uintptr_t)&tMovableDataStart, pFile);
			break;

		default:
			return(false);
	}
	return(true);
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
	gContext = grip{};
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
		gContext = grip{};
	}
}

bool
Object::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&objectDataStart, 1, (uintptr_t)&objectDataEnd -
				(uintptr_t)&objectDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&objectDataStart, 1, (uintptr_t)&objectDataEnd -
				(uintptr_t)&objectDataStart, pFile);
			break;

		default:
			return(false);
	}
	return(true);
}

// Default handle event for all Objects, does nothing,
// and advises the EventMgr that its done nothing (returns false)
//
bool
Object::HandleMsg(Message* pMsg)
{
	(void)pMsg;

	return false;
}


// Default for getting grip to an objects scrimage. If we call this 
// here, it means there is no scrimage.
//
grip
Object::GetScrimage()
{
	return grip{};
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
	ASSERT ((!gContext) || (gContext == gC));

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
Object::SetName(const char* n)
{
	memset(name, 0, OBJ_NAME_SIZE);
	strncpy(name, n, OBJ_NAME_SIZE - 1);
}


bool
Object::InMask(Message* pMsg)
{
	bool	ret = false;

	if (pMsg->type == MSG_NOTICE ||
		 (pMsg->type == MSG_EVENT && (msgMask & pMsg->event.type))
		)
	{
		ret = true;
	}
	return ret;
}
