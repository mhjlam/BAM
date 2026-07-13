// HALL.CPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
//
// Display Hall of Legends.
//
// 3/8/95
//

#include "hall.hpp"

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
#include <stdio.h>
#include "json.hpp"

#define HALL_BANNER_ANIM	9004
#define HALL_BANNER_SLASH	2
#define HALL_BANNER_DOTS	3


//=========================================================

Hall::Hall()
{
	int x;

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
	gCampArr = grip{};

	scrollingDone = true;
	displayCampaign = false;
	buttonsLocked = false;
	scrollDir = 0;
	//prevScrollDir = 0;
	centerColumnMode = 1;
	centerColorMode = 0;
	deleteCnt = 0;
	immedBail = false;

	int	TxPosNames[2] = {5,165};
	int	TxPosScores[2] = {91,251};
	int	TxPosMaxScores[2] = {125,285};
	int	TyPos[8] = {0,14,28,42,56,70,84,98};

	for(x=0;x<2;x++)
	{
		xPosNames[x] = TxPosNames[x];
		xPosScores[x] = TxPosScores[x];
		xPosMaxScores[x] = TxPosMaxScores[x];
	}

	for(x=0;x<8;x++)
		yPos[x] = TyPos[x];

	for(x = 0; x < 2; x++)
	{
		clut[x] = AMalloc(CLUT_SIZE);
	}
}

Hall::~Hall()
{
	// Can't delete gback as it has been given to a scrimage which will properly flush it.
	//if(gback)
	//	ADelete(gback);

	int	i;

	for(i = 0; i < 2; i++)
	{
		AFree(clut[i]);
		clut[i] = grip{};
	}

	if(gCampArr)
		AFree(gCampArr);
}

