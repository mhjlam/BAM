//
//	Copyright 1994, Tachyon, Inc.
//
// Top level of BAM application.
//
//		09-07-94:
//

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "apidlg.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "eventmgr.hpp"

#include "mouseint.hpp"

#include "rect.hpp"
#include "graphmgr.hpp"
//#include "osgraph.hpp"
#include "tigre.hpp"
#include "resmgr.hpp"
#include "scrimage.hpp"
#include	"dialog.hpp"
#include "modex.hpp"
#include "text.hpp"
#include "commmgr.hpp"
#include "comm.hpp"
#include "tenetcomm.hpp"
#include "apires.hpp"
#include "debug.hpp"

#include "game_config.hpp"

#include "bam.hpp"
#include "alldefs.hpp"
#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "makechar.hpp"
#include "option3.hpp"
#include "viewport.hpp"
#include "worldmap.hpp"
#include "jstream.hpp"
#include "savemgr.hpp"
#include "units.hpp"
#include "world.hpp"
#include "winlose2.hpp"
#include "snap.hpp"

#include "smack.h"

#include <signal.h>
#include <execinfo.h>

static void crash_handler(int sig)
{
    void *bt[64];
    int n = backtrace(bt, 64);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\n=== CRASH: signal %d ===", sig);
    backtrace_symbols_fd(bt, n, STDERR_FILENO);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "========================");
    signal(sig, SIG_DFL);
    raise(sig);
}


// WARNING: placing #define NDEBUG higher than app-level HPPs in this file
// will cause significant weirdness.  -Van
#ifdef NDEBUG
//#undef NDEBUG
#endif

#define	COUNT_FRAMES	1

#ifdef COUNT_FRAMES

#include <SDL3/SDL.h>

//--------------------------------------------------------------
// FrameCounter Class
//--------------------------------------------------------------

Debugger BamDebug;

class FrameCounter
{
	public:
		void	Count(void);
		FrameCounter(void);

	private:
		Uint64	time;
		int		count;
};


FrameCounter::FrameCounter()
{
	time = SDL_GetTicks();
	count = 0;
}


void
FrameCounter::Count()
{
	Uint64	newTime = SDL_GetTicks();

	if (newTime > (time+1000))
	{
		time = newTime;
		if (pMono) 
		{	
			pMono->Out("\rframes per sec = %d         ", count);
		}
		count = 0;
	}
	count++;
};

#endif // COUNT_FRAMES

// in eventmgr for debugging
extern int	ki_debugging;

//int debugLine = 0, debugNextLine = 0;
//char *debugFile = "";
//char debugHistory[256][80];
//int32 debugAux1 = 0;

void LoadExitQuote(); //proto

// Because of stupid compiler implementations (the MAC in particular),
// global data that needs to be saved must in a struct.  We cannot count
// on the compiler to put all the initialized and uninitialized data in 
// continguous areas.

BamGlobal::BamGlobal()
{
	altMusicNum = 0;
	fFastCollect = false;
	bGlobal.aiUnitMultiplier = 1;
	bGlobal.aiOveride = 0;
	//story.cpp globals
	storyLine= STORY_NONE;
	legendStart = -1;
	missionsDone = 0;
	curPath = 0; // holds legend path index
	prevChooseSide = 1;

	// tutorial mode
	memset(fTutorialGoals, 0, sizeof(fTutorialGoals));

	versionNum = 1;
	versionSubNum = 99;

	// hash compile-time string for a (virtually) unique version #
	const char	*pTime = __TIME__;
	for(buildID = 0; *pTime; pTime++)
		buildID += (uint16)*pTime;

	//Some rooms can be entered in various 'modes' which effect their startup.
	roomMode = 0;

	replayMap = false;

	//makechar.cpp globals
	curCharId = -1;
	curBodyAnim = 0;
	curFaceAnim = 0;
	curCoverAnim = 0;
	curBanner = 0;
	curCat = G_OTHER;	//gender
	*curName = '\0';

	enemyCharId = -1;
	enemyBodyAnim = 0;
	enemyFaceAnim = 0;
	enemyCoverAnim = 0;
	enemyBanner = 0;
	*enemyName = '\0';

	netWinner = false;
	netDisconnect = false;

	//hall.cpp globals
	memset(curLevelScore,0,sizeof(curLevelScore));
	memset(lastLevelXP,0,sizeof(lastLevelXP));
	curCampaignScore = 0;
	curMaxCampaignScore = 0;
	memset(curXP, 0, sizeof(curXP));
	memset(unitsResearched, 0, sizeof(unitsResearched));
	memset(fTutorialGoals,0,sizeof(fTutorialGoals));

	lawful  = 0;
	neutral = 0;
	chaotic = 0;

	evil   = 0;
	nutral = 0;
	good   = 0;

	//cine.cpp
	cinematic = 15;

	writeOut = true;	//we can write files

  	gWorld = grip{};
	gSnap = grip{};

	// incremented with every ARandom() call, for de-sync tracing
	randGenCalls = 0;

	replaySeed = 0;	// no replay seeds yet
	replaySeed2 = 0;
}

// Save this struct
BamGlobal	bGlobal;

// DON'T SAVE these variables

uint16	saveNum = 0;
uint16	restoreNum = 0;
uint16	netSaveNum = 0;
uint16	netRestoreNum = 0;
int		netSerialNum = 666;
char		saveMessage[SAVE_MESSAGE_MAX] = {0};
char		exitMessage[EXIT_MESSAGE_MAX];
char		exitAuthor[EXIT_AUTHOR_MAX];
Snap	 	*pSnap;

bool	GlobalSave(uint16 state, nlohmann::json& root);

