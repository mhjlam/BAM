// MAKECHAR.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Choose one of two characters in a storyline.
//
// 12/12/94
//


#include "makechar.hpp"
#include "bamfuncs.hpp"
//#include "story.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "context.hpp"
#include	"mono.hpp"
#include	"mouse.hpp"
#include	"graphmgr.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <string.h>

//=========================================================

MakeChar::MakeChar(void)
{

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;

	gback = 0;

	firstCoverAnim=2850;
	firstMenFaceAnim=2900;
	firstWomenFaceAnim=2940;
	firstOtherFaceAnim=2970;
	firstBodyAnim=3000;

	customNameSelected = FALSE;
	maxMenNameNum = 0;
	maxWomenNameNum = 0;
	maxOtherNameNum = 0;

	gender_t tmpcoverGen[MAX_COVER] = {
			G_ALL,G_MEN | G_OTHER,G_ALL,G_ALL,G_MEN,G_WOMEN,
			G_WOMEN,G_ALL,G_WOMEN | G_OTHER,G_MEN | G_WOMEN,G_MEN | G_OTHER,
			G_MEN | G_OTHER,G_MEN | G_OTHER,G_WOMEN | G_OTHER,G_MEN | G_OTHER,
			G_WOMEN | G_OTHER,G_ALL};

	gender_t tmpbodyGen[MAX_BODY] = {
			G_ALL,G_ALL,G_MEN,G_WOMEN | G_OTHER,G_ALL,
			G_ALL,G_ALL,G_ALL,G_WOMEN,G_ALL};

	memcpy(coverGen,tmpcoverGen,sizeof(gender_t)*MAX_COVER);
	memcpy(bodyGen,tmpbodyGen,sizeof(gender_t)*MAX_BODY);

}

MakeChar::~MakeChar(void)
{
	if(gNameText)
		AFree(gNameText);

	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);
}

