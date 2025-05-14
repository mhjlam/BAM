//	story.cpp
//
//	Copyright 1994, Tachyon, Inc.
//
// Story Line intro screen.
//
//		10-06-94:
//


#include "story.hpp"

#include "api.hpp"
#include "apifont.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamfuncs.hpp"
#include "context.hpp"
#include "graphmgr.hpp"
#include "mouse.hpp"
#include	"palette.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "Scrimage.hpp"
#include "tigre.hpp"

#include <string.h>

// Story is controlled by a few global vars.
// bGlobal.storyLine    = the current storyline.
// bGlobal.missionsDone = # of scenarios completed. Needs to be incremented
//                        by world,assessment,or cinematic room.
// bGlobal.chooseSide   = set by choose.cpp so we know where to set region
//                        done markers.

// This room has three 'modes':
//		Entering room for the first time:
//			Pick a storyline or the legend campaign
//		Returning after you have made your character for the legend campaign
//			Pick starting region
//		Return before next scenario starts to show player the region being entered.
//			No player input allowed -except to make any mouse click set room timer to 0.
//
// This takes input (when allowed) from the player and sets a timer. The
// cycle function will call the room change when the timer reaches zero. In
// the mean time a marker will flash over the selected region.

#define	STORY_INSET_ANI	216

Story::Story()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	shot1 = 0;
	flashToggle=0;
	firstPass = FALSE;

//	int tmpDemoRegions[5] = {0,7,10,-1,-1};
	int tmpDemoRegions[5] = {0,10,-1,-1,-1};
	memcpy(demoRegions,tmpDemoRegions,sizeof(int)*5);

	TRACK_MEM("Story: clut");	clut = AMalloc(CLUT_SIZE);

}

Story::~Story()
{
	if(gTCel)
		AFlush(gTCel);

	AFree(clut);
}


