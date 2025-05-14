//
// DEBUG.CPP
//		Routines to assist in debugging.
//
// June 25th, 1996	Van
// (c) Copyright 1996, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include <stdarg.h>
#include <stdio.h>
#include "api.hpp"
#include "mono.hpp"
#include "debug.hpp"

#define NDEBUG
extern bool				fPentium;	// set by application

Debugger::Debugger(void)
{
	pDebugFile = NULL;
	fSilence = FALSE;
	fWindow = FALSE;
	SetScrollCount(1);
}

Debugger::~Debugger()
{
	if(pDebugFile)
		CloseFile();
}

void
Debugger::Silence(bool fNewMode)
{
	fSilence = fNewMode;
}

void
Debugger::Put(char *pString)
{
	#ifndef NDEBUG
	if(!fSilence && !pMono->suspended)
	{
		if(pDebugFile)
			fputs(pString, pDebugFile);

		if(fWindow)
		{
			pMono->SaveWindow();
			pMono->SetWindow(winX1, winY1, winX2, winY2);
			pMono->Goto((uint16)(lastMonoY + 1), (uint16)(lastMonoX + 1));
		}
		pMono->SetScrollCount(scrollCount);
		pMono->Put(pString);
		if(fWindow)
		{
			lastMonoX = pMono->ccol;
			lastMonoY = pMono->cline;
			pMono->RestoreWindow();
		}
	}
	#endif
}

void
Debugger::Out(char *fmt, ...)
{
  	va_list args;
  	char msg[128];

	// I think we still need to pull these from the stack no matter what,
	// to clean up after ourselves
	va_start (args, fmt);
	#ifndef NDEBUG
	vsprintf (msg, fmt, args);
	#endif
	va_end (args);

	#ifndef NDEBUG
	Put(msg);
	#endif
}

bool
Debugger::OpenFile(char *pDebugFileName)
{
	if(pDebugFile)
	{
		CloseFile();
	}

	#ifndef NDEBUG
	pDebugFile = fopen(pDebugFileName, "w");
	return(pDebugFile? TRUE: FALSE);
	#else
	return(FALSE);
	#endif
}

void
Debugger::CloseFile(void)
{
	if(pDebugFile)
	{
		fclose(pDebugFile);
		pDebugFile = NULL;
	}
}

void
Debugger::SetScrollCount(int newScrollCount)
{
	scrollCount = newScrollCount;
}

bool
Debugger::OpenWindow(int x1, int y1, int x2, int y2)
{
	int	int1;

	fWindow = TRUE;

	// clip and normalize coords
	x1 = AMax(0, x1);	x1 = AMin(79, x1);
	x2 = AMax(0, x2);	x2 = AMin(79, x2);
	if(x2 < x1)
	{
		int1 = x1;
		x1 = x2;
		x2 = int1;
	}

	y1 = AMax(0, y1);	y1 = AMin(24, y1);
	y2 = AMax(0, y2);	y2 = AMin(24, y2);
	if(y2 < y1)
	{
		int1 = y1;
		y1 = y2;
		y2 = int1;
	}

	winX1 = x1;	winY1 = y1;	winX2 = x2;	winY2 = y2;

	lastMonoX = winX1;
	lastMonoY = winY1;
	return(TRUE);
}

void
Debugger::CloseWindow(void)
{
	fWindow = FALSE;
}

DebugTimer::DebugTimer(void)
{
	startTime = 0;
	stopTime = 0;
	duration = 0;
	prevDuration = 0;
	delta = 0;
}

DebugTimer::~DebugTimer()
{
}

void
DebugTimer::Start(void)
{
	startTime = SampleTimer() / 1000;
	stopTime = 0;
	prevDuration = duration;
	duration = 0;
}

void
DebugTimer::Continue(void)
{
	startTime = SampleTimer() / 1000;
	stopTime = 0;
}
	
uint32
DebugTimer::Stop(void)
{
	stopTime = SampleTimer() / 1000;
	duration += AMax(stopTime - startTime, 0);
	if(prevDuration)
		delta = ((duration - prevDuration) / prevDuration) * 100;
	else delta = 0;
	return(duration);
}

uint32
DebugTimer::SampleTimer(void)
{
	uint32	timerVal;

	if(!fPentium)
		return(0);

	#pragma aux _READPENTTIMER =												\
		"pusha				"														\
      "DB  0fh, 031h		"														\
      "mov timerVal, eax"													\
		"popa";
	_READPENTTIMER();

	return(timerVal);
}