void
Hall::Setup()
{
	BAM_Guy		*pGuy;
	BAM_Button	*pButton;
	uchar			*pback;
	CelHeader	*pbackCH;
	int			x;
	uchar 		*pClut1;
	uchar 		*pClut2;

	if(bGlobal.roomMgr.prevRoomNum == BR_ASSESS)
		duringPlay = true; //used to on/off certain features like delete button
	else
		duringPlay = false;

	//defaults -assume setup went wrong
	bGlobal.roomMgr.newRoomMode = bGlobal.roomMgr.prevRoomMode;
	bGlobal.roomMgr.prevRoomMode = HALL_NO_CAMP_CHARS ;

	if(!SetupCampaign())
	{
		//we can't just call newroom here 'cause we're still in setup()

		//gotta reset mouse pointer
		BAM_Room::Setup();

		immedBail = true;
		return;
	}
		
	bGlobal.roomMgr.prevRoomMode = HALL_NO_ERROR; //It's ok, so reset

	//======================================================
	// setup background cel filled with black
	gback = ACreateCel(&rNumBack,0,0,320,400,CI_BLACK,100);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	// copy this anim into the dynamic cel that we will use
	CopyCel(pbackCH,0,0,RES_ANIM,8050,1,false);

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
	SetupBanner();

	LoadHall(0);

	//=================
	// Write the necessary text into the background

	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,45,46,47,49,50,CI_BLACK); //CARVED GREEN
	//"1"
	pTxt = sqbHall.Load(HALL_SQB,1);
	pFontMgr->SetString(0,9, pTxt, (uchar *)pbackCH, pbackCH->width, 0, DG_JUST_CENTER);
	//"2"
	pTxt = sqbHall.Load(HALL_SQB,2);
	pFontMgr->SetString(41,244,pTxt, (uchar *)pbackCH, pbackCH->width, 0);

	//===================================================
	//setup current Guy
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP,64,74);
	gCurrent = ACreateCel(&rNumCurrent,0,0,60,15,CI_SKIP,200);
	uchar *pCurrent = AGetResData(gCurrent);
	//"3"
	pTxt = sqbHall.Load(HALL_SQB,3);
	pFontMgr->SetString(0,0,pTxt, pCurrent, 60, 0);
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
	gCampName = ACreateCel(&rNumCampName,0,0,240,22,CI_SKIP,200);
	pGuy = &campNameGuy;
	pGuy->SetRes(RES_CEL,rNumCampName);
	pGuy->SetPos(40,43);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

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
	gCampScore = ACreateCel(&rNumCampScore,0,0,35,22,CI_SKIP,200);
	pGuy = &campScoreGuy;
	pGuy->SetRes(RES_CEL,rNumCampScore);
	pGuy->SetPos(129,244);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	gMaxCampScore = ACreateCel(&rNumMaxCampScore,0,0,35,22,CI_SKIP,200);
	pGuy = &maxCampScoreGuy;
	pGuy->SetRes(RES_CEL,rNumMaxCampScore);
	pGuy->SetPos(202,244);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	LoadCampaignScores(); //into newly created guys

	//================================
	gLevelScore = ACreateCel(&rNumLevelScore,0,0,316,108,CI_SKIP,200);
	pGuy = &levelScoreGuy;
	pGuy->SetRes(RES_CEL,rNumLevelScore);
	pGuy->SetPos(0,286);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(200);

	LoadLevelScores(); //into newly created guy

	//======================================================
	pFontMgr->SetRes(9050);
	//SetFontColors(CI_SKIP,CI_BLACK,CI_BLACK,CI_BLACK,CI_BLACK,CI_BLACK,CI_BLACK);
	SetFontColors(CI_SKIP,44,46,44,46,44,46); //CARVED GREEN
	//"4"
	pTxt = sqbHall.Load(HALL_SQB,4);
	pFontMgr->SetString(163,250,pTxt, (uchar *)pbackCH, pbackCH->width, 0);
	//"5"
	pTxt = sqbHall.Load(HALL_SQB,5);
	pFontMgr->SetString(5,272,pTxt, (uchar *)pbackCH, pbackCH->width, 0);
	pFontMgr->SetString(165,272,pTxt, (uchar *)pbackCH, pbackCH->width, 0);
	//"6"
	pTxt = sqbHall.Load(HALL_SQB,6);
	pFontMgr->SetString(96,272,pTxt, (uchar *)pbackCH, pbackCH->width, 0);
	pFontMgr->SetString(256,272,pTxt, (uchar *)pbackCH, pbackCH->width, 0);
	//"7"
	pTxt = sqbHall.Load(HALL_SQB,7);
	pFontMgr->SetString(138,272,pTxt, (uchar *)pbackCH, pbackCH->width, 0);
	pFontMgr->SetString(298,272,pTxt, (uchar *)pbackCH, pbackCH->width, 0);

	//======================================================
	// lets setup up all the buttons

	pButton = &diskButton;
	pButton->Create(0, 236, 200, RES_ANIM, 129, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &leftButton;
	pButton->Create(23, 64, 200, RES_ANIM, 8055, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pButton = &rightButton;
	pButton->Create(268, 64, 200, RES_ANIM, 8056, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	pFontMgr->SetRes(9050);

	//don't need -will use button cover-up cel 3
	//gTemp = ALoad(RES_ANIM,8052);
	//CopyCel(pbackCH,260,236,gTemp,9,true);	// del button border

	pButton = &delButton;
	pButton->Create(262, 236, 200, RES_ANIM, 8054, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
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
		pButton->Listen(false);
		pButton->SetCel(3);
		pButton->Draw();
	}
	else
	{
		pButton->Select(false);	 // set button to unselected state - will cause drawing into master cel
	}

	pButton = &exitButton;
	pButton->Create(291, 236, 200, RES_ANIM, 8054, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;		// click-type button
	pButton->SetOwnerCel(rNumBack);		// draws itself into this DCEL, instead of being drawn by Animate() directly
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	//"9"
	pTxt = sqbHall.Load(HALL_SQB,9);
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Select(false);				// set button to unselected state - will cause drawing into master cel

	ConfigButtons();

	BAM_Room::Setup();
	pGraphMgr->Animate();
	pal.FadeUp();

}


void
Hall::SetupHalls()
{
	BAM_Guy	*pGuy;

	//THESE THREE CELS WILL BE TRADED AMONGST THE THREE HALL GUYS
	gLeftHall = ACreateCel(&rNumLeftHall,0,0,320,142,254,200);

	gCenterHall = ACreateCel(&rNumCenterHall,0,0,320,142,254,200);

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


void
Hall::SetupBanner()
{
}


//*************************************************************
//Section: LoadHall and related functions
//*************************************************************

void
Hall::LoadHall(int mode)
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
	CopyCel(pCH,0,0,RES_ANIM,rNumTemp,1,false);	// Left Hall
	CopyCel(pCH,160,0,RES_ANIM,rNumTemp,2,false);	// Right Hall

	switch(columnMode)
	{
		case 1:
			//copy both columns w/shadows
			CopyCel(pCH,12 ,0,RES_ANIM,rNumTemp,3,false);	// Left Column
			CopyCel(pCH,254,0,RES_ANIM,rNumTemp,4,false);	// Right Column
			CopyCel(pCH,0  ,0,RES_ANIM,rNumTemp,5,false);	// Left shadow
			CopyCel(pCH,242,0,RES_ANIM,rNumTemp,6,false);	// Right shadow
			break;
		case 2:
			//copy left column w/shadow
			CopyCel(pCH,12 ,0,RES_ANIM,rNumTemp,3,false);	// Left Column
			CopyCel(pCH,0  ,0,RES_ANIM,rNumTemp,5,false);	// Left shadow
			break;
		case 3:
			//copy right column w/shadow
			CopyCel(pCH,254,0,RES_ANIM,rNumTemp,4,false);	// Right Column
			CopyCel(pCH,242,0,RES_ANIM,rNumTemp,6,false);	// Right shadow
			break;
		default:
			APanic("Hall: Invalid columnMode number");
	}

	//--------------------
	//Load portrait
	CopyCel(pCH,140,24,RES_ANIM,pCampArr[idx].curBody,1,true);	// Body
	CopyCel(pCH,140,24,RES_ANIM,pCampArr[idx].curFace,1,true);	// Face
	CopyCel(pCH,140,24,RES_ANIM,pCampArr[idx].curCover,1,true);	// Cover

	//--------------------
	//Load banner
	color = pCampArr[idx].curBanner / 2;

	pal.LoadPartial(9101,192+(color*8),8,destRange);
	pal.UpdateColors(destRange,destRange+8);

	//even num is slash, odd num is dots banner
	//SIDE1 is always slash
	if(pCampArr[idx].curBanner % 2)
		CopyCel(pCH,140,102,RES_ANIM,HALL_BANNER_ANIM,HALL_BANNER_DOTS,true,clut[rangeIdx]);
	else
		CopyCel(pCH,140,102,RES_ANIM,HALL_BANNER_ANIM,HALL_BANNER_SLASH,true,clut[rangeIdx]);

}


//*************************************************************
//Section: SetupCampaign and its related functions
//*************************************************************
static void build_campaign_path(char* buf, size_t sz)
{
	snprintf(buf, sz, "%scampaign.json", get_pref_dir());
}

static bool write_campaign_json(CampaignHeader& hdr, Campaign* arr)
{
	nlohmann::json root;
	root["mostRecent"] = hdr.mostRecent;
	root["campaigns"]  = nlohmann::json::array();

	for (int i = 0; i < hdr.entries; i++) {
		Campaign& c = arr[i];
		if (c.deleted) continue;
		nlohmann::json j;
		j["curCharId"]        = c.curCharId;
		j["deleted"]          = false;
		j["missionsDone"]     = c.missionsDone;
		j["curPath"]          = c.curPath;
		j["curCover"]         = c.curCover;
		j["curFace"]          = c.curFace;
		j["curBody"]          = c.curBody;
		j["curBanner"]        = c.curBanner;
		j["curName"]          = c.curName;
		j["campaignScore"]    = c.campaignScore;
		j["maxCampaignScore"] = c.maxCampaignScore;
		nlohmann::json ls = nlohmann::json::array();
		for (int k = 0; k < 16; k++) ls.push_back(c.levelScore[k]);
		j["levelScore"] = ls;
		root["campaigns"].push_back(j);
	}

	char path[FILENAME_MAX], tmp[FILENAME_MAX];
	build_campaign_path(path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);

	FILE* fp = fopen(tmp, "w");
	if (!fp) return false;
	std::string s = root.dump(2);
	bool ok = (fwrite(s.c_str(), 1, s.size(), fp) == s.size());
	fclose(fp);
	if (ok) ok = (rename(tmp, path) == 0);
	else     remove(tmp);
	return ok;
}

int CompareCamp(void const *pparm1,void const *pparm2);

int
CompareCamp(void const *pparm1,void const *pparm2)
{
	Campaign	*parm1,*parm2;

	parm1 = (Campaign*)pparm1;
	parm2 = (Campaign*)pparm2;

	if(parm1->campaignScore < parm2->campaignScore)
		return (-1);
	else if (parm1->campaignScore > parm2->campaignScore)
		return 1;
	else
		return 0;
}


bool
Hall::read_campaign_json()
{
	char path[FILENAME_MAX];
	build_campaign_path(path, sizeof(path));
	FILE* fp = fopen(path, "r");
	if (!fp) return false;

	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	rewind(fp);
	std::string buf(sz, '\0');
	fread(buf.data(), 1, sz, fp);
	fclose(fp);

	nlohmann::json root;
	try { root = nlohmann::json::parse(buf); }
	catch (...) { return false; }

	header.mostRecent = root.value("mostRecent", 0);
	auto& arr         = root["campaigns"];
	header.entries    = (int)arr.size();

	gCampArr = AMalloc((header.entries + 1) * sizeof(Campaign));
	pCampArr = ADerefAs(Campaign, gCampArr);
	memset(pCampArr, 0, (header.entries + 1) * sizeof(Campaign));

	for (int i = 0; i < header.entries; i++) {
		auto&     j = arr[i];
		Campaign& c = pCampArr[i];
		c.curCharId        = j.value("curCharId", 0);
		c.deleted          = j.value("deleted", false);
		c.missionsDone     = j.value("missionsDone", 0);
		c.curPath          = j.value("curPath", 0);
		c.curCover         = j.value("curCover", 0);
		c.curFace          = j.value("curFace", 0);
		c.curBody          = j.value("curBody", 0);
		c.curBanner        = j.value("curBanner", 0);
		std::string name   = j.value("curName", "");
		strncpy(c.curName, name.c_str(), sizeof(c.curName) - 1);
		c.campaignScore    = j.value("campaignScore", 0);
		c.maxCampaignScore = j.value("maxCampaignScore", 0);
		if (j.contains("levelScore")) {
			auto& ls = j["levelScore"];
			for (int k = 0; k < 16 && k < (int)ls.size(); k++)
				c.levelScore[k] = ls[k].get<int>();
		}
	}
	return true;
}


bool
Hall::SetupCampaign()
{
	int		x, y;
	bool		overwrite = false;

	bool loaded = bGlobal.writeOut && read_campaign_json();

	if (!loaded)
	{
		if (!duringPlay)
			return false;

		memset(&header, 0, sizeof(CampaignHeader));
		gCampArr = AMalloc(sizeof(Campaign));
		pCampArr = ADerefAs(Campaign, gCampArr);
		memset(pCampArr, 0, sizeof(Campaign));
	}

	//if trying to look at chars from mainmenu and no chars
	if (!header.entries && !duringPlay)
		return false;

	pCamp = pCampArr;

	//if struct found in file w/ same Id and less missions done then overwrite
	//NOTE: there is no current campaign if var. duringPlay not set
	if (loaded)
	{
		for (x = 0; x < header.entries; x++)
		{
			if (duringPlay && pCamp->curCharId == bGlobal.curCharId)
			{
				if (bGlobal.missionsDone >= pCamp->missionsDone)
				{
					overwrite = true;
					pCamp->missionsDone     = bGlobal.missionsDone;
					pCamp->deleted          = false;
					pCamp->curPath          = bGlobal.curPath;
					pCamp->curCover         = bGlobal.curCoverAnim;
					pCamp->curFace          = bGlobal.curFaceAnim;
					pCamp->curBody          = bGlobal.curBodyAnim;
					pCamp->curBanner        = bGlobal.curBanner;
					strcpy(pCamp->curName, bGlobal.curName);
					pCamp->campaignScore    = bGlobal.curCampaignScore;
					pCamp->maxCampaignScore = bGlobal.curMaxCampaignScore;
					for (y = 0; y < 16; y++)
						pCamp->levelScore[y] = bGlobal.curLevelScore[y];
				}
				else
				{
					//then more missions done on campaign in file so keep it
					// and branch current campaign id and add to end of array
					bGlobal.curCharId = GetCharId();
				}
			}
			pCamp++;
		}
	}

	// if no overwrite then lets add current campaign to end of array
	//NOTE: there is no current campaign if var. duringPlay not set
	if (duringPlay && !overwrite)
	{
		header.entries++;
		//Hint: pCamp is now at last entry in the array
		pCamp->curCharId        = bGlobal.curCharId;
		pCamp->missionsDone     = bGlobal.missionsDone;
		pCamp->deleted          = false;
		pCamp->curPath          = bGlobal.curPath;
		pCamp->curCover         = bGlobal.curCoverAnim;
		pCamp->curFace          = bGlobal.curFaceAnim;
		pCamp->curBody          = bGlobal.curBodyAnim;
		pCamp->curBanner        = bGlobal.curBanner;
		strcpy(pCamp->curName, bGlobal.curName);
		pCamp->campaignScore    = bGlobal.curCampaignScore;
		pCamp->maxCampaignScore = bGlobal.curMaxCampaignScore;
		for (y = 0; y < 16; y++)
			pCamp->levelScore[y] = bGlobal.curLevelScore[y];
	}

	if (header.entries > 1)
		qsort((void*)pCampArr, header.entries, sizeof(Campaign), CompareCamp);

	pCamp = pCampArr; //reset
	//find most current entry in the sorted array
	if (duringPlay)
	{
		for (x = 0; x < header.entries; x++)
		{
			if (pCamp->curCharId == bGlobal.curCharId)
			{
				header.mostRecent = x;
				campCnt = x;
				break;
			}
			pCamp++;
		}
	}
	else
	{
		campCnt = header.mostRecent;
	}

	//only need to write out if we've updated an entry during play
	if (duringPlay && bGlobal.writeOut)
		write_campaign_json(header, pCampArr);

	return true;
}


//rewrite campaign file without deleted records
void
Hall::WriteCamp()
{
	if (!bGlobal.writeOut)
		return;

	int cnt = 0;
	pCamp = pCampArr;
	for (int x = 0; x < header.entries; x++)
	{
		if (!pCamp->deleted) cnt++;
		pCamp++;
	}

	CampaignHeader tmpHeader = header;
	tmpHeader.entries = cnt;
	write_campaign_json(tmpHeader, pCampArr);
}


//*************************************************************
//Section: DisplayCampaign and its related functions
//*************************************************************

void
Hall::DisplayCampaign()
{
	pCampArr = ADerefAs(Campaign,gCampArr);
	ConfigButtons(scrollDir);
	CheckMostRecent();
	LoadNameRank();
	LoadCampaignScores();
	LoadLevelScores();
}


void
Hall::ConfigButtons(int dir)
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
		leftButton.Listen(false);
		leftButton.Draw();
	}
	else
	{
		//leftButton.Show();
		leftButton.Select(false);
		leftButton.Listen(true);
	}

	if(rightIdx == -1)
	{
		//rightButton.Hide();
		rightButton.SetCel(3); //show "background"
		rightButton.Listen(false);
		rightButton.Draw();
	}
	else
	{
		//rightButton.Show();
		rightButton.Select(false);
		rightButton.Listen(true);
	}
}


// dir should always be passed as 1 or -1.
int
Hall::FindValid(int campCnt_P,int dir)
{
	int	tmpCnt;
	bool  done = false;

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
				done = true;
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
				done = true;
			}
		}
	}

	return (-1);
}
		

