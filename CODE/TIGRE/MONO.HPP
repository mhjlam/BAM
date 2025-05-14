//
// MONO.HPP
//
// December 15, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Mono class definition.
//
//		Handles output to a mono monitor.
//
//----[]-------------------------------------------------------------


#ifndef	mono_hpp
#define	mono_hpp


#include "types.hpp"

const uchar M_NORMAL 	= 0x07;
const uchar M_INTENSE 	= 0x08;
const uchar M_UNDERLINE = 0x01;
const uchar M_REVERSE 	= 0x70;
const uchar M_BLINK 		= 0x80;

#define	MONO_SPACE		0x20
enum monowinspecs{ MONO_WINX1 = 0, MONO_WINY1, MONO_WINX2, MONO_WINY2,
	MONO_CURSORX, MONO_CURSORY, MONO_SCROLLCOUNT, MONO_MAX_SPECS};

class Mono
{
	public:
		Mono();
		virtual	~Mono();

		uint32	cline;
		uint32	ccol;
		uint16*	buffer;
		uint32	winX1, winY1, winX2, winY2, scrollCount;
		uint32	lines;
		uint32	cols;
		uchar		attr;
		FILE*		monoFile;
		bool		suspended;

		#define MAX_COORDS_SAVED	10
		uint32	coordsSaved[MAX_COORDS_SAVED][MONO_MAX_SPECS],
						totalCoordsSaved;

		void	Out(char* fmt, ...), Put(char *pString);
		virtual	void	Clear(), Clear(int x1, int y1, int x2, int y2);
		virtual	void	Goto(uint16 l, uint16 c), SaveWindow(void), RestoreWindow(void);
		virtual	void	Scroll(uint16 num, int x1 = -1, int y1 = -1, int x2 = -1, int y2 = -1);
		virtual	void	SetAttr(uchar vAttr);
		virtual	void	SetReport(char* filename);
		virtual	void	Suspend();
		virtual	void	Resume();
		virtual	void	SetWindow(int x1, int y1, int x2, int y2),
								SetScrollCount(int newScrollCount);

		virtual	void	MemSet(int line, int col, uchar charValue,
								char attrib, int numChars);
		virtual	void	Snapshot();
};


extern Mono*	pMono;


#endif
