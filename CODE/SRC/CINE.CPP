//	CINE.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Play all cinematics except the intro.
//
// 1/30/95


#include "cine.hpp"
#include "bam.hpp"
#include "bamfuncs.hpp"
#include "snap.hpp"

#include "mouse.hpp"
#include "apievt.hpp"
#include "apires.hpp"

extern Snap *pSnap;
extern void	MouseHandler( int draw_mouse );

#define	CINENUM	51
#define	CINE_OLD_INTRO	35

Cine::Cine(void)
{
	msgMask = E_KEY_DOWN | E_KEY_UP | E_MOUSE_DOWN | E_MOUSE_UP;
	nextCine = 0;
	cineExists = FALSE;
	requiredFlic = TRUE;
	alreadyCreatedPopup = FALSE;
}


void	
Cine::Setup(void)
{
	BAM_ErrorPopup		*pPop;
	int	x;
	grip	gback;
	BAM_Guy	*pGuy;

	pMouse->Hide();

	TRACK_MEM("Cine: background cel");
	gback = ACreateCel(&resNumBack,0,0,320,400,CI_BLACK,100);

	//we need a tmp background to go behind any error popups
	pGuy = &back;
	pGuy->SetRes(RES_CEL, resNumBack, 1);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	uint16	cineArr[CINENUM][4] = { //cinematic,nextRoom,preloadSize,nextCine
									//INTRO
									  15,BR_CINE  ,2500,20,
									  20,BR_CINE  ,1000,30,
									  30,BR_MENU  ,2500,0,

									//HOWL OF VENGEANCE
									3502,BR_CHOOSE  ,2000,0,	//STORY OPENING
									//3510,BR_CHOOSE,1000,0,		//STORY OPENING 2
  									3520,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3525,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3530,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3540,BR_DUMMY  ,1000,0,		//WINNING SCENE
  									3545,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3550,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3555,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3564,BR_DUMMY  ,1000,0,		//WINNING SCENE

									//MATCHMAKER MAYHEM
  									3575,BR_CHOOSE  ,2000,0,	//STORY OPENING
  									//3580,BR_CHOOSE,1000,0,		//STORY OPENING 2
  									3590,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3594,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3598,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3605,BR_DUMMY  ,1000,0,		//WINNING SCENE
  									3610,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3614,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3618,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3625,BR_DUMMY  ,1000,0,		//WINNING SCENE

									//TARTARUS UNBOUND -NEED TO SET MUSIC ON DOWN!!!!!
  									3635,BR_CHOOSE  ,2000,0,	//STORY OPENING
  									//3640,BR_CHOOSE,1000,0,		//STORY OPENING 2
  									3645,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3650,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3655,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3662,BR_DUMMY  ,1000,0,		//WINNING SCENE
  									3666,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3670,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3674,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3680,BR_DUMMY  ,1000,0,		//WINNING SCENE

				 					//NUTS AND BOLTS
  									3687,BR_CHOOSE  ,2000,0,	//STORY OPENING
  									//3692,BR_CHOOSE,1000,0,		//STORY OPENING 2
  									3696,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3700,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3705,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3712,BR_DUMMY  ,1000,0,		//WINNING SCENE
  									3716,BR_STORY ,1000,0,		//SETUP SCENE 1
  									3720,BR_STORY ,1000,0,		//SETUP SCENE 2
  									3725,BR_STORY ,1000,0,		//SETUP SCENE 3
  									3735,BR_DUMMY  ,1000,0,		//WINNING SCENE

									//HARVEST OF HORRORS
									3745,BR_CHOOSE  ,2000,0,	//STORY OPENING
									//3750,BR_CHOOSE,1000,0,		//STORY OPENING 2
									3755,BR_STORY ,1000,0,		//SETUP SCENE 1
									3760,BR_STORY ,1000,0,		//SETUP SCENE 2
									3765,BR_STORY ,1000,0,		//SETUP SCENE 3
									3775,BR_DUMMY  ,1000,0,		//WINNING SCENE
									3780,BR_STORY ,1000,0,		//SETUP SCENE 1
									3785,BR_STORY ,1000,0,		//SETUP SCENE 2
									3790,BR_STORY ,1000,0,		//SETUP SCENE 3
									3800,BR_DUMMY  ,1000,0,		//WINNING SCENE

									//LEGEND INTRO 
									3805,BR_CHAR    ,2000,0,
									//3810,BR_CHAR  ,1000,0,

									//BONUS MAP INTRO
									3815,BR_WORLD ,1000,0

									};

	

	for (x=0;x<CINENUM;x++)
	{
		if(bGlobal.cinematic == cineArr[x][0])
		{
			//if we teleported in from mainmenu then go back there
			if(bGlobal.roomMgr.newRoomMode == 1)
			{
				bGlobal.roomMgr.newRoomMode = 0; //reset
		 		nextRoom = BR_MENU;
				preloadSize = cineArr[x][2];
				nextCine = 0;
			}
			else
			{
		 		nextRoom = cineArr[x][1];
				preloadSize = cineArr[x][2];
				nextCine = cineArr[x][3];

				//if finishing a storyline
				if(nextRoom == BR_DUMMY)
				{
					if(GetStoryDone() & 0x1F) 
					{
						//if all stories are done then go to back to main menu
						//to see random campaign button
						bGlobal.roomMgr.newRoomMode = 1; //single menu
						nextRoom = BR_MENU;
					}
					else
					{
						nextRoom = BR_STORY;
					}
				}
			}
			break;
		}
	}

	if(x == CINENUM)
	{
		//APanic("Unrecognized cinematic number!");
		TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
		pPop->Setup(NULL,POPUP_SQB,101,BR_MENU);
		alreadyCreatedPopup = TRUE;
	}

	//all cine's are required now
	//if(x < 3)
	//	requiredFlic = FALSE;

	if(x == 2)	//intro cine
	{
		if (pEventMgr->GetModifiers() & MOD_ALT)
		{
			if(AExists(RES_SMACKER,CINE_OLD_INTRO))
			{
				//HAPPY EASTER!
				bGlobal.cinematic = CINE_OLD_INTRO; //old intro cine
			}
		}
	}


}


