//
// BAM_DG.CPP
//
// July 1, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include <ctype.h>
#include "apievt.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apires.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include "savebase.hpp"
#include "tigre.hpp"
#include "scrimage.hpp"
#include	"dialog.hpp"
#include "bam_dg.hpp"
#include "bamroom.hpp"
#include "bam.hpp"

#define	DEFAULT_POINT	10

//---------------------------------------------------------------------
// class BAM_Control
BAM_Control::BAM_Control(void)
{
	ReplyMask = 0;
	gContext = grip{};
	currState = false;
	rOwnerCel = 0;				// resNum of Cel we're to Draw() ourselves into
	scrim.SetLastRect(0, 0, 0, 0);
	msgMask = E_MOUSE_DOWN;
	fDeselectCurControl = true;
	font = 9050;
}

BAM_Control::~BAM_Control()
{
	// if replyMask & DELETE
	//		send DELETE reply
}

bool
BAM_Control::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&bamConDataStart, 1, (uintptr_t)&bamConDataEnd -
				(uintptr_t)&bamConDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&bamConDataStart, 1, (uintptr_t)&bamConDataEnd -
				(uintptr_t)&bamConDataStart, pFile);
			break;
	}
	return(true);
}

//----------------------------
// class BAM_Button

BAM_Button::BAM_Button(void)
{
	fIsToggle = false;			// default type is click-type 
	CelText[0][0] = '\0';		// each string corresponds to the cel of that #, && will be drawn upon that cel into the master cel
	CelText[1][0] = '\0';
	CelText[2][0] = '\0';
	CelTextHJust = DG_JUST_LEFT;
	CelTextVJust = DG_JUST_TOP;
	TextHOffSet = 0;
	TextVOffSet = 0;

	// init all the resource info to nullptr
	memset(celAnim, 0, sizeof(celAnim));

	offSetH = 0;
	offSetV = 0;
}

// constructor with auto-create, for the lazy typist
BAM_Button::BAM_Button(uint32 xPos, uint32 yPos, uint32 newPri, res_t resType,
					uint32 resNum, uint32 celNum, grip gNewContext,
					uint32 NewReplyMask,uint32 offSetH_P, uint32 offSetV_P)
{
	offSetH = offSetH_P;
	offSetV = offSetV_P;
	Create(xPos, yPos, newPri, resType, resNum, celNum, gNewContext);
	SetupReplies(NewReplyMask);
}

bool
BAM_Button::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&bamButtonDataStart, 1, (uintptr_t)&bamButtonDataEnd -
				(uintptr_t)&bamButtonDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&bamButtonDataStart, 1, (uintptr_t)&bamButtonDataEnd -
				(uintptr_t)&bamButtonDataStart, pFile);
			break;
	}
	return(true);
}

void
BAM_Control::SetOwnerCel(uint rNewOwnerCel)
{
	if(rOwnerCel != rNewOwnerCel)
	{
		rOwnerCel = rNewOwnerCel;
		SetState(S_CHANGED, true);
	}
}

bool
BAM_Button::SetButtonRes(int state, res_t newResType, uint32 newResNum, uint32 newCel)
{
	switch(state)
	{
		case false:
			if(celAnim[0].resType != newResType ||
				celAnim[0].resNum != newResNum ||
				celAnim[0].resCel != newCel)
			{
				celAnim[0].resType = newResType;
				celAnim[0].resNum = newResNum;
				celAnim[0].resCel = newCel;
				SetState(S_CHANGED, true);
			}
			break;
		
		case true:
			if(celAnim[1].resType != newResType ||
				celAnim[1].resNum != newResNum ||
				celAnim[1].resCel != newCel)
			{
				celAnim[1].resType = newResType;
				celAnim[1].resNum = newResNum;
				celAnim[1].resCel = newCel;
				SetState(S_CHANGED, true);
			}
			break;

		case DG_DISABLED:
			if(celAnim[2].resType != newResType ||
				celAnim[2].resNum != newResNum ||
				celAnim[2].resCel != newCel)
			{
				celAnim[2].resType = newResType;
				celAnim[2].resNum = newResNum;
				celAnim[2].resCel = newCel;
				SetState(S_CHANGED, true);
			}
			break;

	}
	return(true);
}

