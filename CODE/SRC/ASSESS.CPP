// ASSESS.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Assessment statistics for player's victory.
//
// 2/10/95


#include "assess.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "apigraph.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "context.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <string.h>

#define LETTERANIM 8060
#define LAST_LETTER_CEL 5


//=========================================================
bool
Pen_Guy::HandleMsg(Message* pMsg)
{
	Assess		*pAss;
	Rectangle	r;

	pAss = ADerefAs(Assess,gContext);
	switch (pMsg->type)
	{
		case MSG_NOTICE:
			if (pMsg->notice.type == N_CUE)
			{
				//copy last frame into background -so letter stays "written"
				int ani = scrim.resNum - LETTERANIM;
				CelHeader *pbackCH = (CelHeader*)AGetResData(pAss->gback);

				int	posX,posY;

				if(pAss->wordSize == 4)
					posX = pAss->letterPosX[curWord][pAss->letterCnt];
				else
					posX = pAss->letterPosAltX[curWord][pAss->letterCnt];
				posY = pAss->letterPosY[pAss->curLine];

				//OK, now we know anim's will "justify" themselves by origin point
				//but copycels don't -all anims we use are origin 0,0 except 8061
				//NEW: trimskip removed the top row of empty pixels in the cel
				//     which fixes the bug that the following commented code use
				//     to fix
				//if(ani == 1)
				//	posY--;


				//should we write player or enemy color cels, but only for lines < 6
				if(curWord && pAss->curLine < 6)
					CopyCel(pbackCH,posX,posY,RES_CEL,pAss->letter[ani],1,TRUE,pAss->clut[1]);
				else
					CopyCel(pbackCH,posX,posY,RES_CEL,pAss->letter[ani],1,TRUE,pAss->clut[0]);

				r.Set(posX,posY,posX+8,posY+10);
				AUpdateRect(&r);
				letterDone = TRUE;
				return TRUE;
			}
			break;
	}//endswitch

	return FALSE;
}

void
Pen_Guy::Cycle(void)
{
	//if not done writing scores then continue
	Assess	*pAss;

	pAss = ADerefAs(Assess,gContext);
	if(!pAss->scoresWritten)
	{
		BAM_Guy::Cycle();
	}
}


//=========================================================

Assess::Assess()
{
	int	i;

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	scoresWritten = FALSE;
	fastForward = FALSE;
	dualWrite = TRUE;
	wordSize = 4; //will latter boost to 5
	launchMusic = 0;

	//========================
	//TEMP TEMP TEMP TEMP TEMP -FOR TESTING ONLY
	//
	//for(x=0;x<5;x++)
	//	objAchievedScore[x] = 123;
	//levelScore = 1234;
	//maxLevelScore = 9999;
	//
	//pBam->unitsCreated[SIDE1] = 1111;
	//pBam->unitsCreated[SIDE2] = 222;
	//pBam->unitsLost[SIDE1] = 3333;
	//pBam->unitsLost[SIDE2] = 444;
	//pBam->enemiesSlain[SIDE1] = 5555;
	//pBam->enemiesSlain[SIDE2] = 666;
	//pBam->structuresDestroyed[SIDE1] = 77;
	//pBam->structuresDestroyed[SIDE2] = 8;
	//pBam->sitesControlled[SIDE1] = 99;
	//pBam->sitesControlled[SIDE2] = 9;
	//
	//TEMP TEMP TEMP TEMP TEMP
	//========================

	memset(objAchievedScore,NULL,sizeof(objAchievedScore));
	levelScore = 0;
	maxLevelScore = 0;

	if(bGlobal.storyLine == NETGAME)
		curLineMax = 7;
	else
		curLineMax = 8;
	curLine = 0;
	curWord = 0;
	letterCnt = 0;
	shot = 0;

	memset(letterArr,NULL,sizeof(letterArr));

	if(pBam->playerSide == SIDE1)
	{
		playerSide = SIDE1;
		enemySide  = SIDE2;
	}
	else
	{
		playerSide = SIDE2;
		enemySide  = SIDE1;
	}

	for(i = 0; i < 2; i++)
	{
		TRACK_MEM("Assess Clut");	clut[i] = AMalloc(CLUT_SIZE);
	}
}

Assess::~Assess()
{
	int	i;

	for(i = 0; i < 2; i++)
	{
		AFree(clut[i]);
		clut[i] = 0;
	}

	for(i = 0; i < 10; i++)
	{
		AFlush(gLetter[i]);
	}
}

