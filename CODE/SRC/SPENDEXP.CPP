// SPENDEXP.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Spend experience popup screen
//
// 11/6/95
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
#include "story.hpp"
#include "spendexp.hpp"

#include <string.h>

#define	SPENDEXP_BASE_PRI	20000
#define	SPENDEXP_WIN_X		20
#define	SPENDEXP_WIN_Y		38

#define	FIRST_SLOT_Y  	78
#define	SLOT_X		  	20
#define	SLOT_COST_X	  	21
#define	SLOT_NAME_X	  	54
#define	SLOT_HEIGHT	  	12
#define	SLOT_WIDTH	  	105

#define	BAD_CLICK_DELAY	8

enum
{
	SQB_GARGOYLE = 20,
	SQB_RANGER,
	SQB_FURY,
	SQB_GNOME,
	SQB_HARPY,
	SQB_GRIFFIN,
	SQB_GOLEM,
	SQB_GHOUL,
	SQB_ENCHANTER,
	SQB_PALADIN,
	SQB_WYRM,
	SQB_NYMPH,
	SQB_WRAITH
};

//=========================================================

SpendExperience::SpendExperience()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;

	curSlot = 0;
	badClick = FALSE;

	rPic.Set(SPENDEXP_WIN_X+148,SPENDEXP_WIN_Y+95,SPENDEXP_WIN_X+186,SPENDEXP_WIN_Y+163);
}

SpendExperience::~SpendExperience()
{
	if(prevFont != 9050)
		pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}


