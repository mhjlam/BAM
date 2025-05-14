// WINLOSE.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A popup screen to handle win/lose conditions.
//
// 1/19/95
//

#ifndef winlose_hpp
#define winlose_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"


class WinLose : public BAM_RM_Popup
{
	public:

	WinLose(void);
	~WinLose();

	bool		HandleMsg(Message* pMsg);
 	void		Setup(grip gPal,bool fadeup=FALSE);
	void		Cycle();
	void		Cleanup();

	FadeTo		fadeTo;

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	int			prevFont;
	BAM_Guy		topBorder;
	BAM_Guy		sideBorder;

	SquibRes		sqbWinLose;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;
	uint32		prevRoomNum;

	BAM_Button	buttonDone;
	BAM_Button	buttonUndone;

	int			commentCnt;
	int			lastVoiceNum;
};

#endif // option_hpp