void
Hall::CheckMostRecent()
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
			pButton->Listen(false);
			pButton->SetCel(3);
			pButton->Draw();
		}
		else
		{
			currentGuy.Hide();

			pFontMgr->SetRes(9050);
			pButton->Listen(true);
			pButton->Select(false);				// set button to unselected state - will cause drawing into master cel
		}
	}
}


void
Hall::LoadNameRank()
{
	uchar		*pName,*pRank;
	char		rank[40];

	rank[0] = '\0';
	pFontMgr->SetRes(9052);
	SetFontColors(CI_SKIP,93,76,74,74,48,CI_BLACK);

	pName = AGetResData(gCampName);
	pCH = (CelHeader*)pName;
	strcpy(tmpStr,pCampArr[campCnt].curName);
	pFontMgr->SetString(0,0,tmpStr, pName, pCH->width, 0, DG_JUST_CENTER);
	campNameGuy.SetState(S_CHANGED, true);

	pRank = AGetResData(gCampRank);
	pCH = (CelHeader*)pRank;
	GetAlignment(&sqbRes,&sqbNum);
	pTxt = sqbHall.Load((uint)sqbRes,sqbNum);
	strcpy(rank,pTxt);
	strcat(rank," ");

	GetRank(&sqbRes,&sqbNum,pCampArr[campCnt].campaignScore);
	pTxt = sqbHall.Load((uint)sqbRes,sqbNum);
	strcat(rank,pTxt);

	pFontMgr->SetString(0,0,rank, pRank, pCH->width, 0, DG_JUST_CENTER);
	campRankGuy.SetState(S_CHANGED, true);
}