void
BAM_Button::SetCelText(uint32 celNum, const char* pNewText)
{
	char	string1[80], *pStr;
	int	newLineCount;

	celNum--;

	if(strncmp(&CelText[celNum][0], pNewText, 40))
	{
		strncpy(&CelText[celNum][0], pNewText, 40);

		// count # of lines in text
		strcpy(string1, pNewText);
		pStr = strtok(string1, "\n");
		newLineCount = 0;
		while(pStr)
		{
			newLineCount++;
			pStr = strtok(nullptr, "\n");
		}
		celTextLines[celNum] = newLineCount; 

		SetState(S_CHANGED, true);
	}
}

void
BAM_Button::SetColors(uint32 celNum, uint32 newColor1, uint32 newColor2, uint32 newBackColor)
{
	SetColors(celNum, newColor1, newColor2, newColor1, newColor2, newColor1, newColor2, newBackColor);
}

void
BAM_Button::SetColors(uint32 celNum, uint32 newColor1, uint32 newColor2,
							 uint32 newColor3, uint32 newColor4, uint32 newBackColor)
{
	SetColors(celNum, newColor1, newColor2, newColor3, newColor4, newColor1, newColor2, newBackColor);
}

void
BAM_Button::SetColors(uint32 celNum, uint32 newColor1, uint32 newColor2,
							 uint32 newColor3, uint32 newColor4,
							 uint32 newColor5, uint32 newColor6, uint32 newBackColor)
{
	uint32 temp = celNum - 1;

	CelTextColors[temp][0] = newBackColor;
	CelTextColors[temp][1] = newColor1;
	CelTextColors[temp][2] = newColor2;
	CelTextColors[temp][3] = newColor3;
	CelTextColors[temp][4] = newColor4;
	CelTextColors[temp][5] = newColor5;
	CelTextColors[temp][6] = newColor6;
}

void
BAM_Button::SetTextJustify(justify NewHJust, justify NewVJust)
{
	if(CelTextHJust != NewHJust || CelTextVJust != NewVJust)
	{
		CelTextHJust = NewHJust;
		CelTextVJust = NewVJust;
		SetState(S_CHANGED, true);
	}
}

void
BAM_Button::SetTextOffSet(int32 NewHOffSet, int32 NewVOffSet)
{
	if(TextHOffSet != NewHOffSet || TextVOffSet != NewVOffSet)
	{
		TextHOffSet = NewHOffSet;
		TextVOffSet = NewVOffSet;
		SetState(S_CHANGED, true);
	}
}

