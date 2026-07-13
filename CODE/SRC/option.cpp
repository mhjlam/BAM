// OPTION.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A popup screen to handle all game options.
//
// 11/16/94
//

#include "api.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "bam_dg.hpp"
#include "option.hpp"
#include "option2.hpp"
#include "option3.hpp"
#include "option4.hpp"
#include "savemenu.hpp"

#include <string.h>

#define	OPTION_BASE_PRI	20000
#define MM_MAIN_SQB		7010

//=========================================================

OptionMenu::OptionMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
}

OptionMenu::~OptionMenu()
{
	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);
	pMouse->ForceShow((int16)oldMouseHide);

	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}

void
OptionMenu::Setup(grip gPal)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			x,buttonY;


	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start Option::Setup");

	//have to do this section first thing because of matching code in destructor
	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);
	oldMouseHide = pMouse->ForceShow(0);

	if(bGlobal.roomMgr.newRoom)
	{
		//newRoom is already set -bail out and pretend we weren't called
	 	delete this;
		return;
	}

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
	{
		mode = M_MODELESS;
	}
	else
	{
		mode = M_MODAL;
		bGlobal.roomMgr.curRoom->Pause(true);
	}

	gbackAnim = ALoad(RES_ANIM,40);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(89,92,89-1+pbackAnimCH->width,92-1+pbackAnimCH->height);

	if(bGlobal.storyLine != NETGAME)
	{	//====================================================
		// 0 percent fade down -just remaps colors to blue-gray range.
		fadeTo.Setup(320,400,OPTION_BASE_PRI - 10,gSelf,gPal,0,&rback);
	}	//====================================================

	// setup background cel filled with black
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,OPTION_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,40,1,false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(89,92);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(OPTION_BASE_PRI);

	pMouse->SetLimits(&rback);


	if(bGlobal.storyLine == NETGAME)
	{
		buttonNum = 4;
		bVal[0] = SAVE_BUTTON;
		//bVal[1] = LOAD_BUTTON;	//for now, load may only be done from netchar
		bVal[1] = OPTIONS_BUTTON;
		bVal[2] = CONTINUE_BUTTON;
		bVal[3] = RESIGN_BUTTON;
	}
	else
	{
		switch(bGlobal.roomMgr.curRoomNum)
		{
			case BR_STORY:
				prevRoomNum = bGlobal.roomMgr.prevRoomNum;
				if (prevRoomNum == BR_CHAR)
				{
					buttonNum = 5;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					//bVal[2] = NEW_STORY_BUTTON;
					bVal[2] = OPTIONS_BUTTON;
					bVal[3] = CONTINUE_BUTTON;
					bVal[4] = LEAVE_BUTTON;
				}
				else
				{
					buttonNum = 5;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					bVal[2] = OPTIONS_BUTTON;
					bVal[3] = CONTINUE_BUTTON;
					bVal[4] = LEAVE_BUTTON;
				}
				break;

			case BR_WORLD:
				buttonNum = 6;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				bVal[2] = REPLAY_BUTTON;
				//bVal[3] = NEW_STORY_BUTTON;
				bVal[3] = OPTIONS_BUTTON;
				bVal[4] = CONTINUE_BUTTON;
				bVal[5] = LEAVE_BUTTON;
				break;

			case BR_CHAR:
				buttonNum = 5;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				//bVal[2] = NEW_STORY_BUTTON;
				bVal[2] = OPTIONS_BUTTON;
				bVal[3] = CONTINUE_BUTTON;
				bVal[4] = LEAVE_BUTTON;
				break;

			case BR_CHOOSE:
				buttonNum = 5;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				//bVal[2] = NEW_STORY_BUTTON;
				bVal[2] = OPTIONS_BUTTON;
				bVal[3] = CONTINUE_BUTTON;
				bVal[4] = LEAVE_BUTTON;
				break;

			case BR_ASSESS:
				buttonNum = 6;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				bVal[2] = REPLAY_BUTTON;
				//bVal[3] = NEW_STORY_BUTTON;
				bVal[3] = OPTIONS_BUTTON;
				bVal[4] = CONTINUE_BUTTON;
				bVal[5] = LEAVE_BUTTON;
				break;

			case BR_HALL:
				prevRoomNum = bGlobal.roomMgr.prevRoomNum;
				if (prevRoomNum == BR_STORY || prevRoomNum == BR_MENU)
				{
					buttonNum = 5;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					bVal[2] = OPTIONS_BUTTON;
					bVal[3] = CONTINUE_BUTTON;
					bVal[4] = LEAVE_BUTTON;
				}
				else
				{
					buttonNum = 6;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					bVal[2] = REPLAY_BUTTON;
					//bVal[3] = NEW_STORY_BUTTON;
					bVal[3] = OPTIONS_BUTTON;
					bVal[4] = CONTINUE_BUTTON;
					bVal[5] = LEAVE_BUTTON;
				}
				break;

			default:
				pMono->Out("Warning: Unknown Previous Room!");
				buttonNum = 5;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				bVal[2] = OPTIONS_BUTTON;
				bVal[3] = CONTINUE_BUTTON;
				bVal[4] = LEAVE_BUTTON;
				break;
		}
	}


	// center buttons vertically
	// height of button caps = 3, height of buttons = 17
	// (number of caps = 4)*(cap size = 3) + (button gap = 6) = (total of 18)
	// master Y for background is 92
	//
	int topMargin,bottomCap1,secondTop,bottomCap2;
	capPosY[0] = topMargin  = 92 + (210-(18+(17*buttonNum)))/2;
	capPosY[1] = bottomCap1 = capPosY[0] + 3 + (17*(buttonNum - 2));
	capPosY[2] = secondTop  = capPosY[1] + 3 + 6;
	capPosY[3] = bottomCap2 = capPosY[2] + 3 + (17 * 2);

	//set button caps
	CopyCel(pbackCH,112-89,capPosY[0]-92,RES_ANIM,40,2,false);
	CopyCel(pbackCH,112-89,capPosY[1]-92,RES_ANIM,40,3,false);
	CopyCel(pbackCH,112-89,capPosY[2]-92,RES_ANIM,40,2,false);
	CopyCel(pbackCH,112-89,capPosY[3]-92,RES_ANIM,40,3,false);

	char *pButtonName; //temp. vars.
	int	tbVal;
	
	//Button Setup
	for(x=0;x<buttonNum;x++)
	{
		pButton = &button[x];

		if (x >= buttonNum - 2)
		{
			buttonY = topMargin + 15 + (x*17);
		}
		else
		{
			buttonY = topMargin + 3 + (x*17);
		}

		if (x % 2 == 0)
			pButton->Create(112, buttonY, OPTION_BASE_PRI + 1, RES_ANIM, 42, 1, gSelf, 89, 92);
		else
			pButton->Create(112, buttonY, OPTION_BASE_PRI + 1, RES_ANIM, 44, 1, gSelf, 89, 92);

		tbVal = bVal[x];
		if      (tbVal == RESIGN_BUTTON)   pButtonName = sqbOption.Load(OPTION_SQB,41);
		else if (tbVal == OPTIONS_BUTTON)  pButtonName = (char*)"OPTIONS";
		else                               pButtonName = sqbOption.Load(OPTION_SQB,tbVal+1);

		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = false;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetCelText(1, pButtonName);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pButtonName);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(false);
	}


	Activate(true);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);

	gCurControl = grip{};									// grip of currently active button, if any

}


