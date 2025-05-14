//
// APIFONT.CPP
//
// November 12, 1993
// WATCOM: October 4, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Font manager specific functions
//
//----[]-------------------------------------------------------------


#include "apifont.hpp"


// Create text manually
grip
ACreateText(uint* resNum, char* msg, uchar fore, uchar back,
				uint maxWidth, justify just)
{
	return pFontMgr->CreateText(resNum, msg, fore, back, maxWidth, just);
}


// Create text using a DrawProc
grip
ACreateProcText(uint* resNum, char* msg, uchar fore, DrawProc ppDraw,
						uint margin, uint maxWidth, justify just)
{
 	return pFontMgr->CreateText(resNum, msg, fore, ppDraw, margin, maxWidth, just);
}


// Create text using a grip
void
ACreateGripText(grip gRes, char* msg, uchar fore, uchar back,
						uint maxWidth, justify just)
{
	pFontMgr->CreateText(gRes, msg, fore, back, maxWidth, just);
}


grip
ACreatePartialText(uint* resNum, char* msg, uchar fore, DrawProc ppDraw,
							uint margin, uint maxWidth, justify just, uint topLine,
							uint bottomLine)
{
	return pFontMgr->CreatePartialText(resNum, msg, fore, ppDraw,
														margin, maxWidth, just,
														topLine, bottomLine);
}


uint
AGetCharWidth(char c)
{
	return pFontMgr->GetCharWidth(c);
}


void
ASetString(uint hOffset, uint vOffset, char* msg, uchar*	pDst,
				uint maxWidth, uint margin, justify just,
				uint topLine, uint bottomLine)
{
	pFontMgr->SetString(hOffset, vOffset, msg, pDst, maxWidth, margin,
									just, topLine, bottomLine);
}


void
AGetCursorPosn(char* msg, uint cursor, uint maxWidth,
					uint& hc, uint& vc, uint margin)
{
	pFontMgr->GetCursorPosn(msg, cursor, maxWidth, hc, vc, margin);
}


void
ASetColors(uchar* newColors, uchar* oldColors, bool alt)
{
	uchar*	theColors = alt ? pFontMgr->altColors : pFontMgr->colors;

	if (oldColors)
	{
		memcpy(oldColors, theColors, 256);
	}
	memcpy(theColors, newColors, 256);
}



// Property access

uchar
AFontColor(uint i)
{
	return pFontMgr->colors[i];
}


uint
AFontPoint()
{
	return pFontMgr->point;
}

void	
ASetFontColor(uint colorIndex, uchar vgaPen)
{
	pFontMgr->colors[colorIndex] = vgaPen;
}

