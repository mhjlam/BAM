// LEGENDOP.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen to show legendary opponent for this map.
//
// 10/28/95
//

#include "api.hpp"
#include "apires.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "context.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "scrimage.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "LEGENDOP.HPP"
#include "story.hpp"

#include <string.h>

#define FIRSTCOVERANIM		2850	
#define FIRSTMENFACEANIM	2900	
#define FIRSTWOMENFACEANIM	2940	
#define FIRSTOTHERFACEANIM	2970	
#define FIRSTBODYANIM		3000	

#define LEGENDOPP_BASE_PRI	21000
#define LEGENDOPP_WIN_X		20
#define LEGENDOPP_WIN_Y		38

//=========================================================

LegendOpponent::LegendOpponent()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;

	maxMenNameNum = 0;
	maxWomenNameNum = 0;
	maxOtherNameNum = 0;

	gender_t tmpcoverGen[MAX_COVER] = {
			G_ALL,G_MEN | G_OTHER,G_ALL,G_ALL,G_MEN,G_WOMEN,
			G_WOMEN,G_ALL,G_ALL,G_MEN | G_WOMEN,G_MEN | G_OTHER};

	gender_t tmpbodyGen[MAX_BODY] = {
			G_ALL,G_ALL,G_MEN,G_WOMEN | G_OTHER,G_ALL,
			G_ALL,G_ALL,G_ALL,G_WOMEN,G_ALL};

	//now each map at each hardness level (maps 1-5,6-10,11-15) has an
	//opponent of fixed gender
	gender_t tmpmapGen[15][3] = {
			G_MEN  ,G_OTHER,G_WOMEN,
			G_WOMEN,G_MEN  ,G_OTHER,
			G_OTHER,G_WOMEN,G_MEN,
			G_MEN  ,G_WOMEN,G_OTHER,
			G_OTHER,G_WOMEN,G_MEN,
			G_MEN  ,G_OTHER,G_WOMEN,
			G_MEN  ,G_WOMEN,G_OTHER,
			G_MEN  ,G_OTHER,G_WOMEN,
			G_WOMEN,G_OTHER,G_WOMEN,
			G_OTHER,G_MEN  ,G_WOMEN,
			G_MEN  ,G_WOMEN,G_OTHER,
			G_OTHER,G_MEN  ,G_WOMEN,
			G_MEN  ,G_OTHER,G_WOMEN,
			G_OTHER,G_MEN  ,G_WOMEN,
			G_OTHER,G_MEN  ,G_MEN};


	memcpy(coverGen,tmpcoverGen,sizeof(gender_t)*MAX_COVER);
	memcpy(bodyGen,tmpbodyGen,sizeof(gender_t)*MAX_BODY);
	memcpy(mapGen,tmpmapGen,sizeof(gender_t)*15*3);

	TRACK_MEM("Choose Clut");	clut = AMalloc(CLUT_SIZE);
}

LegendOpponent::~LegendOpponent()
{
	AFree(clut);
}