// this save function is used for global data
bool
GlobalSave(uint16 state, nlohmann::json& root)
{
	switch (state)
	{
		case BEFORE_SAVE:
			break;

		case BEFORE_RESTORE:
			if (bGlobal.gBam) {
				/* bGlobal.gBam is a system grip (Context::operator new → newSys)
				   so it survives ClearAllocations() without any special flag. */
			}
			if (pBam) {
				pBam->lRoster.count = 0;       pBam->lRoster.gData = grip{};
				pBam->lRoster.first = 0;       pBam->lRoster.last  = 0;
				pBam->lRoster.next  = 0;
				pBam->lServiceables.count = 0; pBam->lServiceables.gData = grip{};
				pBam->lServiceables.first = 0; pBam->lServiceables.last  = 0;
				pBam->lServiceables.next  = 0;
				pBam->lReceivers.count = 0;    pBam->lReceivers.gData = grip{};
				pBam->lReceivers.first = 0;    pBam->lReceivers.last  = 0;
				pBam->lReceivers.next  = 0;
			}
			if (pGraphMgr) {
				pGraphMgr->saveRestorePalette.lUtils.count = 0;
				pGraphMgr->saveRestorePalette.lUtils.gData = grip{};
				pGraphMgr->saveRestorePalette.lUtils.first = 0;
				pGraphMgr->saveRestorePalette.lUtils.last  = 0;
				pGraphMgr->saveRestorePalette.lUtils.next  = 0;
			}
			break;

		case DURING_SAVE:
		case DURING_RESTORE:
		{
			JsonStream js{root["global"], (state == DURING_SAVE)};

			js.sync(bGlobal.roomMgr.curRoomNum, "curRoomNum");
			js.sync(bGlobal.roomMgr.prevRoomNum, "prevRoomNum");
			js.sync(bGlobal.roomMgr.prevRoomMode, "prevRoomMode");

			js.sync(bGlobal.storyLine, "storyLine");
			js.sync(bGlobal.legendStart, "legendStart");
			js.sync(bGlobal.missionsDone, "missionsDone");
			js.sync(bGlobal.curPath, "curPath");
			js.sync(bGlobal.prevChooseSide, "prevChooseSide");

			js.sync(bGlobal.versionNum, "versionNum");
			js.sync(bGlobal.versionSubNum, "versionSubNum");

			js.sync(bGlobal.roomMode, "roomMode");
			js.sync(bGlobal.aiUnitMultiplier, "aiUnitMultiplier");
			js.sync(bGlobal.aiOveride, "aiOveride");

			js.sync(bGlobal.curCharId, "curCharId");
			js.sync(bGlobal.curBodyAnim, "curBodyAnim");
			js.sync(bGlobal.curFaceAnim, "curFaceAnim");
			js.sync(bGlobal.curCoverAnim, "curCoverAnim");
			js.sync(bGlobal.curBanner, "curBanner");
			js.sync(bGlobal.curCat, "curCat");
			js.syncStr(bGlobal.curName, sizeof(bGlobal.curName), "curName");
			js.sync(bGlobal.enemyCharId, "enemyCharId");
			js.sync(bGlobal.enemyBodyAnim, "enemyBodyAnim");
			js.sync(bGlobal.enemyFaceAnim, "enemyFaceAnim");
			js.sync(bGlobal.enemyCoverAnim, "enemyCoverAnim");
			js.sync(bGlobal.enemyBanner, "enemyBanner");
			js.sync(bGlobal.enemyCat, "enemyCat");
			js.syncStr(bGlobal.enemyName, sizeof(bGlobal.enemyName), "enemyName");
			js.syncArray(bGlobal.curLevelScore,  "curLevelScore");
			js.syncArray(bGlobal.curXP,          "curXP");
			js.sync(bGlobal.curCampaignScore, "curCampaignScore");
			js.sync(bGlobal.curMaxCampaignScore, "curMaxCampaignScore");

			js.sync(bGlobal.lawful, "lawful");
			js.sync(bGlobal.neutral, "neutral");
			js.sync(bGlobal.chaotic, "chaotic");
			js.sync(bGlobal.evil, "evil");
			js.sync(bGlobal.nutral, "nutral");
			js.sync(bGlobal.good, "good");

			js.sync(bGlobal.cinematic, "cinematic");
			js.sync(bGlobal.chooseSide, "chooseSide");

			bGlobal.randGen.Save(state == DURING_SAVE ? DURING_SAVE : DURING_RESTORE, root["randGen"]);
			bGlobal.randGen2.Save(state == DURING_SAVE ? DURING_SAVE : DURING_RESTORE, root["randGen2"]);

			js.syncArray(bGlobal.unitsResearched,     "unitsResearched");
			js.syncArray(bGlobal.lastUnitsResearched, "lastUnitsResearched");
			js.syncArray(bGlobal.fTutorialGoals,      "fTutorialGoals");

			js.sync(bGlobal.altMusicNum, "altMusicNum");

			js.sync(pBam->fPauseWorld, "fPauseWorld");
			js.sync(pBam->fUseFog, "fUseFog");
			js.sync(pBam->fUseWinLose, "fUseWinLose");
			js.syncStr(pBam->scenarioName, sizeof(pBam->scenarioName), "scenarioName");
			js.sync(pBam->playerSide, "playerSide");
			js.sync(pBam->fNoIntro, "fNoIntro");
			js.syncArray(pBam->sideColors,          "sideColors");
			js.syncArray(pBam->objAchieved,         "objAchieved");
			js.syncArray(pBam->unitsCreated,        "unitsCreated");
			js.syncArray(pBam->unitsLost,           "unitsLost");
			js.syncArray(pBam->enemiesSlain,        "enemiesSlain");
			js.syncArray(pBam->structuresDestroyed, "structuresDestroyed");
			js.syncArray(pBam->sitesControlled,     "sitesControlled");
			js.syncArray(pBam->playerTypes,         "playerTypes");
			js.sync(pBam->totalFoundations, "totalFoundations");
			if (!js.saving) pBam->voiceChains = 0;
			js.sync(pBam->language, "language");

			// Phase 3: drive World state
			if (js.saving) {
				if (pWorld)
					pWorld->Save(DURING_SAVE, root["world"]);
			} else {
				if (bGlobal.roomMgr.curRoomNum == BR_WORLD) {
					new World;
					pWorld->Save(DURING_RESTORE, root["world"]);
					bGlobal.roomMgr.curRoom  = pWorld;
					bGlobal.roomMgr.gCurRoom = bGlobal.gWorld;
					bGlobal.roomMgr.newRoom  = 0;
				}
			}

			break;
		}

		case AFTER_SAVE:
			break;

		case AFTER_RESTORE:
			pBam = ADerefAs(BAM_Application, bGlobal.gBam);
			pBam->squib1.Reset();
			bGlobal.gSnap = grip{};
			pSnap = nullptr;
			break;
	}

	return false;
}


//--------[ main ]------------------------------------------------

// global pointer for DESTROY_MGR
void *pDestMgr;
// This NULLs the manager pointer before calling (to avoid problems)
#define	DESTROY_MGR_CAREFUL(type, mgr)	if(mgr) {pDestMgr = (mgr); (mgr) = nullptr; delete ((type *) pDestMgr); }
// This doesn't nullptr.  Dangerous, but required for a few managers
#define	DESTROY_MGR(mgr)	if(mgr) { delete (mgr); (mgr) = nullptr; }

