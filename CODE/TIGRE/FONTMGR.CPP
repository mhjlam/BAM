//
// FONTMGR.CPP
//
// February 11, 1994
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// For Multi-Color fonts use the colors array, starting at
// index 1 for the first forecolor, 2 for the 2nd and so on.
// colors[0] is the backColor.  
//
// If you are going to use altColor, you should set the altColors
// array for each color	that you have set in the colors array.
// This means that you should also set it for backColor (colors[0]).
//
// When indexing the color array for just the basic colors, you 
// should use the following defines: 
//
//		FNT_BACK_COLOR
//		FNT_FORE_COLOR
//		FNT_ALT_COLOR    (this is the alt color for the fore color
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "fontmgr.hpp"
#include "mono.hpp"
#include "palette.hpp"
#include	"savemgr.hpp"


bool	FontMgrSave(uint16 state, FILE *fp);

FontMgr* pFontMgr = NULL;

// Constructor - register us with the exceptional pointers
FontMgr::FontMgr()
{
	ForeColor(CI_WHITE);	// White
	AltColor(CI_BLACK);	// Black

	// setup backColor
	colors[FNT_BACK_COLOR] = altColors[FNT_BACK_COLOR] = CI_SKIP;

	curFont = 0;

	// setup exceptional ptr to FontMgr
	if (!pFontMgr)
	{
		pFontMgr = this;
		// setup for save
		AtSave(FontMgrSave);
	}
}


// Destructor
FontMgr::~FontMgr()
{
	pFontMgr = NULL;
}


//--------------------------------------------------
// Get/Set colors

uchar
FontMgr::BackColor(int clr)
{
	uchar oldColor = colors[FNT_BACK_COLOR];

	// check if we don't want to change the color
	if (clr == FNT_DONT_CHANGE)
	{
		return oldColor;
	}

	return (colors[FNT_BACK_COLOR] = (uchar) clr);
};


uchar
FontMgr::ForeColor(int clr)
{
	uchar oldColor = colors[FNT_FORE_COLOR];

	// check if we don't want to change the color
	if (clr == FNT_DONT_CHANGE)
	{
		return oldColor;
	}

	return (colors[FNT_FORE_COLOR] = (uchar) clr);
}


uchar
FontMgr::AltColor(int clr)
{
	uchar oldColor = altColors[FNT_ALT_COLOR];

	// check if we don't want to change the color
	if (clr == FNT_DONT_CHANGE)
	{
		return oldColor;
	}

	return (altColors[FNT_ALT_COLOR] = (uchar) clr);
}


//-----------------------------------------------------


// Load a font resource.
grip
FontMgr::Load(uint num)
{
	return ALoad(RES_FONT, num);
}


// Set a font to be the current font, loading it if necessary.
//
void
FontMgr::SetRes(uint resNum)
{
	Resource*	pRes;
	uchar*		pResData;
	FontHeader*	pFh;

	if (curFont)
	{
		if(pMemMgr->CheckGrip(curFont) == GRIP_VALID)
		{
			// unlock the current font
			ADerefAs(Resource, curFont)->Unlock();
		}
		// else who purged our font?! - Van
	}
	
	Object::SetResCount(RES_FONT, curFontNum, RES_FONT, resNum);

	curFontNum = resNum;
	curFont = ALoad(RES_FONT, resNum);
	pRes = ADerefAs(Resource, curFont);
	pRes->Lock();
	pResData = pRes->Data();
	pFh = (FontHeader*) pResData;
	point = pFh->height;
	widest = pFh->widest;
}

// Register a font as current.
void
FontMgr::Register(grip gFont)
{
	Resource*	pRes;
	uchar*		pResData;
	FontHeader*	pFh;

	if (curFont)
	{
		// unlock the current font
		ADerefAs(Resource, curFont)->Unlock();
	}

	curFont = gFont;
	pRes = ADerefAs(Resource, curFont);
	pRes->Lock();
	pResData = pRes->Data();
	pFh = (FontHeader*) pResData;
	point = pFh->height;
	widest = pFh->widest;
}

// unregister (unlock) whatever font is current
void
FontMgr::Unregister(void)
{
	if(curFont)
	{
		// unlock the current font
		ADerefAs(Resource, curFont)->Unlock();
		curFont = 0;
		curFontNum = 0;
	}
}


