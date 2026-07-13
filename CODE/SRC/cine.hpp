//	CINE.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Play all cinematics except the intro.
//
// 1/30/95


#ifndef cine_hpp
#define cine_hpp

#include "soundmgr.hpp"

#include "bamroom.hpp"
#include "bampopup.hpp"
#include "flicsmk.hpp"


class Cine : public BAM_Room 
{
	public:
		// DOS
		TFlicSmacker flic;
		//TMusic	tMusic;

		// used for tmp background under error popups
		BAM_Guy	back;
		uint		resNumBack;

		int		nextRoom;
		int		preloadSize;
		int		nextCine;
		bool		cineExists;
		bool		requiredFlic;
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


