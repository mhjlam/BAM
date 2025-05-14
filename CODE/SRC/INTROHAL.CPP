// INTROHAL.CPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
//
// Introduce choosen character into Hall of Legends.
//
// 10/28/95
//

#include "introhal.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "context.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <string.h>

#define HALL_BANNER_ANIM	9004
#define HALL_BANNER_SLASH	2
#define HALL_BANNER_DOTS	3


//=========================================================

IntroHall::IntroHall()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
}

IntroHall::~IntroHall()
{
}

void
IntroHall::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;

	pBam->voiceChains = 0;

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("IntroHall: background cel");
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackCH,0,0,RES_ANIM,8050,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	//pal.FadeToBlack();
	pal.Load(8050);

	//=================
	// Write the necessary text into the background

	char		tmpStr[360] = "";
	char		*pTxt;
	int		color,destRange;


	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK); //CARVED GREEN

	pTxt = sqbHall.Load(HALL_SQB,1);	//'HALL OF LEGENDS'
	ASetString(0,9, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);

	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);
	ASetString(40,43,bGlobal.curName, pback, 240, NULL, DG_JUST_CENTER);

	GetAlignment(&sqbRes,&sqbNum);
	pTxt = sqbHall.Load((uint)sqbRes,sqbNum);
	strcpy(tmpStr,pTxt);
	strcat(tmpStr," ");

	GetRank(&sqbRes,&sqbNum,0);
	pTxt = sqbHall.Load((uint)sqbRes,sqbNum);
	strcat(tmpStr,pTxt);

	//write rank
	ASetString(40,66,tmpStr, pback, 240, NULL, DG_JUST_CENTER);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	pTxt = sqbHall.Load(HALL_SQB,20);	//first half of text
	strcpy(tmpStr,pTxt);
	strcat(tmpStr," ");
	strcat(tmpStr,bGlobal.curName);

	pTxt = sqbHall.Load(HALL_SQB,21);	//second half of text
	strcat(tmpStr,pTxt);


	ASetString(11,286, tmpStr, pback, 293, NULL);


	//======================================================
	//Load hall,column, and column shadow cels

	CopyCel(pbackCH,0,94,RES_ANIM,8052,1,FALSE);	// Left Hall
	CopyCel(pbackCH,160,94,RES_ANIM,8052,2,FALSE);	// Right Hall

	//copy both columns w/shadows
	CopyCel(pbackCH,12 ,94,RES_ANIM,8052,3,FALSE);	// Left Column
	CopyCel(pbackCH,254,94,RES_ANIM,8052,4,FALSE);	// Right Column
	CopyCel(pbackCH,0  ,94,RES_ANIM,8052,5,FALSE);	// Left shadow
	CopyCel(pbackCH,242,94,RES_ANIM,8052,6,FALSE);	// Right shadow

	//--------------------
	//Load portrait
	CopyCel(pbackCH,140,118,RES_ANIM,bGlobal.curBodyAnim,1,TRUE);	// Body
	CopyCel(pbackCH,140,118,RES_ANIM,bGlobal.curFaceAnim,1,TRUE);	// Face
	CopyCel(pbackCH,140,118,RES_ANIM,bGlobal.curCoverAnim,1,TRUE);	// Cover

	//--------------------
	//Load banner
	color = bGlobal.curBanner / 2;

	destRange = 192;
	pal.LoadPartial(9101,192+(color*CLUT_SIZE),CLUT_SIZE,destRange);
	pal.UpdateColors(destRange,destRange+8);

	CopyCel(pbackCH,140,196,RES_ANIM,HALL_BANNER_ANIM,HALL_BANNER_SLASH,TRUE);

	//lets copy disabled button cel over background which make it look
	//like a 'delete' button is present
	CopyCel(pbackCH,262,236,RES_ANIM,8054,3,TRUE);

	pBam->voice1.Stop();
	pBam->LaunchVoice(HALL_SQB,20);

	//======================================================
	// lets setup up all the buttons

	pButton = &diskButton;
	pButton->Create(0, 236, 200, RES_ANIM, 129, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel


	pButton = &exitButton;
	pButton->Create(291, 236, 200, RES_ANIM, 8054, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbHall.Load(HALL_SQB,10);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


bool
IntroHall::HandleMsg(Message* pMsg)
{
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
						pMono->Out("\nDisk Button was hit");
						Option(pal.gSelf);
						return(TRUE);
					}
					else if(pMsg->notice.gSource == exitButton.gSelf)
					{
						pMono->Out("\nExit Button was hit");
						pBam->voice1.Stop();
						bGlobal.roomMgr.NewRoom(BR_STORY);
						return(TRUE);
					}
				}
			}
			break;
		case MSG_EVENT:
			if (pMsg->event.type == E_KEY_DOWN)
			{
				switch (pMsg->event.value)
				{
					//pass these keys thru
					case K_F1:
					case K_F2:
					case K_F3:
					case K_F4:
					case K_F5:
					case K_F6:
					case K_F7:
					case K_F8:
					case K_F9:
					case K_F10:
					case K_F11:
					case K_F12:
					case K_X:
						return FALSE;
						break;
				}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


void
IntroHall::Cycle()
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

	BAM_Room::Cycle();
}