// Measure how large a text box must be, given the message and the maximum
// width, and return the coordinates in the passed rectangle.
//
void
FontMgr::TextSize(Rectangle& r, char* msg, uint maxWidth, coord margin, uint numLines)
{
	uint	i;
	uint	maxWide = 0;	// widest line in the dialog - used to snug window
	uint	maxHigh = point;
	uint	lsize;
	uint	strLength = strlen(msg);

	if (!maxWidth)
	{
		maxWidth = 300;
	}
	maxWidth -= margin * 2;

	r.Set(0, 0, 0, 0);

	for (i = 0; i < strLength;)
	{
		lsize = LineSize(msg, maxWidth, i);
		maxWide = AMax(maxWide, lsize + (margin * 2));
		if (i < strLength)
		{
			maxHigh += point;
		}
	}
	lastHPen = lsize;

	// if partial text box
	if (numLines)
	{
		maxHigh = AMin(maxHigh, numLines * point);
	}

	r.Set(0, 0, maxWide, maxHigh+(margin*2));
}


// Copy the string into the cel data pointer.  'hOffset' and 'vOffset'
//	are pixel offsets into the cel data at which to start the copy.
// 'pDst' and 'pSrc' are pointers to the cel data and font data, respect.
void
FontMgr::SetString(
				uint		hOffset,
				uint		vOffset,
				char*		msg, 
				uchar*	pDst, 
				uint		maxWidth, 
				uint		margin,
				justify	just,
				uint		topLine,
				uint		bottomLine)
{
	if(!curFont)
	{
		pMono->Out("FontMgr::SetString(%s) - err, !curFont\n", msg);
		return;
	}
	if(pMemMgr->CheckGrip(curFont) != GRIP_VALID)
	{
		pMono->Out("FontMgr::SetString(%s) - err, curFont invalid!\n", msg);
		return;
	}

	uint		i, j, k;
	uchar*	pSrc = AGetResData(curFont);
	uint		hPen;
	uint		vPen = vOffset;
	uint		wordWidth;
	uint		startChar;
	uint		curLine = 0;
	uint		textWidth = maxWidth - (margin*2);
	bool		useAltColors = FALSE;
	uint		startLineHPen;
	bool		wrapNewLine = FALSE;
	bool		forceNewLine = FALSE;

	// Modify the hPen according to justification
	// Note that the textWidth is the max width of the text area not
	// including margins

	SetHPen(hPen, msg, 0, textWidth, just, hOffset);
	startLineHPen = hPen;

	// Go through each character in the string...
	i = 0;
	while (i < strlen(msg))
	{
		// Check for formatting characters
		if (cAt(i) == '\\')
		{
			switch (cAt(++i))
			{
				case 'n':
				case 'N':
					forceNewLine = TRUE;
					curLine++;
					vPen = ((curLine - topLine) * point) + vOffset;
					SetHPen(hPen, msg, i + 1, textWidth, just, hOffset);
					startLineHPen = hPen;
					i++;
					break;

				default:
					break;
			}
		}
		if (cAt(i) == '\n')
		{
			forceNewLine = TRUE;
			curLine++;
			vPen = ((curLine - topLine) * point) + vOffset;
			SetHPen(hPen, msg, ++i, textWidth, just, hOffset);
			startLineHPen = hPen;
		}

		if (i >= strlen(msg))
		{
			// we are done
			break;
		}

		// If the current word won't fit on the line, go to next line.
		startChar = i;
		wordWidth = WordSize(msg, startChar);
		if (wordWidth + hPen > hOffset + maxWidth - margin)
		{
			//First, lets see if we've got one word that's overflowing
			//the whole text box by itself
			if(wordWidth > maxWidth)
			{
				//lets not put it on its own line first -just wrap the word now
				//if(hPen == startLineHPen)
				//{

				pMono->Out("FontMgr::SetString(%s) - err, maxWidth overflow!\n",msg);
				// lets not incr. the line yet -just write the end of the
				// line, but lets set a flag which will wrap to a new line
				// after we write it
				wrapNewLine = TRUE;
				startChar = i;
				wordWidth = WordSize(msg, startChar, maxWidth);

				//}
				//else
				//{
				//	curLine++;
				//	vPen = ((curLine - topLine) * point) + vOffset;
				//	SetHPen(hPen, msg, i, textWidth, just, hOffset);
				//	startLineHPen = hPen;
				//}

			}
			else
			{
				curLine++;
				vPen = ((curLine - topLine) * point) + vOffset;
				SetHPen(hPen, msg, i, textWidth, just, hOffset);
				startLineHPen = hPen;
			}
		}

		// if passed view area, break out
		if (curLine > bottomLine)
		{
			break;
		}

		// Stuff all the characters in until we get to the end of the word.
		for (j = i; j <= startChar; j++)
		{
			if (caret(j) && (!j || !caret(j - 1)))	// It's a single caret
			{
				// toggle the bit 
				useAltColors = useAltColors ? FALSE : TRUE;
			}
			else
			{
				//if at start of line then delete leading white space -unless
				//prev. line was forced w/ a '\n'
				if (hPen == startLineHPen && cAt(j) == ' ' && !forceNewLine)
				{
					k = j;
					//lets get it all in one shot
					while( cAt(k) == ' ' )
					{
				 		k++;
					}

					//reset the loop counter -yeh I know its ugly but its faster
					j = k;
					//lets re-adjust the line justification
					SetHPen(hPen, msg, j, textWidth, just, hOffset);

				}

				// if in view area
				if (curLine >= topLine)
				{
					hPen += SetChar(hPen, vPen, cAt(j), pDst, pSrc, useAltColors);
				}
			}
		}

		// now that a word has been written after a poss. force -reset
		forceNewLine = FALSE;

		// force new line on a text overflow
		if(wrapNewLine)
		{
			wrapNewLine = FALSE;
			curLine++;
			vPen = ((curLine - topLine) * point) + vOffset;
			SetHPen(hPen, msg, i, textWidth, just, hOffset);
			startLineHPen = hPen;
		}

		// Increment i to point to the end of the word.
		i = j;
	}
}


