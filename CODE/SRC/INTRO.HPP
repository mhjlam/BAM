//	INTRO.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
// Play the intro cinematic
//
//


#ifndef intro_hpp
#define intro_hpp

#include "soundmgr.hpp"

#include "bamroom.hpp"
#include "flic.hpp"

#ifdef OS_MAC
#include <movies.h>
#endif

class Intro : public BAM_Room 
{
	public:
#ifdef OS_MAC
		Boolean		playingMovie;
		Movie			myMovie;
		WindowPtr	pMovieWindow;
		int			startlocation;
		Gun			guns[256];
#else
		// DOS
		Flic		flic;
		TMusic	tMusic;
#endif

					Intro(void);
		virtual ~Intro(void);

		// the following 4 functions should be overloaded in each room
		virtual void	Setup(void);
		virtual void	Cleanup(void);
		virtual bool	HandleMsg(Message* pMsg);
		virtual void	Cycle(void);
};


#endif


