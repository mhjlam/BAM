//
// Clock.CPP
//
// January 22, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


//	ANSI includes
#include <time.h>

// Tigre includes
#include "clock.hpp"


// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


// convert ANSI clock_t to ticks_t
#define	CLOCKS_TO_TICKS(c)	((ticks_t) (c / (CLOCKS_PER_SEC/TICKS_PER_SEC)))

//-----------------------------------------------------------------
//-----------------------------------------------------------------
//		TClock 
//-----------------------------------------------------------------
//-----------------------------------------------------------------


TClock::TClock()
{
	_fPaused = 0;
	_pauseTicksOffset = 0;
	_logicalTicks = 0;

	// BUGBUG: the TClock sets the zero time based on the clock's
	//	creation, this is slightly different that the ATicks() mode.
	SetTicks (0);
}



ticks_t
TClock::GetTicks()
{
	return _logicalTicks;
}


// Count of ticks_t since boot.
//	Instead of using the ANSI call clock() which can't be called
// in an interrupt, look at regulary updated variable
//
ticks_t
TClock::GetBootTicks()
{
	return CLOCKS_TO_TICKS(clock());
}


// Once per cycle, call this to update gTicks
//
ticks_t
TClock::Cycle()
{
	// only update the ticks if we are not paused
	if (_fPaused < 1)
	{
		_logicalTicks = CLOCKS_TO_TICKS(clock()) - _pauseTicksOffset;
	}

	return _logicalTicks;
}


// Pause the global tick timer
//
void
TClock::Pause()
{
	_fPaused++;	
}


// Resume the global tick timer
//
void
TClock::Resume()
{
	if (_fPaused)
 	{
		_fPaused--;

		if (_fPaused <1)
		{
			// time to resume things
			_fPaused = 0;

			_pauseTicksOffset =	CLOCKS_TO_TICKS(clock()) - _logicalTicks;
		}
	}
}


// Force resume the global tick timer
//
void
TClock::ForceResume()
{
	if (_fPaused)
 	{
		// time to resume things
		_fPaused = 0;

		_pauseTicksOffset =	CLOCKS_TO_TICKS(clock()) - _logicalTicks;
	}
}


// set the ticks (usually only used by restore game)
//
void
TClock::SetTicks(ticks_t newTicks)
{
	_logicalTicks = newTicks;
	_pauseTicksOffset = CLOCKS_TO_TICKS(clock()) - _logicalTicks;
}
