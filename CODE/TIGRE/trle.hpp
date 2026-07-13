//
// TRLE.HPP
//
// February 8, 1994
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Header for resource creation.
//
//----[]-------------------------------------------------------------


#ifndef	TRLE_hpp
#define	TRLE_hpp

#include "types.hpp"
#include	"writeres.hpp"

#define	MAX_TRLE_PAGES 5

// Table at the start of rl-encoded cel data
struct ScanOffsetTable
{
	uint16	lastYofPage[MAX_TRLE_PAGES];
	uint16	offset[1];
};


void		ExpandTRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader);
uint32	PackTRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader);

uchar*	PackRLE(uchar* pDst, uchar* pSrc, short n);
uchar*	ExpandRLE(uchar* pDst, uchar* pSrc, coord width);
uchar*	AdvanceRLE(uchar* pSrc, short n);
uchar* 	ScanAddress(ScanOffsetTable* pTable, coord row);
uint32	CopyTRLE(uchar* pResData, CelArrayElement* pElem);

#endif

