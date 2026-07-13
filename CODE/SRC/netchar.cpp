// NETCHAR.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A room in which to create a character for NET game.
//
// 7/31/95
//


#include "netchar.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "mainmenu.hpp"
#include "savemenu.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "comm.hpp"
#include "eventmgr.hpp"
#include	"mono.hpp"
#include	"mouse.hpp"
#include	"graphmgr.hpp"
#include "rect.hpp"
#include	"savemgr.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"
#include "game_config.hpp"

#include <string.h>
#include <ctype.h>

#define FIRSTCOVERANIM		2850	
#define FIRSTMENFACEANIM	2900	
#define FIRSTWOMENFACEANIM	2940	
#define FIRSTOTHERFACEANIM	2970	
#define FIRSTBODYANIM		3000	

#define BASE_PRI	100

enum banner_color
{
	BAN_RED,
	BAN_BLUE,
	BAN_PURPLE,
	BAN_GREEN,
	BAN_GOLD,
	BAN_LAST
};
extern char pErrorStrings[][30];

#define	MAIN_MENU_PRI	100
#define	MAIN_BUTTON_PRI	110

#define	MAKE_MENU_PRI	200
#define	MAKE_BUTTON_PRI	210

#define	LOAD_MENU_PRI	200
#define	LOAD_BUTTON_PRI	210

#define	WAIT_MENU_PRI		300
#define	DISCANCEL_PRI		310

//=========================================================

NetChar::NetChar(void)
{
	int	i;

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;

	gback = grip{};
	gNameText = grip{};
	gCampArr = grip{};

	//------------------
	//make char vars

	maxMenNameNum = 0;
	maxWomenNameNum = 0;
	maxOtherNameNum = 0;

	//since old vars. aren't set to cur. vals. an immed. update packet
	//should be sent out
	oldCoverAnim = 0;
	oldFaceAnim = 0;
	oldBodyAnim = 0;

	gender_t tmpcoverGen[MAX_COVER] = {
			G_ALL,G_MEN | G_OTHER,G_ALL,G_ALL,G_MEN,G_WOMEN,
			G_WOMEN,G_ALL,G_ALL,G_MEN | G_WOMEN,G_MEN | G_OTHER};

	gender_t tmpbodyGen[MAX_BODY] = {
			G_ALL,G_ALL,G_MEN,G_WOMEN | G_OTHER,G_ALL,
			G_ALL,G_ALL,G_ALL,G_WOMEN,G_ALL};

	memcpy(coverGen,tmpcoverGen,sizeof(gender_t)*MAX_COVER);
	memcpy(bodyGen,tmpbodyGen,sizeof(gender_t)*MAX_BODY);


	//----------------
	//load char vars
	memset(&header,0,sizeof(header));

	//---------------
	//general vars
	memset(menuDrawn,false,sizeof(menuDrawn));

	for(i = 0; i < NUM_CLUTS; i++)
	{
		clut[i] = AMalloc(CLUT_SIZE);
	}

	clearWaitMsg = false;

	currFrame = 0;
	sendErrorCnt = 0;
	roomState = ST_INIT;
	sendExpireTicks = 0;
	attemptRestore = false;

	enemyDone = false;
	meDone = false;
	bothDone = false;
	curNameNum = -1;

	errorPopupCreated = false;

//	if(pCommMgr->GetUserID())
//		ASeedRandom2(41);
//	else
//		ASeedRandom2(43);
}

NetChar::~NetChar(void)
{
	int	i;

	if(gNameText)
		AFree(gNameText);

	if(gCampArr)
		AFree(gCampArr);

	for(i = 0; i < NUM_CLUTS; i++)
	{
		AFree(clut[i]);
		clut[i] = grip{};
	}

	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);
}