//Draw is changed!
// Why? Because up until now buttons have always been drawn into a DCEL which
//  was positioned at 0,0. This means screen pos. = DCEL pos. Now we need a
//  draw function which can take screen coords. and convert them to DCEL
//  coords. for when the DCEL is not at pos. 0,0.
void
BAM_Button::Draw(void)
{
	CelHeader	*pCel;
	uint32		x, y, ySize, currLine;
	char			string1[40], *pStr;
	
	if(!CheckState(S_CHANGED))
		return;

	if(!rOwnerCel)
	  return;

	prevFont = pFontMgr->curFontNum;
	if(prevFont != font)
		pFontMgr->SetRes(font);

	pCel = (CelHeader *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rOwnerCel));	// DCEL, so it's really a resource, not a cel per se.

	// copy ourselves into our owner's cel
	CopyCel(pCel, scrim.x - scrim.orgX - offSetH, scrim.y - scrim.orgY - offSetV,
		scrim.resType, scrim.resNum, scrim.cel, true, scrim.gClut);
	//	memset((uchar *)pCel + sizeof(CelHeader) + pCel->width * scrim.y + scrim.x, fIsActive?
	//	CI_WHITE: CI_BLACK, 32);

	// add text for this cel, if any
	memcpy(string1, &CelText[scrim.cel - 1][0], 39);
	string1[39] = '\0';
	pStr = strtok(string1, "\n");

	currLine = 0;
	while(pStr)
	{
		Rectangle	rTextSize;
		pFontMgr->TextSize(rTextSize, pStr, scrim.rect.Wide(), 0);

		switch(CelTextVJust)
		{
			case DG_JUST_TOP:
				y = scrim.y - scrim.orgY -offSetV + 2;
				break;

			case DG_JUST_BOTTOM:
				y = scrim.y + scrim.rect.High() - scrim.orgY - offSetV - rTextSize.y2 - 2;
				break;

			case DG_JUST_CENTER:
			default:
				// figure verticle middle
				y = scrim.y + (scrim.rect.High() / 2) - scrim.orgY - offSetV;

				// figure vert offset for this line
				ySize = (rTextSize.y2 / 2) * celTextLines[scrim.cel - 1] - (rTextSize.y2) * currLine;
				y -= (ySize + 2);

				// really really un-pretty hack for vertical placement of multi-line buttons
				if(celTextLines[scrim.cel - 1] > 1)
					y -= currLine * 2 - 1;
				break;
		}

		switch(CelTextHJust)
		{
			case DG_JUST_LEFT:
				x = scrim.x + 2 - scrim.orgX - offSetH;
				break;

			case DG_JUST_RIGHT:
				x = scrim.x + scrim.rect.Wide() - scrim.orgX - offSetH - rTextSize.x2 - 2;
				break;

			case DG_JUST_CENTER:
			default:
				x = scrim.x + (scrim.rect.Wide() / 2) - scrim.orgX -offSetH;
				x -= (rTextSize.x2 / 2) - 1;
				break;
		}
		x += TextHOffSet;
		y += TextVOffSet;

		int tempCelNum = scrim.cel - 1;
		SetFontColors(CelTextColors[tempCelNum][0], CelTextColors[tempCelNum][1], CelTextColors[tempCelNum][2],
			CelTextColors[tempCelNum][3], CelTextColors[tempCelNum][4],
			CelTextColors[tempCelNum][5], CelTextColors[tempCelNum][6]);
		pFontMgr->SetString(x, y + 4 - scrim.cel, pStr, (uchar *)pCel, pCel->width, 0);

		pStr = strtok(nullptr, "\n");
		currLine++;
	}

	if(prevFont != font)
		pFontMgr->SetRes(prevFont);

	AUpdateRect(&scrim.rect);
	SetState(S_CHANGED, false);
}

// auto-create Guy stuff
bool
BAM_Button::Create(uint32 xPos, uint32 yPos, uint32 newPri, res_t resType, 
						uint32 resNum, uint32 celNum,
						 grip gNewContext,uint32 offSetH_P, uint32 offSetV_P, int font_P)
{
	offSetH = offSetH_P;
	offSetV = offSetV_P;
	font = font_P;

	switch(resType)
	{
		case RES_CEL:
			SetButtonRes(false, resType, resNum, celNum);
			SetButtonRes(true, resType, resNum, celNum);
			SetButtonRes(DG_DISABLED, resType, resNum, celNum);
			break;

		default:
			SetButtonRes(false, resType, resNum, celNum);
			SetButtonRes(true, resType, resNum, celNum + 1);
			SetButtonRes(DG_DISABLED, resType, resNum, celNum + 2);
			break;
	}

	SetRes(resType, resNum, celNum);	// necessary because SetPos() needs cel origin X&Y
	SetPos(xPos, yPos);
//	SetRect();

	SetContext(gNewContext);
	Listen(true);		// listen for event msgs on input stream
	Select(false);
	//	Setup();
	SetPri(newPri);
	SetState(S_CHANGED, true);
	return(true);
}

/*void
BAM_Button::Hide(void)
{
	Context	*pCon;

	Listen(false);
}*/

/*void
BAM_Button::Show(void)
{
	Context	*pCon;

	pCon = ADerefAs(Context, gContext);
	// tell our context to give us msgs.  We only want CT_SERV (->Cycle() calls) if we're active
	pCon->AddObject(gSelf, CT_MSGS);
	
	// push to top of list
	pCon->lReceivers.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);
	Draw();
}*/

