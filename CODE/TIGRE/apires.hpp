//
// APIRES.HPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Definitions for API calls specific to the resource manager.
//
//----[]-------------------------------------------------------------


#ifndef	apires_hpp
#define	apires_hpp


#include "resmgr.hpp"
#include "resource.hpp"
#include "types.hpp"
#include "writeres.hpp"


bool		AResMgr();

void		AFlush(grip g, bool delres = true);

// Create & clone cels
grip		ACreateCel(uint*	resNum,
					  	coord	ox,
					  	coord	oy,
					  	uint	w,
					  	uint	h,
					  	uchar	fill = 0,
					  	int32	pri = 1);
grip		ACreateEasyCel(CelArrayElement* celData, uint* resNum);
grip		ACreateAnim(uint numCels, CelArrayElement* celData[], uint* resNum);
grip		ACloneRes(res_t rType, uint resNum, uint* newResNum, res_t newRType, uint unit = 0);

// Load a resource
grip 		ALoadDebug(const char *file, int line, res_t type, uint num, bool fReturnErrorCode = false);
grip		AIsLoaded(res_t type, uint num, bool force = false);
grip		ALoad(res_t type, uint num, bool fReturnErrorCode = false);

// See if a resource is there
bool		AExists(res_t type, uint num);


// Return the data of a resource
uchar*	AGetResData(grip gRes);

#endif
