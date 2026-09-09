// OPTION3.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A PopUp window which displays a message and gives a choice to accept.
//
// 11/16/94
//

#ifndef option3_hpp
#define option3_hpp

#include "BamRoom.hpp"
#include "BamDg.hpp"
#include "BamGuy.hpp"
#include "Fade.hpp"
#include "Mouse.hpp"
#include "Option.hpp"
#include "Rect.hpp"
#include "Tigre.hpp"
#include "Text.hpp"


// For Sound or Music
class SubOptionMenu : public BAM_RM_Popup
{
	public:

	SubOptionMenu(void);
	~SubOptionMenu();

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Setup(grip gPrevCon_P,option_t menuType_P);
	//virtual void		Cycle();
	virtual void		Cleanup();

	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	int			oldMouseResNum;
	int			oldMouseCel;
	int			oldMouseHide;

	int			prevFont;

	SquibRes		sqbOption;
	char 			*pTxt;

	grip			gPrevCon;
	option_t		menuType;

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	Rectangle	rback;

	BAM_Button	button[2];
	int			buttonY;

};

#endif