#define MIN_MEM_REQ	4000000

BAM_Application		*pBam;
World						*pWorld = nullptr;
TComm						*pComm = nullptr;

void
ReportFreeMem(void)
{
	#ifndef NDEBUG
	char	string1[80];
	int	loop1, int1, int2;

	snprintf(string1, sizeof(string1), "\nAvail() %luk  Largest() %luk  HeapCheck() ",
		pMemMgr->AvailMem() / 1024, pMemMgr->LargestAlloc() / 1024);
	pMono->Out(string1);
	BamDebug.Out(string1);

	pMemMgr->HeapCheck();
	pMono->Out("OK\n");
	BamDebug.Out("OK\n");

	if(pWorld)
	{
		strcpy(string1, "UNITLIB: ");
		for(loop1 = 0, int1 = 0; loop1 < TOTAL_SIDES; loop1++)
		{
			int2 = pWorld->unitLib.lUnits[loop1].count;
			int1 += int2;
			snprintf(string1 + strlen(string1), sizeof(string1) - strlen(string1), "[%d]%d  ", loop1, int2);
		}
		strcat(string1, "\n");
		pMono->Out(string1);
		BamDebug.Out(string1);

		int1 = pWorld->vPort.highestAnim;
		for(loop1 = 0, int2 = 0; loop1 <= int1; loop1++)
			if(pWorld->vPort.gAnims[loop1])
				int2++;

		snprintf(string1, sizeof(string1), "VPORT: highest %d(of %d)  total %d  empty %d\n",
			int1, GANIMS_MAX, int2, int1 - (int2 - 1));
		pMono->Out(string1);
		BamDebug.Out(string1);
	}

	pMono->Out("ResMgr memory useage:\n");
	pMono->Out("RES_ANIM %dk\n", pResMgr->ReportUseage(RES_ANIM));
	pMono->Out("RES_DAC %dk\n", pResMgr->ReportUseage(RES_DAC));
	pMono->Out("other   ???k\n");
	pMono->Out("total %dk\n", pResMgr->ReportUseage(RES_LAST));
	#endif
}

extern void             MouseHandler( int draw_mouse );

// from TEXT.CPP
extern int	squibLanguageNum;
int
main(int argc, char** argv)
{
   char     mess[100], *string1;
   uint16  	saverResult;
	int		loop1, loop2, framesRun, framesPerSec = 0;
	bool		fWaitToSend;
   BAM_WorldEnderPopup     *pWEPop;
	FILE	*pFile;

//	Nothing		*pNothing1 = new Nothing;
//	NothingMore	*pNothingMore1 = new NothingMore;
//	Dummy			*pDummy1 = new Dummy(0x1234);

//	pNothing1->SetValue(0x1234);
//	pNothingMore1->SetValue(0x5678);
//	pDummy1->SetValue(0x90ab);

// Unit	*pUnit = new Unit(true);
//	Unit	unit1(true);

/*	Jay - here's the code for the test case you said you wanted to try next:

	pFile = fopen("unit1.dat", "wb");
	fwrite(&unit1, 1, sizeof(unit1), pFile);
	fclose(pFile);
*/
	
/*
	pFile = fopen("unit1.dat", "rb");
	fread(&unit1, 1, sizeof(unit1), pFile);
	fclose(pFile);
*/

	time_t	startTime, timeDif, lastTime;
	time(&startTime);
	lastTime = startTime;

	#ifndef NDEBUG
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "&ReportFreeMem==0x%08X", (int)ReportFreeMem);
	#endif

	// short-circuit any command line args
	//	argc = 1;

	signal(SIGSEGV, crash_handler);
	signal(SIGABRT, crash_handler);
	signal(SIGBUS,  crash_handler);

	// check for -v before the uppercasing loop so we catch it early
	for(int vi = 1; vi < argc; vi++)
		if(!strcmp(argv[vi], "-v") || !strcmp(argv[vi], "-V"))
			g_verbose = true;

	if(argc > 1 && (!memcmp(argv[1], "?", 2) || !memcmp(argv[1], "-HELP", 6)))
	{
//		printf("Format: BAM [mapNum] [-NOINTRO] [-NOFOG] [-SIDEx] [-NOWIN] [-NET] [-SHOWOFF]\n");
		return(0);
	}

	// if the seed is the same, the sequence will repeat. cool!
//	ASeedRandom(41);			// for repeatability of bugs
//	ASeedRandom2(41);			// for repeatability of bugs
	ASeedRandom(startTime);	// for genuine randomness
	ASeedRandom2(startTime);	// for genuine randomness

	// If you ever don't want to see unfreed memory, like for demos,
	// then uncomment this.
//	extern bool	fPrintUnfreedPtrs;
//	fPrintUnfreedPtrs = false;

	pMono = new Mono;
	// pMono is heap-allocated outside the grip system; not affected by ClearAllocations.

	pMono->SetWindow(0, 7, 79, 12);
	pMono->Clear();
	pMono->Out("Monochrome output initialized\n");
	pMono->Out("EventMgr initializing\n");
	new EventMgr;
	// we are going to update ticks when we want to.
	AAutoUpdateTicks(false);

	pMono->Out("ContextMgr initializing\n");
	new ContextMgr;
	pMono->Out("ResMgr initializing\n");
	new ResourceMgr(true);

	// NOTE - IT IS HIGHLY RECOMMENDED THAT YOU INIT THE SOUND MANAGER
	// BEFORE THE GRAPH MANAGER.  If you don't, streamed sounds will
	// stop and start during restore because of the busy loop in graph
	// manager that happens when the palette fades up.
	pMono->Out("SoundMgr initializing\n");
	new SoundMgr;
	pSoundMgr->Init();
	bGlobal.fFastCollect = LoadGameConfig().fast_collect;

	pMono->Out("GraphMgr initializing\n");
	pGraphMgr = new GraphicsMgr(MODEX_320X400);

	pMono->Out("%dk free after platform initialization\n", AAvailMem() / 1024);

//	Debugger debug1, debug2;