//lets grab all input that happens during play of a smacker file
bool	
Cine::HandleMsg(Message *pMsg)
{
	switch (pMsg->type)
	{
		case MSG_NOTICE:
			break;

		case MSG_EVENT:
				switch (pMsg->event.type)
				{
					case E_MOUSE_UP:
						flic.fFlicDone = TRUE;
						return TRUE;

					case E_KEY_DOWN:
						if(pMsg->event.value == K_F4)
						{
							if(!bGlobal.gSnap)
							{
								//lets setup for screen snapping (capture)
								TRACK_MEM("Snap");	pSnap = new Snap;
								bGlobal.gSnap = pSnap->gSelf;
							}

							//no single shots while snap is toggled on
							if(!pSnap->snapOn)
								pSnap->SingleScreen();
						}
						else
						{
							if(pMsg->event.value == K_ESC &&
							(bGlobal.cinematic == 15 || bGlobal.cinematic == 20))
							{
								nextRoom = BR_MENU;
								nextCine = 0;
							}

							flic.fFlicDone = TRUE;
						}

						return TRUE;
				}
			break;
	}

	return TRUE; //no pass-thru
}


void	
Cine::Cycle(void)
{
	CineErrorPopup	*pCinePop;
	BAM_ErrorPopup	*pPop;
	int	flicErr=0;

	MouseHandler(FALSE);

	if(alreadyCreatedPopup)
		return;

	if(!cineExists)
	{
		if(!AExists(RES_SMACKER,bGlobal.cinematic))
		{
			if(requiredFlic)
			{
				//MUST BE VERY CAREFUL OF CREATING A POPUP WHILE IN A CYCLE
				//ROUTINE. BECAUSE YOU CHANGE THE CONTEXT LIST THE CONTEXT
				//MANAGER IS TRYING TO CYCLE THRU AND YOU CAN GET YOURSELF
				//INTO AN INFINITE LOOP!

				//we can't open the cine
				//lets run an error popup saying to put CD in drive
				//It will either set retry TRUE or quit the game

				pMono->Out("Didn't find required cinematic %d\n",bGlobal.cinematic);

				TRACK_MEM("CineErrorPopup"); pCinePop = new CineErrorPopup;
				pCinePop->Setup();
				alreadyCreatedPopup = TRUE;
			}
			else
			{
				//lets skip it and go on
				pMono->Out("Didn't find cinematic %d\n",bGlobal.cinematic);

				if(nextCine)
					bGlobal.cinematic = nextCine;
				else
					bGlobal.cinematic = 0;

				TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
				pPop->Setup(NULL,POPUP_SQB,102,nextRoom);
				alreadyCreatedPopup = TRUE;
			}

		}
		else
		{
			cineExists = TRUE;
		}
	}
	else
	{
		flicErr = flic.Play(bGlobal.cinematic,preloadSize);

		if(nextCine)
			bGlobal.cinematic = nextCine;
		else
			bGlobal.cinematic = 0;

		if(flicErr)
		{
			bPal.UpdateAll();
			TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
			pPop->Setup(NULL,POPUP_SQB,103,nextRoom);
			alreadyCreatedPopup = TRUE;
		}
		else
		{
			bGlobal.roomMgr.NewRoom(nextRoom);
		}
	}
}

