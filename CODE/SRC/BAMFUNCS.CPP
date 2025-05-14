// BAMFUNCS.CPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
// Global functions for BAM
//
//	09-07-94
//

#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "fontmgr.hpp"
#include "mono.hpp"
#include "mouse.hpp"
#include "resource.hpp"
#include "text.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"
#include "alldefs.hpp"
#include "makechar.hpp"

#include <string.h>

res_t	fred;
uint32 foo;

// copy from X1/Y1 to X2/Y2 of src buffer, into dest at DestX/Y
void
CopyBitMap(CelHeader *pDest, int32 destX, int32 destY, uchar *pSrcBuf,
	int srcWidth, int srcHeight, bool fHasSkip, grip gNewClut)
{
	CelHeader		*pDestCel;
	uchar				*pDestData;
	RCopyPixelsEx	rcp;
	uint32			destX2, destY2;
//	grip				gSrcRes;

	pDestCel = pDest;
	pDestData = (uchar *)pDest + sizeof(CelHeader);
	
	// some boundary checking
	destX2 = destX + srcWidth - 1;
	if(destX2 >= pDest->width)
	{
		destX2 = pDest->width - 1;
	}
	destY2 = destY + srcHeight - 1;
	if(destY2 >= pDest->height)
	{
		destY2 = pDest->height - 1;
	}

	Rectangle	rFillArea(destX, destY, destX2, destY2);

	// fill copyPixels structure
	rcp._vpRectFillArea = &rFillArea;
	rcp._vbufWidth = pDest->width;
	rcp._celX = 0;
	rcp._celY = 0;
	rcp._celWidth = srcWidth;
	rcp._pCelData = pSrcBuf;	
	rcp._pBufData = pDestData;	
	rcp._pDst = rcp._pBufData + ((rFillArea.y1 * pDest->width) + rFillArea.x1);
	rcp._scale = 256;
	rcp._pScrim = NULL;

	// determine copy pixels transfer mode.
	rcp._flags = 0;
	if (!fHasSkip)
		rcp._flags |= CP_NOSKIP;

	// pass CLUT info
	rcp._flags |= CP_EXTENDED;
	if (gNewClut)
	{
		rcp._clut = ADerefAs(uchar ,gNewClut);
	}
	else
	{
		rcp._clut = NULL;
	}
	rcp._clutIdx = CLUT_TEAM_COLORS;
	rcp._clutSize = CLUT_SIZE;

	// fill buffer with platform optimized routine
	ACopyPixels(&rcp);
}

// Return the pixel of a scrimage at a given global coordinate.
uchar
GetPixel(int celXOff, int celYOff, res_t srcType, 
 			uint srcNum, uint srcCel)
{
	uchar			*pTCelData;
	Rectangle	celRect;
	CelHeader	*pTCelH; //temp. cel
	uchar			TCel[chSize+10];

	//lets not create a dest. cel and say we did -fake out!
	celRect.Set(celXOff,celYOff,celXOff+1,celYOff+1);
	pTCelH = (CelHeader*)TCel;
	pTCelH->width = 2;
	pTCelH->height = 2;

	CopyCel(pTCelH, 0, 0, srcType, srcNum, srcCel, &celRect, FALSE);

	pTCelData = (uchar*)pTCelH + sizeof(CelHeader);
	
	//return first pixel of data
	return 		*pTCelData;
}

void
LoadUnitColors(grip gcurPal, unitColor_t colorSide1, unitColor_t colorSide2)
{
	TPalette *pPal;
	uint		loadIndex1;
	uint		loadIndex2;

	if(gcurPal)
		//deref current pal
		pPal = ADerefAs(TPalette,gcurPal);
	else
		APanic("Invalid palette grip");

	//unit color is in ranges of 8 color starting at index 192

	if(colorSide1)
	{
		loadIndex1 = CLUT1_START + (CLUT_SIZE * ((uint)colorSide1-1));
		//partial load into first slot
		pPal->LoadPartial(9101,loadIndex1,CLUT_SIZE,CLUT1_START);
	}

	if(colorSide1)
	{
		loadIndex2 = CLUT1_START + (CLUT_SIZE * ((uint)colorSide2-1));
		//partial load  into second slot
		pPal->LoadPartial(9101,loadIndex2,CLUT_SIZE,CLUT2_START);
	}
}

