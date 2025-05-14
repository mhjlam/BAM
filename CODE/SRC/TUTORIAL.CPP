// TUTORIAL.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen to show tutorial pages.
//
// 11/27/95
//

#include "api.hpp"
#include "apires.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "context.hpp"
#include "eventmgr.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "scrimage.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "TUTORIAL.HPP"

#include <string.h>

#define TUT_WIN_X		20
#define TUT_WIN_Y		38

#define TUTORIAL_BASE_PRI	22000

#define TUT_SETUP_DELAY		2

//=========================================================

Tutorial::Tutorial()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;

	int TdisplayAni[TUT_DISPLAY_NUM][2] = {
	//sqb,  stage
		20, 1,
		30, 2,
		35, 2,
		40, 2,
		45, 2,
		50, 3,
	  115, 4,
	  205, 5,
	  210, 6,
	  215, 6,
	  220, 7,
	  405, 7,
	  410, 8,
	  415, 8,
	  520, 9,
	  525, 9,
	  530,10,
	  535,11,
	  620,12,
	  625,12,
	  720,13,
	  725,13
	  };

	memcpy(displayAni,TdisplayAni,sizeof(TdisplayAni));

	int Tstages[TUT_STAGE_NUM][6] = { 
	//id, ani,cel, X,  Y, fFrame
		1, 125, 1, 38, 77, 0,
		1, 128, 1, 38,121, 0,
		2,	511, 4, 45,107, 0,
		2,	511, 4, 68,107, 0,
		2,	322, 1, 55,150, 0,
		3,	511, 4, 44, 74, 0,
		3,	511, 4, 44,113, 0,
		3,	511, 4, 44,152, 0,
		3,	511, 4, 67, 74, 0,
		3,	511, 4, 67,113, 0,
		3,	511, 4, 67,152, 0,
		4, 109, 1, 37, 80, 0,
		5,2510, 2, 37, 80, 2,
		6,2718, 1, 37, 80, 1,
		7,2558, 1, 37, 80, 1,
		8, 770, 4, 55, 84, 0,
		9,8161, 1, 35, 77, 0,
	  10,8161, 2, 35, 77, 0,
	  11,2708, 1, 37, 80, 1,
	  12,2568, 1, 37, 80, 1,
	  13,2702, 1, 37, 80, 1
	  };

	memcpy(stages,Tstages,sizeof(Tstages));

	advance = FALSE;
	setupDone = FALSE;

	//TRACK_MEM("Choose Clut");	clut = AMalloc(CLUT_SIZE);
}

Tutorial::~Tutorial()
{
	pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	//AFree(clut);
}

void
Tutorial::Setup(grip gPal_P, bool fadeup_P, bool advance_P, tutorialGoals goal_P)
{
	gPal = gPal_P;
	fadeup = fadeup_P;
	advance = advance_P;
	goal = goal_P;

	//with fadeup comes and immediate setup
	if(fadeup)
	{
		Setup();
	}
	else
	{
		//set a timer
		setupTimer = ATicks() + TICKS_PER_SEC * TUT_SETUP_DELAY;

		Activate(TRUE);

		// move us to the top of the ContextMgr receiver list
		pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
		gCurControl	=	NULL;
	}
}