void
NetChar::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	int			i,j;

	curMenu = NC_MAIN;

	//selectively clear globals -lets assume the player just finished a net game
	ClearGlobalVars();

	//======================================================
	// setup background cel filled with black
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,MAIN_MENU_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackCH,0,0,RES_ANIM,8150,1,false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MAIN_MENU_PRI);

	//pal.FadeToBlack();
	pal.Load(8150);

	//======================================================
	// lets copy in all the borders and boundaries into the background

	CopyCel(pbackCH,151,51,RES_ANIM,8154,2,true);	// Screen Divider
	CopyCel(pbackCH, 37,67,RES_ANIM,8154,3,true);	// Name Frame
	CopyCel(pbackCH,181,67,RES_ANIM,8154,3,true);	// Name Frame
	CopyCel(pbackCH, 66,92,RES_ANIM,8154,1,false);	// Portrait Frame
	CopyCel(pbackCH,210,92,RES_ANIM,8154,1,false);	// Portrait Frame
	CopyCel(pbackCH,36,261,RES_ANIM,60,3,true);	// triple button half Frame
	CopyCel(pbackCH,36,328,RES_ANIM,60,4,true);	// start game frame
	CopyCel(pbackCH,180,328,RES_ANIM,60,4,true);	// disconnect frame

	//=============================================
	// Write the necessary text into the background

	pFontMgr->SetRes(9052);
	//SetFontColors(CI_SKIP,1,2,3,4,5,CI_BLACK);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);	//'CARVED GREEN'
	pTxt = sqbNetChar.Load(NETCHAR_SQB,8);		//'YOU'
	ASetString(79,43, pTxt, (uchar *)pbackCH, 101, 0);

	pTxt = sqbNetChar.Load(NETCHAR_SQB,9);		//'OPPONENT'
	ASetString(208,43, pTxt, (uchar *)pbackCH, 101, 0);

	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);	//'BRIGHT BLUE'
	pTxt = sqbNetChar.Load(NETCHAR_SQB,10);	//'WAITING FOR OPPONENT'
	ASetString(175,229, pTxt, (uchar *)pbackCH, 120, 0);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);


	//======================================================
	//setup cluts for pen guys
	//we know player's color (SIDE1) has been load into first clut range

	uchar 		*pClut;


	for(j=0;j<NUM_CLUTS;j++)
	{
		pClut = ADerefAs(uchar, clut[j]);

		for(i=0;i<CLUT_SIZE;i++)
		{
			pClut[i] = (uchar)(CLUT1_START + (j*CLUT_SIZE) + i);
		}
	}

	//load all clut color ranges
	pal.LoadPartial(9101,CLUT1_START,NUM_CLUTS * CLUT_SIZE,CLUT1_START);
	pal.UpdateColors(CLUT1_START,CLUT1_START + (NUM_CLUTS * CLUT_SIZE));

	//======================================================
	if(SetupCampaign())
	{
		GetLoadCharStats();

		SetFontColors(CI_SKIP,93,90,93,90,93,90);
		ASetString(40, 71, bGlobal.curName, (uchar *)pbackCH, pbackCH->width, 0);

		//need to rebuild local vars from globals instead of random settings
		switch(bGlobal.curCat)
		{
			case G_MEN:
				curFace = (bGlobal.curFaceAnim - FIRSTMENFACEANIM) / 2;
				break;
			case G_WOMEN:
				curFace = (bGlobal.curFaceAnim - FIRSTWOMENFACEANIM) / 2;
				break;
			case G_OTHER:
				curFace = (bGlobal.curFaceAnim - FIRSTOTHERFACEANIM) / 2;
				break;
		}

		curBody = (bGlobal.curBodyAnim - FIRSTBODYANIM) / 2;
		curCover = (bGlobal.curCoverAnim - FIRSTCOVERANIM) / 2;
	}
	else
	{
		//no last char -random gen. 
		// pick a gender catagory
		switch (MyRandom(3))
		{
			case 0:
				bGlobal.curCat = G_MEN;
				break;
			case 1:
				bGlobal.curCat = G_WOMEN;
				break;
			case 2:
				bGlobal.curCat = G_OTHER;
				break;
		}

		RandomCover();
		RandomFace();
		RandomBody();
		RandomBanner();
		sqbRes = RandomName();

		pTxt = sqbNetChar.Load(sqbRes,curNameNum);
		strcpy(bGlobal.curName,pTxt);
		ASetString(40, 71, pTxt, (uchar *)pbackCH, pbackCH->width, 0);

		bGlobal.curCharId = GetNewCharId();
	}

	UpdateMainDisplay();
		
	//======================================================
	pButton = &makeCharB;
	pButton->Create(38, 264, MAIN_BUTTON_PRI, RES_ANIM, 62, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(NETCHAR_SQB,1);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(false);

	pButton = &loadCharB;
	pButton->Create(38, 281, MAIN_BUTTON_PRI, RES_ANIM, 62, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(NETCHAR_SQB,2);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(false);

	pButton = &loadGameB;
	pButton->Create(38, 298, MAIN_BUTTON_PRI, RES_ANIM, 62, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(NETCHAR_SQB,3);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(false);

	pButton = &startGameB;
	pButton->Create(38, 331, MAIN_BUTTON_PRI, RES_ANIM, 62, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(NETCHAR_SQB,4);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(false);
	pButton->Listen(false);	//off until we init the room

	pButton = &disCancelB;
	pButton->Create(182, 331, DISCANCEL_PRI, RES_ANIM, 62, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(NETCHAR_SQB,5); //'disconnect'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(false);

	menuDrawn[NC_MAIN] = true;

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

	pMono->Out("NetChar: Finished setup()\n");
}


void
NetChar::UpdateMainDisplay()
{
	//update name
	CopyCel(pbackCH, 37,67,RES_ANIM,8154,3,true);	// Name Frame
	ASetString(40, 71, bGlobal.curName, (uchar *)pbackCH, pbackCH->width, 0);

	CopyCel(pbackCH,68, 95,RES_ANIM,bGlobal.curBodyAnim,1,true);	// Body
	CopyCel(pbackCH,68, 95,RES_ANIM,bGlobal.curFaceAnim,1,true);	// Face
	CopyCel(pbackCH,68, 95,RES_ANIM,bGlobal.curCoverAnim,1,true);	// Cover
	CopyCel(pbackCH,68,171,RES_ANIM,9004,2,true,clut[bGlobal.curBanner/2]);	// banner 1

	SetEnemyBannerColor();
}


void
NetChar::LoadCharPacket()
{
	ncPacket.charId = bGlobal.curCharId;
	ncPacket.bodyAnim = bGlobal.curBodyAnim;
	ncPacket.faceAnim = bGlobal.curFaceAnim;
	ncPacket.coverAnim = bGlobal.curCoverAnim;
	//ncPacket.banner = bGlobal.curBanner;

	strcpy(ncPacket.name, bGlobal.curName);

	GameConfig cfg = LoadGameConfig();
	ncPacket.game_speed   = static_cast<int32>(cfg.game_speed);
	ncPacket.fast_collect = cfg.fast_collect ? 1 : 0;
}


void
NetChar::RefreshEnemyChar()
{
	Rectangle	rect;
	int	i;
	uchar *ptr;


	CopyCel(pbackCH,212,95,RES_ANIM,bGlobal.enemyBodyAnim,1,true);	// Body
	CopyCel(pbackCH,212,95,RES_ANIM,bGlobal.enemyFaceAnim,1,true);	// Face
	CopyCel(pbackCH,212,95,RES_ANIM,bGlobal.enemyCoverAnim,1,true);	// Cover
	CopyCel(pbackCH,212,171,RES_ANIM,9004,3,true,clut[bGlobal.enemyBanner/2]);	// banner 

	//update enemy name
	ptr = pback + sizeof(CelHeader) + (70*pbackCH->width + 183);
	for(i=0;i<11;i++)
	{
		memset(ptr,CI_BLACK,(97));
		ptr += (pbackCH->width);
	}

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	ASetString(184, 71, bGlobal.enemyName, (uchar *)pbackCH, pbackCH->width, 0);

	rect.Set(183,70,280,184);
	AUpdateRect(&rect);
}


void
NetChar::LoadMenu(netchar_t newMenu)
{
	BAM_Button	*pButton;
	int			i;
	netchar_t	oldMenu;

	oldMenu  = curMenu;
	curMenu = newMenu;

	switch(oldMenu)
	{
		//NOTE: we always leave startGameB and disCancelB on
		case	NC_MAIN:
			makeCharB.Listen(false);
			loadCharB.Listen(false);
			loadGameB.Listen(false);
			break;

		case	NC_MAKE:
			for(i=0;i<4;i++)
			{
				leftArrowB[i].Listen(false);
				rightArrowB[i].Listen(false);
			}
			randomCharB.Listen(false);
			manB.Listen(false);
			womanB.Listen(false);
			otherB.Listen(false);
			customNameB.Listen(false);
			randomNameB.Listen(false);

			backMake.Hide();
			curCoverGuy.Hide();
			curFaceGuy.Hide();
			curBodyGuy.Hide();
			curBannerGuy.Hide();

			break;

		case	NC_LOAD:
			leftLoadB.Listen(false);
			rightLoadB.Listen(false);

			backLoad.Hide();

			//we're going back to main -restore globals from backups
			//bGlobal.curCharId = backupCharId;
			//bGlobal.curCoverAnim = backupCoverAnim;
			//bGlobal.curFaceAnim = backupFaceAnim;
			//bGlobal.curBodyAnim = backupBodyAnim;
			//bGlobal.curBanner = backupBanner;
			//bGlobal.curCat = backupCat;
			//strcpy(bGlobal.curName,backupName[40]);

			break;

		case	NC_WAIT:
			//we should never get here -leaving wait menu will cause disconnect
			break;

	}

	switch(newMenu)
	{
		case	NC_MAIN:
			//we know we have already set this menu up
			makeCharB.Listen(true);
			loadCharB.Listen(true);
			loadGameB.Listen(true);

			//lets sync this screen with globals that may have changed in other screens
			UpdateMainDisplay();

			//make sure disCancel button is set to disconnect mode
			pTxt = sqbNetChar.Load(NETCHAR_SQB,5); //'disconnect'
			pButton = &disCancelB;
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();

			break;

		case	NC_MAKE:
			if(menuDrawn[newMenu] == false)
				SetupMakeCharMenu();
			else
			{
				for(i=0;i<4;i++)
				{
					leftArrowB[i].Listen(true);
					rightArrowB[i].Listen(true);
				}
				randomCharB.Listen(true);
				manB.Listen(true);
				womanB.Listen(true);
				otherB.Listen(true);
				customNameB.Listen(true);
				randomNameB.Listen(true);

				backMake.Show();
				curCoverGuy.Show();
				curFaceGuy.Show();
				curBodyGuy.Show();
				curBannerGuy.Show();
			}

			//make sure disCancel button is set to cancel mode
			pTxt = sqbNetChar.Load(NETCHAR_SQB,6); //'cancel'
			pButton = &disCancelB;
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;

		case	NC_LOAD:
			if(menuDrawn[newMenu] == false)
				SetupLoadCharMenu();
			else
			{
				ConfigLoadArrowButtons();

				backLoad.Show();
			}

			//make sure disCancel button is set to cancel mode
			pTxt = sqbNetChar.Load(NETCHAR_SQB,6); //'cancel'
			pButton = &disCancelB;
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;

		case	NC_WAIT:
			//normally here we would want turn buttons on not off
			//but we want startgame button always on until now in wait mode
			startGameB.Listen(false);

			SetupWaitMenu();

			//make sure disCancel button is set to disconnect mode
			pTxt = sqbNetChar.Load(NETCHAR_SQB,5); //'disconnect'
			pButton = &disCancelB;
			pButton->SetCelText(1, pTxt);
			pButton->SetCelText(2, pTxt);
			pButton->Draw();
			break;
	}
}


void
NetChar::SetupMakeCharMenu()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	int			x;
	Rectangle	backRect;

	//======================================================
	// setup background cel filled with black
	gbackMake = ACreateCel(&rNumBackMake,0,0,153,318,CI_BLACK,MAKE_MENU_PRI);
	pbackMake = AGetResData(gbackMake);
	pbackMakeCH = (CelHeader*)pbackMake;

	//lets only copy the upper left corner of the anim into the cel
	backRect.Set(0,0,152,317);
	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackMakeCH,0,0,RES_ANIM,8150,1,&backRect,false);

	pGuy = &backMake;
	pGuy->SetRes(RES_CEL,rNumBackMake);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MAKE_MENU_PRI);


	//======================================================
	// lets copy in all the borders and boundaries into the background

	CopyCel(pbackMakeCH,37, 67,RES_ANIM,8154,3,true);	// Name Frame
	CopyCel(pbackMakeCH,43, 92,RES_ANIM,8154,1,true);	// Portrait Frame
	CopyCel(pbackMakeCH,92, 92,RES_ANIM,8154,4,true);	// Arrow Buttons Frame
	CopyCel(pbackMakeCH,21,205,RES_ANIM,  60,1,true);	// Gender Buttons Frame
	CopyCel(pbackMakeCH,21,245,RES_ANIM,  60,2,true);	// Name Buttons Frame

	//=============================================
	// Write the necessary text into the background

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74,64,74,64,74);

	pTxt = sqbNetChar.Load(NETCHAR_SQB,11);	//'GENDER'
	ASetString(26,193, pTxt, (uchar *)pbackMakeCH, pbackMakeCH->width, 0);
	pTxt = sqbNetChar.Load(NETCHAR_SQB,12);	//'NAME'
	ASetString(26,233, pTxt, (uchar *)pbackMakeCH, pbackMakeCH->width, 0);

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);	//'CARVED GREEN'
	pTxt = sqbNetChar.Load(NETCHAR_SQB,8);		//'YOU'
	ASetString(79,43, pTxt, (uchar *)pbackMakeCH, 101, 0);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	gNameText = AMalloc(20);
	char *pText = ADerefAs(char, gNameText);
	strcpy(pText,bGlobal.curName);
	BAM_Box	*pBox = &nameBox;
	pBox->SetColors(CI_SKIP,93,90,93,90,93,90,155,142);
	pBox->Create(40, 71, 105, 15, MAKE_BUTTON_PRI, gNameText, 16, gSelf, rNumBackMake,0,0);
	pBox->SetupReplies(REPLY_DESELECTED);	// let us know when text changes
	//pBox->Select(true); //makes the box edit active

	//======================================================
	LoadCover();
	LoadFace();
	LoadBody();
	LoadBanner();

	//======================================================
	// lets setup up all the buttons

	uint32 posY[4] =  { 95, 112, 129, 146 };

	for(x=0; x<4; x++)
	{
		// create left arrow button
		pButton = &leftArrowB[x];
		pButton->Create(94, posY[x], MAKE_BUTTON_PRI, RES_ANIM, 8156, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = false;		// click-type button
		pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

		// create right arrow button
		pButton = &rightArrowB[x];
		pButton->Create(115, posY[x], MAKE_BUTTON_PRI, RES_ANIM, 8157, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = false;		// click-type button
		pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->Select(false);				// set button to unselected state - will cause drawing into master cel
	}


	// disk button
//	pButton = &diskB;
//	pButton->Create(23, 285, MAKE_BUTTON_PRI, RES_ANIM, 129, 1, gSelf);
//	pButton->SetupReplies(REPLY_DESELECTED);
//	pButton->fIsToggle = false;		// click-type button
//	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
//	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &randomCharB;
	pButton->Create(94, 163, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(MAKECHAR_SQB,9);	//'RANDOM'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &manB;
	pButton->Create(23,208, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
	pButton->fIsToggle = true;			// click-type button
	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(MAKECHAR_SQB,10);	//'MAN'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	if(bGlobal.curCat == G_MEN)
		pButton->Select(true);				// set button to unselected state - will cause drawing into master cel
	else
		pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &womanB;
	pButton->Create(65,208, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
	pButton->fIsToggle = true;			// click-type button
	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(MAKECHAR_SQB,11);	//'WOMAN;
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	if(bGlobal.curCat == G_WOMEN)
		pButton->Select(true);				// set button to unselected state - will cause drawing into master cel
	else
		pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &otherB;
	pButton->Create(107,208, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
	pButton->fIsToggle = true;			// click-type button
	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(MAKECHAR_SQB,12);	//'OTHER'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	if(bGlobal.curCat == G_OTHER)
		pButton->Select(true);				// set button to unselected state - will cause drawing into master cel
	else
		pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &customNameB;
	pButton->Create(23, 248, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(MAKECHAR_SQB,13);	//'CUSTOM'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &randomNameB;
	pButton->Create(65,248, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbNetChar.Load(MAKECHAR_SQB,9);	//'RANDOM'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

//	pButton = &doneB;
//	pButton->Create(110, 256, MAKE_BUTTON_PRI, RES_ANIM, 8158, 1, gSelf);
//	pButton->SetupReplies(REPLY_DESELECTED);
//	pButton->fIsToggle = false;		// click-type button
//	pButton->SetOwnerCel(rNumBackMake);		// draws itself into this DCEL, instead of being drawn by Animate() directly
//	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
//	pTxt = sqbNetChar.Load(MAKECHAR_SQB,14);	//'DONE'
//	pButton->SetCelText(1, pTxt);
//	pButton->SetColors(1, 93, 90);				// inactive colors
//	pButton->SetCelText(2, pTxt);
//	pButton->SetColors(2, 155, 142);				// active colors
//	pButton->Draw();
//	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	menuDrawn[NC_MAKE] = true;
}


void
NetChar::SetupLoadCharMenu()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	Rectangle	backRect;

	//======================================================
	// setup background cel filled with black
	gbackLoad = ACreateCel(&rNumBackLoad,0,0,153,318,CI_BLACK,LOAD_MENU_PRI);
	pbackLoad = AGetResData(gbackLoad);
	pbackLoadCH = (CelHeader*)pbackLoad;

	//lets only copy the upper left corner of the anim into the cel
	backRect.Set(0,0,152,317);
	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackLoadCH,0,0,RES_ANIM,8150,1,&backRect,false);

	pGuy = &backLoad;
	pGuy->SetRes(RES_CEL,rNumBackLoad);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(LOAD_MENU_PRI);


	//======================================================
	// lets copy in all the borders and boundaries into the background

	CopyCel(pbackLoadCH, 37,67,RES_ANIM,8154,3,true);	// Name Frame
	CopyCel(pbackLoadCH, 66,92,RES_ANIM,8154,1,false);	// Portrait Frame
	CopyCel(pbackLoadCH, 57,211,RES_ANIM,8154,11,false);	//Button Frame

	//if we got here then we have some chars


	//=============================================
	// Write the necessary text into the background

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);	//'CARVED GREEN'
	pTxt = sqbNetChar.Load(NETCHAR_SQB,8); //'YOU'
	ASetString(79,43, pTxt, (uchar *)pbackLoadCH, 101, 0);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	//=============================================
	pButton = &leftLoadB;
	pButton->Create(59, 214, LOAD_BUTTON_PRI, RES_ANIM, 8055, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBackLoad);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Draw();
	pButton->Select(false);		// set button to unselected state - will cause drawing into master cel

	pButton = &rightLoadB;
	pButton->Create(88, 214, LOAD_BUTTON_PRI, RES_ANIM, 8056, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBackLoad);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Draw();
	pButton->Select(false);		// set button to unselected state - will cause drawing into master cel

	ConfigLoadArrowButtons();
	GetLoadCharStats();
	DisplayLoadChar();

	menuDrawn[NC_LOAD] = true;
}


void
NetChar::SetupWaitMenu()
{
	BAM_Guy		*pGuy;
	Rectangle	backRect;

	//======================================================
	// setup background cel filled with black
	gbackWait = ACreateCel(&rNumBackWait,0,0,153,400,CI_BLACK,WAIT_MENU_PRI);
	pbackWait = AGetResData(gbackWait);
	pbackWaitCH = (CelHeader*)pbackWait;

	//lets only copy the upper left corner of the anim into the cel
	backRect.Set(0,0,152,400);
	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackWaitCH,0,0,RES_ANIM,8150,1,&backRect,false);

	pGuy = &backWait;
	pGuy->SetRes(RES_CEL,rNumBackWait);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WAIT_MENU_PRI);


	//======================================================
	// lets copy in all the borders and boundaries into the background

	CopyCel(pbackWaitCH, 37,67,RES_ANIM,8154,3,true);	// Name Frame
	CopyCel(pbackWaitCH, 66,92,RES_ANIM,8154,1,false);	// Portrait Frame

	//copy in last char
	CopyCel(pbackWaitCH,68, 95,RES_ANIM,bGlobal.curBodyAnim,1,true);	// Body
	CopyCel(pbackWaitCH,68, 95,RES_ANIM,bGlobal.curFaceAnim,1,true);	// Face
	CopyCel(pbackWaitCH,68, 95,RES_ANIM,bGlobal.curCoverAnim,1,true);	// Cover
	CopyCel(pbackWaitCH,68,171,RES_ANIM,9004,2,true,clut[bGlobal.curBanner/2]);	// banner 1

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);
	ASetString(40, 71, bGlobal.curName, (uchar *)pbackWaitCH, pbackWaitCH->width-40, 0);

	//=============================================
	// Write the necessary text into the background

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);	//'CARVED GREEN'
	pTxt = sqbNetChar.Load(NETCHAR_SQB,8);	//'YOU'
	ASetString(79,43, pTxt, (uchar *)pbackWaitCH, 101, 0);

	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);	//'BRIGHT BLUE'
	pTxt = sqbNetChar.Load(NETCHAR_SQB,10);	//'WAITING FOR OPPONENT'
	ASetString(31, 229, pTxt, (uchar *)pbackWaitCH, pbackWaitCH->width-31, 0);

	//rect.Set(30,209,152,232);
	//AUpdateRect(&rect);
	//AAnimate();

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	menuDrawn[NC_WAIT] = true;
}