void
GetUnitColors(unitColor_t* color1, unitColor_t* color2, int storyLine_P)
{
	if(!storyLine_P)
		storyLine_P = bGlobal.storyLine; //default to global

	unitColor_t unitColorArr[5][2] = {
											BLUE,RED, //storyline 1
											BLUE,RED,
											BLUE,RED,
											BLUE,RED,
											BLUE,RED};
	
	*color1 = unitColorArr[storyLine_P-1][0];
	*color2 = unitColorArr[storyLine_P-1][1];
}


int
GetCine(int storyLine, int missionsDone, int chooseSide)
{
	int cineArr[3][5][4] = { //[chooseSide][storyLine][missionsDone]
								//chooseSide == 0
								3502,0,0,0, //STORY1 OPENER
								3575,0,0,0, //STORY2 OPENER
								3635,0,0,0, //STORY3 OPENER
								3687,0,0,0, //STORY4 OPENER
								3745,0,0,0, //STORY5 OPENER
								//chooseSide == 1
								//setup1,setup2,setup3,winning
								3520,3525,3530,3540, //STORYLINE1
								3590,3594,3598,3605, //STORYLINE2
								3645,3650,3655,3662, //STORYLINE3
								3696,3700,3705,3712, //STORYLINE4
								3755,3760,3765,3775, //STORYLINE5
								//chooseSide == 2
								3545,3550,3555,3564, //STORYLINE1
								3610,3614,3618,3625, //STORYLINE2
								3666,3670,3674,3680, //STORYLINE3
								3716,3720,3725,3735, //STORYLINE4
								3780,3785,3790,3800, //STORYLINE5
								};

	if(storyLine == STORY_NONE || storyLine == LEGEND)
		return 0;

	return cineArr[chooseSide][storyLine-1][missionsDone];
}


char *
GetScenario(char* sceneOut, int storyLine, int missionsDone, int chooseSide)
{
	int scene=0;

	int scenarioArr[6][3] = { //[storyLine],[missionsDone]	
								//chooseSide == 1
								9110,9130,9150, //STORYLINE1
								9170,9190,9210, //STORYLINE2
								9230,9250,9270, //STORYLINE3
								9290,9310,9330, //STORYLINE4
								9350,9370,9390, //STORYLINE5
								9410,0   ,0   };

	if(storyLine == LEGEND)
	{
		int path[16],*pArr;

		if(missionsDone == 16)
			scene = 0;
		else
		{
			GetLegendPath(path,bGlobal.curPath);
			pArr = (int*)scenarioArr[0];
		
			scene = *(pArr + path[missionsDone]);

			if(scene != 9410)
				scene += 2;
		}
	}
	else
	{
		//if finished a storyline there is no next scenario
		if(missionsDone < 3)
		{
			if(chooseSide == 2)
				scene = scenarioArr[storyLine-1][2-missionsDone];
			else
				scene = scenarioArr[storyLine-1][missionsDone];

			if(chooseSide == 2)
				scene += 5;
		}
	}


	sprintf(sceneOut,"%d",scene);
		
	return sceneOut;
}


void
GetStoryName(int* sqbRes, int* sqbNum, int storyOrMap)
{
	*sqbRes = STORYLINE_SQB;
	*sqbNum = 9;	//nuttin honey

	if(storyOrMap < 10)
	{
		switch(storyOrMap)
		{
			case STORY_NONE:
				*sqbNum = 9;
				break;
			case STORYLINE1:
				*sqbNum = 1;
				break;
			case STORYLINE2:
				*sqbNum = 2;
				break;
			case STORYLINE3:
				*sqbNum = 3;
				break;
			case STORYLINE4:
				*sqbNum = 4;
				break;
			case STORYLINE5:
				*sqbNum = 5;
				break;
			case LEGEND:
				*sqbNum = 6;
				break;
			case TUTORIAL:
				*sqbNum = 8;
				break;
			case SHOW_OFF:
			case NETGAME:
			default:
				*sqbNum = 9;
				break;
		}
	}
	else
	if(storyOrMap % 5)
	{
		//we got a remainder -a legendary or net map (e.g. 9112)
		if(bGlobal.storyLine == LEGEND)
			*sqbNum = 6;
		else
			*sqbNum = 9;
	}
	else
	{
		if(storyOrMap > 9109 && storyOrMap <9156)
		{
			*sqbNum = 1;
		}
		else
		if(storyOrMap > 9169 && storyOrMap <9216)
		{
			*sqbNum = 2;
		}
		else
		if(storyOrMap > 9229 && storyOrMap <9276)
		{
			*sqbNum = 3;
		}
		else
		if(storyOrMap > 9289 && storyOrMap <9336)
		{
			*sqbNum = 4;
		}
		else
		if(storyOrMap > 9339)
		{
			*sqbNum = 5;
		}
	}
}