void
Hall::LoadCampaignScores()
{
	uchar			*pCampScore,*pMaxCampScore;

	//===================================================
	//setup campScoreGuy
	pFontMgr->SetRes(9058);
	SetFontColors(CI_SKIP,67,65,62,CI_BLACK,CI_BLACK,CI_BLACK); //Green

	snprintf(tmpStr, sizeof(tmpStr),"%5d",pCampArr[campCnt].campaignScore);
	pCampScore = AGetResData(gCampScore);
	pFontMgr->SetString(0,0, tmpStr, pCampScore, 35, 0);

	campScoreGuy.SetState(S_CHANGED, true);

	//===================================================
	//setup maxCampScoreGuy
	SetFontColors(CI_SKIP,93,103,101,CI_BLACK,CI_BLACK,CI_BLACK); //Blue

	snprintf(tmpStr, sizeof(tmpStr),"%5d",pCampArr[campCnt].maxCampaignScore);
	pMaxCampScore = AGetResData(gMaxCampScore);
	pFontMgr->SetString(0,0, tmpStr, pMaxCampScore, 35, 0);

	maxCampScoreGuy.SetState(S_CHANGED, true);
}


void
Hall::LoadLevelScores()
{

	uchar			*pLevelScore;
	int			i,tmpNum,missions;

	missions = pCampArr[campCnt].missionsDone;

	pLevelScore = AGetResData(gLevelScore);
	pCH = (CelHeader*)pLevelScore;

	//get legendPath for this campaign
	GetLegendPath(legendPath,pCampArr[campCnt].curPath);

	pFontMgr->SetRes(9050);
	//SetFontColors(CI_SKIP,CI_BLACK,CI_BLACK,CI_BLACK,CI_BLACK,CI_BLACK,CI_BLACK);
	SetFontColors(CI_SKIP,44,46,44,46,44,46); //CARVED GREEN

	//write level names
	for(i=0;i<missions;i++)
	{
		GetLevelName(&sqbRes,&sqbNum,LEGEND,legendPath[i]);
		pTxt = sqbHall.Load(sqbRes,sqbNum);
		if(i<8)
			pFontMgr->SetString(xPosNames[0],yPos[i], pTxt, pLevelScore, pCH->width, 0);
		else
			pFontMgr->SetString(xPosNames[1],yPos[i-8], pTxt, pLevelScore, pCH->width, 0);
	}

	SetFontColors(CI_SKIP,66,66,66,62); //GREEN
	//write level scores
	for(i=0;i<missions;i++)
	{
		snprintf(tmpStr, sizeof(tmpStr),"%d",pCampArr[campCnt].levelScore[i]);
		if(i<8)
			pFontMgr->SetString(xPosScores[0],yPos[i], tmpStr, pLevelScore, 31, 0, DG_JUST_RIGHT);
		else
			pFontMgr->SetString(xPosScores[1],yPos[i-8], tmpStr, pLevelScore, 31, 0, DG_JUST_RIGHT);
	}

	SetFontColors(CI_SKIP,78,78,78,101); //BLUE
	//write level max scores
	for(i=0;i<missions;i++)
	{
		tmpNum = 200 * (i+1);
		snprintf(tmpStr, sizeof(tmpStr),"%d",tmpNum);
		if(i<8)
			pFontMgr->SetString(xPosMaxScores[0],yPos[i], tmpStr, pLevelScore, 31, 0, DG_JUST_RIGHT);
		else
			pFontMgr->SetString(xPosMaxScores[1],yPos[i-8], tmpStr, pLevelScore, 31, 0, DG_JUST_RIGHT);

	}

	levelScoreGuy.SetState(S_CHANGED, true);
}


