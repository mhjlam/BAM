// DEATH.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Show death cinematic and options.
//
// 1/26/95
//

#include "death.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "context.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include "option3.hpp"
#include "savemenu.hpp"


#define MM_MAIN_SQB		7010

//=========================================================

Death::Death()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
	launchMusic = 0;
}

Death::~Death()
{
}

void
Death::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;
	int			portNum;
	char			mess[80];
	char			*pDeathText;
	char			*pButtonText;
	//char			deathText[50];


	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	pMouse->SetPos(265,155);

	//======================================================
	// setup background cel filled with black
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	gbackAnim = ALoad(RES_ANIM,8200);
	CopyCel(pbackCH,0,0,RES_ANIM,8200,1,false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	//===============================================================
	
	if(bGlobal.storyLine == LEGEND)
	{
		//======================LEGENDARY CAMPAIGN=========================
		deathSqbNum = 100;	//has special conditions below
		portNum = 3820;

		switch(bGlobal.missionsDone % 3)
		{
			case 1:
				musicNum = 7046;
				break;

			case 2:
				musicNum = 7082;
				break;

			default:
				musicNum = 7006;
		}
	}
	else
	{
		switch(atoi(pBam->scenarioName))
		{
			//=======================HOWL OF VENGEANCE====================
			//RATHGAR
			case 9110:
			case 9130:
			case 9150:
				deathSqbNum = 1;
				portNum = 3535;
				musicNum = 7006;
				break;
			//AELRIC
			case 9155:
			case 9135:
			case 9115: 
				deathSqbNum = 2;
				portNum = 3560;
				musicNum = 7006;
				break;
			//=======================MATCHMAKER MAYHEM=====================
			//BRYAN
			case 9170: 
			case 9190:
			case 9210:
				deathSqbNum = 3;
				portNum = 3602;
				musicNum = 7026;
				break;
			//ROXANNE
			case 9215:
			case 9195:
			case 9175:
				deathSqbNum = 4;
				portNum = 3620;
				musicNum = 7026;
				break;
			//=======================TARTARUS UNBOUND======================
			//LADY AND LORDS
			case 9230:
			case 9250:
			case 9270:
				deathSqbNum = 5;
				portNum = 3660;
				musicNum = 7046;
				break;
			//TARTARUS
			case 9275:
			case 9255:
			case 9235:
				deathSqbNum = 6;
				portNum = 3678;
				musicNum = 7046;
				break;
			//======================NUTS AND BOLTS=========================
			//GARRULOS
			case 9290:
			case 9310:
			case 9330:
				deathSqbNum = 7;
				portNum = 3710;
				musicNum = 7066;
				break;
			//WORMSKULL
			case 9335:
			case 9315:
			case 9295:
				deathSqbNum = 8;
				portNum = 3730;
				musicNum = 7066;
				break;
			//======================HARVEST OF HORRORS=========================
			case 9350:
			case 9370:
			case 9390:
				deathSqbNum = 9;
				portNum = 3770;
				musicNum = 7082;
				break;

			case 9355:
			case 9375:
			case 9395:
				deathSqbNum = 10;
				portNum = 3795;
				musicNum = 7082;
				break;

		}
	}

	//pal.FadeToBlack();
	pal.Load(portNum);

		if(bGlobal.storyLine == LEGEND)
		{
			LoadUnitColors(pal.gSelf,pBam->sideColors[pBam->playerSide],pBam->sideColors[(pBam->playerSide == SIDE1)? SIDE2: SIDE1]);
		}

	if(AExists(RES_ANIM,portNum))
	{
		CopyCel(pbackCH,45,82,RES_ANIM,portNum,1,false);
	}
	else
	{
		snprintf(mess, sizeof(mess),"\nDeath: Resource %d not found!!!!!!!!!!!!!!!!!!!\n",portNum);
		pMono->Out(mess);
	}

	//=================
	// Write the necessary text into the background

	pDeathText = sqbDeath.Load(DEATH_SQB,deathSqbNum);

	// Write screen 'title'
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,141,139,184,185,183,CI_BLACK);

	pFontMgr->SetString(0,330, pDeathText, (uchar *)pbackCH, pbackCH->width, 0, DG_JUST_CENTER);

	GetStoryName(&sqbRes,&sqbNum,bGlobal.storyLine);
	pDeathText = sqbDeath.Load(sqbRes,sqbNum);

	SetFontColors(CI_SKIP,132,134,135,183,241,CI_BLACK);
	pFontMgr->SetString(0,372, pDeathText, (uchar *)pbackCH, pbackCH->width, 0, DG_JUST_CENTER);

	// setup guys with skip to write into
	// these 'backgrounds' will by default set the margins for proper word wrap
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);

	//======================================================
	// lets setup up all the buttons

	pButton = &replayButton;
	pButton->Create(185, 149, 200, RES_ANIM, 8202, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButtonText = sqbButton.Load(OPTION_SQB,3);
	pButton->SetCelText(1, pButtonText);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pButtonText);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(true);	//turn on click response -already in create()

	pButton = &loadButton;
	pButton->Create(185, 166, 200, RES_ANIM, 8204, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButtonText = sqbButton.Load(OPTION_SQB,2);
	pButton->SetCelText(1, pButtonText);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pButtonText);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(true);	//turn on click response -already in create()

	pButton = &newStoryButton;
	pButton->Create(185, 183, 200, RES_ANIM, 8202, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		if(bGlobal.storyLine == LEGEND)
			pButtonText = sqbButton.Load(OPTION_SQB,56);	//"NEW CAMPAIGN"
		else
			pButtonText = sqbButton.Load(OPTION_SQB,4);	//"NEW STORY"
	pButton->SetCelText(1, pButtonText);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pButtonText);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(true);	//turn on click response -already in create()

	pButton = &leaveButton;
	pButton->Create(185, 200, 200, RES_ANIM, 8204, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButtonText = sqbButton.Load(OPTION_SQB,8);
	pButton->SetCelText(1, pButtonText);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pButtonText);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(true);	//turn on click response -already in create()

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


