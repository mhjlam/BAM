// ENCYMENU.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen for encyclopedia main menu.
//
// 4/20/95
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
#include "ENCYMENU.HPP"
#include "encyclo.HPP"

#include <string.h>

#define	ENCYMENU_BASE_PRI	20000
#define	ENCYMENU_WIN_X		20
#define	ENCYMENU_WIN_Y		38

//=========================================================

EncyMenu::EncyMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	commentCnt = 1;
}

EncyMenu::~EncyMenu()
{
	pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}

void
EncyMenu::Setup(grip gPal,bool fadeup)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			x,buttonX,buttonY;


	//======================================================
	prevFont = pFontMgr->curFontNum;

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "EncyMenu::Setup");

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

	TRACK_MEM("EncyMenu: background anim");
	gbackAnim = ALoad(RES_ANIM,50);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(ENCYMENU_WIN_X,ENCYMENU_WIN_Y,ENCYMENU_WIN_X+pbackAnimCH->width-1,ENCYMENU_WIN_Y+pbackAnimCH->height-1);

	// setup background cel filled with black
	TRACK_MEM("EncyMenu: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,ENCYMENU_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel((CelHeader*)pback,0,0,RES_ANIM,50,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(ENCYMENU_WIN_X,ENCYMENU_WIN_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(ENCYMENU_BASE_PRI);

	pGuy = &topBorder;
	pGuy->SetRes(RES_ANIM,116,5);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(ENCYMENU_BASE_PRI-20);

	pGuy = &sideBorder;
	pGuy->SetRes(RES_ANIM,118,1);
	pGuy->SetPos(0,25);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(ENCYMENU_BASE_PRI-19);

	pMouse->SetLimits(&rback);

	//need to get these new interface pieces onto the screen BEFORE
	//the do the screen capture and fade down
	AAnimate();

	//======================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	if(bGlobal.storyLine != NETGAME)
	{
		fadeTo.Setup(320,400,ENCYMENU_BASE_PRI - 10,gSelf,gPal,0,&rback);
	}

	//===============================================================
	pFontMgr->SetRes(9056);
	SetFontColors(CI_SKIP,159,165,166,168,158,160);
	pTxt = sqbEncyMenu.Load(ENCYMENU_SQB,6);
	ASetString(0,35, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);


	//===============================================================

	buttonNum = 5;
	bVal[0] = CREATURE_BUTTON;
	bVal[1] = STRUCTURE_BUTTON;
	bVal[2] = ITEM_BUTTON;
	bVal[3] = TERRAIN_BUTTON;
	bVal[4] = DONE_BUTTON;



	// center buttons vertically
	// height of button caps = 3, height of buttons = 17
	// (number of caps = 4)*(cap size = 3) + (button gap = 6) = (total of 18)
	// master Y for background is 0, cel height is 266
	// 21 for title
	//
	uint32 topMargin,bottomCap1,secondTop,bottomCap2;
	capPosY[0] = topMargin  = 21 +(266-(18+(17*buttonNum)))/2;
	capPosY[1] = bottomCap1 = capPosY[0] + 3 + (17*(buttonNum - 1));
	capPosY[2] = secondTop  = capPosY[1] + 3 + 6;
	capPosY[3] = bottomCap2 = capPosY[2] + 3 + 17;
	buttonX = 63;

	//Cap setups
	for(x=0; x<4; x++)
	{
		CopyCel(pbackCH,buttonX,capPosY[x],RES_ANIM,40,(x%2)?3:2,FALSE);
	}


	char *pButtonName; //temp. vars.
	
	//Button Setup
	for(x=0;x<buttonNum;x++)
	{
		pButton = &button[x];

		if (x >= buttonNum - 1)
			buttonY = topMargin + 15 + (x*17);
		else
			buttonY = topMargin + 3 + (x*17);

		pButton->Create(ENCYMENU_WIN_X+buttonX, ENCYMENU_WIN_Y+buttonY, ENCYMENU_BASE_PRI + 1, RES_ANIM, (x%2)?44:42, 1,
							 gSelf, ENCYMENU_WIN_X, ENCYMENU_WIN_Y);

		pButtonName = sqbEncyMenu.Load(ENCYMENU_SQB,bVal[x]+1);

		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetCelText(1, pButtonName);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pButtonName);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Draw();
		//pButton->Listen(TRUE);	//turn on click response -already in create()
	}


	//since we had to make a special call to AAnimate above
	//we need to re-update the rect now that we've written our text
	AUpdateRect(&rback);

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;									// grip of currently active button, if any

	//AAnimate();

	if(fadeup)
	{
		TPalette *pPal = ADerefAs(TPalette,gPal);
		pPal->FadeUp();
	}

}


bool
EncyMenu::HandleMsg(Message* pMsg)
{
	char			mess[100];
	uint32		x;
	Encyclopedia *pEncyclo;

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
						for(x=0;x<buttonNum;x++)
						{
							if(pMsg->notice.gSource == button[x].gSelf)
							{
								switch (bVal[x])
								{
									case CREATURE_BUTTON:
										pMono->Out("\nCritter Button was hit");
										TRACK_MEM("Encyclo");	pEncyclo = new Encyclopedia;
										pEncyclo->Setup(CREATURE_BUTTON);
										return TRUE;
										break;
									case STRUCTURE_BUTTON:
										pMono->Out("\nStructure Button was hit");
										TRACK_MEM("Encyclo");	pEncyclo = new Encyclopedia;
										pEncyclo->Setup(STRUCTURE_BUTTON);
										return TRUE;
										break;
									case ITEM_BUTTON:
										pMono->Out("\nItem Button was hit");
										TRACK_MEM("Encyclo");	pEncyclo = new Encyclopedia;
										pEncyclo->Setup(ITEM_BUTTON);
										return TRUE;
										break;
									case TERRAIN_BUTTON:
										pMono->Out("\nTerrain Button was hit");
										TRACK_MEM("Encyclo");	pEncyclo = new Encyclopedia;
										pEncyclo->Setup(TERRAIN_BUTTON);
										return TRUE;
										break;
									case DONE_BUTTON:
										pMono->Out("\nDone Button was hit");
										Cleanup();
										return TRUE;
										break;
									default:
            						sprintf(mess, "Unrecognized button type: %d", pMsg->notice.param);
            						APanic(mess);
										break;
								}
							}
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
							case K_D:	//done
								Cleanup();
								return TRUE;
								break;
						}
				}
				break;
		}//endswitch
	}
	return TRUE;	//no pass-thru
}


void
EncyMenu::Cleanup()
{

	if(bGlobal.storyLine != NETGAME)
	{
		fadeTo.FadeUp();
		bGlobal.roomMgr.curRoom->Pause(FALSE);
	}

	Activate(FALSE);

	delete this;
}

