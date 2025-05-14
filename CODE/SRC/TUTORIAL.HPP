// TUTORIAL.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen to show tutorial pages.
//
// 11/27/95
//

#ifndef tutorial_hpp
#define tutorial_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"

#define	TUT_DISPLAY_NUM	22
#define	TUT_STAGE_NUM		21


class Tutorial : public BAM_RM_Popup
{
	public:

	Tutorial(void);
	~Tutorial();

	bool			HandleMsg(Message* pMsg);
 	void			Setup(grip gPal_P,bool fadeup_P,bool advance_P, tutorialGoals goal_P = (tutorialGoals)0);
 	void			Setup();
	void			Cleanup();
	void			Cycle();
	void			ReplaceName(char *destStr, int res, int num);
	void			Clear();
	void			DisplayPic();

	FadeTo		fadeTo;
	bool			fadeup;

	grip			gPal;
	//grip			clut;	//to get to second color range

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	int			prevFont;
	BAM_Guy		topBorder;
	BAM_Guy		sideBorder;

	SquibRes		sqbTutor;
	char			*pTxt;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;

	BAM_Button	buttonDone;

	int			displayAni[TUT_DISPLAY_NUM][2];
	int 			stages[TUT_STAGE_NUM][6];

	int				curSqbNum;
	tutorialGoals	goalNum;		//internal goal counter
	tutorialGoals	goal;			//the goal we will set true
	bool				advance;

	ticks_t 			setupTimer;
	bool				setupDone;

};

#endif

