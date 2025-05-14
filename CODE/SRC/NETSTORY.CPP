//	netstory.cpp
//
//	Copyright 1995, Tachyon, Inc.
//
// net/modem region map selection screen
//
//		8-28-95: Kevin Sherrill
//


#include "netstory.hpp"

#include "alldefs.hpp"
#include "api.hpp"
#include "apifont.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamroom.hpp"
#include "bampopup.hpp"
#include "bam_dg.hpp"
#include "bamfuncs.hpp"
#include "comm.hpp"
#include "context.hpp"
#include "eventmgr.hpp"
#include "graphmgr.hpp"
#include "mainmenu.hpp"
#include "mouse.hpp"
#include	"palette.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "Scrimage.hpp"
#include "tigre.hpp"

#include <string.h>


#define	STORY_INSET_ANI	216

NetStory::NetStory()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gTCel = 0;

	hasSelected = FALSE;
	enemyHasSelected = FALSE;

	currentGuy = -1;
	enemyCurrentGuy = -1;

	flashToggle=0;
	enemyFlashToggle=0;
	enemyCurX = 0;
	enemyCurY = 0;

	TRACK_MEM("Story: clut");	clut[0] = AMalloc(CLUT_SIZE);
	TRACK_MEM("Story: clut");	clut[1] = AMalloc(CLUT_SIZE);

	worldEnder = FALSE;
	syncingNewRoom = TRUE;

	curRand = 42;
	syncedRand = FALSE;

	currFrame = 0;
	errorSqbNum = 45; //'connection has failed.'

	packetType = NS_EMPTY;
}

NetStory::~NetStory()
{
	if(gTCel)
		AFlush(gTCel);

	AFree(clut[0]);
	AFree(clut[1]);
}