void
BAM_Button::Select(int newState)
{
	Context	*pCon;
	
	if(gContext)
	{
		pCon = ADerefAs(Context, gContext);
	} 
	else pCon = nullptr;

	switch(newState)
	{
		case false:
			if(!celAnim[0].resNum)
			{
				pMono->Out("BAM_Button error: no res specified for false state\n");
			}
			else
			{
				SetRes(celAnim[0].resType, celAnim[0].resNum, celAnim[0].resCel);
				Draw();	// for rOwnerCel mode
			}
			msgMask &= ~E_MOUSE_UP;	// button is OFF - we dont care about mouse_up's now	
			break;
		case true:
			if(!celAnim[1].resNum)
			{
				pMono->Out("BAM_Button error: no res specified for true state\n");
			}
			else
			{
				SetRes(celAnim[1].resType, celAnim[1].resNum, celAnim[1].resCel);
				Draw();	// for rOwnerCel mode
			}
			if(!fIsToggle)			// if click-type, then start Cycle()'ing && watching for MOUSE_UP
			{
				msgMask |= E_MOUSE_UP;		// button is ON - we do care about mouse_up's for the moment
//				pCon->AddObject(gSelf, CT_SERV);
	
				// move us to front of the line, to intercept MOUSE_UP
				pCon->lReceivers.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);
				if(fDeselectCurControl)
					(ADerefAs(BAM_Room,gContext))->SetCurrentControl(gSelf);	// in case we weren't already
			}
			break;
		case DG_DISABLED:
			if(!celAnim[2].resNum)
			{
				pMono->Out("BAM_Button error: no res specified for DISABLED state\n");
			}
			else
			{
				if(currState == true && !fIsToggle)
				{
					// disabling click-type button before MOUSE_UP has arrived.
					// clean up mid-click mode first

					Select(false);
				}
				SetRes(celAnim[2].resType, celAnim[2].resNum, celAnim[2].resCel);
				Draw();	// for rOwnerCel mode
			}
			Listen(false);
			break;
	}
	currState = newState;
}

void
BAM_Control::SetupReplies(uint32 newReplyMask, void *data)
{
	ReplyMask = newReplyMask;
	replyData = data;
}

void
BAM_Control::Listen(bool fListen)
{
	if(!gContext)
	{
		pMono->Out("BAM_Button::Listen() error - no owning context\n");
		return;
	}
	Context	*pCon = ADerefAs(Context, gContext);

	switch(fListen)
	{
		case true:
			pCon->AddObject(gSelf, CT_MSGS);		// tell our context to not give us msgs.  
			break;

		case false:
			pCon->CutObject(gSelf, CT_MSGS);		// tell our context to not give us msgs.  
			break;
	}
}