void
GetCharName(int* sqbRes, int* sqbNum, int storyLine_P, int chooseSide_P, bool specific)
{
	*sqbRes = STORY_CHAR_SQB;

	//NOTE: if we enter a legendary map from the command line
	//we overrun into the specific names for storyline 4 -who cares?

	if(!storyLine_P)
		storyLine_P = bGlobal.storyLine; //default to global

	//if still zero then error out
	if(!storyLine_P)
		APanic("StoryLine not set");

	if(!chooseSide_P)
		chooseSide_P = bGlobal.chooseSide; //default to global val.

	if(storyLine_P == 4 && chooseSide_P == 1)
		if(specific)
			*sqbNum = 11 + bGlobal.missionsDone;
		else
			*sqbNum = ((storyLine_P-1)*2) + chooseSide_P;
	else
		*sqbNum = ((storyLine_P-1)*2) + chooseSide_P;
}


void
GetAbbrCharName(int* sqbRes, int* sqbNum, int storyLine_P, int chooseSide_P, bool specific)
{
	*sqbRes = ABBR_NAME_SQB;

	if(storyLine_P == NETGAME)
		*sqbNum = 14;

	if(!storyLine_P)
		storyLine_P = bGlobal.storyLine; //default to global

	//if still zero then error out
	if(!storyLine_P)
		APanic("StoryLine not set");

	if(!chooseSide_P)
		chooseSide_P = bGlobal.chooseSide; //default to global val.

	if(storyLine_P == 4 && chooseSide_P == 1)
		if(specific)
			*sqbNum = 11 + bGlobal.missionsDone;
		else
			*sqbNum = ((storyLine_P-1)*2) + chooseSide_P;
	else
		*sqbNum = ((storyLine_P-1)*2) + chooseSide_P;
}


int
GetStoryLine(char* mapName)
{

	int mapNum = atoi(mapName);
	char	mapStr[5];

	sprintf(mapStr,"%d",mapNum);

	if(mapName[strlen(mapStr)-1] == '2' || !strcmp(mapStr,"9410"))
		return LEGEND;


	if(mapNum > 9109 && mapNum <9156)
	{
			return STORYLINE1;
	}
	else
	if(mapNum > 9169 && mapNum <9216)
	{
			return STORYLINE2;
	}
	else
	if(mapNum > 9229 && mapNum <9276)
	{
			return STORYLINE3;
	}
	else
	if(mapNum > 9289 && mapNum <9336)
	{
			return STORYLINE4;
	}
	else
	if(mapNum > 9339)
	{
			return STORYLINE5;
	}

	return 0; //not used, but the stupid compiler insists
}