void
NetStory::Setup()
{
	
	BAM_Guy 		*pGuy;
	uint32		x,x1;
	clock_t		newTime;
	BAM_Button	*pButton;


	bGlobal.missionsDone = 0;
	sendErrCnt = 0;


	// lets load a res_anim and copy it to a dcel
	// so we can copy buttons onto it -and attach the dcel to a guy.

	// setup background guy filled with black
	TRACK_MEM("NetStory: background cel");
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,5);

	uchar			*pback;
	CelHeader	*pbackCH;
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	CopyCel(pbackCH,0,0,RES_ANIM,190,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(5);

	//pal.FadeToBlack();
	pal.Load(190);

	LoadUnitColors(pal.gSelf,
						pBam->sideColors[pBam->playerSide],
						pBam->sideColors[(pBam->playerSide==SIDE1)?SIDE2:SIDE1]);

	//----------------------------
	uchar *pClut1 = ADerefAs(uchar, clut[0]);
	uchar *pClut2 = ADerefAs(uchar, clut[1]);

	for(x=0; x<CLUT_SIZE; x++)
	{
		pClut1[x] = (uchar)(CLUT1_START + x);
		pClut2[x] = (uchar)(CLUT2_START + x);
	}

	//----------------------------
	// setup temp. cel for pixel copies
	TRACK_MEM("Story: temp work cel");
	gTCel = ACreateCel(NULL,0,0,2,2,CI_BLACK,5);

	//----------------------------
	// setup a cel for a guy that will display text of region names
	TRACK_MEM("Story: text cel");
	gtitle = ACreateCel(&rNumTitle,0,0,70,72,SKIP_COLOR,250);

	pGuy = &title;
	pGuy->SetRes(RES_CEL,rNumTitle);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(250);

	//----------------------------
	// setup a cel for a guy that will display text of enemy region name
	TRACK_MEM("Story: text cel");
	gEnemyTitle = ACreateCel(&rNumEnemyTitle,0,0,70,72,SKIP_COLOR,250);

	pGuy = &enemyTitle;
	pGuy->SetRes(RES_CEL,rNumEnemyTitle);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(240);

	//----------------------------
	pGuy = &inset;
	pGuy->SetRes(RES_ANIM,221);
	pGuy->SetPos(104,326);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(20);


	for (x=0;x<NETMAXOUTLINEGUYS;x++)
	{
		if(x<=NETMAXREGIONS)
			strcpy(storyNames[x], sqbStory.Load(REGION2_SQB,x+1));
		else // 
			strcpy(storyNames[x], sqbStory.Load(STORYLINE_SQB,x-14));
	}



	//==================================================================
	//note: vas has cels 2 and 3 swapped in anim 204
	uint32  storyAnims [] =	  {200,1,  200,2,  200,3,
					 					201,1,  201,2,  201,3,
					 					202,1,  202,2,  202,3,
					 					203,1,  203,2,  203,3,
					 					204,1,  204,3,  204,2};

	uint32  storyPos [] = {	42,	54,		159,	0,			139,	73,
								 	212,	262,		75,	196,		71,	236,
								 	0,		65,		0,		0,			64,	0,
								 	48,	153,		0,		185,		0,		247,
								 	149,	135,		243,	29,		190,	162};


	for(x=0,x1=0; x<NETMAXSTORYGUYS; x++,x1+=2)
	{
		pGuy = &storyGuy[x];
		pGuy->SetContext(gSelf);
		//pGuy->Setup(CT_ROST);
		pGuy->fIsActive = TRUE; //lets not start them in the roster YET
		pGuy->SetPri(100);
		pGuy->SetRes(RES_ANIM,storyAnims[x1],storyAnims[x1+1]);
		pGuy->SetPos(storyPos[x1],storyPos[x1+1]);
	}

	//==================================================================
	// these positions used for title text box
	int  tmpTitlePos[] = {	//REGIONS
								77,	91,		212,	21,		173,	104,
								232,	289,		85,	230,		138,	281,
								-10,	105,		0,		14,		106,	14,
								81,	181,		4,		214,		11,	281,
								166,	154,		245,	104,		217,	229,

									//MAIN STORIES
								142,	76,		122,	264,  	2,		38,
								28,	205,		230,	181
									};

	memcpy(titlePos,tmpTitlePos,sizeof(titlePos));

	//==================================================================

	pFontMgr->SetRes(9052);
	//SetFontColors(CI_SKIP,45,46,47,49,50,0);		//"ENGRAVED MARBLE"
	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);		//"BLUE GREEN"
	pTxt = sqbStory.Load(STORY_SQB,2); //"choose starting point:"
	ASetString(38,325, pTxt, (uchar *)pbackCH, 62, NULL, DG_JUST_CENTER);

	//Reset color in which to draw names
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	//-----------------------------------------------------------
	int  tmpFlagPos[] = {	//REGION FLAGS
									108,128, 231,55,  216,126,
									250,295, 115,260, 163,295,
									19,155,  37,55,  111,45,
									122,200,  41,240,  31,295,
									201,167, 290,137, 275,238, 

									//MAIN STORY FLAGS
									177,112, 163,300,  37,74,
									63,241, 265,212
									};

	memcpy(flagPos,tmpFlagPos,sizeof(flagPos));

	for(x=0,x1=0; x<NETMAXFLAGGUYS; x++,x1+=2)
	{
		pGuy = &flagGuy[x];
		pGuy->SetContext(gSelf);
		pGuy->SetPri(300);
 		pGuy->SetRes(RES_ANIM,205,3);
	 	pGuy->SetPos(flagPos[x1],flagPos[x1+1]);
		pGuy->SetClut(192,CLUT_SIZE,clut[0]);	//set clut to 1st range
		//no call to Setup() yet
	}

	for(x=0,x1=0; x<NETMAXFLAGGUYS; x++,x1+=2)
	{
		pGuy = &enemyFlagGuy[x];
		pGuy->SetContext(gSelf);
		pGuy->SetPri(300);
 		pGuy->SetRes(RES_ANIM,205,6);
	 	pGuy->SetPos(flagPos[x1],flagPos[x1+1]);
		pGuy->SetClut(192,CLUT_SIZE,clut[1]);	//set clut to 2nd range
		//no call to Setup() yet
	}

	pGuy = &enemyCursor;
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(10000);
	pGuy->SetRes(RES_ANIM,9000,9);
	pGuy->SetPos(pMouse->GetX(),pMouse->GetY());

	//show button as unavailable -change to ani with good "inactive" cel 3
	pButton = &diskButton;
	pButton->Create(4, 365, 200, RES_ANIM, 129, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(DG_DISABLED);

	//==================================================================
	//setup default time and have cycle check
	newTime = clock();
	flagFlashTimer = newTime + FLASH_INTERVAL;
	enemyFlagFlashTimer = flagFlashTimer;


	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

//	pResMgr->Dump();
//	pMemMgr->Dump(1, "Start Option::Setup");
}




