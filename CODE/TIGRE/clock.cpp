//
// Clock.CPP
//
// January 22, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include <SDL3/SDL.h>

#include "clock.hpp"


// Convert SDL millisecond timestamp to game ticks at the given divisor.
static inline ticks_t sdl_ticks(int divisor)
{
	return (ticks_t)(SDL_GetTicks() / divisor);
}


//-----------------------------------------------------------------
//-----------------------------------------------------------------
//		TClock
//-----------------------------------------------------------------
//-----------------------------------------------------------------


TClock::TClock()
{
	_fPaused = 0;
	_tick_divisor = 1000 / TICKS_PER_SEC;
	_pauseTicksOffset = 0;
	_logicalTicks = 0;

	SetTicks(0);
}



ticks_t
TClock::GetTicks()
{
	return _logicalTicks;
}


// Count of ticks_t since SDL init.
ticks_t
TClock::GetBootTicks()
{
	return sdl_ticks(_tick_divisor);
}


// Once per cycle, call this to update gTicks
//
ticks_t
TClock::Cycle()
{
	// only update the ticks if we are not paused
	if (_fPaused < 1)
	{
		_logicalTicks = sdl_ticks(_tick_divisor) - _pauseTicksOffset;
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

			_pauseTicksOffset =	sdl_ticks(_tick_divisor) - _logicalTicks;
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

		_pauseTicksOffset =	sdl_ticks(_tick_divisor) - _logicalTicks;
	}
}


// set the ticks (usually only used by restore game)
//
void
TClock::SetTicks(ticks_t newTicks)
{
	_logicalTicks = newTicks;
	_pauseTicksOffset = sdl_ticks(_tick_divisor) - _logicalTicks;
}


// Change game speed. Re-anchors the clock so the tick count is continuous.
//
void
TClock::SetSpeed(GameSpeed spd)
{
	if      (spd == GameSpeed::Fast)    _tick_divisor = 33;
	else if (spd == GameSpeed::Fastest) _tick_divisor = 25;
	else                                _tick_divisor = 1000 / TICKS_PER_SEC;
	SetTicks(_logicalTicks);
}