void
SetDefaults(char* map)
{
	SquibRes	sqbDefault;
	int		sqbRes;
	int		sqbNum;
	char		*pTxt;

	if(bGlobal.storyLine != NETGAME)
		bGlobal.storyLine = GetStoryLine(map);

	if(bGlobal.storyLine == LEGEND)
	{
		bGlobal.chooseSide = 0;

		if(atoi(map) == 9410)
		{
			bGlobal.missionsDone = 15;

			//lets fake the rest
			bGlobal.legendStart = 0;
			bGlobal.curPath = 0;
		}
		else
		{
			bGlobal.missionsDone = 0;
			bGlobal.legendStart = (atoi(map) - 9112) / 20;
			bGlobal.curPath = bGlobal.legendStart*VARPATHS + ARandom2(VARPATHS);
		}
		bGlobal.curCharId = 100;
		bGlobal.curBodyAnim = 3000;
		bGlobal.curFaceAnim = 2912;
		bGlobal.curCoverAnim = 2868;
		bGlobal.curBanner = 2; //'BLUE'
		bGlobal.curCat = 1;
		pTxt = sqbDefault.Load(MEN_SQB,9); //'DIRK RAMMER'
		strcpy(bGlobal.curName,pTxt);

		//legendOpponent will generate a random enemy and set globals

		if(pBam->playerSide == SIDE1)
			pBam->sideColors[SIDE1] = (unitColor_t) ((bGlobal.curBanner/2) + 1);
		else
			pBam->sideColors[SIDE2] = (unitColor_t) ((bGlobal.curBanner/2) + 1);

	}
	else
	if(bGlobal.storyLine == NETGAME)
	{
		bGlobal.chooseSide = 0;

		if(pCommMgr->GetUserID())
		{
			bGlobal.curCharId = 101;
			bGlobal.curBodyAnim = 3000;
			bGlobal.curFaceAnim = 2912;
			bGlobal.curCoverAnim = 2868;
			bGlobal.curBanner = 2;	//'BLUE'
			bGlobal.curCat = 1;		//'MAN'
			pTxt = sqbDefault.Load(MEN_SQB,9); //'DIRK RAMMER'
			strcpy(bGlobal.curName,pTxt);

			bGlobal.enemyCharId = 100;
			bGlobal.enemyBodyAnim = 3016;
			bGlobal.enemyFaceAnim = 2942;
			bGlobal.enemyCoverAnim = 2862;
			bGlobal.enemyBanner = 0;	//'RED'
			bGlobal.enemyCat = 2;		//'WOMAN'
			pTxt = sqbDefault.Load(WOMEN_SQB,2); //'AMBROSIA'
			strcpy(bGlobal.enemyName,pTxt);
		}
		else
		{
			bGlobal.enemyCharId = 101;
			bGlobal.enemyBodyAnim = 3000;
			bGlobal.enemyFaceAnim = 2912;
			bGlobal.enemyCoverAnim = 2868;
			bGlobal.enemyBanner = 2;	//'BLUE'
			bGlobal.enemyCat = 1;		//'MAN'
			pTxt = sqbDefault.Load(MEN_SQB,9); //'DIRK RAMMER'
			strcpy(bGlobal.enemyName,pTxt);

			bGlobal.curCharId = 100;
			bGlobal.curBodyAnim = 3016;
			bGlobal.curFaceAnim = 2942;
			bGlobal.curCoverAnim = 2862;
			bGlobal.curBanner = 0;	//'RED'
			bGlobal.curCat = 2;		//'WOMAN'
			pTxt = sqbDefault.Load(WOMEN_SQB,2); //'AMBROSIA'
			strcpy(bGlobal.curName,pTxt);
		}

		if(pBam->playerSide == SIDE1)
			pBam->sideColors[SIDE1] = (unitColor_t) ((bGlobal.curBanner/2) + 1);
		else
			pBam->sideColors[SIDE2] = (unitColor_t) ((bGlobal.curBanner/2) + 1);

	}
	else
	{
		if(map[strlen(map)-1] == '5')
			bGlobal.chooseSide = 2;
		else
			bGlobal.chooseSide = 1;

		GetCharName(&sqbRes,&sqbNum);
		pTxt = sqbDefault.Load(sqbRes,sqbNum);
		strcpy(bGlobal.curName,pTxt);

		if(bGlobal.chooseSide == 1)
			GetUnitColors(&pBam->sideColors[SIDE1],&pBam->sideColors[SIDE2],bGlobal.storyLine);
		else
			GetUnitColors(&pBam->sideColors[SIDE2],&pBam->sideColors[SIDE1],bGlobal.storyLine);

		bGlobal.missionsDone = GetMissionsDone(map);
	}

}


int
GetPortraitAnim(int chooseSide_P,bool specific)
{
	uint16 portraitArr[5][2] = {
					//chooseSide==1,chooseSide==2
					2800,2802, 	//storyLine1
					2804,2806, 	//storyLine2
					2813,2814, 	//storyLine3
					2816,2818, 	//storyLine4
					2820,2822,	//storyLine5
					};

	uint16 portraitSpecificArr[3] = { 
					2808,2810,2812}; //storyLine3 specific portraits
	
	if(bGlobal.storyLine == LEGEND)
	{
		pMono->Out("Warning: Legend campaign is not suppose to be calling GetPortraitAnim");
		return 0;
	}

	if(!chooseSide_P)
		chooseSide_P = bGlobal.chooseSide; //default to global val.

	//Set up exceptions for storyLine 3 as it has multiple
	//portraits for the circle of light
	if(bGlobal.storyLine == 3 && chooseSide_P == 1)
		if(specific)
			return (int)portraitSpecificArr[bGlobal.missionsDone];
		else
			return (int)portraitArr[bGlobal.storyLine-1][chooseSide_P-1];
	else
		return (int)portraitArr[bGlobal.storyLine-1][chooseSide_P-1];
}