bool
OptionMenu::HandleMsg(Message* pMsg)
{
	char				mess[100];
	int				x;
	SubOptionMenu	*pSub;
	SaveMenu			*pSave;

	// pass on to receivers first
	if (BAM_Room::HandleMsg(pMsg))
	{
		return(true);
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
					if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
					{
						for(x=0;x<buttonNum;x++)
						{
							if(pMsg->notice.gSource == button[x].gSelf)
							{
								switch (bVal[x])
								{
									case SAVE_BUTTON:

											pMono->Out("\nSave Button was hit");
											if(bGlobal.storyLine == NETGAME)
											{
												//only one net save "slot" 
												netSaveNum = (uint16) NET_SAVEGAME_NUM;

												//need to use ARandom and do this at main loop
												//netSerialNum = ARandom2(99999);
												//snprintf(saveMessage, sizeof(saveMessage),"%d",netSerialNum);

												Cleanup();
											}
											else
											{
												pSave = new SaveMenu;
												pSave->Setup(gSelf,SAVE_BUTTON);
											}
										return true;
										break;
									case LOAD_BUTTON:

											pMono->Out("\nLoad Button was hit");
											pSave = new SaveMenu;
											pSave->Setup(gSelf,LOAD_BUTTON);
										return true;
										break;
									case REPLAY_BUTTON:
										pMono->Out("\nReplay Button was hit");
										pSub = new SubOptionMenu;
										pSub->Setup(gSelf,REPLAY_BUTTON);
										return true;
										break;
									case NEW_STORY_BUTTON:
										pMono->Out("\nNew Story Button was hit");
										pSub = new SubOptionMenu;
										pSub->Setup(gSelf,NEW_STORY_BUTTON);
										return true;
										break;
									case OPTIONS_BUTTON: {
										OptionsMenu *pOptions = new OptionsMenu;
										pOptions->Setup();
										return true;
									}
									case CONTINUE_BUTTON:
										pMono->Out("\nContinue Button was hit");
										Cleanup();
										return true;
										break;
									case RESIGN_BUTTON:
									case LEAVE_BUTTON:
										pMono->Out("\nLeave Button was hit");
										//fadeTo.FadeUp();
										//bGlobal.roomMgr.curRoom->Pause(false);
										//pBam->Quit();
										pSub = new SubOptionMenu;
										pSub->Setup(gSelf,LEAVE_BUTTON);
										return true;
										break;
									default:
            						snprintf(mess, sizeof(mess), "Unrecognized button type: %d", pMsg->notice.param);
            						APanic(mess);
										break;
								}
							}
						}

					}
					else
         		{
            		snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
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
								return(true);
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
									return true;
								}
								break;

							case K_ESC:
							case K_Q:
							case K_C:	//continue
								Cleanup();
								return true;
								break;
						}
				}
				break;
		}//endswitch
	}
	return true;	//no pass-thru
}


void
OptionMenu::Cleanup()
{
	
	if(bGlobal.storyLine != NETGAME)
	{
		fadeTo.FadeUp();
		bGlobal.roomMgr.curRoom->Pause(false);
	}

	Activate(false);

	delete this;
}