//	debug1.OpenWindow(0, 0, 79, 10);
//	debug2.OpenWindow(40, 11, 79, 21);
//	for(loop1 = 0; loop1 < 25; loop1++)
//	{
//		pMono->Out("\n%d", loop1);
//	}
//	sleep(2);

	pFontMgr = new FontMgr;
	// default font
	pFontMgr->SetRes(9050);
	// default font  color
	pFontMgr->ForeColor(TEXT_DEFAULT);
	pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;


		new MouseInt;

	if(pMouse->hideCount == 999)
	{
		// init error
		ShutDownSoundMgr();
		APrintUnfreedPtrs(false);
		exit(1);
	}
	pMouse->Init(0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1);
	pMouse->SetRes(RES_ANIM, POINTER_RES, 1);
	pMouse->Hide(); //let bamroom show it at room change time.

	// this is our global data saver
	AtSave(GlobalSave);

	// create the application instance
	pBam = new BAM_Application;
	bGlobal.gBam = pBam->gSelf;
	pBam->msgMask = E_MOUSE_DOWN | E_MOUSE_UP | E_KEY_DOWN;

	// command-line options parsed here
	memcpy(pBam->scenarioName, "9110", 5);	// default scenario
	pBam->fNoIntro = false;
	pBam->fDefaultScenario = false;
	pBam->fUseFog = true;
	pBam->fUseWinLose = true;
	pBam->playerSide = SIDE1;
	pBam->fMapEdit = false;
	pBam->playerTypes[SIDE0] = PLAYER_NONE;
	pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
	pBam->playerTypes[SIDE2] = PLAYER_NONE;
	pBam->playerTypes[SIDE3] = PLAYER_NONE;
	pBam->playerTypes[SIDE4] = PLAYER_NONE;

	for(loop1 = 1; loop1 < argc; loop1++)
	{
		strcpy(mess, argv[loop1]);
		string1 = mess;
		do
		{
			*string1 = (char) toupper(*string1);
			string1++;
		}	while(*string1);

		if(mess[0] == '-')
		{
			if(!memcmp(mess, "-NOINTRO", 9))		// turn off opening cinematic
				pBam->fNoIntro = true;
			else if(!memcmp(mess, "-NOWIN", 6))	// turn on win/lose conditions
				pBam->fUseWinLose = false;
			else if(!memcmp(mess, "-NOFOG", 6))		// turn off fog
				pBam->fUseFog = false;
			else if(!memcmp(mess, "-NET", 4))		// network play
			{
				pBam->fNetworkTest = true;
				bGlobal.storyLine = NETGAME;
				// RNG seed is exchanged in TEnetComm::Connect(); parse sub-argument.
				// Note: mess is uppercased; use argv[loop1+1] (original case) for IP.
				const char* netArg = (loop1 + 1 < argc) ? argv[loop1 + 1] : nullptr;
				if (netArg && netArg[0] != '-') {
					loop1++;
					char argLower[256];
					strncpy(argLower, netArg, 255);
					argLower[255] = '\0';
					for (char* p = argLower; *p; ++p) *p = (char)tolower((unsigned char)*p);
					if (!strncmp(argLower, "host", 4)) {
						TEnetComm::s_is_host = true;
						const char* colon = strchr(argLower + 4, ':');
						if (colon) TEnetComm::s_port = (uint16_t)atoi(colon + 1);
					} else {
						TEnetComm::s_is_host = false;
						const char* colon = strchr(netArg, ':');
						if (colon) {
							size_t iplen = (size_t)(colon - netArg);
							if (iplen > 255) iplen = 255;
							strncpy(TEnetComm::s_remote_host, netArg, iplen);
							TEnetComm::s_remote_host[iplen] = '\0';
							TEnetComm::s_port = (uint16_t)atoi(colon + 1);
						} else {
							strncpy(TEnetComm::s_remote_host, netArg, 255);
							TEnetComm::s_remote_host[255] = '\0';
						}
					}
				} else {
					TEnetComm::s_is_host = true;  // bare -NET defaults to host
				}
			}
			else if(!memcmp(mess, "-MUSIC", 6))
			{
				bGlobal.altMusicNum = atoi(mess + 6);
			}
			else if(!memcmp(mess, "-AIUNITS", 8))
			{
				bGlobal.aiUnitMultiplier = atoi(mess + 8);
			}
			else if(!memcmp(mess, "-AI", 3))
			{
				bGlobal.aiOveride = atoi(mess + 3);
			}
			else if(!memcmp(mess, "-FRENCH", 7))
			{
				SetLanguage(LANG_FRENCH);
			}
			else if(!memcmp(mess, "-GERMAN", 7))
			{
				SetLanguage(LANG_GERMAN);
			}
			else if(!memcmp(mess, "-ENGLISH", 8))
			{
				SetLanguage(LANG_ENGLISH);
			}
			else if(!memcmp(mess, "-SHOWOFF", 8))	// storefront demo mode
			{
				bGlobal.storyLine = SHOW_OFF;
			}
			else if(!memcmp(mess, "-HACKRES", 8))	// allow single resource replacement
			{
				pResMgr->fSearchStuffsFirst = false;
			}
			else if(!memcmp(mess, "-SIDE", 5))	// set player side (1 or 2)
			{
				switch(mess[5])
				{
					case '0':
						pBam->playerSide = SIDE0;
						pBam->playerTypes[SIDE1] = PLAYER_NONE;
						pBam->playerTypes[SIDE2] = PLAYER_NONE;
						break;

					case '1':
						pBam->playerSide = SIDE1;
						pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
						pBam->playerTypes[SIDE2] = PLAYER_NONE;
						break;

					case '2':
						pBam->playerSide = SIDE2;
						pBam->playerTypes[SIDE1] = PLAYER_NONE;
						pBam->playerTypes[SIDE2] = PLAYER_LOCAL;
						break;

					default:
						pMono->Out("Invalid option to command line arg -SIDEx\n");
						break;
				}
			}
		}
		else if(atoi(argv[loop1]))
		{
			memcpy(pBam->scenarioName, argv[loop1], 8);
			pBam->fDefaultScenario = true;
		}
	}

	InitStoryDone();

	// initialize game start,
	pBam->Activate(true);

	pContextMgr->msgMask = E_MOUSE_DOWN | E_MOUSE_UP | E_KEY_DOWN;

	#ifdef COUNT_FRAMES
		FrameCounter	fps;
	#endif // COUNT_FRAMES

	new TCommMgr;

	if(pBam->fNetworkTest)	// if set up network from command line
	{
		printf("%s on port %d...\n",
		       TEnetComm::s_is_host ? "Waiting for connection" : "Connecting",
		       (int)TEnetComm::s_port);
		fflush(stdout);

		TEnetComm* pEnet = new TEnetComm();
		if (pCommMgr->Init(pEnet, 0) != TComm::ALL_OK) {
			fprintf(stderr, "TEnetComm::Init() failed\n");
			exit(1);
		}
		pComm = pEnet;  // set global so netchar.cpp's pComm->ClearError() works
		if (pCommMgr->Connect() != TComm::ALL_OK) {
			fprintf(stderr, "TEnetComm::Connect() failed\n");
			exit(1);
		}

		// Apply the RNG seed exchanged during Connect()
		ASeedRandom(TEnetComm::s_rng_seed);
		ASeedRandom2(TEnetComm::s_rng_seed);

		// Assign sides, matching original ConnectComm() logic:
		// lower user ID → SIDE1, higher → SIDE2
		int me = pCommMgr->GetUserID();
		uint16 others[2];
		pCommMgr->GetUserList(others);
		if (me < others[0]) {
			pBam->playerSide        = SIDE1;
			pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
			pBam->playerTypes[SIDE2] = me;
		} else {
			pBam->playerSide        = SIDE2;
			pBam->playerTypes[SIDE2] = PLAYER_LOCAL;
			pBam->playerTypes[SIDE1] = me;
		}

		printf("Connected. Local side: %s\n",
		       (pBam->playerSide == SIDE1) ? "SIDE1 (host)" : "SIDE2 (client)");
		fflush(stdout);
	}

	pMono->Out("TIGRE engine initialized\n");
	ReportFreeMem();

	// side1 and side2 MUST have a player setting of some kind
	if(pBam->playerTypes[SIDE1] == PLAYER_NONE)
		pBam->playerTypes[SIDE1] = PLAYER_COMPUTER;
	if(pBam->playerTypes[SIDE2] == PLAYER_NONE)
		pBam->playerTypes[SIDE2] = PLAYER_COMPUTER;

	// initial room
	if(pBam->fDefaultScenario)
	{
		SetDefaults(pBam->scenarioName);
		bGlobal.roomMgr.NewRoom(BR_WORLD);			// skip everything, go to game
	}
	else if(bGlobal.storyLine == NETGAME)
		bGlobal.roomMgr.NewRoom(BR_NET_CHAR);		// go to net character selection
	else if(bGlobal.storyLine == SHOW_OFF)
	{
		bGlobal.roomMgr.NewRoom(BR_WORLD);			// skip everything, go to game
	}
	else if(pBam->fNoIntro)
		bGlobal.roomMgr.NewRoom(BR_MENU);			// skip intro, go to story selector
	else bGlobal.roomMgr.NewRoom(BR_CINE);		// skip nothing

	bGlobal.roomMgr.CheckRoomChange();

	
	ticks_t	currTicks = ATicks();
	int		snapShot = 0;

	//=======================================================================
	//MAIN GAME LOOP
	//=======================================================================

	ReportFreeMem();

	enum timerTags {TIMER_CYC_PER_FRAME = 0, TIMER_UPDATE_TICKS, TIMER_CONTEXT_CYCLE,
		TIMER_ANIMATE, TIMER_SYNC_SEND, TIMER_SYNC_RECV, TIMER_MAX};
	DebugTimer	timers[TIMER_MAX];

	while (!pContextMgr->fQuitting)
	{
		#ifndef NDEBUG
		timers[TIMER_CYC_PER_FRAME].Start();
		#endif

		// our own exceptional ptrs
		pBam = ADerefAs(BAM_Application, bGlobal.gBam);
		if (bGlobal.gWorld)
		{
			pWorld = ADerefAs(World, bGlobal.gWorld);
		}
		else
		{
			pWorld = nullptr;
		}

		bGlobal.roomMgr.Cycle();

		//fps.Count();

		
      //
      // MDB - Modified the mouse handler.
      //
      MouseHandler( false );

		#ifndef NDEBUG
		//mem_check();
		#endif

		bGlobal.roomMgr.CheckRoomChange();

      //
      // MDB - Modified the mouse handler.
      //
      MouseHandler( false );

		//if snap is on lets handle tick updates ourself	
		if(bGlobal.gSnap && pSnap->snapOn)
		{
			//if snap just toggled on
			if(!snapShot)
			{
				snapShot++;
				currTicks = ATicks();
			}
			else
			{
				currTicks += 2;
			}
			ASetTicks(currTicks);
		}
		else
		{
			snapShot = 0;	//reset

			// add local actions to actionPool2
			pEventMgr->PublishNext();

	      //
   	   // MDB - Modified the mouse handler.
      	//
	      MouseHandler( false );

			if(pWorld)
			{
				// wait for next tick to occur, because otherwise there's no point.
				// note: UpdateTicks() cannot be trusted to change, in case of Pause()

				currTicks = ATicks();
				#ifndef NDEBUG
				timers[TIMER_UPDATE_TICKS].Start();
				#endif
				if(bGlobal.storyLine == NETGAME)
				{
					// Pace to real-time: spin until a tick fires (so the game runs at
					// the configured speed). Deadline guards against a paused clock.
					// Keep rendering during the wait so the cursor stays smooth.
					Uint64 deadline = SDL_GetTicks() + 2 * (1000 / TICKS_PER_SEC);
					UpdateTicks();
					while(ATicks() <= currTicks && SDL_GetTicks() < deadline)
					{
						MouseHandler(false);
						pGraphMgr->Animate();
						UpdateTicks();
					}
					// Force exactly +1 per frame for RNG determinism across both machines
					// (covers both normal advance and paused-clock fallthrough).
					ASetTicks(currTicks + 1);
				}
				else
				{
					UpdateTicks();

					// did the tick counter fail to increment?
					if(ATicks() == currTicks)
					{
						// loop on clock() delta, then resume.  A tick will have changed by
						// then or else we're in Pause() mode, so who cares.

						Uint64	currClock = SDL_GetTicks();
						while(currClock == SDL_GetTicks())
						{
					      MouseHandler( false );
						};

						// try again - if it fails again, assume Pause() and don't worry
						UpdateTicks();
					}

					// if we skipped over a tick somehow..
					if(ATicks() > currTicks + 1)
					{
						// dont allow the tick counter to increment by more than 1
						// tick per frame, or we may get out of sync with the
						// remote machine
						ASetTicks(currTicks + 1);
					}
				}
				#ifndef NDEBUG
				timers[TIMER_UPDATE_TICKS].Stop();
				#endif

				// sync1 - send actionPool2 to remote
				#ifndef NDEBUG
				timers[TIMER_SYNC_SEND].Start();
				#endif

				fWaitToSend = false;
				if(pCommMgr && pCommMgr->totalPacketsWaiting)
					fWaitToSend = true;

				if(!pWorld->SyncSend())
				{
			      MouseHandler( false );
					//pWorld->AITakeOver();
					fprintf(stderr, "[NET] bam.cpp SyncSend() returned false, frame=%d netDisconnect=%d\n", pWorld->currFrame, (int)bGlobal.netDisconnect);
					if(!pBam->fWorldEnderPopupExists)
					{
						pWEPop = new BAM_WorldEnderPopup;
						pWEPop->Setup();
						pBam->fWorldEnderPopupExists = true;
					}
				}
				#ifndef NDEBUG
				timers[TIMER_SYNC_SEND].Stop();
				#endif

				// process actionPool1
				pWorld->ProcessActions();
		      MouseHandler( false );
			}
			else
			{
				#ifndef NDEBUG
				timers[TIMER_UPDATE_TICKS].Start();
				#endif

				UpdateTicks();
				#ifndef NDEBUG
				timers[TIMER_UPDATE_TICKS].Stop();
				#endif
			}

			if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
			{
				pCommMgr->EnQueueData();
		      MouseHandler( false );
			}
		}

		#ifndef NDEBUG
		timers[TIMER_CONTEXT_CYCLE].Start();
		#endif
		pContextMgr->Cycle();
		#ifndef NDEBUG
		timers[TIMER_CONTEXT_CYCLE].Stop();
		#endif
      MouseHandler( false );

		if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		{
			pCommMgr->EnQueueData();
	      MouseHandler( false );
		}

		#ifndef NDEBUG
		timers[TIMER_ANIMATE].Start();
		#endif
		pGraphMgr->Animate();
		#ifndef NDEBUG
		timers[TIMER_ANIMATE].Stop();
		#endif

      MouseHandler( false );

		if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		{
			pCommMgr->EnQueueData();
   	   MouseHandler( false );
		}

		pSoundMgr->Cycle();

		// current frame now done

		// gather cmds from remote for next frame, then prepare to run it
		if(pWorld)
		{
			pWorld->currFrame++;

			// sync2 - add remote actions to local actions in actionPool2
			#ifndef NDEBUG
			timers[TIMER_SYNC_RECV].Start();
			#endif
			if(!pWorld->SyncReceive())
			{
   		   MouseHandler( false );
				//pWorld->AITakeOver();
				fprintf(stderr, "[NET] bam.cpp SyncReceive() returned false, frame=%d netDisconnect=%d\n", pWorld->currFrame, (int)bGlobal.netDisconnect);
				if(!pBam->fWorldEnderPopupExists)
				{
					pWEPop = new BAM_WorldEnderPopup;
					pWEPop->Setup();
					pBam->fWorldEnderPopupExists = true;
				}
			}

			// sync3 - swap action pools
			pWorld->SwapActionPools();
			#ifndef NDEBUG
			timers[TIMER_SYNC_RECV].Stop();
			#endif
		}

		if(bGlobal.gSnap && pSnap->snapOn)
		{
			pSnap->SnapScreen();
		}

		if(netRestoreNum || netSaveNum)
		{
			//this means we're in a netgame save or restore
			//we don't run the eventmgr to keep both sides in sync
			//and we set the restore number here, after the context cycle,
			//to insure the otherside has had time to be notified of this
			if(netRestoreNum)
				restoreNum = netRestoreNum;	//our one net save game

			if(netSaveNum)
			{
				saveNum = netSaveNum;
				netSerialNum = ARandom(99999);
				snprintf(saveMessage, sizeof(saveMessage),"%d",netSerialNum);
			}
		}

      if (saveNum)
      {
         pMemMgr->Dump((uint16) (100 + saveNum), "Save Dump", true);
         saverResult = saveMgr.Save((uint16) saveNum, bGlobal.versionNum,
				bGlobal.versionSubNum, bGlobal.buildID, saveMessage);
         if (saverResult)
         {
            snprintf(mess, sizeof(mess), "save failed.  error #%d", saverResult);
            APanic(mess);
         }
			netSaveNum = 0;	//reset this after save
         saveNum = 0;    
      }
      if (restoreNum)
      {
         saverResult = saveMgr.Restore((uint16) restoreNum, bGlobal.versionNum,
				bGlobal.versionSubNum, bGlobal.buildID);
         if (saverResult)
         {
//         	snprintf(mess, sizeof(mess), "restore failed.  error #%d", saverResult);
				BamDebug.Out("Restore failed!  Error %d\n", saverResult);
//         	APanic(mess);
         }
         pMemMgr->Dump((uint16) (200 + restoreNum), "Restore Dump", true);
			netRestoreNum = 0;	//reset this after restore
         restoreNum = 0; 
			bGlobal.roomMgr.Cycle();

			// frantic temporary debugging measure
			pBam = ADerefAs(BAM_Application, bGlobal.gBam);
			if (bGlobal.gWorld)
			{
				pWorld = ADerefAs(World, bGlobal.gWorld);
				{ nlohmann::json dummy; pWorld->Save(AFTER_RESTORE, dummy); }
			}
			else
			{
				pWorld = nullptr;
			}
			bGlobal.roomMgr.Cycle();
			pSoundMgr->Cycle();

			if(pWorld)
			{
				pBam->fPauseWorld = true;	//force ResumeTicks() to be called in PauseWorld
				pWorld->Pause(false, true);
				pBam->PauseWorld();
			}
			else bGlobal.roomMgr.curRoom->Pause(false, true);
   	   MouseHandler( false );
      }

		if(pWorld)
			if(pBam->fPauseWorld != pWorld->fIsPaused)
				pBam->PauseWorld();

		#ifndef NDEBUG
		timers[TIMER_CYC_PER_FRAME].Stop();
		#endif

		#ifndef NDEBUG
		pMono->SaveWindow();
		pMono->Goto(24, 1);
		pMono->Out("CpF%5d UTick%5d CmC%5d GmA%5d Tx%4d:%dp@%4db]%c Rx[%4d]",
			timers[TIMER_CYC_PER_FRAME].duration, timers[TIMER_UPDATE_TICKS].duration,
			timers[TIMER_CONTEXT_CYCLE].duration, timers[TIMER_ANIMATE].duration,
			timers[TIMER_SYNC_SEND].duration, pCommMgr->pComm->totalPacketsSent,
			pCommMgr->pComm->totalBytesSent,
			fWaitToSend? 'W': 'w', timers[TIMER_SYNC_RECV].duration);
		pMono->Goto(25, 1);
		time(&timeDif);
		if(timeDif != lastTime)
		{
			lastTime = timeDif;
			framesPerSec = AMin(framesRun, 99);
			framesRun = 0;
		}
		else framesRun++;
		timeDif -= startTime;

		pMono->Out("%05dkt %05dkl %02d:%02d:%02d %2dFPS", pMemMgr->AvailMem() / 1024,
			pMemMgr->LargestAlloc() / 1024, timeDif / 3600, (timeDif / 60) % 60,
			timeDif % 60, framesPerSec);

		if(pWorld)
		{
			pMono->Goto(25, 32);
			pMono->Out("r%d g%d 0x%08x", pWorld->tileResNum,
				ALoad(RES_TILELIB, pWorld->tileResNum),
				AGetResData(ALoad(RES_TILELIB, pWorld->tileResNum)));
		}

		pCommMgr->pComm->totalBytesSent = 0;
		pCommMgr->pComm->totalPacketsSent = 0;
		pMono->RestoreWindow();
		#endif

      MouseHandler( false );
	}

	//===================================
	LoadExitQuote();

	//===================================

	// get rid of our current room
	bGlobal.roomMgr.DeleteCurRoom();

	// shut down communications
	if(pCommMgr)
	{
		pCommMgr->Disconnect();
		pCommMgr->DiscardData();
		delete pCommMgr; //its destructor will delete pComm
		pCommMgr = nullptr;
	}

	if(bGlobal.gSnap)
		ADelete(bGlobal.gSnap);
	ADelete(bGlobal.gBam);

	// all actors should be removed from graphmgr, do one more
	// animate to clear lists.
	pGraphMgr->Animate();

	ADelete(pContextMgr->gSelf);