void
GetLevelName(int* sqbRes,int* sqbNum,int storyLine_P,int missionsDone_P,int chooseSide_P)
{
	int missionsTmp;

	*sqbRes = REGION_SQB;

	if(storyLine_P == NETGAME)
	{
		missionsTmp = (atoi(pBam->scenarioName) - 9110)/20;

		//in this case missionsDone is used as a direct index into the sqb #'s
		//ugly but effective
		*sqbNum = missionsTmp + 1;
	}
	else
	if(storyLine_P == LEGEND)
	{
		//in this case missionsDone is used as a direct index into the sqb #'s
		//ugly but effective
		*sqbNum = missionsDone_P + 1;
	}
	else
	{
		if(chooseSide_P == 1)
			missionsTmp = missionsDone_P;
		else
			missionsTmp = 2-missionsDone_P;

		*sqbNum = ((storyLine_P-1)*3) + missionsTmp + 1;
	}

}


void
GetRank(int* sqbRes, int* sqbNum, int score)
{
	int x=0;
	int totalRanks = 16;

	*sqbRes = RANK_SQB;

	int ranks[16]  = {	 450,
								 900,
								1500,
								2250,
								3150,
								4200,
								5400,
								6750,
								8250,
								9900,
							  11700,
							  13650,
							  15750,
							  18000,
							  25000,
							1000000};


	//lets take a score and find a rank
	for(x=0;x<totalRanks;x++)
	{
		if(score < ranks[x])
		{
			*sqbNum = x+1;
			break;
		}
	}
}


