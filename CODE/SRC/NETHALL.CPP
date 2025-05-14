// NETHALL.CPP
//
//	Copyright 1995 Tachyon, Inc.
//
//
// Display Hall of Net Champions.
//
// 10/8/95
//

#include "nethall.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "apires.hpp"
#include "context.hpp"
#include	"graphmgr.hpp"
#include	"mouse.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <string.h>

#define SCROLL_STEP_SIZE 80
#define SCROLL_STEPS 4
#define CLUT3_START			208
#define CLUT4_START			216
#define CLUT5_START			224
#define CLUT6_START			232
#define SLASH 0
#define DOT   1

#define HALL_BANNER_ANIM	9004
#define HALL_BANNER_SLASH	2
#define HALL_BANNER_DOTS	3


//=========================================================

NetHall::NetHall()
{
	int x,i,j;

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	gCampArr = 0;

	scrollingDone = TRUE;
	displayCampaign = FALSE;
	buttonsLocked = FALSE;
	scrollDir = 0;
	centerColumnMode = 1;
	centerColorMode = 0;
	deleteCnt = 0;
	immedBail = FALSE;

	i = 30;
	j = 12;
	int	TxPosNames[2] = {0,147};
	int	TxPosScores[2] = {97,246};
	int	TyPos[8] = {i,i+j,i+(2*j),i+(3*j),i+(4*j),i+(5*j),i+(6*j),i+(7*j)};

	for(x=0;x<2;x++)
	{
		xPosNames[x] = TxPosNames[x];
		xPosScores[x] = TxPosScores[x];
	}

	for(x=0;x<8;x++)
		yPos[x] = TyPos[x];

	for(x = 0; x < 2; x++)
	{
		TRACK_MEM("NetHall Clut");	clut[x] = AMalloc(CLUT_SIZE);
	}
}

NetHall::~NetHall()
{
	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);

	int	i;

	for(i = 0; i < 2; i++)
	{
		AFree(clut[i]);
		clut[i] = 0;
	}

	if(gCampArr)
		AFree(gCampArr);
}

