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
#include "settingsmenu.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "scrimage.hpp"
#include "savemenu.hpp"
#include "tigre.hpp"
#include "tilelib.hpp"
#include "world.hpp"

#include <string.h>

#define	MM_BACKGROUND_ANI	32
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
#define	MM_BUTTON_PAD		0

#define	MM_CAP_ANI			34
// for break between button groups -usually separates the cancel button
#define	MM_MAIN_BREAK		(MM_MAIN_LAST-1)
#define	MM_SINGLE_BREAK 	(MM_SINGLE_LAST-1)
#define	MM_DOUBLE_BREAK 	(MM_DOUBLE_LAST-1)
#define	MM_BREAK_HEIGHT 	12

#define	MM_BUTTON_PRI		200
#define	MM_MENU_X	90
#define	MM_MENU_Y	132
#define	MM_BUTTON_START_X	(MM_MENU_X+20)
#define	MM_BUTTON_START_Y	(MM_MENU_Y+30)

#define	DEMO_DELAY_TIME	20

static ticks_t* s_idleTimer = nullptr;

void MainMenu_ResetIdleTimer()
{
	if (s_idleTimer)
		*s_idleTimer = ATicks();
}

//=========================================================

MainMenu::MainMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gbackMain = grip{};
	gbackSingle = grip{};
	gbackDouble = grip{};

	rNumBackMain = 0;
	rNumBackSingle = 0;
	rNumBackDouble = 0;

	memset(menuDrawn,false,sizeof(menuDrawn));

}