// Create a cel with the passed string, using the foreground and background
// color passed.  If 'maxWidth' is specified, the text will be adjusted so
// that it wraps before the width is reached.
//
grip
FontMgr::CreateText(uint* resNum, char* msg, uchar fore, uchar back, uint maxWidth, justify just)
{
	grip				gTextCel;
	uchar*			pCelData;
	uint				resourceNum;

	if (!resNum)
	{
		resNum = &resourceNum;
	}

	if (!maxWidth)
	{
		maxWidth = 300;
	}

	ForeColor(fore);
	BackColor(back);

	Rectangle r;
	TextSize(r, msg, maxWidth, 3);

	TRACK_MEM("FM::CreateText");
	gTextCel = ACreateCel(resNum, 0, 0,
									r.Wide(),
									r.High(),
									colors[FNT_BACK_COLOR]);
	pCelData = AGetResData(gTextCel);
	SetString(0, 2, msg, pCelData, r.Wide(), 0, just);

	return gTextCel;
}


// Create a cel with the passed string, draw with DrawProc.
// If 'maxWidth' is specified, the text will be adjusted so
// that it wraps before the width is reached.
//
grip
FontMgr::CreateText(uint* resNum, char* msg, uchar fore, DrawProc ppDraw, uint margin, uint maxWidth, justify just)
{
	grip				gTextCel;
	uchar*			pCelData;
	uint				resourceNum;

	if (!resNum)
	{
		resNum = &resourceNum;
	}

	if (!maxWidth)
	{
		maxWidth = 300;
	}

	ForeColor(fore);
	BackColor(CI_SKIP);

	Rectangle r;
	TextSize(r, msg, maxWidth, margin);

	TRACK_MEM("FM::CreateText");
	gTextCel = ACreateCel(resNum, 0, 0,
									r.Wide(),
									r.High(),
									colors[FNT_BACK_COLOR]);

	// draw control under-lay
	ppDraw(gTextCel);

	// draw text on top
	pCelData = AGetResData(gTextCel);
	SetString(margin, margin, msg, pCelData, r.Wide(), margin, just);

	return gTextCel;
}


