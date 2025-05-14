//
// RESOURCE.CPP
//
// November 12, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include "apimem.hpp"
#include "apires.hpp"
#include "resmgr.hpp"
#include "resource.hpp"
#include "debug.hpp"

#define NDEBUG
extern Debugger ResMgrDebug;

// Default constructor
Resource::Resource()
{
	int i;
						  
	type = RES_CEL;	// "res_t" version of 0
	number = 0;
	lockCount = 0;
	attr = RA_MOVEABLE;
	compType = CMP_NONE;
	size = 0;
	time = 0;
	for (i = 0; i < 4; i++)
	{
		data[i] = 0;
	}
	numHeaders = 0;
	useCount = 0;
	purgeType = RES_NOPURGE;

	SetName("Resource");
}


// Default destructor
Resource::~Resource()
{
	// Tell the resource manager to get rid of us, but don't try to
	// invoke our destructor again
	if (AResMgr())
	{
		AFlush(gSelf, FALSE);
	}

	#ifndef NDEBUG
 	ResMgrDebug.Out("~Res() g%d type%u num%u\n", gSelf, (uint)type, number);
	#endif
}


// copy this member into the passed grip.  return the
// pointer to the copied member.
Resource*
Resource::CopyAndInit(grip gRes, res_t rType, uint rNum)
{
	Resource*	pRes;

	pRes = ADerefAs(Resource, gRes);
	memcpy(pRes, this, sizeof(Resource));
	pRes->gSelf = gRes;
	pRes->Init(rType, rNum);

	return pRes;
}


// Initialize the resource according to attribute flags
//
void
Resource::Init(res_t rType, uint rNum)
{
	type = rType;
	number = rNum;

	#ifndef NDEBUG
	ResMgrDebug.Out("Init() g%d type%u num%u\n", gSelf, (uint)type, rNum);
	#endif

	// Figure out what the default attributes should be for this type
	if (pResMgr->config.HasParam(type, "G"))
	{
		SetAttr(RA_SYS_GRIP, TRUE);
	}

	if (pResMgr->config.HasParam(type, "H"))
	{
		SetAttr(RA_HEADERLESS, TRUE);
	}

	if (pResMgr->config.HasParam(type, "L"))
	{
		Lock();
	}

	if (pResMgr->config.HasParam(type, "M"))
	{
		SetAttr(RA_MOVEABLE, FALSE);
	}

	if (pResMgr->config.HasParam(type, "S"))
	{
		SetAttr(RA_SAVEABLE, TRUE);
	}
}


// Lock the resource.
void
Resource::Lock()
{
	attr |= RA_LOCKED;
	++lockCount;
	#ifndef NDEBUG
	ResMgrDebug.Out("Lock() g%d t%d n%u locks%d\n", gSelf, (int)type,
		(uint)number, lockCount);
	#endif
}


// Unlock the resource.  If 'force' is passed and TRUE, the
// lock count will be set to 0.  Default is FALSE.
void
Resource::Unlock(bool force)
{
	if (force)
	{
		lockCount = 1;
	}

	if(lockCount > 0)
	{
		lockCount--;

		if(!lockCount)
		{
			attr &= ~RA_LOCKED;
		}
	}
	#ifndef NDEBUG
	ResMgrDebug.Out("Unlock(%d) g%d t%d n%u locks%d\n", force? 1: 0,
		gSelf, (int)type,	(uint)number, lockCount);
	#endif
}


// Check to see if an attribute is set.
bool
Resource::CheckAttr(uint16 a)
{
	return (attr & a) ? TRUE : FALSE;
}


// Set an attribute.
void
Resource::SetAttr(uint16 a, bool on)
{
	if (on)
	{
		attr |= a;
	}
	else
	{
		attr &= ~a;
	}
}


// Return a pointer to the actual resource data.
uchar*
Resource::Data()
{
	return ((uchar *) this + sizeof(Resource));
}


// Return a pointer to a cel header.  'c' is a 1-relative cel number.
//
CelHeader*
Resource::GetHeader(uint c)
{
	return &((CelHeader*) Data())[c - 1];
}


// Return whether this resource is dynamic or not.
//
bool
Resource::IsDynamic()
{
	return (number & RES_DYNAMIC) ? TRUE : FALSE;
}


// Set the use count to a specific value.
//
void
Resource::SetUseCount(uint newCount)
{
	useCount = newCount;

	if (!useCount)
	{
		// No one's using us, see if we want to purge the resource
		Purge();
	}
}


// Return the current use count.
//
uint
Resource::GetUseCount()
{
	return useCount;
}


// Purge the resource.
// This method gets called when the useCount of a resource drops to 0.
// If the purgeType is set to RES_PURGE, the resource will be yanked out
// of the resource list and destroyed, whether it is dynamic or not.
//
bool
Resource::Purge()
{
	if (purgeType == RES_PURGE)
	{
		delete this;
		return TRUE;
	}
	return FALSE;
}
