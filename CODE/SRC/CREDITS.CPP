// CREDITS.CPP
//
//	Copyright 1994,1995, Tachyon, Inc.
//
//
// Display game credits.
//
// 7/21/95
//

#include "credits.hpp"

#include "api.hpp"
#include "apievt.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "context.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <string.h>
#include <ctype.h>

#define	SCREEN_WIDTH	320
#define	SCREEN_HEIGHT	400

//listed in sqb number order
#define	HEADLINES_SQB				8010
#define	CORE_TEAM_SQB				8020
#define	BACK_ILLUST_SQB			8030
#define	TIGRE_MAC_SUPPORT_SQB	8040
#define	SUPPORT_SQB					8050
#define	MEDIATECH_SQB				8060
#define	VOICE_SQB					8070
#define	VOICE2_SQB					8080
#define	INTERPLAY_SQB				8090
#define	INTERPLAY_SOUND_SQB		8100
#define	QA_SQB						8110
#define	TSR_SQB						8120
#define	DEDICATION_SQB				8120
#define	INTERPLAY2_SQB				8130
#define	QA2_SQB						8140

//headline sqb nums
#define	H_CORE_TEAM		2
#define	H_BACK_ILLUST	3
#define	H_TIGRE_PROG	4
#define	H_MAC_PROG		5
#define	H_MAC_ARTIST	6
#define	H_STAFF			7
#define	H_MEDIATECH		8
#define	H_VOICE			9
#define	H_INTERPLAY		10
#define	H_IPLAY_SOUND	11
#define	H_QA				12
#define	H_TSR				13

#define	LAST_PAGE	23

//each page's name in proper order
enum cr_page_t
{
	CR_NONE,
	CR_CORE_VAS,
	CR_CORE_JOHN,
	CR_CORE_VAN,
	CR_CORE_KEV,
	CR_CORE_TRAY,
	CR_CORE_BRIAN,
	CR_CORE_JANA,
	CR_CORE_LEIGH,
	CR_BACK_ILLUST,
	CR_TIGRE,
	CR_MAC_PROG,
	CR_MAC_ART,
	CR_SUPPORT,
	CR_MEDIATECH,
	CR_VOICE,
	CR_VOICE2,
	CR_INTERPLAY,
	CR_INTERPLAY2,
	CR_INTERPLAY_SOUND,
	CR_QA,
	CR_QA2,
	CR_TSR,
	CR_DEDICATION
};

char	pBrodie[] = "BRODIE";

//=========================================================

Credits::Credits()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	page = 1;
	fBrodie = FALSE;
}

Credits::~Credits()
{
	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);
}

void
Credits::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	char			*pTxt;

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("Credits: background cel");
	gback = ACreateCel(&resNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	rScreen.Set(0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1);

	// copy this anim into the dynamic cel that we will use
	//WILL CALL SetupPage instead
	//CopyCel(pbackCH,0,0,RES_ANIM,9020,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, resNumBack, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	pal.Load(9020);

	//======================================================
	// lets setup up all the buttons

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	//Button Setup for PREV button
	pButton = &leftButton;
	pButton->Create(0, 0, 200, RES_ANIM, 9024, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(resNumBack);
	pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_TOP);
	pButton->SetTextOffSet(21,5);
	pTxt = sqbCredits.Load(HEADLINES_SQB,100);	//'PREV' text
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(DG_DISABLED);

	//Button Setup for NEXT button
	pButton = &rightButton;
	pButton->Create(270, 0, 200, RES_ANIM, 9026, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(resNumBack);
	pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_TOP);
	pButton->SetTextOffSet(2,5);
	pTxt = sqbCredits.Load(HEADLINES_SQB,101);	//'NEXT' text
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);

	pButton = &doneButton;
	pButton->Create(135,370, 200, RES_ANIM, 8306, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(resNumBack);
	pTxt = sqbCredits.Load(HEADLINES_SQB,1); //'Done'
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetTextOffSet(0,0);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);

	SetupPage();

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


