// SPENDEXP.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Spend experience popup screen
//
// 11/6/95
//

#ifndef spendexp_hpp
#define spendexp_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "clock.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"


class SpendExperience : public BAM_RM_Popup
{
	public:

	SpendExperience();
	~SpendExperience();

 	void		Setup(grip gPal);
	bool		HandleMsg(Message* pMsg);
	void		Cycle();
	void		Cleanup();
	void		SetupBadClick();
	int		GetCreatureInfo(int creature, int *sqb, int *portrait);


	FadeTo		fadeTo;

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			gprevBack;
	int			prevFont;
	BAM_Guy		topBorder;
	BAM_Guy		sideBorder;

	SquibRes		sqbExp;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;
	uint32		prevRoomNum;

	BAM_Button	cancelB;

	BAM_Guy		slotCursor;
	Rectangle	rSlot;
	Rectangle	rPic;
	int			slotNum;
	unitType		slot[15];
	bool			slotAvail[15];
	int			slotSqb;
	int			slotPortrait;
	int			curSlot;

	//we've got the game clock paused so we need our own clock
	TClock		expClock;
	bool			badClick;
	ticks_t		badClickTime;

};

#endif