// Return the pixel of a scrimage at a given global coordinate.
uchar
NetStory::GetPixel(coord celXOff, coord celYOff, res_t srcType, 
				uint srcNum, uint srcCel)
{
	uchar			*pTCelData;
	Rectangle	celRect;
	CelHeader	*pTCelH; //temp. cel

	celRect.Set(celXOff,celYOff,celXOff+1,celYOff+1);

	pTCelH = (CelHeader*)AGetResData(gTCel);

	CopyCel(pTCelH, 0, 0, srcType, srcNum, srcCel, &celRect, FALSE);

	pTCelData = (uchar*)pTCelH + sizeof(CelHeader);
	
	//return first pixel of data
	return 		*pTCelData;
}


// take array index and return storyline region number from 1 to 5.
uint32
NetStory::GetStory(uint32 aindex)
{
	return (aindex / 3) + 1;
}


bool
NetStory::HandleMsg(Message* pMsg)
{
	uint32	x;
	CelHeader	*ptitle;

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			break;

		case MSG_EVENT:
			switch(pMsg->event.type)
			{
				case E_MOUSE_UP:

				//if a right mouse click, then deselect a selected region
				if(pMsg->event.value == RIGHT_BTN)
				{
					if(hasSelected)
					{
						hasSelected = FALSE;
						packetType = NS_UNDONE;

						flagGuy[currentGuy].Hide();

						ptitle = (CelHeader *)AGetResData(gtitle);
						// clear region name
						memset(((uchar*)ptitle)+sizeof(CelHeader),SKIP_COLOR,L2R(ptitle->width) * ptitle->height);
					}

					return TRUE;
				}

				//lets see if user selected/re-selected a region
				coord		celOffX,celOffY;
				BAM_Guy	*pGuy;
				uchar		color;

				// now lets check the storyline regions
				for(x=0; x<NETMAXSTORYGUYS; x++)
				{

					pGuy = &storyGuy[x];

					if (pGuy->scrim.rect.Contains(pMsg->event.x,pMsg->event.y))
					{
						celOffX = pMsg->event.x - pGuy->scrim.rect.x1;
						celOffY = pMsg->event.y - pGuy->scrim.rect.y1;
						color = GetPixel(celOffX, celOffY, pGuy->scrim.resType, pGuy->scrim.resNum, pGuy->scrim.cel);
						if (color != CI_SKIP)
						{
							//lets pick starting point
							if (pMono)
								pMono->Out("\nNet Game starting point %d selected.",x);

							//if hasSelected is already set then we are doing a reselect
							if(hasSelected)
								ResetCurrentGuy(NS_LOCAL,x);
							else
								currentGuy = x;

							packetType = NS_DONE;
							hasSelected = TRUE;
							flagFlashTimer =  clock() + FLASH_INTERVAL;

							return TRUE;
						}//endif
					}//endif
				}//endfor
				break;

				case E_KEY_DOWN:
					switch (pMsg->event.value)
					{
						case K_X:
							if(!(pMsg->event.modifiers & MOD_ALT))
								return TRUE;

							//if alt-x then fall-thru

						case K_ESC:
							if(!bGlobal.netDisconnect)
							{
								//send disconnect packet
								sPacket1.header.ID = BMSG_DISCONNECT;
								sPacket1.header.destID = (pCommMgr->GetUserID ()) ? 0 : 1;	// other player
								sPacket1.header.len = sizeof(NetStoryPacket);
								sPacket1.pData = &nsPacket;
								pCommMgr->SendData(&sPacket1);
							}

							pCommMgr->Disconnect();
							bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;
							bGlobal.roomMgr.NewRoom(BR_MENU);
							return TRUE;
					}
					break;
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


void	
NetStory::Cycle(void)
{
	coord			celOffX,celOffY;
	BAM_Guy		*pGuy,*pOutGuy;
	BAM_ErrorPopup		*pPop;
	uchar			color;
	bool			found=FALSE;
	uint32		calcGuy,x;
	CelHeader	*ptitle;
	clock_t		newTime;

	clock_t		charTime=0;

	theX = pMouse->GetX();
	theY = pMouse->GetY();
	newTime = clock();

	if(!hasSelected)
	{
		// now lets check the storyline regions
		for(x=0; x<NETMAXSTORYGUYS; x++)
		{

			pGuy = &storyGuy[x];

			if (pGuy->scrim.rect.Contains(theX,theY))
			{
				celOffX = theX - pGuy->scrim.rect.x1;
				celOffY = theY - pGuy->scrim.rect.y1;
				color = GetPixel(celOffX, celOffY, pGuy->scrim.resType, 
								pGuy->scrim.resNum, pGuy->scrim.cel);
				if (color != CI_SKIP) //we found it
				{
					found = TRUE;

					calcGuy = x;

					if (calcGuy != currentGuy) //if new guy
					{
						if (pMono)
							pMono->Out("\nRegion is %2d, StoryLine is %d",x,GetStory(x));

						// cut old storyGuy only if there is one and the enemy
						// isn't using it
						if (currentGuy != -1 && currentGuy != enemyCurrentGuy)
						{
							pOutGuy = &storyGuy[currentGuy];
							pOutGuy->fIsActive = FALSE;
							CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
						}

						currentGuy = calcGuy;

						// add new region outline unless enemy has the region alread active
						if(currentGuy != enemyCurrentGuy)
						{
							pOutGuy = &storyGuy[calcGuy];
							pOutGuy->fIsActive = TRUE;
							AddObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
						}

						ptitle = (CelHeader *)AGetResData(gtitle);

						// clear region name
						memset(((uchar*)ptitle)+sizeof(CelHeader),SKIP_COLOR,L2R(ptitle->width) * ptitle->height);

						pFontMgr->SetRes(9050);
						SetFontColors(CI_SKIP,93,90,93,90,93,90);

						//startup new region name
						ASetString(0,0,storyNames[calcGuy], (uchar *)ptitle, ptitle->width, NULL, DG_JUST_CENTER);
						title.SetPos(titlePos[currentGuy*2],titlePos[currentGuy*2+1]);
						break;
					}
				}//endif
			}//endif
		}//endfor

		//cursor outside of all guys and not used by enemy -shutdown current guy
		if(!found && currentGuy != -1 && currentGuy != enemyCurrentGuy)
		{
			pOutGuy = &storyGuy[currentGuy];
			pOutGuy->fIsActive = FALSE;
			CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);

			currentGuy = -1;

			//shutdown last region name
			ptitle = (CelHeader *)AGetResData(gtitle);
			memset(((uchar*)ptitle)+sizeof(CelHeader),SKIP_COLOR,L2R(ptitle->width) * ptitle->height);
			title.SetState(S_CHANGED, TRUE);
		}
	}
	else //no more selecting just show next region in path or wrap up.
	{
		if(newTime > flagFlashTimer)
		{
			if (flashToggle)
			{
				flashToggle=0;
				flagGuy[currentGuy].Hide();
			}
			else
			{
		 		flashToggle++;
				flagGuy[currentGuy].Show();
			}
			flagFlashTimer = newTime + FLASH_INTERVAL;
		}
	}

	//===========================================================
	// cycle anthing else
	BAM_Room::Cycle();

	//===========================================================
	if(!SyncWithRemote())
	{
		if(!bGlobal.netDisconnect)
		{
			pMono->Out("NetStory::Cycle() - !SyncWithRemote()\n");

			bGlobal.storyLine = STORY_NONE;
			pCommMgr->Disconnect();
			bGlobal.netDisconnect = TRUE;
			bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;

			TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
			pPop->SetMode(M_MODAL);
			pPop->Setup(pal.gSelf,OPTION_SQB,errorSqbNum,BR_MENU);//"connection has failed"
		}
		return;
	}

	//===========================================================
	//set enemy marker


	CycleEnemy();

	if(enemyHasSelected)
	{
		if(newTime > enemyFlagFlashTimer)
		{
			if (enemyFlashToggle)
			{
				enemyFlashToggle=0;
				enemyFlagGuy[enemyCurrentGuy].Hide();
			}
			else
			{
		 		enemyFlashToggle++;
				enemyFlagGuy[enemyCurrentGuy].Show();
			}
			enemyFlagFlashTimer = newTime + FLASH_INTERVAL;
		}
	}


	//===========================================================
	if(hasSelected && enemyHasSelected && (currentGuy == enemyCurrentGuy))
	{
		//if we've synced a new rand lets use it
		if(syncedRand)
			ASeedRandom(newRand);
		else
			ASeedRandom(curRand);

		// wait a second for lingering packets to be ACKed (so no one has to
		// timeout) - Van
		ticks_t tWait = ATicks() + TICKS_PER_SEC / 2;
		while(ATicks() < tWait)
		{
			// we wont send anything, but make it possible to ACK leftover
			// packets sent to us
			UpdateTicks();
			pCommMgr->DataAvailable();
		}

		//lets sync tick counters
		ASetTicks(1001);

		int scenarioArr[15] = {
									9112,9132,9152,
									9172,9192,9212,
									9232,9252,9272,
									9292,9312,9332,
									9352,9372,9392,
									};

		int tmpNum = scenarioArr[currentGuy];

		sprintf(pBam->scenarioName,"%d",tmpNum);

		bGlobal.roomMgr.NewRoom(BR_WORLD);
		worldEnder = TRUE;
	}
}


bool
NetStory::SyncWithRemote()
{
	int			error, packetCount;
	ticks_t		currTicks, endTicks;
	struct NetStoryPacket	*pPacket;
	Rectangle	rect;
	CelHeader	*pEnemyTitle;
	BAM_ErrorPopup		*pPop;


	if(worldEnder)
	{
		return(TRUE);
	}

	pMono->Out("\nStory::Sync() - send frame%d. ", currFrame);
	pComm->ClearError();		// why?  Cuz Russell said to.

	//if we are synced then lets check for incoming packets
	if(!syncingNewRoom)
	{
		pComm->ClearError();

		// wait for incoming packet
		endTicks = ATicks() + TICKS_PER_SEC * 6;
		do
		{
			UpdateTicks();
			currTicks = ATicks();
			if(currTicks > endTicks)
			{
				//notify of broken connection and return to mainmenu
				pMono->Out("NetStory: time out while waiting for packet\n");
				errorSqbNum = 48;
				return(FALSE);	//this will do it
			}

			pMono->Out("\r  Waiting for frame %d, %d ticks", currFrame, endTicks - currTicks);
			if (pCommMgr->GetError ())
			{
				pMono->Out ("\nCommError %d(%s)\n", pCommMgr->GetError(), pErrorStrings[pCommMgr->GetError()]);
				//APanic("NetStory: pCommMgr returned error");

				//notify of broken connection and return to mainmenu
				errorSqbNum = 49;
				return(FALSE);	//this will do it
			}
		}	while(!pCommMgr->DataAvailable ());
		pMono->Out("\n");

		// receive no more than 3 packets
		packetCount = 3;
		while(pCommMgr->DataAvailable() && packetCount--)
		{
			sPacket1.pData = &nsPacket;
			int error = pCommMgr->ReceiveData(&sPacket1);

			if(error)
			{
				pMono->Out("  Error, no packet received.  Error == %d(%s)\n", error, pErrorStrings[error]);
				//APanic("NetStory: no packet received.");
				errorSqbNum = 46;
				return FALSE;
			}
			else
			{
				if(pCommMgr->DataAvailable())
				{
					pMono->Out("(WARNING: next incoming packet already waiting)\n");
				}

				pPacket = (NetStoryPacket*)sPacket1.pData;
				if(pPacket->frameNum != currFrame + 1)
				{
					pMono->Out("  Frame count error!  Local=%d remote=%d, ", currFrame, pPacket->frameNum);

//					no need to crash out, just go on.  -Van				
//					APanic("NetStory: Frame count error!");

					// are we the one who's behind?
					if(currFrame < pPacket->frameNum - 1)
					{
						// then catch up
						pMono->Out("adjusting local\n");
						currFrame = pPacket->frameNum - 1;
					}
					else
					{
						// other side will catch up w/ us
						pMono->Out("expecting remote to adjust\n");
					}
				}

				// receive & process packet anyway - Van
				pMono->Out("  Packet received\n");
				enemyCurX = pPacket->coordX;
				enemyCurY = pPacket->coordY;
				enemyCursor.SetPos(enemyCurX,enemyCurY);

				receivedRand = pPacket->newRand;
				if(pCommMgr->GetUserID())
				{
					if(receivedRand == newRand)
						syncedRand = TRUE;
				}
				else
				{
					//player 0 is synced when he's received two packets w/ same #
					if(receivedRand == newRand)
						syncedRand = TRUE;
					else
						newRand = receivedRand;
				}

				//decode packet by type placed in header id
				switch(sPacket1.header.ID)
				{
					case NS_EMPTY:
						break;

					case NS_DONE:
						pMono->Out("NS_DONE received\n");
						//if a reselect
						if(enemyHasSelected)
						{
							enemyFlagGuy[enemyCurrentGuy].Hide();
							ResetCurrentGuy(NS_ENEMY,pPacket->curGuy);
						}
						else
						{
							enemyCurrentGuy = pPacket->curGuy;
						}

						enemyHasSelected = TRUE;
						enemyFlagFlashTimer = flagFlashTimer;
						break;

					case NS_UNDONE:
						enemyHasSelected = FALSE;

						pEnemyTitle = (CelHeader *)AGetResData(gEnemyTitle);
						// clear region name
						memset(((uchar*)pEnemyTitle)+sizeof(CelHeader),SKIP_COLOR,L2R(pEnemyTitle->width) * pEnemyTitle->height);

						//make sure the old region is cut if cursor isn't there any more
						ResetCurrentGuy(NS_ENEMY,enemyCurrentGuy);
						enemyFlagGuy[enemyCurrentGuy].Hide();
						break;

					case BMSG_DISCONNECT:
						pCommMgr->Disconnect();
						bGlobal.netDisconnect = TRUE;
						bGlobal.roomMgr.newRoomMode = MM_MENU_DOUBLE;
						TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
						pPop->SetMode(M_MODAL);
						pPop->Setup(pal.gSelf,OPTION_SQB,40,BR_MENU);//"opp has discon"
						break;
				}
			}
		}
		currFrame++;
	}

	//user 1 creates and sends the new rand number
	if(!currFrame && pCommMgr->GetUserID())
		newRand = ARandom2(32768);

	nsPacket.frameNum = currFrame;
	nsPacket.newRand = pCommMgr->GetUserID() ? newRand : receivedRand;
	nsPacket.coordX = theX;
	nsPacket.coordY = theY;
	nsPacket.curGuy = currentGuy;

	// send it out
	sPacket1.header.ID = packetType;
	sPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player
	sPacket1.header.len = sizeof(NetStoryPacket);
	sPacket1.pData = &nsPacket;

	//we've send our packet now lets clear it
	packetType = NS_EMPTY;

	UpdateTicks();
	currTicks = ATicks();

	// send off the packet
	pCommMgr->SendData(&sPacket1);
	error = pCommMgr->GetError();
	if(error)
	{
		pMono->Out(" Error%d(%s)\n", error, pErrorStrings[error]);
		pComm->ClearError();

		//if syncing a new room keep trying until somebody cancels us

		if(syncingNewRoom && sendErrCnt++ > 10)
		{
			pMono->Out("NetStory::Excessive retries on send, still trying...\n");
			errorSqbNum = 47;
			return(FALSE);
		}

		if(!syncingNewRoom && sendErrCnt++ >= 3)
		{
			pMono->Out("NetStory::Too many retries on send, aborting..\n");
			errorSqbNum = 47;
			return(FALSE);
		}
	}
	else
	{
		syncingNewRoom = FALSE;
		sendErrCnt = 0;
	}

	return TRUE;
}



void
NetStory::CycleEnemy()
{
	coord			celOffX,celOffY;
	BAM_Guy		*pGuy,*pOutGuy;
	uchar			color;
	bool			found=FALSE;
	uint32		calcGuy,x;
	CelHeader	*pEnemyTitle;

	if(!enemyHasSelected)
	{
		// now lets check the storyline regions
		for(x=0; x<NETMAXSTORYGUYS; x++)
		{
			pGuy = &storyGuy[x];

			if (pGuy->scrim.rect.Contains(enemyCurX,enemyCurY))
			{
				celOffX = enemyCurX - pGuy->scrim.rect.x1;
				celOffY = enemyCurY - pGuy->scrim.rect.y1;
				color = GetPixel(celOffX, celOffY, pGuy->scrim.resType, 
								pGuy->scrim.resNum, pGuy->scrim.cel);
				if (color != CI_SKIP) //we found it
				{
					found = TRUE;

					calcGuy = x;

					if (calcGuy != enemyCurrentGuy) //if new guy
					{
						if (pMono)
							pMono->Out("\nRegion is %2d, StoryLine is %d",x,GetStory(x));

						// cut old storyGuy only if there is one and un-enemy
						// isn't using it
						if (enemyCurrentGuy != -1 && enemyCurrentGuy != currentGuy)
						{
							pOutGuy = &storyGuy[enemyCurrentGuy];
							pOutGuy->fIsActive = FALSE;
							CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
						}

						enemyCurrentGuy = calcGuy;

						// add new region outline unless enemy has the region alread active
						if(enemyCurrentGuy != currentGuy)
						{
							pOutGuy = &storyGuy[calcGuy];
							pOutGuy->fIsActive = TRUE;
							AddObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
						}

						pEnemyTitle = (CelHeader *)AGetResData(gEnemyTitle);

						// clear region name
						memset(((uchar*)pEnemyTitle)+sizeof(CelHeader),SKIP_COLOR,L2R(pEnemyTitle->width) * pEnemyTitle->height);

						pFontMgr->SetRes(9050);
						SetFontColors(CI_SKIP,93,90,93,90,93,90);

						//startup new region name
						ASetString(0,0,storyNames[calcGuy], (uchar *)pEnemyTitle, pEnemyTitle->width, NULL, DG_JUST_CENTER);
						enemyTitle.SetPos(titlePos[enemyCurrentGuy*2],titlePos[enemyCurrentGuy*2+1]);
						break;
					}
				}//endif
			}//endif
		}//endfor

		//cursor outside of all guys and not used by un-enemy -shutdown current guy
		if(!found && enemyCurrentGuy != -1 && enemyCurrentGuy != currentGuy)
		{
			pOutGuy = &storyGuy[enemyCurrentGuy];
			pOutGuy->fIsActive = FALSE;
			CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);

			enemyCurrentGuy = -1;

			//shutdown last region name
			pEnemyTitle = (CelHeader *)AGetResData(gEnemyTitle);
			memset(((uchar*)pEnemyTitle)+sizeof(CelHeader),SKIP_COLOR,L2R(pEnemyTitle->width) * pEnemyTitle->height);
			enemyTitle.SetState(S_CHANGED, TRUE);
		}
	}
}