void
Credits::SetupPage()
{
	int	i,j,textX,textX2,textY;
	char	*pTxt;
	

	//new background
	CopyCel(pbackCH,0,0,RES_ANIM,9020,1,FALSE);

	switch((cr_page_t)page)
	{
		case CR_CORE_VAS:
		case CR_CORE_JOHN:
		case CR_CORE_VAN:
		case CR_CORE_KEV:
		case CR_CORE_TRAY:
		case CR_CORE_BRIAN:
		case CR_CORE_JANA:
		case CR_CORE_LEIGH:
			//Portrait Border
			CopyCel(pbackCH,139,78,RES_ANIM,9022,1,FALSE);	// Portrait Border

			//Portrait
			if(fBrodie)
				CopyCel(pbackCH,141,81,RES_ANIM,9030+page-1,2,FALSE);	// Portrait
			else
				CopyCel(pbackCH,141,81,RES_ANIM,9030+page-1,1,FALSE);	// Portrait

			//=================
			// Write the necessary text into the background

			// Write section headline
			pFontMgr->SetRes(9056);
			SetFontColors(CI_SKIP,CI_BLACK,149,150,151,109,110);
			pTxt = sqbCredits.Load(HEADLINES_SQB,H_CORE_TEAM);
			pFontMgr->SetString(0,18, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);

			textY = 167;
			// write person name
			pFontMgr->SetRes(9062);
			SetFontColors(CI_SKIP,81,78,62,72,CI_BLACK,CI_BLACK);
			pTxt = sqbCredits.Load(CORE_TEAM_SQB,page*10);
			pFontMgr->SetString(0,textY, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);

			textY = 200;
			i = 1;
			SetFontColors(CI_SKIP,168,165,163,CI_BLACK,CI_BLACK,CI_BLACK);

			if(fBrodie)
			{
				pTxt = sqbCredits.Load(CORE_TEAM_SQB,(page*10)+9,TRUE);
				if(!pTxt)
					break;
				pFontMgr->SetString(0,textY, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);

			}
			else
			{
				while(TRUE)
				{
					// write credit text
					pTxt = sqbCredits.Load(CORE_TEAM_SQB,(page*10)+i,TRUE);
					if(!pTxt)
						break;
					pFontMgr->SetString(0,textY, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);
					textY += 25;
					i++;
				}
			}
			break;

		case CR_BACK_ILLUST:
			WritePage(H_BACK_ILLUST,BACK_ILLUST_SQB,35,173);
			break;

		case CR_TIGRE:
		case CR_MAC_PROG:
		case CR_MAC_ART:
			// Write section headline
			pFontMgr->SetRes(9056);
			SetFontColors(CI_SKIP,CI_BLACK,149,150,151,109,110);
			pTxt = sqbCredits.Load(HEADLINES_SQB,page-CR_TIGRE+H_TIGRE_PROG);
			pFontMgr->SetString(0,18, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);

			textX  = 51;
			textX2 = 193;
			textY  = 51;
			i = 0;
			pFontMgr->SetRes(9062);
			SetFontColors(CI_SKIP,81,78,62,72,CI_BLACK,CI_BLACK);
			while(TRUE)
			{

				if(!pTxt)
					break;

				//one or two column list of names
				if(page == CR_MAC_PROG)
				{
					// write credit text
					pTxt = sqbCredits.Load(TIGRE_MAC_SUPPORT_SQB,(page-CR_TIGRE+1)*10 + i,TRUE);
					if(!pTxt)
						break;
					pFontMgr->SetString(0,textY, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);
					i++;
				}
				else
				{
					// write credit text
					pTxt = sqbCredits.Load(TIGRE_MAC_SUPPORT_SQB,(page-CR_TIGRE+1)*10 + i,TRUE);
					if(!pTxt)
						break;
					pFontMgr->SetString(textX,textY, pTxt, pback, pbackCH->width, NULL);

					pTxt = sqbCredits.Load(TIGRE_MAC_SUPPORT_SQB,(page-CR_TIGRE+1)*10 + i + 1,TRUE);
					if(!pTxt)
						break;
					pFontMgr->SetString(textX2,textY, pTxt, pback, pbackCH->width, NULL);

					i += 2;
				}

				textY += 25;
			}
			break;

		case CR_SUPPORT:
			WritePage(H_STAFF,SUPPORT_SQB);
			break;
		case CR_MEDIATECH:
			WritePage(H_MEDIATECH,MEDIATECH_SQB);
			break;
		case CR_VOICE:
			WritePage(H_VOICE,VOICE_SQB,32);
			break;
		case CR_VOICE2:
			WritePage(H_VOICE,VOICE2_SQB,32);
			break;
		case CR_INTERPLAY:
			WritePage(H_INTERPLAY,INTERPLAY_SQB,32);
			break;
		case CR_INTERPLAY2:
			WritePage(H_INTERPLAY,INTERPLAY2_SQB,32);
			break;
		case CR_INTERPLAY_SOUND:
			WritePage(H_IPLAY_SOUND,INTERPLAY_SOUND_SQB,32);
			break;
		case CR_QA:
			textX  = 33;
			textX2 = 163;
			textY = WritePage(H_QA,QA_SQB,25);
			textY += 12; //padding

			i = 0;
			pFontMgr->SetRes(9062);
			SetFontColors(CI_SKIP,81,78,62,72,CI_BLACK,CI_BLACK);
			while(TRUE)
			{
				// write credit text
				pTxt = sqbCredits.Load(QA_SQB,100 + i,TRUE);
				if(!pTxt)
					break;
				pFontMgr->SetString(textX,textY, pTxt, pback, pbackCH->width, NULL);

				pTxt = sqbCredits.Load(QA_SQB,100 + i + 1,TRUE);
				if(!pTxt)
					break;
				pFontMgr->SetString(textX2,textY, pTxt, pback, pbackCH->width, NULL);

				i += 2;

				textY += 25;
			}
			break;

		case CR_QA2:
			// Write section headline
			pFontMgr->SetRes(9056);
			SetFontColors(CI_SKIP,CI_BLACK,149,150,151,109,110);
			pTxt = sqbCredits.Load(HEADLINES_SQB,H_QA);
			pFontMgr->SetString(0,18, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);

			textX  = 51;
			textX2 = 193;
			textY  = 51;
			i = 0;
			pFontMgr->SetRes(9062);
			SetFontColors(CI_SKIP,81,78,62,72,CI_BLACK,CI_BLACK);
			while(TRUE)
			{
				// write credit text
				pTxt = sqbCredits.Load(QA2_SQB,10 + i,TRUE);
				if(!pTxt)
					break;
				pFontMgr->SetString(textX,textY, pTxt, pback, pbackCH->width, NULL);

				pTxt = sqbCredits.Load(QA2_SQB,10 + i + 1,TRUE);
				if(!pTxt)
					break;
				pFontMgr->SetString(textX2,textY, pTxt, pback, pbackCH->width, NULL);

				i += 2;

				textY += 25;
			}
			break;
		case CR_TSR:
			WritePage(H_TSR,TSR_SQB);
			break;
		case CR_DEDICATION:
			pFontMgr->SetRes(9062);
			SetFontColors(CI_SKIP,81,78,62,72,CI_BLACK,CI_BLACK);

			j = 0;
			textY = 155;
			while(TRUE)
			{
				// write person name
				pTxt = sqbCredits.Load(DEDICATION_SQB,100+j,TRUE);
				if(!pTxt)
					break;
				pFontMgr->SetString(0,textY, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);
				textY += 25;
				j++;
			}
			break;

	}

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	back.SetState(S_CHANGED,TRUE);

	//redraw buttons
	leftButton.SetState(S_CHANGED,TRUE);
	rightButton.SetState(S_CHANGED,TRUE);
	doneButton.SetState(S_CHANGED,TRUE);

	leftButton.Draw();
	rightButton.Draw();
	doneButton.Draw();
}