void
LegendOpponent::Setup(grip gPal_P,bool fadeup)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackCH,*pbackAnimCH;
	int			i;

	gPal = gPal_P;

	//======================================================
	//we do no setup for the bonus level
	if(bGlobal.missionsDone == 15)
	{
		if(fadeup)
		{
			TPalette *pPal = ADerefAs(TPalette,gPal);
			pPal->FadeUp();
		}

		delete this;
		return;
	}

	//======================================================

	prevFont = pFontMgr->curFontNum;

	//======================================================

	//pResMgr->Dump();
	//pMemMgr->Dump(1, "Start LegendOpponent::Setup");

	mode = M_MODAL;
	bGlobal.roomMgr.curRoom->Pause(TRUE);

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	TRACK_MEM("LegendOpponent: background anim");
	gbackAnim = ALoad(RES_ANIM,50);
	pbackAnimCH = (CelHeader*) AGetResData(gbackAnim);

	// os-mac stuff is for this screen to be centered. since
	// the blue/grey fadeTo doesn't work the first time in and
	// it looks funny to have the screen in the top left
	#ifdef OS_MAC
		extern bool startup_game;
		int xCoord;
		int yCoord;
		if (startup_game)
		{
			xCoord = 160 - (pbackAnimCH->width/2);
			yCoord = 200 - (pbackAnimCH->height/2);
			rback.Set(xCoord,yCoord,xCoord-1+pbackAnimCH->width,yCoord-1+pbackAnimCH->height);
		}
		else
			rback.Set(LEGENDOPP_WIN_X+0, LEGENDOPP_WIN_Y+0, LEGENDOPP_WIN_X+pbackAnimCH->width-1, LEGENDOPP_WIN_Y+pbackAnimCH->height-1);
	#else
		rback.Set(LEGENDOPP_WIN_X+0, LEGENDOPP_WIN_Y+0, LEGENDOPP_WIN_X+pbackAnimCH->width-1, LEGENDOPP_WIN_Y+pbackAnimCH->height-1);
	#endif

	//======================================================
	// setup background cel filled with black
	TRACK_MEM("LegendOpponent: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,LEGENDOPP_BASE_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*) pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,50,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	#ifdef OS_MAC
		if(startup_game)
			pGuy->SetPos(xCoord,yCoord);
		else
			pGuy->SetPos(LEGENDOPP_WIN_X,LEGENDOPP_WIN_Y);
	#else
		pGuy->SetPos(LEGENDOPP_WIN_X,LEGENDOPP_WIN_Y);
	#endif
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(LEGENDOPP_BASE_PRI);

	pMouse->SetLimits(&rback);


	//======================================================
	// 0 percent fade down -just remaps colors to blue-gray range.
	//fade already done by winlose
	//fadeTo.Setup(320,400,LEGENDOPP_BASE_PRI - 10,gSelf,gPal,0,&rback);

	//======================================================
	uchar 		*pClut;
	pClut = ADerefAs(uchar, clut);

	for(i=0;i<CLUT_SIZE;i++)
	{
		pClut[i] = (uchar)(CLUT2_START + i);
	}

	//===============================================================
	//generate random enemy for this map -no repeat enemy names or faces
	
	int hardness;
	int md = bGlobal.missionsDone;

 	GetLegendPath(legendPath,bGlobal.curPath);
	int mapId = legendPath[md];

	if(md < 5)
		hardness = 1;
	else
	if(md < 10)
		hardness = 2;
	else
		hardness = 3;
		
	curSqbNum = (mapId * 100) + (hardness * 10) + 1;

	if(bGlobal.replayMap)
	{
		bGlobal.replayMap = FALSE;
	}
	else
	{
		//get opponent gender
		bGlobal.enemyCat = mapGen[mapId][hardness-1];

		RandomCover();
		RandomFace();
		RandomBody();
		RandomBanner();
		int	nameSqb = RandomName();
		pTxt = sqbLegendOpp.Load(nameSqb,enemyNameNum);
		strcpy(bGlobal.enemyName,pTxt);
	}


	//===============================================================
	//Get and display text message for this map

	//char		oppText[250];
	//sqbRes = LEGENDOPP_SQB;
	//ReplaceName(oppText,sqbRes,curSqbNum);

	pTxt = sqbLegendOpp.Load(LEGENDOPP_SQB,curSqbNum);

	pFontMgr->SetRes(9050);
	//SetFontColors(CI_SKIP,93,90,93,90,93,90);
	SetFontColors(CI_SKIP,94,102,94,102,94,102);
	ASetString(85, 48, pTxt, pback, 107, NULL);

	pBam->voice1.Stop();
	pBam->voice1.Play(LEGENDOPP_SQB * 10000 +curSqbNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);

	//==================================================
	//copy borders into background cel

	CopyCel((CelHeader*)pback,35,77,RES_ANIM,50,2,TRUE);	// Portrait Border 2
	CopyCel((CelHeader*)pback,64,212,RES_ANIM,50,3,TRUE);	// 'Done' w/undone button frame

	//Load portrait of enemy
	CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.enemyBodyAnim,1,TRUE);	// Body
	CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.enemyFaceAnim,1,TRUE);	// Face
	CopyCel(pbackCH,37,80,RES_ANIM,bGlobal.enemyCoverAnim,1,TRUE);	// Cover
	CopyCel(pbackCH,37,158,RES_ANIM,9004,3,TRUE,clut);	//Banner
	
	//==================================================
	pButton = &buttonDone;
	#ifdef OS_MAC
		if(startup_game)
			pButton->Create(66+xCoord, 215+yCoord, LEGENDOPP_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf, xCoord, yCoord);
		else
			pButton->Create(LEGENDOPP_WIN_X+66, LEGENDOPP_WIN_Y+215, LEGENDOPP_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf,
								 LEGENDOPP_WIN_X, LEGENDOPP_WIN_Y);
	#else
		pButton->Create(LEGENDOPP_WIN_X+66, LEGENDOPP_WIN_Y+215, LEGENDOPP_BASE_PRI + 1, RES_ANIM, 8160, 1, gSelf,
							 LEGENDOPP_WIN_X, LEGENDOPP_WIN_Y);
	#endif
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetTextOffSet(0, 0);
	pTxt = sqbLegendOpp.Load(OPTION_SQB,22); //'continue'
	pButton->SetCelText(1, pTxt);
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(FALSE);


	//==================================================

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;									// grip of currently active button, if any

	AAnimate();

	if(fadeup)
	{
		TPalette *pPal = ADerefAs(TPalette,gPal);
		pPal->FadeUp();
	}

}