bool
NetChar::HandleMsg(Message* pMsg)
{
	char			mess[100];
	Rectangle	rect;
	int			x;
	char			fileName[14];
	char			saveName[14];
	bool			found = false;
	BAM_ErrorPopup		*pPop;

	if (pMsg->type == MSG_NOTICE)
	{
		// here we are only interested in this type of notice.
		if (pMsg->notice.type == N_CONTROL_REPLY)
		{
			//=========================================================
			//=========================================================
			if(curMenu == NC_MAIN)
			{
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == makeCharB.gSelf)
					{
						//pMono->Out("make char button was hit\n");
						LoadMenu(NC_MAKE);
						return(true);
					}
					else if(pMsg->notice.gSource == loadCharB.gSelf)
					{
						//pMono->Out("load char button was hit\n");
						if(header.entries)
							LoadMenu(NC_LOAD);
						else
						{
							pPop = new BAM_ErrorPopup;
							pPop->SetBasicNet(false);	//netchar must handle all input
							pPop->Setup(pal.gSelf,POPUP_SQB,100,0);
						}

						return(true);
					}
					else if(pMsg->notice.gSource == loadGameB.gSelf)
					{
						//pMono->Out("load game button was hit\n");

						//first verify existence of the net save game
						if(!saveMgr.GetFirstSave(fileName))
						{
							do
							{
								if(GetSaveNum(fileName)	== NET_SAVEGAME_NUM)
								{
									found = true;
									break;
								}
							} while (saveMgr.GetNextSave(fileName) == 0);
						}

						if(!found)
						{
							//net game not found
							pPop = new BAM_ErrorPopup;
							pPop->SetBasicNet(false);
							pPop->Setup(pal.gSelf,OPTION_SQB,42,0);
							return(true);
						}

						if (saveMgr.GetSaveInfo(fileName, saveName, MAX_SAVE_TEXT_SIZE, &newVerNum, &newVerSubNum) != 0)
	 					{
							//bad info in save game
							pPop = new BAM_ErrorPopup;
							pPop->SetBasicNet(false);
							pPop->Setup(pal.gSelf,OPTION_SQB,43,0);
							return(true);
						}

						//retrieve the serial number from game
						netSerialNum = atoi(saveName);

						//set flag telling Cycle() to process net restore packets
						//-lets not use this var yet as it could lockout eventmgr in main loop
						//netRestoreNum = NET_SAVEGAME_NUM;
						attemptRestore = true;

						//no need to set here -cycle will set and send it
						//packetType = BMSG_RESTORE;

						return(true);
					}
					else if(pMsg->notice.gSource == startGameB.gSelf)
					{
						//pMono->Out("start game button was hit\n");

						meDone = true;
						packetType = BMSG_DONE;

						LoadMenu(NC_WAIT); //deactivate all buttons but disCancel

						return(true);
					}
					else if(pMsg->notice.gSource == disCancelB.gSelf)
					{
						//pMono->Out("disCancel button was hit\n");
						// send disconnect msg
						packetType = BMSG_DISCONNECT;
						return(true);
					}

				}
			}
			//=========================================================
			//=========================================================
			else if(curMenu == NC_MAKE)
			{
				// a reply from one of our TOGGLE buttons.
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_ACTIVATED)
				{
					if(pMsg->notice.gSource == manB.gSelf)
					{
						//pMono->Out("\nMan button was hit.");
						womanB.Select(false);
						otherB.Select(false);
						bGlobal.curCat = G_MEN;
						UpdateGender();
						return(true);
					}
					if(pMsg->notice.gSource == womanB.gSelf)
					{
						//pMono->Out("\nWoman button was hit.");
						manB.Select(false);
						otherB.Select(false);
						bGlobal.curCat = G_WOMEN;
						UpdateGender();
						return(true);
					}
					if(pMsg->notice.gSource == otherB.gSelf)
					{
						//pMono->Out("\nOther button was hit.");
						manB.Select(false);
						womanB.Select(false);
						bGlobal.curCat = G_OTHER;
						UpdateGender();
						return(true);
					}
				}
				else
				// a reply from one of our CLICK buttons.
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == nameBox.gSelf)
					{
						//pMono->Out("\nGot a nameBox deactivation message.");
						char *pText = ADerefAs(char, gNameText);
						if(strcmp(bGlobal.curName,pText))
						{
							strcpy(bGlobal.curName,pText);
							curNameNum = -1; //custom name
						}
						
						return(true);
					}

					for(x=0;x<4;x++)
					{
						if(pMsg->notice.gSource == leftArrowB[x].gSelf)
						{
							//pMono->Out("\nLeft Arrow button was hit.");
							switch(x)
							{
								case C_COVER:
									AdvCover(C_DECREMENT);
									break;
								case C_FACE:
									AdvFace(C_DECREMENT);
									break;
								case C_BODY:
									AdvBody(C_DECREMENT);
									break;
								case C_BANNER:
									AdvBanner(C_DECREMENT);
									break;
							}
							return true;
						}

						if(pMsg->notice.gSource == rightArrowB[x].gSelf)
						{
							//pMono->Out("\nRight Arrow button was hit.");
							switch(x)
							{
								case C_COVER:
									AdvCover(C_INCREMENT);
									break;
								case C_FACE:
									AdvFace(C_INCREMENT);
									break;
								case C_BODY:
									AdvBody(C_INCREMENT);
									break;
								case C_BANNER:
									AdvBanner(C_INCREMENT);
									break;
							}
							return true;
						}
					}
					if(pMsg->notice.gSource == randomCharB.gSelf)
					{
						//pMono->Out("\nRandom Char. button was hit.");
						UpdateRandom();
						return(true);
					}
					if(pMsg->notice.gSource == customNameB.gSelf)
					{
						//pMono->Out("\nCustom name button was hit");
						nameBox.Select(true);
						return(true);
					}
					if(pMsg->notice.gSource == randomNameB.gSelf)
					{
						//pMono->Out("\nRandom name button was hit");
						LoadName(RandomName());
						return(true);
					}
					if(pMsg->notice.gSource == startGameB.gSelf)
					{
						//pMono->Out("start game button was hit\n");

						//lets give the newly made char an id
						bGlobal.curCharId = GetNewCharId();

						meDone = true;
						packetType = BMSG_DONE;

						LoadMenu(NC_WAIT); //deactivate all buttons but disCancel

						return(true);
					}
					if(pMsg->notice.gSource == disCancelB.gSelf)
					{
						//pMono->Out("disCancel button was hit\n");

						//lets give the newly made char an id
						//can't hurt (same char = same id)
						bGlobal.curCharId = GetNewCharId();

						if(meDone)
							// send disconnect msg
							packetType = BMSG_DISCONNECT;
						else
							LoadMenu(NC_MAIN);

						return(true);
					}
				}
				else
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DEACTIVATED)
				{
					if(pMsg->notice.gSource == manB.gSelf)
					{
						//pMono->Out("\nGot a MAN deactivation message.");
						//reactivate
						manB.Select(true);
						return(true);
					}
					else
					if(pMsg->notice.gSource == womanB.gSelf)
					{
						//pMono->Out("\nGot a WOMAN deactivation message.");
						//reactivate
						womanB.Select(true);
						return(true);
					}
					else
					if(pMsg->notice.gSource == otherB.gSelf)
					{
						//pMono->Out("\nGot a OTHER deactivation message.");
						//reactivate
						otherB.Select(true);
						return(true);
					}
				}
				else
         	{
            	snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
            	APanic(mess);
         	}
			}
			//=========================================================
			//=========================================================
			else if(curMenu == NC_LOAD)
			{
				// a reply from one of our CLICK buttons.
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == leftLoadB.gSelf)
					{
						pMono->Out("\nleftLoad button was hit.");

						if(!campCnt)
							return(true);

						campCnt--;
						GetLoadCharStats();
						DisplayLoadChar();
						ConfigLoadArrowButtons();

						return(true);
					}
					if(pMsg->notice.gSource == rightLoadB.gSelf)
					{
						pMono->Out("\nrightLoad button was hit");

						if(campCnt == (header.entries - 1))
							return(true);

						campCnt++;
						GetLoadCharStats();
						DisplayLoadChar();
						ConfigLoadArrowButtons();
						return(true);
					}
					if(pMsg->notice.gSource == startGameB.gSelf)
					{
						//pMono->Out("start game button was hit\n");

						meDone = true;
						packetType = BMSG_DONE;

						LoadMenu(NC_WAIT); //deactivate all buttons but disCancel

						return(true);
					}
					if(pMsg->notice.gSource == disCancelB.gSelf)
					{
						//pMono->Out("disCancel button was hit\n");

						if(meDone)
							// send disconnect msg
							packetType = BMSG_DISCONNECT;
						else
							LoadMenu(NC_MAIN);

						return(true);
					}
					return(true);	//shouldn't get here
				}
				else
         	{
            	snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
            	APanic(mess);
         	}
			}
			//=========================================================
			//=========================================================
			else if(curMenu == NC_WAIT)
			{
				// a reply from one of our CLICK buttons.
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == disCancelB.gSelf)
					{
						//pMono->Out("disCancel button was hit\n");
						// send disconnect msg
						packetType = BMSG_DISCONNECT;
						return(true);
					}
				}
				else
         	{
            	snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
            	APanic(mess);
         	}
			}
		}
	}

	// LETS CHECK WITH RECEIVERS FIRST BEFORE WE START HANDLING ANY
	//  KEYSTROKE EVENTS FOR TEST PURPOSES
	if(BAM_Room::HandleMsg(pMsg))
		return true;

	//OK, buttons have now had a shot at it so:
	if (pMsg->type == MSG_EVENT)
	{
		switch(pMsg->event.type)
		{
			case E_KEY_DOWN:
				switch (pMsg->event.value)
				{
					case K_X:
						if(pMsg->event.modifiers & MOD_ALT)
							packetType = BMSG_DISCONNECT;
						return true;
						break;
				}
				break;

			case E_MOUSE_UP:
				// the following is done to make sure that the active button
				// (if any) receives the MOUSE_UP event even if mouse has
				// wandered out of the rect
				if(gCurControl)
				{
					Object	*pObject;
					pObject = ADerefAs(Object, gCurControl);
					if(pObject->HandleMsg(pMsg))
						return(true);
				}
				break;
		}
	}

	return false;
}


