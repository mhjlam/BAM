// SAVEMENU.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Save game room.
//
// 12/28/94
//

#include "api.hpp"
#include "apires.hpp"
#include "apifont.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include	"savemgr.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include "savemenu.hpp"

#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "bam_dg.hpp"
#include "mainmenu.hpp"
#include "option2.hpp"
#include "option3.hpp"
#include "story.hpp"

#include <string.h>

#define	SAVEMENU_BASE_PRI	20000

//=========================================================

SaveMenu::SaveMenu()
{

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
	buttonNum = 10;

	buttonFrameX = 40;
	textFrameX = 77;
	topMargin = 57;
	spaceY = 25;

	//set flags to show no valid save games exist until found 
	//and that no text boxes have been created yet
	memset(textBoxCreated,false,sizeof(textBoxCreated));
	memset(saveGameExist,false,sizeof(saveGameExist));

	memset(gsaveText,0,sizeof(gsaveText));
}

SaveMenu::~SaveMenu()
{
	int i;

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	if(menuType == SAVE_BUTTON)
	{
		for(i=0;i<buttonNum;i++)
		{
			if(gsaveText[i])
				AFree(gsaveText[i]);
		}
	}

	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}

void
SaveMenu::Setup(grip gprevContext_P,option_t menuType_P)
{
	BAM_Room		*pRoom;
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;
	int			x,buttonY,result;

	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	gprevContext = gprevContext_P;
	menuType = menuType_P;

	// room has already been paused
	//save current font and load new here? need to?

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start Option::Setup");

	mode = M_MODAL;

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	//======================================================
	gbackAnim = ALoad(RES_ANIM,8150);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(0,0,pbackAnimCH->width-1,pbackAnimCH->height-1);

	// setup background cel filled with black
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,SAVEMENU_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,8150,1,false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SAVEMENU_BASE_PRI);

	pMouse->SetLimits(&rback);

	//======================================================

	//	copy button frames into background cel
	for(x=0; x<buttonNum; x++)
	{
		CopyCel(pbackCH,buttonFrameX,topMargin+(spaceY*x)-3,RES_ANIM,8154,9,false);
	}

	//	copy text box frames into background cel
	for(x=0; x<buttonNum; x++)
	{
		CopyCel(pbackCH,textFrameX,topMargin+(spaceY*x),RES_ANIM,8154,8,false);
	}

	// cancel button frame
	CopyCel(pbackCH, buttonFrameX, 328,RES_ANIM,8154,7,false);


	char *pButtonName; //temp. var.
	
	//Save Buttons Setup
	for(x=0;x<buttonNum;x++)
	{
		pButton = &button[x];
		buttonY = topMargin + (spaceY*x);
		pButton->Create(buttonFrameX + 2, buttonY, SAVEMENU_BASE_PRI+10, RES_ANIM, 8159, 1, gSelf);

		pButtonName = sqbSave.Load(OPTION_SQB,11+x); //numbers on buttons

		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = false;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetTextOffSet(0, 0);
		pButton->SetCelText(1, pButtonName);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pButtonName);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Draw();
		//pButton->Listen(true);	//turn on click response -already in create()
	}

	pButton = &cancelButton;
	buttonY = topMargin + (spaceY*x);
	pButton->Create(buttonFrameX+2, 331, SAVEMENU_BASE_PRI+10, RES_ANIM, 8158, 1, gSelf);

	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetTextOffSet(0, 0);
	pButtonName = sqbSave.Load(OPTION_SQB,21);
	pButton->SetCelText(1, pButtonName);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pButtonName);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(true);	//turn on click response -already in create()


	BAM_Box	*pBox;

	//if no save games..
	if(saveMgr.GetFirstSave(fileName))
	{
		SetupEmptySlots();
	}
	else
	{
		do
		{
			result = saveMgr.GetSaveInfo(fileName, saveName, MAX_SAVE_TEXT_SIZE, &newVerNum, &newVerSubNum);
			if (result == SM_VERSION_ERROR)
			{
				// Incompatible save format — treat as empty slot, keep going.
				pMono->Out("SaveMenu: skipping incompatible save '%s'", fileName);
				continue;
			}
			if (result != 0)
	 		{
				// some sort of error
				// do something
				pMono->Out("SaveMenu: GetSaveInfo error: %d",result);
				BAM_ErrorPopup		*pPop;
				pPop = new BAM_ErrorPopup;
				pPop->Setup(grip{},MM_MAIN_SQB,17,BR_MENU);

				grip prevCtx = gprevContext;
				Cleanup();

				//lets remove calling option window
				if (prevCtx)
				{
					pRoom = ADerefAs(BAM_Room,prevCtx);
					pRoom->Cleanup();
				}

				return;
			}
			else
			{
				//at this point we should do some version checking


				//get save game num from filename
				x = GetSaveNum();

				if(x == 10)
				{
					//10 is really save game 11 -our net save game, so ignore
					continue;
				}

				saveGameExist[x] = true;

				if(menuType == SAVE_BUTTON)
				{
					// make text box for this save game
					gsaveText[x] = AMalloc(MAX_SAVE_TEXT_SIZE+1);
					char *pText = ADerefAs(char, gsaveText[x]);
					strcpy(pText,saveName);
					pBox = &saveBox[x];
					pBox->SetColors(CI_SKIP,64,74,64,74,64,74,155,142);
					pBox->Create(textFrameX+3, topMargin+(spaceY*x)+4, 233, 14, SAVEMENU_BASE_PRI+10,
									gsaveText[x], MAX_SAVE_TEXT_SIZE, gSelf, rNumBack,0,0);
					pBox->SetupReplies(REPLY_DESELECTED|REPLY_CANCELLED);	// let us know when text changes
					//pBox->Select(true); //makes the box edit active
					textBoxCreated[x] = true;
				}
				else
				{
					SetFontColors(CI_SKIP,64,74,64,74,64,74);
					//just write text directly into text box
					ASetString(textFrameX+3, topMargin+(spaceY*x)+4, saveName, (uchar *)pbackCH, pbackCH->width, 0);
				}
			}
		} while (saveMgr.GetNextSave(fileName) == 0);

		//create the buttons left with blanks
		SetupEmptySlots();

	}


	//=================
	// Write the necessary text into the background

	// Write screen 'title'
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK); //old color scheme

	if(menuType == SAVE_BUTTON)
		sqbNum = 1;
	else
		sqbNum = 2;
	pTxt = sqbSave.Load(OPTION_SQB,sqbNum); //"SAVE GAME" or "LOAD GAME"
	ASetString(0,330, pTxt, (uchar *)pbackCH, pbackCH->width, 0, DG_JUST_CENTER);

	GetStoryName(&sqbRes,&sqbNum,bGlobal.storyLine);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbSave.Load(sqbRes,sqbNum);
	ASetString(0,372, pTxt, (uchar *)pbackCH, pbackCH->width, 0, DG_JUST_CENTER);

	pFontMgr->SetRes(9050); //reset to default just for safety

	Activate(true);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);

	gCurControl = grip{};									// grip of currently active button, if any

}