void
Story::Setup()
{
	
	BAM_Guy 		*pGuy;
	BAM_Button	*pButton;
	uint32		x,x1;
	clock_t		newTime;
	int			backAni;
	CelHeader	*ptitle;
	CelHeader	*pInset;

	pBam->voiceChains = 0;

	//default -no selection made by player
	//			 -is reset latter if selection was prev. made
	currentGuy = -1;

 #ifdef INTERACTIVE_DEMO
		ClearGlobals();
		bGlobal.storyLine = LEGEND;	//lets fake out this room
 #else
	//we've finished a complete campaign -so reset vars.
	if (bGlobal.storyLine == LEGEND && bGlobal.missionsDone > MAXREGIONS + 1)
	{
		ClearGlobals();
	}

	//we've finished a complete storyline -so reset vars.
	if (bGlobal.storyLine != LEGEND && bGlobal.missionsDone > 2)
	{
		ClearGlobals();
	}

	storyDone = GetStoryDone();

 #endif

	backAni = 190; //default

	//if making other than the first pass thru this room then set room
	//to change to.
	if (bGlobal.storyLine == STORY_NONE)
	{
		hasSelected = 0;
		firstPass = TRUE;	//use this var. because 'storyLine' will change later
								//(i.e. hasSelected will change)
	}
	else if(bGlobal.storyLine == LEGEND)
	{
		if(bGlobal.missionsDone == 0)
		{
			//need to select starting region
			hasSelected = 0;
		}
		else
		{
			hasSelected = BR_WORLD;
			pMouse->Hide();
		}
	}
	else
	{
		//storyLine already selected 
		hasSelected = BR_WORLD;
		pMouse->Hide();
	}


	// lets load a res_anim and copy it to a dcel
	// so we can copy buttons onto it -and attach the dcel to a guy.

	// setup background guy filled with black
	TRACK_MEM("Story: background cel");
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,5);

	uchar			*pback;
	CelHeader	*pbackCH;
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	CopyCel(pbackCH,0,0,RES_ANIM,backAni,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(5);

	//pal.FadeToBlack();
	pal.Load(backAni);

	//----------------------------
	uchar *pClut = ADerefAs(uchar, clut);

	for(x=0; x<CLUT_SIZE; x++)
	{
		pClut[x] = (uchar)(CLUT2_START + x);
	}

	//----------------------------
	// setup temp. cel for pixel copies
	TRACK_MEM("Story: temp work cel");
	gTCel = ACreateCel(NULL,0,0,2,2,CI_BLACK,5);

	//----------------------------
	// setup a cel for a guy that will display text of region names
	TRACK_MEM("Story: text cel");
	gtitle = ACreateCel(&rNumTitle,0,0,82,72,SKIP_COLOR,250);

	pGuy = &title;
	pGuy->SetRes(RES_CEL,rNumTitle);
	
	pGuy->SetPos(0,0);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(250);

	//----------------------------
	// setup a cel for a guy that will display text of region names
	TRACK_MEM("Story: Inset cel");
	gInset = ACreateCel(&rNumInset,0,0,212,68,SKIP_COLOR,20);

	pGuy = &inset;
	pGuy->SetRes(RES_CEL,rNumInset);
	
	pGuy->SetPos(104,326);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(20);


	//----------------------------
	//char tempNames[21][STORY_NAME_SIZE] =
	//	  {"DOEGAN CAPITAL" ,"SERPENT VALLEY" ,"OLD STONE KEEP",
	//		"HERNE'S WOOD"   ,"WEB MOUNTAINS"  ,"EDENVALE CASTLE",
	//	 	"REALM OF TIDES" ,"REALM OF LANDS" ,"REALM OF FLAME",
	//	 	"GRAND CITY"     ,"PUZZLE PALACE"  ,"HEPHAISTOS HALL",
	//	 	"9350 NOT DONE"  ,"9370 NOT DONE"  ,"9390 NOT DONE",
	//	 	"LEGEND CAMPAIGN",
	//	 	"HOWL OF VENGEANCE","MATCHMAKER MAYHEM","TARTARUS UNBOUND",
	//	 	"NUTS AND !BOLTS"  ,"UNWRITTEN STORY"};

	for (x=0;x<MAXOUTLINEGUYS;x++)
	{
		if(x<=MAXREGIONS)
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


	for(x=0,x1=0; x<MAXSTORYGUYS; x++,x1+=2)
	{
		pGuy = &storyGuy[x];
		pGuy->SetContext(gSelf);
		if(bGlobal.storyLine == LEGEND)
			pGuy->fIsActive = TRUE; //lets not start them in the roster YET
		else
			if(ShowThisStory(GetStory(x),storyDone))
				pGuy->Setup(CT_ROST);
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
 	if(bGlobal.storyLine == LEGEND)
	{
		//set fixed legend inset
		pInset = (CelHeader *)AGetResData(gInset);
		CopyCel(pInset,0,0,RES_ANIM,221,1,FALSE);

		pFontMgr->SetRes(9052);
		//SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);		//"ENGRAVED MARBLE"
		SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);		//"BLUE GREEN"

		#ifdef INTERACTIVE_DEMO
		 	pTxt = sqbStory.Load(STORY_SQB,4); //"choose scenario:"
		 	ASetString(38,325, pTxt, (uchar *)pbackCH, 62, NULL, DG_JUST_CENTER);

		#else
 			if(bGlobal.missionsDone)
			{
				pTxt = sqbStory.Load(STORY_SQB,3); //"now entering:"
				ASetString(38,325, pTxt, (uchar *)pbackCH, 62, NULL, DG_JUST_CENTER);
			}
			else //must be entering a selected storyline
			{
				pTxt = sqbStory.Load(STORY_SQB,2); //"choose starting point:"
				ASetString(38,325, pTxt, (uchar *)pbackCH, 62, NULL, DG_JUST_CENTER);
			}
		#endif

		//Reset color in which to draw names
		pFontMgr->SetRes(9050);
		SetFontColors(CI_SKIP,93,90,93,90,93,90);

		#ifdef INTERACTIVE_DEMO
			int dr;
			for(x=0,x1=0; x<5; x++,x1+=2)
			{
				dr = demoRegions[x];

				if(dr == -1)
					break;

				//lets setup story guys for fixed regions
				pGuy = &storyGuy[dr];
				pGuy->Setup(CT_ROST);

				//let setup and show the fixed region's titles
				TRACK_MEM("Story: title cel");
				gtitleCel[dr] = ACreateCel(&rNumTitleCel[dr],0,0,82,72,SKIP_COLOR,250);

				pGuy = &titleGuy[dr];
				pGuy->SetRes(RES_CEL,rNumTitleCel[dr]);

				pGuy->SetPos(titlePos[dr*2],titlePos[(dr*2)+1]);
				pGuy->SetContext(gSelf);
				pGuy->Setup(CT_ROST);
				pGuy->SetPri(250);

				ptitle = (CelHeader *)AGetResData(gtitleCel[dr]);
				ASetString(0,0,storyNames[dr], (uchar *)ptitle, ptitle->width, NULL, DG_JUST_CENTER);
			}
		#else
 			// if there's some missions done then we can also setup the next region.
 			if(bGlobal.missionsDone)
			{
 				GetLegendPath(legendPath,bGlobal.curPath);

				pFontMgr->SetRes(9050);
				SetFontColors(CI_SKIP,93,90,93,90,93,90);

				int lp;
				for(x=0,x1=0; x<=bGlobal.missionsDone; x++,x1+=2)
				{
					lp = legendPath[x];

					//lets setup story guys for prev conquered regions and the next to enter
					pGuy = &storyGuy[lp];
					pGuy->Setup(CT_ROST);

					//let setup and show just the prev conquered region's titles and the next to enter
					TRACK_MEM("Story: title cel");
					gtitleCel[lp] = ACreateCel(&rNumTitleCel[lp],0,0,82,72,SKIP_COLOR,250);

					pGuy = &titleGuy[legendPath[lp]];
					pGuy->SetRes(RES_CEL,rNumTitleCel[lp]);
	
					pGuy->SetPos(titlePos[lp*2],titlePos[(lp*2)+1]);
					pGuy->SetContext(gSelf);
					pGuy->Setup(CT_ROST);
					pGuy->SetPri(250);

					ptitle = (CelHeader *)AGetResData(gtitleCel[lp]);
					ASetString(0,0,storyNames[lp], (uchar *)ptitle, ptitle->width, NULL, DG_JUST_CENTER);
				}

				currentGuy = legendPath[x-1];
				pBam->LaunchVoice(STORY_SQB, 3, REGION_SQB, currentGuy+1);
			}
			else
			{
				pBam->LaunchVoice(STORY_SQB, 2);
			}
		#endif

	}
	else
	{
		uint32  outlineAnims [] =	  {200,4,  200,5,  200,6,	//region outlines -old
												201,4,  201,5,  201,6,
												202,4,  202,5,  202,6,
												203,4,  203,5,  203,6,
												204,4,  204,5,  204,6,

												206,1,  207,1,  208,1,	//story outlines
												209,1,  210,1};

		uint32  outlinePos [] = {	0,0,		0,0,		0,0, //only story outlines
											0,0,		0,0,		0,0, //used now
											0,0,		0,0,		0,0,
											0,0,		0,0,		0,0,
											0,0,		0,0,		0,0,

										  	42,0,    71,196,	0,0,
									   	0,153, 149,29};

		//lets only do the story outline guys and not the regions
		for(x=15,x1=30; x<MAXOUTLINEGUYS; x++,x1+=2)
		{
			pGuy = &outlineGuy[x];
			pGuy->SetContext(gSelf);
			pGuy->SetPri(200);
	 		pGuy->SetRes(RES_ANIM,outlineAnims[x1],outlineAnims[x1+1]);
	 		pGuy->SetPos(outlinePos[x1],outlinePos[x1+1]);

			if(firstPass)
			{
				TRACK_MEM("STORY: palcycler");
				pGuy->SetupUtil(new PalCycler(PCYC_REV_OSC, 247, 250, pal.gSelf, 2, 0));
			}
		}

		//-------------------------------------------
		pFontMgr->SetRes(9052);
		//SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK);		//"ENGRAVED MARBLE"
		SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);		//"BLUE GREEN"

		if(firstPass)
		{
			pTxt = sqbStory.Load(STORY_SQB,1); //"choose story:"
			ASetString(38,325, pTxt, (uchar *)pbackCH, 62, NULL, DG_JUST_CENTER);
		}
		else //must be entering a selected storyline
		{
			pTxt = sqbStory.Load(STORY_SQB,3); //"now entering:"
			ASetString(38,325, pTxt, (uchar *)pbackCH, 62, NULL, DG_JUST_CENTER);

			//set the story's inset
			pInset = (CelHeader *)AGetResData(gInset);
			CopyCel(pInset,0,0,RES_ANIM,STORY_INSET_ANI+bGlobal.storyLine-1,1,FALSE);

			//add difficulty text to inset portrait
			pFontMgr->SetRes(9064);
			SetFontColors(CI_SKIP,93,90,93,90,93,90);
			pFontMgr->colors[9] = CI_BLACK;

			pTxt = sqbStory.Load(STORY_SQB,10+bGlobal.storyLine);
			ASetString(0,58, pTxt, (uchar *)pInset, 100, NULL, DG_JUST_LEFT);
		}

		//Reset color in which to draw names
		pFontMgr->SetRes(9050);
		SetFontColors(CI_SKIP,93,90,93,90,93,90);


		//-------------------------------------------
		//int  tmpFlagPos[] = {	//REGION FLAGS
		//								108,128, 231,55,  216,126,
		//								250,295, 115,260, 163,295,
		//								19,155,  37,55,  111,45,
		//								122,200,  41,240,  31,295,
		//								201,167, 290,137, 275,238, 
		//
		//								//MAIN STORY FLAGS
		//								177,112, 163,300,  37,74,
		//								63,241, 265,212
		//								};
		//
		//memcpy(flagPos,tmpFlagPos,sizeof(flagPos));
		//
		//for(x=0,x1=0; x<MAXFLAGGUYS; x++,x1+=2)
		//{
		//	pGuy = &flagGuy[x];
		//	pGuy->SetContext(gSelf);
		//	pGuy->SetPri(300);
 		//	pGuy->SetRes(RES_ANIM,205,3);
 		//	pGuy->SetPos(flagPos[x1],flagPos[x1+1]);
		//	// if firstPass or a map ending in '0' then blue clut
		//	if(firstPass || bGlobal.chooseSide == 1)
		//		pGuy->SetClut(192,CLUT_SIZE,clut);
		//	//no call to Setup() yet
		//}

 		// if there's some missions done then we can also setup the next region too.
		if(!firstPass)
		{
			pFontMgr->SetRes(9050);
			SetFontColors(CI_SKIP,93,90,93,90,93,90);

			int	curRegion;
			//setup all flags in regions conquered and next region.
			for(x=0; x<=bGlobal.missionsDone; x++)
			{
				
				if(bGlobal.chooseSide==1)
				{
					curRegion = GetRegion()+x;
				}
				else //==2
				{
					curRegion = GetRegion()+2-x;
				}

				//let setup and show the prev conquered region's titles and the next to enter
				TRACK_MEM("Story: title cel");
				gtitleCel[curRegion] = ACreateCel(&rNumTitleCel[curRegion],0,0,82,72,SKIP_COLOR,250);

				pGuy = &titleGuy[curRegion];
				pGuy->SetRes(RES_CEL,rNumTitleCel[curRegion]);
	
				pGuy->SetPos(titlePos[curRegion*2],titlePos[(curRegion*2)+1]);
				pGuy->SetContext(gSelf);
				pGuy->Setup(CT_ROST);
				pGuy->SetPri(250);

				ptitle = (CelHeader *)AGetResData(gtitleCel[curRegion]);
				ASetString(0,0,storyNames[curRegion], (uchar *)ptitle, ptitle->width, NULL, DG_JUST_CENTER);
			}

			if(bGlobal.chooseSide==1)
				currentGuy = GetRegion()+bGlobal.missionsDone;
			else //==2
				currentGuy = GetRegion()+2-bGlobal.missionsDone;

			//lets turn on story border for regions we've setup
			outlineGuy[MAXREGIONS + GetStory(currentGuy)].Setup(CT_ROST|CT_SERV);

			pBam->LaunchVoice(STORY_SQB, 3, REGION_SQB, currentGuy+1);
		}
		else
		{
			pBam->LaunchVoice(STORY_SQB, 1);
		}
	}

	//==================================================================
	//setup default time and have cycle check
	newTime = clock();
	expireTime = newTime + CLOCKS_PER_SEC*3;
	flagFlashTimer = newTime + FLASH_INTERVAL;

	//==================================================================
	pButton = &diskButton;
	if(!hasSelected)
	{
		pButton->Create(4, 365, 200, RES_ANIM, 129, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;		// click-type button
		pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
	}
	else
	{
		//show button as unavailable -change to ani with good "inactive" cel 3
		pButton->Create(4, 365, 200, RES_ANIM, 129, 1, gSelf);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;		// click-type button
		pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->Select(DG_DISABLED);
	}

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

//	pResMgr->Dump();
//	pMemMgr->Dump(1, "Start Option::Setup");
}




// Return the pixel of a scrimage at a given global coordinate.
uchar
Story::GetPixel(coord celXOff, coord celYOff, res_t srcType, 
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
Story::GetStory(uint32 aindex)
{
	return (aindex / 3) + 1;
}


// return the region number for a given storyLine
uint32
Story::GetRegion()
{
	if (bGlobal.storyLine != LEGEND)
		return ((bGlobal.storyLine-1) * 3);
	else
		// needs to return next region in the chosen path
		//return legendPath[missionsDone]
		return (0);
}


bool
Story::HandleMsg(Message* pMsg)
{
	char		mess[100];
	uint32	x,i;
	bool		legitDemoRegion=FALSE;

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
			switch(pMsg->event.type)
			{
				case E_MOUSE_UP:
				//if no storyLine yet selected then lets see they have now
	 	 		if (!hasSelected)
	  	 		{
					coord		celOffX,celOffY;
					BAM_Guy	*pGuy;
					uchar		color;

					// now lets check the storyline regions
					for(x=0; x<MAXSTORYGUYS; x++)
					{

						pGuy = &storyGuy[x];

						if (pGuy->scrim.rect.Contains(pMsg->event.x,pMsg->event.y))
						{
							celOffX = pMsg->event.x - pGuy->scrim.rect.x1;
							celOffY = pMsg->event.y - pGuy->scrim.rect.y1;
							color = GetPixel(celOffX, celOffY, pGuy->scrim.resType, pGuy->scrim.resNum, pGuy->scrim.cel);
							if (color != CI_SKIP)
							{
					 			if(bGlobal.storyLine != LEGEND) //we're not back from makechar room to pick a starting region
					 			{
									//ignore stories not "shown"
									if(ShowThisStory(GetStory(x),storyDone))
									{
										bGlobal.storyLine = GetStory(x);

										//if first pass then flash all three regions
										//in story else flash region now entering
										if(firstPass)
											//set currentGuy to the middle of the 3 regions
											currentGuy = GetRegion()+1;
										else
											currentGuy = x;

										if (pMono)
											pMono->Out("\n			Clicked on Region is %2d, StoryLine is %d, X:%3d,Y:%3d",x,bGlobal.storyLine,pMsg->event.x,pMsg->event.y);

										hasSelected = BR_CINE;
										//assign opening cinematic
										bGlobal.cinematic = GetCine(bGlobal.storyLine,0,0);
										expireTime = clock()+STORY_DELAY;
										flagFlashTimer =  clock() + FLASH_INTERVAL;
										return TRUE;
									}
					 			}
					 			else //legend already selected -lets pick starting point
					 			{
									#ifdef INTERACTIVE_DEMO
										for(i=0;i<5;i++)
											if(x == demoRegions[i])
												legitDemoRegion = TRUE;

										if(legitDemoRegion)
										{
											currentGuy = x;
											bGlobal.chooseSide = 1;

											int scenarioArr[18] = {
																		9110,9130,9150, //STORYLINE1
																		9170,9190,9210, //STORYLINE2
																		9230,9250,9270, //STORYLINE3
																		9290,9310,9330, //STORYLINE4
																		9350,9370,9390, //STORYLINE5
																		9410,0   ,0   };

											sprintf(pBam->scenarioName,"%d",scenarioArr[x]);

											//adjust for missions that should have been done to reach this map
											bGlobal.missionsDone = GetMissionsDone(pBam->scenarioName);

											GetCharName(&sqbRes,&sqbNum,GetStory(currentGuy),TRUE);
											pTxt = sqbStory.Load(sqbRes,sqbNum);
											strcpy(bGlobal.curName,pTxt); //set global

											//since we know chooseSide is 1 -this is usually done in CHOOSE room
											GetUnitColors(&pBam->sideColors[SIDE1],&pBam->sideColors[SIDE2],GetStory(currentGuy));

											//set global
											hasSelected = BR_WORLD;
											expireTime = clock()+STORY_DELAY;
											flagFlashTimer =  clock() + FLASH_INTERVAL;
										}
									#else
										if (pMono)
											pMono->Out("\nLegend Campaign starting point %d selected.",x);

										bGlobal.legendStart = x;
										currentGuy = x;
										bGlobal.curPath = bGlobal.legendStart*VARPATHS + ARandom2(VARPATHS);
 										GetLegendPath(legendPath,bGlobal.curPath);

										bGlobal.chooseSide = 0;	//not used in legend camp.
										GetScenario(pBam->scenarioName,LEGEND,0);

										//set global
										hasSelected = BR_WORLD;
										expireTime = clock()+STORY_DELAY;
										flagFlashTimer =  clock() + FLASH_INTERVAL;
									#endif

									return TRUE;
					 			}
							}//endif
						}//endif
					}//endfor
	     		}//endif !hasSelected
		  		else
		  		{
		    		// a one shot deal
		  	 		if(!shot1)
			 		{
			 			shot1++;
						//reset time to expire sooner
						expireTime = clock();
						return TRUE;
			 		}
		  		}
				break;

				case E_KEY_DOWN:
					switch (pMsg->event.value)
					{
						case K_ESC:
						case K_Q:
							pBam->Quit();
							return TRUE;
						case K_A:
							if(pMsg->event.modifiers & MOD_ALT)
							{
								WriteStoryDone(1);
								WriteStoryDone(2);
								WriteStoryDone(3);
								WriteStoryDone(4);
								WriteStoryDone(5);
								//lets reset storyDone
								storyDone = GetStoryDone();
								//lets make sure they're all setup
								for(i=0;i<MAXSTORYGUYS;i++)
									storyGuy[i].Setup(CT_ROST);
							}
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
Story::Cycle(void)
{
	coord			celOffX,celOffY;
	BAM_Guy		*pGuy,*pOutGuy;
	uchar			color;
	bool			found=FALSE;
	uint32		calcGuy,x;
	CelHeader	*ptitle,*pInset;
	int			insetCel,i=0;
	bool			legitDemoRegion=FALSE;
	clock_t		newTime;

	clock_t		charTime=0;

	coord theX = pMouse->GetX();
	coord theY = pMouse->GetY();

	//this section is for multi-part voice sounds only
	if(pBam->voiceChains && !pBam->voice1.IsPlaying())
	{
		while(pBam->voiceChains > 1)
		{
			//lets reduce the queue to size one -take the last entry
			memmove(&pBam->rVoiceChains[0][0], &pBam->rVoiceChains[1][0],
				sizeof(pBam->rVoiceChains[0][0]) * pBam->voiceChains * 3);
			pBam->voiceChains--;
		}

		if(pBam->rVoiceChains[0][0] != lastVoiceNum)
		{
			// speech! speech!
			pBam->voice1.Play(pBam->rVoiceChains[0][0], MAX_VOLUME, MkPan(100, 100), NULL, 1);
			if(pBam->rVoiceChains[0][1])
				pBam->voice1.AddToChain(pBam->rVoiceChains[0][1]);
			if(pBam->rVoiceChains[0][2])
				pBam->voice1.AddToChain(pBam->rVoiceChains[0][2]);
			lastVoiceNum = pBam->rVoiceChains[0][0];
		}
		pBam->voiceChains--;
	}

	if(!hasSelected)
	{
		// now lets check the storyline regions
		for(x=0; x<MAXSTORYGUYS; x++)
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

					if(bGlobal.storyLine == LEGEND)
						calcGuy = x;
					else
						//lets get passed the region guys to the story guys
						// in the array
						calcGuy = GetStory(x)+MAXREGIONS;

					ptitle = (CelHeader *)AGetResData(gtitle);

					if (calcGuy != currentGuy) //if new guy
					{
						if (pMono)
							pMono->Out("\nRegion is %2d, StoryLine is %d",x,GetStory(x));

						if(bGlobal.storyLine == LEGEND)
						{
							#ifdef INTERACTIVE_DEMO
								for(i=0;i<5;i++)
									if(demoRegions[i] != -1 && currentGuy == demoRegions[i])
										legitDemoRegion = TRUE;

								// cut old storyGuy
								if (currentGuy != -1 && !legitDemoRegion)
								{
									pOutGuy = &storyGuy[currentGuy];
									pOutGuy->fIsActive = FALSE;
									CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
								}
							#else
								// cut old storyGuy
								if (currentGuy != -1)
								{
									pOutGuy = &storyGuy[currentGuy];
									pOutGuy->fIsActive = FALSE;
									CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
								}
							#endif

							// add new region outline
							currentGuy = calcGuy;
							pOutGuy = &storyGuy[calcGuy];
							pOutGuy->fIsActive = TRUE;
							AddObject(pOutGuy->gSelf, CT_SERV | CT_ROST);

							//voice new region name
							pBam->LaunchVoice(REGION_SQB, currentGuy+1);
						}
						else
						{
							// cut old outline
							if (currentGuy != -1)
							{
								pOutGuy = &outlineGuy[currentGuy];
								pOutGuy->fIsActive = FALSE;
								CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
							}

							// add new region outline
							currentGuy = calcGuy;
							pOutGuy = &outlineGuy[calcGuy];
							pOutGuy->fIsActive = TRUE;
							AddObject(pOutGuy->gSelf, CT_SERV | CT_ROST);

							pBam->LaunchVoice(STORYLINE_SQB, currentGuy-MAXREGIONS);
						}

						if(bGlobal.storyLine != LEGEND)
						{
							//copy in new story inset anim -inset is fixed for legend
							pInset = (CelHeader *)AGetResData(gInset);
							insetCel = (ShowThisStory(GetStory(x),storyDone))?1:2;
							CopyCel(pInset,0,0,RES_ANIM,STORY_INSET_ANI+GetStory(x)-1,insetCel,FALSE);

							//add difficulty text to inset portrait
							pFontMgr->SetRes(9064);
							SetFontColors(CI_SKIP,93,90,93,90,93,90);
							pFontMgr->colors[9] = CI_BLACK;

							pTxt = sqbStory.Load(STORY_SQB,10+GetStory(x));
							ASetString(0,58, pTxt, (uchar *)pInset, 100, NULL, DG_JUST_LEFT);

							inset.SetState(S_CHANGED,TRUE);
						}

						// shutdown region name and reset vars.
						//ptitle = (CelHeader *)AGetResData(gtitle);
						memset(((uchar*)ptitle)+sizeof(CelHeader),SKIP_COLOR,L2R(ptitle->width) * ptitle->height);

						pFontMgr->SetRes(9050);
						SetFontColors(CI_SKIP,93,90,93,90,93,90);
						//startup new region name
						ASetString(0,0,storyNames[calcGuy], (uchar *)ptitle, ptitle->width, NULL, DG_JUST_CENTER);
						//title.SetState(S_CHANGED, TRUE);
						title.SetPos(titlePos[currentGuy*2],titlePos[currentGuy*2+1]);
						break;
					}
				}//endif
			}//endif
		}//endfor
		if(!found && currentGuy != -1) //cursor outside of all guys -shutdown current guy
		{
			if(bGlobal.storyLine == LEGEND)
			{
				pOutGuy = &storyGuy[currentGuy];
				pOutGuy->fIsActive = FALSE;
				CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
				currentGuy = -1;
			}
			else
			{
				pOutGuy = &outlineGuy[currentGuy];
				pOutGuy->fIsActive = FALSE;
				CutObject(pOutGuy->gSelf, CT_SERV | CT_ROST);
				currentGuy = -1;
			}

			if(bGlobal.storyLine != LEGEND)
			{
				//clear only if not legend - legend inset is fixed
				pInset = (CelHeader *)AGetResData(gInset);
				memset(((uchar*)pInset)+sizeof(CelHeader),SKIP_COLOR,L2R(pInset->width) * pInset->height);
				inset.SetState(S_CHANGED, TRUE);
			}

			//shutdown last region name
			ptitle = (CelHeader *)AGetResData(gtitle);
			memset(((uchar*)ptitle)+sizeof(CelHeader),SKIP_COLOR,L2R(ptitle->width) * ptitle->height);
			title.SetState(S_CHANGED, TRUE);
		}
	}
	else //no more selecting just show next region in path or wrap up.
	{
		//need to flash flag in next or selected region in path

		newTime = clock();

		//lets flash legend starting point or next region to enter
		if(bGlobal.storyLine == LEGEND)
		{
			//lets flash legend starting point or next region to enter
			if(newTime > flagFlashTimer)
			{
				if (flashToggle)
				{
					flashToggle=0;
					storyGuy[currentGuy].Hide();
				}
				else
				{
		 			flashToggle++;
					storyGuy[currentGuy].Show();
				}
				flagFlashTimer = newTime + FLASH_INTERVAL;
			}
		}
		else
		{
			//flash region until time expires
			if(newTime > flagFlashTimer)
			{
				if (flashToggle)
				{
					flashToggle=0;
					//need to destinguish between:
					//  entering room for first time (flash selected story flag)
					//  entering room before first scenario (flash region entering)
					//  -missionsDone is 0 for both instances.
					if(firstPass)
					{
						//flash storyline marker
						storyGuy[currentGuy-1].Hide();
						storyGuy[currentGuy].Hide();
						storyGuy[currentGuy+1].Hide();
					}
					else
					{
						//flash region marker
						storyGuy[currentGuy].Hide();
					}
				}
				else
				{
		 			flashToggle++;
					if(firstPass)
					{
						//flash storyline marker
						storyGuy[currentGuy-1].Show();
						storyGuy[currentGuy].Show();
						storyGuy[currentGuy+1].Show();
					}
					else
					{
						//flash region marker
						storyGuy[currentGuy].Show();
					}
				}
				flagFlashTimer = newTime + FLASH_INTERVAL;
			}
		}

		if (newTime > expireTime)
		{
			pBam->voice1.Stop();
			#ifdef INTERACTIVE_DEMO
				//lets waits 'til the last instant to change storyline back
				bGlobal.storyLine = GetStory(currentGuy);
			#endif

			//since this room is on a expire timer lets check to see if the
			//user has already tried to use the option menu to return to the
			//main menu -if so then let 'em
			//Remember: the check for new room is at the top of the main loop

			if(!bGlobal.roomMgr.newRoom)
				bGlobal.roomMgr.NewRoom(hasSelected);
		}
	}

	// call the superclasses method last
	BAM_Room::Cycle();
}


bool
Story::ShowThisStory(int storyNum,int storyDone)
{
	int	storyCvt;

	//always show first story
	if(storyNum == 1)
		return(TRUE);

	//now we know storyNum is >= 2

	//e.g. we show story 2 because story 1 is done
	storyCvt = 1<<(storyNum-2);

	if(storyCvt & storyDone)
		return(TRUE);

	return(FALSE);
}
