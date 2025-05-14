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
#include "Option.hpp"
#include "Option2.hpp"
#include "Option3.hpp"
#include "savemenu.hpp"

#include <string.h>

#define	OPTION_BASE_PRI	20000
#define MM_MAIN_SQB		7010

//=========================================================

OptionMenu::OptionMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
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
		bGlobal.roomMgr.curRoom->Pause(TRUE);
	}

	TRACK_MEM("Option: background anim");
	gbackAnim = ALoad(RES_ANIM,40);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(89,92,89-1+pbackAnimCH->width,92-1+pbackAnimCH->height);

	if(bGlobal.storyLine != NETGAME)
	{	//====================================================
		// 0 percent fade down -just remaps colors to blue-gray range.
		fadeTo.Setup(320,400,OPTION_BASE_PRI - 10,gSelf,gPal,0,&rback);
	}	//====================================================

	// setup background cel filled with black
	TRACK_MEM("Option: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,OPTION_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,40,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(89,92);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(OPTION_BASE_PRI);

	pMouse->SetLimits(&rback);


	if(bGlobal.storyLine == NETGAME)
	{
		buttonNum = 5;
		bVal[0] = SAVE_BUTTON;
		//bVal[1] = LOAD_BUTTON;	//for now, load may only be done from netchar
		bVal[1] = SOUND_BUTTON;
		bVal[2] = MUSIC_BUTTON;
		bVal[3] = CONTINUE_BUTTON;
		bVal[4] = RESIGN_BUTTON;
	}
	else
	{
		switch(bGlobal.roomMgr.curRoomNum)
		{
			case BR_STORY:
				prevRoomNum = bGlobal.roomMgr.prevRoomNum;
				if (prevRoomNum == BR_CHAR)
				{
					buttonNum = 6;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					//bVal[2] = NEW_STORY_BUTTON;
					bVal[2] = SOUND_BUTTON;
					bVal[3] = MUSIC_BUTTON;
					bVal[4] = CONTINUE_BUTTON;
					bVal[5] = LEAVE_BUTTON;
				}
				else
				{
					buttonNum = 6;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					bVal[2] = SOUND_BUTTON;
					bVal[3] = MUSIC_BUTTON;
					bVal[4] = CONTINUE_BUTTON;
					bVal[5] = LEAVE_BUTTON;
				}
				break;

			case BR_WORLD:
				buttonNum = 7;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				bVal[2] = REPLAY_BUTTON;
				//bVal[3] = NEW_STORY_BUTTON;
				bVal[3] = SOUND_BUTTON;
				bVal[4] = MUSIC_BUTTON;
				bVal[5] = CONTINUE_BUTTON;
				bVal[6] = LEAVE_BUTTON;
				break;

			case BR_CHAR:
				buttonNum = 6;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				//bVal[2] = NEW_STORY_BUTTON;
				bVal[2] = SOUND_BUTTON;
				bVal[3] = MUSIC_BUTTON;
				bVal[4] = CONTINUE_BUTTON;
				bVal[5] = LEAVE_BUTTON;
				break;

			case BR_CHOOSE:
				buttonNum = 6;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				//bVal[2] = NEW_STORY_BUTTON;
				bVal[2] = SOUND_BUTTON;
				bVal[3] = MUSIC_BUTTON;
				bVal[4] = CONTINUE_BUTTON;
				bVal[5] = LEAVE_BUTTON;
				break;

			case BR_ASSESS:
				buttonNum = 7;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				bVal[2] = REPLAY_BUTTON;
				//bVal[3] = NEW_STORY_BUTTON;
				bVal[3] = SOUND_BUTTON;
				bVal[4] = MUSIC_BUTTON;
				bVal[5] = CONTINUE_BUTTON;
				bVal[6] = LEAVE_BUTTON;
				break;

			case BR_HALL:
				prevRoomNum = bGlobal.roomMgr.prevRoomNum;
				if (prevRoomNum == BR_STORY || prevRoomNum == BR_MENU)
				{
					buttonNum = 6;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					bVal[2] = SOUND_BUTTON;
					bVal[3] = MUSIC_BUTTON;
					bVal[4] = CONTINUE_BUTTON;
					bVal[5] = LEAVE_BUTTON;
				}
				else
				{
					buttonNum = 7;
					bVal[0] = SAVE_BUTTON;
					bVal[1] = LOAD_BUTTON;
					bVal[2] = REPLAY_BUTTON;
					//bVal[3] = NEW_STORY_BUTTON;
					bVal[3] = SOUND_BUTTON;
					bVal[4] = MUSIC_BUTTON;
					bVal[5] = CONTINUE_BUTTON;
					bVal[6] = LEAVE_BUTTON;
				}
				break;

			default:
				pMono->Out("Warning: Unknown Previous Room!");
				buttonNum = 6;
				bVal[0] = SAVE_BUTTON;
				bVal[1] = LOAD_BUTTON;
				bVal[2] = SOUND_BUTTON;
				bVal[3] = MUSIC_BUTTON;
				bVal[4] = CONTINUE_BUTTON;
				bVal[5] = LEAVE_BUTTON;
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
	CopyCel(pbackCH,112-89,capPosY[0]-92,RES_ANIM,40,2,FALSE);
	CopyCel(pbackCH,112-89,capPosY[1]-92,RES_ANIM,40,3,FALSE);
	CopyCel(pbackCH,112-89,capPosY[2]-92,RES_ANIM,40,2,FALSE);
	CopyCel(pbackCH,112-89,capPosY[3]-92,RES_ANIM,40,3,FALSE);

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
		if(tbVal == RESIGN_BUTTON)
			pButtonName = sqbOption.Load(OPTION_SQB,41);	//OK, I should of left room in the sqb -sue me!
		else
			pButtonName = sqbOption.Load(OPTION_SQB,tbVal+1);

		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetCelText(1, pButtonName);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pButtonName);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Select(FALSE);
	}


	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;									// grip of currently active button, if any

}