void
NetChar::Cycle()
{
	ticks_t		sendSomethingTicks;
	struct NetcharPacket	*pPacket;
	Rectangle	rect;
	int			restoreReply = BMSG_RESTORE_NAK;
	char			fileName[14];
	char			saveName[14];
	bool			found = false;
	BAM_ErrorPopup		*pPop;

//	pMono->Out("NetChar: Start Cycle()\n");

	//lets first cycle anything else that needs it -like a bam_box cursor
	BAM_Room::Cycle();

	if(!bothDone)
	{
		if(attemptRestore)
		{
			packetType = BMSG_RESTORE;
			ncPacket.BPHdr.serialNum = netSerialNum;
			ncPacket.BPHdr.aux1  = newVerNum;
			ncPacket.BPHdr.aux2  = newVerSubNum;
		}

		//we handle only init. packets until we've sync'd the two rooms
		if(roomState == ST_INIT)
			packetType = BMSG_INIT;

		if(packetType == BMSG_NULL)
			//no MAJOR msgs set it so lets now check for character changes
			CheckChar();	//will set packetType to BMSG_DATA if nec.

		UpdateTicks();
		//we want some activity every 8 secs -even a null packet
		//unless we recently have send or received 
		sendSomethingTicks = ATicks();

		//lets only send if we have to -lets send null and init packets only when timer expires
		if((packetType != BMSG_NULL && packetType != BMSG_INIT) || sendSomethingTicks > sendExpireTicks)
		{
			pMono->Out("NetChar: Sending frame %d, packetType %d.\n", currFrame,packetType);
			pComm->ClearError();
			ncPacket.frameNum = currFrame;

			if(packetType == BMSG_DATA)
				LoadCharPacket();

			// send it out
			ncPacket.BPHdr.sourceRoom = bGlobal.roomMgr.curRoomNum;
			sPacket1.header.ID = packetType;
			sPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player
			sPacket1.header.len = sizeof(NetcharPacket);
			sPacket1.pData = &ncPacket;


			pCommMgr->SendData(&sPacket1);
			sendExpireTicks = ATicks() + TICKS_PER_SEC * 8;
			lastSendError = pCommMgr->GetError();
			if (lastSendError)
			{
				pMono->Out ("Send error %d[%s]\n", (int)lastSendError,
					pErrorStrings[lastSendError]);
				sendErrorCnt++;

				//lets give it some chance
				if(sendErrorCnt > 2)
				{
					//APanic("NetChar: failed to send.\n");
					pCommMgr->Disconnect();
					bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;
					if(!errorPopupCreated)
					{
						pPop = new BAM_ErrorPopup;
						pPop->SetMode(M_MODAL);
						pPop->Setup(pal.gSelf,OPTION_SQB,47,BR_MENU);
						errorPopupCreated = true;
					}
				}

				//next cycle we'll try again
			}
			else
			{
				sendErrorCnt = 0;
			}
		}


		pComm->ClearError();		// why?  Cuz Russell said to.

		//BEFORE LOOKING AT THE INCOMING PACKET -DID WE SEND A DISCONNECT?
		if(packetType == BMSG_DISCONNECT)
		{
			//BUGBUG -we're assuming disconnect packet was sent w/o error
			//LETS BAIL
			pCommMgr->Disconnect();
			bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;
			bGlobal.roomMgr.NewRoom(BR_MENU);

		}

		//quickly check for incoming packet
		if(pCommMgr->DataAvailable())
		{
			//lets preset data ptr before receiving
			sPacket1.pData = &ncPacket;
			int error = pCommMgr->ReceiveData(&sPacket1);

			if(error)
			{
				pMono->Out("  Error, no packet received.  Error == %d\n", error);
				//APanic("NetChar: no packet received.");
				pCommMgr->Disconnect();
				bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;
				pPop = new BAM_ErrorPopup;
				pPop->Setup(pal.gSelf,OPTION_SQB,46,BR_MENU);
			}
			else
			{
				sendExpireTicks = ATicks() + TICKS_PER_SEC * 8;
				pPacket = (NetcharPacket*)sPacket1.pData;
				if(pPacket->frameNum != currFrame)
				{
					//WE NOW RUN ASYNC
					//pMono->Out("  Frames out-of-sync OK?  Local=%d remote=%d\n", currFrame, pPacket->frameNum);
					//APanic("NetChar: Frame count error!\n");
				}

				//decode packet by type placed in header id
				switch(sPacket1.header.ID)
				{
					case BMSG_NULL:
						pMono->Out("NetChar: Received BMSG_NULL, with frame %d.\n", pPacket->frameNum);
						break;

					case BMSG_INIT:
						//if matching room nums
						pMono->Out("NetChar: Received BMSG_INIT, with frame %d.\n", pPacket->frameNum);
						if(roomState == ST_INIT && pPacket->BPHdr.sourceRoom == bGlobal.roomMgr.curRoomNum)
						{
							// send an init packet right back out
							ncPacket.BPHdr.sourceRoom = bGlobal.roomMgr.curRoomNum;
							sPacket1.header.ID = BMSG_INIT;
							sPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player
							sPacket1.header.len = sizeof(NetcharPacket);
							sPacket1.pData = &ncPacket;
							pCommMgr->SendData(&sPacket1);
							if (!pCommMgr->GetError())
							{
								roomState = ST_DATA;
								startGameB.Listen(true);
							}
							else
							{
								//APanic("NetChar: failed to send return INIT.\n");
								pCommMgr->Disconnect();
								bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;
								pPop = new BAM_ErrorPopup;
								pPop->Setup(pal.gSelf,OPTION_SQB,50,BR_MENU);
							}
						}
						break;

					case BMSG_RESTORE:
						//we must verify a matching save game on this side
						pMono->Out("NetChar: Received BMSG_RESTORE, with frame %d.\n", pPacket->frameNum);
						if(!saveMgr.GetFirstSave(fileName))
						{
							do
							{
								if(GetSaveNum(fileName)	== NET_SAVEGAME_NUM)
								{
									found = true;
									break;
								}
							} while (saveMgr.GetNextSave(fileName) == 0);
						}

						//restoreReply is preset to NAK
						if(found)
						{
							if (saveMgr.GetSaveInfo(fileName, saveName, MAX_SAVE_TEXT_SIZE, &newVerNum, &newVerSubNum) != 0)
							{
								//bad info in save game
								pPop = new BAM_ErrorPopup;
								pPop->SetBasicNet(false);
								pPop->Setup(pal.gSelf,OPTION_SQB,43,0);
								netSerialNum = 0;
							}
							else
							{
								//retrieve the serial number from game
								netSerialNum = atoi(saveName);
							}

						
							if(netSerialNum == pPacket->BPHdr.serialNum 
								&& newVerNum == pPacket->BPHdr.aux1
								&& newVerSubNum == pPacket->BPHdr.aux2)
							{
								restoreReply = BMSG_RESTORE_ACK;
							}
						}

						//send an ack or nak
						ncPacket.BPHdr.sourceRoom = bGlobal.roomMgr.curRoomNum;
						sPacket1.header.ID = restoreReply;
						sPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player
						sPacket1.header.len = sizeof(NetcharPacket);
						sPacket1.pData = &ncPacket;
						pCommMgr->SendData(&sPacket1);
						if (!pCommMgr->GetError ())
						{
							//it got thru -lets set our side
							netRestoreNum = NET_SAVEGAME_NUM;
						}

						break;

					case BMSG_RESTORE_ACK:
						//a matching save game was found on the other side
						pMono->Out("NetChar: Received BMSG_RESTORE_ACK, with frame %d.\n", pPacket->frameNum);
						attemptRestore = false;
						netRestoreNum = NET_SAVEGAME_NUM;
						break;

					case BMSG_RESTORE_NAK:
						//NEED error popup here -restore failed to find matching save game
						pMono->Out("NetChar: Received BMSG_RESTORE_NAK, with frame %d.\n", pPacket->frameNum);

						pPop = new BAM_ErrorPopup;
						pPop->SetBasicNet(false);
						pPop->Setup(pal.gSelf,OPTION_SQB,44,0);

						attemptRestore = false;
						break;

					case BMSG_DATA:
						//set enemy global vars
						pMono->Out("NetChar: Received BMSG_DATA, with frame %d.\n", pPacket->frameNum);
						bGlobal.enemyCharId = pPacket->charId;
						bGlobal.enemyBodyAnim = pPacket->bodyAnim;
						bGlobal.enemyFaceAnim = pPacket->faceAnim;
						bGlobal.enemyCoverAnim = pPacket->coverAnim;
						//bGlobal.enemyBanner = pPacket->banner;	//enemyBanner is now an opp. of curBanner
						strcpy(bGlobal.enemyName,pPacket->name);

						if (pBam->playerSide == SIDE2) {
							ASetGameSpeed(static_cast<GameSpeed>(pPacket->game_speed));
							bGlobal.fFastCollect = (pPacket->fast_collect != 0);
						}

						//clear "wait for opponent" msg after first data packet
						if(!clearWaitMsg)
						{
							clearWaitMsg = true;
							rect.Set(174,228,292,266);
							CopyCel(pbackCH,174,228,RES_ANIM,8150,1,&rect,false);
							AUpdateRect(&rect);
						}

						RefreshEnemyChar();
						break;

					case BMSG_DISCONNECT:
						//popup w/ disconnect info
						pMono->Out("NetChar: Received BMSG_DISCONNECT, with frame %d.\n", pPacket->frameNum);

						bGlobal.storyLine = STORY_NONE;
						bGlobal.netDisconnect = true;
						pPop = new BAM_ErrorPopup;
						pPop->Setup(pal.gSelf,OPTION_SQB,40,BR_MENU); //'opponent has disconnected'
						return;
						break;

					case BMSG_DONE:
						pMono->Out("NetChar: Received BMSG_DONE, with frame %d.\n", pPacket->frameNum);
						enemyDone = true;
						//write enemy done msg on screen
						pFontMgr->SetRes(9052);
						SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);	//'BRIGHT BLUE'
						pTxt = sqbNetChar.Load(NETCHAR_SQB,7);	//'ready to play'
						ASetString(195, 229, pTxt, (uchar *)pbackCH, pbackCH->width-195, 0);
						rect.Set(194,228,296,252);
						AUpdateRect(&rect);
						pFontMgr->SetRes(9050);
						SetFontColors(CI_SKIP,93,90,93,90,93,90);
						break;
				}
			}
		}
		//else
		//  	pMono->Out("\nNetChar: no incoming data this frame");


		//we've sent our packet now lets clear it
		packetType = BMSG_NULL;

		currFrame++;
	}

	if(enemyDone && meDone)
	{
		pMono->Out("    NetChar: Both done at frame %d\n",currFrame-1);
		pMono->Out("    meDone is %d, and enemyDone is %d\n",meDone,enemyDone);
		bothDone = true;

		//lets set side colors from banner colors
		pBam->sideColors[pBam->playerSide] = (unitColor_t) ((bGlobal.curBanner/2) + 1);
		pBam->sideColors[(pBam->playerSide==SIDE1)?SIDE2:SIDE1] = (unitColor_t) ((bGlobal.enemyBanner/2) + 1);

		//lets make sure we reset these -nethall relies on this.
		bGlobal.missionsDone = 0;
		bGlobal.curCampaignScore = 0;

		bGlobal.roomMgr.NewRoom(BR_NET_STORY);
	}

