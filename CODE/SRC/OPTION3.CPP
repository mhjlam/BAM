// OPTION3.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A PopUp window which displays a message and gives a choice to accept.
//
// 11/16/94
//

#include "Option3.hpp"

#include "apievt.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "context.hpp"
#include "fontmgr.hpp"
#include "rect.hpp"
#include "option2.hpp"
#include "sconfig.hpp"
#include "scrimage.hpp"
#include "story.hpp"
#include "tigre.hpp"

#include <string.h>

#define	OPTION3_BASE_PRI	20000

#define	OPT3_WOFF_X	89
#define	OPT3_WOFF_Y	92

//=========================================================

SubOptionMenu::SubOptionMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
}

SubOptionMenu::~SubOptionMenu()
{
	pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);
	pMouse->ForceShow((int16)oldMouseHide);
}

void
SubOptionMenu::Setup(grip gPrevCon_P,option_t menuType_P)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			leaveNum;

	gPrevCon = gPrevCon_P;
	menuType = menuType_P;

	//ADump();
	//pMemMgr->Dump(1, "Start SubOptionMenu::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		mode = M_MODELESS;
	else
		mode = M_MODAL;

	prevFont = pFontMgr->curFontNum;
	pFontMgr->SetRes(9050);

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);
	oldMouseHide = pMouse->ForceShow(0);

	TRACK_MEM("Option3: background anim");
	gbackAnim = ALoad(RES_ANIM,40);
	pbackAnimCH = (CelHeader *)AGetResData(gbackAnim);
	rback.Set(OPT3_WOFF_X, OPT3_WOFF_Y, OPT3_WOFF_X-1+pbackAnimCH->width, OPT3_WOFF_Y-1+pbackAnimCH->height);

	// setup background cel filled with black
	TRACK_MEM("Option3: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,OPTION3_BASE_PRI + 10);
	pback = AGetResData(gback);
	pbackCH = (CelHeader *)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,40,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(OPT3_WOFF_X,OPT3_WOFF_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(OPTION3_BASE_PRI + 10);

	pMouse->SetLimits(&rback);

	//---------------------------------------

	//maxLeaveNum = atoi(sqbOption.Load(OPTION2_SQB,1));
	//if(!maxLeaveNum)
	//	APanic("Sqb Problem: 11002.1 is invalid");
	//leaveNum = ARandom(maxLeaveNum) + 2;
	leaveNum = 2;

	//---------------------------------------
	// Two Text boxes for OFF & MAX
	uchar	saveColorFore,saveColorFore1,saveColorBack;

	saveColorFore  = AFontColor(FNT_FORE_COLOR);
	saveColorFore1 = AFontColor(FNT_FORE_COLOR + 1);
	saveColorBack  = AFontColor(FNT_BACK_COLOR);

	ASetFontColor(FNT_FORE_COLOR,93);
	ASetFontColor(FNT_FORE_COLOR + 1,90);
	ASetFontColor(FNT_BACK_COLOR,CI_SKIP);

	int savePoint;
	savePoint = pFontMgr->point;
	pFontMgr->point = 14;
	switch(menuType)
	{
		case LEAVE_BUTTON:
			pTxt = sqbOption.Load(OPTION2_SQB,leaveNum);
			ASetString(20,68, pTxt, (uchar *)pbackCH, 140-20-20, NULL, DG_JUST_CENTER);
			break;

		case REPLAY_BUTTON:
			pTxt = sqbOption.Load(OPTION_SQB,34);
			ASetString(20,68, pTxt, (uchar *)pbackCH, 140-20-20, NULL, DG_JUST_CENTER);
			break;

		case NEW_STORY_BUTTON:
			#ifdef INTERACTIVE_DEMO
				pTxt = sqbOption.Load(OPTION_SQB,55);	  //"BEGIN A NEW SCENARIO?"
			#else
				if(bGlobal.storyLine == LEGEND)
					pTxt = sqbOption.Load(OPTION_SQB,57); //"BEGIN A NEW CAMPAIGN?"
				else
					pTxt = sqbOption.Load(OPTION_SQB,35); //"BEGIN A NEW STORY?"
			#endif
			ASetString(20,68, pTxt, (uchar *)pbackCH, 140-20-20, NULL,DG_JUST_CENTER);
			break;
	}
	pFontMgr->point = savePoint;

	//Reset color in which to draw names
	ASetFontColor(FNT_FORE_COLOR,saveColorFore);
	ASetFontColor(FNT_FORE_COLOR + 1,saveColorFore1);
	ASetFontColor(FNT_BACK_COLOR,saveColorBack);

	//---------------------------------------
	//Cap setups

	buttonY = 219;
	//remember to minus window offsets (OPT3_WOFF_X,OPT3_WOFF_Y).
	CopyCel(pbackCH,112-OPT3_WOFF_X,buttonY-OPT3_WOFF_Y,RES_ANIM,40,2,FALSE);
	buttonY += 3;

	//---------------------------------------
	//Button Setup for DONE button
	pButton = &button[0];
	pButton->Create(112, buttonY, OPTION3_BASE_PRI + 11, RES_ANIM, 42, 1, gSelf, OPT3_WOFF_X, OPT3_WOFF_Y);
	buttonY += 17;
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbOption.Load(OPTION_SQB,28);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(TRUE);	//turn on click response -already in create()

	//Button Setup for CANCEL button
	pButton = &button[1];
	pButton->Create(112, buttonY, OPTION3_BASE_PRI + 11, RES_ANIM, 44, 1, gSelf, OPT3_WOFF_X, OPT3_WOFF_Y);
	buttonY += 17;
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbOption.Load(OPTION_SQB,58);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(TRUE);	//turn on click response -already in create()

	//---------------------------------------
	//Cap setups
	CopyCel(pbackCH,112-OPT3_WOFF_X,buttonY-OPT3_WOFF_Y,RES_ANIM,40,3,FALSE);


	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void *)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;			// grip of currently active button, if any

}


