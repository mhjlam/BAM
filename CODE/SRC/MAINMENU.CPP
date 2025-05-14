// MAINMENU.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Shows main menu options.
//
// 5/30/95
//

#include "mainmenu.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "context.hpp"
#include "eventmgr.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include	"option.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "scrimage.hpp"
#include "savemenu.hpp"
#include "tigre.hpp"
#include "tilelib.hpp"
#include "world.hpp"

#include "comm.hpp"
#include "serial.hpp"
#include "commmgr.hpp"

#include <string.h>

#define	MM_BACKGROUND_ANI	32
#define	MM_MODEM_BACKGROUND_ANI	33
#define	MM_BACKGROUND_PRI	100

#define	MM_TITLE_Y			115
#define	MM_SIDE_TITLE_Y 	200
#define	MM_SIDE_TITLE_X1 	23
#define	MM_SIDE_TITLE_X2 	230
#define	MM_SIDE_TITLE_WIDTH 	67
#define	MM_BUTTON_ANI_A	35
#define	MM_BUTTON_ANI_B	36
#define	MM_BUTTON_WIDTH	100
#define	MM_BUTTON_HEIGHT	22
#define	MM_MODEM_BUTTON_HEIGHT	17
#define	MM_BUTTON_PAD		0
#define	MM_MAX_DIALBOX_TEXT 		28

#define	MM_CAP_ANI			34
// for break between button groups -usually separates the cancel button
#define	MM_MAIN_BREAK		(MM_MAIN_LAST-1)
#define	MM_SINGLE_BREAK 	(MM_SINGLE_LAST-1)
#define	MM_DOUBLE_BREAK 	(MM_DOUBLE_LAST-1)
#define	MM_DIRECT_BREAK 	3
#define	MM_BREAK_HEIGHT 	12

#define	MM_BUTTON_PRI		200
#define	MM_MENU_X	90
#define	MM_MENU_Y	132
#define	MM_MODEM_MENU_X	50
#define	MM_MODEM_MENU_Y	136
#define	MM_BUTTON_START_X	(MM_MENU_X+20)
#define	MM_BUTTON_START_Y	(MM_MENU_Y+30)
#define	MM_MODEM_BUTTON_START_X	(MM_MODEM_MENU_X+99)
#define	MM_MODEM_BUTTON_START_Y	(MM_MODEM_MENU_Y+26)
#define	MM_DIRECT_BUTTON_START_X	(MM_MENU_X+65)
#define	MM_DIRECT_BUTTON_START_Y	(MM_MENU_Y+56)

#define	MM_COMM_PRI				300

#define	DEMO_DELAY_TIME	20

int	CheckForCancel(int notFirstTime);
bool	commInit = FALSE;

static const char clearText[] = "Z";
static const char initText[] = "M1 X4 &K1 &D2 &C1";

//=========================================================

MainMenu::MainMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gbackMain = 0;
	gbackSingle = 0;
	gbackDouble = 0;
	gbackModem = 0;
	gbackDirect = 0;
	gCommBack = 0;
	gbackModemSetup = 0;

	gdialBoxText = 0;
	gclearBoxText = 0;
	ginitBoxText = 0;

	rNumBackMain = 0;
	rNumBackSingle = 0;
	rNumBackDouble = 0;
	rNumBackModem = 0;
	rNumBackDirect = 0;
	rNumBackModemSetup = 0;
	rNumCommBack = 0;

	commErr = MM_CLEAR;
	cancelButtonSet = FALSE;

	memset(menuDrawn,FALSE,sizeof(menuDrawn));

}

MainMenu::~MainMenu()
{
	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);

	if(gdialBoxText)
		AFree(gdialBoxText);

	if(gclearBoxText)
		AFree(gclearBoxText);

	if(ginitBoxText)
		AFree(ginitBoxText);


	//we need to flush all backgrounds except the last one assigned to the guy
	if(gbackMain && rNumBackMain != back.scrim.resNum)
		AFlush(gbackMain);

//	if(gbackSingle && rNumBackSingle != back.scrim.resNum)
// we can now call flush repeatedly without error
	if(gbackSingle)
		AFlush(gbackSingle);

	if(gbackDouble)
		AFlush(gbackDouble);

	if(gbackModem)
		AFlush(gbackModem);

	if(gbackDirect)
		AFlush(gbackDirect);

	if(gbackModemSetup)
		AFlush(gbackModemSetup);

	if(gCommBack)
		AFlush(gCommBack);

}

void
MainMenu::Setup()
{
	BAM_ErrorPopup *pPop;

	pBam->playerSide = SIDE1;
	pBam->playerTypes[SIDE0] = PLAYER_NONE;
	pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
	pBam->playerTypes[SIDE2] = PLAYER_COMPUTER;
	pBam->playerTypes[SIDE3] = PLAYER_NONE;
	pBam->playerTypes[SIDE4] = PLAYER_NONE;

	pBam->voice1.Stop();
	ClearGlobals();

	BAM_Room::Setup();

	switch(bGlobal.roomMgr.newRoomMode)
	{
		case MM_MENU_MAIN:
			currMenu = MM_MENU_MAIN;
			SetupMainMenu();
			break;

		case MM_MENU_SINGLE:
			bGlobal.roomMgr.newRoomMode = 0;
			currMenu = MM_MENU_SINGLE;
			SetupSingleMenu();

			if(bGlobal.roomMgr.prevRoomNum == BR_HALL	&&
				bGlobal.roomMgr.prevRoomMode)
			{
				TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
				pPop->SetWindowOffsets(90,132);
				pPop->Setup(pal.gSelf,MM_MAIN_SQB,bGlobal.roomMgr.prevRoomMode);
			}

			bGlobal.roomMgr.prevRoomMode = 0; //reset
			break;

		case MM_MENU_DOUBLE:
			bGlobal.storyLine = NETGAME;
			bGlobal.roomMgr.newRoomMode = 0;
			currMenu = MM_MENU_DOUBLE;
			SetupDoubleMenu();

			if(bGlobal.roomMgr.prevRoomNum == BR_NET_HALL	&&
				bGlobal.roomMgr.prevRoomMode)
			{
				TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
				pPop->SetWindowOffsets(90,132);
				pPop->Setup(pal.gSelf,MM_MAIN_SQB,bGlobal.roomMgr.prevRoomMode);
			}

			bGlobal.roomMgr.prevRoomMode = 0; //reset
			break;
	}

	pGraphMgr->Animate();
	pal.FadeUp();
}