int*
GetLegendPath(int* legendPath, int pathIndex)
{
	int y;

	// the number of possible paths for each starting point is defined by VARPATH
	int tempPath [][16] = {
 			 0, 6, 7, 8, 1,13,14, 3, 5,11,10, 9, 4,12, 2,15,
 			 0, 2, 1, 8, 7, 6,10,11, 5, 3,14,13,12, 4, 9,15,
 			 0, 1, 2,12,13,14, 3, 5, 4, 9,11,10, 6, 7, 8,15,
 			 0, 9, 6,10,11, 4, 5, 3,14,12,13, 2, 1, 8, 7,15,

 			 1,13,14, 3, 5,11,10, 6, 7, 8, 0, 2,12, 9, 4,15,
 			 1, 2,12, 9, 0, 8, 7, 6,10,11, 4, 5, 3,14,13,15,
 			 1, 0, 8, 7, 6,10, 9,12, 2,13,14, 4,11, 5, 3,15,
 			 1, 8, 0, 2,13,14, 3, 5,11, 4,12, 9,10, 6, 7,15,

 			 2,13,14, 3, 5, 4,12, 9, 0, 1, 8, 7, 6,10,11,15,
 			 2, 1, 8, 7, 6, 0, 9,10,11, 4,12,13,14, 3, 5,15,
 			 2, 0, 9,12,13, 1, 8, 7, 6,10,11, 5, 3,14, 4,15,
 			 2,12, 0, 1, 8, 7, 6, 9,10,11, 4, 5, 3,14,13,15,

 			 3,14, 5,11, 4,12,13, 1, 2, 0, 9,10, 6, 7, 8,15,
 			 3,14,13, 1, 8, 7, 6,10,11, 5, 4,12, 2, 0, 9,15,
 			 3, 5,11, 4,14,13,12, 9,10, 6, 0, 2, 1, 8, 7,15,
 			 3, 5,11,10, 6, 7, 8, 0, 9, 4,14,13, 1, 2,12,15,

 			 4, 5, 3,14,13, 1, 2,12, 0, 8, 7, 6, 9,10,11,15,
 			 4,14, 3, 5,11,10, 6, 7, 8, 0, 9,12,13, 1, 2,15,
 			 4,12, 2, 0, 9,10,11, 5, 3,14,13, 1, 8, 7, 6,15,
 			 4, 9, 6,10,11, 5, 3,14,12, 2,13, 1, 8, 7, 0,15,

 			 5, 3,14, 4,11,10, 9,12,13, 1, 2, 0, 6, 7, 8,15,
 			 5, 4,11,10, 6, 7, 8, 0, 9,12, 2, 1,13,14, 3,15,
 			 5, 3,14,13, 1, 2,12, 4,11,10, 9, 6, 7, 8, 0,15,
 			 5,11,10, 9, 4,12, 2, 0, 6, 7, 8, 1,13,14, 3,15,

 			 6, 7, 8, 1, 0, 9,10,11, 5, 4,12, 2,13,14, 3,15,
 			 6, 0, 9,10,11, 5, 3,14, 4,12,13, 2, 1, 8, 7,15,
 			 6, 9, 0, 7, 8, 1, 2,13,12,14, 3, 5, 4,11,10,15,
 			 6,10,11, 5, 3,14, 4, 9, 0, 7, 8, 1, 2,12,13,15,

 			 7, 8, 1,13, 2, 0, 6,10, 9,12,14, 3, 5, 4,11,15,
 			 7, 0, 8, 1, 2,12,13,14, 4, 9, 6,10,11, 5, 3,15,
 			 7, 0, 6, 9,10,11, 4, 5, 3,14,13,12, 2, 1, 8,15,
 			 7, 6,10,11, 4,12, 9, 0, 8, 1, 2,13,14, 3, 5,15,

 			 8, 7, 0, 6,10,11, 4, 9,12, 2, 1,13,14, 3, 5,15,
 			 8, 0, 2, 1,13,12,14, 3, 5, 4,11,10, 9, 6, 7,15,
 			 8, 0, 7, 6, 9,12, 2, 1,13,14, 3, 5, 4,11,10,15,
 			 8, 1,13,12, 2, 0, 7, 6,10, 9, 4,11, 5, 3,14,15,

 			 9,10,11, 5, 3,14, 4,12, 0, 6, 7, 8, 1,13, 2,15,
 			 9, 0, 7, 8, 1, 2,13,12,14, 3, 5, 4,11,10, 6,15,
 			 9,12, 4, 5, 3,14,13, 1, 2, 0, 8, 7, 6,10,11,15,
 			 9, 4,12, 2, 0, 8, 7, 6,10,11, 5, 3,14,13, 1,15,

			10, 6, 7, 8, 1,13,12, 2, 0, 9, 4,11, 5, 3,14,15,
			10, 9, 0, 2,12, 4,11, 5, 3,14,13, 1, 8, 7, 6,15,
			10, 9, 4,11, 5, 3,14,12, 0, 6, 7, 8, 1,13, 2,15,
			10,11, 4, 9, 6, 0, 7, 8, 1, 2,13,12,14, 3, 5,15,

			11, 5, 3,14, 4, 9,10, 6, 7, 8, 0,12,13, 2, 1,15,
			11, 4, 5, 3,14,12, 2, 0, 9,10, 6, 7, 8, 1,13,15,
			11, 4, 9,10, 6, 0, 7, 8, 1,13, 2,12,14, 3, 5,15,
			11,10, 6, 7, 8, 0,12, 2, 1,13,14, 3, 5, 4, 9,15,

			12,13, 1, 2, 0, 8, 7, 6,10,11, 5, 3,14, 4, 9,15,
			12, 9,10,11, 4, 5, 3,14,13, 2, 1, 8, 7, 6, 0,15,
			12,14, 3, 5, 4,11,10, 9, 6, 0, 2,13, 1, 8, 7,15,
			12, 4,11, 5, 3,14,13, 1, 2, 0, 8, 7, 6, 9,10,15,

			13, 2, 0, 1, 8, 7, 6,10,11, 5, 3,14, 4, 9,12,15,
			13, 1, 8, 7, 0, 2,12, 9, 6,10,11, 4, 5, 3,14,15,
			13,12, 2, 1, 8, 7, 6, 0, 9,10,11, 5, 3,14, 4,15,
			13,14, 3, 5, 4,12, 2, 1, 0, 8, 7, 6, 9,10,11,15,

			14, 3, 5, 4,11,10, 9, 0, 6, 7, 8, 1,13, 2,12,15,
			14, 3, 5,11,10, 9, 4,12,13, 2, 0, 6, 7, 8, 1,15,
			14, 4, 9,12,13, 1, 2, 0, 8, 7, 6,10,11, 5, 3,15,
			14,13, 1, 8, 7, 0, 2,12, 9, 6,10,11, 4, 5, 3,15 };


	for(y=0;y<16;y++)
	 	legendPath[y] = tempPath[pathIndex][y];

	return legendPath;
}


