// CHOOSE.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Choose one of two characters in a storyline.
//
// 12/7/94
//

#ifndef choose_hpp
#define choose_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"

class Choose : public BAM_Room
{
	public:

	Choose(void);
	~Choose();

	bool		HandleMsg(Message* pMsg);
 	void		Setup();
	void		Cycle();

	BAM_Guy		back;
	TPalette		pal;

	grip			gback;
	uint			resNumBack;
	grip			gbackAnim;
	grip			gbanner;

	SquibRes		sqbChoose;
	int			sqbCharRes,sqbCharNumA,sqbCharNumB;
	char			*pTxt;

	BAM_Button	diskButton;
	BAM_Button	portButton1;
	BAM_Button	portButton2;

	uint32		portraitA,portraitB;
	grip			clut[2];

	int			lastVoiceNum;

};

#endif