//	TSound	exitSound;
//	exitSound.Play(667);
//	while(exitSound.IsPlaying())
//		pSoundMgr->Cycle();

	// storm the office!  kill all managers!
	ADelete(pGraphMgr->gSelf);
	ADelete(pFontMgr->gSelf);
	ADelete(pSoundMgr->gSelf);
	ADelete(pResMgr->gSelf);
	ADelete(pEventMgr->gSelf);
	ADelete(pMouse->gSelf);

	delete pMono;

//	DESTROY_MGR_CAREFUL(GraphicsMgr, pGraphMgr);
//	DESTROY_MGR_CAREFUL(Mono, pMono);
//	DESTROY_MGR_CAREFUL(FontMgr, pFontMgr);
//	DESTROY_MGR_CAREFUL(SoundMgr, pSoundMgr);
//	DESTROY_MGR(pResMgr);
//	DESTROY_MGR(pContextMgr);
//	DESTROY_MGR_CAREFUL(EventMgr, pEventMgr);

	SDL_Log("%s", exitMessage);
	SDL_Log("%s", exitAuthor);

	OS_Quit();  // skips static destructors that crash after audio teardown
}


//----------------------------------------------------------
//	BAM_Application
//----------------------------------------------------------

void* BAM_Application::operator new(size_t size)
{
	grip g = pMemMgr->SysMalloc(size);
	return pMemMgr->Deref(g);
}