int
GetCharId()
{
	FILE *idFile;
	int	i=1;

	if(bGlobal.writeOut)
	{
		idFile = fOpenPrefs("charId.dat","rb+");
		if(!idFile)
		{
			//lets create it
			idFile = fOpenPrefs("charId.dat","wb+");
			if(!idFile)
				APanic("Hall: Error, couldn't open charId.dat");
		}
		else
		{
			fread(&i,1,sizeof(int),idFile);
			i++;
			rewind(idFile);
		}

		fwrite(&i,sizeof(int),1,idFile);
		fclose(idFile);
	}

	return i;
}


void
AlignLawfulChaotic(int	pct)
{
	int	i;

	if(pct < 0 || pct > 100)
		APanic("Align func. error");

	int range[11][3] = {
							89,	2,	2,
							78,	2,	1,
							67,	2,	1,
							61,	1,	2,
							54,	1,	3,
							47,	1,	2,
							40,	1,	1,
							34,	1,	1,
							23,	0,	2,
							12,	0,	3,
							 0,	0,	4
							 };
	for(i=0; i<11; i++)
	{
	 	if(pct >= range[i][0])
			break;
	}

	switch (range[i][1])
	{
	 	case 2:
			bGlobal.lawful += range[i][2];
			break;
		case 1:
			bGlobal.neutral += range[i][2];
			break;
		case 0:
			bGlobal.chaotic += range[i][2];
			break;

	}
}


void
AlignGoodEvil(int	pct)
{
	int	i;

	if(pct < 0 || pct > 100)
		APanic("Align func. error");

	int range[11][3] = {
							89,	2,	2,
							78,	2,	1,
							67,	2,	1,
							61,	1,	2,
							54,	1,	3,
							47,	1,	2,
							40,	1,	1,
							34,	1,	1,
							23,	0,	2,
							12,	0,	3,
							 0,	0,	4
							 };
	for(i=0; i<11; i++)
	{
	 	if(pct >= range[i][0])
			break;
	}

	switch (range[i][1])
	{
	 	case 2:
			bGlobal.evil += range[i][2];
			break;
		case 1:
			bGlobal.nutral += range[i][2];
			break;
		case 0:
			bGlobal.good += range[i][2];
			break;

	}
}


void
GetAlignment(int *sqbRes, int *sqbNum)
{
	int	idx1,idx2;

	*sqbRes = ALIGN_SQB;

	if(bGlobal.lawful >= bGlobal.neutral && bGlobal.lawful >= bGlobal.chaotic)
	{
		idx1 = 0;
	}
	else
	if(bGlobal.neutral >= bGlobal.lawful && bGlobal.neutral >= bGlobal.chaotic)
	{
		idx1 = 3;
	}
	else
	{
		idx1 = 6;
	}


	if(bGlobal.good >= bGlobal.nutral && bGlobal.good >= bGlobal.evil)
	{
		idx2 = 1;
	}
	else
	if(bGlobal.nutral >= bGlobal.good && bGlobal.nutral >= bGlobal.evil)
	{
		idx2 = 2;
	}
	else
	{
		idx2 = 3;
	}

	*sqbNum = idx1 + idx2;
}


char *GetFirstToken(char *str)
{
	static char buffer[40];
	char	*str2;

	strcpy(buffer, strtok(str, " \r\n\t"));
	str2 = buffer;
	do
	{
		if(*str2 == '_')
			*str2 = ' ';
	} while(*(str2++));
	return(buffer);
}

char *GetNextToken(void)
{
	static char buffer[40];
	char	*str2;

	str2 = strtok(NULL, " \r\n\t");
	if(!str2)
		return(NULL);

	strcpy(buffer, str2);
	str2 = buffer;
	do
	{
		if(*str2 == '_')
			*str2 = ' ';
	} while(*(str2++));
	return(buffer);
}


void
FakeMouseClick(grip gButton)
{
	int			oldX, oldY, buttonX, buttonY;
	BAM_Button	*pButton;

	pButton = ADerefAs(BAM_Button, gButton);
	buttonX = (pButton->scrim.rect.x1 + pButton->scrim.rect.x2) / 2;
	buttonY = (pButton->scrim.rect.y1 + pButton->scrim.rect.y2) / 2;
	oldX = pMouse->GetX();
	oldY = pMouse->GetY();
	pMouse->SetPos(buttonX, buttonY);
	APostEvent(E_MOUSE_DOWN, LEFT_BTN, FALSE);
	APostEvent(E_MOUSE_UP, LEFT_BTN, FALSE);
	pMouse->SetPos(oldX, oldY);
}