void
Tutorial::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackCH,*pbackAnimCH;
	int			i;

	//======================================================
	prevFont = pFontMgr->curFontNum;

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start Tutorial::Setup");

	mode = M_MODAL;
	bGlobal.roomMgr.curRoom->Pause(TRUE);

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	//======================================================
	//checkout goal parameter -and patch up if nec.

	//find lowest false goal
	for(i=0;i<TUT_TOTAL_GOALS && bGlobal.fTutorialGoals[i];i++);

	//if last goal satisfied then end it now
	if(goal == TUT_BLDG_DESTROYED)
		goalNum = TUT_BLDG_DESTROYED;
	else
		goalNum = (tutorialGoals)i;

	//========================================
	//NOTE: goal will be set true in Cleanup()
	//========================================

	if(advance)
	{
		//lets do some weird stuff to handle that goal pair

		if(goalNum == TUT_UNIT_TRANSFORMED)
		{
			//skip over TUT_UNIT_HEALED
			goalNum = TUT_BLDG_REPAIRED;
		}
		else
		if(goalNum == TUT_UNIT_HEALED || goalNum == TUT_BLDG_REPAIRED)
		{
			//both goals must be true or we bail
			if(goalNum == TUT_UNIT_HEALED   && bGlobal.fTutorialGoals[TUT_BLDG_REPAIRED] ||
				goalNum == TUT_BLDG_REPAIRED && bGlobal.fTutorialGoals[TUT_UNIT_HEALED] ||
				bGlobal.fTutorialGoals[TUT_BLDG_REPAIRED] && bGlobal.fTutorialGoals[TUT_UNIT_HEALED])
			{
				goalNum = TUT_BUILDING_MADE;
			}
			else
			{
				bGlobal.fTutorialGoals[goal] = TRUE;

				bGlobal.roomMgr.curRoom->Pause(FALSE);
				delete this;
				return;
			}
		}
		else
			//all others are linear
			goalNum = (tutorialGoals)(goalNum+1);	//incr. goal
	}
	else
	{
		//no advance, but still have to make the goal pair point to same sqb
		if(goalNum == TUT_UNIT_HEALED)
			goalNum = TUT_BLDG_REPAIRED;
	}	



	TRACK_MEM("Tutorial: background anim");
	gbackAnim = ALoad(RES_ANIM,50);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(TUT_WIN_X,TUT_WIN_Y,TUT_WIN_X+pbackAnimCH->width-1,TUT_WIN_Y+pbackAnimCH->height-1);

	// setup background cel filled with black
	TRACK_MEM("Tutorial: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,TUTORIAL_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*) pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,50,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(TUT_WIN_X,TUT_WIN_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(TUTORIAL_BASE_PRI);

	pGuy = &topBorder;
	pGuy->SetRes(RES_ANIM,116,5);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(TUTORIAL_BASE_PRI-20);

	pGuy = &sideBorder;
	pGuy->SetRes(RES_ANIM,118,1);
	pGuy->SetPos(0,25);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(TUTORIAL_BASE_PRI-19);

	pMouse->SetLimits(&rback);

	//need to get these new interface pieces onto the screen BEFORE
	//the do the screen capture and fade down
	AAnimate();

	//======================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	fadeTo.Setup(320,400,TUTORIAL_BASE_PRI - 10,gSelf,gPal,0,&rback);

	//uchar 		*pClut;
	//pClut = ADerefAs(uchar, clut);
	//
	//for(i=0;i<CLUT_SIZE;i++)
	//{
	//	pClut[i] = (uchar)(CLUT2_START + i);
	//}


	//==================================================
	pButton = &buttonDone;
	pButton->Create(86, 253, TUTORIAL_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, TUT_WIN_X, TUT_WIN_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbTutor.Load(OPTION_SQB,22); //'continue'
	pButton->SetCelText(1, pTxt);
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);


	//===============================================================
	//Get and display text message for this map


	curSqbNum = (goalNum * 100) + 5;

	pTxt = sqbTutor.Load(TUTORIAL_SQB,curSqbNum);

	pFontMgr->SetRes(9066);
	//uppercase:1,2  numbers:3,4  lowercase:5,6
	//				blue			 blue				 red
	//SetFontColors(CI_SKIP,87,114,87,114,183,187);
	SetFontColors(CI_SKIP,94,102,94,102,156,144);

	ASetString(85, 48, pTxt, pback, 107, NULL);

	//since we had to make a special call to AAnimate above
	//we need to re-update the rect now that we've written our text
	AUpdateRect(&rback);

	DisplayPic();
	pBam->voice1.Stop();
	pBam->voice1.Play(TUTORIAL_SQB * 10000 +curSqbNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);

	//==================================================

	if(fadeup)
	{
		Activate(TRUE);

		// move us to the top of the ContextMgr receiver list
		pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
		gCurControl	=	NULL;									// grip of currently active button, if any

		//AAnimate(); done above already

		TPalette *pPal = ADerefAs(TPalette,gPal);
		pPal->FadeUp();
	}

}


bool
Tutorial::HandleMsg(Message* pMsg)
{
	uchar			*pback;

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

							curSqbNum += 5;
							pTxt = sqbTutor.Load(TUTORIAL_SQB,curSqbNum,TRUE);

						
							if(pTxt)
							{
								//more sqb's to write
								Clear();

								pback = AGetResData(gback);
								//uppercase:1,2  numbers:3,4  lowercase:5,6
								//				blue			 blue				 red
								//SetFontColors(CI_SKIP,87,114,87,114,183,187);
								SetFontColors(CI_SKIP,94,102,94,102,156,144);
								ASetString(85, 48, pTxt, pback, 107, NULL);

								DisplayPic();
								pBam->voice1.Play(TUTORIAL_SQB * 10000 +curSqbNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);
							}
							else
							{
								//if advance and next goal is already met then display that text too

								if(advance)
								{
									//lets do some weird stuff to handle that goal pair

									//===========================================================
									//NOTE: goalNum represents the next set of sqbs to display
									//IF we have satisfied all goal up to this point.
									//===========================================================

									if(goalNum == TUT_UNIT_TRANSFORMED)
									{
										//to show this goal we've advanced to, it must already be true
										if(bGlobal.fTutorialGoals[goalNum])
										{
											goalNum = TUT_BLDG_REPAIRED;
										}
										else
										{
											Cleanup();
											return TRUE;
										}
									}
									else
									if(goalNum == TUT_UNIT_HEALED || goalNum == TUT_BLDG_REPAIRED)
									{
										//both goals must be true or we bail
										if(bGlobal.fTutorialGoals[TUT_UNIT_HEALED] &&
											bGlobal.fTutorialGoals[TUT_BLDG_REPAIRED])
										{
											goalNum = TUT_BUILDING_MADE;
										}
										else
										{
											Cleanup();
											return TRUE;
										}
									}
									else
									{
										//if tutorial done then bail
										if(goalNum >= TUT_DUMMY_GOAL)
										{
											Cleanup();
											bGlobal.roomMgr.NewRoom(BR_MENU);
											return TRUE;
										}


										//to show this goal we've advanced to, it must be already true
										if(bGlobal.fTutorialGoals[goalNum])
										{
											goalNum = (tutorialGoals)(goalNum+1);	//incr. goal
										}
										else
										{
											Cleanup();
											return TRUE;
										}
									}


									curSqbNum = (goalNum*100) + 5;	//reset
									pTxt = sqbTutor.Load(TUTORIAL_SQB,curSqbNum,TRUE);

									if(pTxt)
									{
										//more sqb's to write
										Clear();

										pback = AGetResData(gback);
										//uppercase:1,2  numbers:3,4  lowercase:5,6
										//				blue			 blue				 red
										//SetFontColors(CI_SKIP,87,114,87,114,183,187);
										SetFontColors(CI_SKIP,94,102,94,102,156,144);
										ASetString(85, 48, pTxt, pback, 107, NULL);

										DisplayPic();
										pBam->voice1.Play(TUTORIAL_SQB * 10000 +curSqbNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);
									}
								}
								else
								{
									Cleanup();
								}
							}

							return TRUE;
						}
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
								return pWorld->HandleMsg(pMsg);
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
								Cleanup();

								if(goalNum > TUT_TOTAL_GOALS)
										bGlobal.roomMgr.NewRoom(BR_MENU);

								return TRUE;
								break;

							case K_C:
								FakeMouseClick(buttonDone.gSelf);
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
Tutorial::Cleanup()
{
	if(advance)
		bGlobal.fTutorialGoals[goal] = TRUE;

	fadeTo.FadeUp();	//doesn't do anything if not already faded down
	bGlobal.roomMgr.curRoom->Pause(FALSE);

	pBam->voice1.Stop();

	Activate(FALSE);

	pBam->voice1.Stop();

	delete this;
}


