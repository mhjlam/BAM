//
// FONTMGR.HPP
//
// February 11, 1994
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// FontMgr class definition.
//
// 	The FontMgr handles all font functions, from loading a font
//		and registering it as the current default font to copying
//		characters from the font into a cel buffer.
//
//----[]-------------------------------------------------------------

#ifndef	fontmgr_hpp
#define	fontmgr_hpp

//#include <limits.h>

#include "manager.hpp"
#include "rect.hpp"
#include "types.hpp"


#define	caret(x)	(*(msg + x) == '^' && *(msg + x + 1) != '^')
#define	cAt(x)	(*(msg + x))

#define	MAX_CHARS	256

const uchar	FNT_BACK_COLOR		= 0;
const uchar	FNT_FORE_COLOR		= 1;
const uchar FNT_ALT_COLOR		= 1;
const int 	FNT_DONT_CHANGE	= 256;

class FontMgr : public Manager, public Object
{
	public:

		FontMgr();
		virtual ~FontMgr();

		grip		curFont;		// Font currently the "default" font
		uint		curFontNum;	// Resource number of curFont
		uint		point;		// point size of current font
		uint		widest;		// widest character in current font

		uchar		colors[MAX_CHARS];		// foreground colors of font
		uchar		altColors[MAX_CHARS];	// alternate colors of font

		uint		lastHPen;	// last hPen position - used for tmps

		// Set color, returns previous color
		uchar	BackColor(int clr = FNT_DONT_CHANGE);
		uchar	ForeColor(int clr = FNT_DONT_CHANGE);
		uchar	AltColor(int clr = FNT_DONT_CHANGE);

		virtual	grip	Load(uint num);
		virtual	void	SetRes(uint resNum);
		virtual	void	Register(grip gFont), Unregister(void);
		virtual	void	TextSize(Rectangle& r, 
										char* msg, 
										uint maxWidth = 0, 
										coord margin = DG_MARGIN,
										uint numLines = 0);
		virtual	void	SetString(	uint		hOffset,
											uint		vOffset,
											char*		msg,
											uchar*	pDst,
											uint		maxWidth = 0,
											uint		margin = DG_MARGIN,
											justify	just = DG_JUST_LEFT,
											uint		topLine = 0,
											uint		bottomLine = UINT_MAX);
		virtual	grip	CreateText(uint* resNum,
											char* msg,
											uchar fore,
											uchar back,
											uint maxWidth = 0,
											justify just = DG_JUST_LEFT);
		virtual	grip	CreateText(uint* resNum,
											char* msg,
											uchar fore,
											DrawProc ppDraw,
											uint margin = 8,
											uint maxWidth = 0,
											justify just = DG_JUST_LEFT);
		virtual	void	CreateText(grip gRes,
											char* msg,
											uchar fore,
											uchar back = 254,
											uint maxWidth = 0,
											justify just = DG_JUST_LEFT);
		virtual	grip	CreatePartialText(uint* resNum,
											char* msg,
											uchar fore,
											DrawProc ppDraw,
											uint margin = 8,
											uint maxWidth = 0,
											justify just = DG_JUST_LEFT,
											uint topLine = 0,
											uint bottomLine = UINT_MAX);
		virtual	void	GetCursorPosn(char* msg,
											uint cursor,
											uint maxWidth,
											uint& hc,
											uint& vc,
											uint margin);
		virtual	uint	GetCharWidth(uchar c);
		virtual	uint32	GetCharOffset(uchar c);

		uint	SetChar(uint hPen,
								uint vPen,
								uchar ch,
								uchar* pDst,
								uchar* pSrc,
								bool useAltColors);

	private:

		uint	CharSize(uchar c);
		uint	WordSize(char* msg, uint& startChar);
		uint	WordSize(char* msg, uint& startChar, uint maxWidth);
		uint	LineSize(char* msg, uint maxWidth, uint& startChar);
		void	SetHPen(uint& hPen,
							char* msg,
							uint startChar,
							uint maxWidth = 300,
							justify just = DG_JUST_LEFT,
							uint hOffset = 0);
};


extern FontMgr* pFontMgr;
#endif


/*

Font Colors:

Fonts have three colors: foreground, background, and alternate.  Font files
contain bitmap data that will consist of 0x00's (indicating background) and
0xff's (indicating foreground).  When transferring a character from a font
into a buffer, all 0x00's are replaced with the font's background, and all
0xff's are replaced with the font's foreground.

In addition, the alternate color has been provided for "in-string" high-
lighting.  For example, the accelerator key of a button could be displayed
in a different color by following these steps:

	1.	Set the foreground of the font to be black (0)
	2. Set the background of the font to be skip (254)
	3. Set the alternate of the font to be red
	4. When creating the button, use a string such as "^C^ancel"

The carets (^) indicate when color should be switched from foreground to
alternate and vice versa.  If an actual caret character is required in the
string, use two (as in "This is a caret: ^^");

*/
