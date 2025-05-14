//
// MONO.CPP
//
// December 15, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include <io.h>
#include <stdarg.h>

#include "api.hpp"
#include "dpmi.hpp"
#include "mono.hpp"


#pragma pack (1)
#define NDEBUG

Mono*	pMono = NULL;

// Constructor - clear the display
Mono::Mono()
{
	#ifndef NDEBUG
	DPMI	dpmi;

	// delete old snapshot file, if any
	unlink("mono.cap");

	totalCoordsSaved = 0;
	if (!pMono)
	{
		pMono = this;
	}


// Determine if a monochrome monitor exists for output
  	REGS regs;
	memset (&regs, 0, sizeof (regs));
	regs.h.ah = 0x1a;
	int386 (0x10, &regs, &regs);

	if (regs.h.bh)
	{
		buffer = (uint16*) dpmi.RealToProtected(0xB0000000);
	}
	else
	{
		buffer = (uint16*) NULL;
	}

	SetWindow(0, 0, 79, 24);
	SetScrollCount(1);
	attr = M_NORMAL;
	Clear();
	monoFile = NULL;
	suspended = FALSE;
	#endif
}


// Destructor - give termination message
Mono::~Mono()
{
	#ifndef NDEBUG
	Out("\nMono display closed normally\n");
	if (monoFile)
	{
		fclose(monoFile);
	}
	#endif
}

void
Mono::SetScrollCount(int newScrollCount)
{
	scrollCount = newScrollCount;
	scrollCount = AMax(scrollCount, 1);
	scrollCount = AMin(scrollCount, 25);
}

void
Mono::SetWindow(int x1, int y1, int x2, int y2)
{
	winX1 = x1;
	winY1 = y1;
	winX2 = x2;
	winY2 = y2;
	lines = winY2 - winY1 + 1;
	cols = winX2 - winX1 + 1;
}

// Write a simple string out to the mono monitor
void
Mono::Put(char *pString)
{
	#ifndef NDEBUG
	uint16	i;
	char		c;

	if (!suspended)
	{
		if (monoFile)
		{
			fwrite(pString, strlen(pString), 1, monoFile);
		}

		if (!buffer)
		{
			return;
		}

		for (i = 0; i < strlen(pString); i++)
		{
			c = *(pString + i);
			switch (c)
			{
				default:
					memcpy(buffer + cline * 80 + ccol, pString + i, 1);
					++ccol;
					if (ccol <= winX2)
					{
						// no need to line wrap
						break;
					}
					// fall through on purpose

				case '\n':
					ccol = winX1;
					++cline;
					if (cline > winY2)
					{
						Scroll((uint16)scrollCount);
						cline = (winY2 + 1) - scrollCount;
						ccol = winX1;
					}
					break;

				case '\r':
					// clear this same line
					// (this causes major tearing.  let's not do this)
//					MemSet(cline, 0, MONO_SPACE, attr, cols);
					ccol = winX1;
					break;

				case 0x0c:
					cline = winY1;
					ccol = winX1;
					break;

			}
		}
	}
	#endif
}

// Write a complex string out to the mono monitor
void
Mono::Out(char* fmt, ...)
{
  	va_list args;
  	char msg[160];

	va_start (args, fmt);
	#ifndef NDEBUG
	vsprintf (msg, fmt, args);
	#endif
	va_end (args);

	#ifndef NDEBUG
	Put(msg);
	#endif
}


// Clear the mono display
void
Mono::Clear()
{
	#ifndef NDEBUG
	Clear(winX1, winY1, winX2, winY2);

	cline = winY1;
	ccol = winX1;
	#endif
}

// Clear a rectangle of the mono display
void
Mono::Clear(int x1, int y1, int x2, int y2)
{
	#ifndef NDEBUG
	int	loopY;

	if(buffer)
	{
		for(loopY = y1; loopY <= y2; loopY++)
		{
			MemSet(loopY, x1, MONO_SPACE, attr, x2 - x1 + 1);
		}
	}
	#endif
}

void
Mono::SaveWindow(void)
{
	#ifndef NDEBUG
	if(totalCoordsSaved >= MAX_COORDS_SAVED)
	{
		Out("Mono::SaveWindow() - save stack full!\n");
	}
	else
	{
		coordsSaved[totalCoordsSaved][MONO_WINX1] = winX1;
		coordsSaved[totalCoordsSaved][MONO_WINY1] = winY1;
		coordsSaved[totalCoordsSaved][MONO_WINX2] = winX2;
		coordsSaved[totalCoordsSaved][MONO_WINY2] = winY2;
		coordsSaved[totalCoordsSaved][MONO_CURSORX] = ccol;
		coordsSaved[totalCoordsSaved][MONO_CURSORY] = cline;
		coordsSaved[totalCoordsSaved][MONO_SCROLLCOUNT] = scrollCount;
		totalCoordsSaved++;
	}
	#endif
}