bool
OptionMenu::HandleMsg(Message* pMsg)
{
	char				mess[100];
	int				x;
	NoiseMenu		*pNoise;
	SubOptionMenu	*pSub;
	SaveMenu			*pSave;
	BAM_ErrorPopup *pPop;

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
									case SAVE_BUTTON:
										#ifdef INTERACTIVE_DEMO
										TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
										pPop->SetWindowOffsets(89,92);
										pPop->Setup(NULL,MM_MAIN_SQB,201);
										return(TRUE);
										#endif

										#ifdef OS_MAC
											// on Mac, just set saveNum, so make loop 
											// will call saveMgr.Save() which will do the rest
											//pMono->Out("Not implemented on the Mac, yet");
											saveNum = 1;
										#else
											pMono->Out("\nSave Button was hit");
											if(bGlobal.storyLine == NETGAME)
											{
												//only one net save "slot" 
												netSaveNum = (uint16) NET_SAVEGAME_NUM;

												//need to use ARandom and do this at main loop
												//netSerialNum = ARandom2(99999);
												//sprintf(saveMessage,"%d",netSerialNum);

												Cleanup();
											}
											else
											{
												TRACK_MEM("SaveMenu");	pSave = new SaveMenu;
												pSave->Setup(gSelf,SAVE_BUTTON);
											}
										#endif
										return TRUE;
										break;
									case LOAD_BUTTON:
										#ifdef INTERACTIVE_DEMO
										TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
										pPop->SetWindowOffsets(89,92);
										pPop->Setup(NULL,MM_MAIN_SQB,201);
										return(TRUE);
										#endif

										#ifdef OS_MAC
											// on Mac, just set saveNum, so make loop 
											// will call saveMgr.Save() which will do the rest
											//pMono->Out("Not implemented on the Mac, yet");
											restoreNum = 1;
										#else
											pMono->Out("\nLoad Button was hit");
											TRACK_MEM("RestoreMenu");	pSave = new SaveMenu;
											pSave->Setup(gSelf,LOAD_BUTTON);
										#endif
										return TRUE;
										break;
									case REPLAY_BUTTON:
										pMono->Out("\nReplay Button was hit");
										TRACK_MEM("SubOptionMenu");	pSub = new SubOptionMenu;
										pSub->Setup(gSelf,REPLAY_BUTTON);
										return TRUE;
										break;
									case NEW_STORY_BUTTON:
										pMono->Out("\nNew Story Button was hit");
										TRACK_MEM("SubOptionMenu");	pSub = new SubOptionMenu;
										pSub->Setup(gSelf,NEW_STORY_BUTTON);
										return TRUE;
										break;
									case SOUND_BUTTON:
										pMono->Out("\nSound Button was hit");
										TRACK_MEM("Noise");	pNoise = new NoiseMenu;
										pNoise->Setup(SOUND_BUTTON);
										return TRUE;
										break;
									case MUSIC_BUTTON:
										pMono->Out("\nMusic Button was hit");
										TRACK_MEM("Noise");	pNoise = new NoiseMenu;
										pNoise->Setup(MUSIC_BUTTON);
										return TRUE;
										break;
									case CONTINUE_BUTTON:
										pMono->Out("\nContinue Button was hit");
										Cleanup();
										return TRUE;
										break;
									case RESIGN_BUTTON:
									case LEAVE_BUTTON:
										pMono->Out("\nLeave Button was hit");
										//fadeTo.FadeUp();
										//bGlobal.roomMgr.curRoom->Pause(FALSE);
										//pBam->Quit();
										TRACK_MEM("SubOptionMenu");	pSub = new SubOptionMenu;
										pSub->Setup(gSelf,LEAVE_BUTTON);
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
							case K_C:	//continue
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
OptionMenu::Cleanup()
{
	
	if(bGlobal.storyLine != NETGAME)
	{
		fadeTo.FadeUp();
		bGlobal.roomMgr.curRoom->Pause(FALSE);
	}

	Activate(FALSE);

	delete this;
}