void
ClearGlobals()
{
	bGlobal.storyLine = STORY_NONE;
	bGlobal.legendStart = -1;
	bGlobal.missionsDone = 0;
	bGlobal.curPath = 0;
	bGlobal.chooseSide = 0;
	bGlobal.prevChooseSide = 1;

	memset(bGlobal.fTutorialGoals, NULL, sizeof(bGlobal.fTutorialGoals));

	bGlobal.replayMap = FALSE;

	//makechar.cpp globals
	bGlobal.curCharId = -1;
	bGlobal.curBodyAnim = 0;
	bGlobal.curFaceAnim = 0;
	bGlobal.curCoverAnim = 0;
	bGlobal.curBanner = 0;
	bGlobal.curCat = G_OTHER;	//gender
	*bGlobal.curName = '\0';

	bGlobal.enemyCharId = -1;
	bGlobal.enemyBodyAnim = 0;
	bGlobal.enemyFaceAnim = 0;
	bGlobal.enemyCoverAnim = 0;
	bGlobal.enemyBanner = 0;
	*bGlobal.enemyName = '\0';

	bGlobal.netWinner = FALSE;
	bGlobal.netDisconnect = FALSE;

	memset(bGlobal.curLevelScore,0,sizeof(bGlobal.curLevelScore));
	memset(bGlobal.lastLevelXP,0,sizeof(bGlobal.lastLevelXP));
	bGlobal.curCampaignScore = 0;
	bGlobal.curMaxCampaignScore = 0;

	memset(bGlobal.curXP, 0, sizeof(bGlobal.curXP));
	memset(bGlobal.unitsResearched, 0, sizeof(bGlobal.unitsResearched));
	memset(bGlobal.lastUnitsResearched, 0, sizeof(bGlobal.lastUnitsResearched));
	memset(bGlobal.fTutorialGoals, 0 , sizeof(bGlobal.fTutorialGoals));

	bGlobal.lawful  = bGlobal.lastLawful  = 0;
	bGlobal.neutral = bGlobal.lastNeutral = 0;
	bGlobal.chaotic = bGlobal.lastChaotic = 0;

	bGlobal.evil   = bGlobal.lastEvil   = 0;
	bGlobal.nutral = bGlobal.lastNutral = 0;
	bGlobal.good   = bGlobal.lastGood   = 0;

	bGlobal.roomMgr.curRoom->Pause(FALSE,TRUE);
}


//called by SetDefaults for storylines
int
GetMissionsDone(char *map)
{
	int	mapNum=atoi(map),i,j;
	int	missionsDone=0;
	bool	side2;

	int scenarioArr[5][3] = {
									9110,9130,9150, //STORYLINE1
									9170,9190,9210, //STORYLINE2
									9230,9250,9270, //STORYLINE3
									9290,9310,9330, //STORYLINE4
									9350,9370,9390, //STORYLINE5
									};

	if(map[strlen(map)-1] == '5')
	{
		mapNum -= 5;
		side2 = TRUE;
	}
	else
	{
		side2 = FALSE;
	}

	for(i=0;i<5;i++)
	{
		for(j=0;j<3;j++)
		{
			if(mapNum == scenarioArr[i][j])
			{
				if(side2)
					missionsDone = 2 - j;
				else
					missionsDone = j;

				break;
			}
		}
	}


	return(missionsDone);
}	


bool
IsStoryDone(int storyNum,int storyDone)
{
	int	storyCvt;

	storyCvt = 1<<(storyNum-1);

	if(storyCvt & storyDone)
		return(TRUE);

	return(FALSE);
}


int
GetStoryDone()
{
	FILE *idFile;
	int	i=0;

	if(bGlobal.writeOut)
	{
		idFile = fOpenPrefs("done.dat","rb+");
		if(idFile)
		{
			fread(&i,1,sizeof(int),idFile);
			fclose(idFile);
		}
	}

	return(i);
}


void
WriteStoryDone(int storyDone)
{
	FILE		*idFile;
	int	alreadyDone=0,curDone;

	if(bGlobal.writeOut)
	{
		curDone = 1<<(storyDone-1);

		idFile = fOpenPrefs("done.dat","rb+");
		if(!idFile)
		{
			//lets create it
			idFile = fOpenPrefs("done.dat","wb+");
		}
		else
		{
			fread(&alreadyDone,1,sizeof(int),idFile);
			rewind(idFile);
		}

		if(idFile)
		{
			curDone |= alreadyDone;
			fwrite(&curDone,sizeof(int),1,idFile);
			fclose(idFile);
		}
	}
}