bool
BAM_Button::HandleMsg(Message *pMsg)
{
	Context		*pCon;
	CelHeader	*pCel;
	Resource		*pRes;
	uchar			*pCelData;
	grip			gRes;
	int			x, y;
	bool			fClaimed = false;
	BAM_Room		*pRoom;

	switch(pMsg->type)
	{
		case MSG_NOTICE:
			switch(pMsg->notice.type)
			{
				default:
					break;
			}
			break;
		case MSG_EVENT:
			switch(pMsg->event.type)
			{
				case E_MOUSE_UP:
					if(currState != true)
						break;

					pCon = ADerefAs(Context, gContext);
					if(scrim.rect.Contains(pMsg->event.x, pMsg->event.y))
					{
						gRes = ALoadDebug(__FILE__, __LINE__, scrim.resType, scrim.resNum);
						pRes = ADerefAs(Resource, gRes);
						pCel = (CelHeader *)pRes->GetHeader(scrim.cel);
						pCelData = AGetResData(gRes) + pRes->numHeaders * sizeof(CelHeader) + pCel->offset;

						// calc X,Y within our rect
						x = pMsg->event.x - scrim.x;
						y = pMsg->event.y - scrim.y;

						if(*(pCelData + x + (pCel->width * y)) != CI_SKIP)
						{
							// MOUSE_UP in our rectangle - do our thang
							if(fIsToggle)
							{
								// do nothing.  Toggle-type buttons are actuated by MOUSE_DOWN
								return(true);
							}
							if(pMsg->event.value != LEFT_BTN)
							{
								// do nothing.  MOUSE_UP w/ LEFT_BTN is all we care about.
								return(true);
							}
							else
							{	// only click-types use this stuff
								Select(false);		// click-type button, so turn select status off
								if(fDeselectCurControl)
									(ADerefAs(BAM_Room,gContext))->SetCurrentControl(grip{});	// we're not selected anymore
								pCon->CutObject(gSelf, CT_SERV);	// we dont want Cycle() calls anymore
							}

							// if someone wants to know we've been deselected
							if(ReplyMask & REPLY_DESELECTED)
								ASendNotice(N_CONTROL_REPLY, gContext, gSelf, (void *)REPLY_DESELECTED);	// let em know
							return(true);
						}
					}

					// not our click - should we clear ourselves?
					pRoom = ADerefAs(BAM_Room, gContext);
					if(pRoom->gCurControl == gSelf)
					{
						// note: this implies click-type button, since toggle-types never use CurControl.
						// They dont become "current", they just toggle on MOUSE_DOWN.
						Select(false);
						if(fDeselectCurControl)
							(ADerefAs(BAM_Room,gContext))->SetCurrentControl(grip{});	// we're not selected anymore
//						pCon->CutObject(gSelf, CT_SERV);	// we dont want Cycle() calls anymore
//						return(true);
					}
					break;
				case E_MOUSE_DOWN:
					if(scrim.rect.Contains(pMsg->event.x, pMsg->event.y))
					{	
						gRes = ALoadDebug(__FILE__, __LINE__, scrim.resType, scrim.resNum);
						pRes = ADerefAs(Resource, gRes);
						pCel = (CelHeader *)pRes->GetHeader(scrim.cel);
						pCelData = AGetResData(gRes) + pRes->numHeaders * sizeof(CelHeader) + pCel->offset;

						// calc X,Y within our rect
						x = pMsg->event.x - scrim.x;
						y = pMsg->event.y - scrim.y;

						if(*(pCelData + x + (pCel->width * y)) != CI_SKIP)
						{
							if(pMsg->event.value == LEFT_BTN)	// only actuate on LEFT_BTN.  Gobble but ignore other buttons.
							{
								Select((bool)!currState);	// 	then toggle
								// if someone wants to know we've been activated
								if(currState == true && (ReplyMask & REPLY_ACTIVATED))
									ASendNotice(N_CONTROL_REPLY, gContext, gSelf, (void *)REPLY_ACTIVATED);	// let em know
								// if someone wants to know we've been deactivated
								else if(currState != true && (ReplyMask & REPLY_DEACTIVATED))
									ASendNotice(N_CONTROL_REPLY, gContext, gSelf, (void *)REPLY_DEACTIVATED);	// let em know
		
								if(ReplyMask & REPLY_SELECTED_KEY)
								{
									// fake some keyboard input
									APostEvent(E_KEY_DOWN, (int32)(uintptr_t)replyData);
								}
							}
							return(true);
						}
					}
					break;			
				case E_KEY_DOWN:
					break;
			}
	}
	return(BAM_Control::HandleMsg(pMsg));
}


BAM_Button::~BAM_Button()
{
}

void
BAM_Button::Cycle(void)
{
	bool	fInside;
	Message	mousePos;

	pMouse->GetPos(&mousePos);
	fInside = scrim.rect.Contains(mousePos.event.x, mousePos.event.y);

	if(currState == true && !fInside)
	{
		Select(false);
	}
	else if(currState == false && fInside)
	{
		Select(true);
	}
}

void
BAM_Button::Listen(bool fListen)
{
	BAM_Control::Listen(fListen);

	if(fListen)
	{
		if(currState == DG_DISABLED)
			Select(false);		// kludge, auto-wakeup from disabled state
	}
}

//----------------------------
// class BAM_Box
BAM_Box::BAM_Box(void)
{
	gCel = grip{};
	gCel2 = grip{};
	gBack = grip{};
	editBuf[0] = '\0';
	currLen = 0;
	flashToggle = 1;
	boxMsgPriority = PRIORITY_MAX/2;
	gNextBox = grip{};
	rOwnerCel = 0;
	backCopyColor = -1;
}

BAM_Box::~BAM_Box()
{
	CelHeader	*pOwner;

	if(rOwnerCel)
	{
		if(AIsLoaded(RES_CEL,rOwnerCel))
		{
			pOwner = (CelHeader*) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rOwnerCel));
			CopyCel(pOwner,rOwner.x1,rOwner.y1,RES_CEL,rNumBack,1,false);
		}
	}

	if(gCel)
		AFlush(gCel);
	if(gCel2)
		AFlush(gCel2);
	if(gBack)
		AFlush(gBack);
}