void
MainMenu::LoadMenu(mm_menu_t newMenu)
{
	BAM_Guy	*pGuy;
	uint		rNum;
	int		i;
	mm_menu_t	oldMenu;

	oldMenu  = currMenu;
	currMenu = newMenu;

	switch(oldMenu)
	{
		case	MM_MENU_MAIN:
			for(i=0;i<MM_MAIN_LAST;i++)
			{
				mainButtons[i].Listen(FALSE);
			}
			break;

		case	MM_MENU_SINGLE:
			for(i=0;i<MM_SINGLE_LAST;i++)
			{
				singleButtons[i].Listen(FALSE);
			}
			break;

		case	MM_MENU_DOUBLE:
			for(i=0;i<MM_DOUBLE_LAST;i++)
			{
				doubleButtons[i].Listen(FALSE);
			}
			break;

		case	MM_MENU_NET:
			break;

		case	MM_MENU_MODEM:
			for(i=0;i<MM_MODEM_LAST;i++)
			{
				modemButtons[i].Listen(FALSE);
			}
			dialBox.Listen(FALSE);
			break;

		case	MM_MENU_DIRECT:
			for(i=0;i<MM_DIRECT_LAST;i++)
			{
				directButtons[i].Listen(FALSE);
			}
			break;

		case	MM_MENU_CONNECT:
			//this menu cleans up immediately -nothing needed here
			break;

		case	MM_MENU_MODEM_SETUP:
			for(i=0;i<MM_MODEM_SETUP_LAST;i++)
			{
				modemSetupButtons[i].Listen(FALSE);
			}
			clearBox.Listen(FALSE);
			initBox.Listen(FALSE);
			break;

	}

	switch(newMenu)
	{
		case	MM_MENU_MAIN:
			bGlobal.storyLine = STORY_NONE;
			if(menuDrawn[newMenu] == FALSE)
				SetupMainMenu();
			else
			{
				for(i=0;i<MM_MAIN_LAST;i++)
				{
					mainButtons[i].Listen(TRUE);
				}
			}

			UpdateTicks();
			mainMenuStartTime = ATicks();
			rNum = rNumBackMain;
			break;

		case	MM_MENU_SINGLE:
			bGlobal.storyLine = STORY_NONE;
			if(menuDrawn[newMenu] == FALSE)
				SetupSingleMenu();
			else
			{
				for(i=0;i<MM_SINGLE_LAST;i++)
				{
					singleButtons[i].Listen(TRUE);
				}
			}
			rNum = rNumBackSingle;
			break;

		case	MM_MENU_DOUBLE:
			bGlobal.storyLine = NETGAME;
			if(menuDrawn[newMenu] == FALSE)
				SetupDoubleMenu();
			else
			{
				for(i=0;i<MM_DOUBLE_LAST;i++)
				{
					doubleButtons[i].Listen(TRUE);
				}
			}
			rNum = rNumBackDouble;
			break;

		case	MM_MENU_NET:
			//if coming from menu DOUBLE (since there is no menu NET)
			//lets go straight to menu CONNECT -and visa versa
			if(oldMenu == MM_MENU_DOUBLE)
			{
				currMenu = MM_MENU_CONNECT;
				SetupConnectScreen(newMenu);
				rNum = 0;
			}
			else
			{
				//backing up to menu DOUBLE from menu CONNECT
			 	currMenu = MM_MENU_DOUBLE;
				for(i=0;i<MM_DOUBLE_LAST;i++)
				{
					doubleButtons[i].Listen(TRUE);
				}
				rNum = rNumBackDouble;
			}
			break;

		case	MM_MENU_MODEM:
			if(menuDrawn[newMenu] == FALSE)
				SetupModemMenu();
			else
			{
				for(i=0;i<MM_MODEM_LAST;i++)
				{
					modemButtons[i].Listen(TRUE);
				}
			}
			dialBox.Listen(TRUE);
			rNum = rNumBackModem;
			break;

		case	MM_MENU_DIRECT:
			if(menuDrawn[newMenu] == FALSE)
				SetupDirectMenu();
			else
			{
				for(i=0;i<MM_DIRECT_LAST;i++)
				{
					directButtons[i].Listen(TRUE);
				}
			}
			rNum = rNumBackDirect;
			break;

		case	MM_MENU_CONNECT:
			SetupConnectScreen(oldMenu);
			rNum = 0;
			break;

		case	MM_MENU_MODEM_SETUP:
			if(menuDrawn[newMenu] == FALSE)
				SetupModemSetupMenu();
			else
			{
				for(i=0;i<MM_MODEM_SETUP_LAST;i++)
				{
					modemSetupButtons[i].Listen(TRUE);
				}
				clearBox.Listen(TRUE);
				initBox.Listen(TRUE);

				LoadModemSettings();
			}
			rNum = rNumBackModemSetup;
			break;

	}


	if(rNum)
	{
		pGuy = &back;
		pGuy->SetRes(RES_CEL, rNum, 1);
		//pGuy->SetPri(MM_BACKGROUND_PRI);

		//pal.Load(MM_BACKGROUND_ANI);
	}
}


void
MainMenu::SetupMainMenu()
{
	int			i,y,ani;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	char			verStr[40];

	//======================================================
	// setup main background cel filled with black
	TRACK_MEM("MainMenu: Main menu background cel");
	gbackMain = ACreateCel(&rNumBackMain,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackMain = AGetResData(gbackMain);
	pbackMainCH = (CelHeader*)pbackMain;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackMainCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBackMain, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_BACKGROUND_PRI);

	pal.Load(MM_BACKGROUND_ANI);

	//======================================================
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMain.Load(MM_MAIN_SQB,10);
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackMainCH, 67, NULL, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackMainCH, 67, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);

	//======================================================
	capPosY[0] = MM_BUTTON_START_Y - 3;
	capPosY[1] = capPosY[0] + 3 + (MM_BUTTON_HEIGHT*MM_MAIN_BREAK);
	capPosY[2] = capPosY[1] + MM_BREAK_HEIGHT - 3;
	capPosY[3] = capPosY[2] + 3 + (MM_BUTTON_HEIGHT * (MM_MAIN_LAST - MM_MAIN_BREAK));

	//Cap setups
	for(i=0; i<4; i++)
	{
		CopyCel(pbackMainCH,MM_BUTTON_START_X,capPosY[i],RES_ANIM,MM_CAP_ANI,(i%2)?2:1,FALSE);
	}

	//======================================================
	for(i=0;i<MM_MAIN_LAST;i++)
	{
		pButton = &mainButtons[i];
		ani = (i%2)?MM_BUTTON_ANI_B:MM_BUTTON_ANI_A;
		y = (i>=MM_MAIN_BREAK)?MM_BREAK_HEIGHT:0;
		y += MM_BUTTON_START_Y + ((MM_BUTTON_HEIGHT+MM_BUTTON_PAD)*i);
		pButton->Create(MM_BUTTON_START_X, y, MM_BUTTON_PRI, RES_ANIM, ani, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;				// click-type button
		pButton->SetOwnerCel(rNumBackMain);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_MAIN_SQB,i+1);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel
	}


	//draw in version number
	SetFontColors(CI_SKIP,64,74,64,74,64,74);

	#ifdef INTERACTIVE_DEMO
	strcpy(verStr,"DEMO ver.");
	#else
	sprintf(verStr,"ver %d.%d",bGlobal.versionNum,bGlobal.versionSubNum);
	#endif

	//strcpy(verStr,"WORK IN PROGRESS");
	//ASetString(0,120, verStr, (uchar *)pbackMainCH, pbackMainCH->width, NULL,DG_JUST_CENTER);
	//strcpy(verStr,"ver BETA");

	ASetString(248,338, verStr, (uchar *)pbackMainCH, 50, NULL,DG_JUST_LEFT);

	WriteLicenseInfo(pbackMainCH);

	UpdateTicks();
	mainMenuStartTime = ATicks();

	menuDrawn[MM_MENU_MAIN] = TRUE;
}


void
MainMenu::SetupSingleMenu()
{
	int			i,y,ani,breakNum;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	bool			fShow;

	//======================================================
	// setup single background cel filled with black
	TRACK_MEM("MainMenu: single menu background cel");
	gbackSingle = ACreateCel(&rNumBackSingle,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackSingle = AGetResData(gbackSingle);
	pbackSingleCH = (CelHeader*)pbackSingle;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackSingleCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBackSingle, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_BACKGROUND_PRI);

	pal.Load(MM_BACKGROUND_ANI);

	//======================================================
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMain.Load(MM_SINGLE_SQB,10);
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackSingleCH, 67, NULL, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackSingleCH, 67, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);

	//======================================================
	fShow = ShowLegendButton();

	//if not to show legend button, then move the caps up one button
	breakNum = (fShow)?MM_SINGLE_BREAK:(MM_SINGLE_BREAK-1);

	//======================================================
	#ifdef INTERACTIVE_DEMO
		capPosY[0] = MM_BUTTON_START_Y - 3;
		capPosY[1] = capPosY[0] + 3 + MM_BUTTON_HEIGHT;
		capPosY[2] = capPosY[1] - 3 + MM_BREAK_HEIGHT;
		capPosY[3] = capPosY[2] + 3 + MM_BUTTON_HEIGHT;
	#else
		capPosY[0] = MM_BUTTON_START_Y - 3;
		capPosY[1] = capPosY[0] + 3 + (MM_BUTTON_HEIGHT*breakNum);
		capPosY[2] = capPosY[1] + MM_BREAK_HEIGHT - 3;
		capPosY[3] = capPosY[2] + 3 + (MM_BUTTON_HEIGHT * (MM_SINGLE_LAST - MM_SINGLE_BREAK));
	#endif

	//Cap setups
	for(i=0; i<4; i++)
	{
		CopyCel(pbackSingleCH,MM_BUTTON_START_X,capPosY[i],RES_ANIM,MM_CAP_ANI,(i%2)?2:1,FALSE);
	}


	//======================================================
	#ifdef INTERACTIVE_DEMO
		pButton = &singleButtons[MM_SINGLE_NEW_STORY];
		ani = MM_BUTTON_ANI_A;
		y = MM_BUTTON_START_Y;

		pButton->Create(MM_BUTTON_START_X, y, MM_BUTTON_PRI, RES_ANIM, ani, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumBackSingle);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_SINGLE_SQB,20); //"new scenario"
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel


		pButton = &singleButtons[MM_SINGLE_CANCEL];
		ani = MM_BUTTON_ANI_B;
		y += MM_BREAK_HEIGHT + (MM_BUTTON_HEIGHT+MM_BUTTON_PAD);

		pButton->Create(MM_BUTTON_START_X, y, MM_BUTTON_PRI, RES_ANIM, ani, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumBackSingle);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_SINGLE_SQB,5);	//"cancel"
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);

	#else
		//======================================================
		for(i=0;i<MM_SINGLE_LAST;i++)
		{
			pButton = &singleButtons[i];

			if(i == MM_SINGLE_LEGEND)
				ani = 37;
			else
				ani = (i%2)?MM_BUTTON_ANI_B:MM_BUTTON_ANI_A;

			y = (i>=breakNum)?MM_BREAK_HEIGHT:0;

			if(!fShow && i > MM_SINGLE_LEGEND)
				y += MM_BUTTON_START_Y + ((MM_BUTTON_HEIGHT+MM_BUTTON_PAD)*(i-1));
			else
				y += MM_BUTTON_START_Y + ((MM_BUTTON_HEIGHT+MM_BUTTON_PAD)*i);

			if(i != MM_SINGLE_LEGEND || fShow)
			{
				pButton->Create(MM_BUTTON_START_X, y, MM_BUTTON_PRI, RES_ANIM, ani, 1, gSelf);
				pButton->SetupReplies(REPLY_DESELECTED);
				pButton->fIsToggle = FALSE;					// click-type button
				pButton->SetOwnerCel(rNumBackSingle);		// draws itself into this DCEL, instead of being drawn by Animate() directly
				pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
				pTxt = sqbMain.Load(MM_SINGLE_SQB,i+1);
				pButton->SetCelText(1, pTxt);
				pButton->SetColors(1, 93, 90);				// inactive colors
				pButton->SetCelText(2, pTxt);
				pButton->SetColors(2, 155, 142);				// active colors
				pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel
			}
		}
	#endif

	WriteLicenseInfo(pbackSingleCH);

	menuDrawn[MM_MENU_SINGLE] = TRUE;
}