bool
SubOptionMenu::HandleMsg(Message *pMsg)
{
	char		mess[100];
	BAM_Room *pRoom;

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
						if(pMsg->notice.gSource == button[0].gSelf)
						{
							pMono->Out("\nDone Button was hit");

							switch(menuType)
							{
								case LEAVE_BUTTON:
									pMono->Out("\nLeave Game confirmed.");
									Cleanup();

									//lets remove calling option window
									if (gPrevCon)
									{
										pRoom = ADerefAs(BAM_Room,gPrevCon);
										pRoom->Cleanup();
									}

									if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
									{
										bsPacket.header.ID = PLAYER_LEAVING;
										bsPacket.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player
										strcpy(mess,"goodbye world");
										bsPacket.header.len = strlen(mess);
										bsPacket.pData = mess;	//have it point to somethin
										pCommMgr->SendData(&bsPacket);
										bGlobal.netDisconnect = TRUE;

										//declare defeat
										pWorld->tWorldEnds = ATicks();
										pWorld->worldEnder = (pBam->playerSide == SIDE1)? SIDE2: SIDE1;
									}
									else
									{
										bGlobal.roomMgr.NewRoom(BR_MENU);
									}
									return TRUE;
									break;

								case REPLAY_BUTTON:
									pMono->Out("\nReplay confirmed.");
									Cleanup(); // This cleanup first to properly
												  // restore mouse boundaries.
									//lets remove calling option window
									if (gPrevCon)
									{
										pRoom = ADerefAs(BAM_Room,gPrevCon);
										pRoom->Cleanup();
									}

									//if called from assess room or after, then unincrement missionsDone
									if(bGlobal.storyLine == LEGEND)
									{
										if(bGlobal.roomMgr.curRoomNum == BR_ASSESS ||
											bGlobal.roomMgr.curRoomNum == BR_HALL)
										{
											bGlobal.missionsDone--;
											GetScenario(pBam->scenarioName,bGlobal.storyLine,bGlobal.missionsDone,bGlobal.prevChooseSide);
											bGlobal.curXP[pBam->playerSide] = bGlobal.lastLevelXP[0];
											memcpy(bGlobal.unitsResearched, bGlobal.lastUnitsResearched[0], UNITMAXTYPES);

											//these vars. were updated in assess room -must now reset them
											bGlobal.lawful	 = bGlobal.lastLawful;
											bGlobal.neutral = bGlobal.lastNeutral;
											bGlobal.chaotic = bGlobal.lastChaotic;

											bGlobal.evil   = bGlobal.lastEvil;
											bGlobal.nutral = bGlobal.lastNutral;
									 		bGlobal.good   = bGlobal.lastGood;
										}
										else
										{
											//we're still in world or death room
											bGlobal.curXP[pBam->playerSide] = bGlobal.lastLevelXP[1];
											memcpy(bGlobal.unitsResearched, bGlobal.lastUnitsResearched[1], UNITMAXTYPES);
										}

										bGlobal.replayMap = TRUE;
									}
									else
									{
										if(bGlobal.roomMgr.curRoomNum == BR_ASSESS)
										{
											bGlobal.missionsDone--;
											GetScenario(pBam->scenarioName,bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);
											bGlobal.curXP[pBam->playerSide] = bGlobal.lastLevelXP[0];
											memcpy(bGlobal.unitsResearched, bGlobal.lastUnitsResearched[0], UNITMAXTYPES);

											//these vars. were updated in assess room -must now reset them
											bGlobal.lawful	 = bGlobal.lastLawful;
											bGlobal.neutral = bGlobal.lastNeutral;
											bGlobal.chaotic = bGlobal.lastChaotic;

											bGlobal.evil   = bGlobal.lastEvil;
											bGlobal.nutral = bGlobal.lastNutral;
											bGlobal.good   = bGlobal.lastGood;
										}
										else
										{
											//we're still in world room
											bGlobal.curXP[pBam->playerSide] = bGlobal.lastLevelXP[1];
											memcpy(bGlobal.unitsResearched, bGlobal.lastUnitsResearched[1], UNITMAXTYPES);
										}
									}

									bGlobal.roomMgr.NewRoom(BR_WORLD);
									return TRUE;
									break;

								case NEW_STORY_BUTTON:
									pMono->Out("\nNew story confirmed.");
									Cleanup(); // This cleanup first to properly
												  // restore mouse boundaries.
									//lets remove calling option window
									if (gPrevCon)
									{
										pRoom = ADerefAs(BAM_Room,gPrevCon);
										pRoom->Cleanup();
									}

									ClearGlobals();

									if(bGlobal.storyLine == LEGEND)
									{
										ClearGlobals();
										bGlobal.storyLine = LEGEND;	//reset
										bGlobal.roomMgr.NewRoom(BR_CHAR);
									}
									else
									{
										//if all stories done
										if(GetStoryDone() & 0x1F)
										{
											bGlobal.roomMgr.newRoomMode = 1; //single menu
											bGlobal.roomMgr.NewRoom(BR_MENU);
										}
										else
										{
											bGlobal.roomMgr.NewRoom(BR_STORY);
										}
									}
									return TRUE;
									break;

							}

							//should never get here
							return TRUE;
						}
						else if(pMsg->notice.gSource == button[1].gSelf)
						{
							pMono->Out("\nCancel Button was hit");
							Cleanup();
							return TRUE;
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
				switch(pMsg->event.type)
				{
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
							case K_A:
								FakeMouseClick(button[0].gSelf);
								return TRUE;
								break;

							case K_ESC:
							case K_C:
								Cleanup();
								return TRUE;
								break;
						}
						break;
				}
				break;
		}//endswitch
	}
	return TRUE;	//no pass-thru
}


void
SubOptionMenu::Cleanup()
{
	Activate(FALSE);

	delete this;
}