void	
Death::Cleanup()
{
	pBam->voice1.Stop();

		pDigiMusic->Stop();
}

bool
Death::HandleMsg(Message* pMsg)
{
	char		mess[100];
	SubOptionMenu	*pSub;
	SaveMenu			*pSave;
	BAM_ErrorPopup	*pPop;

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == replayButton.gSelf)
					{
						pMono->Out("\nreplayButton was hit");
						pSub = new SubOptionMenu;
						pSub->Setup(grip{},REPLAY_BUTTON);
						return(true);
					}
					else if(pMsg->notice.gSource == loadButton.gSelf)
					{

							pMono->Out("\nloadButton was hit");
							pSave = new SaveMenu;
							pSave->Setup(grip{},LOAD_BUTTON);
						return(true);
					}
					else if(pMsg->notice.gSource == newStoryButton.gSelf)
					{
						pMono->Out("\nnew story button was hit");
						pSub = new SubOptionMenu;
						pSub->Setup(grip{},NEW_STORY_BUTTON);
						return(true);
					}
					else if(pMsg->notice.gSource == leaveButton.gSelf)
					{
						pMono->Out("\nleaveButton was hit");
						pSub = new SubOptionMenu;
						pSub->Setup(grip{},LEAVE_BUTTON);
						return(true);
					}
				}
				else
         	{
            	snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
            	APanic(mess);
         	}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}



void
Death::Cycle()
{
	if(!launchMusic)
	{
		launchMusic++;

			pDigiMusic = pBam->FindAvailTSound();
			pDigiMusic->Play(musicNum,MAX_VOLUME,DIGI_PAN_CENTER,grip{},0,false);

		pBam->voice1.Stop();
		pBam->voice1.Play((DEATH_SQB * 10000) + deathSqbNum, MAX_VOLUME, MkPan(100, 100), grip{}, 1);
	}

	BAM_Room::Cycle();
}
	