void
SaveMenu::SetupEmptySlots()
{
	BAM_Box	*pBox;
	int		x;

	pMono->Out("no save files");	

	if(menuType == SAVE_BUTTON)
	{
		//create the buttons left with blanks
		for(x=0;x<buttonNum;x++)
		{
			if(!textBoxCreated[x])
			{
				gsaveText[x] = AMalloc(MAX_SAVE_TEXT_SIZE+1);
				char *pText = ADerefAs(char, gsaveText[x]);
				strcpy(pText,"");
				pBox = &saveBox[x];
				pBox->SetColors(CI_SKIP,64,74,64,74,64,74,155,142);
				pBox->Create(textFrameX+3, topMargin+(spaceY*x)+4, 233, 14, SAVEMENU_BASE_PRI+10,
								gsaveText[x], MAX_SAVE_TEXT_SIZE, gSelf, rNumBack,0,0);
				pBox->SetFont(9050);
				pBox->SetupReplies(REPLY_DESELECTED|REPLY_CANCELLED);	// let us know when text changes
				//pBox->Select(true); //makes the box edit active
				textBoxCreated[x] = true;
			}
		}
	}
}


int
SaveMenu::GetSaveNum()
{
	char		fn[14],*pfn;
	char		*pStr;
	int		x,cnt;

	//get save game num from filename
	pStr = strrchr(fileName,'\\');
	if(!pStr)
		pStr = strrchr(fileName, '/');
	if(!pStr)
		pStr = fileName;
	else
		pStr++; //get past separator

	cnt=0;
	pfn = fn;
	while (*pStr && *pStr != '.' && cnt < (int)sizeof(fn) - 1)
	{
		*pfn++ = *pStr++;
		cnt++;
	}
	*pfn = '\0';
	// array is 0-9 but save games are 1-10 so subtract 1
	x = atoi(fn);
	x--;

	return x;
}


