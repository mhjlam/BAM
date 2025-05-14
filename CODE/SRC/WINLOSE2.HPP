// WINLOSE2.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A popup screen to handle victory/defeat scenario.
//
// 1/24/95
//

#ifndef victoryDefeat_hpp
#define victoryDefeat_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"


class VictoryDefeat : public BAM_Room
{
	public:

	VictoryDefeat(void);
	~VictoryDefeat();

	bool		HandleMsg(Message* pMsg);
 	void		Setup(grip gPal,bool victory);
	void		Cleanup();
	void		ReplaceName(char *destStr, int res, int num, bool myNameFirst);
	virtual bool	Save(uint16 state, FILE *pFile = NULL);

	FadeTo		fadeTo;
	BAM_Guy		back;
	BAM_Guy		topBorder;
	BAM_Guy		sideBorder;
	SquibRes		sqbVD;
	Rectangle	rback;
	Rectangle	oldMouseLimits;
	BAM_Button	buttonDone;

	char			winlose2DataStart;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			gprevBack;
	int			prevFont;
	char			*pTxt;

	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;
	uint32		prevRoomNum;

	bool			victory;
	char			winlose2DataEnd;
};

#endif // option_hpp