//*************************************************************
//Section: HandleMsg and related functions
//*************************************************************

bool
Hall::HandleMsg(Message* pMsg)
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
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
				{
					if(pMsg->notice.gSource == diskButton.gSelf)
					{
						pMono->Out("\nDiskButton was hit");
						if(!buttonsLocked)
							Option(pal.gSelf);
						return(true);
					}
					else if(pMsg->notice.gSource == delButton.gSelf)
					{
						pMono->Out("\ndelButton was hit");
						if(!buttonsLocked)
						{
							deleteCnt++;
							pCampArr = ADerefAs(Campaign,gCampArr);
							pCampArr[campCnt].deleted = true;

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
								NextRoom();
							}
							else
							{
								buttonsLocked = true;
								ClearAll();
								scrollingDone = false;
								scrollDir = dir;
								scrollStepCnt = 0;
								LoadHall(dir);
							}
						}
						return(true);
					}
					else if(pMsg->notice.gSource == exitButton.gSelf)
					{
						pMono->Out("\nexitButton was hit");
						if(!buttonsLocked)
						{
							pCampArr = ADerefAs(Campaign,gCampArr);
							if(deleteCnt) 
								WriteCamp(); //rewrite w/o deleted records

							NextRoom();
						}
						return(true);
					}
					else if(pMsg->notice.gSource == leftButton.gSelf)
					{
						pMono->Out("\nleftButton was hit");
						if(!buttonsLocked)
						{
							pCampArr = ADerefAs(Campaign,gCampArr);
							//left button will hidden if no valid entries in that dir
							buttonsLocked = true;
							ClearAll();
							scrollingDone = false;
							//prevScrollDir = scrollDir;
							scrollDir = -1;		//to go left = to scroll left
							scrollStepCnt = 0;
							//if we've already been there, then no need to re-setup
							//if(prevScrollDir != 1) //prev campaign could be deleted
								LoadHall(-1); //left == -1
						}
						return(true);
					}
					else if(pMsg->notice.gSource == rightButton.gSelf)
					{
						pMono->Out("\nrightButton was hit");
						if(!buttonsLocked)
						{
							pCampArr = ADerefAs(Campaign,gCampArr);
							//right button will hidden if no valid entries in that dir
							buttonsLocked = true;
							ClearAll();
							scrollingDone = false;
							//prevScrollDir = scrollDir;
							scrollDir = 1;
							scrollStepCnt = 0;
							//if we've already been there, then no need to re-setup
							//if(prevScrollDir != -1) //prev campaign could be deleted
								LoadHall(1);  //right == 1
						}
						return(true);
					}

				}
				else
         	{
            	snprintf(mess, sizeof(mess), "Unrecognized button notice reply: %d", pMsg->notice.param);
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
						return false;

					case K_ESC:
						FakeMouseClick(diskButton.gSelf);
						return true;

					case K_D: //delete
						//FakeMouseClick(delButton.gSelf);
						return true;

					case K_E: //exit
						FakeMouseClick(exitButton.gSelf);
						return true;

					case K_LEFT:
						FakeMouseClick(leftButton.gSelf);
						return true;

					case K_RIGHT:
						FakeMouseClick(rightButton.gSelf);
						return true;

				}
			}
			break;
	}//endswitch

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}


