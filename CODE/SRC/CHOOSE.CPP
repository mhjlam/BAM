// CHOOSE.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Choose one of two characters in a storyline.
//
// 12/12/94
//

#include "choose.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "context.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <string.h>


//=========================================================

Choose::Choose()
{
	int	i;

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;

	gback = 0;

	for(i = 0; i < 2; i++)
	{
		TRACK_MEM("Choose Clut");	clut[i] = AMalloc(CLUT_SIZE);
	}
}

Choose::~Choose()
{
	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);
	int	i;

	for(i = 0; i < 2; i++)
	{
		AFree(clut[i]);
		clut[i] = 0;
	}
}

void
Choose::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;
	int			x,y;
	int			sqbDescA,sqbDescB;
	int			sqbStoryRes,sqbStoryNum;
	uchar 		*pClut1;
	uchar 		*pClut2;

	pBam->voiceChains = 0;

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("Choose: background cel");
	gback = ACreateCel(&resNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	TRACK_MEM("Choose: background anim");
	gbackAnim = ALoad(RES_ANIM,8150);
	CopyCel(pbackCH,0,0,RES_ANIM,8150,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, resNumBack, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	//pal.FadeToBlack();
	pal.Load(8150);
	//setup unit colors for this room -unit color will be reset based on player's choice
	GetUnitColors(&pBam->sideColors[SIDE1],&pBam->sideColors[SIDE2],bGlobal.storyLine);
	LoadUnitColors(pal.gSelf,pBam->sideColors[SIDE1],pBam->sideColors[SIDE2]);

	sqbDescB = (bGlobal.storyLine*2);
	sqbDescA = sqbDescB - 1;

	portraitA = GetPortraitAnim(1);
	portraitB = GetPortraitAnim(2);
	GetCharName(&sqbCharRes,&sqbCharNumA,bGlobal.storyLine,1);
	GetCharName(&sqbCharRes,&sqbCharNumB,bGlobal.storyLine,2);
	GetStoryName(&sqbStoryRes,&sqbStoryNum,bGlobal.storyLine);

	//======================================================
	// lets copy in all the borders and boundaries into the background

	TRACK_MEM("Choose: Portrait Border");
	CopyCel((CelHeader*)pback, 67,82,RES_ANIM,8154,1,TRUE);	// Portrait Border 1
	CopyCel((CelHeader*)pback,211,82,RES_ANIM,8154,1,TRUE);	// Portrait Border 2
	CopyCel((CelHeader*)pback,151,51,RES_ANIM,8154,2,TRUE);	// Screen Divider
	CopyCel((CelHeader*)pback,21,281,RES_ANIM,8154,10,TRUE);	// Option Button Frame

	//===============================
	//setup cluts for pen guys
	//we know player's color (SIDE1) has been load into first clut range

	pClut1 = ADerefAs(uchar, clut[0]);
	pClut2 = ADerefAs(uchar, clut[1]);

	for(x=0;x<CLUT_SIZE;x++)
	{
		pClut1[x] = (uchar)(CLUT1_START + x);
		pClut2[x] = (uchar)(CLUT2_START + x);
	}

	TRACK_MEM("Choose: Banner anim");
	gbanner = ALoad(RES_ANIM,9004);
	CopyCel((CelHeader*)pback, 69,162,RES_ANIM,9004,2,TRUE,clut[0]);	// banner 1
	CopyCel((CelHeader*)pback,213,162,RES_ANIM,9004,3,TRUE,clut[1]);	// banner 2

	//=================
	// Write the necessary text into the background

	// Write screen 'title'
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);
	pTxt = sqbChoose.Load(CHOOSE_SQB,11); //"choose your mage"
	pFontMgr->SetString(0,330, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);

	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbChoose.Load(sqbStoryRes,sqbStoryNum);
	pFontMgr->SetString(0,372, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);

	// setup guys with skip to write into
	// these 'backgrounds' will by default set the margins for proper word wrap
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);

	y = (bGlobal.storyLine == STORYLINE4)? 55 : 61;

	pTxt = sqbChoose.Load(sqbCharRes,sqbCharNumA);
	ASetString(23,y, pTxt, (uchar *)pbackCH, 130, NULL, DG_JUST_CENTER);

	pTxt = sqbChoose.Load(sqbCharRes,sqbCharNumB);
	ASetString(172,y, pTxt, (uchar *)pbackCH, 130, NULL, DG_JUST_CENTER);

	//Char Desc A
	pTxt = sqbChoose.Load(CHOOSE_SQB,sqbCharNumA);
	ASetString(29,189, pTxt, (uchar *)pbackCH, 125, NULL);

	//Char Desc B
	pTxt = sqbChoose.Load(CHOOSE_SQB,sqbCharNumB);
	ASetString(172,189, pTxt, (uchar *)pbackCH, 125, NULL);

	pBam->LaunchVoice(CHOOSE_SQB,11);

	//======================================================
	// lets setup up all the buttons

	pButton = &diskButton;
	pButton->Create(23, 285, 200, RES_ANIM, 129, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(resNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &portButton1;
	pButton->Create(69, 85, 200, RES_ANIM, portraitA, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(resNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
	pButton->SetButtonRes(TRUE,RES_ANIM,104,1);

	pButton = &portButton2;
	pButton->Create(213, 85, 200, RES_ANIM, portraitB, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(resNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
	pButton->SetButtonRes(TRUE,RES_ANIM,104,1);

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


bool
Choose::HandleMsg(Message* pMsg)
{
	char		mess[100];

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == diskButton.gSelf)
					{
						pMono->Out("\nDiskButton was hit");
						Option(pal.gSelf);
						return(TRUE);
					}
					else if(pMsg->notice.gSource == portButton1.gSelf)
					{
						pMono->Out("\nPortButton1 was hit");
						pBam->playerSide = SIDE1;

						bGlobal.chooseSide = 1;
						pTxt = sqbChoose.Load(sqbCharRes,sqbCharNumA);
						strcpy(bGlobal.curName,pTxt); //set global
						//so set player(SIDE1) to color1
						GetUnitColors(&pBam->sideColors[SIDE1],&pBam->sideColors[SIDE2],bGlobal.storyLine);

						bGlobal.cinematic = GetCine(bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);
						GetScenario(pBam->scenarioName,bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);

						pBam->voice1.Stop();
						bGlobal.roomMgr.NewRoom(BR_CINE);
						return(TRUE);
					}
					else if(pMsg->notice.gSource == portButton2.gSelf)
					{
						pMono->Out("\nPortButton2 was hit");
						pBam->playerSide = SIDE1;

						bGlobal.chooseSide = 2;
						pTxt = sqbChoose.Load(sqbCharRes,sqbCharNumB);
						strcpy(bGlobal.curName,pTxt);
						//so set player(SIDE1) to color2
						GetUnitColors(&pBam->sideColors[SIDE2],&pBam->sideColors[SIDE1],bGlobal.storyLine);

						bGlobal.cinematic = GetCine(bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);
						GetScenario(pBam->scenarioName,bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);

						pBam->voice1.Stop();
						bGlobal.roomMgr.NewRoom(BR_CINE);
						return(TRUE);
					}

				}
				else
         	{
            	sprintf(mess, "Unrecognized button notice reply: %d", pMsg->notice.param);
            	APanic(mess);
         	}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


void
Choose::Cycle()
{
	coord theX = pMouse->GetX();
	coord theY = pMouse->GetY();

	if(pBam->voiceChains && !pBam->voice1.IsPlaying())
	{
		while(pBam->voiceChains > 1)
		{
			//lets reduce the queue to size one -take the last entry
			memmove(&pBam->rVoiceChains[0][0], &pBam->rVoiceChains[1][0],
				sizeof(pBam->rVoiceChains[0][0]) * pBam->voiceChains * 3);
			pBam->voiceChains--;
		}

		if(pBam->rVoiceChains[0][0] != lastVoiceNum)
		{
			// speech! speech!
			pBam->voice1.Play(pBam->rVoiceChains[0][0], MAX_VOLUME, MkPan(100, 100), NULL, 1);
			if(pBam->rVoiceChains[0][1])
				pBam->voice1.AddToChain(pBam->rVoiceChains[0][1]);
			if(pBam->rVoiceChains[0][2])
				pBam->voice1.AddToChain(pBam->rVoiceChains[0][2]);
			lastVoiceNum = pBam->rVoiceChains[0][0];
		}
		pBam->voiceChains--;
	}

	if(portButton1.scrim.rect.Contains(theX,theY))
		pBam->LaunchVoice(sqbCharRes,sqbCharNumA);

	if(portButton2.scrim.rect.Contains(theX,theY))
		pBam->LaunchVoice(sqbCharRes,sqbCharNumB);

	BAM_Room::Cycle();
}
