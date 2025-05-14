// WINLOSE.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen to handle win/lose conditions.
//
// 1/23/95
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

#include "bam.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "story.hpp"
#include "WINLOSE.HPP"
#include "world.hpp"

#include <string.h>

#define	WINLOSE_BASE_PRI	20000
#define	WINLOSE_WIN_X		20
#define	WINLOSE_WIN_Y		38

//=========================================================

WinLose::WinLose()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	commentCnt = 1;
}

WinLose::~WinLose()
{
	pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}

void
WinLose::Setup(grip gPal,bool fadeup)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			i,portNum;

	prevFont = pFontMgr->curFontNum;
	pFontMgr->SetRes(9050);

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start WinLose::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		mode = M_MODELESS;
	else
		mode = M_MODAL;

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	TRACK_MEM("WinLose: background anim");
	gbackAnim = ALoad(RES_ANIM,50);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	// os-mac stuff is for this screen to be centered. since
	// the blue/grey fadeTo doesn't work the first time in and
	// it looks funny to have the screen in the top left
	#ifdef OS_MAC
		extern bool startup_game;
		int xCoord;
		int yCoord;
		if (startup_game)
		{
			xCoord = 160 - (pbackAnimCH->width/2);
			yCoord = 200 - (pbackAnimCH->height/2);
			rback.Set(xCoord,yCoord,xCoord-1+pbackAnimCH->width,yCoord-1+pbackAnimCH->height);
		}
		else
			//rback.Set(0,0,pbackAnimCH->width-1,pbackAnimCH->height-1);
			rback.Set(WINLOSE_WIN_X+0,WINLOSE_WIN_Y+0,WINLOSE_WIN_X+pbackAnimCH->width-1,WINLOSE_WIN_Y+pbackAnimCH->height-1);
	#else
		//rback.Set(0,0,pbackAnimCH->width-1,pbackAnimCH->height-1);
		rback.Set(WINLOSE_WIN_X+0,WINLOSE_WIN_Y+0,WINLOSE_WIN_X+pbackAnimCH->width-1,WINLOSE_WIN_Y+pbackAnimCH->height-1);
	#endif

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("WinLose: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,WINLOSE_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,50,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	#ifdef OS_MAC
		if(startup_game)
			pGuy->SetPos(xCoord,yCoord);
		else
			pGuy->SetPos(WINLOSE_WIN_X,WINLOSE_WIN_Y);
	#else
		pGuy->SetPos(WINLOSE_WIN_X,WINLOSE_WIN_Y);
	#endif
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WINLOSE_BASE_PRI);

	pGuy = &topBorder;
	pGuy->SetRes(RES_ANIM,116,5);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WINLOSE_BASE_PRI-20);

	pGuy = &sideBorder;
	pGuy->SetRes(RES_ANIM,118,1);
	pGuy->SetPos(0,25);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(WINLOSE_BASE_PRI-19);

	pMouse->SetLimits(&rback);

	//need to get these new interface pieces onto the screen BEFORE
	//the do the screen capture and fade down
	AAnimate();

	//======================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	fadeTo.Setup(320,400,WINLOSE_BASE_PRI - 10,gSelf,gPal,0,&rback);

	//===============================================================
	char		winText[100],loseText[100];
	int		winNum,loseNum;

	sqbRes = WINLOSE_SQB;

	// load generic "front-end" used in all phrases
	pTxt = sqbWinLose.Load(sqbRes,1);
	strcpy(winText,pTxt);
	pTxt = sqbWinLose.Load(sqbRes,2);
	strcpy(loseText,pTxt);

	struct WinningCondition	*pWinCon;

	//defaults
	winNum  = atoi(pBam->scenarioName);
	loseNum = winNum + 1;

	if((bGlobal.storyLine == LEGEND && winNum != 9410) ||
		bGlobal.storyLine == NETGAME)
	{
		winNum  = 10;
		loseNum = 20;

		for(i=0;i<MAX_WINCONS;i++)
		{
			pWinCon = &pWorld->winCons[i];

			if(pWinCon->targetSpec == BUILDING &&
				pWinCon->structSpec	== FUNC_KEEP)
			{
				if(pWinCon->ownerSide == pBam->playerSide)
					winNum  = 11;

				if(pWinCon->ownerSide == ((pBam->playerSide==SIDE1)?SIDE2:SIDE1))
					loseNum = 21;
			}
		}
	}

	pTxt = sqbWinLose.Load(sqbRes,winNum);
	strcat(winText," ");
	strcat(winText,pTxt);
	pTxt = sqbWinLose.Load(sqbRes,loseNum);
	strcat(loseText," ");
	strcat(loseText,pTxt);

	pBam->voiceChains = 0;
	pBam->voice1.Stop();
	pBam->LaunchVoice(WINLOSE_SQB,winNum,WINLOSE_SQB,loseNum);

	//==================================================
	SetFontColors(CI_SKIP,94,102,94,102,156,144);

	int savePoint;
	savePoint = pFontMgr->point;
	pFontMgr->point = 14;

	ASetString(85, 48, winText, pback,102, NULL);
	ASetString(85, 137, loseText,pback,102, NULL);

	pFontMgr->point = savePoint;

	//==================================================
	//copy borders into background cel
	if(bGlobal.storyLine != LEGEND && bGlobal.storyLine != NETGAME)
	{
		CopyCel(pbackCH,35, 35,RES_ANIM,50,2,TRUE);	// Portrait Border 1
		CopyCel(pbackCH,35,123,RES_ANIM,50,2,TRUE);	// Portrait Border 2
	}
	else
	{
		CopyCel(pbackCH,35,77,RES_ANIM,50,2,TRUE);	// Portrait Border 2
	}

	if(bGlobal.storyLine == STORYLINE4)
	{
		CopyCel(pbackCH,35,212,RES_ANIM,50,4,TRUE);	// 'Done' button frame
		CopyCel(pbackCH,92,212,RES_ANIM,50,3,TRUE);	// 'Undone' button frame
	}
	else
	{
		CopyCel(pbackCH,64,212,RES_ANIM,50,3,TRUE);	// 'Done' w/undone button frame
	}

	if(bGlobal.storyLine != LEGEND && bGlobal.storyLine != NETGAME)
	{
		portNum = GetPortraitAnim(bGlobal.chooseSide,TRUE);
		CopyCel(pbackCH,37,38,RES_ANIM,portNum,3,TRUE); //VICTORY
		CopyCel(pbackCH,37,126,RES_ANIM,portNum,2,TRUE); //DEFEAT
	}
	else
	{
		//Load portrait
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curBodyAnim,1,TRUE);	// Body
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curFaceAnim,1,TRUE);	// Face
		CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.curCoverAnim,1,TRUE);	// Cover
	}

	
	pButton = &buttonDone;
	if(bGlobal.storyLine == STORYLINE4)
		#ifdef OS_MAC
			if(startup_game)
				pButton->Create(37+xCoord, 215+yCoord, WINLOSE_BASE_PRI + 1, RES_ANIM, 8158, 1, gSelf, xCoord, yCoord);
			else
				pButton->Create(WINLOSE_WIN_X+37, WINLOSE_WIN_Y+215, WINLOSE_BASE_PRI + 1, RES_ANIM, 8158, 1, gSelf, WINLOSE_WIN_X, WINLOSE_WIN_Y);
		#else
			pButton->Create(WINLOSE_WIN_X+37, WINLOSE_WIN_Y+215, WINLOSE_BASE_PRI + 1, RES_ANIM, 8158, 1, gSelf, WINLOSE_WIN_X, WINLOSE_WIN_Y);
		#endif
	else
		#ifdef OS_MAC
			if(startup_game)
				pButton->Create(66+xCoord, 215+yCoord, WINLOSE_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, xCoord, yCoord);
			else
				pButton->Create(WINLOSE_WIN_X+66, WINLOSE_WIN_Y+215, WINLOSE_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, WINLOSE_WIN_X, WINLOSE_WIN_Y);
		#else
			pButton->Create(WINLOSE_WIN_X+66, WINLOSE_WIN_Y+215, WINLOSE_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, WINLOSE_WIN_X, WINLOSE_WIN_Y);
		#endif
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetTextOffSet(0, 0);
	if(bGlobal.storyLine == STORYLINE4)
	{
		pTxt = sqbWinLose.Load(OPTION_SQB,23);
		pButton->SetCelText(1, pTxt);
		pButton->SetCelText(2, pTxt);
	}
	else
	{
		pTxt = sqbWinLose.Load(OPTION_SQB,22);
		pButton->SetCelText(1, pTxt);
		pButton->SetCelText(2, pTxt);
	}
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);


	if(bGlobal.storyLine == STORYLINE4)
	{
		pButton = &buttonUndone;
		#ifdef OS_MAC
			if(startup_game)
				pButton->Create(94+xCoord, 215+yCoord, WINLOSE_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, xCoord, yCoord);
			else
				pButton->Create(WINLOSE_WIN_X+94, WINLOSE_WIN_Y+215, WINLOSE_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, WINLOSE_WIN_X, WINLOSE_WIN_Y);
		#else
			pButton->Create(WINLOSE_WIN_X+94, WINLOSE_WIN_Y+215, WINLOSE_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, WINLOSE_WIN_X, WINLOSE_WIN_Y);
		#endif
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pTxt = sqbWinLose.Load(OPTION_SQB,24);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);
	}


	//==================================================

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;									// grip of currently active button, if any

	//since we had to make a special call to AAnimate above
	//we need to re-update the rect now that we've written our text
	AUpdateRect(&rback);
	//AAnimate();

	if(fadeup)
	{
		TPalette *pPal = ADerefAs(TPalette,gPal);
		pPal->FadeUp();
	}

}