//	pMono->Out("NetChar: End Cycle()");
}


void
NetChar::CheckChar()
{
	if(oldCoverAnim != bGlobal.curCoverAnim ||
		oldFaceAnim  != bGlobal.curFaceAnim  ||
		oldBodyAnim  != bGlobal.curBodyAnim  ||
		strcmp(oldName,bGlobal.curName))
	{
		oldCoverAnim = bGlobal.curCoverAnim;
		oldFaceAnim  = bGlobal.curFaceAnim;
		oldBodyAnim  = bGlobal.curBodyAnim;
		strcpy(oldName,bGlobal.curName);
		packetType = BMSG_DATA;
	}
}


int
NetChar::GetSaveNum(char *fileName)
{
	char		fn[14],*pfn;
	char		*pStr;
	int		x,cnt;

	//get save game num from filename
	pStr = strrchr(fileName,'\\');
	if(!pStr)
		pStr = fileName;
	else
		pStr++; //get past '\'

	cnt=0;
	pfn = fn;
	do
	{
		*pfn++ = *pStr++;
		cnt++;
	}while (*pStr != '.' && cnt < 14);
	*pfn = '\0';
	// array is 0-9 but save games are 1-10 so subtract 1
	x = atoi(fn);

	return x;
}


int
NetChar::GetCoverAnim(int cover)
{
	return(FIRSTCOVERANIM + (2*cover));
}