void
BAM_Box::AddChar(char newByte)
{
	if(currLen < maxLen)
	{
		editBuf[currLen++] = newByte;
		editBuf[currLen] = '\0';
		Draw();
	}
}

void
BAM_Box::DelChar(void)
{
	if(currLen > 0)
	{
		currLen--;
		editBuf[currLen] = '\0';
		Draw();
	}
}

// copy string from current edit buf into buffer passed to us at creation
void
BAM_Box::ReturnString(void)
{
	char	*pText;

	pText = ADerefAs(char, gOrigText);
	//strncpy(pText, editBuf, maxLen); -no! a string can have maxLen chars
	// but we also have to copy the ending null char
	strcpy(pText,editBuf);
}

void
BAM_Box::SetNextBox(grip gNewNextBox)
{
	// when editing in current box is terminated by keyboard input, cause next box to be automatically
	// selected.
	gNextBox = gNewNextBox;
}

bool
BAM_Box::HandleMsg(Message *pMsg)
{
	char		char1, *pText;
	BAM_Box	*pBox;

	switch(pMsg->type)
	{
		case MSG_NOTICE:
			// let these pass through - they're more important;
			if(pMsg->notice.type)
				return(false);
			else return(false);

		case MSG_EVENT:
			switch(pMsg->event.type)
			{
				case E_MOUSE_UP:
//					if(currState == true)
//						return true; //if box is active then eat all input
					break;
				case E_MOUSE_DOWN:
					if(scrim.rect.Contains(pMsg->event.x, pMsg->event.y))
					{
						Select(true);
						return(true);
					}
					if(currState == true)
						return true; //if box is active then eat all input
					break;
				case E_KEY_DOWN:
					if(currState != true)		// if we're not currently editing, dont care about keystrokes
						break;
					switch(pMsg->event.value)
					{
						case K_RETURN:
							Select(false);
	
							ReturnString();
	
							if(ReplyMask & REPLY_DESELECTED)
								APostNotice(N_CONTROL_REPLY, gContext, (void *)REPLY_DESELECTED, gSelf);
	
							if(gNextBox)
							{
								// cause next box to be selected
								pBox = ADerefAs(BAM_Box, gNextBox);
								pBox->Select(true);
							}
							return(true);
	
						case K_ESC:
							Select(false);
	
							pText = ADerefAs(char,gOrigText); //restore orig text
							strcpy(editBuf, pText);
							currLen = strlen(editBuf);
							Draw();
	
							if(ReplyMask & REPLY_CANCELLED)
							{
								APostNotice(N_CONTROL_REPLY, gContext, (void *)REPLY_CANCELLED, gSelf);
							}
							else if(ReplyMask & REPLY_DESELECTED)
							{
								APostNotice(N_CONTROL_REPLY, gContext, (void *)REPLY_DESELECTED, gSelf);
							}
	
							if(gNextBox)
							{
								// cause next box to be selected
								pBox = ADerefAs(BAM_Box, gNextBox);
								pBox->Select(true);
							}
							return(true);
				
						case K_DEL:
						case K_BACK_SPACE:
							DelChar();
							return(true);
	
					}
					char1 = AScanToASCII(pMsg->event);
					switch(char1)
					{
						// filter unwanted chars
						case '_':
							break;
						default:
							if(isprint(char1))
								AddChar(char1);	// add a character to the string at current cursor pos
							break;
					}
					return(true);		// when we're active, grab all keyboard input.  Nothing gets through.
			}
			break;
	}
	return(BAM_Control::HandleMsg(pMsg));
}

void
BAM_Box::Select(bool fState)
{
	Context	*pCon;
	node*	pNode;

	pCon = ADerefAs(Context,gContext);
	switch(fState)
	{
		case true:
			currState = true;
			Draw(); // make it draw cursor
			pNode = pCon->lReceivers.Find(grip_to_ptr(gSelf));

			if (!pNode)	APanic("Bogus reset of priority");

			// save current msg priority before reset to max
			boxMsgPriority = pNode->key;
			//we should now have highest priority and immune to interruptions
			pCon->ResetMsgPri(gSelf,PRIORITY_MAX);
			break;
		
		case false:
			currState = false;
			Draw(); // make it draw without cursor
			pCon->ResetMsgPri(gSelf,boxMsgPriority);
			break;
	}
}