void
NetHall::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;
	int			x;
	uchar 		*pClut1;
	uchar 		*pClut2;
	bool			bail = FALSE;

	if(bGlobal.roomMgr.prevRoomNum == BR_ASSESS)
		duringPlay = TRUE; //used to on/off certain features like delete button
	else
		duringPlay = FALSE;

	//defaults -assume setup went wrong
	bGlobal.roomMgr.newRoomMode = bGlobal.roomMgr.prevRoomMode;
	bGlobal.roomMgr.prevRoomMode = HALL_NO_NET_CAMP_CHARS ;

	if(!SetupCampaign())
	{
		//we can't just call newroom here 'cause we're still in setup()

		//gotta reset mouse pointer
		BAM_Room::Setup();

		immedBail = TRUE;
		return;
	}
		
	bGlobal.roomMgr.prevRoomMode = HALL_NO_ERROR; //It's ok, so reset

	//we know we're in a net game
	if(!bGlobal.netDisconnect)
		SetBasicNet(TRUE);

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("NetHall: background cel");
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	TRACK_MEM("NetHall: background anim");
	CopyCel(pbackCH,0,0,RES_ANIM,8050,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(100);

	//pal.FadeToBlack();
	pal.Load(8050);

	//setup cluts for banners
	pClut1 = ADerefAs(uchar, clut[0]);
	pClut2 = ADerefAs(uchar, clut[1]);

	for(x = 0; x < CLUT_SIZE; x++)
	{
		pClut1[x] = (uchar)(CLUT1_START + x);
		pClut2[x] = (uchar)(CLUT2_START + x);
	}

	SetupHalls();

	LoadHall(0);

	//=================
	// Write the necessary text into the background

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK); //CARVED GREEN

	//"HALL OF LEGENDS"
	pTxt = sqbHall.Load(HALL_SQB,1);
	ASetString(0,9, pTxt, (uchar *)pbackCH, pbackCH->width, NULL, DG_JUST_CENTER);

	//"TOTAL SCORE"
	pTxt = sqbHall.Load(HALL_SQB,18);
	ASetString(91,244,pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	//===================================================
	//setup current Guy
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);
	TRACK_MEM("NetHall:currentGuy");
	gCurrent = ACreateCel(&rNumCurrent,0,0,60,15,CI_SKIP,200);
	uchar *pCurrent = AGetResData(gCurrent);

	//"CURRENT"
	pTxt = sqbHall.Load(HALL_SQB,3);
	ASetString(0,0,pTxt, pCurrent, 60, NULL);
	pGuy = &currentGuy;
	pGuy->SetRes(RES_CEL, rNumCurrent);
	pGuy->SetPos(25,45);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);
	if(!duringPlay)
		pGuy->Hide();

	//===================================================
	//setup name/rank Guys
	TRACK_MEM("NetHall:campNameGuy");
	gCampName = ACreateCel(&rNumCampName,0,0,240,22,CI_SKIP,200);
	pGuy = &campNameGuy;
	pGuy->SetRes(RES_CEL,rNumCampName);
	pGuy->SetPos(40,43);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	TRACK_MEM("NetHall:campRankGuy");
	gCampRank = ACreateCel(&rNumCampRank,0,0,240,22,CI_SKIP,200);
	pGuy = &campRankGuy;
	pGuy->SetRes(RES_CEL,rNumCampRank);
	pGuy->SetPos(40,66);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	LoadNameRank();

	//===================================================
	//setup Score Guys
	TRACK_MEM("NetHall:campScoreGuy");
	gCampScore = ACreateCel(&rNumCampScore,0,0,50,22,CI_SKIP,200);
	pGuy = &campScoreGuy;
	pGuy->SetRes(RES_CEL,rNumCampScore);
	pGuy->SetPos(173,244);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	LoadCampaignScores(); //into newly created guys

	//================================
	TRACK_MEM("NetHall:WinLoseStatGuy");
	gWinLoseStat = ACreateCel(&rNumWinLoseStat,0,0,292,124,CI_SKIP,200);
	pGuy = &winLoseStatGuy;
	pGuy->SetRes(RES_CEL,rNumWinLoseStat);
	pGuy->SetPos(26,272);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	LoadWinLoseStats(); //into newly created guy

	//======================================================
	// lets setup up all the buttons

	//if(!duringPlay)
	//{
	//	pButton = &diskButton;
	//	pButton->Create(0, 236, 200, RES_ANIM, 129, 1, gSelf);
	//	pButton->SetupReplies(REPLY_DESELECTED);
	//	pButton->fIsToggle = FALSE;		// click-type button
	//	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	//	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
	//}

	pButton = &leftButton;
	pButton->Create(23, 64, 200, RES_ANIM, 8055, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pButton = &rightButton;
	pButton->Create(268, 64, 200, RES_ANIM, 8056, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	pFontMgr->SetRes(9050);

	//don't need -will use button cover-up cel 3
	//gTemp = ALoad(RES_ANIM,8052);
	//CopyCel(pbackCH,260,236,gTemp,9,TRUE);	// del button border

	pButton = &delButton;
	pButton->Create(262, 236, 200, RES_ANIM, 8054, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	//"8"
	pTxt = sqbHall.Load(HALL_SQB,8);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	if(duringPlay)
	{
		pButton->Listen(FALSE);
		pButton->SetCel(3);
		pButton->Draw();
	}
	else
	{
		pButton->Select(FALSE);	 // set button to unselected state - will cause drawing into master cel
	}

	pButton = &exitButton;
	pButton->Create(291, 236, 200, RES_ANIM, 8054, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	//"9"
	pTxt = sqbHall.Load(HALL_SQB,9);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel

	ConfigButtons();

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


void
NetHall::SetupHalls()
{
	BAM_Guy	*pGuy;

	//THESE THREE CELS WILL BE TRADED AMONGST THE THREE HALL GUYS
	TRACK_MEM("Left Hall Guy");
	gLeftHall = ACreateCel(&rNumLeftHall,0,0,320,142,254,200);

	TRACK_MEM("Center Hall Guy");
	gCenterHall = ACreateCel(&rNumCenterHall,0,0,320,142,254,200);

	TRACK_MEM("Right Hall Guy");
	gRightHall = ACreateCel(&rNumRightHall,0,0,320,142,254,200);

	pGuy = &leftHallGuy;
	pGuy->SetRes(RES_CEL, rNumLeftHall);
	pGuy->SetPos(-320,94);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	pGuy = &centerHallGuy;
	pGuy->SetRes(RES_CEL, rNumCenterHall);
	pGuy->SetPos(0,94);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	pGuy = &rightHallGuy;
	pGuy->SetRes(RES_CEL, rNumRightHall);
	pGuy->SetPos(320,94);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);
}


//*************************************************************
//Section: LoadHall and related functions
//*************************************************************

void
NetHall::LoadHall(int mode)
{
	CelHeader	*pCH;
	int			idx,color,columnMode,destRange,rangeIdx;
	uint			rNumTemp;

	if(!mode) //center panel
	{
		pCH = (CelHeader*)AGetResData(gCenterHall);
		idx = campCnt;
		columnMode = centerColumnMode;
		destRange = (centerColorMode)?200:192; //for banner
		rangeIdx  = centerColorMode;
	}
	else if(mode == 1) //right panel
	{
		pCH = (CelHeader*)AGetResData(gRightHall);
		idx = rightIdx;
		columnMode = centerColumnMode;
		//columnMode++; //columnMode is now fixed at showing two colomns
		//if(columnMode > 3)
		//	columnMode = 1;
		destRange = (centerColorMode)?192:200;
		rangeIdx  = (centerColorMode)?0:1;
	}
	else //left panel
	{
		pCH = (CelHeader*)AGetResData(gLeftHall);
		idx = leftIdx;
		columnMode = centerColumnMode;
		//columnMode--; //columnMode is now fixed at showing two columns
		//if(!columnMode)
		//	columnMode = 3;
		destRange = (centerColorMode)?192:200;
		rangeIdx  = (centerColorMode)?0:1;
	}


	//--------------------
	//Load hall,column, and column shadow cels
	rNumTemp = 8052;
	CopyCel(pCH,0,0,RES_ANIM,rNumTemp,1,FALSE);	// Left Hall
	CopyCel(pCH,160,0,RES_ANIM,rNumTemp,2,FALSE);	// Right Hall

	switch(columnMode)
	{
		case 1:
			//copy both columns w/shadows
			CopyCel(pCH,12 ,0,RES_ANIM,rNumTemp,3,FALSE);	// Left Column
			CopyCel(pCH,254,0,RES_ANIM,rNumTemp,4,FALSE);	// Right Column
			CopyCel(pCH,0  ,0,RES_ANIM,rNumTemp,5,FALSE);	// Left shadow
			CopyCel(pCH,242,0,RES_ANIM,rNumTemp,6,FALSE);	// Right shadow
			break;
		case 2:
			//copy left column w/shadow
			CopyCel(pCH,12 ,0,RES_ANIM,rNumTemp,3,FALSE);	// Left Column
			CopyCel(pCH,0  ,0,RES_ANIM,rNumTemp,5,FALSE);	// Left shadow
			break;
		case 3:
			//copy right column w/shadow
			CopyCel(pCH,254,0,RES_ANIM,rNumTemp,4,FALSE);	// Right Column
			CopyCel(pCH,242,0,RES_ANIM,rNumTemp,6,FALSE);	// Right shadow
			break;
		default:
			APanic("NetHall: Invalid columnMode number");
	}

	//--------------------
	//Load portrait
	CopyCel(pCH,140,24,RES_ANIM,pCampArr[idx].curBody,1,TRUE);	// Body
	CopyCel(pCH,140,24,RES_ANIM,pCampArr[idx].curFace,1,TRUE);	// Face
	CopyCel(pCH,140,24,RES_ANIM,pCampArr[idx].curCover,1,TRUE);	// Cover

	//--------------------
	//Load banner
	color = pCampArr[idx].curBanner / 2;

	pal.LoadPartial(9101,192+(color*8),8,destRange);
	pal.UpdateColors(destRange,destRange+8);

	//even num is slash, odd num is dots banner
	//SIDE1 is always slash
	if(pCampArr[idx].curBanner % 2)
		CopyCel(pCH,140,102,RES_ANIM,HALL_BANNER_ANIM,HALL_BANNER_DOTS,TRUE,clut[rangeIdx]);
	else
		CopyCel(pCH,140,102,RES_ANIM,HALL_BANNER_ANIM,HALL_BANNER_SLASH,TRUE,clut[rangeIdx]);

}


//*************************************************************
//Section: SetupCampaign and its related functions
//*************************************************************

int
NetCompareCamp(void const *pparm1,void const *pparm2)
{
	NetCampaign	*parm1,*parm2;

	parm1 = (NetCampaign*)pparm1;
	parm2 = (NetCampaign*)pparm2;

	if(parm1->campaignScore < parm2->campaignScore)
		return (-1);
	else if (parm1->campaignScore > parm2->campaignScore)
		return 1;
	else
		return 0;
}


int
NetCompareOpp(void const *pparm1,void const *pparm2)
{
	NetOpp	*parm1,*parm2;

	parm1 = (NetOpp*)pparm1;
	parm2 = (NetOpp*)pparm2;

	if(parm1->oppNumWin < parm2->oppNumWin)
		return (-1);
	else if (parm1->oppNumWin > parm2->oppNumWin)
		return 1;
	else
		return 0;
}


//returns whether it found any chars in campaign file
bool
NetHall::SetupCampaign()
{
	int				x,y,slot;
	int				charSize;
	NetCampaignHeader	*pHead;
	NetCampaign 		*pCamp;
	bool				overwrite = FALSE;
	bool  			newFile = FALSE;
	bool				foundOpp = FALSE;

	//open netcamp.dat
	//	-if it doesn't exist then create
	//if more than one entry then sort
	//find position of current entry for start point -put in entryCnt

	if(bGlobal.writeOut)
	{
		campFile = fOpenPrefs("netcamp.dat", "rb+");

		if(!campFile)
		{
			//no file, no chars found
			if(!duringPlay)
				return FALSE;

			campFile = fOpenPrefs("netcamp.dat", "wb+"); //create it

			newFile = TRUE;
			if(!campFile)
			{
				//APanic("NetHall: Error, couldn't open netcamp.dat");
				bGlobal.roomMgr.prevRoomMode = HALL_NO_OPEN_NET_FILE;
				return FALSE;
			}
		}
	}
	else
	{
		//lets pretend new file since writes not allowed

		//no file, no chars found
		//if trying to revue chars from mainmenu -fail
		if(!duringPlay)
			return FALSE;

		newFile = TRUE;
	}


	pHead = &header;

	if(newFile)
	{
		//setup new file header
		memset(pHead,0,sizeof(NetCampaignHeader));

		//lets now write out at least a emptyheader to the file if we can
		if(bGlobal.writeOut)
			fwrite(&header,sizeof(NetCampaignHeader),1,campFile);
	}
	else
	{
		//read file header
		charSize = fread(pHead,sizeof(NetCampaignHeader),1,campFile);
		if(!charSize)
		{
			fclose(campFile);
			//APanic("NetHall: Error, couldn't read header in netcamp.dat");
			bGlobal.roomMgr.prevRoomMode = HALL_NO_READ_NET_HEADER;
			return FALSE;
		}
	}

	//if trying to look at chars from mainmenu and no chars
	if((newFile || !pHead->entries) && !duringPlay)
	{
		if(campFile)
			fclose(campFile);
		return FALSE;
	}
	
	size_t theSize = (pHead->entries+1)*sizeof(NetCampaign);
	TRACK_MEM("NetCampArr");	gCampArr = AMalloc(theSize);
	pCampArr = ADerefAs(NetCampaign,gCampArr);
	pCamp = pCampArr;

	//lets clear all the mem
	memset(pCampArr,NULL,theSize);

	if(!newFile)
	{
		//read structures from file into array
		for(x=0;x<pHead->entries;x++)
		{
			charSize = fread(pCamp,1,sizeof(NetCampaign),campFile);
			if(!charSize)
			{
				pMono->Out("NetHall: Warning -read 0 bytes from netcamp.dat");
				fclose(campFile);
				return FALSE;
			}

			//if struct found in file w/ same Id and less missions done then overwrite
			//NOTE: there is no current campaign if var. duringPlay not set
	 		if(duringPlay && pCamp->curCharId == bGlobal.curCharId)
			{
				//NOW WE DON'T OVERWRITE -BECAUSE THIS ASSUMES A NEWLY CREATED
				//NET CHAR. WILL HAVE A UNIQUE ID. NOW ITS BEEN DECIDED THAT NEW
				//OR OLD IF A CHAR. HAS THE SAME NAME AND LOOK THEN HE HAS THE
				//SAME ID. SO NOW WE DON'T OVERWRITE WE "MERGE". (We only
				//update the existing stats in the file.

				//"I've found a match" flag
				overwrite = TRUE;

				pCamp->missionsDone++;

				if(bGlobal.netWinner)
				{
					pCamp->missionsWon++;

					foundOpp = FALSE;
					for(y=0;y<MAX_OPP_LIST_SIZE;y++)
					{
						if(pCamp->win[y].oppCharId == bGlobal.enemyCharId)
						{
							foundOpp = TRUE;
							break;
						}
					}

					if(foundOpp)
					{
						//just need to update win cnt
						pCamp->win[y].oppNumWin++; 	
					}
					else
					{
						//new opponent beaten

						//list not full?
						if(pCamp->oppBeatenNum < MAX_OPP_LIST_SIZE)
						{
							slot = pCamp->oppBeatenNum++;

							strcpy(pCamp->win[slot].oppName,bGlobal.enemyName);
							pCamp->win[slot].oppCharId = bGlobal.enemyCharId;
							pCamp->win[slot].oppNumWin = 1;
						}
						else
						{
							//list is full -replace last in the list
							slot = MAX_OPP_LIST_SIZE-1;

							strcpy(pCamp->win[slot].oppName,bGlobal.enemyName);
							pCamp->win[slot].oppCharId = bGlobal.enemyCharId;
							pCamp->win[slot].oppNumWin = 1;
						}
					}

					//sort the beaten enemy list
					qsort((void*)pCamp->win, pCamp->oppBeatenNum, sizeof(NetOpp), NetCompareOpp);

				}
				else
				{
					pCamp->missionsLost++;

					foundOpp = FALSE;
					for(y=0;y<MAX_OPP_LIST_SIZE;y++)
					{
						if(pCamp->lose[y].oppCharId == bGlobal.enemyCharId)
						{
							foundOpp = TRUE;
							break;
						}
					}

					if(foundOpp)
					{
						pCamp->lose[y].oppNumWin++; 	
					}
					else
					{
						//new opponent lost to

						//list not full?
						if(pCamp->oppLostToNum < MAX_OPP_LIST_SIZE)
						{
							slot = pCamp->oppLostToNum++;

							strcpy(pCamp->lose[slot].oppName,bGlobal.enemyName);
							pCamp->lose[slot].oppCharId = bGlobal.enemyCharId;
							pCamp->lose[slot].oppNumWin = 1;
						}
						else
						{
							//list is full -replace last in the list
							slot = MAX_OPP_LIST_SIZE-1;

							strcpy(pCamp->lose[slot].oppName,bGlobal.enemyName);
							pCamp->lose[slot].oppCharId = bGlobal.enemyCharId;
							pCamp->lose[slot].oppNumWin = 1;
							
						}
					}

					//sort the lost to enemy list
					qsort((void*)pCamp->lose,pCamp->oppLostToNum,sizeof(NetOpp), NetCompareOpp);
				}

				//all this stuff should already match
				//pCamp->deleted = FALSE;
				//pCamp->curCover = bGlobal.curCoverAnim;
				//pCamp->curFace = bGlobal.curFaceAnim;
				//pCamp->curBody = bGlobal.curBodyAnim;
				//pCamp->curBanner = bGlobal.curBanner;
				//strcpy(pCamp->curName,bGlobal.curName);

				pCamp->campaignScore += bGlobal.curCampaignScore;

				pCamp->lawful  += bGlobal.lawful;
				pCamp->neutral	+= bGlobal.neutral;
				pCamp->chaotic	+= bGlobal.chaotic;
				pCamp->evil		+= bGlobal.evil;
				pCamp->nutral	+= bGlobal.nutral;
				pCamp->good		+= bGlobal.good;

			}
			pCamp++;
		}
	}

	// if no overwrite then lets add current campaign to end of array
	//NOTE: there is no current campaign if var. duringPlay not set
	if(duringPlay && !overwrite)
	{
		pHead->entries++;
		//Hint: pCamp is now at last entry in the array
		pCamp->curCharId = bGlobal.curCharId;
		pCamp->deleted = FALSE;
		pCamp->missionsDone = bGlobal.missionsDone;
		if(bGlobal.netWinner)
		{
			pCamp->missionsWon = 1;
			pCamp->oppBeatenNum = 1;

			strcpy(pCamp->win[0].oppName,bGlobal.enemyName);
			pCamp->win[0].oppCharId = bGlobal.enemyCharId;
			pCamp->win[0].oppNumWin = 1;
		}
		else
		{
			pCamp->missionsLost = 1;
			pCamp->oppLostToNum = 1;

			strcpy(pCamp->lose[0].oppName,bGlobal.enemyName);
			pCamp->lose[0].oppCharId = bGlobal.enemyCharId;
			pCamp->lose[0].oppNumWin = 1;
		}
		pCamp->curCover = bGlobal.curCoverAnim;
		pCamp->curFace = bGlobal.curFaceAnim;
		pCamp->curBody = bGlobal.curBodyAnim;
		pCamp->curBanner = bGlobal.curBanner;
		pCamp->curCat = bGlobal.curCat;
		strcpy(pCamp->curName,bGlobal.curName);
		pCamp->campaignScore = bGlobal.curCampaignScore;

		pCamp->lawful  += bGlobal.lawful;
		pCamp->neutral	+= bGlobal.neutral;
		pCamp->chaotic	+= bGlobal.chaotic;
		pCamp->evil		+= bGlobal.evil;
		pCamp->nutral	+= bGlobal.nutral;
		pCamp->good		+= bGlobal.good;
	}

	if(pHead->entries > 1)
		qsort((void*)pCampArr,pHead->entries,sizeof(NetCampaign), NetCompareCamp);

	pCamp = pCampArr; //reset

	//find most current entry in the sorted array
	if(duringPlay)
	{
		for(x=0;x<pHead->entries;x++)
		{
			if(pCamp->curCharId == bGlobal.curCharId)
			{
				pHead->mostRecent = x;
				campCnt = x;
				break;
			}
			pCamp++;
		}
	}
	else
	{
		campCnt = pHead->mostRecent;
	}


	//only need to write out if we've updated an entry during play
	if(duringPlay && bGlobal.writeOut)
	{
		//write new file back out
		pCamp = pCampArr; //reset
		rewind(campFile);
		fwrite(&header,sizeof(NetCampaignHeader),1,campFile);
		for(x=0;x<header.entries;x++)
		{
			if(!pCamp->deleted)
				fwrite(pCamp,sizeof(NetCampaign),1,campFile);
			pCamp++;
		}
	}

	if(campFile)
		fclose(campFile);

	return(TRUE);
}


//rewrite campaign file without deleted records
void
NetHall::WriteCamp()
{
	int  	x,cnt = 0;
	NetCampaignHeader tmpHeader;

	if(bGlobal.writeOut)
	{
		pCamp = pCampArr;

		campFile = fOpenPrefs("netcamp.dat", "wb+");
		if(!campFile)
			APanic("NetHall: Error, couldn't open netcamp.dat");

		//lets count number of valid undeleted records
		for(x=0;x<header.entries;x++)
		{
			if(!pCamp->deleted)
				cnt++;
			pCamp++;
		}
		tmpHeader = header;
		tmpHeader.entries = cnt;

		pCamp = pCampArr;
		//write new file back out
		fwrite(&tmpHeader,sizeof(NetCampaignHeader),1,campFile);
		for(x=0;x<header.entries;x++)
		{
			if(!pCamp->deleted)
				fwrite(pCamp,sizeof(NetCampaign),1,campFile);
			pCamp++;
		}
		fclose(campFile);
	}
}


//*************************************************************
//Section: DisplayCampaign and its related functions
//*************************************************************

void
NetHall::DisplayCampaign()
{
	pCampArr = ADerefAs(NetCampaign,gCampArr);
	ConfigButtons(scrollDir);
	CheckMostRecent();
	LoadNameRank();
	LoadCampaignScores();
	LoadWinLoseStats();
}


void
NetHall::ConfigButtons(int dir)
{
	if(!dir)
	{
		leftIdx   = FindValid(campCnt,-1);
		rightIdx  = FindValid(campCnt,1);
	}
	else if(dir == 1)
	{
		campCnt  = rightIdx;
		leftIdx  = FindValid(campCnt,-1); //we recheck this Idx in case it was just deleted
		rightIdx = FindValid(campCnt,1);
	}
	else if(dir == -1)
	{
		campCnt  = leftIdx;
		leftIdx  = FindValid(campCnt,-1);
		rightIdx = FindValid(campCnt,1); //we recheck this Idx in case it was just deleted
	}


	if(leftIdx == -1)
	{
		//leftButton.Hide(); don't work
		leftButton.SetCel(3); //show "background"
		leftButton.Listen(FALSE);
		leftButton.Draw();
	}
	else
	{
		//leftButton.Show();
		leftButton.Select(FALSE);
		leftButton.Listen(TRUE);
	}

	if(rightIdx == -1)
	{
		//rightButton.Hide();
		rightButton.SetCel(3); //show "background"
		rightButton.Listen(FALSE);
		rightButton.Draw();
	}
	else
	{
		//rightButton.Show();
		rightButton.Select(FALSE);
		rightButton.Listen(TRUE);
	}
}


// dir should always be passed as 1 or -1.
int
NetHall::FindValid(int campCnt_P,int dir)
{
	int	tmpCnt;
	bool  done = FALSE;

	//(dir == -1) is left, (dir == 1) is right
	tmpCnt = campCnt_P;

	if(dir == -1)
	{
		while(!done)
		{
			tmpCnt--;
			if(tmpCnt > -1)
			{
				//test for valid entry
				if(!pCampArr[tmpCnt].deleted)
				{
					return tmpCnt;
				}
				//else fall-thru and loop and check next entry
			}
			else
			{
				//we hit -1 and found no valid entry -so bail out
				done = TRUE;
			}
		}
	}
	else if (dir == 1)
	{
		while(!done)
		{
			tmpCnt++;
			if(tmpCnt < header.entries)
			{
				//test for valid entry
				if(!pCampArr[tmpCnt].deleted)
				{
					return tmpCnt;
				}
				//else fall-thru and loop and check next entry
			}
			else
			{
				//we hit max and found no valid entry -so bail out
				done = TRUE;
			}
		}
	}

	return (-1);
}
		

void
NetHall::CheckMostRecent()
{
	BAM_Button	*pButton = &delButton;

	//if at most recent campaign score then announce it -if in a game
	if(!duringPlay)
	{
	 	currentGuy.Hide();
	}
	else
	{
		if(campCnt == header.mostRecent)
		{
			currentGuy.Show();

			//hide 'delete' button -can't delete current campaign
			pButton->Listen(FALSE);
			pButton->SetCel(3);
			pButton->Draw();
		}
		else
		{
			currentGuy.Hide();

			pFontMgr->SetRes(9050);
			pButton->Listen(TRUE);
			pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
		}
	}
}


void
NetHall::LoadNameRank()
{
	uchar		*pName,*pRank;
	char		rank[40];

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);

	pName = AGetResData(gCampName);
	pCH = (CelHeader*)pName;
	strcpy(tmpStr,pCampArr[campCnt].curName);
	ASetString(0,0,tmpStr, pName, pCH->width, NULL, DG_JUST_CENTER);
	campNameGuy.SetState(S_CHANGED, TRUE);

	pRank = AGetResData(gCampRank);
	pCH = (CelHeader*)pRank;
	GetAlignment(&sqbRes,&sqbNum);
	pTxt = sqbHall.Load((uint)sqbRes,sqbNum);
	strcpy(rank,pTxt);
	strcat(rank," ");

	GetRank(&sqbRes,&sqbNum,pCampArr[campCnt].campaignScore);
	pTxt = sqbHall.Load((uint)sqbRes,sqbNum);
	strcat(rank,pTxt);

	ASetString(0,0,rank, pRank, pCH->width, NULL, DG_JUST_CENTER);
	campRankGuy.SetState(S_CHANGED, TRUE);
}


void
NetHall::LoadCampaignScores()
{
	uchar			*pCampScore;

	//===================================================
	//setup campScoreGuy
	pFontMgr->SetRes(9058);
	SetFontColors(CI_SKIP,67,65,62,CI_BLACK,CI_BLACK,CI_BLACK); //Green

	sprintf(tmpStr,"%d",pCampArr[campCnt].campaignScore);
	pCampScore = AGetResData(gCampScore);
	ASetString(0,0, tmpStr, pCampScore, 50, NULL);

	campScoreGuy.SetState(S_CHANGED, TRUE);
}


void
NetHall::LoadWinLoseStats()
{

	uchar	*pWinLoseStat;
	int	i;

	pWinLoseStat = AGetResData(gWinLoseStat);
	pCH = (CelHeader*)pWinLoseStat;
	
	pCamp = &pCampArr[campCnt];
	
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,44,46,44,46,44,46); //CARVED GREEN
	
	//"TOTAL WINS"
	pTxt = sqbHall.Load(HALL_SQB,11);
	ASetString( 0, 0,pTxt, pWinLoseStat, 120, NULL);
	
	//"TOTAL LOSSES"
	pTxt = sqbHall.Load(HALL_SQB,12);
	ASetString(147, 0,pTxt, pWinLoseStat, 120, NULL);
	
	//"EASY ENEMIES"
	pTxt = sqbHall.Load(HALL_SQB,14);
	ASetString( 0, 14,pTxt, pWinLoseStat, 100, NULL);
	
	//"TOUGH ENEMIES"
	pTxt = sqbHall.Load(HALL_SQB,13);
	ASetString(147, 14,pTxt, pWinLoseStat, 100, NULL);
	
	//"WINS"
	pTxt = sqbHall.Load(HALL_SQB,16);
	ASetString(101, 14,pTxt, pWinLoseStat, 40, NULL);
	
	//"LOSSES"
	pTxt = sqbHall.Load(HALL_SQB,17);
	ASetString(243, 14,pTxt, pWinLoseStat, 40, NULL);
	
	//SetFontColors(CI_SKIP,44,46,44,46,44,46); //CARVED GREEN
	//SetFontColors(CI_SKIP,78,78,78,101); //BLUE ?
	SetFontColors(CI_SKIP,93,90,93,90,93,90);

	//total wins
	sprintf(tmpStr,"%d",pCamp->missionsWon);
	ASetString(78,0, tmpStr, pWinLoseStat, 40, NULL);
	
	//total losses
	sprintf(tmpStr,"%d",pCamp->missionsLost);
	ASetString(233,0, tmpStr, pWinLoseStat, 40, NULL);

	//write opponents beaten
	for(i=0;i<8;i++)
	{
		if(i<pCamp->oppBeatenNum)
		{
			ASetString(xPosNames[0],yPos[i], pCamp->win[i].oppName, pWinLoseStat, pCH->width, NULL);
			sprintf(tmpStr,"%d",pCamp->win[i].oppNumWin);
			ASetString(xPosScores[0],yPos[i], tmpStr, pWinLoseStat, 15, NULL, DG_JUST_RIGHT);
		}
	}
	
	//write opponents lost to
	for(i=0;i<8;i++)
	{
		if(i<pCamp->oppLostToNum)
		{
			ASetString(xPosNames[1],yPos[i], pCamp->lose[i].oppName, pWinLoseStat, pCH->width, NULL);
			sprintf(tmpStr,"%d",pCamp->lose[i].oppNumWin);
			ASetString(xPosScores[1],yPos[i], tmpStr, pWinLoseStat, 15, NULL, DG_JUST_RIGHT);
		}
	}
	
	winLoseStatGuy.SetState(S_CHANGED, TRUE);
}


//*************************************************************
//Section: HandleMsg and related functions
//*************************************************************

bool
NetHall::HandleMsg(Message* pMsg)
{
	char		mess[100];
	int		dir;

	switch (pMsg->type)
	{
		case MSG_NOTICE:
			// here we are only interested in this type of notice.
			if (pMsg->notice.type == N_CONTROL_REPLY)
			{
				// a reply from one of our buttons.  Determine exact meaning of msg.
				if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
				{
					//if(pMsg->notice.gSource == diskButton.gSelf)
					//{
					//	pMono->Out("\nDiskButton was hit");
					//	if(!buttonsLocked)
					//		Option(pal.gSelf);
					//	return(TRUE);
					//}
					//else
					if(pMsg->notice.gSource == delButton.gSelf)
					{
						pMono->Out("\ndelButton was hit");
						if(!buttonsLocked)
						{
							deleteCnt++;
							pCampArr = ADerefAs(NetCampaign,gCampArr);
							pCampArr[campCnt].deleted = TRUE;

							//lets find a valid entry
							if(leftIdx != -1)
								dir = -1;
							else if(rightIdx != -1)
								dir = 1;
							else
								//no valid entries left
								dir = 0;

							if(!dir)
							{
								WriteCamp(); //rewrite w/o deleted records

								if(bGlobal.roomMgr.prevRoomNum == BR_MENU ||
									bGlobal.netDisconnect)
								{
									//return to the room in the mode we left
									bGlobal.roomMgr.NewRoom(BR_MENU);
								}
								else
								if(bGlobal.roomMgr.prevRoomNum == BR_ASSESS)
								{
									bGlobal.roomMgr.NewRoom(BR_NET_CHAR);
								}
								else 
								{
									APanic("NetHall: Bad previous room!");
								}
							}
							else
							{
								buttonsLocked = TRUE;
								ClearAll();
								scrollingDone = FALSE;
								scrollDir = dir;
								scrollStepCnt = 0;
								LoadHall(dir);
							}
						}
						return(TRUE);
					}
					else if(pMsg->notice.gSource == exitButton.gSelf)
					{
						pMono->Out("\nexitButton was hit");
						if(!buttonsLocked)
						{
							pCampArr = ADerefAs(NetCampaign,gCampArr);
							if(deleteCnt) 
								WriteCamp(); //rewrite w/o deleted records

							if(bGlobal.roomMgr.prevRoomNum == BR_MENU ||
								bGlobal.netDisconnect)
							{
								//return to the room in the mode we left
								bGlobal.roomMgr.NewRoom(BR_MENU);
							}
							else 
							if(bGlobal.roomMgr.prevRoomNum == BR_ASSESS)
							{
								bGlobal.roomMgr.NewRoom(BR_NET_CHAR);
							}
							else
							{
								APanic("NetHall: Bad previous room!");
							}
						}
						return(TRUE);
					}
					else if(pMsg->notice.gSource == leftButton.gSelf)
					{
						pMono->Out("\nleftButton was hit");
						if(!buttonsLocked)
						{
							pCampArr = ADerefAs(NetCampaign,gCampArr);
							//left button will hidden if no valid entries in that dir
							buttonsLocked = TRUE;
							ClearAll();
							scrollingDone = FALSE;
							//prevScrollDir = scrollDir;
							scrollDir = -1;		//to go left = to scroll left
							scrollStepCnt = 0;
							//if we've already been there, then no need to re-setup
							//if(prevScrollDir != 1) //prev campaign could be deleted
								LoadHall(-1); //left == -1
						}
						return(TRUE);
					}
					else if(pMsg->notice.gSource == rightButton.gSelf)
					{
						pMono->Out("\nrightButton was hit");
						if(!buttonsLocked)
						{
							pCampArr = ADerefAs(NetCampaign,gCampArr);
							//right button will hidden if no valid entries in that dir
							buttonsLocked = TRUE;
							ClearAll();
							scrollingDone = FALSE;
							//prevScrollDir = scrollDir;
							scrollDir = 1;
							scrollStepCnt = 0;
							//if we've already been there, then no need to re-setup
							//if(prevScrollDir != -1) //prev campaign could be deleted
								LoadHall(1);  //right == 1
						}
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

					case K_ESC:
					case K_E:
						FakeMouseClick(exitButton.gSelf);
						return TRUE;

					case K_LEFT:
						FakeMouseClick(leftButton.gSelf);
						return TRUE;
					case K_RIGHT:
						FakeMouseClick(rightButton.gSelf);
						return TRUE;

				}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


void
NetHall::ClearAll()
{
	uchar		*pName,*pRank;
	uchar		*pNameData,*pRankData;
	uchar		*pCampScore,*pCampScoreData;
	uchar		*pWinLoseStat,*pWinLoseStatData;

	//clear name
	pName = AGetResData(gCampName);
	pCH = (CelHeader*)pName;
	pNameData = pName + sizeof(CelHeader); //we know its a single cel
	memset(pNameData,CI_SKIP,L2R(pCH->width) * pCH->height); //clear cel
	campNameGuy.SetState(S_CHANGED,TRUE);

	//clear rank
	pRank = AGetResData(gCampRank);
	pCH = (CelHeader*)pRank;
	pRankData = pRank + sizeof(CelHeader); //we know its a single cel
	memset(pRankData,CI_SKIP,L2R(pCH->width) * pCH->height); //clear cel
	campRankGuy.SetState(S_CHANGED,TRUE);

	//clear CampScore
	pCampScore = AGetResData(gCampScore);
	pCH = (CelHeader*)pCampScore;
	pCampScoreData = pCampScore + sizeof(CelHeader); //we know its a single cel
	memset(pCampScoreData,CI_SKIP,L2R(pCH->width) * pCH->height); //clear cel
	campScoreGuy.SetState(S_CHANGED,TRUE);

	//clear WinLoseStat
	pWinLoseStat = AGetResData(gWinLoseStat);
	pCH = (CelHeader*)pWinLoseStat;
	pWinLoseStatData = pWinLoseStat + sizeof(CelHeader); //we know its 1 cel
	memset(pWinLoseStatData,CI_SKIP,L2R(pCH->width) * pCH->height); //clear cel
	winLoseStatGuy.SetState(S_CHANGED,TRUE);
}


//*************************************************************
//Section: Cycle and related functions
//*************************************************************

void
NetHall::Cycle()
{
	if(immedBail)
	{
		//return to the room in the mode we left
		bGlobal.roomMgr.NewRoom(BR_MENU);
	}

	//this first so campaign is displayed on the cycle after scrolling done
	if(displayCampaign)
	{
	  	displayCampaign = FALSE;
		DisplayCampaign();
		ResetHalls();
	}

	if(!scrollingDone)
	{
		//incr scroll display
		if(scrollDir == 1) //right
		{
			centerHallGuy.OffsetPos(-SCROLL_STEP_SIZE,0);
			rightHallGuy.OffsetPos(-SCROLL_STEP_SIZE,0);
		}
		else //left
		{
			centerHallGuy.OffsetPos(SCROLL_STEP_SIZE,0);
			leftHallGuy.OffsetPos(SCROLL_STEP_SIZE,0);
		}

		scrollStepCnt++;

		//if done
		if(scrollStepCnt == SCROLL_STEPS)
		{
			scrollingDone = TRUE;
			buttonsLocked = FALSE;
			displayCampaign = TRUE;
		}
	}

	BAM_Room::Cycle();
}


void
NetHall::ResetHalls()
{
	BAM_Guy	*pGuy;
	grip		gTemp;
	uint		rNumTemp;

	//we have just finished shifting the left or right panel to the center
	//and the center to the left or right
	//so reassign panels to proper guys. So left is left, etc.

	if(scrollDir == -1) //scrolled left
	{
		//mode is now fixed at showing two columns
		//centerColumnMode--;
		//if(!centerColumnMode)
		//	centerColumnMode = 3;

		//left cel moved to center and center cel to right, so:
		gTemp = gRightHall;
		rNumTemp = rNumRightHall;
		gRightHall = gCenterHall;
		rNumRightHall = rNumCenterHall;
		gCenterHall = gLeftHall;
		rNumCenterHall = rNumLeftHall;
		gLeftHall = gTemp;
		rNumLeftHall = rNumTemp;

		//now the named grip vars equal the grip to the cel that's at the
		//named location

		//lets reset the positions of the guys that moved and set their new cels
		pGuy = &rightHallGuy;
		pGuy->SetRes(RES_CEL,rNumRightHall);

		pGuy = &centerHallGuy;
		pGuy->SetRes(RES_CEL,rNumCenterHall);
		pGuy->SetPos(0,94);
		
		pGuy = &leftHallGuy;
		pGuy->SetRes(RES_CEL,rNumLeftHall);
		pGuy->SetPos(-320,94);

	}
	else //scrolled right
	{
		//mode is now fixed at showing two columns
		//centerColumnMode++;
		//if(centerColumnMode > 3)
		//	centerColumnMode = 1;

		//right moved to center and center moved to left
		gTemp = gLeftHall;
		rNumTemp = rNumLeftHall;
		gLeftHall = gCenterHall;
		rNumLeftHall = rNumCenterHall;
		gCenterHall = gRightHall;
		rNumCenterHall = rNumRightHall;
		gRightHall = gTemp;
		rNumRightHall = rNumTemp;

		//lets reset the positions of the guys that moved and set their new cels
		pGuy = &leftHallGuy;
		pGuy->SetRes(RES_CEL,rNumLeftHall);

		pGuy = &centerHallGuy;
		pGuy->SetRes(RES_CEL,rNumCenterHall);
		pGuy->SetPos(0,94);
		
		pGuy = &rightHallGuy;
		pGuy->SetRes(RES_CEL,rNumRightHall);
		pGuy->SetPos(320,94);
	}
	//left or right we have to set centerColorMode to this range
	centerColorMode = (centerColorMode)?0:1;
}
