// WINLOSE2.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen to handle victory/defeat scenario.
//
// 1/24/95
//

#include "api.hpp"
#include "apires.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "context.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "scrimage.hpp"

#include "WINLOSE2.HPP"
#include "alldefs.hpp"
#include "bam.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "savebase.hpp"

#include <string.h>

#define	WINLOSE2_BASE_PRI	20010
#define	WINLOSE2_WIN_X		20
#define	WINLOSE2_WIN_Y		38


//=========================================================

VictoryDefeat::VictoryDefeat()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
}

VictoryDefeat::~VictoryDefeat()
{
	if(prevFont != 9050)
		pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}

bool
VictoryDefeat::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&winlose2DataStart, 1, (int)&winlose2DataEnd -
				(int)&winlose2DataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&winlose2DataStart, 1, (int)&winlose2DataEnd -
				(int)&winlose2DataStart, pFile);
			break;
	}
	return(TRUE);
}

void
VictoryDefeat::Setup(grip gPal,bool victory_P)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			portNum;
	char			winStr[120];
	char			loseStr[120];

	//======================================================

	victory = victory_P;

	//set global used IF a netgame
	//victor and loser will both go to assess room
	bGlobal.netWinner = victory_P;
		

	//======================================================
	prevFont = pFontMgr->curFontNum;
	if(prevFont != 9050)
		pFontMgr->SetRes(9050);

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start Option::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		SetBasicNet(TRUE);

	mode = M_MODAL;


	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	TRACK_MEM("Option: background anim");
	gbackAnim = ALoad(RES_ANIM,50);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(WINLOSE2_WIN_X+0,WINLOSE2_WIN_Y+0,WINLOSE2_WIN_X+pbackAnimCH->width-1,WINLOSE2_WIN_Y+pbackAnimCH->height-1);

	// setup background cel filled with black
	TRACK_MEM("Option: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,WINLOSE2_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,50,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(WINLOSE2_WIN_X,WINLOSE2_WIN_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WINLOSE2_BASE_PRI);

	pGuy = &topBorder;
	pGuy->SetRes(RES_ANIM,116,5);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WINLOSE2_BASE_PRI-20);

	pGuy = &sideBorder;
	pGuy->SetRes(RES_ANIM,118,1);
	pGuy->SetPos(0,25);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WINLOSE2_BASE_PRI-19);

	pMouse->SetLimits(&rback);

	//need to get these new interface pieces onto the screen BEFORE
	//the do the screen capture and fade down
	AAnimate();

	//======================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	fadeTo.Setup(320,400,WINLOSE2_BASE_PRI - 10,gSelf,gPal,0,&rback);

	//===============================================================
	//copy borders into background cel
	CopyCel(pbackCH,35, 77,RES_ANIM,50,2,TRUE);	// Portrait Border 1
	CopyCel(pbackCH,64,212,RES_ANIM,50,3,TRUE);	// 'done' button frame

	int		sqbWinNum,sqbLoseNum;
	
	if(bGlobal.storyLine == NETGAME)
	{
		sqbWinNum = 101;
		sqbLoseNum = sqbWinNum + 1;

		ReplaceName(winStr,WINLOSE2_SQB,sqbWinNum,TRUE);
		ReplaceName(loseStr,WINLOSE2_SQB,sqbLoseNum,FALSE);

		//Load portrait
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curBodyAnim,1,TRUE);	// Body
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curFaceAnim,1,TRUE);	// Face
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curCoverAnim,1,TRUE);	// Cover
	}
	else
	if(bGlobal.storyLine == LEGEND)
	{
		//get phrase pairs for this map
		sqbWinNum = (bGlobal.missionsDone * 2) + 1;
		sqbLoseNum = sqbWinNum + 1;

		pTxt = sqbVD.Load(WINLOSE2_SQB,sqbWinNum);
		strcpy(winStr,pTxt);
		pTxt = sqbVD.Load(WINLOSE2_SQB,sqbLoseNum);
		strcpy(loseStr,pTxt);

		//Load portrait
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curBodyAnim,1,TRUE);	// Body
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curFaceAnim,1,TRUE);	// Face
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curCoverAnim,1,TRUE);	// Cover
	}
	else
	{	
		//get phrase pairs for this map
		sqbWinNum = atoi(pBam->scenarioName);
		sqbLoseNum = sqbWinNum + 1;

		pTxt = sqbVD.Load(WINLOSE2_SQB,sqbWinNum);
		strcpy(winStr,pTxt);
		pTxt = sqbVD.Load(WINLOSE2_SQB,sqbLoseNum);
		strcpy(loseStr,pTxt);

		portNum = GetPortraitAnim(bGlobal.chooseSide,TRUE);
		if(victory)
			CopyCel(pbackCH,37,80,RES_ANIM,portNum,3,TRUE);
		else
			CopyCel(pbackCH,37,80,RES_ANIM,portNum,2,TRUE);
	}


	pBam->voice1.Stop();
	if(bGlobal.storyLine != NETGAME)
	{
		if(victory)
			pBam->voice1.Play(WINLOSE2_SQB * 10000 + sqbWinNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);
		else
			pBam->voice1.Play(WINLOSE2_SQB * 10000 + sqbLoseNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);
	}

	//===========================================================
	pButton = &buttonDone;
	pButton->Create(WINLOSE2_WIN_X+66, WINLOSE2_WIN_Y+215, WINLOSE2_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, WINLOSE2_WIN_X, WINLOSE2_WIN_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetTextOffSet(0, 0);
	pTxt = sqbVD.Load(OPTION_SQB,22);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(TRUE);	//turn on click response -already in create()


	//==================================================

	SetFontColors(CI_SKIP,94,102,94,102,156,144);

	int savePoint;
	savePoint = pFontMgr->point;
	pFontMgr->point = 14;

	if(victory)
	{
		ASetString(85, 90, winStr, pback,102, NULL);
	}
	else
	{
		ASetString(85, 90, loseStr, pback,102, NULL);
	}

	pFontMgr->point = savePoint;

	//since we had to make a special call to AAnimate above
	//we need to re-update the rect now that we've written our text
	AUpdateRect(&rback);

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;									// grip of currently active button, if any

}