bool
SaveMenu::HandleMsg(Message* pMsg)
{
	char			mess[100];
	char			*pText;
	uint32		x;
	BAM_Room		*pRoom;

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
						if(pMsg->notice.gSource == cancelButton.gSelf)
						{
							Cleanup();
							return true;
						}
						else
						{
							if(menuType == SAVE_BUTTON)
							{
								//did we get a click on a box?
								for(x=0;x<buttonNum;x++)
								{
									// this code only executes for a save game
									if(pMsg->notice.gSource == saveBox[x].gSelf)
									{
										pMono->Out("\nGot a save game message.");
										//we've got a request to save game

										pText = ADerefAs(char, gsaveText[x]);
										//if(saveMgr.Save((uint16)x,versionNum,versionSubNum,pText))
										//{
										//	pMono->Out("\nError saving game.");
										//}
										saveNum = (uint16)(x+1);
										strcpy(saveMessage,pText);

										grip prevCtx = gprevContext;
										Cleanup(); //this must be first to properly reset mouse boundaries

										//lets remove calling option window
										if (prevCtx)
										{
											pRoom = ADerefAs(BAM_Room,prevCtx);
											pRoom->Cleanup();
										}

										return(true);
									}
								}
							}

							//did we get a click on a button?
							for(x=0;x<buttonNum;x++)
							{
								if(menuType == SAVE_BUTTON)
								{
									if(pMsg->notice.gSource == button[x].gSelf)
									{
										snprintf(mess, sizeof(mess),"\nButton %d was hit.",x);
										pMono->Out(mess);
										//activate the corresponding text box
										saveBox[x].Select(true);
										return(true);
									}
								}
								else
								{
									if(pMsg->notice.gSource == button[x].gSelf && saveGameExist[x])
									{
										snprintf(mess, sizeof(mess),"\nButton %d was hit and restoreNum set.",x);
										pMono->Out(mess);
										//load this save game
										restoreNum = (uint16)(x+1);
										grip prevCtx = gprevContext;
										Cleanup(); //this must be first to properly reset mouse boundaries

										//lets remove calling option window
										if (prevCtx)
										{
											pRoom = ADerefAs(BAM_Room,prevCtx);
											pRoom->Cleanup();
										}

										return(true);
									}
								}
							}
						}
					}
					else
					if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_CANCELLED)
					{
						//do nothing -wait for user
						return(true);
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
					case E_KEY_DOWN:
						switch (pMsg->event.value)
						{
							case K_ESC:
								Cleanup();
								return true;
								break;
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
								return(true);
						}
						break;
				}
				break;
		}//endswitch
	}
	return false;
}


void
SaveMenu::Cleanup()
{
	Activate(false);

	delete this;
}

