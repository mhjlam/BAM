//
// APIRES.CPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Resource manager specific functions
//
//----[]-------------------------------------------------------------

#include "apires.hpp"
#include "apimem.hpp"
#include "os.hpp"	
#include "debug.hpp"

#define NDEBUG

extern Debugger	ResMgrDebug;

// Signal whether the manager has been created
bool
AResMgr()
{
	return pResMgr ? TRUE : FALSE;
}

bool
APurge(uint size)
{
	bool	ret = TRUE;

	if (!pResMgr || !pResMgr->Purge((uint16) size))
	{
		///////////////////////////////DEBUG
		#ifndef NDEBUG
			// looking for fragmentation
			pResMgr->DumpLocked();
		#endif 
		///////////////////////////////DEBUG
		ret = FALSE;
	}
	return ret;
}

void
AFlush(grip g, bool delres)
{
	pResMgr->Flush(g, delres);
}


// Return the grip of a created cel
grip
ACreateCel(uint* resNum, coord ox, coord oy, uint w, uint h, uchar fill, int32 pri)
{
	CelArrayElement	celArrayEl;
	uint32				theSize;
	grip					g1;
	grip					g2;

	#ifndef NDEBUG
	ResMgrDebug.Out("ACreateCel() ");
	#endif

	celArrayEl.celHeader.orgX = ox;
	celArrayEl.celHeader.orgY = oy;
	celArrayEl.celHeader.width = (uint16) w;
	celArrayEl.celHeader.height = (uint16) h;
	celArrayEl.celHeader.priority = (uint16) pri;
	celArrayEl.celHeader.offset = 0;

	theSize = L2R(w) * h + sizeof(CelHeader);

	TRACK_MEM("CreateCel: g1");
	g1 = AMalloc(theSize);
	celArrayEl.celData = ADerefAs(uchar, g1);
	memset(celArrayEl.celData, fill, theSize);
	TRACK_MEM("CreateCel: g2");
	g2 = pResMgr->CreateCel(&celArrayEl, resNum);
	AFree(g1);

	return g2;
}


// Return the grip of a cel created via a CelArrayElement
grip
ACreateEasyCel(CelArrayElement* celData, uint* resNum)
{
	return pResMgr->CreateCel(celData, resNum);
}


grip
ACreateAnim(uint numCels, CelArrayElement* celData[], uint* resNum)
{
	return pResMgr->CreateAnim(numCels, celData, resNum);
}


grip
ACloneRes(res_t rType, uint resNum, uint* newResNum, res_t newRType, uint unit)
{
	return pResMgr->CloneRes(rType, resNum, newResNum, newRType, unit);
}


// Load a cel
//
grip
ALoadDebug(char *file, int line, res_t type, uint num, bool fReturnErrorCode)
{
	#ifndef NDEBUG
		ResMgrDebug.Out("%s:%d ALoad(t%d n%u)\n", file, line, (int)type, num);
	#endif
	return(ALoad(type, num, fReturnErrorCode));
}

grip
ALoad(res_t type, uint num, bool fReturnErrorCode)
{
	return pResMgr->Load(type, num, NULL, fReturnErrorCode);
}

grip
AIsLoaded(res_t type, uint num, bool force)
{
	return pResMgr->IsLoaded(type, num, force);
}


// Return a pointer to the actual data of a resource
//
uchar*
AGetResData(grip gRes)
{
	return pResMgr->GetResData(gRes);
}


// See if a resource is there
//
bool
AExists(res_t type, uint num)
{
	return pResMgr->Exists(type, num);
}