void
Tutorial::Clear()
{
	CelHeader	*pbackCH;
	Rectangle	rTxt;

	pbackCH = (CelHeader*) AGetResData(gback);
	//rTxt.Set(84,47,193,200);
	//CopyCel(pbackCH,84,47,RES_ANIM,50,1,&rTxt,FALSE);
	rTxt.Set(0,0,pbackCH->width-1,pbackCH->height-1);
	CopyCel(pbackCH,0,0,RES_ANIM,50,1,&rTxt,FALSE);
	back.SetState(S_CHANGED,TRUE);

	pBam->voice1.Stop();
}


void
Tutorial::DisplayPic()
{
	CelHeader	*pbackCH,*tempCelHeader;
	Resource		*pSrcRes;
	grip			gSrcRes;
	int	i,j,stageNum;
	bool	found = FALSE;
	int	orgX, orgY;

	pbackCH = (CelHeader*) AGetResData(gback);

	CopyCel(pbackCH,64,212,RES_ANIM,50,3,TRUE);	// 'Done' w/undone button frame
 	buttonDone.SetState(S_CHANGED,TRUE);
	buttonDone.Draw();

	//loop thru and find ani that matches curSqbNum if any
	//take curSqbNum's stageNum
	//loop thru all matching stages display those objects
	for(i=0;i<TUT_DISPLAY_NUM;i++)
	{
		//we found the displayNum
		if(displayAni[i][0] == curSqbNum)
		{
			found = TRUE;
			stageNum = displayAni[i][1];

			//copy in all pic objects
			for(j=0;j<TUT_STAGE_NUM;j++)
			{
				gSrcRes = ALoad(RES_ANIM, stages[j][1]);
				pSrcRes = ADerefAs(Resource, gSrcRes);

				tempCelHeader = pSrcRes->GetHeader((uint16) (stages[j][2]));
				orgX = tempCelHeader->orgX;
				orgY = tempCelHeader->orgY;

				if( stages[j][0] == stageNum )
				{
					
					CopyCel(pbackCH,stages[j][3]-orgX,stages[j][4]-orgY,RES_ANIM,stages[j][1],stages[j][2],TRUE);

					if(stages[j][5] == 1)
						CopyCel(pbackCH,35,77,RES_ANIM,50,2,TRUE);	// Portrait Border
					else 
					if(stages[j][5] == 2)
						CopyCel(pbackCH,35,77,RES_ANIM,50,5,TRUE);	// Mini Portrait Border
				}

				//if we've looped past the num we're lookin' fer
				if(stages[j][0] > stageNum)
					break;
			}

			break;
		}

		//if we've looped past the num we're lookin' fer
		if(displayAni[i][0] > curSqbNum)
			break;
	}
	
	if(!found)
	{
	  	//then lets do the default
		CopyCel(pbackCH,35,77,RES_ANIM,50,2,TRUE);	// Portrait Border 2
		//Load portrait
		CopyCel(pbackCH,37,80,RES_ANIM,2824,1,TRUE);	// Tutorial Wizard

	}
}


void
Tutorial::Cycle()
{
	//if delayed setup
	if(!fadeup && !setupDone && ATicks() > setupTimer)
	{
		setupDone = TRUE;
		Setup();
	}
}




