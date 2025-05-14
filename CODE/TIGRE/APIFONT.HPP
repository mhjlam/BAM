//
// APIFONT.HPP
//
// November 12, 1993
// WATCOM: October 4, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Definitions for API calls specific to the font manager.
//
//----[]-------------------------------------------------------------


#ifndef	apifont_hpp
#define	apifont_hpp

#include <limits.h>

#include "fontmgr.hpp"
#include "tigre.hpp"
#include "types.hpp"


grip	ACreateText(uint* resNum, char* msg, uchar fore, uchar back,
						uint maxWidth = 0, justify just = DG_JUST_LEFT);

grip	ACreateProcText(uint* resNum, char* msg, uchar fore, DrawProc ppDraw,
								uint margin = 8, uint maxWidth = 0,
								justify just = DG_JUST_LEFT);

void	ACreateGripText(grip gRes, char* msg, uchar fore,
								uchar back = 254, uint maxWidth = 0,
								justify just = DG_JUST_LEFT);

grip	ACreatePartialText(uint* resNum, char* msg, uchar fore, DrawProc ppDraw,
									uint margin = 8, uint maxWidth = 0,
									justify just = DG_JUST_LEFT, uint topLine = 0,
									uint bottomLine = UINT_MAX);

uint	AGetCharWidth(char c);

void	ASetString(uint hOffset, uint vOffset, char* msg, uchar*	pDst,
						uint maxWidth = 0, uint margin = DG_MARGIN,
						justify just = DG_JUST_LEFT,
						uint topLine = 0, uint bottomLine = UINT_MAX);

void	AGetCursorPosn(char* msg, uint cursor, uint maxWidth,
							uint& hc, uint& vc, uint margin);

void	ASetColors(uchar* newColors, uchar* oldColors = NULL, bool alt = FALSE);


// Property access

uchar	AFontColor(uint i);
uint	AFontPoint();

void	ASetFontColor(uint colorIndex, uchar vgaPen);

#endif
