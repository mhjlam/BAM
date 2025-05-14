// OPTION.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A popup screen to handle all game options.
//
// 11/16/94
//

#ifndef option_hpp
#define option_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"

enum option_t
{
	SAVE_BUTTON=0,
	LOAD_BUTTON,
	REPLAY_BUTTON,
	NEW_STORY_BUTTON,
	SOUND_BUTTON,
	MUSIC_BUTTON,
	CONTINUE_BUTTON,
	LEAVE_BUTTON,
	RESIGN_BUTTON
};

class OptionMenu : public BAM_RM_Popup
{
	public:

	OptionMenu(void);
	~OptionMenu();

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Setup(grip gPal);
	virtual void		Cleanup();

	FadeTo		fadeTo;

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;

	SquibRes		sqbOption;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	int			oldMouseResNum;
	int			oldMouseCel;
	int			oldMouseHide;
	int			prevRoomNum;

	int			buttonNum;
	BAM_Button 	button[8];
	option_t	  	bVal[8];
	int			capPosY[4];

};

#endif // option_hpp