BAM_Application::BAM_Application()
{
	// BUGBUG - this is always opened, in case certain modules do not
	// have NDEBUG defined
	classID = CID_BAMAPP;
	BamDebug.OpenFile("BAM.DBG");
	BamDebug.Out("BAM status/error log, single run\n");

	// from TEXT.CPP
	language = squibLanguageNum;

	fPauseWorld = false;
	fWorldEnderPopupExists = false;
	saveNum = 0;
	restoreNum = 0;
	fShowTileNums = false;
	sideColors[SIDE1] = BLUE;
	sideColors[SIDE2] = RED;
	fNetworkTest = false;
	language = LANG_DEFAULT;

	memset(objAchieved        ,0,sizeof(objAchieved));
	memset(unitsCreated       ,0,sizeof(unitsCreated));
	memset(unitsLost          ,0,sizeof(unitsLost));
	memset(enemiesSlain       ,0,sizeof(enemiesSlain));
	memset(structuresDestroyed,0,sizeof(structuresDestroyed));
	memset(sitesControlled    ,0,sizeof(sitesControlled));
}

bool
BAM_Application::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&bamAppDataStart, 1, (uintptr_t)&bamAppDataEnd -
				(uintptr_t)&bamAppDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&bamAppDataStart, 1, (uintptr_t)&bamAppDataEnd -
				(uintptr_t)&bamAppDataStart, pFile);
			break;
	}
	return(true);
}

