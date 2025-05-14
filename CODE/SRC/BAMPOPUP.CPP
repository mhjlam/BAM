//	BAMPOPUP.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Bam Popup Class
//
// This is the basic popup class for SOME of the game's popups.
//

#include "BAMPOPUP.HPP"

#include "api.hpp"
#include "apievt.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "mouse.hpp"
#include "resource.hpp"
#include "scrimage.hpp"

#include <string.h>

//default defines
#define	POPUP_BASE_PRI	20000000
#define	SCREEN_WIDTH	320
#define	SCREEN_HEIGHT	400

#define	BACK_ANI			40
#define	BUTTON_ANIM_A	42
#define	BUTTON_ANIM_B	44

#define	WIN_OFF_X		89
#define	WIN_OFF_Y		92
//offset of buttons into background cel
#define	BUTTON_START_X 23
#define	BUTTON_START_Y 127

//=========================================================

BAM_Popup::BAM_Popup()
{
	msgMask = E_KEY_DOWN | E_KEY_UP | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	rback.Set(0,0,0,0);
	backAni = BACK_ANI;
	popupPri = POPUP_BASE_PRI;
	fadeOn = FALSE;
	pause = FALSE;
	explicitNet = -1;
	explicitPause = -1;

	explicitMode = M_INACTIVE;

	winOffX = WIN_OFF_X;
	winOffY = WIN_OFF_Y;

	textSqbRes = 0;
	textSqbNum = 0;
	textJust = DG_JUST_CENTER;
}

BAM_Popup::~BAM_Popup()
{
	if(prevFont != 9050)
		pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);
	pMouse->ForceShow((int16)oldMouseHideCnt);
}

void
BAM_Popup::Setup(grip gPal, char* pMsg)
{
	BAM_Guy		*pGuy;

	//======================================================
	prevFont = pFontMgr->curFontNum;
	if(prevFont != 9050)
		pFontMgr->SetRes(9050);

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start BAM_Popup::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
	{
		mode = M_MODELESS;
		BAM_Room::SetBasicNet(TRUE);
	}
	else
	{
		mode = M_MODAL;
		pause = TRUE;

	}

	//explicit override
	if(explicitMode != M_INACTIVE)
	{
		mode = explicitMode;
		if(mode == M_MODELESS)
			BAM_Room::SetBasicNet(TRUE);
	}

	if(explicitNet != -1)
		BAM_Room::SetBasicNet((bool)explicitNet);

	if(explicitPause != -1)
		pause = (bool) explicitPause;

	if(pause)
		bGlobal.roomMgr.curRoom->Pause(TRUE);

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);
	oldMouseHideCnt = pMouse->ForceShow(0);

	TRACK_MEM("BAM_Popup: background anim");
	gbackAnim = ALoad(RES_ANIM,backAni);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);
	rback.Set(winOffX,winOffY,winOffX-1+pbackAnimCH->width,winOffY-1+pbackAnimCH->height);

	//====================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	if (fadeOn)
		fadeTo.Setup(SCREEN_WIDTH,SCREEN_HEIGHT,popupPri - 1,gSelf,gPal,0,&rback);
	//====================================================

	//====================================================
	// setup background cel filled with black
	TRACK_MEM("BAM_Popup: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,popupPri);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,backAni,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(winOffX,winOffY);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(popupPri);

	pMouse->SetLimits(&rback);

	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	char	*pTxt;
	if(!pMsg)
	{
		if(!textSqbRes || !textSqbNum)
	  		APanic("Bad sqb info passed to popup");
		pTxt = sqbPopup.Load(textSqbRes,textSqbNum);
	}
	else
	{
		pTxt = pMsg;
	}

	ASetString(20,60,pTxt, pback, pbackCH->width-40, NULL, textJust);

	//No button setup stuff here -overload this function and do it yourself
}


void
BAM_Popup::ActivatePopup()
{
	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;		// grip of currently active button, if any
}


bool
BAM_Popup::HandleMsg(Message* pMsg)
{
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
				return( HandleNotice(pMsg) );
				break;

			case MSG_EVENT:
				return( HandleEvent(pMsg) );
				break;
		}//endswitch
	}

	//eat all input regardless
	return TRUE;
}


