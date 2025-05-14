// SAVEMENU.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Save game room.
//
// 12/28/94
//

#ifndef savemenu_hpp
#define savemenu_hpp

#include "bam.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "option.hpp"
#include "rect.hpp"
#include "tigre.hpp"
#include "text.hpp"

// this cannot be larger than SAVE_MESSAGE_MAX
#define	MAX_SAVE_TEXT_SIZE	33

class SaveMenu : public BAM_Room
{
	public:

	SaveMenu(void);
	~SaveMenu();

	bool		HandleMsg(Message* pMsg);
 	void		Setup(grip gprevContext_P,option_t menuType_P);
	void		Cleanup();
	void		SetupEmptySlots();
	int		GetSaveNum();

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			gprevContext;
	option_t		menuType;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;

	SquibRes		sqbSave;

	uint32			buttonNum;
	BAM_Button		button[10];
	BAM_Button		cancelButton;
	grip				gsaveText[10];
	BAM_Box			saveBox[10];
	bool				textBoxCreated[10];
	bool				saveGameExist[10];

	char		fileName[14];
	char		saveName[MAX_SAVE_TEXT_SIZE+1];
	uint16	newVerNum,newVerSubNum;

	uint32 buttonFrameX;
	uint32 textFrameX;
	uint32 topMargin;
	uint32 spaceY;

};

#endif // option_hpp