void
BAM_Application::Quit(void)
{
	SubOptionMenu *pSub;

	bGlobal.roomMgr.curRoom->Pause(true);

	pSub = new SubOptionMenu;
	pSub->Setup(grip{},LEAVE_BUTTON);

}


void
BAM_Application::PauseWorld()
{
	// At this point, world has already run its pause method for cosmetic
	// screen changes but we haven't touched the tick counter until now.

	if(fPauseWorld)	// if already on and turning off
	{
		ResumeTicks();
	}
	else	// pause off, and turning on
	{	
		PauseTicks();
	}

	fPauseWorld = pWorld->fIsPaused;
}


bool
BAM_Application::HandleMsg(Message* pMsg)
{
 	BAM_TeleportPopup *pTeleport;


	switch(pMsg->type)
	{
		case MSG_NOTICE:
			switch(pMsg->notice.type)
			{
				case N_QUIT:
					return(true);
			}
			break;

		case MSG_EVENT:
			switch (pMsg->event.type)
			{
				case E_KEY_DOWN:
					switch(pMsg->event.value)
					{
						case K_X:
							#ifndef NDEBUG
							if(pMsg->event.modifiers & MOD_ALT)
								pContextMgr->Quit();
							#endif
							break;

						case K_ESC:
							Quit();
							return(true);

						#ifndef NDEBUG
						case K_Y:
							//restoreNum = 1;
							break;

						case K_F4:
							//one screen capture

							if(!bGlobal.gSnap)
							{
								if(!pWorld || (pWorld && bGlobal.storyLine != NETGAME))
								{
									//lets setup for screen snapping (capture)
									pSnap = new Snap;
									bGlobal.gSnap = pSnap->gSelf;
								}
							}

							//no single shots while snap is toggled on
							if(!pSnap->snapOn)
								pSnap->SingleScreen();

							//delete snap alloc -now elsewhere
							//if(bGlobal.gSnap)
							//{
							//	ADelete(bGlobal.gSnap);
							//	bGlobal.gSnap = 0;
							//}

							return(true);

						//case K_F6:
							////toggle on/off screen snap
							//
							//if(!bGlobal.gSnap)
							//{
							//	//lets setup for screen snapping (capture)
							//	TRACK_MEM("Snap");	pSnap = new Snap;
							//	bGlobal.gSnap = pSnap->gSelf;
							//}
							//
							//if(pSnap->snapOn)
							//	pSnap->StopSnap();
							//else
							//	pSnap->StartSnap();
						//	return(true);

						case K_F10:
							SetFontColors(CI_SKIP,93,90);
							// teleport
							pTeleport = new BAM_TeleportPopup;
							pTeleport->Setup(OPTION_SQB,38);
							return(true);

						case K_F11:
							SetFontColors(CI_SKIP,93,90);
							// RUN CINEMATIC NUMBER?
							//can only use this from main menu
							if(bGlobal.roomMgr.curRoomNum == BR_MENU)
							{
								bGlobal.roomMgr.prevRoomMode = 0;
								bGlobal.roomMgr.newRoomMode = 1;
								pTeleport = new BAM_TeleportPopup;
								pTeleport->Setup(OPTION_SQB,53);
							}
							return(true);

						case	K_PRINT_SCREEN:
							return(true);
							#endif
					}
			}
	}
	return(Context::HandleMsg(pMsg));
}

