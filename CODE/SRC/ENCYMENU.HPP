// ENCYMENU.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen for encyclopedia main menu.
//
// 4/20/95
//

#ifndef encymenu_hpp
#define encymenu_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"


enum encyclo_t
{
	CREATURE_BUTTON,
	STRUCTURE_BUTTON,
	ITEM_BUTTON,
	TERRAIN_BUTTON,
	DONE_BUTTON,
	INTERFACE		//accessed by '?' button
};


class EncyMenu : public BAM_RM_Popup
{
	public:

	EncyMenu();
	~EncyMenu();

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Setup(grip gPal,bool fadeup=FALSE);
	virtual void		Cleanup();

	FadeTo		fadeTo;

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			gprevBack;
	int			prevFont;
	BAM_Guy		topBorder;
	BAM_Guy		sideBorder;

	SquibRes		sqbEncyMenu;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	BAM_Guy		textGuy1;
	BAM_Guy		textGuy2;
	grip			gtext1;
	grip			gtext2;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;
	uint32		prevRoomNum;

	uint32			buttonNum;
	BAM_Button		button[8];
	char				buttonName[10][15];
	encyclo_t		bVal[8];
	uint32			capPosY[4];

	int			commentCnt;
};

#endif // option_hpp