bool
BAM_Popup::HandleNotice(Message *pMsg)
{
	pMsg = pMsg;
	pMono->Out("You need to overload this popup HandleNotice");

	//eat all input regardless
	return(TRUE);
}


bool
BAM_Popup::HandleEvent(Message *pMsg)
{
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
				case K_ESC:
				case K_Q:
					Cleanup();
					return TRUE;
					break;
			}
	}

	//eat all input regardless
	return(TRUE);
}


void
BAM_Popup::Cleanup()
{
	if(fadeOn)
		fadeTo.FadeUp();

	if(pause)
		bGlobal.roomMgr.curRoom->Pause(FALSE);

	Activate(FALSE);

	delete this;
}


void
BAM_Popup::SetAni(int ani)
{
	backAni = ani;
}


void
BAM_Popup::SetPri(int pri)
{
	popupPri = pri;
}


void
BAM_Popup::SetMode(modalType mode_P)
{
	explicitMode = mode_P;
}


void
BAM_Popup::SetWindowOffsets(int x, int y)
{
	winOffX = x;
	winOffY = y;
}


void
BAM_Popup::SetFade(bool on)
{
	fadeOn = on;
}


void
BAM_Popup::SetTextSqb(int sqbRes,int sqbNum,justify just)
{
	textSqbRes = sqbRes;
	textSqbNum = sqbNum;
	textJust = just;
}


void
BAM_Popup::SetBasicNet(bool fPos)
{
	explicitNet = fPos;
}


void
BAM_Popup::SetPause(bool fPos)
{
	explicitPause = fPos;
}


//=======================================================================
//=======================================================================


BAM_DefaultPopup::BAM_DefaultPopup(int buttonNum_P)
{
	buttonNum = buttonNum_P;

	TRACK_MEM("BAM_Popup1 buttons"); pBUTTON = new BAM_Button[buttonNum];

	TRACK_MEM("BAM_Popup1 sqb res arr"); pSqbRes = new int[buttonNum];
	TRACK_MEM("BAM_Popup1 sqb res arr"); pSqbNum = new int[buttonNum];

	buttonStartX = BUTTON_START_X;
	buttonStartY = BUTTON_START_Y;

}

BAM_DefaultPopup::~BAM_DefaultPopup()
{
	delete [] pBUTTON;
	delete pSqbRes;
	delete pSqbNum;
}


void
BAM_DefaultPopup::Setup(grip gPal,char* pMsg)
{
	BAM_Button  *pButton;
	int			i,buttonX,buttonY;
	char			*pTxt;

	//======================================================
	BAM_Popup::Setup(gPal,pMsg);
	//======================================================

	//reset button pos now as win offset may have been prev changed
	buttonX = buttonStartX + winOffX;
	buttonY = buttonStartY + winOffY;

	//---------------------------------------
	//Cap setups

	//set button cap
	CopyCel(pbackCH,buttonX-winOffX,buttonY-winOffY,RES_ANIM,40,2,FALSE);
	buttonY += 3;

	//---------------------------------------
	//Setup buttons
	for(i=0;i<buttonNum;i++)
	{
		pButton = &pBUTTON[i];
		pButton->Create(buttonX, buttonY, popupPri + 1, RES_ANIM, 42, 1, gSelf, winOffX, winOffY);
		buttonY += 17;
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);

		if(!pSqbRes[i] || !pSqbNum[i])
			APanic("Bad sqb info passed to popup");
		pTxt = sqbPopup.Load(pSqbRes[i],pSqbNum[i]);
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Draw();
	}

	//---------------------------------------
	//Cap setups
	CopyCel(pbackCH,buttonX-winOffX,buttonY-winOffY,RES_ANIM,40,3,FALSE);

}

void
BAM_DefaultPopup::SetButtonStartOffsets(int x, int y)
{
 	buttonStartX = x;
	buttonStartY = y;
}