bool
VictoryDefeat::HandleMsg(Message* pMsg)
{
	char			mess[100];

	// pass on to receivers first
	if (BAM_Room::HandleMsg(pMsg))
	{
		return(TRUE);
	}
	else
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
						if(pMsg->notice.gSource == buttonDone.gSelf)
						{
							pMono->Out("\nDone Button was hit");
							Cleanup();

							if (!victory && bGlobal.storyLine != NETGAME)
								bGlobal.roomMgr.NewRoom(BR_DEATH);
							else
								bGlobal.roomMgr.NewRoom(BR_ASSESS);
							return TRUE;
						}
						else
						{
            			sprintf(mess, "Unrecognized button type: %d", pMsg->notice.param);
            			APanic(mess);
						}
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
					case E_MOUSE_DOWN:
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
								return(TRUE);
						}
						break;

					case E_KEY_DOWN:
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
								return pBam->HandleMsg(pMsg);
								break;
							case K_X:
								if(pMsg->event.modifiers & MOD_ALT)
								{
									Cleanup();
									pContextMgr->Quit();
									return TRUE;
								}
								break;
							case K_ESC:
							case K_Q:
							case K_C:
								Cleanup();

								if (!victory && bGlobal.storyLine != NETGAME)
									bGlobal.roomMgr.NewRoom(BR_DEATH);
								else
									bGlobal.roomMgr.NewRoom(BR_ASSESS);

								return TRUE;
								break;
						}
				}
				break;
		}//endswitch
	}
	return TRUE;	//don't pass-thru
}


void
VictoryDefeat::Cleanup()
{
	fadeTo.FadeUp();

	Activate(FALSE);

	delete this;

	pBam->voice1.Stop();
}


void
VictoryDefeat::ReplaceName(char *destStr, int res, int num, bool myNameFirst)
{
	char	*pDest,*pSrc;
	char	srcStr[300];
	int	nameCnt;

	if(myNameFirst)
		nameCnt = 1;
	else
		nameCnt = 0;

	pTxt = sqbVD.Load(res,num);
	strcpy(srcStr,pTxt);
	strcpy(destStr,pTxt);

	pSrc = strstr(srcStr, "(" );

	if(pSrc)
	{
		while(pSrc)
		{
			//we have some name replacing to do

			//find same parenth in dest
			pDest = strstr(destStr, "(" );
			//trunc string at start parenth
			*pDest = '\0';

			//cat in either my name or enemy name in alternating pattern
			if(nameCnt % 2)
				strcat(destStr,bGlobal.curName);
			else
				strcat(destStr,bGlobal.enemyName);

			//let remove leading parenth from src
			*pSrc = 'x';

			//now lets find matching parenth in src and remove
			pSrc = strstr(srcStr, ")" );
			*pSrc = 'x';
			pSrc++;

			//cat in the rest of src
			strcat(destStr,pSrc);

			//any more '(' ?
			pSrc = strstr(srcStr, "(" );
			nameCnt++;
		}

	}
}
