// DEATH.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Show death cinematic and options.
//
// 1/26/95
//

#ifndef death_hpp
#define death_hpp

#include "AllDefs.hpp"
#include "BamRoom.hpp"
#include "BamGuy.hpp"
#include "BAM_DG.HPP"
#include "Palette.hpp"
#include "Text.hpp"

class Death : public BAM_Room
{
	public:

	Death(void);
	~Death();

	bool		HandleMsg(Message* pMsg);
 	void		Setup();
	void		Cycle();
	void		Cleanup();

	BAM_Guy		back;
	TPalette		pal;

	#ifdef INTERACTIVE_DEMO
		TMusic		tMusic;
	#else
		TSound		*pDigiMusic;
	#endif
	uint16		musicNum;
	int			deathSqbNum;
	int			launchMusic;

	SquibRes		sqbDeath; //holds generic text for this room - usually 8001.sqb
	SquibRes		sqbButton; //holds text for standard option buttons
	int			sqbRes;
	int			sqbNum;

	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			gPortBord;
	grip			gbanner;

	BAM_Button	replayButton;
	BAM_Button	loadButton;
	BAM_Button	newStoryButton;
	BAM_Button	leaveButton;

};

#endif