//=======================================================================
//=======================================================================

CineErrorPopup::CineErrorPopup() : BAM_DefaultPopup(2)
{
}


void
CineErrorPopup::Setup()
{
	SetTextSqb(OPTION_SQB,52,DG_JUST_CENTER); //"Cinematic not found. Please insert CD."
	SetButtonSqb(0,OPTION_SQB,51); //'RETRY' text
	SetButtonSqb(1,OPTION_SQB,36); //'QUIT GAME' text
	SetButtonStartOffsets(23,137);
	BAM_DefaultPopup::Setup(0,NULL);

	BAM_Popup::ActivatePopup();
}


bool
CineErrorPopup::HandleNotice(Message *pMsg)
{
	if (pMsg->notice.type == N_CONTROL_REPLY)
	{
		if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
		{
			if(pMsg->notice.gSource == pBUTTON[0].gSelf)
			{
				//pMono->Out("retry button was hit\n");
				//by default we will try again next cycle
				(ADerefAs(Cine,bGlobal.roomMgr.gCurRoom))->alreadyCreatedPopup = FALSE;
				Cleanup();				
				return(TRUE);
			}
			else
			if(pMsg->notice.gSource == pBUTTON[1].gSelf)
			{
				//pMono->Out("quit button was hit\n");
				pContextMgr->Quit();
				Cleanup();				
				return(TRUE);
			}
		}
	}

	//eat all input regardless
	return(TRUE);
}


bool
CineErrorPopup::HandleEvent(Message *pMsg)
{
	switch (pMsg->event.type)
	{
		//case E_MOUSE_UP:
		//	// the following is done to make sure that the active button
		//	// (if any) receives the MOUSE_UP event even if mouse has
		//	// wandered out of the rect
		//	if(gCurControl)
		//	{
		//		Object	*pObject;
		//		pObject = ADerefAs(Object, gCurControl);
		//		if(pObject->HandleMsg(pMsg))
		//			return(TRUE);
		//	}
		//	break;

		case E_KEY_DOWN:
			switch (pMsg->event.value)
			{
				case K_ESC:
				case K_Q:
					pContextMgr->Quit();
					Cleanup();				
					break;
			}
	}

	//eat all input regardless
	return(TRUE);
}