int
Credits::WritePage(int headlineNum, int pageSqb, int pad, int textX)
{
	int	i,j,textY;
	char	*pTxt;

	// Write section headline
	pFontMgr->SetRes(9056);
	SetFontColors(CI_SKIP,CI_BLACK,149,150,151,109,110);
	pTxt = sqbCredits.Load(HEADLINES_SQB,headlineNum);
	pFontMgr->SetString(0,18, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);


	j = 1;
	textY = 51;
	while(TRUE)
	{
		// write person name
		pFontMgr->SetRes(9062);
		SetFontColors(CI_SKIP,81,78,62,72,CI_BLACK,CI_BLACK);
		pTxt = sqbCredits.Load(pageSqb,j*10,TRUE);
		if(!pTxt)
			break;
		pFontMgr->SetString(33,textY, pTxt, pback, pbackCH->width, NULL);

		i = 1;
		textY -= 25; //KLUDGE US INTO LOOP
		SetFontColors(CI_SKIP,168,165,163,CI_BLACK,CI_BLACK,CI_BLACK);
		while(TRUE)
		{
			// write credit text
			pTxt = sqbCredits.Load(pageSqb,(j*10)+i,TRUE);
			if(!pTxt)
				break;
			textY += 25;
			pFontMgr->SetString(textX,textY, pTxt, pback, pbackCH->width, NULL);
			i++;
		}
		textY += pad;
		j++;
	}

	return(textY);
}


