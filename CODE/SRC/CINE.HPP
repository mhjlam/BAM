//	CINE.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Play all cinematics except the intro.
//
// 1/30/95


#ifndef cine_hpp
#define cine_hpp

#include "SoundMgr.hpp"

#include "BamRoom.hpp"
#include "BamPopup.hpp"
#include "FlicSmk.hpp"

#ifdef OS_MAC
#include <movies.h>
#endif

class Cine : public BAM_Room 
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
		TFlicSmacker flic;
		//TMusic	tMusic;
#endif

		// used for tmp background under error popups
		BAM_Guy	back;
		uint		resNumBack;

		int		nextRoom;
		int		preloadSize;
		int		nextCine;
		bool		cineExists;
		bool		alreadyCreatedPopup;

		Cine(void);

		void	Setup(void);
		bool	HandleMsg(Message* pMsg);
		void	Cycle(void);
};

class CineErrorPopup : public BAM_DefaultPopup
{
	public:

	CineErrorPopup();

 	void		Setup();
	bool		HandleNotice(Message* pMsg);
	bool		HandleEvent(Message *pMsg);
};



#endif


