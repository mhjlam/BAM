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


#define MM_MAIN_SQB		17001
#define MM_SINGLE_SQB	17002
#define MM_DOUBLE_SQB	17003
#define MM_MODEM_SQB		17004
#define MM_DIRECT_SQB	17004
#define MM_MODEM_SETUP_SQB		17005

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
	MM_MODEM_SETUP_HANGUP,
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

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Setup();
 	virtual void		Cycle();
	virtual void		LoadMenu(mm_menu_t menu);

	virtual void		SetupMainMenu();
	virtual void		SetupSingleMenu();
	virtual void		SetupDoubleMenu();
	virtual void		SetupModemMenu();
	virtual int			InitModemButtons(int i);

	virtual void		SetupModemSetupMenu();
	virtual void		LoadModemSettings();
	virtual void		SaveModemSettings();
	virtual void		ModemSettingsToDefaults();

	virtual void		SetupDirectMenu();
	virtual int			InitDirectButtons(int i);

	virtual void		SetupConnectScreen(mm_menu_t menu);
	virtual mm_comm_err_t	InitComm(mm_menu_t menu);
	virtual mm_comm_err_t	ConnectComm(mm_menu_t menu);
	virtual void		SetupConnectCancelButton();
	virtual void		CleanupConnectScreen();

	virtual void		SetMode(mm_mode_t mode=MM_MODE_NONE);
	virtual void		SetBaud(mm_menu_t menu,int baud=0);
	virtual void		SetCom(mm_menu_t menu,int com=0);
	virtual void		SetIrq(mm_menu_t menu,int irq=0);
	virtual void		SetDial(mm_dial_t mode=MM_DIAL_NONE);

	virtual mm_comm_err_t  	InitComm(mm_menu_t menu);

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
	BAM_Box		hangupBox;
	grip			gclearBoxText;
	grip			ginitBoxText;
	grip			ghangupBoxText;
	bool			modemSettingsChanged;

	//connection screen stuff
	BAM_Guy		commBack;
	grip			gCommBack;
	uint			rNumCommBack;
	uchar			*pCommBack;
	CelHeader	*pCommBackCH;
	Rectangle	rCommBack;
	BAM_Button	commCancelButton;
	bool			cancelButtonSet;
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
