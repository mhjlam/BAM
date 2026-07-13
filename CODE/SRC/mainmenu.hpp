// MAINMENU.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Shows main menu options.
//
// 5/30/95
//

#ifndef mainmenu_hpp
#define mainmenu_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "fade.hpp"
#include "palette.hpp"
#include "text.hpp"


#define MM_MAIN_SQB		7010
#define MM_SINGLE_SQB	7020
#define MM_DOUBLE_SQB	7030

enum mm_menu_t
{
	MM_MENU_MAIN,
	MM_MENU_SINGLE,
	MM_MENU_DOUBLE,
	MM_MENU_NET,
	MM_MENU_LAST
};

enum mm_main_t
{
	MM_MAIN_SINGLE,
	MM_MAIN_DOUBLE,
	MM_MAIN_TUTORIAL,
	MM_MAIN_CREDITS,
	MM_MAIN_SETTINGS,
	MM_MAIN_LEAVE_GAME,
	MM_MAIN_LAST
};

//SINGLE PLAYER SUBMENU
enum mm_single_t
{
	MM_SINGLE_LOAD,
	MM_SINGLE_NEW_STORY,
	MM_SINGLE_LEGEND,
	MM_SINGLE_HALL,
	MM_SINGLE_CANCEL,
	MM_SINGLE_LAST
};

//DOUBLE PLAYER SUBMENU
enum mm_double_t
{
	MM_DOUBLE_NET,
	MM_DOUBLE_HALL,
	MM_DOUBLE_CANCEL,
	MM_DOUBLE_LAST
};


	
class MainMenu : public BAM_Room
{
	public:

	MainMenu(void);
	~MainMenu();

	bool	HandleMsg(Message* pMsg);
 	void	Setup();
 	void	Cycle();
	void	LoadMenu(mm_menu_t menu);
	void	WriteLicenseInfo(CelHeader *pbackCH);

	void	SetupMainMenu();
	void	SetupSingleMenu();
	void	SetupDoubleMenu();
	bool	ShowLegendButton();

	BAM_Guy		back;
	TPalette		pal;

	SquibRes		sqbMain;
	char			*pTxt;
	int			sqbNum;

	FadeTo		fadeTo;

	//main menu
	grip			gbackMain;
	uint			rNumBackMain;
	uchar			*pbackMain;
	CelHeader	*pbackMainCH;
	ticks_t		mainMenuStartTime;

	//single menu
	grip			gbackSingle;
	uint			rNumBackSingle;
	uchar			*pbackSingle;
	CelHeader	*pbackSingleCH;

	//double menu
	grip			gbackDouble;
	uint			rNumBackDouble;
	uchar			*pbackDouble;
	CelHeader	*pbackDoubleCH;


	int			capPosY[4];

	mm_menu_t	currMenu;
	bool			menuDrawn[MM_MENU_LAST];

	BAM_Button	mainButtons[MM_MAIN_LAST];
	BAM_Button	singleButtons[MM_SINGLE_LAST];
	BAM_Button	doubleButtons[MM_DOUBLE_LAST];

	int			playerCount;
	uint16			others[2];
};

// Called by popups opened from MainMenu to reset the idle/showoff timer.
void MainMenu_ResetIdleTimer();

#endif
