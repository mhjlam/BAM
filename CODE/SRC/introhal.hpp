// INTROHAL.HPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
//
// Introduce choosen character into Hall of Legends.
//
// 10/28/95
//

#ifndef introhall_hpp
#define introhall_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"


class IntroHall : public BAM_Room
{
	public:

	IntroHall(void);
	~IntroHall();

	bool	HandleMsg(Message* pMsg);
 	void	Setup();
	void 	Cycle();

	BAM_Guy		back;
	TPalette		pal;

	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			clut[2];

	SquibRes		sqbHall; //holds generic text for this room - 6001.sqb
	int			sqbRes;
	int			sqbNum;

	BAM_Button	diskButton;
	BAM_Button	exitButton;

	int			lastVoiceNum;
};

#endif