// Create a cel of scrolled text, with line number topLine (zero relative) 
//	at the top of the text cel, and line number bottomLine visible (if text
//	is suffciently long) at the bottom line of the cel
//
grip
FontMgr::CreatePartialText(uint*		resNum, 
									char* 	msg, 
									uchar		fore, 
									DrawProc ppDraw, 
									uint		margin, 
									uint		maxWidth, 
									justify	just,
									uint		topLine, 
									uint		bottomLine)
{
	grip				gTextCel;
	uchar*			pCelData;
	uint				resourceNum;

	if (!resNum)
	{
		resNum = &resourceNum;
	}

	if (!maxWidth)
	{
		maxWidth = 300;
	}

	ForeColor(fore);
	BackColor(CI_SKIP);

	Rectangle r;
	TextSize(r, msg, maxWidth, margin, bottomLine - topLine + 1);

	TRACK_MEM("FM::CreateText");
	gTextCel = ACreateCel(resNum, 0, 0,
									r.Wide(),
									r.High(),
									colors[FNT_BACK_COLOR]);

	// draw control under-lay
	ppDraw(gTextCel);

	// draw text on top
	pCelData = AGetResData(gTextCel);
	SetString(margin, margin, msg, pCelData, r.Wide(), margin, just, topLine, bottomLine);

	return gTextCel;
}


// Put the passed string into an existing cel, using the foreground and
//	background color passed.  'back' will default to skip.  'maxWidth' is
//	set to the width of the cel passed.
//
void
FontMgr::CreateText(grip gRes, char* msg, uchar fore, uchar back, uint maxWidth, justify just)
{
	uchar*			pCelData = AGetResData(gRes);

	maxWidth = ((CelHeader*) pCelData)->width;
	ForeColor(fore);
	BackColor(back);

	SetString(DG_MARGIN, DG_MARGIN, msg, pCelData, maxWidth, DG_MARGIN, just);
}


// Calculate the horizontal and vertical offsets of a given character.
void
FontMgr::GetCursorPosn(char* msg, uint cursor, uint maxWidth, uint& hc, uint& vc, uint margin)
{
	if (!cursor)
	{
		vc = point + margin;
		hc = margin;
	}
	else
	{
		TRACK_MEM("FontMgr buf");
		grip			buf = AMalloc(cursor + 1);
		char*			pBuf = ADerefAs(char, buf);
		Rectangle	r(0, 0, 0, 0);

		memset(pBuf, NULL, cursor + 1);
		memcpy(pBuf, msg, cursor);
		TextSize(r, pBuf, maxWidth, margin);
		hc = lastHPen;
		vc = r.High() - 1;
		AFree(buf);
	}
}


uint
FontMgr::GetCharWidth(uchar c)
{
	uchar*			pSrc = AGetResData(curFont) + fhSize;
	FontCharHeader	hFc = ((FontCharHeader*) pSrc)[c];
	return hFc.charWide;
}


uint32
FontMgr::GetCharOffset(uchar c)
{
	uchar*			pSrc = AGetResData(curFont) + fhSize;
	FontCharHeader hFc = ((FontCharHeader*) pSrc)[c];
	return hFc.offset;
}


//----[Private Functions]--------------------------------------------