void
MainMenu::SetupDoubleMenu()
{
	int			i,y,ani;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;

	//======================================================
	// setup single background cel filled with black
	TRACK_MEM("MainMenu: double menu background cel");
	gbackDouble = ACreateCel(&rNumBackDouble,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackDouble = AGetResData(gbackDouble);
	pbackDoubleCH = (CelHeader*)pbackDouble;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackDoubleCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBackDouble, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_BACKGROUND_PRI);

	pal.Load(MM_BACKGROUND_ANI);

	//======================================================
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMain.Load(MM_DOUBLE_SQB,10);
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackDoubleCH, 67, NULL, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackDoubleCH, 67, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);

	//======================================================
	capPosY[0] = MM_BUTTON_START_Y - 3;
	capPosY[1] = capPosY[0] + 3 + (MM_BUTTON_HEIGHT*MM_DOUBLE_BREAK);
	capPosY[2] = capPosY[1] + MM_BREAK_HEIGHT - 3;
	capPosY[3] = capPosY[2] + 3 + (MM_BUTTON_HEIGHT * (MM_DOUBLE_LAST - MM_DOUBLE_BREAK));

	//Cap setups
	for(i=0; i<4; i++)
	{
		CopyCel(pbackDoubleCH,MM_BUTTON_START_X,capPosY[i],RES_ANIM,MM_CAP_ANI,(i%2)?2:1,FALSE);
	}

	//======================================================
	for(i=0;i<MM_DOUBLE_LAST;i++)
	{
		pButton = &doubleButtons[i];
		ani = (i%2)?MM_BUTTON_ANI_B:MM_BUTTON_ANI_A;
		y = (i>=MM_DOUBLE_BREAK)?MM_BREAK_HEIGHT:0;
		y += MM_BUTTON_START_Y + ((MM_BUTTON_HEIGHT+MM_BUTTON_PAD)*i);
		pButton->Create(MM_BUTTON_START_X, y, MM_BUTTON_PRI, RES_ANIM, ani, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumBackDouble);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_DOUBLE_SQB,i+1);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel
	}

	WriteLicenseInfo(pbackDoubleCH);

	menuDrawn[MM_MENU_DOUBLE] = TRUE;
}