void
Credits::ConfigButtons()
{
	if(page == 1)
	{
		leftButton.Select(DG_DISABLED);
	}
	else
	{
		leftButton.Listen(TRUE);
	}

	if(page == LAST_PAGE)
	{
		rightButton.Select(DG_DISABLED);
	}
	else
	{
		rightButton.Listen(TRUE);
	}
}


bool
Credits::HandleMsg(Message* pMsg)
{
	char		mess[100];

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == doneButton.gSelf)
					{
						pMono->Out("\nDone Button was hit");
						bGlobal.roomMgr.NewRoom(BR_MENU);
						return(TRUE);
					}
					else if(pMsg->notice.gSource == leftButton.gSelf)
					{
						pMono->Out("\nLeft Button was hit");
						if(page != 1)
							page--;
						ConfigButtons();
						SetupPage();
						return(TRUE);
					}
					else if(pMsg->notice.gSource == rightButton.gSelf)
					{
						pMono->Out("\nRight Button was hit");
						if(page != LAST_PAGE)
							page++;
						ConfigButtons();
						SetupPage();
						return(TRUE);
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
				case E_MOUSE_UP:
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

						case K_X:
							if(pMsg->event.modifiers & MOD_ALT)
							{
								Cleanup();
								pContextMgr->Quit();
							}
							return TRUE;

						case K_LEFT:
							//pMono->Out("\nLeft Button was hit");
							FakeMouseClick(leftButton.gSelf);
							return(TRUE);

						case K_RIGHT:
							//pMono->Out("\nRight Button was hit");
							FakeMouseClick(rightButton.gSelf);
							return(TRUE);

						case K_ESC:
							FakeMouseClick(doneButton.gSelf);
							return(TRUE);

						case K_D: //done
							if(!(pMsg->event.modifiers & MOD_ALT))
							{
								FakeMouseClick(doneButton.gSelf);
								return(TRUE);
							}
							//possible fall-thru

						default:
							char	*pStr, char1;
							int codeLen, compOff;

							if(pMsg->event.modifiers & MOD_ALT)
							{
								memmove(brodieBuf, brodieBuf + 1, BRODIE_MAX_LEN - 2);
								brodieBuf[BRODIE_MAX_LEN - 1] = NULL;	// terminator
								char1 = (char)AScanToASCII(pMsg->event);
								char1 = (char)toupper(char1);
								brodieBuf[BRODIE_MAX_LEN - 2] = char1;

								codeLen = strlen(pBrodie);
								compOff = (BRODIE_MAX_LEN - 1) - codeLen;
								pStr = brodieBuf + compOff;
								if(!strcmp(pStr, pBrodie))
								{
									fBrodie = TRUE;
									ConfigButtons();
									SetupPage();
								}
							}
							return TRUE;
					}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