void
Hall::ClearAll()
{
	uchar		*pName,*pRank;
	uchar		*pNameData,*pRankData;
	uchar			*pCampScore,*pMaxCampScore;
	uchar			*pCampScoreData,*pMaxCampScoreData;
	uchar		*pLevelScore,*pLevelScoreData;

	//clear name
	pName = AGetResData(gCampName);
	pCH = (CelHeader*)pName;
	pNameData = pName + sizeof(CelHeader); //we know its a single cel
	memset(pNameData,CI_SKIP, (pCH->width) * pCH->height); //clear cel
	campNameGuy.SetState(S_CHANGED,true);

	//clear rank
	pRank = AGetResData(gCampRank);
	pCH = (CelHeader*)pRank;
	pRankData = pRank + sizeof(CelHeader); //we know its a single cel
	memset(pRankData,CI_SKIP, (pCH->width) * pCH->height); //clear cel
	campRankGuy.SetState(S_CHANGED,true);

	//clear CampScore
	pCampScore = AGetResData(gCampScore);
	pCH = (CelHeader*)pCampScore;
	pCampScoreData = pCampScore + sizeof(CelHeader); //we know its a single cel
	memset(pCampScoreData,CI_SKIP, (pCH->width) * pCH->height); //clear cel
	campScoreGuy.SetState(S_CHANGED,true);

	//clear MaxCampScore
	pMaxCampScore = AGetResData(gMaxCampScore);
	pCH = (CelHeader*)pMaxCampScore;
	pMaxCampScoreData = pMaxCampScore + sizeof(CelHeader); //we know its 1 cel
	memset(pMaxCampScoreData,CI_SKIP, (pCH->width) * pCH->height);
	maxCampScoreGuy.SetState(S_CHANGED,true);

	//clear LevelScore
	pLevelScore = AGetResData(gLevelScore);
	pCH = (CelHeader*)pLevelScore;
	pLevelScoreData = pLevelScore + sizeof(CelHeader); //we know its 1 cel
	memset(pLevelScoreData,CI_SKIP, (pCH->width) * pCH->height); //clear cel
	levelScoreGuy.SetState(S_CHANGED,true);
}