void
BAM_DefaultPopup::SetButtonSqb(int buttonNum,int sqbRes,int sqbNum)
{
	pSqbRes[buttonNum] = sqbRes; 	
	pSqbNum[buttonNum] = sqbNum; 	
}


//=======================================================================
//=======================================================================
//Note: an existing error popup receiving a packet
//will destruct itself and create a new popup with the new message
BAM_ErrorPopup::BAM_ErrorPopup() : BAM_DefaultPopup(1)
{
}


void
BAM_ErrorPopup::Setup(grip gPal,int sqbRes,int sqbNum,int room)
{
	SetTextSqb(sqbRes,sqbNum,DG_JUST_CENTER);
	BAM_ErrorPopup::Setup(gPal,(char*)NULL,room);

}


void
BAM_ErrorPopup::Setup(grip gPal,char* pMsg,int room)
{
	toRoom = room;

	SetFade(FALSE);
	SetButtonSqb(0,OPTION_SQB,23); //'DONE' text
	SetButtonStartOffsets(23,145);
	BAM_DefaultPopup::Setup(gPal,pMsg);

	BAM_Popup::ActivatePopup();
}


bool
BAM_ErrorPopup::HandleNotice(Message *pMsg)
{
	if (pMsg->notice.type == N_CONTROL_REPLY)
	{
		if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
		{
			if(pMsg->notice.gSource == pBUTTON[0].gSelf)
			{
				pMono->Out("ok button was hit\n");
				if(toRoom)
					bGlobal.roomMgr.NewRoom(toRoom);
				Cleanup();				
				return(TRUE);
			}
		}
	}

	//eat all input regardless
	return(TRUE);
}

//we have are own Cycle() to detect self-destructs
//we don't want error popups to "stack up" on the screen
void	
BAM_ErrorPopup::Cycle()
{

	Context::Cycle();

	if(basicNet && !bGlobal.netDisconnect)
	{
		bsPacket.pData = NULL;

		//pass our toRoom on to the next error popup -if one
		if(NetHandleMsg(&bsPacket,toRoom))
			Cleanup();	//self-destruct
	}
}


//=======================================================================
//=======================================================================
BAM_WorldEnderPopup::BAM_WorldEnderPopup() : BAM_DefaultPopup(2)
{
}


void
BAM_WorldEnderPopup::Setup()
{
	

	SetFade(FALSE);
	SetPause(TRUE);
	SetBasicNet(FALSE);
	SetMode(M_MODAL);

	//at this point Van's code has only set bGlobal.netDisconnect
	//if the connection failed. We can also get here if the other
	//player delibrately left.
	if(bGlobal.netDisconnect)
	{
		fConnFail = TRUE;
		SetTextSqb(POPUP_SQB,3,DG_JUST_CENTER); //'Conn. failed. Play...'
		SetButtonSqb(0,POPUP_SQB,5); //'PLAY COMPUTER' text
		SetButtonSqb(1,OPTION_SQB,36); //'QUIT GAME' text
	}
	else
	{
		fConnFail = FALSE;
		SetTextSqb(POPUP_SQB,4,DG_JUST_CENTER); //'Opponent quit. Play...'
		SetButtonSqb(0,POPUP_SQB,5); //'PLAY COMPUTER' text
		SetButtonSqb(1,POPUP_SQB,6); //'DECLARE VICTORY' text
	}

	//lets now force this code to TRUE for all other code that relies on it
	bGlobal.netDisconnect = TRUE;

	SetButtonStartOffsets(23,145);
	BAM_DefaultPopup::Setup(NULL,(char*)NULL);

	BAM_Popup::ActivatePopup();
}


bool
BAM_WorldEnderPopup::HandleNotice(Message *pMsg)
{
	if (pMsg->notice.type == N_CONTROL_REPLY)
	{
		if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
		{
			if(pMsg->notice.gSource == pBUTTON[0].gSelf)
			{
				pMono->Out("Play Computer button was hit\n");

				pWorld->AITakeOver();

				pBam->fWorldEnderPopupExists = FALSE;
				Cleanup();				
				return(TRUE);
			}
			else
			if(pMsg->notice.gSource == pBUTTON[1].gSelf)
			{
				pMono->Out("Quit Game button was hit\n");

				if(fConnFail)
				{
					//connection failed -bail
					bGlobal.roomMgr.NewRoom(BR_MENU);
				}
				else
				{
					//declare victory
					pWorld->tWorldEnds = ATicks();
					pWorld->worldEnder = pWorld->playerSide;
				}

				pBam->fWorldEnderPopupExists = FALSE;
				Cleanup();				
				return(TRUE);
			}
		}
	}

	//eat all input regardless
	return(TRUE);
}


