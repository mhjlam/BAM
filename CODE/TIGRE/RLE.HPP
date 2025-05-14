//
// RLE.HPP
//
// February 8, 1994
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Header for resource creation.
//
//----[]-------------------------------------------------------------


#ifndef	RLE_hpp
#define	RLE_hpp

#include "types.hpp"

#define	MAX_TRLE_PAGES 5


uchar*	PackRLE(uchar* pDst, uchar* pSrc, short n);
uchar*	ExpandRLE(uchar* pDst, uchar* pSrc, coord width);
uchar*	AdvanceRLE(uchar* pSrc, short n);

// Table at the start of rl-encoded cel data
struct ScanOffsetTable
{
	uint16	lastYofPage[MAX_TRLE_PAGES];
	uint16	offset[1];
};


uchar* 	ScanAddress(ScanOffsetTable* pTable, coord row);

#endif