MainMenu::~MainMenu()
{
	s_idleTimer = nullptr;

	// we can now call flush repeatedly without error
	if(gbackMain && rNumBackMain != back.scrim.resNum)
		AFlush(gbackMain);

	if(gbackSingle)
		AFlush(gbackSingle);

	if(gbackDouble)
		AFlush(gbackDouble);
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
				pPop = new BAM_ErrorPopup;
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
				pPop = new BAM_ErrorPopup;
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
				mainButtons[i].Listen(false);
			}
			break;

		case	MM_MENU_SINGLE:
			for(i=0;i<MM_SINGLE_LAST;i++)
			{
				singleButtons[i].Listen(false);
			}
			break;

		case	MM_MENU_DOUBLE:
			for(i=0;i<MM_DOUBLE_LAST;i++)
			{
				doubleButtons[i].Listen(false);
			}
			break;

		case	MM_MENU_NET:
			break;

	}

	switch(newMenu)
	{
		case	MM_MENU_MAIN:
			bGlobal.storyLine = STORY_NONE;
			if(menuDrawn[newMenu] == false)
				SetupMainMenu();
			else
			{
				for(i=0;i<MM_MAIN_LAST;i++)
				{
					mainButtons[i].Listen(true);
				}
			}

			UpdateTicks();
			mainMenuStartTime = ATicks();
			rNum = rNumBackMain;
			break;

		case	MM_MENU_SINGLE:
			bGlobal.storyLine = STORY_NONE;
			if(menuDrawn[newMenu] == false)
				SetupSingleMenu();
			else
			{
				for(i=0;i<MM_SINGLE_LAST;i++)
				{
					singleButtons[i].Listen(true);
				}
			}
			rNum = rNumBackSingle;
			break;

		case	MM_MENU_DOUBLE:
			bGlobal.storyLine = NETGAME;
			if(menuDrawn[newMenu] == false)
				SetupDoubleMenu();
			else
			{
				for(i=0;i<MM_DOUBLE_LAST;i++)
				{
					doubleButtons[i].Listen(true);
				}
			}
			rNum = rNumBackDouble;
			break;

		case	MM_MENU_NET:
			bGlobal.roomMgr.NewRoom(BR_NET_CONNECT);
			rNum = 0;
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
	gbackMain = ACreateCel(&rNumBackMain,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackMain = AGetResData(gbackMain);
	pbackMainCH = (CelHeader*)pbackMain;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackMainCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,false);

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
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackMainCH, 67, 0, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackMainCH, 67, 0, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);

	//======================================================
	capPosY[0] = MM_BUTTON_START_Y - 3;
	capPosY[1] = capPosY[0] + 3 + (MM_BUTTON_HEIGHT*MM_MAIN_BREAK);
	capPosY[2] = capPosY[1] + MM_BREAK_HEIGHT - 3;
	capPosY[3] = capPosY[2] + 3 + (MM_BUTTON_HEIGHT * (MM_MAIN_LAST - MM_MAIN_BREAK));

	//Cap setups
	for(i=0; i<4; i++)
	{
		CopyCel(pbackMainCH,MM_BUTTON_START_X,capPosY[i],RES_ANIM,MM_CAP_ANI,(i%2)?2:1,false);
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
		pButton->fIsToggle = false;				// click-type button
		pButton->SetOwnerCel(rNumBackMain);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_MAIN_SQB,i+1);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(false);	  	// set button to unselected state - will cause drawing into master cel
	}

	// Override the label for the settings button (replaces "Replay Intro" SQB text)
	mainButtons[MM_MAIN_SETTINGS].SetCelText(1, "SETTINGS");
	mainButtons[MM_MAIN_SETTINGS].SetCelText(2, "SETTINGS");
	mainButtons[MM_MAIN_SETTINGS].Select(false);

	//draw in version number
	SetFontColors(CI_SKIP,64,74,64,74,64,74);

	snprintf(verStr, sizeof(verStr),"ver %d.%d",bGlobal.versionNum,bGlobal.versionSubNum);

	//strcpy(verStr,"WORK IN PROGRESS");
	//ASetString(0,120, verStr, (uchar *)pbackMainCH, pbackMainCH->width, 0,DG_JUST_CENTER);
	//strcpy(verStr,"ver BETA");

	ASetString(248,338, verStr, (uchar *)pbackMainCH, 50, 0,DG_JUST_LEFT);

	WriteLicenseInfo(pbackMainCH);

	UpdateTicks();
	mainMenuStartTime = ATicks();
	s_idleTimer = &mainMenuStartTime;

	menuDrawn[MM_MENU_MAIN] = true;
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
	gbackSingle = ACreateCel(&rNumBackSingle,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackSingle = AGetResData(gbackSingle);
	pbackSingleCH = (CelHeader*)pbackSingle;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackSingleCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,false);

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
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackSingleCH, 67, 0, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackSingleCH, 67, 0, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);

	//======================================================
	fShow = ShowLegendButton();

	//if not to show legend button, then move the caps up one button
	breakNum = (fShow)?MM_SINGLE_BREAK:(MM_SINGLE_BREAK-1);

	//======================================================
		capPosY[0] = MM_BUTTON_START_Y - 3;
		capPosY[1] = capPosY[0] + 3 + (MM_BUTTON_HEIGHT*breakNum);
		capPosY[2] = capPosY[1] + MM_BREAK_HEIGHT - 3;
		capPosY[3] = capPosY[2] + 3 + (MM_BUTTON_HEIGHT * (MM_SINGLE_LAST - MM_SINGLE_BREAK));

	//Cap setups
	for(i=0; i<4; i++)
	{
		CopyCel(pbackSingleCH,MM_BUTTON_START_X,capPosY[i],RES_ANIM,MM_CAP_ANI,(i%2)?2:1,false);
	}


	//======================================================
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
				pButton->fIsToggle = false;					// click-type button
				pButton->SetOwnerCel(rNumBackSingle);		// draws itself into this DCEL, instead of being drawn by Animate() directly
				pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
				pTxt = sqbMain.Load(MM_SINGLE_SQB,i+1);
				pButton->SetCelText(1, pTxt);
				pButton->SetColors(1, 93, 90);				// inactive colors
				pButton->SetCelText(2, pTxt);
				pButton->SetColors(2, 155, 142);				// active colors
				pButton->Select(false);	  	// set button to unselected state - will cause drawing into master cel
			}
		}

	WriteLicenseInfo(pbackSingleCH);

	menuDrawn[MM_MENU_SINGLE] = true;
}