void
NetChar::RandomCover()
{
	do
	{
		curCover = MyRandom(MAX_COVER);

	} while(!(coverGen[curCover] & bGlobal.curCat));

	bGlobal.curCoverAnim = FIRSTCOVERANIM + (2*curCover);
}


void
NetChar::LoadCover()
{
	BAM_Guy	*pGuy;

	pGuy = &curCoverGuy;
	if(coverGen[curCover] & bGlobal.curCat)
		pGuy->SetRes(RES_ANIM,bGlobal.curCoverAnim,1);
	else
		AdvCover(C_INCREMENT);
	pGuy->SetPos(45,95);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MAKE_BUTTON_PRI+30);
}


int
NetChar::GetFaceAnim(int cat, int face)
{
	switch(cat)
	{
		case G_MEN:
			return(FIRSTMENFACEANIM + (2*face));
			break;
		case G_WOMEN:
			return(FIRSTWOMENFACEANIM + (2*face));
			break;
		case G_OTHER:
			return(FIRSTOTHERFACEANIM + (2*face));
			break;
		default:
			APanic("Bad catagory in NetChar room");
			break;
	}
	return 0;
}


void
NetChar::RandomFace()
{
	switch(bGlobal.curCat)
	{
		case G_MEN:
			curFace = MyRandom(MAX_FACE_MEN);
			bGlobal.curFaceAnim = FIRSTMENFACEANIM + (2*curFace);
			break;
		case G_WOMEN:
			curFace = MyRandom(MAX_FACE_WOMEN);
			bGlobal.curFaceAnim = FIRSTWOMENFACEANIM + (2*curFace);
			break;
		case G_OTHER:
			curFace = MyRandom(MAX_FACE_OTHER);
			bGlobal.curFaceAnim = FIRSTOTHERFACEANIM + (2*curFace);
			break;
		default:
			APanic("Bad catagory in NetChar room");
			break;
	}
}


void
NetChar::LoadFace()
{
	BAM_Guy	*pGuy;

	pGuy = &curFaceGuy;
	pGuy->SetRes(RES_ANIM,bGlobal.curFaceAnim,1);
	pGuy->SetPos(45,95);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MAKE_BUTTON_PRI+20);
}


int
NetChar::GetBodyAnim(int body)
{
	return (FIRSTBODYANIM + (2*body));
}


void
NetChar::RandomBody()
{
	do
	{
		curBody = MyRandom(MAX_BODY);

	} while(!(bodyGen[curBody] & bGlobal.curCat));

	bGlobal.curBodyAnim = FIRSTBODYANIM + (2*curBody);
}


void
NetChar::LoadBody()
{
	BAM_Guy	*pGuy;

	pGuy = &curBodyGuy;
	if(bodyGen[curBody] & bGlobal.curCat)
		pGuy->SetRes(RES_ANIM,bGlobal.curBodyAnim,1);
	else
		AdvBody(C_INCREMENT);
	pGuy->SetPos(45,95);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MAKE_BUTTON_PRI+10);
}


void
NetChar::RandomBanner()
{
	int			bannerTmp;

	bannerTmp = MyRandom(MAX_BANNER);
	bGlobal.curBanner =	bannerTmp*2;
}


void
NetChar::LoadBanner()
{
	BAM_Guy		*pGuy;

	//OLD: curBanner is number which reflects the color of the banner and its type.
	//		 odd numbers were 'slashes' and even were 'dots'
	//		 first two banners 'belong' to palette '1'
	//		 second set of banners 'belong' to palette '2', etc.
	//
	//NEW: player always sees himself as "slash" and enemy as "dots"
	//		 so just set curBanner to an even multiple of two to handle color the same way.

	pGuy = &curBannerGuy;
	pGuy->SetRes(RES_ANIM,9004,2);	// banner "slash"
	pGuy->SetPos(45,171);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(MAKE_BUTTON_PRI);
	//pGuy->SetClut(CLUT1_START+(CLUT_SIZE*bGlobal.curBanner/2),CLUT_SIZE,clut[bGlobal.curBanner/2]);
	pGuy->SetClut(CLUT1_START,CLUT_SIZE,clut[bGlobal.curBanner/2]);

	//ShiftPal(bannerTmp);

}


void
NetChar::AdvCover(dir_t dir)
{
	bool	nextCoverInGen=false;

	// keep advancing until next anim in Gender is found
	do
	{
		//range from 0 to (max. size - 1)
		if(dir == C_INCREMENT)
		{
			curCover++;
			if(curCover >= MAX_COVER)
				curCover = 0;
		}
		else // == C_DECREMENT
		{
			curCover--;
			if(curCover < 0)
				curCover = MAX_COVER - 1;
		}

		if(coverGen[curCover] & bGlobal.curCat)
		{
			bGlobal.curCoverAnim = FIRSTCOVERANIM + (2*curCover);
			curCoverGuy.SetRes(RES_ANIM,bGlobal.curCoverAnim,1);
			nextCoverInGen = true;
		}

	} while(!nextCoverInGen);
}


