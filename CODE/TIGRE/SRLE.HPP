//
// SRLE.HPP
//
// December 9, 1994	CLS
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Header for SRLE compression
//
//----[]-------------------------------------------------------------


#ifndef	SRLE_hpp
#define	SRLE_hpp

#include "types.hpp"

uint32	PackSRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader);
void		ExpandSRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader);

struct SRLEline
{
	char		skipNum;
	int		runLength;
	uchar*	runData;
};

#endif