void
LegendOpponent::ReplaceName(char *destStr,int res,int num)
{
	char	*pDest,*pSrc;
	char	srcStr[300];

	pTxt = sqbLegendOpp.Load(res,num);
	strcpy(srcStr,pTxt);
	strcpy(destStr,pTxt);

	pSrc = strstr(srcStr, "(" );

	if(pSrc)
	{
		while(pSrc)
		{
			//we have some name replacing to do

			//find same parenth in dest
			pDest = strstr(destStr, "(" );
			//trunc string at start parenth
			*pDest = '\0';

			//cat in enemy name
			strcat(destStr,bGlobal.enemyName);

			//let remove leading parenth from src
			*pSrc = 'x';

			//now lets find matching parenth in src and remove
			pSrc = strstr(srcStr, ")" );
			*pSrc = 'x';
			pSrc++;

			//cat in the rest of src
			strcat(destStr,pSrc);

			//any more '(' ?
			pSrc = strstr(srcStr, "(" );
		}

	}
}


bool
LegendOpponent::HandleMsg(Message* pMsg)
{
	char			oppText[250];
	uchar			*pback;

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
						if(pMsg->notice.gSource == buttonDone.gSelf)
						{
							pMono->Out("\nDone Button was hit");

							curSqbNum++;
							pTxt = sqbLegendOpp.Load(LEGENDOPP_SQB,curSqbNum,TRUE);

						
							if(pTxt)
							{
								//more sqb's to write
								ReplaceName(oppText,LEGENDOPP_SQB,curSqbNum);
								ClearText();

								pBam->voice1.Stop();
								pBam->voice1.Play(LEGENDOPP_SQB * 10000 +curSqbNum, MAX_VOLUME, MkPan(100, 100), NULL, 1);

								pback = AGetResData(gback);
								SetFontColors(CI_SKIP,94,102,94,102,94,102);
								ASetString(85, 48, oppText, pback, 107, NULL);
							}
							else
							{
								pBam->voice1.Stop();
								Cleanup();
							}

							return TRUE;
						}
					}
				}
				break;

			case MSG_EVENT:
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
								//lets go thru world
								//return pBam->HandleMsg(pMsg);
								break;
							case K_X:
								if(pMsg->event.modifiers & MOD_ALT)
								{
									Cleanup();
									pContextMgr->Quit();
									return TRUE;
								}
								break;
							case K_ESC:
							case K_Q:
								pBam->voice1.Stop();
								Cleanup();
								return TRUE;
								break;

							case K_C:	//continue
								FakeMouseClick(buttonDone.gSelf);
								return TRUE;
								break;
						}
				}
				break;
		}//endswitch
	}
	return TRUE;	//don't pass thru
}


void
LegendOpponent::Cleanup()
{
	pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	//fadeTo.FadeUp();
	bGlobal.roomMgr.curRoom->Pause(FALSE);

	Activate(FALSE);

	delete this;
}



void
LegendOpponent::RandomCover()
{
	bool	nextCoverInGen = FALSE;
	int	curCover = (bGlobal.curCoverAnim - FIRSTCOVERANIM) / 2;

	do
	{
		enemyCover = MyRandom(MAX_COVER,curCover);

		if(coverGen[enemyCover] & bGlobal.enemyCat)
		{
			bGlobal.enemyCoverAnim = FIRSTCOVERANIM + (2*enemyCover);
			nextCoverInGen = TRUE;
		}

	}while(!nextCoverInGen);

}


void
LegendOpponent::RandomFace()
{
	do
	{
		switch(bGlobal.enemyCat)
		{
			case G_MEN:
				enemyFace = MyRandom(MAX_FACE_MEN);
				bGlobal.enemyFaceAnim = FIRSTMENFACEANIM + (2*enemyFace);
				break;
			case G_WOMEN:
				enemyFace = MyRandom(MAX_FACE_WOMEN);
				bGlobal.enemyFaceAnim = FIRSTWOMENFACEANIM + (2*enemyFace);
				break;
			case G_OTHER:
				enemyFace = MyRandom(MAX_FACE_OTHER);
				bGlobal.enemyFaceAnim = FIRSTOTHERFACEANIM + (2*enemyFace);
				break;
			default:
				APanic("Bad catagory in LegendOpponent room");
				break;
		}
	}while ( LastFaceMatch(bGlobal.enemyFaceAnim) );

	bGlobal.lastEnemyFaceAnim[bGlobal.missionsDone] = bGlobal.enemyFaceAnim;
}