// Copy a character from font data to a buffer & increment the pen position.
// Returns new pen char width to add to horizontal pen position.
//
uint
FontMgr::SetChar(uint hPen, uint vPen, uchar ch, uchar* pDst, uchar* pSrc, bool useAltColors)
{
	uint32	celOffset = 0;
	uint32	fontOffset = 0;
	uint		mWide = 0;		// width of receiving buffer
	uint32	cWide = 0;		// width of character cel
	uint		cHigh = 0;		// height of character cel (point size)
	uint		row;
//	uint		col;
	uchar		*drawColors;

	if (useAltColors)
	{
		drawColors = altColors;
	}
	else
	{
		drawColors = colors;
	}

	// Get the height
	FontHeader hF = ((FontHeader*) pSrc)[0];
	cHigh = hF.height;
	pSrc += fhSize;

	// Get the widths
	cWide = GetCharWidth(ch);
	fontOffset = GetCharOffset(ch);
	mWide = L2R(((CelHeader*) pDst)->width);

	// Adjust the pointers by header sizes
	pSrc += fchSize * (hF.lastChar - hF.firstChar + 1);
	pDst += chSize;

	// Copy the character
	celOffset = mWide * vPen + L2R(hPen);

/*	#ifdef OS_DOS && _OPTIMIZING_

		uint16	eSReg;

		for (row = 0; row < cHigh; row++)
		{
			// ebx = index into font data
			// ecx = index into cel data
			// edx = column number
			#pragma aux DRAW_CHAR =
				"	mov	eSReg,es"					\
				"	mov	edx,0"						\
				"StartLoop:"							\
				"	mov	ebx,dword ptr pSrc"		\	// Load ebx with addr of font pixel
				"	add	ebx,fontOffset"			\
				"	add	ebx,edx"						\
				"	movzx	eax, byte ptr [ebx]"		\	// Load eax with font pixel
				"	mov	ecx,drawColors"			\
				"	mov	al,byte ptr [ecx + eax]"\	// get the pen to use for this pixel

				"	cmp	al, 0xfe"					\	// See if the pen is skip
				"	jz		NextPixel"					\	// ... and if so, don't copy it.
				"	mov	ecx,dword ptr pDst"		\	// Load ecx with addr of cel pixel
				"	add	ecx,celOffset"				\
				"	add	ecx,edx"						\
				"	mov	[ecx],al"					\	// Move color into cel

				"NextPixel:"							\
				"	inc	edx"							\	// Next column...
				"	cmp	edx,cWide"					\	// Until colum == character width
				"	jne	StartLoop"					\
				"	mov	es,eSReg";

			celOffset += mWide;
			fontOffset += cWide;
		}

	#else		// OS_MAC OS_3DO etc.*/

		for (row = 0; row < cHigh; row++)
		{
			uint	i;
			uchar	fontPen;

			for (i = 0; i < cWide; i++)
			{
				// get the pen from the foreColors array based on
				// the font pixel value
				fontPen = drawColors[*(pSrc + fontOffset + i)];

				if (fontPen != 0xfe)
				{
					// Font pen is not skip, so copy it into the cel
					*(pDst + celOffset + L2R(i)) = fontPen;
					#ifdef DOUBLE_PIX
						*(pDst + celOffset + L2R(i) + 1) = fontPen;
					#endif
				}
			}
			celOffset += mWide;
			fontOffset += cWide;
		}

//	#endif

	return cWide;
}


// Calculate the total width, in pixels, of the message passed from the
// starting character specified to the end of the word (next space).  The
// last character of the word will be returned in 'startChar'.
uint
FontMgr::WordSize(char* msg, uint& startChar)
{
	uint	i;
	uint	wordWidth;

	// Go through each character in the string adding the size of each
	// character until we find a space or reach the end of the string.
	// Carets are ignored, but double-carets are counted as one caret.
	wordWidth = 0;
	for (i = startChar; i < strlen(msg); i++)
	{
		// Formatting character
		if ((cAt(i) == '\\' && cAt(i + 1) == 'n') || cAt(i) == '\n')
		{
			--i;
			break;
		}

		// Non-caret or second caret
		if (!caret(i) || (i && caret(i - 1)))
		{
			if (cAt(i) == ' ')
			{
				//if space is at start of word then pass it as a word itself
				//otherwise don't include end-of-word space in size
				if(i == startChar)
				{
					wordWidth += GetCharWidth(cAt(i));
					break;
				}

				--i;
				break;
			}

			wordWidth += GetCharWidth(cAt(i));

			if (cAt(i) == '-')
				break;
		}
	}
	startChar = i;
	return wordWidth;
}


// This routine is used specifically to force a word to be broken and
// forced onto the next line. Only used in the extreme situation where
// one word is so large as to overrun the entire line.
uint
FontMgr::WordSize(char* msg, uint& startChar, uint maxWidth)
{
	uint	i;
	uint	wordWidth, charWidth;

	// Go through each character in the string adding the size of each
	// character until we find a space or reach the end of the string.
	// Carets are ignored, but double-carets are counted as one caret.
	wordWidth = 0;
	for (i = startChar; i < strlen(msg); i++)
	{
		// Formatting character
		if ((cAt(i) == '\\' && cAt(i + 1) == 'n') || cAt(i) == '\n')
		{
			--i;
			break;
		}

		// Non-caret or second caret
		if (!caret(i) || (i && caret(i - 1)))
		{
			//
			if (cAt(i) == ' ')
			{
				//if space is at start of word then pass it as a word itself
				//otherwise don't include end-of-word space in size
				if(i == startChar)
				{
					wordWidth += GetCharWidth(cAt(i));
					break;
				}

				--i;
				break;
			}

			charWidth = GetCharWidth(cAt(i));

			if( wordWidth + charWidth > maxWidth)
			{
				--i;
				break;
			}

			wordWidth += charWidth;

			if (cAt(i) == '-')
				break;
		}
	}
	startChar = i;
	return wordWidth;
}