void
MainMenu::SetupDoubleMenu()
{
	int			i,y,ani;
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;

	//======================================================
	// setup single background cel filled with black
	gbackDouble = ACreateCel(&rNumBackDouble,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,MM_BACKGROUND_PRI);
	pbackDouble = AGetResData(gbackDouble);
	pbackDoubleCH = (CelHeader*)pbackDouble;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackDoubleCH,0,0,RES_ANIM,MM_BACKGROUND_ANI,1,false);

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
	ASetString(MM_SIDE_TITLE_X1,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackDoubleCH, 67, 0, DG_JUST_CENTER);
	ASetString(MM_SIDE_TITLE_X2,MM_SIDE_TITLE_Y, pTxt, (uchar *)pbackDoubleCH, 67, 0, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);

	//======================================================
	capPosY[0] = MM_BUTTON_START_Y - 3;
	capPosY[1] = capPosY[0] + 3 + (MM_BUTTON_HEIGHT*MM_DOUBLE_BREAK);
	capPosY[2] = capPosY[1] + MM_BREAK_HEIGHT - 3;
	capPosY[3] = capPosY[2] + 3 + (MM_BUTTON_HEIGHT * (MM_DOUBLE_LAST - MM_DOUBLE_BREAK));

	//Cap setups
	for(i=0; i<4; i++)
	{
		CopyCel(pbackDoubleCH,MM_BUTTON_START_X,capPosY[i],RES_ANIM,MM_CAP_ANI,(i%2)?2:1,false);
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
		pButton->fIsToggle = false;					// click-type button
		pButton->SetOwnerCel(rNumBackDouble);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbMain.Load(MM_DOUBLE_SQB,i+1);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(false);	  	// set button to unselected state - will cause drawing into master cel
	}

	// Override labels: SQB strings are from the DOS 5-button layout (Network/Modem/Direct/Hall/Cancel).
	// We only have 3 buttons now, so fix up the mismatched labels.
	doubleButtons[MM_DOUBLE_HALL].SetCelText(1, "Hall of Legends");
	doubleButtons[MM_DOUBLE_HALL].SetCelText(2, "Hall of Legends");
	doubleButtons[MM_DOUBLE_HALL].Select(false);
	doubleButtons[MM_DOUBLE_CANCEL].SetCelText(1, "Cancel");
	doubleButtons[MM_DOUBLE_CANCEL].SetCelText(2, "Cancel");
	doubleButtons[MM_DOUBLE_CANCEL].Select(false);

	WriteLicenseInfo(pbackDoubleCH);

	menuDrawn[MM_MENU_DOUBLE] = true;
}


bool
MainMenu::HandleMsg(Message* pMsg)
{
	int			i;
	char			mess[100];


	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
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
											LoadMenu(MM_MENU_DOUBLE);
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
										case MM_MAIN_SETTINGS:
											pMono->Out("main menu; settings Button was hit\n");
											{ SettingsMenu *pSettings = new SettingsMenu; pSettings->Setup(); }
											break;
										case MM_MAIN_LEAVE_GAME:
											pMono->Out("main menu; leave game Button was hit\n");
											pContextMgr->Quit();
											break;
									}
									return(true);
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
												SaveMenu	*pSave;
												pSave = new SaveMenu;
												pSave->Setup(grip{},LOAD_BUTTON);
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
									return(true);
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
											return(true);
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
									return(true);
								}
							}
							break;
						case MM_MENU_NET:
							break;

					}
				}
				else
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_CANCELLED)
				{
					//do nothing -wait for user
					return(true);
				}
				else
         	{
            	snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
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
				//			return(true);
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
									return(true);
									break;
								case MM_MENU_SINGLE:
									LoadMenu(MM_MENU_MAIN);
									return(true);
									break;
								case MM_MENU_DOUBLE:
									LoadMenu(MM_MENU_MAIN);
									return(true);
									break;
								case MM_MENU_NET:
									//doesn't really exist
									return(true);
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

}


bool
MainMenu::ShowLegendButton()
{
	return (GetStoryDone() & STORYLINE5_DONE) != 0;
}

void
MainMenu::WriteLicenseInfo(CelHeader *pbackCH)
{
	//lets write the the licensing info
	pFontMgr->SetRes(9060);
	pFontMgr->point++;
	SetFontColors(CI_SKIP,10,7,10,7,26,23);
	pTxt = sqbMain.Load(MM_MAIN_SQB,300);
	ASetString(183,1, pTxt, (uchar *)pbackCH, 138, 0,DG_JUST_LEFT);
}


