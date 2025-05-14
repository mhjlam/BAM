// OPTION2.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A room in which to test new code.
//
// 11/16/94
//

#include "Option2.hpp"

#include "apifont.hpp"
#include "apires.hpp"
#include "apigraph.hpp"
#include "context.hpp"
#include "Scrimage.hpp"
#include "api.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "rect.hpp"
#include "sconfig.hpp"
#include "tigre.hpp"

#include <string.h>

#define	OPTION2_BASE_PRI	20000

#define	OPT2_WOFF_X	89
#define	OPT2_WOFF_Y	92

//=========================================================

NoiseMenu::NoiseMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
}

NoiseMenu::~NoiseMenu()
{
	// is assigned to a Guy -it will take care of it
	//if (gback)
	//	ADelete(gback);
}

void
NoiseMenu::Setup(option_t sndOrMus_P)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;

	// room has already been paused

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start NoiseMenu::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		mode = M_MODELESS;
	else
		mode = M_MODAL;

	sndOrMus = sndOrMus_P;

	TRACK_MEM("Option2: background anim");
	gbackAnim = ALoad(RES_ANIM,40);
	pbackAnimCH = (CelHeader*)AGetResData(gbackAnim);
	rback.Set(OPT2_WOFF_X, OPT2_WOFF_Y, OPT2_WOFF_X-1+pbackAnimCH->width, OPT2_WOFF_Y-1+pbackAnimCH->height);

	// setup background cel filled with black
	TRACK_MEM("Option2: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,OPTION2_BASE_PRI + 10);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,40,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(OPT2_WOFF_X,OPT2_WOFF_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(OPTION2_BASE_PRI + 10);

	pGuy = &slideBack;
	pGuy->SetRes(RES_ANIM,40,4);
	pGuy->SetPos(112,140);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(OPTION2_BASE_PRI + 11);

	//---------------------------------------
	// Two Text boxes for OFF & MAX
	uchar	  saveColorFore,saveColorFore1,saveColorBack;

	saveColorFore  = AFontColor(FNT_FORE_COLOR);
	saveColorFore1 = AFontColor(FNT_FORE_COLOR + 1);
	saveColorBack  = AFontColor(FNT_BACK_COLOR);

	pFontMgr->colors[FNT_FORE_COLOR] = 93;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = 90;
	pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;

	pOffTxt = sqbOption.Load(OPTION_SQB,29); //"OFF"
	ASetString(24,79, pOffTxt, (uchar *)pbackCH, pbackCH->width-24, NULL);
	pMaxTxt = sqbOption.Load(OPTION_SQB,30); //"MAX"
	ASetString(98,79, pMaxTxt, (uchar *)pbackCH, pbackCH->width-98, NULL);

	rOff.Set(110,170,135,180); //just loosely enclose the word
	rMax.Set(185,170,210,180);

	//---------------------------------------
	// Two Text lines for 'SET MUSIC VOLUME' or 'SET SOUNDS VOLUME'
	// setup a cel with SKIP for a guy that will display text of region names

	if(sndOrMus == SOUND_BUTTON)
	{
		pTxt = sqbOption.Load(OPTION_SQB,31);
		ASetString(20,98, pTxt, (uchar *)pbackCH, 140-20-20, NULL, DG_JUST_CENTER);
	}
	else
	{
		pTxt = sqbOption.Load(OPTION_SQB,32);
		ASetString(20,98, pTxt, (uchar *)pbackCH, 140-20-20, NULL, DG_JUST_CENTER);
	}

	//Reset color in which to draw names
	pFontMgr->colors[FNT_FORE_COLOR] = saveColorFore;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = saveColorFore1;
	pFontMgr->colors[FNT_BACK_COLOR] = saveColorBack;

	//---------------------------------------
	// We'll setup the slider after the writing of the text above
	// to allow the slider to possible overwrite some words in hilite.
	if (sndOrMus == SOUND_BUTTON)
	{
		if (pSoundMgr)
		{
			maxLevel = MAX_VOLUME;
			saveLevel = pSoundMgr->GetMasterDigiVolume();
		}
	}
	else
	{
		if (pSoundMgr)
		{
			maxLevel = MAX_VOLUME;
			saveLevel = pSoundMgr->GetMasterMidiVolume();
		}
	}

	//slider.Setup(grip gContext, uint theAni, uint theCel, coord theX, coord theY,
	//				   int thePri, int limitA, int limitB, int activeWidth, int theDir, int16 level_P,int16 maxLevel_P);
	slider.Setup(gSelf,RES_ANIM,40,5,120,155,OPTION2_BASE_PRI + 12,120,198,12,HORIZONTAL,saveLevel,maxLevel);

	//---------------------------------------
	//Cap setups

	CopyCel(pbackCH,112-OPT2_WOFF_X,219-OPT2_WOFF_Y,RES_ANIM,40,2,FALSE);
	CopyCel(pbackCH,112-OPT2_WOFF_X,256-OPT2_WOFF_Y,RES_ANIM,40,3,FALSE);


	//---------------------------------------
	//Button Setup for DONE button
	pButton = &button[0];
	pButton->Create(112, 222, OPTION2_BASE_PRI + 11, RES_ANIM, 42, 1, gSelf, OPT2_WOFF_X, OPT2_WOFF_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbOption.Load(OPTION_SQB,28);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(TRUE);	//turn on click response -already in create()


	//Button Setup for CANCEL button
	pButton = &button[1];
	pButton->Create(112, 239, OPTION2_BASE_PRI + 11, RES_ANIM, 44, 1, gSelf, OPT2_WOFF_X, OPT2_WOFF_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbOption.Load(OPTION_SQB,58);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();
	//pButton->Listen(TRUE);	//turn on click response -already in create()


	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;			// grip of currently active button, if any

}


bool
NoiseMenu::HandleMsg(Message* pMsg)
{
	char		mess[100];

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
						if(pMsg->notice.gSource == button[0].gSelf)
						{
							pMono->Out("\nDone Button was hit");
							if (pSoundMgr)
							{
							 if(sndOrMus == SOUND_BUTTON)
							 {
								SetSound(DIGI,slider.level); //always call
							 }
							 else
							 {
								SetSound(MIDI,slider.level);
							 }
							}

			
							Cleanup();
							return TRUE;
						}
						else if(pMsg->notice.gSource == button[1].gSelf)
						{
							pMono->Out("\nCancel Button was hit");
							if (pSoundMgr)
							{
							 if(sndOrMus == SOUND_BUTTON)
							 {
								pSoundMgr->SetMasterDigiVolume(saveLevel);
							 }
							 else
							 {
								pSoundMgr->SetMasterMidiVolume(saveLevel);
							 }
							}

							Cleanup();
							return TRUE;
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
							case K_A:
								FakeMouseClick(button[0].gSelf);
								return TRUE;
								break;

							case K_ESC:
							case K_C:
								FakeMouseClick(button[1].gSelf);
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
NoiseMenu::Cleanup()
{
	Activate(FALSE);

	delete this;
}


void
NoiseMenu::SetSound(int16 soundType, int16 level)
{
	SConfig	sConfig;
	char		configToken[8];

	if (pSoundMgr)
	{
		if (soundType == MIDI)
		{
			pSoundMgr->SetMasterMidiVolume(level);
			sConfig.LoadConfig("sound.cfg");
			sprintf(configToken, "%d", level);
			sConfig.SetEntry(MIDI_VOLUME, configToken);
			sConfig.SaveConfig();
		}
		else
		{
			pSoundMgr->SetMasterDigiVolume(level);
			sConfig.LoadConfig("sound.cfg");
			sprintf(configToken, "%d", level);
			sConfig.SetEntry(DIGI_VOLUME, configToken);
			sConfig.SaveConfig();
		}
	}
}


//===============================================================
void
NoiseSlider::Setup(grip gContext, res_t theType, uint theNum, uint theCel, coord theX, coord theY,
				  int thePri, int limitA, int limitB, int activeWidth, int theDir,
				  int16 level_P, int16 maxLevel_P)
{
	int	tempWidth;
	int	curPosn;
	int	percentage;

	fOff = FALSE;
	fMax = FALSE;

	maxLevel = maxLevel_P;
	level = level_P;

	msgMask =  E_MOUSE_DOWN | E_MOUSE_UP;
	fIsMouseDown = FALSE;

	SetContext(gContext);
	SetRes(theType, theNum, theCel);
	SetPri(thePri);
	BAM_Guy::Setup(CT_ROST | CT_MSGS); //No service yet
	orientation = theDir;

	if(orientation == HORIZONTAL)
	{
		slideRect.Set(limitA,theY,limitB,theY);
		containRect.Set(limitA-5,theY-activeWidth,limitB+5,theY+activeWidth);
	}
	else
	{
		slideRect.Set(theX,limitA,theX,limitB);
		containRect.Set(theX-activeWidth,limitA-5,theX+activeWidth,limitB+5);
	}

	topOrLeft = limitA;
	rightOrBottom = limitB;

	EndCheck();

	if (maxLevel != 0)
	{
		tempWidth = rightOrBottom - topOrLeft;
		percentage = level * 100 / maxLevel;
		curPosn = percentage * tempWidth / 100;
	}
	else
	{
		curPosn = 0;
	}
	if (orientation == HORIZONTAL)
		BAM_Guy::SetPos(curPosn + topOrLeft, theY);
	else
		BAM_Guy::SetPos(theX, rightOrBottom - curPosn); // where bott=0 and top=max
}


void
NoiseSlider::Cycle()
{
	NoiseMenu	*pNoise;

	Slider::Cycle();

	pNoise = ADerefAs(NoiseMenu,gContext);

	if (pSoundMgr)
	{
		if (pNoise->sndOrMus == MUSIC_BUTTON)
		{
			pSoundMgr->SetMasterMidiVolume(level);
		}
		else
		{
			pSoundMgr->SetMasterDigiVolume(level);
		}
	}

	EndCheck();
}


void
NoiseSlider::EndCheck()
{
	uchar	  saveColorFore,saveColorFore1,saveColorBack;
	NoiseMenu	*pNoise;

	pNoise = ADerefAs(NoiseMenu,gContext);
	CelHeader *pCel = (CelHeader *)AGetResData(pNoise->gback);

	if(level == 0) //then hilite text 'OFF'
	{
		if(!fOff)	  // if already done then ignore
		{
			fOff = TRUE;

			saveColorFore  = AFontColor(FNT_FORE_COLOR);
			saveColorFore1 = AFontColor(FNT_FORE_COLOR + 1);
			saveColorBack  = AFontColor(FNT_BACK_COLOR);

			pFontMgr->colors[FNT_FORE_COLOR] = 155;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = 142;
			pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;

			ASetString(24,79, pNoise->pOffTxt, (uchar *)pCel, pCel->width, NULL);

			//Reset color in which to draw names
			pFontMgr->colors[FNT_FORE_COLOR] = saveColorFore;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = saveColorFore1;
			pFontMgr->colors[FNT_BACK_COLOR] = saveColorBack;
			AUpdateRect(&(pNoise->rOff));
		}
	}
	else 
	{
		if(fOff) // if hilite text on then lets turn it off
		{
			fOff = FALSE;

			saveColorFore  = AFontColor(FNT_FORE_COLOR);
			saveColorFore1 = AFontColor(FNT_FORE_COLOR + 1);
			saveColorBack  = AFontColor(FNT_BACK_COLOR);

			pFontMgr->colors[FNT_FORE_COLOR] = 93;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = 90;
			pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;

			ASetString(24,79, pNoise->pOffTxt, (uchar *)pCel, pCel->width, NULL);

			//Reset color in which to draw names
			pFontMgr->colors[FNT_FORE_COLOR] = saveColorFore;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = saveColorFore1;
			pFontMgr->colors[FNT_BACK_COLOR] = saveColorBack;
			AUpdateRect(&(pNoise->rOff));
		}
	}

	if(level == maxLevel)
	{
		if(!fMax)
		{
			fMax = TRUE;

			saveColorFore  = AFontColor(FNT_FORE_COLOR);
			saveColorFore1 = AFontColor(FNT_FORE_COLOR + 1);
			saveColorBack  = AFontColor(FNT_BACK_COLOR);

			pFontMgr->colors[FNT_FORE_COLOR] = 155;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = 142;
			pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;

			ASetString(98,79, pNoise->pMaxTxt, (uchar *)pCel, pCel->width, NULL);

			//Reset color in which to draw names
			pFontMgr->colors[FNT_FORE_COLOR] = saveColorFore;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = saveColorFore1;
			pFontMgr->colors[FNT_BACK_COLOR] = saveColorBack;
			AUpdateRect(&(pNoise->rMax));
		}
	}
	else
	{
		if(fMax)
		{
			fMax = FALSE;

			saveColorFore  = AFontColor(FNT_FORE_COLOR);
			saveColorFore1 = AFontColor(FNT_FORE_COLOR + 1);
			saveColorBack  = AFontColor(FNT_BACK_COLOR);

			pFontMgr->colors[FNT_FORE_COLOR] = 93;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = 90;
			pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;

			ASetString(98,79, pNoise->pMaxTxt, (uchar *)pCel, pCel->width, NULL);

			//Reset color in which to draw names
			pFontMgr->colors[FNT_FORE_COLOR] = saveColorFore;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = saveColorFore1;
			pFontMgr->colors[FNT_BACK_COLOR] = saveColorBack;
			AUpdateRect(&(pNoise->rMax));
		}
	}
}