void
SpendExperience::Setup(grip gPal)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			i,mapNum;
	bool			extraChar;


	//======================================================
	prevFont = pFontMgr->curFontNum;
	if(prevFont != 9050)
		pFontMgr->SetRes(9050);

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "SpendExperience::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
	{
		mode = M_MODELESS;
	}
	else
	{
		mode = M_MODAL;
		bGlobal.roomMgr.curRoom->Pause(TRUE);
	}


	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	TRACK_MEM("SpendExperience: background anim");
	gbackAnim = ALoad(RES_ANIM,8312);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(SPENDEXP_WIN_X+0,SPENDEXP_WIN_Y+0,SPENDEXP_WIN_X+pbackAnimCH->width-1,SPENDEXP_WIN_Y+pbackAnimCH->height-1);

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("SpendExperience: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,SPENDEXP_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,8312,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(SPENDEXP_WIN_X,SPENDEXP_WIN_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SPENDEXP_BASE_PRI);

	pGuy = &topBorder;
	pGuy->SetRes(RES_ANIM,116,5);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SPENDEXP_BASE_PRI-20);

	pGuy = &sideBorder;
	pGuy->SetRes(RES_ANIM,118,1);
	pGuy->SetPos(0,25);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SPENDEXP_BASE_PRI-19);

	pMouse->SetLimits(&rback);

	//need to get these new interface pieces onto the screen BEFORE
	//the do the screen capture and fade down
	AAnimate();

	//======================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	if(bGlobal.storyLine != NETGAME)
		fadeTo.Setup(320,400,SPENDEXP_BASE_PRI - 10,gSelf,gPal,0,&rback);


	pGuy = &slotCursor;
	pGuy->SetRes(RES_ANIM,9002, 7);
	pGuy->SetPos(SPENDEXP_WIN_X+SLOT_X, SPENDEXP_WIN_Y+FIRST_SLOT_Y);
	pGuy->SetContext(gSelf);
	//pGuy->Setup(CT_ROST|CT_SERV);
	pGuy->Setup();
	pGuy->SetPri(SPENDEXP_BASE_PRI+10);
	pGuy->SetClock(expClock.gSelf);
	
	TRACK_MEM("PALCYCLER");
	Utility	*pUtil = new PalCycler(PCYC_FORWARD, 220, 223, gPal, 2, 0);
	pUtil->SetClock(expClock.gSelf);
	pGuy->SetupUtil(pUtil);

	//===============================================================
	pFontMgr->SetRes(9062);
	SetFontColors(CI_SKIP,94,92,90,89,89,CI_BLACK);

	pTxt = sqbExp.Load(POPUP_SQB,12); //'exp'
	ASetString(27,28, pTxt, pback, pbackCH->width, NULL, DG_JUST_LEFT);

	pTxt = sqbExp.Load(POPUP_SQB,10); //'cost'
	ASetString(21,50, pTxt, pback, pbackCH->width, NULL, DG_JUST_LEFT);

	pTxt = sqbExp.Load(POPUP_SQB,13);	//'creature'
	ASetString(54,28, pTxt, pback, pbackCH->width, NULL, DG_JUST_LEFT);

	pTxt = sqbExp.Load(POPUP_SQB,11);	//'ability'
	ASetString(54,50, pTxt, pback, pbackCH->width, NULL, DG_JUST_LEFT);

	pFontMgr->SetRes(9050);

	//===============================================================

	expClock.SetTicks(ATicks());

	//===============================================================
	mapNum = atoi(pBam->scenarioName);
	if(mapNum >= 9350 && mapNum <= 9395)
		extraChar = TRUE;
	else
		extraChar = FALSE;

	#ifdef INTERACTIVE_DEMO
		slotNum = 4;

	 	slot[0] = GARGOYLE;
	 	slot[1] = RANGER;
	 	slot[2] = FURY;
	 	slot[3] = GNOME;
	#else

	if ((bGlobal.storyLine == STORYLINE5 && bGlobal.chooseSide == 2)
		 || (bGlobal.storyLine == LEGEND  && extraChar)
		 || (bGlobal.storyLine == NETGAME && extraChar))
	{
		//we're RedFang -show enchanters and harpies
		//or net game

		slotNum = 13;

	 	slot[0] = GARGOYLE;
	 	slot[1] = RANGER;
	 	slot[2] = FURY;
	 	slot[3] = GNOME;
	 	slot[4] = HARPY;
	 	slot[5] = GRIFFIN;
	 	slot[6] = GOLEM;
	 	slot[7] = GHOUL;
	 	slot[8] = ENCHANTER;
	 	slot[9] = PALADIN;
	 	slot[10] = WYRM;
	 	slot[11] = NYMPH;
	 	slot[12] = WRAITH;
		

	}
	else
	{
		//if legend campaign or any other storyline
		//no show enchanters and harpies

		slotNum = 11;

	 	slot[0] = GARGOYLE;
	 	slot[1] = RANGER;
	 	slot[2] = FURY;
	 	slot[3] = GNOME;
	 	slot[4] = GRIFFIN;
	 	slot[5] = GOLEM;
	 	slot[6] = GHOUL;
	 	slot[7] = PALADIN;
	 	slot[8] = WYRM;
	 	slot[9] = NYMPH;
	 	slot[10] = WRAITH;
	}

	#endif

	//set master rect which holds all slots
	rSlot.Set(SPENDEXP_WIN_X+SLOT_X, SPENDEXP_WIN_Y+FIRST_SLOT_Y,
				 SPENDEXP_WIN_X+SLOT_X+SLOT_WIDTH-1, SPENDEXP_WIN_Y+FIRST_SLOT_Y + (SLOT_HEIGHT * slotNum) - 1);

	int	y,cost;
	UnitGroup	*pGrp;
	char	costStr[6];

	for(i=0;i<slotNum;i++)
	{
		// set slot text x,y
		y = FIRST_SLOT_Y + (SLOT_HEIGHT * i) + 1;

		GetCreatureInfo(slot[i],&slotSqb,&slotPortrait);

		pGrp = pWorld->unitLib.GetUnitGroup(slot[i]);
		cost = pGrp->expRequired;
		sprintf(costStr,"%d",cost);

		if(bGlobal.unitsResearched[slot[i]])
		{
			//we have already researched -show pic in full color
			slotAvail[i] = TRUE;	

			SetFontColors(CI_SKIP,171,175,16,23,CI_BLACK,CI_BLACK);

			//write name
			pTxt = sqbExp.Load(POPUP_SQB,slotSqb);
			ASetString(SLOT_NAME_X,y, pTxt, pback, pbackCH->width - SLOT_NAME_X, NULL, DG_JUST_LEFT);
		}
		else
		if(cost <= bGlobal.curXP[pBam->playerSide])
		{
			//we have enough to buy
			slotAvail[i] = TRUE;

			//uppercase:1,2  numbers:3,4  lowercase:5,6
			//				blue			 red				 blue
			SetFontColors(SKIP_COLOR,87,114,183,187,87,114);

			//write cost
			ASetString(SLOT_COST_X,y, costStr, pback, 25, NULL, DG_JUST_RIGHT);

			//write creature name
			pTxt = sqbExp.Load(POPUP_SQB,slotSqb);
			ASetString(SLOT_NAME_X,y, pTxt, pback, pbackCH->width - SLOT_NAME_X, NULL, DG_JUST_LEFT);
		}
		else
		{
			//not yet available
			slotAvail[i] = FALSE;

			SetFontColors(CI_SKIP,175,177,175,177,CI_BLACK,CI_BLACK);

			//write cost
			ASetString(SLOT_COST_X,y, costStr, pback, 25, NULL, DG_JUST_RIGHT);

			//write creature name
			pTxt = sqbExp.Load(POPUP_SQB,slotSqb);
			ASetString(SLOT_NAME_X,y, pTxt, pback, pbackCH->width - SLOT_NAME_X, NULL, DG_JUST_LEFT);

		}

		if(i == 0)
			//start w/ gargoyle portrait on screen
			CopyCel(pbackCH,148,95,RES_ANIM,2568,(slotAvail[0])?1:2,FALSE);

	}


	//===============================================================
	pButton = &cancelB;
	pButton->Create(SPENDEXP_WIN_X+84, SPENDEXP_WIN_Y+236, SPENDEXP_BASE_PRI + 1, RES_ANIM, 8306, 1, gSelf, SPENDEXP_WIN_X, SPENDEXP_WIN_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbExp.Load(OPTION_SQB,21);	//'cancel'
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);	 // set button to unselected state - will cause drawing into master cel

	//since we had to make a special call to AAnimate above
	//we need to re-update the rect now that we've written our text
	AUpdateRect(&rback);

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;									// grip of currently active button, if any

}


