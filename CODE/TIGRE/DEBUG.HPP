//
// DEBUG.CPP
//		Routines to assist in debugging.
//
// June 25th, 1996	Van
// (c) Copyright 1996, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#ifndef DEBUG_HPP
#define DEBUG_HPP

class Debugger
{
	public:
					Debugger(void);
					~Debugger();
		void		Out(char *fmt, ...), CloseFile(void), CloseWindow(void),
						SetScrollCount(int newScrollCount), Silence(bool fNewMode);
		void		Put(char *pString);
		bool		OpenFile(char *pDebugFileName),
						OpenWindow(int x1, int y1, int x2, int y2);
		FILE		*pDebugFile;
		bool		fSilence, fWindow;
		int		winX1, winY1, winX2, winY2, lastMonoX, lastMonoY, scrollCount;
};

#endif

class DebugTimer
{
	public:
					DebugTimer(void);
					~DebugTimer();

		void		Start(void), Continue(void);
		uint32	Stop(void), SampleTimer(void);

		uint32	startTime, stopTime, duration, prevDuration;
		int		delta;
};