void
BAM_Box::Cycle(void)
{
	CelHeader	*pBackCel;
	Uint64		newTime;

	if(currState == true)
	{
		newTime = SDL_GetTicks();
		if(newTime > boxFlashTimer)
		{
			if (flashToggle)
			{
				flashToggle=0;
				pBackCel = (CelHeader*)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rOwnerCel));
				CopyCel(pBackCel, scrim.x-scrim.orgX-offSetH, scrim.y-scrim.orgY-offSetV, RES_CEL, rNumCel, 1, false);
				AUpdateRect(&scrim.rect);
			}
			else
			{
		 		flashToggle++;
				pBackCel = (CelHeader*)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rOwnerCel));
				CopyCel(pBackCel, scrim.x-scrim.orgX-offSetH, scrim.y-scrim.orgY-offSetV, RES_CEL, rNumCel2, 1, false);
				AUpdateRect(&scrim.rect);
			}
			boxFlashTimer = newTime + BOX_FLASH_INTERVAL;
		}
	}
}


void
BAM_Box::SetColors(uchar backColor_P, uchar foreColor1_P, uchar foreColor2_P,
						 uchar foreColor3_P, uchar foreColor4_P, uchar foreColor5_P,
						 uchar foreColor6_P, uchar cursorColor1_P, uchar cursorColor2_P)
{
	backColor = backColor_P;
	foreColor1 = foreColor1_P;
	foreColor2 = foreColor2_P;
	foreColor3 = foreColor3_P;
	foreColor4 = foreColor4_P;
	foreColor5 = foreColor5_P;
	foreColor6 = foreColor6_P;
	cursorColor1 = cursorColor1_P;
	cursorColor2 = cursorColor2_P;
}


void
BAM_Box::SetFont(int font_P)
{
	font = font_P;
}


void
BAM_Box::SetBackCopyColor(int color)
{
	backCopyColor = color;
}


void
BAM_Box::Create(uint32 xPos, uint32 yPos, uint32 maxWidth, uint32 maxHeight, int32 newPri, grip gMsgText,
	uint32 newMaxLen, grip gNewContext, uint rNewOwnerCel, uint32 offSetH_P, uint32 offSetV_P)
{
	CelHeader	*pBack;
	Uint64		newTime;

	offSetH = offSetH_P;
	offSetV = offSetV_P;

	gNextBox = grip{};

	maxLen = (newMaxLen > 79)? 79: newMaxLen-1;  // -1 for null ending char.
	SetEditBuffer(gMsgText);

	newTime = SDL_GetTicks();
	boxFlashTimer = newTime + BOX_FLASH_INTERVAL;

   // create two matching cels -one w/text -one w/text and cursor -for blink effect                                         
	gCel  = ACreateCel(&rNumCel, 0, 0, maxWidth, maxHeight, CI_WHITE, 1);
	gCel2 = ACreateCel(&rNumCel2, 0, 0, maxWidth, maxHeight, CI_WHITE, 1);
	SetContext(gNewContext);
	SetOwnerCel(rNewOwnerCel);
	SetRes(RES_CEL, rNumCel);
	SetPos(xPos, yPos);
	Setup(CT_SERV);
	currState = false;
	SetPri(newPri);
	Listen(true); // handles patching in to message list
	msgMask = E_MOUSE_DOWN|E_KEY_DOWN|E_MOUSE_UP;

	rOwner.Set(xPos-offSetH, yPos-offSetV, xPos+maxWidth-offSetH-1, yPos+maxHeight-offSetV-1);

	gBack = ACreateCel(&rNumBack, 0, 0, maxWidth, maxHeight, CI_BLACK, 1);
	pBack = (CelHeader*)AGetResData(gBack);

	// lets get a copy of the section of background we're writing into
	CopyCel(pBack,0,0,RES_CEL,rOwnerCel,1,&rOwner,false);

	SetState(S_CHANGED, true);
	Draw();
}