void
MainMenu::SetupModemMenu()
{
	int			i,x=0,y=0;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;

	//======================================================
	// setup single background cel filled with black
	TRACK_MEM("MainMenu: modem menu background cel");
	gbackModem = ACreateCel(&rNumBackModem,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackModem = AGetResData(gbackModem);
	pbackModemCH = (CelHeader*)pbackModem;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackModemCH,0,0,RES_ANIM,MM_MODEM_BACKGROUND_ANI,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBackModem, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_BACKGROUND_PRI);

	pal.Load(MM_BACKGROUND_ANI);

	//first button block frame
	CopyCel(pbackModemCH,MM_MODEM_BUTTON_START_X-2,MM_MODEM_BUTTON_START_Y-3,RES_ANIM,34,3,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	//phone button frame
	CopyCel(pbackModemCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+135,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	//text frame
	CopyCel(pbackModemCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+157,RES_ANIM,34,8,FALSE);
	//second button block frame
	CopyCel(pbackModemCH,MM_MODEM_BUTTON_START_X-2,MM_MODEM_MENU_Y+180,RES_ANIM,34,4,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE

	//======================================================
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMain.Load(MM_MODEM_SQB,4);
	ASetString(0,MM_TITLE_Y, pTxt, (uchar *)pbackModemCH, pbackModemCH->width, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	for(i=0;i<5;i++)
	{
		y = MM_MODEM_BUTTON_START_Y + 4 + ((MM_MODEM_BUTTON_HEIGHT+MM_BUTTON_PAD)*i);
		pTxt = sqbMain.Load(MM_MODEM_SQB,(i+1)*10);
		ASetString(MM_MODEM_MENU_X,y, pTxt, (uchar *)pbackModemCH, 95, NULL,DG_JUST_RIGHT);
	}

	//**********************************
	bool newFile = FALSE;
	FILE *mfn;

	if(bGlobal.writeOut)
	{
		mfn = fOpenPrefs("modemdef.dat", "r+");
		if(!mfn)
		{
			mfn = fOpenPrefs("modemdef.dat", "w+"); //create it
			newFile = TRUE;
			if(!mfn)
				APanic("MainMenu: Error, couldn't open modemdef.dat");
		}
	}
	else
	{
		//lets pretend we started a new file
	 	newFile = TRUE;
	}

	//*****************
	TRACK_MEM("MainMenu: clear box text");	gclearBoxText = AMalloc(MM_MAX_DIALBOX_TEXT+2);
	char *pclearTxt = ADerefAs(char, gclearBoxText);

	TRACK_MEM("MainMenu: init box text");	ginitBoxText = AMalloc(MM_MAX_DIALBOX_TEXT+2);
	char *pinitTxt = ADerefAs(char, ginitBoxText);

	if(newFile)
	{
		strcpy(pclearTxt,clearText);
		strcpy(pinitTxt,initText);

		SaveModemSettings();
	}
	else
	{
		fgets(pclearTxt,MM_MAX_DIALBOX_TEXT+2,mfn);
		pclearTxt[strlen(pclearTxt)-1] = '\0'; //remove \n char

		fgets(pinitTxt,MM_MAX_DIALBOX_TEXT+2,mfn);
		pinitTxt[strlen(pinitTxt)-1] = '\0'; //remove \n char

		fclose(mfn);
	}


	//======================================================
	for(i=0;i<MM_MODEM_LAST;i++)
	{
		pButton = &modemButtons[i];
		switch(i)
		{
			case MM_MODEM_MODE:
			case MM_MODEM_BAUD:
			case MM_MODEM_COM:
			case MM_MODEM_IRQ:
			case MM_MODEM_DIAL:
//			case MM_MODEM_CALL_WAIT:
				x = MM_MODEM_BUTTON_START_X;
				y = MM_MODEM_BUTTON_START_Y + (MM_MODEM_BUTTON_HEIGHT*i);
				break;
			case MM_MODEM_PHONE:
				x = MM_MODEM_MENU_X + 23;
				y = MM_MODEM_MENU_Y + 138;
				break;
			case MM_MODEM_OK:
			case MM_MODEM_SETUP:
			case MM_MODEM_CANCEL:
				x = MM_MODEM_BUTTON_START_X;
				y = MM_MODEM_MENU_Y + 183 + (MM_MODEM_BUTTON_HEIGHT * (i - MM_MODEM_OK));
				break;
			default:
				APanic("MainMenu: bad modem button");
				break;
		}
		pButton->Create(x, y, MM_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumBackModem);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		sqbNum = InitModemButtons(i);
		pTxt = sqbMain.Load(MM_MODEM_SQB,sqbNum);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel
	}

	BAM_Box	*pBox = &dialBox;
	TRACK_MEM("MainMenu: dial box text");	gdialBoxText = AMalloc(MM_MAX_DIALBOX_TEXT+2);
	char *pT = ADerefAs(char, gdialBoxText);
	strcpy(pT,"");
	pBox->SetColors(CI_SKIP,93,90,93,90,93,90,155,142);
	pBox->Create(74, 297, 174, 12, MM_BUTTON_PRI,
					 gdialBoxText, MM_MAX_DIALBOX_TEXT, gSelf, rNumBackModem,0,0);
	pBox->SetupReplies(REPLY_DESELECTED|REPLY_CANCELLED);	// let us know when text changes
	//pBox->Select(TRUE); //makes the box edit active

	WriteLicenseInfo(pbackModemCH);

	menuDrawn[MM_MENU_MODEM] = TRUE;
}


int
MainMenu::InitModemButtons(int i)
{
	//DEFAULTS LISTED BELOW
	switch(i)
	{
		case MM_MODEM_MODE:
			modemMode = MM_MODE_CALL;
			return 11;

		case MM_MODEM_BAUD:
			modemBaud = 9600;
			return 23;

		case MM_MODEM_COM:
			modemCom = 1;
			return 201;

		case MM_MODEM_IRQ:
			modemIrq = 4;
			return 204;

		case MM_MODEM_DIAL:
			modemDial = MM_DIAL_TONE;
			return 51;	//tone

//		case MM_MODEM_CALL_WAIT:
//			modemCallWait = MM_CALL_WAIT_ON;
//			return 61;  //on

		case MM_MODEM_PHONE:
			return 70;

		case MM_MODEM_OK:
			return 80;

		case MM_MODEM_SETUP:
			return 90;

		case MM_MODEM_CANCEL:
			return 100;

		default:
			APanic("MainMenu: bad init on modem buttons");
			break;
	}
	return NULL;
}


void
MainMenu::SetupModemSetupMenu()
{
	int			i,x=0,y=0;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;

	//======================================================
	// setup single background cel filled with black
	TRACK_MEM("MainMenu: modem menu background cel");
	gbackModemSetup = ACreateCel(&rNumBackModemSetup,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackModemSetup = AGetResData(gbackModemSetup);
	pbackModemSetupCH = (CelHeader*)pbackModemSetup;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackModemSetupCH,0,0,RES_ANIM,MM_MODEM_BACKGROUND_ANI,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBackModemSetup, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_BACKGROUND_PRI);

	pal.Load(MM_BACKGROUND_ANI);

	//single button frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+57,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	//text frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+79,RES_ANIM,34,8,FALSE);

	//single button frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+102,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	//text frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+124,RES_ANIM,34,8,FALSE);

	//single button frame -for hangup
	//CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+147,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	//text frame
	//CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+21,MM_MODEM_MENU_Y+168,RES_ANIM,34,8,FALSE);

	// OK button frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+30,MM_MODEM_MENU_Y+204,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	// DEFAULT button frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+81,MM_MODEM_MENU_Y+204,RES_ANIM,34,6,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE
	// CANCEL button frame
	CopyCel(pbackModemSetupCH,MM_MODEM_MENU_X+144,MM_MODEM_MENU_Y+204,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE

	//======================================================
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMain.Load(MM_MODEM_SETUP_SQB,10);
	ASetString(0,MM_TITLE_Y, pTxt, (uchar *)pbackModemSetupCH, pbackModemSetupCH->width, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	//======================================================
	int ani;
	for(i=0;i<MM_MODEM_SETUP_LAST;i++)
	{
		pButton = &modemSetupButtons[i];
		switch(i)
		{
			case MM_MODEM_SETUP_CLEAR:
			case MM_MODEM_SETUP_INIT:
				ani = 8158;
				x = MM_MODEM_MENU_X + 23;
				y = MM_MODEM_MENU_Y + 60 + ((MM_MODEM_BUTTON_HEIGHT+28)*i);
				break;
			case MM_MODEM_SETUP_OK:
				ani = 8158;
				x = MM_MODEM_MENU_X + 32;
				y = MM_MODEM_MENU_Y + 207;
				break;
			case MM_MODEM_SETUP_DEFAULT:
				ani = 117;
				x = MM_MODEM_MENU_X + 83;
				y = MM_MODEM_MENU_Y + 207;
				break;
			case MM_MODEM_SETUP_CANCEL:
				ani = 8158;
				x = MM_MODEM_MENU_X + 146;
				y = MM_MODEM_MENU_Y + 207;
				break;
			default:
				APanic("MainMenu: bad modem setup button");
				break;
		}
		pButton->Create(x, y, MM_BUTTON_PRI, RES_ANIM, ani, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumBackModemSetup);	// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_MODEM_SETUP_SQB,i+1);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel
	}


	BAM_Box	*pBox;
	pBox = &clearBox;
	pBox->SetColors(CI_SKIP,93,90,93,90,93,90,155,142);
	pBox->Create(MM_MODEM_MENU_X + 24, MM_MODEM_MENU_Y + 83, 174, 12, MM_BUTTON_PRI,
					 gclearBoxText, MM_MAX_DIALBOX_TEXT, gSelf, rNumBackModemSetup,0,0);
	pBox->SetupReplies(REPLY_DESELECTED|REPLY_CANCELLED);	// let us know when text changes
	//pBox->Select(TRUE); //makes the box edit active

	//*****************
	pBox = &initBox;
	pBox->SetColors(CI_SKIP,93,90,93,90,93,90,155,142);
	pBox->Create(MM_MODEM_MENU_X + 24, MM_MODEM_MENU_Y + 128, 174, 12, MM_BUTTON_PRI,
					 ginitBoxText, MM_MAX_DIALBOX_TEXT, gSelf, rNumBackModemSetup,0,0);
	pBox->SetupReplies(REPLY_DESELECTED|REPLY_CANCELLED);	// let us know when text changes
	//pBox->Select(TRUE); //makes the box edit active

	WriteLicenseInfo(pbackModemSetupCH);

	menuDrawn[MM_MENU_MODEM_SETUP] = TRUE;
}


void
MainMenu::LoadModemSettings()
{
	bool newFile = FALSE;
	FILE *mfn;

	char *pclearTxt;
	char *pinitTxt;

	if(bGlobal.writeOut)
	{
		mfn = fOpenPrefs("modemdef.dat", "r+");
		if(!mfn)
		{
			mfn = fOpenPrefs("modemdef.dat", "w+"); //create it
			newFile = TRUE;
			if(!mfn)
				APanic("MainMenu: Error, couldn't open modemdef.dat");
		}
	}
	else
	{
		//lets pretend a new file
		newFile = TRUE;
	}

	//============================
	pclearTxt = ADerefAs(char, gclearBoxText);
	pinitTxt = ADerefAs(char, ginitBoxText);

	if(newFile)
	{
		strcpy(pclearTxt,clearText);
		strcpy(pinitTxt,initText);
	}
	else
	{
		fgets(pclearTxt,MM_MAX_DIALBOX_TEXT+2,mfn);
		pclearTxt[strlen(pclearTxt)-1] = '\0'; //remove \n char

		fgets(pinitTxt,MM_MAX_DIALBOX_TEXT+2,mfn);
		pinitTxt[strlen(pinitTxt)-1] = '\0'; //remove \n char

		fclose(mfn);
	}

	//============================
	clearBox.UpdateString();
	initBox.UpdateString();
}


void
MainMenu::SaveModemSettings()
{
	bool newFile = FALSE;
	FILE *mfn = fOpenPrefs("modemdef.dat", "r+");

	if(bGlobal.writeOut)
	{
		mfn = fOpenPrefs("modemdef.dat", "r+");
		if(!mfn)
		{
			mfn = fOpenPrefs("modemdef.dat", "w+"); //create it
			newFile = TRUE;
			if(!mfn)
				APanic("MainMenu: Error, couldn't open modemdef.dat");
		}

		//============================
		char *pclearTxt = ADerefAs(char, gclearBoxText);
		char *pinitTxt = ADerefAs(char, ginitBoxText);

		//============================
		char tmpstr[MM_MAX_DIALBOX_TEXT+2];

		//lets add \n chars for string storage
		strcpy(tmpstr,pclearTxt);
		strcat(tmpstr,"\n");
		fputs(tmpstr,mfn);

		strcpy(tmpstr,pinitTxt);
		strcat(tmpstr,"\n");
		fputs(tmpstr,mfn);

		fclose(mfn);
	}
}


void
MainMenu::ModemSettingsToDefaults()
{
	char *pclearTxt = ADerefAs(char, gclearBoxText);
	char *pinitTxt = ADerefAs(char, ginitBoxText);

	strcpy(pclearTxt,clearText);
	strcpy(pinitTxt,initText);

	clearBox.UpdateString();
	initBox.UpdateString();
}


void
MainMenu::SetupDirectMenu()
{
	int			i,y=0;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;

	//======================================================
	// setup single background cel filled with black
	TRACK_MEM("MainMenu: direct menu background cel");
	gbackDirect = ACreateCel(&rNumBackDirect,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackDirect = AGetResData(gbackDirect);
	pbackDirectCH = (CelHeader*)pbackDirect;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackDirectCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBackDirect, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_BACKGROUND_PRI);

	pal.Load(MM_BACKGROUND_ANI);

	//first button block frame
	CopyCel(pbackDirectCH,MM_DIRECT_BUTTON_START_X-2,MM_DIRECT_BUTTON_START_Y-3,RES_ANIM,34,4,FALSE);
	//second button block frame
	CopyCel(pbackDirectCH,MM_DIRECT_BUTTON_START_X-2,MM_DIRECT_BUTTON_START_Y+60,RES_ANIM,34,5,FALSE);

	//======================================================
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMain.Load(MM_DIRECT_SQB,5);
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackDirectCH, 67, NULL, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackDirectCH, 67, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	for(i=0;i<3;i++)
	{
		y = MM_DIRECT_BUTTON_START_Y + 4 + (MM_MODEM_BUTTON_HEIGHT*i);
		pTxt = sqbMain.Load(MM_DIRECT_SQB,(i+2)*10);
		ASetString(MM_BUTTON_START_X,y, pTxt, (uchar *)pbackDirectCH, 37, NULL,DG_JUST_RIGHT);
	}

	//======================================================
	for(i=0;i<MM_DIRECT_LAST;i++)
	{
		pButton = &directButtons[i];
		y = (i>=MM_DIRECT_BREAK)?MM_BREAK_HEIGHT:0;
		y += MM_DIRECT_BUTTON_START_Y + (MM_MODEM_BUTTON_HEIGHT*i);
		pButton->Create(MM_DIRECT_BUTTON_START_X, y, MM_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumBackDirect);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		sqbNum = InitDirectButtons(i);
		pTxt = sqbMain.Load(MM_DIRECT_SQB,sqbNum);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel
	}

	WriteLicenseInfo(pbackDirectCH);

	menuDrawn[MM_MENU_DIRECT] = TRUE;
}


int
MainMenu::InitDirectButtons(int i)
{
	//DEFAULTS LISTED BELOW
	switch(i)
	{
		case MM_DIRECT_BAUD:
			directBaud = 9600;
			return 23;

		case MM_DIRECT_COM:
			directCom = 1;
			return 201;

		case MM_DIRECT_IRQ:
			directIrq = 4;
			return 204;

		case MM_DIRECT_OK:
			return 80;

		case MM_DIRECT_CANCEL:
			return 100;

		default:
			APanic("MainMenu: bad init on direct buttons");
			break;
	}
	return NULL;
}


void
MainMenu::SetupConnectScreen(mm_menu_t menu)
{
	BAM_Guy		*pGuy;
	grip			gbackAnim;
	CelHeader	*pbackAnimCH;

	prevMenu = menu;

	gbackAnim = ALoad(RES_ANIM,40);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rCommBack.Set(MM_MENU_X,MM_MENU_Y,MM_MENU_X-1+pbackAnimCH->width,MM_MENU_Y-1+pbackAnimCH->height);

	//====================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	fadeTo.Setup(SCREEN_WIDTH,SCREEN_HEIGHT,MM_COMM_PRI - 10,gSelf,pal.gSelf,0,&rCommBack);
	//====================================================

	// setup background cel filled with black
	TRACK_MEM("MainMenu: comm background cel");
	gCommBack = ACreateCel(&rNumCommBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,MM_COMM_PRI);
	pCommBack = AGetResData(gCommBack);
	pCommBackCH = (CelHeader*)pCommBack;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pCommBackCH,0,0,RES_ANIM,40,1,FALSE);

	pGuy = &commBack;
	pGuy->SetRes(RES_CEL,rNumCommBack);
	pGuy->SetPos(MM_MENU_X,MM_MENU_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MM_COMM_PRI);


	//====================================================
	// write 'Initializing...' text
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);
	pTxt = sqbMain.Load(MM_MAIN_SQB,14);
	ASetString(21,60,pTxt, pCommBack, pCommBackCH->width, NULL);
	AAnimate(); 

	//set mouse to hourglass
	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,7);

	commErr = InitComm(menu);

	//Ok, lets reset the mouse anim after the init
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	SetupConnectCancelButton(); //lets set this up before the connect attempt
	if (commErr == MM_INIT_FAIL)
	{
		//'Failed to Init'
		SetFontColors(CI_SKIP,93,90,93,90,93,90);
		pTxt = sqbMain.Load(MM_MAIN_SQB,12);
		ASetString(21,72,pTxt, pCommBack, pCommBackCH->width, NULL);
		AUpdateRect(&rCommBack);

		return;
	}

	SetFontColors(CI_SKIP,93,90,93,90,93,90);
	pTxt = sqbMain.Load(MM_MAIN_SQB,11);
	ASetString(21,72,pTxt, pCommBack, pCommBackCH->width, NULL);
	AUpdateRect(&rCommBack);
	AAnimate(); 

	userAbort = FALSE;
	commCancel = FALSE;	//prep global that can cause breakout from following function
	commErr = ConnectComm(menu);

	//if we failed to connect and not a user abort
	if(commErr == MM_CONNECT_FAIL && !userAbort)
	{
		//if we're here we haven't shut down the connect screen yet
		pTxt = sqbMain.Load(MM_MAIN_SQB,13); //'Fail To Connect'
		ASetString(21,84,pTxt, pCommBack, pCommBackCH->width, NULL);
		AUpdateRect(&rCommBack);
	}
}


void
MainMenu::SetupConnectCancelButton()
{
	BAM_Button	*pButton;

	if(!cancelButtonSet)
	{
		cancelButtonSet = TRUE;

		//button cap
		CopyCel(pCommBackCH,135-MM_MENU_X,294-MM_MENU_Y,RES_ANIM,34,7,FALSE); //YES IS HAS SKIP BUT WE DON'T CARE

		//====================================================
		//setup cancel button
		pButton = &commCancelButton;
		pButton->Create(137,297, MM_COMM_PRI+10, RES_ANIM, 8158, 1, gSelf, MM_MENU_X, MM_MENU_Y);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;					// click-type button
		pButton->SetOwnerCel(rNumCommBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_MODEM_SQB,100); //'Cancel'
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);	  	// set button to unselected state - will cause drawing into master cel

		AUpdateRect(&rCommBack); //so cap will show
	}
}


void
MainMenu::CleanupConnectScreen()
{
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);
	fadeTo.FadeUp();
	if(cancelButtonSet)
		commCancelButton.Listen(FALSE);
	commBack.Cleanup();
	AFlush(gCommBack);
	gCommBack = 0;
	commErr = MM_CLEAR;
	cancelButtonSet = FALSE;
}


mm_comm_err_t
MainMenu::InitComm(mm_menu_t menu)
{
	if(!pCommMgr)
	{
		pMono->Out("New TCommMgr\n");
		TRACK_MEM("TCommMgr");
		pCommMgr = new TCommMgr;
	}

	//before anything else lets set comm abort function
	pCommMgr->SetUserAbortFn(CheckForCancel);

	switch (menu)
	{
		case MM_MENU_NET:
			if(pComm)
				delete(pComm);
			TRACK_MEM("TNetwork");	pComm = new TNetwork;
			break;

		case MM_MENU_MODEM:
			if(pComm)
				delete(pComm);
			TRACK_MEM("TModem");		pComm = new TModem;

			((TModem*) pComm)->SetPort(modemCom);
			((TModem*) pComm)->SetBaud(modemBaud);

			switch(modemDial)
			{
				case MM_DIAL_PULSE:
					((TModem*) pComm)->SetDial(TModem::PULSE);
					break;
				case MM_DIAL_TONE:
				default:
					((TModem*) pComm)->SetDial(TModem::TONE);
					break;
			}
			break;
		case MM_MENU_DIRECT:
			if(pComm)
				delete(pComm);
			TRACK_MEM("TSerial");		pComm = new TSerial;

			((TSerial*) pComm)->SetPort(directCom);
			((TSerial*) pComm)->SetBaud(directBaud);
			break;
	}

	pMono->Out("Calling Init\n");
	if (pCommMgr->Init(pComm) != TComm::ALL_OK)
	{
		return MM_INIT_FAIL;
	}
	return MM_INIT_OK;
}


mm_comm_err_t
MainMenu::ConnectComm(mm_menu_t menu)
{
	if(menu == MM_MENU_MODEM)
	{
		char *pclearTxt = ADerefAs(char, gclearBoxText);
		char *pinitTxt = ADerefAs(char, ginitBoxText);

		((TModem*) pComm)->Write(pclearTxt);
		((TModem*) pComm)->Write(pinitTxt);
		
		if(modemMode == MM_MODE_CALL)
		{
			pMono->Out("Dialing out\n");
			char *pT = ADerefAs(char, gdialBoxText);
			((TModem*) pComm)->Dial(pT);
		}
		else
		{
			pMono->Out("Waiting for call\n");
			((TModem*) pComm)->WaitForCall();
		}

		if( ((TSerial*)pComm)->GetError() != TComm::ALL_OK)
			return MM_CONNECT_FAIL;

	}

	pMono->Out("Connecting...\n");
	if (pCommMgr->Connect() != TComm::ALL_OK)
	{
		return MM_CONNECT_FAIL;
	}

	pMono->Out("Connected!!!!\n");

	int	me;

	// Count and then get list
	playerCount = pCommMgr->GetUserList();
	pCommMgr->GetUserList(others);

	//assume only one other player -so GetUserID will return a 0 or 1
	me = pCommMgr->GetUserID();
	if(me < others[0])
	{
		//we're user 0
		pBam->playerSide = SIDE1;
		pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
		pBam->playerTypes[SIDE2] = me;	//this is the way Van does it ?!
	}
	else
	{
		//we're user 1
		pBam->playerSide = SIDE2;
		pBam->playerTypes[SIDE2] = PLAYER_LOCAL;
		pBam->playerTypes[SIDE1] = me;
	}

	return MM_CONNECTED;
}


bool
MainMenu::HandleMsg(Message* pMsg)
{
	int			i;
	char			mess[100];

	#ifdef INTERACTIVE_DEMO
		BAM_ErrorPopup *pPop;
	#endif

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
				{
					switch(currMenu)
					{
						case MM_MENU_MAIN:
							for(i=0;i<MM_MAIN_LAST;i++)
							{
								if(pMsg->notice.gSource == mainButtons[i].gSelf)
								{
									switch(i)
									{
										case MM_MAIN_SINGLE:
											pMono->Out("main menu; single Button was hit\n");
											LoadMenu(MM_MENU_SINGLE);
											break;
										case MM_MAIN_DOUBLE:
											pMono->Out("main menu; double Button was hit\n");
											#ifdef INTERACTIVE_DEMO
												TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
												pPop->SetWindowOffsets(90,132);
												pPop->Setup(pal.gSelf,MM_MAIN_SQB,200); //"TO PLAY A HEAD-TO-HEAD GAME..."
											#else
												#ifdef NDEBUG
												// BUGBUG! temporarily block access to savegame for release vers, since
												//	it's all under reconstruction - Van
												return(FALSE);
												#else
												LoadMenu(MM_MENU_DOUBLE);
												#endif
											#endif
											break;
										case MM_MAIN_TUTORIAL:
											pMono->Out("main menu; tutorial Button was hit\n");
											bGlobal.storyLine = TUTORIAL;
											strcpy(pBam->scenarioName,"9430");
											bGlobal.roomMgr.NewRoom(BR_WORLD);
											break;
										case MM_MAIN_CREDITS:
											pMono->Out("main menu; credits Button was hit\n");
											bGlobal.roomMgr.NewRoom(BR_CREDITS);
											break;
										case MM_MAIN_REPLAY_INTRO:
											pMono->Out("main menu; replay intro Button was hit\n");
											bGlobal.cinematic = 15;
											bGlobal.roomMgr.NewRoom(BR_CINE);
											break;
										case MM_MAIN_LEAVE_GAME:
											pMono->Out("main menu; leave game Button was hit\n");
											pContextMgr->Quit();
											break;
									}
									return(TRUE);
								}
							}
							break;

						case MM_MENU_SINGLE:
							for(i=0;i<MM_SINGLE_LAST;i++)
							{
								if(pMsg->notice.gSource == singleButtons[i].gSelf)
								{
									switch(i)
									{
										case MM_SINGLE_LOAD:
											pMono->Out("single menu; load Button was hit\n");
											#ifdef OS_MAC
												//on Mac, just set restoreNum, so make loop
												//will call saveMgr.Restore() which will do the rest
												restoreNum = 1;
											#else
												SaveMenu	*pSave;
												TRACK_MEM("SaveMenu");	pSave = new SaveMenu;
												pSave->Setup(NULL,LOAD_BUTTON);
											#endif
											break;
										case MM_SINGLE_NEW_STORY:
											pMono->Out("single menu; new story Button was hit\n");
											bGlobal.roomMgr.NewRoom(BR_STORY);
											break;
										case MM_SINGLE_LEGEND:
											pMono->Out("single menu; legendary campaign Button was hit\n");
											bGlobal.storyLine = LEGEND;	//reset
											bGlobal.cinematic = 3805;	//legend start cine
											bGlobal.roomMgr.NewRoom(BR_CINE);
											break;
										case MM_SINGLE_HALL:
											pMono->Out("single menu; hall of legends Button was hit\n");
											bGlobal.roomMgr.prevRoomMode = MM_MENU_SINGLE;
											bGlobal.roomMgr.newRoomMode  = 0; //clear
											bGlobal.roomMgr.NewRoom(BR_HALL);
											break;
										case MM_SINGLE_CANCEL:
											pMono->Out("single menu; cancel Button was hit\n");
											LoadMenu(MM_MENU_MAIN);
											break;
									}
									return(TRUE);
								}
							}
							break;

						case MM_MENU_DOUBLE:
							for(i=0;i<MM_DOUBLE_LAST;i++)
							{
								if(pMsg->notice.gSource == doubleButtons[i].gSelf)
								{
									switch(i)
									{
										case MM_DOUBLE_NET:
											pMono->Out("double menu; network Button was hit\n");
											LoadMenu(MM_MENU_NET);
											return(TRUE);
											break;
										case MM_DOUBLE_MODEM:
											pMono->Out("double menu; modem Button was hit\n");
											LoadMenu(MM_MENU_MODEM);
											return(TRUE);
											break;
										case MM_DOUBLE_DIRECT:
											pMono->Out("double menu; direct Button was hit\n");
											LoadMenu(MM_MENU_DIRECT);
											return(TRUE);
											break;
										case MM_DOUBLE_HALL:
											pMono->Out("double menu; hall of legends Button was hit\n");
											bGlobal.roomMgr.prevRoomMode = MM_MENU_DOUBLE;
											bGlobal.roomMgr.newRoomMode  = 0;  //clear
											bGlobal.roomMgr.NewRoom(BR_NET_HALL);
											break;
										case MM_DOUBLE_CANCEL:
											pMono->Out("double menu; cancel Button was hit\n");
											LoadMenu(MM_MENU_MAIN);
											break;
									}
									return(TRUE);
								}
							}
							break;
						case MM_MENU_NET:
							break;

						case MM_MENU_MODEM:
							for(i=0;i<MM_MODEM_LAST;i++)
							{
								if(pMsg->notice.gSource == modemButtons[i].gSelf)
								{
									switch(i)
									{
										case MM_MODEM_MODE:
											pMono->Out("modem menu; mode Button was hit\n");
											SetMode();
											break;
										case MM_MODEM_BAUD:
											pMono->Out("modem menu; baud Button was hit\n");
											SetBaud(MM_MENU_MODEM);
											break;
										case MM_MODEM_COM:
											pMono->Out("modem menu; com Button was hit\n");
											SetCom(MM_MENU_MODEM);
											break;
										case MM_MODEM_IRQ:
											pMono->Out("modem menu; irq Button was hit\n");
											SetIrq(MM_MENU_MODEM);
											break;
										case MM_MODEM_DIAL:
											pMono->Out("modem menu; dial Button was hit\n");
											SetDial();
											break;
										case MM_MODEM_PHONE:
											pMono->Out("modem menu; phone Button was hit\n");
											dialBox.Select(TRUE);
											break;
										case MM_MODEM_OK:
											pMono->Out("modem menu; ok Button was hit\n");
											LoadMenu(MM_MENU_CONNECT);
											break;
										case MM_MODEM_SETUP:
											pMono->Out("modem menu; setup Button was hit\n");
									  		LoadMenu(MM_MENU_MODEM_SETUP);
											break;
										case MM_MODEM_CANCEL:
											pMono->Out("modem menu; cancel Button was hit\n");
											LoadMenu(MM_MENU_DOUBLE);
											break;
									}
									return(TRUE);
								}
							}
							if(pMsg->notice.gSource == dialBox.gSelf)
							{
								//got a phone number
								return(TRUE);
							}
							break;

						case MM_MENU_DIRECT:
							for(i=0;i<MM_DIRECT_LAST;i++)
							{
								if(pMsg->notice.gSource == directButtons[i].gSelf)
								{
									switch(i)
									{
										case MM_DIRECT_BAUD:
											pMono->Out("direct menu; baud Button was hit\n");
											SetBaud(MM_MENU_DIRECT);
											break;
										case MM_DIRECT_COM:
											pMono->Out("direct menu; com Button was hit\n");
											SetCom(MM_MENU_DIRECT);
											break;
										case MM_DIRECT_IRQ:
											pMono->Out("direct menu; irq Button was hit\n");
											SetIrq(MM_MENU_DIRECT);
											break;
										case MM_DIRECT_OK:
											pMono->Out("direct menu; ok Button was hit\n");
											LoadMenu(MM_MENU_CONNECT);
											break;
										case MM_DIRECT_CANCEL:
											pMono->Out("direct menu; cancel Button was hit\n");
											LoadMenu(MM_MENU_DOUBLE);
											break;
									}
									return(TRUE);
								}
							}
							break;

						case MM_MENU_MODEM_SETUP:
							for(i=0;i<MM_MODEM_SETUP_LAST;i++)
							{
								if(pMsg->notice.gSource == modemSetupButtons[i].gSelf)
								{
									switch(i)
									{
										case MM_MODEM_SETUP_CLEAR:
											pMono->Out("modem setup menu; clear Button was hit\n");
											clearBox.Select(TRUE);
											break;
										case MM_MODEM_SETUP_INIT:
											pMono->Out("modem setup menu; init Button was hit\n");
											initBox.Select(TRUE);
											break;
										case MM_MODEM_SETUP_OK:
											pMono->Out("modem setup menu; ok Button was hit\n");
											SaveModemSettings();
											LoadMenu(MM_MENU_MODEM);
											break;
										case MM_MODEM_SETUP_DEFAULT:
											pMono->Out("modem setup menu; default Button was hit\n");
											ModemSettingsToDefaults();
											break;
										case MM_MODEM_SETUP_CANCEL:
											pMono->Out("modem setup menu; cancel Button was hit\n");
											LoadMenu(MM_MENU_MODEM);
											break;
									}
									return(TRUE);
								}
							}
							if(pMsg->notice.gSource == clearBox.gSelf)
							{
								return(TRUE);
							}
							if(pMsg->notice.gSource == initBox.gSelf)
							{
								return(TRUE);
							}
							break;

						case MM_MENU_CONNECT:
							if(pMsg->notice.gSource == commCancelButton.gSelf)
							{
								pMono->Out("connect menu; cancel Button was hit\n");
								commCancel = TRUE;
								switch(commErr)
								{
									case MM_INIT_FAIL:
										//we've shown the error msg -lets bail.
										CleanupConnectScreen();
										LoadMenu(prevMenu);
										return(TRUE);
										break;

									case MM_INIT_OK:
										//this is the state before entering ConnectComm()
										//this tells us ConnectComm hasn't returned yet
										//so we know that thru CheckForCancel() we are
										//now processing the user abort click
										userAbort = TRUE;
										CleanupConnectScreen();
										LoadMenu(prevMenu);
										return(TRUE);
										break;

									case MM_CONNECT_FAIL:
										//we've shown the error msg -lets bail.
										CleanupConnectScreen();
										LoadMenu(prevMenu);
										return(TRUE);
										break;

									default:
										APanic("MainMenu: Improper handling of cancel button notice");
										break;
								}
							}
							break;
					}
				}
				else
				if ((uint16)pMsg->notice.param == REPLY_CANCELLED)
				{
					//do nothing -wait for user
					return(TRUE);
				}
				else
         	{
            	sprintf(mess, "Unrecognized button notice reply: %d", pMsg->notice.param);
            	APanic(mess);
         	}
			}
			break;
		case MSG_EVENT:
			switch (pMsg->event.type)
			{
				//case E_MOUSE_DOWN:
				//	break;
				//
				//case E_MOUSE_UP:
				//	// the following is done to make sure that the active button
				//	// (if any) receives the MOUSE_UP event even if mouse has
				//	// wandered out of the rect
				//	if(gCurControl)
				//	{
				//		Object	*pObject;
				//		pObject = ADerefAs(Object, gCurControl);
				//		if(pObject->HandleMsg(pMsg))
				//			return(TRUE);
				//	}
				//	break;

				case E_KEY_DOWN:
					switch (pMsg->event.value)
					{
						case K_ESC:
							switch(currMenu)
							{
								case MM_MENU_MAIN:
									pContextMgr->Quit();
									return(TRUE);
									break;
								case MM_MENU_SINGLE:
									LoadMenu(MM_MENU_MAIN);
									return(TRUE);
									break;
								case MM_MENU_DOUBLE:
									LoadMenu(MM_MENU_MAIN);
									return(TRUE);
									break;
								case MM_MENU_NET:
									//doesn't really exist
									return(TRUE);
									break;
								case MM_MENU_MODEM:
									if(!dialBox.currState)
										LoadMenu(MM_MENU_DOUBLE);
									break;
								case MM_MENU_DIRECT:
									LoadMenu(MM_MENU_DOUBLE);
									return(TRUE);
									break;
								case MM_MENU_CONNECT:
									commCancel = TRUE;
									return(TRUE);
									break;
								case MM_MENU_MODEM_SETUP:
									//if no active text boxes
									if(!clearBox.currState && !initBox.currState)
									{
										LoadMenu(MM_MENU_MODEM);
										return(TRUE);
									}
									break;
							}
							break;
					}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


void
MainMenu::Cycle()
{
//	if(pSoundMgr->pInitErr)
//	{
//		BAM_ErrorPopup *pPop;
//		TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
//		pPop->SetWindowOffsets(90,132);
//		pPop->Setup(pal.gSelf,MM_MAIN_SQB,bGlobal.roomMgr.prevRoomMode);
//	}

	if(currMenu == MM_MENU_MAIN)
	{
		if(ATicks() > mainMenuStartTime + (DEMO_DELAY_TIME * TICKS_PER_SEC))
		{
			bGlobal.storyLine = SHOW_OFF;
			bGlobal.roomMgr.NewRoom(BR_WORLD);
		}
	}

	if(currMenu == MM_MENU_CONNECT)
	{
		if(commInit)
		{
			SetupConnectCancelButton();
		}

		if(commErr == MM_CONNECTED)
		{
			pMono->Out("MainMenu::Cycle() we're connected");
			CleanupConnectScreen();
			bGlobal.storyLine = NETGAME;
			bGlobal.roomMgr.NewRoom(BR_NET_CHAR);
		}
	}
}


void
MainMenu::SetMode(mm_mode_t mode)
{
	BAM_Button	*pButton;

	pButton = &modemButtons[MM_MODEM_MODE];

	//if not zero then explicitly set
	if(mode)
	{
		modemMode = mode;
	}
	else
	{
		//lets advance it
		switch(modemMode)
		{
			case MM_MODE_CALL:
				modemMode = MM_MODE_ANSWER;
				break;
			case MM_MODE_ANSWER:
				modemMode = MM_MODE_CALL;
				break;
		}
	}
		
		
	switch(modemMode)
	{
		case MM_MODE_CALL:
			pTxt = sqbMain.Load(MM_MODEM_SQB,11);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			//modemButtons[MM_MODEM_PHONE].Select(DG_DISABLE);
			//dialBox.Show();
			//dialBox.Listen(TRUE);
			break;
		case MM_MODE_ANSWER:
			pTxt = sqbMain.Load(MM_MODEM_SQB,12);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			//modemButtons[MM_MODEM_PHONE].Listen(TRUE);
			//dialBox.Hide();
			//dialBox.Listen(FALSE);
			break;
		default:
			APanic("MainMenu: Bad modemMode set");
	}
}


void
MainMenu::SetBaud(mm_menu_t menu,int baud)
{
	BAM_Button	*pButton;
	int			commBaud;

	if(menu == MM_MENU_MODEM)
	{
		pButton = &modemButtons[MM_MODEM_BAUD];
		commBaud = modemBaud;
	}
	else
	{
		pButton = &directButtons[MM_DIRECT_BAUD];
		commBaud = directBaud;
	}

	//if not zero then explicitly set
	if(baud)
	{
		commBaud = baud;
	}
	else
	{
		//lets advance it
		switch(commBaud)
		{
			case 2400:
				commBaud = 4800;
				break;
			case 4800:
				commBaud = 9600;
				break;
			case 9600:
				commBaud = 19200;
				break;
			case 19200:
				commBaud = 38400;
				break;
			case 38400:
				commBaud = 2400;
				break;
		}
	}
	
	switch(commBaud)
	{
		case 2400:
			pTxt = sqbMain.Load(MM_MODEM_SQB,21);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 4800:
			pTxt = sqbMain.Load(MM_MODEM_SQB,22);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 9600:
			pTxt = sqbMain.Load(MM_MODEM_SQB,23);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 19200:
			pTxt = sqbMain.Load(MM_MODEM_SQB,24);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 38400:
			pTxt = sqbMain.Load(MM_MODEM_SQB,25);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		default:
			APanic("MainMenu: Bad commBaud set");
	}

	if(menu == MM_MENU_MODEM)
		modemBaud = commBaud;
	else
		directBaud = commBaud;

}


void
MainMenu::SetCom(mm_menu_t menu,int com)
{
	BAM_Button	*pButton;
	int			commCom;

	if(menu == MM_MENU_MODEM)
	{
		pButton = &modemButtons[MM_MODEM_COM];
		commCom = modemCom;
	}
	else
	{
		pButton = &directButtons[MM_DIRECT_COM];
		commCom = directCom;
	}

	//if not zero then explicitly set
	if(com)
		commCom = com;
	else
		if(++commCom > 4) //otherwise advance it
			commCom = 1;
		
	switch(commCom)
	{
		case 1:
			pTxt = sqbMain.Load(MM_MODEM_SQB,201);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			SetIrq(menu,4);
			break;
		case 2:
			pTxt = sqbMain.Load(MM_MODEM_SQB,202);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			SetIrq(menu,3);
			break;
		case 3:
			pTxt = sqbMain.Load(MM_MODEM_SQB,203);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			SetIrq(menu,4);
			break;
		case 4:
			pTxt = sqbMain.Load(MM_MODEM_SQB,204);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			SetIrq(menu,3);
			break;
		default:
			APanic("MainMenu: Bad commCom set");
	}

	if(menu == MM_MENU_MODEM)
		modemCom = commCom;
	else
		directCom = commCom;
}


void
MainMenu::SetIrq(mm_menu_t menu,int irq)
{
	BAM_Button	*pButton;
	int			commIrq;

	if(menu == MM_MENU_MODEM)
	{
		pButton = &modemButtons[MM_MODEM_IRQ];
		commIrq = modemIrq;
	}
	else
	{
		pButton = &directButtons[MM_DIRECT_IRQ];
		commIrq = directIrq;
	}

	//if not zero then explicitly set
	if(irq)
	{
		commIrq = irq;
	}
	else
	{
		//lets advance it
		switch(commIrq)
		{
			case 2:
				commIrq++;
				break;
			case 3:
				commIrq++;
				break;
			case 4:
				commIrq++;
				break;
			case 5:
				commIrq = 7;
				break;
			case 7:
				commIrq = 9;
				break;
			case 9:
				commIrq = 2;
				break;
		}
	}
		
	switch(commIrq)
	{
		case 2:
			pTxt = sqbMain.Load(MM_MODEM_SQB,202);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 3:
			pTxt = sqbMain.Load(MM_MODEM_SQB,203);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 4:
			pTxt = sqbMain.Load(MM_MODEM_SQB,204);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 5:
			pTxt = sqbMain.Load(MM_MODEM_SQB,205);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 7:
			pTxt = sqbMain.Load(MM_MODEM_SQB,207);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case 9:
			pTxt = sqbMain.Load(MM_MODEM_SQB,209);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		default:
			APanic("MainMenu: Bad commIrq set");
	}

	if(menu == MM_MENU_MODEM)
		modemIrq = commIrq;
	else
		directIrq = commIrq;
}


void
MainMenu::SetDial(mm_dial_t dial)
{
	BAM_Button	*pButton;

	pButton = &modemButtons[MM_MODEM_DIAL];

	//if not zero then explicitly set
	if(dial)
	{
		modemDial = dial;
	}
	else
	{
		//lets advance it
		switch(modemDial)
		{
			case MM_DIAL_TONE:
				modemDial = MM_DIAL_PULSE;
				break;
			case MM_DIAL_PULSE:
				modemDial = MM_DIAL_TONE;
				break;
		}
	}
		
		
	switch(modemDial)
	{
		case MM_DIAL_TONE:
			pTxt = sqbMain.Load(MM_MODEM_SQB,51);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		case MM_DIAL_PULSE:
			pTxt = sqbMain.Load(MM_MODEM_SQB,52);
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
		default:
			APanic("MainMenu: Bad modemDial set");
	}
}


bool
MainMenu::ShowLegendButton()
{
	FILE *idFile;
	uint32	i;

	#ifdef INTERACTIVE_DEMO
		return FALSE;
	#endif

	if(bGlobal.writeOut)
	{
		idFile = fOpenPrefs("done.dat","rb+");
		if(!idFile)
		{
			return FALSE;
		}
		else
		{
			fread(&i,1,sizeof(int),idFile);
			fclose(idFile);
			if(i & STORYLINE5_DONE)
				return TRUE;
		}

	}

	return FALSE;
}

void
MainMenu::WriteLicenseInfo(CelHeader *pbackCH)
{
	//lets write the the licensing info
	pFontMgr->SetRes(9060);
	pFontMgr->point++;
	SetFontColors(CI_SKIP,10,7,10,7,26,23);
	pTxt = sqbMain.Load(MM_MAIN_SQB,300);
	ASetString(183,1, pTxt, (uchar *)pbackCH, 138, NULL,DG_JUST_LEFT);
}


int
CheckForCancel(int notFirstTime)
{
	if (notFirstTime)
	{
		//it takes so long to try a connect before this routine is ever
		//called that I now use the following var.
		commInit = TRUE;

		pBam = ADerefAs(BAM_Application, bGlobal.gBam);
		if (bGlobal.gWorld)
		{
			pWorld = ADerefAs(World, bGlobal.gWorld);
		}

		pBam = pBam;

		bGlobal.roomMgr.Cycle();

		pMemMgr->Cycle();
	
		pSoundMgr->Cycle();

		// the heart of the TIGRE animation cycle
		pContextMgr->Cycle();
		pEventMgr->PublishNext();
		pMouse->UpdatePointer();

		pGraphMgr->Animate();

		if(commCancel)
		{
			commCancel = FALSE; //reset
			return ((int)TRUE);
		}

	}
	else
	{
	 	commInit = FALSE;
	}
	return ((int)FALSE);
}