//*************************************************************
//Section: Cycle and related functions
//*************************************************************

void
Hall::Cycle()
{
	if(immedBail)
	{
		//return to the room in the mode we left
		bGlobal.roomMgr.NewRoom(BR_MENU);
		return;
	}
	
	//this first so campaign is displayed on the cycle after scrolling done
	if(displayCampaign)
	{
	  	displayCampaign = false;
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
			scrollingDone = true;
			buttonsLocked = false;
			displayCampaign = true;
		}
	}

	BAM_Room::Cycle();
}


void
Hall::ResetHalls()
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


void
Hall::NextRoom()
{
	if(bGlobal.roomMgr.prevRoomNum == BR_ASSESS)
	{
		//do we go to cine before bonus map?
		if(bGlobal.missionsDone == 15)
		{
			bGlobal.cinematic = 3815;
			bGlobal.roomMgr.NewRoom(BR_CINE);
		}
		else
		if(bGlobal.missionsDone == 16)
		{
			//we've finished campaign
			bGlobal.roomMgr.NewRoom(BR_MENU);
		}
		else
		{
			// missions 1-14: scenarioName already set by assess.cpp; go play next scenario
			bGlobal.roomMgr.NewRoom(BR_WORLD);
		}
	}
	else
	if(bGlobal.roomMgr.prevRoomNum == BR_MENU)
	{
		//return to the room in the mode we left
		bGlobal.roomMgr.NewRoom(BR_MENU);
	}
	else 
	{
		APanic("Hall: Bad previous room!");
	}
}