bool
WinLose::HandleMsg(Message* pMsg)
{
	char			mess[100];
	BAM_Button  *pButton;

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
							return TRUE;
						}
						else
						if(bGlobal.storyLine == STORYLINE4 && pMsg->notice.gSource == buttonUndone.gSelf)
						{
							pMono->Out("\nUndone Button was hit");
							switch(commentCnt)
							{
								case 0:
									pButton = &buttonUndone;
									pTxt = sqbWinLose.Load(OPTION_SQB,24);
									pButton->SetCelText(1, pTxt);
									pButton->SetCelText(2, pTxt);
									pButton->Draw();
									commentCnt++;
									return TRUE;
									break;
								case 1:
									pButton = &buttonUndone;
									pTxt = sqbWinLose.Load(OPTION_SQB,25);
									pButton->SetCelText(1, pTxt);
									pButton->SetCelText(2, pTxt);
									pButton->Draw();
									commentCnt++;
									return TRUE;
									break;
								case 2:
									pButton = &buttonUndone;
									pTxt = sqbWinLose.Load(OPTION_SQB,26);
									pButton->SetCelText(1, pTxt);
									pButton->SetCelText(2, pTxt);
									pButton->Draw();
									commentCnt++;
									return TRUE;
									break;
								case 3:
									pButton = &buttonUndone;
									pTxt = sqbWinLose.Load(OPTION_SQB,27);
									pButton->SetCelText(1, pTxt);
									pButton->SetCelText(2, pTxt);
									pButton->Draw();
									commentCnt = 0;
									return TRUE;
									break;
							}
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
								return TRUE;
								break;
						}
				}
				break;
		}//endswitch
	}
	return TRUE;	//don't pass thru
}


void
WinLose::Cleanup()
{
	fadeTo.FadeUp();
	bGlobal.roomMgr.curRoom->Pause(FALSE);

	Activate(FALSE);

	delete this;

	pBam->voiceChains = 0;
	pBam->voice1.Stop();

	#ifdef OS_MAC
		if(startup_game)
		{
			if (musNum != 1)
				pWorld->music1.Play((uint16) musNum);

			startup_game = FALSE;
		}
	#endif
}


void
WinLose::Cycle()
{
	BAM_Room::Cycle();

	if(pWorld->worldEnder)
		Cleanup();

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
}