void
NetStory::ResetCurrentGuy(int side,int	newGuy)
{
	BAM_Guy		*pOutGuy;
	BAM_Guy		*ptitleGuy;
	CelHeader	*ptitle;

	int			*curGuy;
	int			*oppCurGuy;

	if(side == NS_LOCAL)
	{
		curGuy	 = &currentGuy;
		oppCurGuy = &enemyCurrentGuy;
		ptitle	 = (CelHeader *)AGetResData(gtitle);
		ptitleGuy = &title;
	}
	else
	{
		oppCurGuy = &currentGuy;
		curGuy	 = &enemyCurrentGuy;
		ptitle	 = (CelHeader *)AGetResData(gEnemyTitle);
		ptitleGuy = &enemyTitle;
	}


	// cut old storyGuy only if there is one and the OTHER guy
	// isn't using it
	if (*curGuy != -1 && *curGuy != *oppCurGuy)
	{
		pOutGuy = &storyGuy[*curGuy];
		pOutGuy->fIsActive = FALSE;
		CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
	}

	flagGuy[*curGuy].Hide();	//make sure old guy is hid

	*curGuy = newGuy;

	// add new region outline unless OTHER guy has the region already active
	if(*curGuy != *oppCurGuy)
	{
		pOutGuy = &storyGuy[newGuy];
		pOutGuy->fIsActive = TRUE;
		AddObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
	}

	// clear region name
	memset(((uchar*)ptitle)+sizeof(CelHeader),SKIP_COLOR,L2R(ptitle->width) * ptitle->height);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	//startup new region name
	ASetString(0,0,storyNames[newGuy], (uchar *)ptitle, ptitle->width, NULL, DG_JUST_CENTER);
	ptitleGuy->SetPos(titlePos[(*curGuy)*2],titlePos[(*curGuy)*2+1]);
}