void
NetChar::AdvFace(dir_t dir)
{
	int	max;
	int	firstAnim;
	
	switch(bGlobal.curCat)
	{
		case G_MEN:
			max = MAX_FACE_MEN;
			firstAnim = FIRSTMENFACEANIM;
			break;
		case G_WOMEN:
			max = MAX_FACE_WOMEN;
			firstAnim = FIRSTWOMENFACEANIM;
			break;
		case G_OTHER:
			max = MAX_FACE_OTHER;
			firstAnim = FIRSTOTHERFACEANIM;
			break;
		default:
			APanic("Bad catagory in NetChar room");
			break;
	}

	//range from 0 to (max. size - 1)
	if(dir == C_INCREMENT)
	{
		curFace++;
		if(curFace >= max)
			curFace = 0;
	}
	else // == C_DECREMENT
	{
		curFace--;
		if(curFace < 0)
			curFace = (max - 1);
	}

	bGlobal.curFaceAnim = firstAnim + (2*curFace);
	curFaceGuy.SetRes(RES_ANIM,bGlobal.curFaceAnim,1);
}


void
NetChar::AdvBody(dir_t dir)
{
	bool	nextBodyInGen=false;

	// keep advancing until next anim in Gender is found
	do
	{
		//range from 0 to (max. size - 1)
		if(dir == C_INCREMENT)
		{
			curBody++;
			if(curBody >= MAX_BODY)
				curBody = 0;
		}
		else // == C_DECREMENT
		{
			curBody--;
			if(curBody < 0)
				curBody = MAX_BODY - 1;
		}

		if(bodyGen[curBody] & bGlobal.curCat)
		{
			bGlobal.curBodyAnim = FIRSTBODYANIM + (2*curBody);
			curBodyGuy.SetRes(RES_ANIM,bGlobal.curBodyAnim,1);
			nextBodyInGen = true;
		}

	} while(!nextBodyInGen);
}


void
NetChar::AdvBanner(dir_t dir)
{
	BAM_Guy	*pGuy;

	//range from 0 to (max. size - 1)
	if(dir == C_INCREMENT)
	{
		bGlobal.curBanner += 2;
		if(bGlobal.curBanner >= MAX_BANNER*2)
			bGlobal.curBanner -= (MAX_BANNER*2);

		//ShiftPal(bGlobal.curBanner / 2);
	}
	else // == C_DECREMENT
	{
		bGlobal.curBanner -= 2;
		if(bGlobal.curBanner < 0)
			bGlobal.curBanner += (MAX_BANNER*2);

		//ShiftPal(bGlobal.curBanner / 2);
	}

	pGuy = &curBannerGuy;
	//pGuy->SetClut(CLUT1_START+(CLUT_SIZE*(bGlobal.curBanner/2)),CLUT_SIZE,clut[bGlobal.curBanner/2]);
	pGuy->SetClut(CLUT1_START,CLUT_SIZE,clut[bGlobal.curBanner/2]);

	//opposing banner must be an opposite
	SetEnemyBannerColor();
	RefreshEnemyChar();
}


void
NetChar::UpdateGender()
{
	// Leave banner as is

	// check cover for need to adv. to proper gender
	if(!(coverGen[curCover] & bGlobal.curCat))
		AdvCover(C_INCREMENT);

	// check body for need to adv. to proper gender
	if(!(bodyGen[curBody] & bGlobal.curCat))
		AdvBody(C_INCREMENT);

	AdvFace(C_INCREMENT);
	if(curNameNum != -1) //if not custom name
	{
		LoadName(RandomName());
	}
}


void
NetChar::UpdateRandom()
{
	//cover ====================================
	curCover = MyRandom(MAX_COVER,curCover);
	bGlobal.curCoverAnim = FIRSTCOVERANIM + (2*curCover);

	if(coverGen[curCover] & bGlobal.curCat)
	{
		curCoverGuy.SetRes(RES_ANIM,bGlobal.curCoverAnim,1);
	}
	else
	{
		AdvCover(C_INCREMENT);
	}

	//face =====================================
	switch(bGlobal.curCat)
	{
		case G_MEN:
			curFace = MyRandom(MAX_FACE_MEN,curFace);
			bGlobal.curFaceAnim = FIRSTMENFACEANIM + (2*curFace);
			break;
		case G_WOMEN:
			curFace = MyRandom(MAX_FACE_WOMEN,curFace);
			bGlobal.curFaceAnim = FIRSTWOMENFACEANIM + (2*curFace);
			break;
		case G_OTHER:
			curFace = MyRandom(MAX_FACE_OTHER,curFace);
			bGlobal.curFaceAnim = FIRSTOTHERFACEANIM + (2*curFace);
			break;
		default:
			APanic("Bad catagory in NetChar room");
			break;
	}

	curFaceGuy.SetRes(RES_ANIM,bGlobal.curFaceAnim,1);

	//body ======================================
	curBody = MyRandom(MAX_BODY,curBody);
	bGlobal.curBodyAnim = FIRSTBODYANIM + (2*curBody);

	if(bodyGen[curBody] & bGlobal.curCat)
		curBodyGuy.SetRes(RES_ANIM,bGlobal.curBodyAnim,1);
	else
		AdvBody(C_INCREMENT);

	//banner ====================================
	bGlobal.curBanner = MyRandom(MAX_BANNER,bGlobal.curBanner);
	//curBannerGuy.SetClut(CLUT1_START+(CLUT_SIZE*(bGlobal.curBanner/2)),CLUT_SIZE,clut[bGlobal.curBanner/2]);
	curBannerGuy.SetClut(CLUT1_START,CLUT_SIZE,clut[bGlobal.curBanner/2]);

	//opposing banner must be an opposite
	SetEnemyBannerColor();
	RefreshEnemyChar();

	// first two banners 'belong' to palette '1'
	// second set of banners 'belong' to palette '2', etc.
	//ShiftPal(bGlobal.curBanner / 2);
}



int
NetChar::RandomName()
{
	switch(bGlobal.curCat)
	{
		case G_MEN:
			sqbRes = MEN_SQB;
			if(!maxMenNameNum)
				maxMenNameNum = atoi(sqbNetChar.Load(MEN_SQB,1));
			//add 2 to num to get zero based random num past first sqb used above
			curNameNum = MyRandom(maxMenNameNum,curNameNum-2) + 2;
			return sqbRes;
			break;
		case G_WOMEN:
			sqbRes = WOMEN_SQB;
			if(!maxWomenNameNum)
				maxWomenNameNum = atoi(sqbNetChar.Load(WOMEN_SQB,1));
			curNameNum = MyRandom(maxWomenNameNum,curNameNum-2) + 2;
			return sqbRes;
			break;
		case G_OTHER:
			sqbRes = OTHER_SQB;
			if(!maxOtherNameNum)
				maxOtherNameNum = atoi(sqbNetChar.Load(OTHER_SQB,1));
			curNameNum = MyRandom(maxOtherNameNum,curNameNum-2) + 2;
			return sqbRes;
			break;
	}
	return 0;
}

void
NetChar::LoadName(int sqbRes)
{
	pTxt = sqbNetChar.Load(sqbRes,curNameNum);
	char *pText = ADerefAs(char, gNameText);
	strcpy(bGlobal.curName,pTxt);
	strcpy(pText,pTxt); //copy into name box

	nameBox.UpdateString();
}

int
NetChar::MyRandom(int max, int oldnum)
{
	int num;

	if(oldnum == -1)
	{
		num = ARandom2(max);
		return num;
	}

	do
	{
		num = ARandom2(max);
	} while(num == oldnum);
	return num;
}


//void
//NetChar::ShiftPal(int bannerColor,int range)
//{
//	//load extended palette and
//	//remap bitmap to the appropriate palette range or
//	// copy palette range into range used by banner bitmap
//	// the second is prob. faster
//
//	if(range)
//	{
//		pal.LoadPartial(9101,CLUT2_START+(bannerColor*CLUT_SIZE),CLUT_SIZE,CLUT2_START);
//		pal.UpdateColors(CLUT2_START,207);
//	}
//	else
//	{
//		pal.LoadPartial(9101,CLUT1_START+(bannerColor*CLUT_SIZE),CLUT_SIZE,CLUT1_START);
//		pal.UpdateColors(CLUT1_START,200);
//	}
//	
//	//update banner guy
//	curBannerGuy.SetState(S_CHANGED, true);
//
//
//}


