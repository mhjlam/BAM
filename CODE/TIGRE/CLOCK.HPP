//
// Clock.HPP
//
// November, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Palette class definition.
//
//----[]-------------------------------------------------------------


#ifndef	Clock_hpp
#define	Clock_hpp


#include "tigre.hpp"



//---------[ TClock ]--------------------------------------------
// 
//	TClock gives number of ticks since the creation of the TClock.
//	It can be paused and resumed. It must be cycled as often as the
// time it reports needs to be updated.
//

	/* API description

		// the tick count that TClock reports can only be changed in this call.
		// it should be called each game cycle.
		//
		ticks_t	Cycle();

		//	return the "logical" time of the clock, subtracting paused time.
		//
		ticks_t	GetTicks();

		//	Given a true number of ticks since boot, return the "logical"
		// number of ticks (subtracting the paused time).
		//
		ticks_t	ConvertBootTicks(ticks_t t);

		// Get the actual number of ticks since booting
		//
		ticks_t	GetBootTicks();

		// Set the Clock to a specific time
		//
		void		SetTicks(ticks_t t);

		// Pausing is nested.
		//
		void		Pause();
		void		Resume();
	*/


class TClock : public TMovable
{
	public:

					TClock();

		ticks_t	Cycle();

		ticks_t	GetTicks();
		ticks_t	ConvertBootTicks(ticks_t t);
		ticks_t	GetBootTicks();
		void		SetTicks(ticks_t t);

		void		Pause();
		void		Resume();
		void		ForceResume();


	protected:
		
		int		_fPaused;
		ticks_t	_pauseTicksOffset;
		ticks_t	_logicalTicks;
};


#endif