bool
SpendExperience::HandleMsg(Message* pMsg)
{
	int	sn;
	int	rCost,rTime;
	UnitGroup	*pGrp;

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
						if(pMsg->notice.gSource == cancelB.gSelf)
						{
							Cleanup();
							return TRUE;
						}
					}
				}
				break;

			case MSG_EVENT:
				switch (pMsg->event.type)
				{
					case E_MOUSE_DOWN:
						//are we in slots?
						if(rSlot.Contains(pMsg->event.x,pMsg->event.y))
						{
							//get slot number
							sn = (pMsg->event.y - FIRST_SLOT_Y - SPENDEXP_WIN_Y) / SLOT_HEIGHT;
						
							//get research cost
							//get research time
							pGrp = pWorld->unitLib.GetUnitGroup(slot[sn]);
							rCost = pGrp->expRequired;
							rTime = pGrp->researchTime;
						
							if(bGlobal.curXP[pBam->playerSide] < rCost ||
								bGlobal.unitsResearched[slot[sn]])
							{
								SetupBadClick();
							 	return(TRUE);
							}
						
							//bGlobal.curXP[pBam->playerSide] -= rCost;
							pWorld->AdjustXP(pBam->playerSide, -rCost);
						
							//call set research function
							pWorld->StartResearch(slot[sn],rTime);
						
							Cleanup();
							return(TRUE);
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
								//lets go thru world
								//return pBam->HandleMsg(pMsg);
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
							case K_C:	//cancel
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
SpendExperience::Cycle()
{
	ticks_t	curTime;
	int		x,y,sn;
	CelHeader	*pbackCH;

	expClock.Cycle();

	BAM_Room::Cycle();

	if(badClick)
	{
		curTime = expClock.GetTicks();
		if(curTime > badClickTime)
		{
			badClick = FALSE;
			pMouse->SetRes(RES_ANIM,POINTER_RES,1);
		}
	}

	x = pMouse->GetX();
	y = pMouse->GetY();

	if(rSlot.Contains(x,y) )
	{
		//get slot number
		sn = (y - FIRST_SLOT_Y - SPENDEXP_WIN_Y) / SLOT_HEIGHT;

		if(sn != curSlot)
		{
			curSlot = sn;

			slotCursor.SetPos(SPENDEXP_WIN_X+SLOT_X, SPENDEXP_WIN_Y+FIRST_SLOT_Y + (SLOT_HEIGHT * curSlot));

			GetCreatureInfo(slot[sn],&slotSqb,&slotPortrait);
			pbackCH = (CelHeader*)AGetResData(gback);
			CopyCel(pbackCH,148,95,RES_ANIM,slotPortrait,(slotAvail[sn])?1:2,FALSE);
			AUpdateRect(&rPic);
		}
	}
}


void
SpendExperience::Cleanup()
{
	if(bGlobal.storyLine != NETGAME)
	{
		fadeTo.FadeUp();
		bGlobal.roomMgr.curRoom->Pause(FALSE);
	}

	Activate(FALSE);

	delete this;
}


void
SpendExperience::SetupBadClick()
{
	pMouse->SetRes(RES_ANIM,POINTER_RES,8);

	badClick = TRUE;
	badClickTime = expClock.GetTicks() + BAD_CLICK_DELAY;
}


int
SpendExperience::GetCreatureInfo(int creature, int *sqb, int *portrait)
{
	int ret = 0;

 	switch(creature)
	{
		case GARGOYLE:
			*portrait = 2568;
			ret = *sqb = SQB_GARGOYLE;
			break;
		case RANGER:
			*portrait = 2588;
			ret = *sqb = SQB_RANGER;
			break;
		case FURY:
			*portrait = 2566;
			ret = *sqb = SQB_FURY;
			break;
		case GNOME:
			*portrait = 2572;
			ret = *sqb = SQB_GNOME;
			break;
		case HARPY:
			*portrait = 2612;
			ret = *sqb = SQB_HARPY;
			break;
		case GRIFFIN:
			*portrait = 2578;
			ret = *sqb = SQB_GRIFFIN;
			break;
		case GOLEM:
			*portrait = 2574;
			ret = *sqb = SQB_GOLEM;
			break;
		case GHOUL:
			*portrait = 2570;
			ret = *sqb = SQB_GHOUL;
			break;
		case ENCHANTER:
			*portrait = 2594;
			ret = *sqb = SQB_ENCHANTER;
			break;
		case PALADIN:
			*portrait = 2586;
			ret = *sqb = SQB_PALADIN;
			break;
		case WYRM:
			*portrait = 2606;
			ret = *sqb = SQB_WYRM;
			break;
		case NYMPH:
			*portrait = 2582;
			ret = *sqb = SQB_NYMPH;
			break;
		case WRAITH:
			*portrait = 2604;
			ret = *sqb = SQB_WRAITH;
			break;
	}

	return ret;
}