void
Mono::RestoreWindow(void)
{
	#ifndef NDEBUG
	if(totalCoordsSaved <= 0)
	{
		Out("Mono::RestoreWindow() - save stack empty!\n");
	}
	else
	{
		totalCoordsSaved--;
		SetWindow(coordsSaved[totalCoordsSaved][MONO_WINX1],
			coordsSaved[totalCoordsSaved][MONO_WINY1], coordsSaved[totalCoordsSaved][MONO_WINX2],
			coordsSaved[totalCoordsSaved][MONO_WINY2]);
		Goto((uint16)(coordsSaved[totalCoordsSaved][MONO_CURSORY] + 1),
			(uint16)(coordsSaved[totalCoordsSaved][MONO_CURSORX] + 1));
		scrollCount = coordsSaved[totalCoordsSaved][MONO_SCROLLCOUNT];
	}
	#endif
}

// Move the cursor to a line and column on the mono display
void
Mono::Goto(uint16 l, uint16 c)
{
	#ifndef NDEBUG
	if (!suspended)
	{
		cline = (uint16) (l - 1);
		ccol = (uint16) (c - 1);
	}
	#endif
}


// Scroll the given area by NUM lines
void
Mono::Scroll(uint16 num, int x1, int y1, int x2, int y2)
{
	#ifndef NDEBUG
	int	loopY;
	uint16	*pSrc, *pDest;

	if(x1 == -1)
	{
		// default to entire screen
		x1 = winX1;
		y1 = winY1;
		x2 = winX2;
		y2 = winY2;
	}
	
	uint16	at = (uint16) ((attr << 8) | 0x20);

	if (!suspended && buffer)
	{
		pDest = buffer + y1 * 80 + x1;
		pSrc = buffer + (y1 + num) * 80 + x1;

		for(loopY = y1; loopY <= (y2 - num); loopY++, pDest += 80,
			pSrc += 80)
		{
			memcpy(pDest, pSrc, (x2 - x1 + 1) * 2);
		}
		for(; loopY <= y2; loopY++)
		{
			MemSet(loopY, x1, MONO_SPACE, attr, (x2 - x1) + 1);
		}
	}
	#endif
}


// Set the video attributes
void
Mono::SetAttr(uchar vAttr)
{
	#ifndef NDEBUG
	attr = vAttr;
	#endif
}


// Set up a disk report of everything that goes to mono
void
Mono::SetReport(char* filename)
{
	#ifndef NDEBUG
	//if (monoFile)
	//{
	//	fclose(monoFile);
	//}
	monoFile = fOpenPrefs(filename, "w");
	#endif
}


// Suspend output to the mono monitor
void
Mono::Suspend()
{
	#ifndef NDEBUG
	suspended = TRUE;
	#endif
}


// Resume output to the mono monitor
void
Mono::Resume()
{
	#ifndef NDEBUG
	suspended = FALSE;
	#endif
}

// since memset only deals with bytes, and we need to set attribute
// and char value, this routine takes care of that.
// Pass:
//   line	- line to start at (0 is the first line)
//   col    - colume to start at (0 is the first column)
//   charValue	- character value
//   attrib		- character attribute
//   numChars  - number of chars to repeat this

void
Mono::MemSet(int line, int col, uchar charValue, char attrib, int numChars)
{
	#ifndef NDEBUG
	uint16	*bufAddr;
	int	i;
	uint16 at = (uint16) ((attrib << 8) | charValue);

	if (buffer)
	{
		bufAddr = buffer + (line * 80) + col;

		i = numChars;
		while(i)
		{
			*bufAddr = at;
			bufAddr++;

			i--;
		}
	}
	#endif
}

void
Mono::Snapshot(void)
{
	FILE	*pFile;
	char	string1[81], *pVidBuffer;
	int	loop1, loop2;

	if(!buffer)
		return;

	pVidBuffer = (char *)buffer;
	pFile = fopen("mono.cap", "w");
	if(!pFile)
		return;
	for(loop1 = 0; loop1 < 25; loop1++)
	{
		for(loop2 = 0; loop2 < 80; loop2++)
			string1[loop2] = *(pVidBuffer + loop1 * 160 + loop2 * 2);
		string1[80] = NULL;
		fprintf(pFile, "%s\n", string1);
	}
	fclose(pFile);
}
