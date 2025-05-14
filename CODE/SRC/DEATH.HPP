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

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"

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