void
BAM_Application::Restart()
{
}

BAM_Application::~BAM_Application()
{
}

void
BAM_Application::Cycle(void)
{
	Context::Cycle();
}

void
BAM_Application::LaunchVoice(int rSquib1, int cel1, int rSquib2, int cel2, int rSquib3, int cel3)
{
	int		rSound1 = 0, rSound2 = 0, rSound3 = 0;

	if(voiceChains >= MAX_VOICE_CHAINS)
		return;

//	rSound1 = (rSquib1 + language) * 10000 + cel1;
	rSound1 = rSquib1 * 10000 + cel1;
	if(rSound1 && !ALoadDebug(__FILE__, __LINE__, RES_DAC, rSound1, true))
	{
		return;
	}

	//IF there's a second sound, validate and add it
	if(rSquib2 && cel2)
	{
//		rSound2 = (rSquib2 + language) * 10000 + cel2;
		rSound2 = rSquib2 * 10000 + cel2;
		if(rSound2 && !ALoadDebug(__FILE__, __LINE__, RES_DAC, rSound2, true))
		{
			return;
		}
	}

	//IF there's a third sound, validate and add it
	if(rSquib2 && cel2)
	{
//		rSound3 = (rSquib3 + language) * 10000 + cel3;
		rSound3 = rSquib3 * 10000 + cel3;
		if(rSound3 && !ALoadDebug(__FILE__, __LINE__, RES_DAC, rSound3, true))
		{
			return;
		}
	}

	rVoiceChains[voiceChains][0] = rSound1;
	rVoiceChains[voiceChains][1] = rSound2;
	rVoiceChains[voiceChains][2] = rSound3;

//	pMono->Out("Voice %d] %d %d %d\n", voiceChains, rSound1, rSound2, rSound3);
	voiceChains++;
}

TSound *
BAM_Application::FindAvailTSound(void)
{
	TSound	*pSound;
	grip		gSound;
	int		loop1;

	if(pSoundMgr->NumberDigiPlaying() >= MAX_DIGI_SOUNDS)	// if all TSounds in use
	{
		pMono->Out("BAM::FindAvailTSound() - taking oldest.\n");
		gSound = pSoundMgr->OldestDigiPlaying();
		if(!gSound)
			return(0);
		pSound = ADerefAs(TSound, gSound);
		pSound->Stop();
		return(pSound);
	}

	// find first avail TSound
	for(loop1 = 0, pSound = &sounds[0]; loop1 < MAX_DIGI_SOUNDS; loop1++, pSound++)
	 	if(!pSound->IsPlaying())
		{
			pMono->Out("BAM::FindAvailTSound() - allocating [%d]\n", loop1);
			return(pSound);
		}

	pMono->Out("BAM::FindAvailTSound() - SoundMgr->NumberDigiPlaying is wrong!\n");
	return(&sounds[0]);
}

char *
BAM_Application::BuildString(char *pBuf, int sq1, int cel1, int sq2, int cel2, int sq3, int cel3)
{
	*pBuf = '\0';
	char	*pStr;

	if(sq1 && cel1)
	{
		pStr = squib1.Load(sq1, cel1, true);
		if(pStr)
			strcat(pBuf, pStr);
	}
	if(sq2 && cel2)
	{
		if(*pBuf)
			strcat(pBuf, " ");
		pStr = squib1.Load(sq2, cel2, true);
		if(pStr)
			strcat(pBuf, pStr);
	}
	if(sq3 && cel3)
	{
		if(*pBuf)
			strcat(pBuf, " ");
		pStr = squib1.Load(sq3, cel3, true);
		if(pStr)
			strcat(pBuf, pStr);
	}
	return(pBuf);
}

void
LoadExitQuote()
{
	SquibRes		sqbEnd;
	int			quoteNum,maxQuoteNum;
	int			authorNum;
	char			*pTxt;

	maxQuoteNum = atoi(sqbEnd.Load(ENDQUOTE_SQB,1000));
	quoteNum = ((ARandom(maxQuoteNum)+1) * 10) + 1000;

	pTxt = sqbEnd.Load(ENDQUOTE_SQB,quoteNum);
	strcpy(exitMessage,pTxt);

	authorNum = atoi(sqbEnd.Load(ENDQUOTE_SQB,quoteNum+1));
	pTxt = sqbEnd.Load(ENDQUOTE_SQB,authorNum);
	strcpy(exitAuthor,pTxt);
}

// Return random between 0 and range-1
int
ARandomDebug(int range, int line, const char *file)
{
	#ifndef NDEBUG
	BamDebug.Out("#%d@%d ARand(%d)l%d %s\n",
		bGlobal.randGenCalls, ATicks(), range,	line, file);
	#else
	(void)line;
	(void)file;
	#endif
	bGlobal.randGenCalls++;
	return bGlobal.randGen.GetNumber(range);
}

// Return random between 0 and range-1
#ifndef ARandom
int
ARandom(int range)
{
	bGlobal.randGenCalls++;
	return bGlobal.randGen.GetNumber(range);
}
#endif

// Change random seed
void
ASeedRandom(uint32 newSeed)
{
	bGlobal.randGen.ReSeed(newSeed);
}

// Return random between 0 and range-1
int
ARandom2(int range)
{
	return bGlobal.randGen2.GetNumber(range);
}

// Change random seed
void
ASeedRandom2(uint32 newSeed)
{
	bGlobal.randGen2.ReSeed(newSeed);
}
