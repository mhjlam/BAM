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
#define MM_MODEM_SQB		7040
#define MM_DIRECT_SQB	7040
#define MM_MODEM_SETUP_SQB	7050

enum mm_menu_t
{
	MM_MENU_MAIN,
	MM_MENU_SINGLE,
	MM_MENU_DOUBLE,
	MM_MENU_NET,
	MM_MENU_MODEM,
	MM_MENU_DIRECT,
	MM_MENU_CONNECT,
	MM_MENU_MODEM_SETUP,
	MM_MENU_LAST
};

enum mm_main_t
{
	MM_MAIN_SINGLE,
	MM_MAIN_DOUBLE,
	MM_MAIN_TUTORIAL,
	MM_MAIN_CREDITS,
	MM_MAIN_REPLAY_INTRO,
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
	MM_DOUBLE_MODEM,
	MM_DOUBLE_DIRECT,
	MM_DOUBLE_HALL,
	MM_DOUBLE_CANCEL,
	MM_DOUBLE_LAST
};

enum mm_connect_t
{
	MM_CONNECT_CANCEL,
	MM_CONNECT_LAST
};

enum mm_modem_t
{
	MM_MODEM_MODE,
	MM_MODEM_BAUD,
	MM_MODEM_COM,
	MM_MODEM_IRQ,
	MM_MODEM_DIAL,
//	MM_MODEM_CALL_WAIT,
	MM_MODEM_PHONE,
	MM_MODEM_OK,
	MM_MODEM_SETUP,
	MM_MODEM_CANCEL,
	MM_MODEM_LAST
};

enum mm_direct_t
{
	MM_DIRECT_BAUD,
	MM_DIRECT_COM,
	MM_DIRECT_IRQ,
	MM_DIRECT_OK,
	MM_DIRECT_CANCEL,
	MM_DIRECT_LAST
};

enum mm_modem_setup_t
{
	MM_MODEM_SETUP_CLEAR,
	MM_MODEM_SETUP_INIT,
	MM_MODEM_SETUP_OK,
	MM_MODEM_SETUP_DEFAULT,
	MM_MODEM_SETUP_CANCEL,
	MM_MODEM_SETUP_LAST
};

enum mm_comm_err_t
{
	MM_CLEAR,
	MM_INIT_OK,
	MM_INIT_FAIL,
	MM_CONNECTED,
	MM_CONNECT_FAIL
};

enum mm_mode_t
{
	MM_MODE_NONE,
	MM_MODE_CALL,
	MM_MODE_ANSWER,
	MM_MODE_LAST
};

enum mm_dial_t
{
	MM_DIAL_NONE,
	MM_DIAL_TONE,
	MM_DIAL_PULSE,
	MM_DIAL_LAST
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
	void	SetupModemMenu();
	int	InitModemButtons(int i);
	bool	ShowLegendButton();

	void	SetupModemSetupMenu();
	void	LoadModemSettings();
	void	SaveModemSettings();
	void	ModemSettingsToDefaults();

	void	SetupDirectMenu();
	int	InitDirectButtons(int i);

	void	SetupConnectScreen(mm_menu_t menu);
	mm_comm_err_t	InitComm(mm_menu_t menu);
	mm_comm_err_t	ConnectComm(mm_menu_t menu);
	void	SetupConnectCancelButton();
	void	CleanupConnectScreen();

	void	SetMode(mm_mode_t mode=MM_MODE_NONE);
	void	SetBaud(mm_menu_t menu,int baud=0);
	void	SetCom(mm_menu_t menu,int com=0);
	void	SetIrq(mm_menu_t menu,int irq=0);
	void	SetDial(mm_dial_t mode=MM_DIAL_NONE);

	mm_comm_err_t  InitComm(mm_menu_t menu);

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

	//modem menu
	grip			gbackModem;
	uint			rNumBackModem;
	uchar			*pbackModem;
	CelHeader	*pbackModemCH;
	BAM_Box		dialBox;
	grip			gdialBoxText;
	mm_mode_t	modemMode;
	int			modemBaud;
	int			modemCom;
	int			modemIrq;
	mm_dial_t	modemDial;

	//direct menu
	grip			gbackDirect;
	uint			rNumBackDirect;
	uchar			*pbackDirect;
	CelHeader	*pbackDirectCH;
	int			directBaud;
	int			directCom;
	int			directIrq;

	//modem setup menu
	grip			gbackModemSetup;
	uint			rNumBackModemSetup;
	uchar			*pbackModemSetup;
	CelHeader	*pbackModemSetupCH;
	BAM_Box		clearBox;
	BAM_Box		initBox;
	grip			gclearBoxText;
	grip			ginitBoxText;

	//connection screen stuff
	BAM_Guy		commBack;
	grip			gCommBack;
	uint			rNumCommBack;
	uchar			*pCommBack;
	CelHeader	*pCommBackCH;
	Rectangle	rCommBack;
	BAM_Button	commCancelButton;
	bool			cancelButtonSet;
	bool			userAbort;
	mm_comm_err_t	commErr;
	mm_menu_t 	prevMenu;
	res_t 		oldMouseResType;
	int 			oldMouseResNum;
	int 			oldMouseCel;

	int			capPosY[4];

	mm_menu_t	currMenu;
	bool			menuDrawn[MM_MENU_LAST];

	BAM_Button	mainButtons[MM_MAIN_LAST];
	BAM_Button	singleButtons[MM_SINGLE_LAST];
	BAM_Button	doubleButtons[MM_DOUBLE_LAST];
	BAM_Button	modemButtons[MM_MODEM_LAST];
	BAM_Button	modemSetupButtons[MM_MODEM_SETUP_LAST];
	BAM_Button	directButtons[MM_DIRECT_LAST];

	int			playerCount;
	WORD			others[2];
};

#endif