void
BAM_Box::SetEditBuffer(grip gMsgText)
{
	char	*pMsgText;

	gOrigText = gMsgText;
	pMsgText = ADerefAs(char, gOrigText);
 	memcpy(editBuf, pMsgText, maxLen);
	editBuf[maxLen] = '\0';
	currLen = strlen(editBuf);
	SetState(S_CHANGED, true);
}

void
BAM_Box::Draw(void)
{
	CelHeader	*pSrcCel, *pSrcCel2, *pDestCel;
	char			tmpstr[80];
	int			tmplen;

	prevFont = pFontMgr->curFontNum;
	if(prevFont != font)
		pFontMgr->SetRes(font);

	pSrcCel = (CelHeader*)AGetResData(gCel);
	pSrcCel2 = (CelHeader*)AGetResData(gCel2);
	pDestCel = (CelHeader*)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rOwnerCel));

	if(backCopyColor != -1)
	{
		memset((uchar*)pSrcCel + sizeof(CelHeader),  (uchar)backCopyColor,(pSrcCel->width) * pSrcCel->height);
		memset((uchar*)pSrcCel2 + sizeof(CelHeader), (uchar)backCopyColor,(pSrcCel2->width) * pSrcCel2->height);
	}
	else
	{
		// copy preserved background into cels
		CopyCel(pSrcCel,  0, 0, RES_CEL, rNumBack, 1, false);
		CopyCel(pSrcCel2, 0, 0, RES_CEL, rNumBack, 1, false);
	}

	SetFontColors(backColor,foreColor1,foreColor2,foreColor3,foreColor4,foreColor5,foreColor6);
	ASetFontColor(FNT_FORE_COLOR+6, cursorColor1);
	ASetFontColor(FNT_FORE_COLOR+7, cursorColor2);

//	pMono->Out("editBuf == \"%s\"\n", editBuf);
	pFontMgr->SetString(0, 0, editBuf, (uchar *)pSrcCel, pSrcCel->width, 0);
	strcpy(tmpstr,editBuf);
	tmplen = strlen(tmpstr);
	tmpstr[tmplen++] = 17;
	tmpstr[tmplen] = '\0';
	pFontMgr->SetString(0, 0, tmpstr, (uchar *)pSrcCel2, pSrcCel->width, 0);

// old draw cursor stuff
//	pFontMgr->TextSize(rCursor, editBuf, pSrcCel->width, 0);
//	memset(pCelData + rCursor.High() * pSrcCel->width + rCursor.Wide(), pFontMgr->colors[FNT_FORE_COLOR], 8);

	if(currState == true)
		CopyCel(pDestCel, scrim.x - scrim.orgX - offSetH, scrim.y - scrim.orgY - offSetV, RES_CEL, rNumCel2, 1, false);
	else
		CopyCel(pDestCel, scrim.x - scrim.orgX - offSetH, scrim.y - scrim.orgY - offSetV, RES_CEL, rNumCel, 1, false);

	if(prevFont != font)
		pFontMgr->SetRes(prevFont);

	AUpdateRect(&scrim.rect);
}


void
BAM_Box::Listen(bool fListen)
{
	if(!gContext)
	{
		pMono->Out("BAM_Box::Listen() error - no owning context\n");
		return;
	}
	Context	*pCon = ADerefAs(Context, gContext);

	switch(fListen)
	{
		case true:
			pCon->AddObject(gSelf, CT_MSGS, boxMsgPriority);		// tell our context to not give us msgs.  
			pCon->lReceivers.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);
			break;

		case false:
			pCon->CutObject(gSelf, CT_MSGS);		// tell our context to not give us msgs.  
			break;
	}
}

void
BAM_Box::UpdateString()
{
	char		*pOrigText = ADerefAs(char, gOrigText);

	memcpy(editBuf, pOrigText, maxLen);
	editBuf[maxLen] = '\0';
	currLen = strlen(editBuf);

	Draw();
}

bool
BAM_Box::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&bamBoxDataStart, 1, (uintptr_t)&bamBoxDataEnd -
				(uintptr_t)&bamBoxDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&bamBoxDataStart, 1, (uintptr_t)&bamBoxDataEnd -
				(uintptr_t)&bamBoxDataStart, pFile);
			break;
	}
	return(true);
}