int
NetChar::GetNewCharId()
{
	int	i,len;
	char	tmpName[40];
	uint32	*nptr,sum = 0;
	char	numStr[5];

 	//lets hash it

	len = strlen(bGlobal.curName);
	strcpy(tmpName,bGlobal.curName);

	//lets pad it out to a even multiple of 4
	while(len % 4)
	{
		tmpName[len++] = ' ';
		tmpName[len] = '\0';
	}

	for(i=0;i<len;i++)
		tmpName[i] = (char)toupper(tmpName[i]);

	nptr = (uint32*)tmpName;

	for(i=0;i<len/4;i++)
	{
	 	sum += *nptr;
		nptr++;
	}

	//reuse str var
	snprintf(numStr, sizeof(numStr),"%4d",bGlobal.curBodyAnim);
	strcpy(tmpName,numStr);

	snprintf(numStr, sizeof(numStr),"%4d",bGlobal.curFaceAnim);
	strcat(tmpName,numStr);

	snprintf(numStr, sizeof(numStr),"%4d",bGlobal.curCoverAnim);
	strcat(tmpName,numStr);

	nptr = (uint32*)tmpName;

	for(i=0;i<3;i++)
	{
	 	sum += *nptr;
		nptr++;
	}

	return sum;
}


//int
//NetChar::GetNewCharId()
//{
//	FILE *idFile;
//	int	i;
//
//	if(bGlobal.writeOut)
//	{
//		idFile = fOpenPrefs("netchrId.dat","rb+");
//		if(!idFile)
//		{
//			//lets create it
//			idFile = fOpenPrefs("netchrId.dat","wb+");
//			if(!idFile)
//			{
//				pMono->Out("NetChar: Error, couldn't open netchrId.dat");
//				i = 1;
//			}
//			else
//			{
//				//make it rand so each player will have chars in a diff range
//				i = ARandom(1000000000);
//			}
//		}
//		else
//		{
//			fread(&i,1,sizeof(int),idFile);
//			i++;
//			rewind(idFile);
//		}
//
//		if(idFile)
//		{
//			fwrite(&i,sizeof(int),1,idFile);
//			fclose(idFile);
//		}
//	}
//	else
//	{
//		i = 1;
//	}
//
//	return i;
//}
//
//
//int
//NetChar::GetLastCharId()
//{
//	FILE *idFile;
//	int	i;
//
//	if(bGlobal.writeOut)
//	{
//		idFile = fOpenPrefs("lastchar.dat","rb+");
//		if(!idFile)
//		{
//			i = 0;
//		}
//		else
//		{
//			fread(&i,1,sizeof(int),idFile);
//			fclose(idFile);
//		}
//
//	}
//	else
//		i = 0;
//
//	return i;
//}
//
//
//void
//NetChar::WriteLastCharId()
//{
//	FILE *idFile;
//
//	if(bGlobal.writeOut)
//	{
//		idFile = fOpenPrefs("lastchar.dat","rb+");
//		if(!idFile)
//		{
//			//lets create it
//			idFile = fOpenPrefs("lastchar.dat","wb+");
//			if(!idFile)
//				APanic("NetChar: Error, couldn't open lastChar.dat");
//		}
//
//		fwrite(&bGlobal.curCharId,sizeof(int),1,idFile);
//		fclose(idFile);
//	}
//}


void
NetChar::SetEnemyBannerColor()
{
	int	i,idx;

	unitColor_t unitColorArr[5][2] = {
											RED,BLUE,
											BLUE,RED,
											PURPLE,GOLD,
											GOLD,PURPLE,
											GREEN,GOLD};	//THIS LAST NOT IN USE

	//colors start at 1, but curBanner/2 starts at 0
	idx = bGlobal.curBanner/2 + 1;

	for(i=0;i<5;i++)
	{
	 	if(idx == unitColorArr[i][0])
		{
			bGlobal.enemyBanner = (unitColorArr[i][1] - 1) * 2;
			break;
		}
	}
}


//===================================================================
//===================================================================
//Load Char. functions

//returns whether it found any chars in campaign file
bool
NetChar::SetupCampaign()
{
	NetCampaignHeader	*pHead;
	NetCampaign 		*pCamp;
	int					x,charSize;

	if(bGlobal.writeOut)
	{
		campFile = fOpenPrefs("netcamp.dat", "rb+");

		if(!campFile)
		{
			//no file, no chars found
			return false;
		}
	}
	else
	{
		//lets pretend new file since writes not allowed
		//no file, no chars found

		return false;
	}


	pHead = &header;
	memset(pHead,0,sizeof(NetCampaignHeader));

	//read file header
	charSize = (int)SDL_ReadIO(campFile,pHead,sizeof(NetCampaignHeader));
	if(!charSize)
	{
		SDL_CloseIO(campFile);
		return false;
	}

	if(!pHead->entries)
	{
		SDL_CloseIO(campFile);
		return false;
	}

	size_t theSize = (pHead->entries+1)*sizeof(NetCampaign);
	gCampArr = AMalloc(theSize);
	pCampArr = ADerefAs(NetCampaign,gCampArr);
	pCamp = pCampArr;

	//read structures from file into array
	for(x=0;x<pHead->entries;x++)
	{
		charSize = (int)SDL_ReadIO(campFile,pCamp,sizeof(NetCampaign));
		if(!charSize)
		{
			pMono->Out("NetHall: Warning -read 0 bytes from netcamp.dat");
			SDL_CloseIO(campFile);
			return false;
		}

		pCamp++;
	}


	if(pHead->entries > 1)
		qsort((void*)pCampArr,pHead->entries,sizeof(NetCampaign), NetCompareCamp);

	campCnt = pHead->mostRecent;	//default

	SDL_CloseIO(campFile);
	return(true);
}


void
NetChar::ConfigLoadArrowButtons()
{
	if(!campCnt)
	{
		//leftLoadB.Hide(); don't work
		leftLoadB.SetCel(3); //show "background"
		leftLoadB.Listen(false);
		leftLoadB.Draw();
	}
	else
	{
		//leftLoadB.Show();
		leftLoadB.Select(false);
		leftLoadB.Listen(true);
	}

	if(campCnt == (header.entries-1))
	{
		//rightLoadB.Hide();
		rightLoadB.SetCel(3); //show "background"
		rightLoadB.Listen(false);
		rightLoadB.Draw();
	}
	else
	{
		//rightLoadB.Show();
		rightLoadB.Select(false);
		rightLoadB.Listen(true);
	}
}


void
NetChar::GetLoadCharStats()
{
	//set some values to zero
	pCamp = &pCampArr[campCnt];

	bGlobal.curCharId    = pCamp->curCharId;
	bGlobal.curCoverAnim = pCamp->curCover;
	bGlobal.curFaceAnim  = pCamp->curFace;
 	bGlobal.curBodyAnim  = pCamp->curBody;
	bGlobal.curBanner    = pCamp->curBanner;
	bGlobal.curCat       = pCamp->curCat;
	strcpy(bGlobal.curName,pCamp->curName);
}


void
NetChar::DisplayLoadChar()
{
	CopyCel(pbackLoadCH, 37,67,RES_ANIM,8154,3,true);	// Name Frame
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);
	//char name
	ASetString(40,71, bGlobal.curName, (uchar *)pbackLoadCH, 101, 0);

	CopyCel(pbackLoadCH,68, 95,RES_ANIM,bGlobal.curBodyAnim,1,true);	// Body
	CopyCel(pbackLoadCH,68, 95,RES_ANIM,bGlobal.curFaceAnim,1,true);	// Face
	CopyCel(pbackLoadCH,68, 95,RES_ANIM,bGlobal.curCoverAnim,1,true);	// Cover
	CopyCel(pbackLoadCH,68,171,RES_ANIM,9004,2,true,clut[bGlobal.curBanner/2]);	// banner 1

	SetEnemyBannerColor();
}

void
NetChar::ClearGlobalVars()
{
	bGlobal.missionsDone = 0;
	bGlobal.chooseSide = 0;

	bGlobal.replayMap = false;

	bGlobal.netWinner = false;
	bGlobal.netDisconnect = false;

	memset(bGlobal.curLevelScore,0,sizeof(bGlobal.curLevelScore));
	memset(bGlobal.lastLevelXP,0,sizeof(bGlobal.lastLevelXP));
	bGlobal.curCampaignScore = 0;
	bGlobal.curMaxCampaignScore = 0;

	memset(bGlobal.curXP, 0, sizeof(bGlobal.curXP));
	memset(bGlobal.unitsResearched, 0, sizeof(bGlobal.unitsResearched));
	memset(bGlobal.lastUnitsResearched, 0, sizeof(bGlobal.lastUnitsResearched));

	bGlobal.lawful  = bGlobal.lastLawful  = 0;
	bGlobal.neutral = bGlobal.lastNeutral = 0;
	bGlobal.chaotic = bGlobal.lastChaotic = 0;

	bGlobal.evil   = bGlobal.lastEvil   = 0;
	bGlobal.nutral = bGlobal.lastNutral = 0;
	bGlobal.good   = bGlobal.lastGood   = 0;
}