// Calculate the total width, in pixels, of all characters that will fit
// on one line.  The last character to fit will be returned in 'startChar'.
uint
FontMgr::LineSize(char* msg, uint maxWidth, uint& startChar)
{
	uint	i;
//	uint	j;
	uint	w = 0; //sum line width
	uint	p = 0;
	uint	wordWidth;
	uint	lrcPos = 0; //last real char of last word that is NOT a whitespace
	uint	lrcWidth = 0;

	// Go through each character in the string until we run out of string
	// or run out of room.
	for (i = startChar; i < strlen(msg); i++)
	{
		//skip this code 'til loop counter catches up to the end of the current word
		if (!p)
		{
			// Find the end of the word and calculate the word size, ignoring
			// carets (unless double carets).
			startChar = i;
			wordWidth = WordSize(msg, startChar);

			// if a single word by itself will overrun the whole line
			// then lets forceably break the word.
			if( wordWidth > maxWidth )
			{
				startChar = i;
				wordWidth = WordSize(msg, startChar, maxWidth);
				lrcPos = startChar;
				lrcWidth = w;
			}

			// Now see if the word will fit on the current line.
			if (wordWidth + w >= maxWidth)
			{
				//lets see about pruning any trailing spaces from the line
				if(lrcPos != startChar)
				{
					//reset
					i = lrcPos;
					w = lrcWidth;
				}
				break;
			}

			// See if we hit a new line
			if ( (cAt(startChar + 1) == '\\') || 
				  (cAt(startChar + 1) == '\n') ||
				  (cAt(startChar + 1) == '\r'))
			{
				//a new line char will not cause trailing spaces to be pruned
				w += wordWidth;
				i = startChar + 2;
				break;
			}

			w += wordWidth;
			p = startChar;	//point to char at end of current word

			//if this new word is not a space
			if(cAt(i) != ' ')
			{
				lrcWidth = w;
				lrcPos = p;
			}

		}


		// We've caught up to the end of the word
		if (i == p)
		{
			//lets look at the next word now
			p = 0;
		}
	}
	startChar = i;
	return w;
}


// Set the hPen value according to justification
void
FontMgr::SetHPen(uint& hPen, char* msg, uint startChar, uint maxWidth, justify just, uint hOffset)
{
	uint s,ls;

	switch (just)
	{
		case DG_JUST_LEFT:
			hPen = 0;
			break;

		case DG_JUST_CENTER:
			s = startChar;
			ls = LineSize(msg, maxWidth, s); //returns 0 if no fit

			// lets get rid of the weird word wrap that happens if
			// lineSize is greater than maxWidth
			if(ls == 0 && strlen(msg) > 0 || ls > maxWidth)
				hPen = 0;	// best we can do
			else
				hPen = (maxWidth - ls) / 2;
			break;

		case DG_JUST_RIGHT:
			s = startChar;
			ls = LineSize(msg, maxWidth, s);
			if(ls == 0 && strlen(msg) > 0 || ls > maxWidth)
				hPen = 0;	// best we can do
			else
				hPen = maxWidth - ls;
			break;
	}
	hPen += hOffset;
}

bool
FontMgrSave(uint16 state, FILE *fp)
{
	bool	error = FALSE;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
			// save font manager info
			if (fwrite(pFontMgr, 1, sizeof(FontMgr), fp) != sizeof(FontMgr))
			{
				// some sort of error
				error = TRUE;
				break;
			}
			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			break;

		case DURING_RESTORE:
			// restore font manager info
			if (fread(pFontMgr, 1, sizeof(FontMgr), fp) != sizeof(FontMgr))
			{
				// some sort of error
				error = TRUE;
				break;
			}
			break;

		case AFTER_RESTORE:
			break;
	}
	return error;
}