void
LegendOpponent::RandomBody()
{
	bool	nextBodyInGen = FALSE;
	int curBody = (bGlobal.curBodyAnim - FIRSTBODYANIM) / 2;

	do
	{
		enemyBody = MyRandom(MAX_BODY,curBody);

		if(bodyGen[enemyBody] & bGlobal.enemyCat)
		{
			bGlobal.enemyBodyAnim = FIRSTBODYANIM + (2*enemyBody);
			nextBodyInGen = TRUE;
		}

	}while(!nextBodyInGen);
}


void
LegendOpponent::RandomBanner()
{
	int	bannerColor;

	do
	{
		//get us a color not used by player
		bannerColor = MyRandom(MAX_BANNER,bGlobal.curBanner/2) * 2;

		//and also not used by last enemy
	}while( bannerColor == bGlobal.enemyBanner );

	bGlobal.enemyBanner = bannerColor;

	if(pBam->playerSide == SIDE1)
		pBam->sideColors[SIDE2] = (unitColor_t) ((bannerColor/2) + 1);
	else
		pBam->sideColors[SIDE1] = (unitColor_t) ((bannerColor/2) + 1);

	LoadUnitColors(gPal,pBam->sideColors[SIDE1],pBam->sideColors[SIDE2]);
}


int
LegendOpponent::RandomName()
{
	int	res;

	do
	{
		switch(bGlobal.enemyCat)
		{
			case G_MEN:
				res = MEN_SQB;
				if(!maxMenNameNum)
					maxMenNameNum = atoi(sqbLegendOpp.Load(MEN_SQB,1));
				//add 2 to num to get zero based random num past first sqb used above
				enemyNameNum = MyRandom(maxMenNameNum) + 2;
				break;
			case G_WOMEN:
				res = WOMEN_SQB;
				if(!maxWomenNameNum)
					maxWomenNameNum = atoi(sqbLegendOpp.Load(WOMEN_SQB,1));
				enemyNameNum = MyRandom(maxWomenNameNum) + 2;
				break;
			case G_OTHER:
				res = OTHER_SQB;
				if(!maxOtherNameNum)
					maxOtherNameNum = atoi(sqbLegendOpp.Load(OTHER_SQB,1));
				enemyNameNum = MyRandom(maxOtherNameNum) + 2;
				break;
		}
	} while ( LastNameMatch(enemyNameNum,res) );

	bGlobal.lastEnemyNameNum[bGlobal.missionsDone] = enemyNameNum;

	return res;
}


int
LegendOpponent::MyRandom(int max, int oldnum)
{
	int num;

	if(oldnum == -1)
	{
		num = ARandom2(max);
		return num;
	}

	do
	{
		num = ARandom2(max);
	} while(num == oldnum);
	return num;
}


//have we used this face before?
bool
LegendOpponent::LastFaceMatch(int faceAnim)
{
	bool	ret = FALSE;
	int	i;

	for(i=0;i<bGlobal.missionsDone;i++)
	{
		if(faceAnim == bGlobal.lastEnemyFaceAnim[i])
		{
			ret = TRUE;
			break;
		}
	}

	//can't use players face
	if(faceAnim == bGlobal.curFaceAnim)
		ret = TRUE;

	return (ret);
}



//have we used this name before?
bool
LegendOpponent::LastNameMatch(int nameNum, int nameRes)
{
	bool	ret = FALSE;
	int	i;

	for(i=0;i<bGlobal.missionsDone;i++)
	{
		//gender cat and name num together give a specific name
		if(bGlobal.enemyCat == bGlobal.lastEnemyCatNum[i]
			&& nameNum == bGlobal.lastEnemyNameNum[i])
		{
			ret = TRUE;
			break;
		}
	}

	if(!ret)	
	{
		//does enemy name match player's name?
		pTxt = sqbLegendOpp.Load(nameRes,nameNum);
		if(!strcmp(pTxt,bGlobal.curName))
			ret = TRUE;
	}

	return (ret);
}


void
LegendOpponent::ClearText()
{
	CelHeader	*pbackCH;
	Rectangle	rTxt;

	pbackCH = (CelHeader*) AGetResData(gback);
	rTxt.Set(84,47,193,200);
	CopyCel(pbackCH,84,47,RES_ANIM,50,1,&rTxt,FALSE);
	back.SetState(S_CHANGED,TRUE);

}