bool
BAM_WorldEnderPopup::HandleEvent(Message *pMsg)
{
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
				case K_P:
					FakeMouseClick(pBUTTON[0].gSelf);
					return TRUE;

				case K_ESC:
				case K_D:
				case K_Q:
					FakeMouseClick(pBUTTON[1].gSelf);
					return TRUE;
			}
	}

	//eat all input regardless
	return(TRUE);
}


//=======================================================================
//=======================================================================

BAM_TeleportPopup::BAM_TeleportPopup() : BAM_DefaultPopup(2)
{
	gTeleText = 0;
}


BAM_TeleportPopup::~BAM_TeleportPopup()
{
	if (gTeleText)
		AFree(gTeleText);
}


void
BAM_TeleportPopup::Setup(int sqbRes,int sqbNum)
{
	SetTextSqb(sqbRes,sqbNum,DG_JUST_CENTER); //"TELEPORT TO ROOM?" or "RUN CINEMATIC NUMBER?"
	SetButtonSqb(0,OPTION_SQB,39); //'OK' text
	SetButtonSqb(1,OPTION_SQB,21); //'cancel' text
	SetButtonStartOffsets(23,137);
	BAM_DefaultPopup::Setup(0,NULL);

	//set text box frame
	CopyCel(pbackCH,49,104,RES_ANIM,34,7,TRUE);

	TRACK_MEM("Teleport text");	gTeleText = AMalloc(8);
	pTeleText = ADerefAs(char, gTeleText);
	*pTeleText = '\0';
	BAM_Box	*pBox = &teleBox;
	pBox->SetColors(CI_SKIP,93,90,93,90,93,90,155,142);
	pBox->Create(52+winOffX,110+winOffY, 42, 13, popupPri + 1, gTeleText, 5, gSelf, rNumBack,winOffX,winOffY);
	pBox->SetupReplies(REPLY_DESELECTED);	// let us know when text changes
	//pBox->Select(TRUE); //makes the box edit active

	BAM_Popup::ActivatePopup();
}


bool
BAM_TeleportPopup::HandleNotice(Message *pMsg)
{
	int newRoom;

	if (pMsg->notice.type == N_CONTROL_REPLY)
	{
		if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
		{
			if(pMsg->notice.gSource == pBUTTON[0].gSelf)
			{
				pMono->Out("ok button was hit\n");
				newRoom = atoi(pTeleText);

				//is it a cinematic?
				if(newRoom > 14)
				{
					bGlobal.cinematic = newRoom;
					bGlobal.roomMgr.NewRoom(BR_CINE);
				}
				else
				if(newRoom)
					bGlobal.roomMgr.NewRoom(newRoom);
				Cleanup();				
				return(TRUE);
			}
			else
			if(pMsg->notice.gSource == pBUTTON[1].gSelf)
			{
				pMono->Out("cancel button was hit\n");
				Cleanup();				
				return(TRUE);
			}
			else
			if(pMsg->notice.gSource == teleBox.gSelf)
			{
				pMono->Out("teleBox was hit\n");
				return(TRUE);
			}
		}
	}

	//eat all input regardless
	return(TRUE);
}


//=======================================================================
//=======================================================================
//
//
//SamplePopup::SamplePopup() : BAM_DefaultPopup(1)
//{
//}
//
//
//void
//SamplePopup::Setup(grip gPal)
//{
//	SetTextSqb(CREDITS_SQB,10,DG_JUST_LEFT);
//	SetButtonSqb(0,OPTION_SQB,21); //'cancel' text
//	BAM_DefaultPopup::Setup(gPal,NULL);
//
//}