void
Assess::Setup()
{
	BAM_Guy		*pGuy;
	Pen_Guy		*pPen;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;
	int			x,y;
	uint			resNumBack;
	uchar 		*pClut1;
	uchar 		*pClut2;


	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		SetBasicNet(TRUE);

	// setup background cel filled with black
	TRACK_MEM("Assess: background cel");
	gback = ACreateCel(&resNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	TRACK_MEM("Assess: background anim");
	CopyCel(pbackCH,0,0,RES_ANIM,8000,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, resNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	//pal.FadeToBlack();
	pal.Load(8000);
	LoadUnitColors(pal.gSelf,pBam->sideColors[playerSide],pBam->sideColors[enemySide]);

	//======================================================
	//Setup dcels of the anim numbers so we can copy them in on fastforward

	uint	newResNum;
	for(x=0;x<10;x++)
	{
		gLetter[x] = ACloneRes(RES_ANIM,LETTERANIM+x,&newResNum,RES_CEL,LAST_LETTER_CEL);
		letter[x] = newResNum;
	}

	//===============================
	//setup cluts for pen guys and banner
	//we know player's color has been load into first clut range

	pClut1 = ADerefAs(uchar, clut[0]);
	pClut2 = ADerefAs(uchar, clut[1]);

	for(x = 0; x < CLUT_SIZE; x++)
	{
		pClut1[x] = (uchar)(CLUT1_START + x);
		pClut2[x] = (uchar)(CLUT2_START + x);
	}

	//======================================================
	// Write the necessary text into the background


	pFontMgr->SetRes(9056);
	SetFontColors(CI_SKIP,0,171,172,172,108,169);
	pTxt = sqbAssess.Load(ASSESS_SQB,1);	//'objective achieved'
	ASetString(0,92, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	pTxt = sqbAssess.Load(ASSESS_SQB,2);	//'followers created'
	ASetString(0,119, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	pTxt = sqbAssess.Load(ASSESS_SQB,3);	//'followers lost'
	ASetString(0,146, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	pTxt = sqbAssess.Load(ASSESS_SQB,4);	//'enemies slain'
	ASetString(0,173, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	pTxt = sqbAssess.Load(ASSESS_SQB,5);	//structures destroyed'
	ASetString(0,200, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	pTxt = sqbAssess.Load(ASSESS_SQB,6);	//'sites under control'
	ASetString(0,227, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	pTxt = sqbAssess.Load(ASSESS_SQB,7);	//'level score'
	ASetString(0,254, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);

	if(bGlobal.storyLine != NETGAME)
	{
		pTxt = sqbAssess.Load(ASSESS_SQB,8);	//'experience bonus'
		ASetString(0,281, pTxt, (uchar *)pbackCH, 181, NULL, DG_JUST_RIGHT);
	}

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,93,90,93,90,93,90);
	int savePoint;
	savePoint = pFontMgr->point;
	pFontMgr->point = 14;
	ASetString(98,17,bGlobal.curName, pback, 100, NULL);
	pGuy->SetState(S_CHANGED, TRUE);
	pFontMgr->point = savePoint;

	SetFontColors(CI_SKIP,19,21,19,21,19,21);
	pTxt = sqbAssess.Load(ASSESS_SQB,9); //"OF"
	ASetString(250,259, pTxt, pback, pbackCH->width, NULL);

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,19,21,229,22,229,CI_BLACK);
	pTxt = sqbAssess.Load(ASSESS_SQB,10); //'YOU'
	ASetString(221,48, pTxt, pback, pbackCH->width, NULL);
	pTxt = sqbAssess.Load(ASSESS_SQB,11); //'ENEMY'
	ASetString(263,48, pTxt, pback, pbackCH->width, NULL);

	if(bGlobal.storyLine == LEGEND)
	{
		GetLegendPath(legendPath,bGlobal.curPath);
		GetLevelName(&sqbRes,&sqbNum,LEGEND,legendPath[bGlobal.missionsDone]);

	}
	else
	{
		GetLevelName(&sqbRes,&sqbNum,bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);
	}
	pTxt = sqbAssess.Load(sqbRes,sqbNum);

	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK); //old color scheme
	ASetString(0,330, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);


	GetStoryName(&sqbRes,&sqbNum,bGlobal.storyLine);
	pTxt = sqbAssess.Load(sqbRes,sqbNum);

	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);
	ASetString(0,372, pTxt, pback, pbackCH->width, NULL, DG_JUST_CENTER);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);
	//======================================================
	// lets copy in all the borders and boundaries into the background
	TRACK_MEM("Assess:Banner Border");
	CopyCel(pbackCH,98,47,RES_ANIM,9004,4,TRUE);	// Banner border

	if(bGlobal.storyLine == LEGEND || bGlobal.storyLine == NETGAME)
	{
		CopyCel(pbackCH,50,13,RES_ANIM,bGlobal.curBodyAnim,1,TRUE);	// Body
		CopyCel(pbackCH,50,13,RES_ANIM,bGlobal.curFaceAnim,1,TRUE);	// Face
		CopyCel(pbackCH,50,13,RES_ANIM,bGlobal.curCoverAnim,1,TRUE);	// Cover
	}
	else
	{
		int portraitAnim = GetPortraitAnim(bGlobal.chooseSide,TRUE);
		CopyCel(pbackCH,50,13,RES_ANIM,portraitAnim,1,FALSE);	// Portrait
	}

	TRACK_MEM("Assess: Banner anim");
	CopyCel(pbackCH,100,49,RES_ANIM,9004,2,TRUE,clut[0]);	// banner 1

	//======================================================
	// lets setup animLetter guys

	uint16	posX[2][4]    = {    218,224,230,236,  266,272,278,284};
	uint16	posAltX[2][5] = {212,218,224,230,236,  266,272,278,284,290};

	for(x=0;x<2;x++)
		for(y=0;y<4;y++)
			letterPosX[x][y] = posX[x][y];

	for(x=0;x<2;x++)
		for(y=0;y<5;y++)
			letterPosAltX[x][y] = posAltX[x][y];

	uint16	posY[8] = {91,118,145,172,199,226,253,280};

	for(x=0;x<8;x++)
		letterPosY[x] = posY[x];


	CalcScores();
	SetupArr();


	//===============================
	//player's pen
	pPen = &pen1;
	pPen->curWord	= 0;		//pen1 assigned to column 1
	pPen->letterDone	= FALSE;
	pPen->SetContext(gSelf);
	pPen->Setup();
	pPen->SetPri(200);
	pPen->SetClut(192,CLUT_SIZE,clut[0]);
	SetupPen(pPen);

	//===============================
	//enemies' pen
	pPen = &pen2;
	pPen->curWord	= 1;
	pPen->letterDone	= FALSE;

	pPen->SetContext(gSelf);
	pPen->Setup();
	pPen->SetPri(200);
	pPen->SetClut(192,CLUT_SIZE,clut[1]);
	SetupPen(pPen);

	//======================================================
	// lets setup up all the buttons

	pButton = &diskButton;
	pButton->Create(0, 285, 200, RES_ANIM, 129, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(resNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	if(bGlobal.storyLine == NETGAME)
		pButton->Select(DG_DISABLED);
	else
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel


	//======================================
	//======================================
	bGlobal.missionsDone++;
	bGlobal.prevChooseSide = bGlobal.chooseSide; //used by Legend Campaign
	//======================================
	//======================================

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


bool
Assess::HandleMsg(Message* pMsg)
{
	char		mess[100];

	//let buttons and such have a shot at it
	if(BAM_Room::HandleMsg(pMsg))
		return TRUE;

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == diskButton.gSelf)
					{
						pMono->Out("\nDiskButton was hit");
						Option(pal.gSelf);
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
			if (pMsg->event.type == E_KEY_DOWN)
			{
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
					case K_X:
						return FALSE;
						break;
				}
			}

			if (pMsg->event.type == E_MOUSE_UP ||
			    pMsg->event.type == E_KEY_DOWN)
			{
				//use first click to acknowledge need to speedup.
				if(!scoresWritten && !fastForward)
				{
					fastForward = TRUE;
					return TRUE;
				}

				//any event now causes room to end.
				//now time to setup next map and play continuity cine's.

				#ifdef INTERACTIVE_DEMO
					bGlobal.roomMgr.NewRoom(BR_STORY);
				#else
					if(bGlobal.storyLine == NETGAME)
					{
						bGlobal.roomMgr.NewRoom(BR_NET_HALL);
					}
					else
					if(bGlobal.storyLine == LEGEND)
					{
						GetScenario(pBam->scenarioName,LEGEND,bGlobal.missionsDone,0);
						bGlobal.roomMgr.NewRoom(BR_HALL);
					}
					else
					{
						//if we've finished a complete storyline so...
						if (bGlobal.missionsDone > 2)
						{
							WriteStoryDone(bGlobal.storyLine);
						}

						bGlobal.cinematic = GetCine(bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);
						GetScenario(pBam->scenarioName,bGlobal.storyLine,bGlobal.missionsDone,bGlobal.chooseSide);
						bGlobal.roomMgr.NewRoom(BR_CINE);
					}
				#endif

				return(TRUE);
			}
			break;
	}//endswitch

	return FALSE;
}


void
Assess::Cycle(void)
{
	Pen_Guy *pPen;
	int		ani;

	//if not done writing scores then continue
	if(!scoresWritten)
	{
	 	if(fastForward) //skip animating text and just copy it in
		{
			//hide the pen guys
			pen1.Hide();
			pen2.Hide();

			//make sure current anim is written into background
			
			while (curLine < curLineMax)
			{
				while (curWord < 2)
				{
					while (letterCnt < wordSize)
					{
						ani = letterArr[curLine][curWord][letterCnt];
						if(ani)
						{
							ani -= LETTERANIM;
							CelHeader *pbackCH = (CelHeader*)AGetResData(gback);
							if(curLine < 6)
							{
								//if column two then use enemy color clut
								CopyCel(pbackCH,letterPosX[curWord][letterCnt],
											 		letterPosY[curLine],RES_CEL,
													letter[ani],1,TRUE,clut[curWord]);
							}
							else
							{
								wordSize = 5;
								// the last lines written have a wordSize of 5
								// and use diff. positioning. also no enemy color needed
								CopyCel(pbackCH,letterPosAltX[curWord][letterCnt],
												 	letterPosY[curLine],RES_CEL,
													letter[ani],1,TRUE,clut[0]);
							}
						}
						letterCnt++;
					}
					letterCnt = 0;
					curWord++;
				}
				letterCnt = 0;
				curWord = 0;
				curLine++;
			}

			back.SetState(S_CHANGED,TRUE);
			scoresWritten = TRUE; //will also shutdown anim pens
		}
		else
		{
			if(dualWrite && pen1.letterDone && pen2.letterDone)
			{
				letterCnt++;
				if(letterCnt >= wordSize)
				{
					curLine++;
					letterCnt = 0;

					//lets see if its time to switch to single write mode
					if(curLine == 6)  // 6 is the last dual line
						dualWrite = FALSE;
				}

				if(dualWrite) //skip over as soon as dualWrite goes false
				{
					//===============================
					pPen = &pen1;
					pPen->curWord	= 0;		//pen1 assigned to column 1
					pPen->letterDone	= FALSE;	//reset flag

					SetupPen(pPen);

					//===============================
					pPen = &pen2;
					pPen->curWord	= 1;
					pPen->letterDone	= FALSE;

					SetupPen(pPen);
				}
			}
			else if (!dualWrite && pen1.letterDone)
			{
				if(!shot)  //detect first entry into this section
				{
					shot++;
					wordSize = 5; //now boosted!
					curWord = 0;
					letterCnt = 0;
					pen2.Hide();
				}
				else
				{
					letterCnt++;
				}

				if(letterCnt >= wordSize)
				{
					letterCnt = 0;

					//Order of: word0,word1,next line,etc.
					if (curWord == 1)
					{
						curWord = 0;
						curLine++;
					}
					else
					{
						curWord = 1;
					}
				}

				//LETS CHECK TO SEE IF WE'VE COPIED ALL THE LINES
				if(curLine == curLineMax)
				{
					scoresWritten = TRUE; //will also shutdown anim pens
				}
				else
				{
					pPen = &pen1;
					pPen->curWord	= curWord;
					pPen->letterDone	= FALSE;

					SetupPen(pPen);
				}
			}
		}
	}

	if(!launchMusic)
	{
		launchMusic++;

		#ifdef INTERACTIVE_DEMO
			tMusic.Play(7002);
		#else
			pDigiMusic = pBam->FindAvailTSound();
			pDigiMusic->Play(7002,MAX_VOLUME,DIGI_PAN_CENTER,NULL,0,FALSE);
		#endif
	}

	//lets not let the music loop -it sounds pathetic
	//#ifdef INTERACTIVE_DEMO
	//	//make it loop
	//	if(!tMusic.IsPlaying())
	//		tMusic.Play(7002);	//restart
	//#endif

	// call the superclasses method last
	BAM_Room::Cycle();
}


void	
Assess::Cleanup()
{
	#ifdef INTERACTIVE_DEMO
		tMusic.Stop();
	#else
		pDigiMusic->Stop();
	#endif
}


void
Assess::SetupArr()
{
	char		wordArr[8];

	sprintf(wordArr,"%4d",objAchievedScore[playerSide]);
	LoadArr(0,0,wordArr);

	sprintf(wordArr,"%4d",objAchievedScore[enemySide]);
	LoadArr(0,1,wordArr);

	sprintf(wordArr,"%4d",pBam->unitsCreated[playerSide]);
	LoadArr(1,0,wordArr);

	sprintf(wordArr,"%4d",pBam->unitsCreated[enemySide]);
	LoadArr(1,1,wordArr);

	sprintf(wordArr,"%4d",pBam->unitsLost[playerSide]);
	LoadArr(2,0,wordArr);

	sprintf(wordArr,"%4d",pBam->unitsLost[enemySide]);
	LoadArr(2,1,wordArr);

	sprintf(wordArr,"%4d",pBam->enemiesSlain[playerSide]);
	LoadArr(3,0,wordArr);

	sprintf(wordArr,"%4d",pBam->enemiesSlain[enemySide]);
	LoadArr(3,1,wordArr);

	sprintf(wordArr,"%4d",pBam->structuresDestroyed[playerSide]);
	LoadArr(4,0,wordArr);

	sprintf(wordArr,"%4d",pBam->structuresDestroyed[enemySide]);
	LoadArr(4,1,wordArr);

	sprintf(wordArr,"%4d",pBam->sitesControlled[playerSide]);
	LoadArr(5,0,wordArr);

	sprintf(wordArr,"%4d",pBam->sitesControlled[enemySide]);
	LoadArr(5,1,wordArr);

	sprintf(wordArr,"%5d",levelScore);
	LoadArr(6,0,wordArr,5);

	sprintf(wordArr,"%5d",maxLevelScore);
	LoadArr(6,1,wordArr,5);

	sprintf(wordArr,"%5d",bonusXP);
	LoadArr(7,0,wordArr,5);
}


void
Assess::LoadArr(int lineIndex,int sideIndex,char* word, int wordSiz)
{
	int	x;

	for(x=0;x<wordSiz;x++)
	{
		if (word[x] == ' ')
			letterArr[lineIndex][sideIndex][x] = 0;
		else
			letterArr[lineIndex][sideIndex][x] = LETTERANIM + ((int)word[x] - 48);
	}
}


void
Assess::CalcScores()
{
	int	pct,sumSites;

	//note: missionsDone hasn't been incr yet
	int	mapNum = bGlobal.missionsDone+1;

	//objAchievedScore[playerSide] = pBam->objAchieved[playerSide] * (mapNum * 100);

	if(bGlobal.storyLine != NETGAME)
	{
		objAchievedScore[playerSide] = mapNum * 100;
		objAchievedScore[enemySide]  = 0;
	}
	else //netgame
	{
		if (bGlobal.netWinner == TRUE)
		{
			objAchievedScore[playerSide] = mapNum * 100;
			objAchievedScore[enemySide]  = 0;
		}
		else
		{
			objAchievedScore[playerSide] = 0;
			objAchievedScore[enemySide]  = mapNum * 100;
		}
	}

	levelScore = objAchievedScore[playerSide];
	maxLevelScore = mapNum * 100;

	//followers created/lost (% of (map# x 25)pts.)
	if(!pBam->unitsCreated[playerSide])
		pct = 0;
	else
		pct = ((pBam->unitsCreated[playerSide] - pBam->unitsLost[playerSide])*1000) / pBam->unitsCreated[playerSide];
	levelScore += (pct * (mapNum * 25))/1000;
	maxLevelScore += mapNum * 25;

	if(pBam->enemiesSlain[playerSide] == 0 && pBam->enemiesSlain[enemySide] == 0)
		pct = 1000;	//will prob. only happen in testing circumstances
	else
		//enemies slain (% of (map# x 25)pts)
		pct =  (pBam->enemiesSlain[playerSide]*1000) / (pBam->enemiesSlain[playerSide] + pBam->enemiesSlain[enemySide]);
	levelScore += (pct * (mapNum * 25))/1000;
	maxLevelScore += mapNum * 25;

	//sites under control
	sumSites = (pBam->sitesControlled[playerSide] + pBam->sitesControlled[enemySide] + pBam->totalFoundations);
	if(!sumSites)
		pct = 1000;	//no sites in  bonus map
	else
		pct =  (pBam->sitesControlled[playerSide]*1000) / sumSites;
	levelScore += (pct * (mapNum * 50))/1000;
	maxLevelScore += mapNum * 50;

	bGlobal.curCampaignScore += levelScore;
	bGlobal.curMaxCampaignScore += maxLevelScore;
	bGlobal.curLevelScore[bGlobal.missionsDone] = levelScore;

	if(bGlobal.storyLine == LEGEND)
	{
		bonusXP = levelScore;
		bGlobal.curXP[pBam->playerSide] += bonusXP;
	}
	else
	{
		bonusXP = (5*levelScore);
		bGlobal.curXP[pBam->playerSide] += bonusXP;
	}

	//legend only
	if(mapNum == 16)
	{
		bGlobal.curCampaignScore += 4600;
		bGlobal.curMaxCampaignScore += 4600;
	}

	//lets record players current XP's encase he restarts the map
	//use first one if player restarts here in assess or hall room
	bGlobal.lastLevelXP[0] = bGlobal.lastLevelXP[1];
	bGlobal.lastLevelXP[1] = bGlobal.curXP[pBam->playerSide];

	memcpy(bGlobal.lastUnitsResearched[0], bGlobal.lastUnitsResearched[1], UNITMAXTYPES);
	memcpy(bGlobal.lastUnitsResearched[1], bGlobal.unitsResearched, UNITMAXTYPES);

	//-------------------------------------------------------
	//alignment stuff

	//lets record current alignment vars. encase player restarts map
	bGlobal.lastLawful  = bGlobal.lawful;
	bGlobal.lastNeutral = bGlobal.neutral;
	bGlobal.lastChaotic = bGlobal.chaotic;

	bGlobal.lastEvil   = bGlobal.evil;
	bGlobal.lastNutral = bGlobal.nutral;
	bGlobal.lastGood   = bGlobal.good;

	//% followers created = ( your's created / (yours + his))
	if(pBam->unitsCreated[playerSide] == 0 && pBam->unitsCreated[enemySide] == 0)
		pct = 50;
	else
		pct = (pBam->unitsCreated[playerSide] * 100) / (pBam->unitsCreated[playerSide] + pBam->unitsCreated[enemySide]);
	AlignLawfulChaotic(pct);

	//% followers lost
	//guard against div by zero
	if(pBam->unitsLost[playerSide] == 0 && pBam->unitsLost[enemySide] == 0)
		pct = 50;
	else
		pct = (pBam->unitsLost[playerSide] * 100) / (pBam->unitsLost[playerSide] + pBam->unitsLost[enemySide]);
	AlignGoodEvil(pct);

	//% enemies slain
	if(pBam->enemiesSlain[playerSide] == 0 && pBam->enemiesSlain[enemySide] == 0)
		pct = 50;
	else
		pct =  (pBam->enemiesSlain[playerSide]*100) / (pBam->enemiesSlain[playerSide] + pBam->enemiesSlain[enemySide]);
	AlignLawfulChaotic(pct);
	AlignGoodEvil(pct);

	//% structures destroyed
	if(pBam->structuresDestroyed[playerSide] == 0 && pBam->structuresDestroyed[enemySide] == 0)
		pct = 50;
	else
		pct =  (pBam->structuresDestroyed[playerSide]*100) / (pBam->structuresDestroyed[playerSide] + pBam->structuresDestroyed[enemySide]);
	AlignGoodEvil(pct);

	//% sites under control
	if(pBam->sitesControlled[playerSide] == 0 && pBam->sitesControlled[enemySide] == 0)
		pct = 50;
	else
		pct =  (pBam->sitesControlled[playerSide]*100) / (pBam->sitesControlled[playerSide] + pBam->sitesControlled[enemySide]);
	AlignLawfulChaotic(pct);

}


void
Assess::SetupPen(Pen_Guy* pPen)
{
	if(letterArr[curLine][pPen->curWord][letterCnt] == 0)
	{
		pPen->Hide();
		pPen->letterDone = TRUE;
	}
	else
	{
		if(pPen->CheckState(S_HIDDEN))
			pPen->Show();
		pPen->SetRes(RES_ANIM, letterArr[curLine][pPen->curWord][letterCnt],1);

		if (curLine < 6)
			pPen->SetPos(letterPosX[pPen->curWord][letterCnt],letterPosY[curLine]);
		else
			pPen->SetPos(letterPosAltX[pPen->curWord][letterCnt],letterPosY[curLine]);

		TRACK_MEM("Assess:new Animator");	pPen->SetupUtil(new Animator(AN_TO_END,pPen->gSelf,2));
	}
}