void
MakeChar::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;
	int			x;
	uint			rNumBord;

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("MakeChar background cel");
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	gbackAnim = ALoad(RES_ANIM,8150);
	CopyCel(pbackCH,0,0,RES_ANIM,8150,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	//pal.FadeToBlack();
	pal.Load(8150);

	// pick a gender catagory
	switch (ARandom(3))
	{
		case 0:
			bGlobal.curCat = G_MEN;
			break;

		case 1:
			bGlobal.curCat = G_WOMEN;
			break;

		case 2:
			bGlobal.curCat = G_OTHER;
			break;

	}

	//======================================================
	// lets copy in all the borders and boundaries into the background

	gPortBord = ALoad(RES_ANIM,8154);
	rNumBord = 8154;
	CopyCel((CelHeader*)pback,108,67,RES_ANIM,rNumBord,3,TRUE);	// Name Frame
	CopyCel((CelHeader*)pback,108,92,RES_ANIM,rNumBord,1,TRUE);	// Portrait Frame
	CopyCel((CelHeader*)pback,157,92,RES_ANIM,rNumBord,4,TRUE);	// Arrow Buttons Frame
	CopyCel((CelHeader*)pback,108,191,RES_ANIM,rNumBord,5,TRUE);	// Gender Buttons Frame
	CopyCel((CelHeader*)pback,108,222,RES_ANIM,rNumBord,6,TRUE);	// Name Buttons Frame
	CopyCel((CelHeader*)pback,108,253,RES_ANIM,rNumBord,7,TRUE);	// 'Done' Button Frame
	CopyCel((CelHeader*)pback,21,281,RES_ANIM,rNumBord,10,TRUE);	// Option Button Frame

	//=============================================
	// Write the necessary text into the background

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);

	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,1);
	ASetString(206,  99, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,2);
	ASetString(206, 116, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,3);
	ASetString(206, 133, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,4);
	ASetString(206, 150, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,5);
	ASetString( 70, 198, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,6);
	ASetString( 82, 229, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,7);
	pFontMgr->SetString(0,330, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);

	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,8);
	pFontMgr->SetString(0,372, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);


	switch(bGlobal.curCat)
	{
		case G_MEN:
			sqbRes = MEN_SQB;
			if(!maxMenNameNum)
				maxMenNameNum = atoi(sqbMakeChar.Load(MEN_SQB,1));
			//add 2 to num to get zero based random num past first sqb used above
			curNameNum = MyRandom(maxMenNameNum,curNameNum) + 2;
			break;
		case G_WOMEN:
			sqbRes = WOMEN_SQB;
			if(!maxWomenNameNum)
				maxWomenNameNum = atoi(sqbMakeChar.Load(WOMEN_SQB,1));
			curNameNum = MyRandom(maxWomenNameNum,curNameNum) + 2;
			break;
		case G_OTHER:
			sqbRes = OTHER_SQB;
			if(!maxOtherNameNum)
				maxOtherNameNum = atoi(sqbMakeChar.Load(OTHER_SQB,1));
			curNameNum = MyRandom(maxOtherNameNum,curNameNum) + 2;
			break;
	}


	pTxt = sqbMakeChar.Load(sqbRes,curNameNum);
	TRACK_MEM("NameText");	gNameText = AMalloc(20);
	char *pText = ADerefAs(char, gNameText);
	strcpy(bGlobal.curName,pTxt);
	strcpy(pText,pTxt);
	BAM_Box	*pBox = &nameBox;
	pBox->SetColors(CI_SKIP,64,74,64,74,64,74,155,142);
	pBox->Create(111, 71, 105, 15, 200, gNameText, 16, gSelf, rNumBack,0,0);
	pBox->SetupReplies(REPLY_DESELECTED);	// let us know when text changes
	//pBox->Select(TRUE); //makes the box edit active

	//======================================================
	// setup current portrait -randomly init.

	LoadCover();
	LoadFace();
	LoadBody();
	LoadBanner();

	//======================================================
	// lets setup up all the buttons

	uint32 posY[4] =  { 95, 112, 129, 146 };

	for(x=0; x<4; x++)
	{
		// create left arrow button
		pButton = &leftArrowB[x];
		pButton->Create(159, posY[x], 200, RES_ANIM, 8156, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;		// click-type button
		pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

		// create right arrow button
		pButton = &rightArrowB[x];
		pButton->Create(180, posY[x], 200, RES_ANIM, 8157, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;		// click-type button
		pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
	}


	// disk button
	pButton = &diskB;
	pButton->Create(23, 285, 200, RES_ANIM, 129, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &randomCharB;
	pButton->Create(159, 163, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,9);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &manB;
	pButton->Create(110, 194, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
	pButton->fIsToggle = TRUE;			// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,10);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	if(bGlobal.curCat == G_MEN)
		pButton->Select(TRUE);				// set button to unselected state - will cause drawing into master cel
	else
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &womanB;
	pButton->Create(152, 194, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
	pButton->fIsToggle = TRUE;			// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,11);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	if(bGlobal.curCat == G_WOMEN)
		pButton->Select(TRUE);				// set button to unselected state - will cause drawing into master cel
	else
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &otherB;
	pButton->Create(194, 194, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
	pButton->fIsToggle = TRUE;			// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,12);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	if(bGlobal.curCat == G_OTHER)
		pButton->Select(TRUE);				// set button to unselected state - will cause drawing into master cel
	else
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &customNameB;
	pButton->Create(110, 225, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,13);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &randomNameB;
	pButton->Create(152, 225, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,9);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &doneB;
	pButton->Create(110, 256, 200, RES_ANIM, 8158, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbMakeChar.Load(MAKECHAR_SQB,14);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


bool
MakeChar::HandleMsg(Message* pMsg)
{
	char		mess[100];
	int		x;

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our TOGGLE buttons.
				if ((uint16)pMsg->notice.param == REPLY_ACTIVATED)
				{
					if(pMsg->notice.gSource == manB.gSelf)
					{
						pMono->Out("\nMan button was hit.");
						womanB.Select(FALSE);
						otherB.Select(FALSE);
						bGlobal.curCat = G_MEN;
						UpdateGender();
						return(TRUE);
					}
					if(pMsg->notice.gSource == womanB.gSelf)
					{
						pMono->Out("\nWoman button was hit.");
						manB.Select(FALSE);
						otherB.Select(FALSE);
						bGlobal.curCat = G_WOMEN;
						UpdateGender();
						return(TRUE);
					}
					if(pMsg->notice.gSource == otherB.gSelf)
					{
						pMono->Out("\nOther button was hit.");
						manB.Select(FALSE);
						womanB.Select(FALSE);
						bGlobal.curCat = G_OTHER;
						UpdateGender();
						return(TRUE);
					}
				}
				else
				// a reply from one of our CLICK buttons.
				if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == nameBox.gSelf)
					{
						pMono->Out("\nGot a nameBox deactivation message.");
						char *pText = ADerefAs(char, gNameText);
						if(strcmp(bGlobal.curName,pText))
						{
							strcpy(bGlobal.curName,pText);
							customNameSelected = TRUE;
						}
							
						return(TRUE);
					}

					for(x=0;x<4;x++)
					{
						if(pMsg->notice.gSource == leftArrowB[x].gSelf)
						{
							pMono->Out("\nLeft Arrow button was hit.");
							switch(x)
							{
								case C_COVER:
									AdvCover(C_DECREMENT);
									break;
								case C_FACE:
									AdvFace(C_DECREMENT);
									break;
								case C_BODY:
									AdvBody(C_DECREMENT);
									break;
								case C_BANNER:
									AdvBanner(C_DECREMENT);
									break;
							}
							return TRUE;
						}

						if(pMsg->notice.gSource == rightArrowB[x].gSelf)
						{
							pMono->Out("\nRight Arrow button was hit.");
							switch(x)
							{
								case C_COVER:
									AdvCover(C_INCREMENT);
									break;
								case C_FACE:
									AdvFace(C_INCREMENT);
									break;
								case C_BODY:
									AdvBody(C_INCREMENT);
									break;
								case C_BANNER:
									AdvBanner(C_INCREMENT);
									break;
							}
							return TRUE;
						}
					}
					if(pMsg->notice.gSource == randomCharB.gSelf)
					{
						pMono->Out("\nRandom Char. button was hit.");
						UpdateRandom();
						return(TRUE);
					}
					if(pMsg->notice.gSource == customNameB.gSelf)
					{
						pMono->Out("\nCustom name button was hit");
						nameBox.Select(TRUE);
						return(TRUE);
					}
					if(pMsg->notice.gSource == randomNameB.gSelf)
					{
						pMono->Out("\nRandom name button was hit");
						RandomName();
						customNameSelected = FALSE;
						return(TRUE);
					}
					if(pMsg->notice.gSource == doneB.gSelf)
					{
						pMono->Out("\nDone button was hit.");
						bGlobal.curCharId = GetCharId();

						pBam->playerSide = SIDE1;
						pBam->sideColors[SIDE1] = (unitColor_t) ((bGlobal.curBanner/2) + 1);
						//legend opponent room will now set enemy color
						//pBam->sideColors[SIDE2] = (unitColor_t) (MyRandom(MAX_BANNER,bGlobal.curBanner/2) + 1);

						bGlobal.roomMgr.NewRoom(BR_INTRO_HALL);
						return(TRUE);
					}
					if(pMsg->notice.gSource == diskB.gSelf)
					{
						pMono->Out("\nDisk button was hit.");
						Option(pal.gSelf);
						return(TRUE);
					}
				}
				else
				if ((uint16)pMsg->notice.param == REPLY_DEACTIVATED)
				{
					if(pMsg->notice.gSource == manB.gSelf)
					{
						pMono->Out("\nGot a MAN deactivation message.");
						//reactivate
						manB.Select(TRUE);
						return(TRUE);
					}
					else
					if(pMsg->notice.gSource == womanB.gSelf)
					{
						pMono->Out("\nGot a WOMAN deactivation message.");
						//reactivate
						womanB.Select(TRUE);
						return(TRUE);
					}
					else
					if(pMsg->notice.gSource == otherB.gSelf)
					{
						pMono->Out("\nGot a OTHER deactivation message.");
						//reactivate
						otherB.Select(TRUE);
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
			switch(pMsg->event.type)
			{
				case E_KEY_DOWN:
					//switch (pMsg->event.value)
					//{
					//	case K_ESC:
					//	case K_Q:
					//		pBam->Quit();
					//		return TRUE;
					//		break;
					//}
					break;
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	if(BAM_Room::HandleMsg(pMsg))
		return TRUE;
	else
	{
		//OK, buttons have now had a shot at it so:
		switch (pMsg->type)
		{
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
				}
				break;
		}
	}
	return FALSE;
}


void
MakeChar::LoadCover()
{
	BAM_Guy	*pGuy;

	curCover = ARandom(MAX_COVER);

	bGlobal.curCoverAnim = firstCoverAnim + (2*curCover);

	pGuy = &curCoverGuy;
	if(coverGen[curCover] & bGlobal.curCat)
		pGuy->SetRes(RES_ANIM,bGlobal.curCoverAnim,1);
	else
		AdvCover(C_INCREMENT);
	pGuy->SetPos(110,95);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(230);

}


void
MakeChar::LoadFace()
{
	BAM_Guy	*pGuy;

	switch(bGlobal.curCat)
	{
		case G_MEN:
			curFace = ARandom(MAX_FACE_MEN);
			bGlobal.curFaceAnim = firstMenFaceAnim + (2*curFace);
			break;
		case G_WOMEN:
			curFace = ARandom(MAX_FACE_WOMEN);
			bGlobal.curFaceAnim = firstWomenFaceAnim + (2*curFace);
			break;
		case G_OTHER:
			curFace = ARandom(MAX_FACE_OTHER);
			bGlobal.curFaceAnim = firstOtherFaceAnim + (2*curFace);
			break;
		default:
			APanic("Bad catagory in MakeChar room");
			break;
	}

	pGuy = &curFaceGuy;
	pGuy->SetRes(RES_ANIM,bGlobal.curFaceAnim,1);
	pGuy->SetPos(110,95);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(220);
}


void
MakeChar::LoadBody()
{
	BAM_Guy	*pGuy;

	curBody = ARandom(MAX_BODY);

	bGlobal.curBodyAnim = firstBodyAnim + (2*curBody);

	pGuy = &curBodyGuy;
	if(bodyGen[curBody] & bGlobal.curCat)
	{
		pGuy->SetRes(RES_ANIM,bGlobal.curBodyAnim,1);
	}
	else
	{
		AdvBody(C_INCREMENT);
	}
	pGuy->SetPos(110,95);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(210);
}


void
MakeChar::LoadBanner()
{
	BAM_Guy		*pGuy;
	int			bannerTmp;

	//curBanner is number which reflects the color of the banner and its type.
	bannerTmp = ARandom(MAX_BANNER);
	if(pBam->playerSide == SIDE2)
		bGlobal.curBanner =	(bannerTmp*2)+1;
	else
		bGlobal.curBanner =	bannerTmp*2;


	pGuy = &curBannerGuy;
	if(pBam->playerSide == SIDE2)
	{
		pGuy->SetRes(RES_ANIM,9004,3);	// banner 2 "dots"
	}
	else
	{
		pGuy->SetRes(RES_ANIM,9004,2);	// banner 1 "slash"
	}
	pGuy->SetPos(110,171);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(210);

	// first two banners 'belong' to palette '1'
	// second set of banners 'belong' to palette '2', etc.
	ShiftPal(bannerTmp);

}


void
MakeChar::AdvCover(dir_t dir)
{
	bool	nextCoverInGen=FALSE;

	// keep advancing until next anim in Gender is found
	do
	{
		//range from 0 to (max. size - 1)
		if(dir == C_INCREMENT)
		{
			curCover++;
			if(curCover >= MAX_COVER)
				curCover = 0;
		}
		else // == C_DECREMENT
		{
			curCover--;
			if(curCover < 0)
				curCover = MAX_COVER - 1;
		}

		if(coverGen[curCover] & bGlobal.curCat)
		{
			bGlobal.curCoverAnim = firstCoverAnim + (2*curCover);
			curCoverGuy.SetRes(RES_ANIM,bGlobal.curCoverAnim,1);
			nextCoverInGen = TRUE;
		}

	} while(!nextCoverInGen);
}


void
MakeChar::AdvFace(dir_t dir)
{
	int	max;
	int	firstAnim;
	
	switch(bGlobal.curCat)
	{
		case G_MEN:
			max = MAX_FACE_MEN;
			firstAnim = firstMenFaceAnim;
			break;
		case G_WOMEN:
			max = MAX_FACE_WOMEN;
			firstAnim = firstWomenFaceAnim;
			break;
		case G_OTHER:
			max = MAX_FACE_OTHER;
			firstAnim = firstOtherFaceAnim;
			break;
		default:
			APanic("Bad catagory in MakeChar room");
			break;
	}

	//range from 0 to (max. size - 1)
	if(dir == C_INCREMENT)
	{
		curFace++;
		if(curFace >= max)
			curFace = 0;
	}
	else // == C_DECREMENT
	{
		curFace--;
		if(curFace < 0)
			curFace = (max - 1);
	}

	bGlobal.curFaceAnim = firstAnim + (2*curFace);
	curFaceGuy.SetRes(RES_ANIM,bGlobal.curFaceAnim,1);
}


void
MakeChar::AdvBody(dir_t dir)
{
	bool	nextBodyInGen=FALSE;

	// keep advancing until next anim in Gender is found
	do
	{
		//range from 0 to (max. size - 1)
		if(dir == C_INCREMENT)
		{
			curBody++;
			if(curBody >= MAX_BODY)
				curBody = 0;
		}
		else // == C_DECREMENT
		{
			curBody--;
			if(curBody < 0)
				curBody = MAX_BODY - 1;
		}

		if(bodyGen[curBody] & bGlobal.curCat)
		{
			bGlobal.curBodyAnim = firstBodyAnim + (2*curBody);
			curBodyGuy.SetRes(RES_ANIM,bGlobal.curBodyAnim,1);
			nextBodyInGen = TRUE;
		}

	} while(!nextBodyInGen);
}


void
MakeChar::AdvBanner(dir_t dir)
{
	//range from 0 to (max. size - 1)
	if(dir == C_INCREMENT)
	{
		bGlobal.curBanner += 2;
		if(bGlobal.curBanner >= MAX_BANNER*2)
			bGlobal.curBanner -= (MAX_BANNER*2);

		ShiftPal(bGlobal.curBanner / 2);
	}
	else // == C_DECREMENT
	{
		bGlobal.curBanner -= 2;
		if(bGlobal.curBanner < 0)
			bGlobal.curBanner += (MAX_BANNER*2);

		ShiftPal(bGlobal.curBanner / 2);
	}
}


void
MakeChar::UpdateGender()
{
	// Leave banner as is

	// check cover for need to adv. to proper gender
	if(!(coverGen[curCover] & bGlobal.curCat))
		AdvCover(C_INCREMENT);

	// check body for need to adv. to proper gender
	if(!(bodyGen[curBody] & bGlobal.curCat))
		AdvBody(C_INCREMENT);

	AdvFace(C_INCREMENT);
	if(!customNameSelected)
		RandomName();
}


void
MakeChar::UpdateRandom()
{
	//cover ====================================
	curCover = MyRandom(MAX_COVER,curCover);
	bGlobal.curCoverAnim = firstCoverAnim + (2*curCover);

	if(coverGen[curCover] & bGlobal.curCat)
	{
		curCoverGuy.SetRes(RES_ANIM,bGlobal.curCoverAnim,1);
	}
	else
	{
		AdvCover(C_INCREMENT);
	}

	//face =====================================
	switch(bGlobal.curCat)
	{
		case G_MEN:
			curFace = MyRandom(MAX_FACE_MEN,curFace);
			bGlobal.curFaceAnim = firstMenFaceAnim + (2*curFace);
			break;
		case G_WOMEN:
			curFace = MyRandom(MAX_FACE_WOMEN,curFace);
			bGlobal.curFaceAnim = firstWomenFaceAnim + (2*curFace);
			break;
		case G_OTHER:
			curFace = MyRandom(MAX_FACE_OTHER,curFace);
			bGlobal.curFaceAnim = firstOtherFaceAnim + (2*curFace);
			break;
		default:
			APanic("Bad catagory in MakeChar room");
			break;
	}

	curFaceGuy.SetRes(RES_ANIM,bGlobal.curFaceAnim,1);

	//body ======================================
	curBody = MyRandom(MAX_BODY,curBody);
	bGlobal.curBodyAnim = firstBodyAnim + (2*curBody);

	if(bodyGen[curBody] & bGlobal.curCat)
		curBodyGuy.SetRes(RES_ANIM,bGlobal.curBodyAnim,1);
	else
		AdvBody(C_INCREMENT);

	//banner ====================================
	bGlobal.curBanner = 2 * MyRandom(MAX_BANNER,bGlobal.curBanner/2);

	// first two banners 'belong' to palette '1'
	// second set of banners 'belong' to palette '2', etc.
	ShiftPal(bGlobal.curBanner / 2);
}



void
MakeChar::RandomName()
{
	switch(bGlobal.curCat)
	{
		case G_MEN:
			sqbRes = MEN_SQB;
			if(!maxMenNameNum)
				maxMenNameNum = atoi(sqbMakeChar.Load(MEN_SQB,1));
			//add 2 to num to get zero based random num past first sqb used above
			curNameNum = MyRandom(maxMenNameNum,curNameNum) + 2;
			break;
		case G_WOMEN:
			sqbRes = WOMEN_SQB;
			if(!maxWomenNameNum)
				maxWomenNameNum = atoi(sqbMakeChar.Load(WOMEN_SQB,1));
			curNameNum = MyRandom(maxWomenNameNum,curNameNum) + 2;
			break;
		case G_OTHER:
			sqbRes = OTHER_SQB;
			if(!maxOtherNameNum)
				maxOtherNameNum = atoi(sqbMakeChar.Load(OTHER_SQB,1));
			curNameNum = MyRandom(maxOtherNameNum,curNameNum) + 2;
			break;
	}


	pTxt = sqbMakeChar.Load(sqbRes,curNameNum);
	char *pText = ADerefAs(char, gNameText);
	strcpy(bGlobal.curName,pTxt);
	strcpy(pText,pTxt); //copy into name box

	nameBox.UpdateString();
}

int
MakeChar::MyRandom(int max, int oldnum)
{
	int num;

	do
	{
		num = ARandom(max);
	} while(num == oldnum);
	return num;
}


void
MakeChar::ShiftPal(int bannerColor)
{
	//load extended palette and
	//remap bitmap to the appropriate palette range or
	// copy palette range into range used by banner bitmap
	// the second is prob. faster

	pal.LoadPartial(9101,192+(bannerColor*8),8,192);
	pal.UpdateColors(192,200);

	//update banner guy
	curBannerGuy.SetState(S_CHANGED, TRUE);

}

