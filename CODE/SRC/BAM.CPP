//
//	Copyright 1994, Tachyon, Inc.
//
// Top level of BAM application.
//
//		09-07-94:
//

#include <stdio.h>
#include <conio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "apidlg.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "dpmi.hpp"
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
#include "apires.hpp"
#include "debug.hpp"

#include "BAM.HPP"
#include "alldefs.hpp"
#include "bamfuncs.hpp"
#include "bampopup.hpp"
#include "makechar.hpp"
#include "option3.hpp"
#include "viewport.hpp"
#include "worldmap.hpp"
#include "Savemgr.hpp"
#include "units.hpp"
#include "world.hpp"
#include "winlose2.hpp"
#include "snap.hpp"

#include "smack.h"
#include "last2e.h"

//
// MDB - Define this to use old mouse interrupt driver.
//
//#define       OLD_MOUSE_INTERRUPT_HANDLER             1


// WARNING: placing #define NDEBUG higher than app-level HPPs in this file
// will cause significant weirdness.  -Van
#ifdef NDEBUG
//#undef NDEBUG
#endif

#define	COUNT_FRAMES	1

#ifdef COUNT_FRAMES

#include	<time.h>

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
		clock_t	time;
		int		count;
};


FrameCounter::FrameCounter()
{
	time = clock();
	count = 0;
}


void
FrameCounter::Count()
{
	clock_t	newTime = clock();

	if (newTime > (time+CLOCKS_PER_SEC))
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
	bGlobal.aiUnitMultiplier = 1;
	bGlobal.aiOveride = 0;
	//story.cpp globals
	storyLine= STORY_NONE;
	legendStart = -1;
	missionsDone = 0;
	curPath = 0; // holds legend path index
	prevChooseSide = 1;

	// tutorial mode
	memset(fTutorialGoals, NULL, sizeof(fTutorialGoals));

	versionNum = 0;
	versionSubNum = 93;

	// hash compile-time string for a (virtually) unique version #
	char	*pTime = __TIME__;
	for(buildID = 0; *pTime; pTime++)
		buildID += (uint16)*pTime;

	//Some rooms can be entered in various 'modes' which effect their startup.
	roomMode = 0;

	replayMap = FALSE;

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

	netWinner = FALSE;
	netDisconnect = FALSE;

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

	#ifdef INTERACTIVE_DEMO
	writeOut = FALSE;
	#else
	writeOut = TRUE;	//we can write files
	#endif

  	gWorld = 0;
	gSnap = 0;

	// incremented with every ARandom() call, for de-sync tracing
	randGenCalls = 0;

	antiPiracyCrashFlag = 0x9A3FCD50;

	replaySeed = 0;	// no replay seeds yet
	replaySeed2 = 0;
}

// Save this struct
BamGlobal	bGlobal;

// DON'T SAVE these variables

bool		commCancel = FALSE;
uint16	saveNum = 0;
uint16	restoreNum = 0;
uint16	netSaveNum = 0;
uint16	netRestoreNum = 0;
int		netSerialNum = 666;
char		saveMessage[SAVE_MESSAGE_MAX] = {0};
char		exitMessage[EXIT_MESSAGE_MAX];
char		exitAuthor[EXIT_AUTHOR_MAX];
Snap	 	*pSnap;

bool	GlobalSave(uint16 state, FILE *fp);

// this save function is used for global data
bool
GlobalSave(uint16 state, FILE *fp)
{
	bool		error = FALSE;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
			// save all global data

			if (fwrite(&bGlobal, 1, sizeof(BamGlobal), fp) != sizeof(BamGlobal))
			{
				// some sort of error
				error = TRUE;
				break;
			}
			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			break;

		case DURING_RESTORE:
			// restore all global data

			if (fread(&bGlobal, 1, sizeof(BamGlobal), fp) != sizeof(BamGlobal))
			{
				// some sort of error
				error = TRUE;
				break;
			}
			break;

		case AFTER_RESTORE:
			if(bGlobal.gSnap)
				pSnap = ADerefAs(Snap,bGlobal.gSnap);
			break;
	}

	return error;
}


//--------[ main ]------------------------------------------------

// global pointer for DESTROY_MGR
void *pDestMgr;
// This NULLs the manager pointer before calling (to avoid problems)
#define	DESTROY_MGR_CAREFUL(type, mgr)	if(mgr) {pDestMgr = (mgr); (mgr) = NULL; delete ((type *) pDestMgr); }
// This doesn't NULL.  Dangerous, but required for a few managers
#define	DESTROY_MGR(mgr)	if(mgr) { delete (mgr); (mgr) = NULL; }

#define MIN_MEM_REQ	4000000

BAM_Application		*pBam;
World						*pWorld = NULL;
TComm						*pComm = NULL;

//int CycleConnection(int isInit);
//
int CycleConnection(int isInit)
{
	static ticks_t endTicks;
	ticks_t	currTicks = ATicks();
	
	if(!isInit)
		endTicks = currTicks + TICKS_PER_SEC * 60;

	pEventMgr->PublishNext();	// needed for TIGRE keyboard management
	UpdateTicks();
	pMono->Out("%3d seconds til timeout    \r", (endTicks - currTicks) / TICKS_PER_SEC);
	if(currTicks > endTicks)
	{
		return(1);
	}
	else return(0);		
}

int __far critical_error_handler(unsigned deverr, unsigned errcode, unsigned far *devhdr)
{
	pMono->Out("** ERROR **\ncritical_error_handler(): deverr=%4.4X errcode=%d devhdr=%Fp\n",
		deverr, errcode, devhdr);
	return(_HARDERR_IGNORE);
}

void
ReportFreeMem(void)
{
	#ifndef NDEBUG
	char	string1[80];
	int	loop1, int1, int2;

	sprintf(string1, "\nAvail() %luk  Largest() %luk  HeapCheck() ",
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
			sprintf(&string1[strlen(string1)], "[%d]%d  ", loop1, int2);
		}
		strcat(string1, "\n");
		pMono->Out(string1);
		BamDebug.Out(string1);

		int1 = pWorld->vPort.highestAnim;
		for(loop1 = 0, int2 = 0; loop1 <= int1; loop1++)
			if(pWorld->vPort.gAnims[loop1])
				int2++;

		sprintf(string1, "VPORT: highest %d(of %d)  total %d  empty %d\n",
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

class Nothing
{
	public:
		virtual void SetValue(int32 value);
		int32 myval;
};

void
Nothing::SetValue(int32 value)
{
	myval = value;
}

class NothingMore : public Nothing
{
	public:
//$$		virtual void SetValue(int32 value);
	virtual int GetValue();
};

int NothingMore::GetValue()
{
return myval;
}

class Dummy : public NothingMore
{
	public:
		Dummy(int32 value);
		virtual ~Dummy() {};

//$$		virtual void SetValue(int32 value);
		
		int32	flag;
};

Dummy::Dummy(int32 value)
{
	SetValue(value);
}

/*void
Dummy::SetValue(int32 value)
{
	flag = value;
}
*/

extern void             MouseHandler( int draw_mouse );

// from TEXT.CPP
extern int	squibLanguageNum;
bool			fPentium = FALSE;
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

// Unit	*pUnit = new Unit(TRUE);
//	Unit	unit1(TRUE);

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

	// disable critical error handler
	_harderr(critical_error_handler);

	AInitializePlatform();

	#ifndef NDEBUG
	printf("\n\n&ReportFreeMem==0x%08X\n", (int)ReportFreeMem);
	#endif

#ifdef OS_DOS
	printf("\nBLOOD & MAGIC Copyright (C) 1996 by Tachyon Studios Inc..\n");
	printf("Developed by Tachyon Studios for Interplay Productions.\n");
   printf("BLOOD & MAGIC, FORGOTTEN REALMS and the TSR LOGO are Trademarks\n");
	printf("owned by TSR, Inc and are used under license.\n");
	printf("\nCompiled %s %s\n", __DATE__, __TIME__);
	printf("%dk free after platform initialization\n", AAvailMem() / 1024);

	if(AAvailMem() < MIN_MEM_REQ)
	{
		printf("\nWARNING: Free XMS memory is low (<%dk)!\nBAM may not run reliably\n",
			MIN_MEM_REQ / 1000);
		sleep(4);
	}

#endif

	// short-circuit any command line args
	//	argc = 1;

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
//	fPrintUnfreedPtrs = FALSE;

	TRACK_MEM("Mono");			pMono = new Mono;
	// if we really must allocate this on the fly instead of
	// having it global, then we must tell the memmgr not to save it.
	ASetSaveStatus(AGetGrip(pMono), FALSE);
	// also, we don't want this to be purged on a restore.
	ASetPurgeStatus(AGetGrip(pMono), FALSE);

	pMono->SetWindow(0, 7, 79, 12);
	pMono->Clear();
	pMono->Out("Monochrome output initialized\n");

	#ifndef NDBEUG
	pMono->Out("&ReportFreeMem==0x%08X\n", (int)ReportFreeMem);
	sleep(4);
	#endif

	pMono->Out("EventMgr initializing\n");
	TRACK_MEM("EventMgr");		new EventMgr;
	// we are going to update ticks when we want to.
	AAutoUpdateTicks(FALSE);

	pMono->Out("ContextMgr initializing\n");
	TRACK_MEM("ContextMgr");	new ContextMgr;
	pMono->Out("ResMgr initializing\n");
	TRACK_MEM("ResMgr");			new ResourceMgr(TRUE);

	// NOTE - IT IS HIGHLY RECOMMENDED THAT YOU INIT THE SOUND MANAGER
	// BEFORE THE GRAPH MANAGER.  If you don't, streamed sounds will
	// stop and start during restore because of the busy loop in graph
	// manager that happens when the palette fades up.
	pMono->Out("SoundMgr initializing\n");
	TRACK_MEM("SndMgr");	new SoundMgr;
	pSoundMgr->Init();

	// BUGBUG! If we bind w/ DOS4GW/Pro we will crash here!
	//setup smacker sound stuff
	SmackSoundUseSOS3((u16)pSoundMgr->hDigiDriverHandle,0);

	pMono->Out("GraphMgr initializing\n");
	TRACK_MEM("GraphMgr");		pGraphMgr = new GraphicsMgr(MODEX_320X400);

	pMono->Out("%dk free after platform initialization\n", AAvailMem() / 1024);

//	Debugger debug1, debug2;

//	debug1.OpenWindow(0, 0, 79, 10);
//	debug2.OpenWindow(40, 11, 79, 21);
//	for(loop1 = 0; loop1 < 25; loop1++)
//	{
//		pMono->Out("\n%d", loop1);
//	}
//	sleep(2);

	TRACK_MEM("FontMgr");		pFontMgr = new FontMgr;
	// default font
	pFontMgr->SetRes(9050);
	// default font  color
	pFontMgr->ForeColor(TEXT_DEFAULT);
	pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;

	#ifdef OS_MAC
//		#include "menubar.hpp"
//		DS9MenuBar*	pMenuBar;
//		TRACK_MEM("menuBar");	pMenuBar = new DS9MenuBar;

		TRACK_MEM("Mouse");	new Mouse;
	#else

		TRACK_MEM("Mouse");	new MouseInt;
	#endif

	if(pMouse->hideCount == 999)
	{
		// init error
		ShutDownSoundMgr();
		APrintUnfreedPtrs(FALSE);
		exit(1);
	}
	pMouse->Init(0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1);
	pMouse->SetRes(RES_ANIM, POINTER_RES, 1);
	pMouse->Hide(); //let bamroom show it at room change time.

	// this is our global data saver
	AtSave(GlobalSave);

	// create the application instance
	TRACK_MEM("BAM App");
	pBam = new BAM_Application;
	bGlobal.gBam = pBam->gSelf;
	pBam->msgMask = E_MOUSE_DOWN | E_MOUSE_UP | E_KEY_DOWN;

	// command-line options parsed here
	memcpy(pBam->scenarioName, "9110", 5);	// default scenario
	pBam->fNoIntro = FALSE;
	pBam->fDefaultScenario = FALSE;
	pBam->fUseFog = TRUE;
	pBam->fUseWinLose = TRUE;
	pBam->playerSide = SIDE1;
	pBam->fMapEdit = FALSE;
	pBam->playerTypes[SIDE0] = PLAYER_NONE;
	pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
	pBam->playerTypes[SIDE2] = PLAYER_NONE;
	pBam->playerTypes[SIDE3] = PLAYER_NONE;
	pBam->playerTypes[SIDE4] = PLAYER_NONE;

	{
		FILE	*pPentiumFile;

		pPentiumFile = fopen("PENTIUM.VAN", "rb");
		if(pPentiumFile)
		{
			fclose(pPentiumFile);
			fPentium = TRUE;
		}
	}

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
//			#ifdef ENABLE_EDITOR
//			if(!memcmp(mess, "-EDIT", 5))				// turn on edit mode
//				pBam->fMapEdit = TRUE;
//			#endif
			
			if(!memcmp(mess, "-NOINTRO", 9))		// turn off opening cinematic
				pBam->fNoIntro = TRUE;
			else if(!memcmp(mess, "-NOWIN", 6))	// turn on win/lose conditions
				pBam->fUseWinLose = FALSE;
			else if(!memcmp(mess, "-NOFOG", 6))		// turn off fog
				pBam->fUseFog = FALSE;
			else if(!memcmp(mess, "-NET", 4))		// network play
			{
				pBam->fNetworkTest = TRUE;
				bGlobal.storyLine = NETGAME;
				ASeedRandom(42);	// need sync'ed RNGs
			}
			else if(!memcmp(mess, "-PENTIUM", 8))
			{
				fPentium = TRUE;
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
				pResMgr->fSearchStuffsFirst = FALSE;
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
			pBam->fDefaultScenario = TRUE;
		}
	}

	#ifdef ENABLE_EDITOR
	if(pBam->fMapEdit)				// if editor mode, then turn off fog
		pBam->fUseFog = FALSE;
		pBam->fShowTileNums = TRUE;
	#endif

	// initialize game start, 
	pBam->Activate(TRUE);

	pContextMgr->msgMask = E_MOUSE_DOWN | E_MOUSE_UP | E_KEY_DOWN;

	#ifdef COUNT_FRAMES
		FrameCounter	fps;
	#endif // COUNT_FRAMES

	TRACK_MEM("TCommMgr");		new TCommMgr;

	if(pBam->fNetworkTest)	// if set up network from command line
	{
//		TNetwork			*pComm;
//		TModem			*pComm;
		TComm::ERROR	errVal;
		int	int1;

//		pMono->Out("New TCommMgr\n");
//		TRACK_MEM("TCommMgr");		pCommMgr = new TCommMgr;	//it isn't an object -no gSelf

		TRACK_MEM("TNetwork");		pComm = new TNetwork;
//		pComm = new TModem;
//		((TModem*) pComm)->SetPort(3);
//		((TModem*) pComm)->SetBaud(19200);

		#ifdef OS_MAC
		int	commInitArg = 0;	// ignored by DOS/TIGRE
			// player 1 is caller, player two is listener
			// at least for now, using command line args to specify direction
			if (pBam->playerTypes[SIDE1] == PLAYER_LOCAL)
			{
				commInitArg = TCommMacintosh::kATalkCaller;
			}
			else
			{
				commInitArg = TCommMacintosh::kATalkListener;
			}
			pCommMgr->Init(pComm, commInitArg);
		#else
			pCommMgr->SetUserAbortFn ((pIntFnInt) CycleConnection);
			pCommMgr->Init(pComm);
		#endif

		pMono->Out("Attempting connection...\n");
		int1 = (int)pCommMgr->Connect();
		errVal = (TComm::ERROR)int1;

		if(errVal != TComm::ALL_OK)
		{
			pMono->Out("Main() - error, commMgr->Connect() failed\n");
			sleep(1);
			pBam->fNetworkTest = FALSE;
		}
		else
		{
			pMono->Out("Main() - connection established\n");
			pBam->playerTypes[(pBam->playerSide == SIDE1)? SIDE2: SIDE1] = pCommMgr->GetUserID();
			pMono->Out("User ID==%d\n", pCommMgr->GetUserID());
//			sleep(2);
		}
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
	else if(bGlobal.storyLine == SHOW_OFF)
	{
		bGlobal.roomMgr.NewRoom(BR_WORLD);			// skip everything, go to game
	}
	else if(pBam->fNoIntro)
		bGlobal.roomMgr.NewRoom(BR_MENU);			// skip intro, go to story selector
	else bGlobal.roomMgr.NewRoom(BR_CINE);		// skip nothing

	bGlobal.roomMgr.CheckRoomChange();

	#ifdef OS_MAC
		#include "menubar.hpp"
		DS9MenuBar *pMenuBar;
		TRACK_MEM("menuBar");	MenuBar.Init();
	#endif
	
	ticks_t	currTicks = ATicks();
	int		snapShot = 0;

	// anti-piracy stuff 
	pFile = fopen("HMICARDS.386", "rb");
	if(!pFile)
		bGlobal.antiPiracyCrashFlag &= 0xFFFFF0FF;
	else
	{
		for(loop1 = 0; loop1 < 5; loop1++)
		{
			char string1[128];
			unsigned long lineCheck1, lineCheck2;

			fread(string1, 32, 1, pFile);								// camoflage string
			fread(string1, 128, 1, pFile);							// encrypted string
			fread(&lineCheck1, sizeof(lineCheck1), 1, pFile);		// line checksum

			// verify checksum
			for(lineCheck2 = 0, loop2 = 0; loop2 < 128; loop2++)
				lineCheck2 = lineCheck2 + string1[loop2];
			lineCheck2 ^= 0xF61E22C9;
			if(lineCheck1 != lineCheck2)
			{
				// failure - set the delayed crash flag
				fclose(pFile);
				bGlobal.antiPiracyCrashFlag &= 0xFFFFF0FF;
			}
		}
		fclose(pFile);
	}

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

		#ifdef DEBUG_BIG_NET
			pMono->Out("main loop()..");
		#endif

		// our own exceptional ptrs
		pBam = ADerefAs(BAM_Application, bGlobal.gBam);
		if (bGlobal.gWorld)
		{
			pWorld = ADerefAs(World, bGlobal.gWorld);
		}
		else
		{
			pWorld = NULL;
		}

		#ifdef DEBUG_BIG_NET
			pMono->Out("roomMgr.cycle()..");
		#endif
		bGlobal.roomMgr.Cycle();

		//fps.Count();

		#ifdef DEBUG_BIG_NET
			pMono->Out("memMgr.cycle()..");
		#endif
		pMemMgr->Cycle();
		
		#ifndef OLD_MOUSE_INTERRUPT_HANDLER
      //
      // MDB - Modified the mouse interrupt handler.
      //
      MouseHandler( FALSE );
		#endif

		#ifndef NDEBUG
		//mem_check();
		#endif

		#ifdef DEBUG_BIG_NET
			pMono->Out("roomMgr.CheckRoomChange()..");
		#endif
		bGlobal.roomMgr.CheckRoomChange();

		#ifndef OLD_MOUSE_INTERRUPT_HANDLER
      //
      // MDB - Modified the mouse interrupt handler.
      //
      MouseHandler( FALSE );
		#endif

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

			#ifdef DEBUG_BIG_NET
				pMono->Out("EventMgr->Publish()..");
			#endif

			// add local actions to actionPool2
			pEventMgr->PublishNext();

			#ifndef OLD_MOUSE_INTERRUPT_HANDLER
	      //
   	   // MDB - Modified the mouse interrupt handler.
      	//
	      MouseHandler( FALSE );
			#endif

			if(pWorld)
			{
				// wait for next tick to occur, because otherwise there's no point.
				// note: UpdateTicks() cannot be trusted to change, in case of Pause()

				currTicks = ATicks();
//				#ifndef NDEBUG
//				BamDebug.Out("main() - update tick counter (curr=%d)\n",
//					currTicks);
//				#endif
				#ifndef NDEBUG	
				timers[TIMER_UPDATE_TICKS].Start();
				#endif
				UpdateTicks();

				// did the tick counter fail to increment?
				if(ATicks() == currTicks)
				{
					// loop on clock() delta, then resume.  A tick will have changed by
					// then or else we're in Pause() mode, so who cares.

					clock_t	currClock = clock();
					while(currClock == clock())
					{
						#ifndef OLD_MOUSE_INTERRUPT_HANDLER
				      MouseHandler( FALSE );
						#endif
					};

					// try again - if it fails again, assume Pause() and don't worry
					UpdateTicks();
				}

				// if we skipped over a tick somehow..
				if(ATicks() > currTicks + 1)
				{					
//					#ifndef NDEBUG
//					BamDebug.Out("main() WARNING: ATicks() advanced by %d! - retarding\n",
//						ATicks() - currTicks);
//					#endif

					// dont allow the tick counter to increment by more than 1
					// tick per frame, or we may get out of sync with the
					// remote machine
					ASetTicks(currTicks + 1);
				}
				#ifndef NDEBUG
				timers[TIMER_UPDATE_TICKS].Stop();
				#endif

//				#ifndef NDEBUG
//				BamDebug.Out("ATicks() == %d\n", ATicks());
//				#endif

				// sync1 - send actionPool2 to remote
				#ifndef NDEBUG
				timers[TIMER_SYNC_SEND].Start();
				#endif

				fWaitToSend = FALSE;
				if(pCommMgr && pCommMgr->totalPacketsWaiting)
					fWaitToSend = TRUE;

				if(!pWorld->SyncSend())
				{
					#ifndef OLD_MOUSE_INTERRUPT_HANDLER
			      MouseHandler( FALSE );
					#endif
					//pWorld->AITakeOver();
					if(!pBam->fWorldEnderPopupExists)
					{
						TRACK_MEM("BAM_WorldEnderPopup");       pWEPop = new BAM_WorldEnderPopup;
						pWEPop->Setup();
						pBam->fWorldEnderPopupExists = TRUE;
					}
				}
				#ifndef NDEBUG
				timers[TIMER_SYNC_SEND].Stop();
				#endif

				// process actionPool1
				pWorld->ProcessActions();
				#ifndef OLD_MOUSE_INTERRUPT_HANDLER
		      MouseHandler( FALSE );
				#endif
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
				#ifndef OLD_MOUSE_INTERRUPT_HANDLER
		      MouseHandler( FALSE );
				#endif
			}
		}

		#ifdef DEBUG_BIG_NET
			pMono->Out("contextMgr.cycle()..");
		#endif
		#ifndef NDEBUG
		timers[TIMER_CONTEXT_CYCLE].Start();
		#endif
		pContextMgr->Cycle();
		#ifndef NDEBUG
		timers[TIMER_CONTEXT_CYCLE].Stop();
		#endif
		#ifndef OLD_MOUSE_INTERRUPT_HANDLER
      MouseHandler( FALSE );
		#endif

		if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		{
			pCommMgr->EnQueueData();
			#ifndef OLD_MOUSE_INTERRUPT_HANDLER
	      MouseHandler( FALSE );
			#endif
		}

		#ifdef DEBUG_BIG_NET
		pMono->Out("GraphMgr->Animate()..");
		#endif
		#ifndef NDEBUG
		timers[TIMER_ANIMATE].Start();
		#endif
		pGraphMgr->Animate();
		#ifndef NDEBUG
		timers[TIMER_ANIMATE].Stop();
		#endif

		#ifndef OLD_MOUSE_INTERRUPT_HANDLER
      MouseHandler( FALSE );
		#endif

		if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		{
			pCommMgr->EnQueueData();
			#ifndef OLD_MOUSE_INTERRUPT_HANDLER
   	   MouseHandler( FALSE );
			#endif
		}

		#ifdef DEBUG_BIG_NET
			pMono->Out("soundMgr.cycle()..");
		#endif
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
				#ifndef OLD_MOUSE_INTERRUPT_HANDLER
   		   MouseHandler( FALSE );
				#endif
				//pWorld->AITakeOver();
				if(!pBam->fWorldEnderPopupExists)
				{
					TRACK_MEM("BAM_WorldEnderPopup");       pWEPop = new BAM_WorldEnderPopup;
					pWEPop->Setup();
					pBam->fWorldEnderPopupExists = TRUE;
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
				sprintf(saveMessage,"%d",netSerialNum);
			}
		}

      if (saveNum)
      {
         pMemMgr->Dump((uint16) (100 + saveNum), "Save Dump", TRUE);
         saverResult = saveMgr.Save((uint16) saveNum, bGlobal.versionNum,
				bGlobal.versionSubNum, bGlobal.buildID, saveMessage);
         if (saverResult)
         {
            sprintf(mess, "save failed.  error #%d", saverResult);
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
//         	sprintf(mess, "restore failed.  error #%d", saverResult);
				BamDebug.Out("Restore failed!  Error %d\n", saverResult);
//         	APanic(mess);
         }
         pMemMgr->Dump((uint16) (200 + restoreNum), "Restore Dump", TRUE);
			netRestoreNum = 0;	//reset this after restore
         restoreNum = 0; 
			bGlobal.roomMgr.Cycle();

			// frantic temporary debugging measure
			pBam = ADerefAs(BAM_Application, bGlobal.gBam);
			if (bGlobal.gWorld)
			{
				pWorld = ADerefAs(World, bGlobal.gWorld);
				pWorld->Save(AFTER_RESTORE);
			}
			else
			{
				pWorld = NULL;
			}
			bGlobal.roomMgr.Cycle();
			pMemMgr->Cycle();
			pSoundMgr->Cycle();

			if(pWorld)
			{
				pBam->fPauseWorld = TRUE;	//force ResumeTicks() to be called in PauseWorld
				pWorld->Pause(FALSE, TRUE);
				pBam->PauseWorld();
			}
			else bGlobal.roomMgr.curRoom->Pause(FALSE, TRUE);
			#ifndef OLD_MOUSE_INTERRUPT_HANDLER
   	   MouseHandler( FALSE );
			#endif
      }

		if(pWorld)
			if(pBam->fPauseWorld != pWorld->fIsPaused)
				pBam->PauseWorld();

		#ifdef DEBUG_BIG_NET
			pMono->Out("main loop() done\n");
		#endif

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

		#ifndef OLD_MOUSE_INTERRUPT_HANDLER
      MouseHandler( FALSE );
		#endif
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
		pCommMgr = NULL;
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

	#ifdef INTERACTIVE_DEMO
	DPMI dpmi;
	char *pScreen = (char *)dpmi.RealToProtected(0xB8000000);

	// kludge - remove "Installer" from data
	for(loop1 = 0; loop1 < 9; loop1++)
		MSG_DEMO_ENGL[86 + loop1 * 2] = ' ';

	memcpy(pScreen, (char *)MSG_DEMO_ENGL, 4000);
	while(!kbhit());
	getch();
	#endif
	system("mode co80");	// yeah, mega-cheesy I know.  It's a bug patch. -Van

	printf("\n%s\n",exitMessage);
	printf("%s\n",exitAuthor);

	return 0;						
}


//----------------------------------------------------------
//	BAM_Application
//----------------------------------------------------------

BAM_Application::BAM_Application()
{
	// BUGBUG - this is always opened, in case certain modules do not
	// have NDEBUG defined
	classID = CID_BAMAPP;
	BamDebug.OpenFile("BAM.DBG");
	BamDebug.Out("BAM status/error log, single run\n");

	// from TEXT.CPP
	language = squibLanguageNum;

	fPauseWorld = FALSE;
	fWorldEnderPopupExists = FALSE;
	saveNum = 0;
	restoreNum = 0;
	fShowTileNums = FALSE;
	sideColors[SIDE1] = BLUE;
	sideColors[SIDE2] = RED;
	fNetworkTest = FALSE;
	language = LANG_DEFAULT;

	memset(objAchieved        ,NULL,sizeof(objAchieved));
	memset(unitsCreated       ,NULL,sizeof(unitsCreated));
	memset(unitsLost          ,NULL,sizeof(unitsLost));
	memset(enemiesSlain       ,NULL,sizeof(enemiesSlain));
	memset(structuresDestroyed,NULL,sizeof(structuresDestroyed));
	memset(sitesControlled    ,NULL,sizeof(sitesControlled));
}

bool
BAM_Application::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&bamAppDataStart, 1, (int)&bamAppDataEnd -
				(int)&bamAppDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&bamAppDataStart, 1, (int)&bamAppDataEnd -
				(int)&bamAppDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
BAM_Application::Quit(void)
{
	SubOptionMenu *pSub;

	bGlobal.roomMgr.curRoom->Pause(TRUE);

	TRACK_MEM("SubOptionMenu");	pSub = new SubOptionMenu;
	pSub->Setup(NULL,LEAVE_BUTTON);

/*	//================================================================
	Dialog	*pDialog;
	Rectangle	oldMouseLimits, newMouseLimits;
	grip		gDialog;
	uint32	retVal;

	oldMouseLimits.Copy(&pMouse->mouseLimits);

	TRACK_MEM("QuitDialog");	pDialog = new Dialog(0, 0, DG_DFT_WINDOW, CI_WHITE, CI_BLACK);
	gDialog = pDialog->gSelf;
	pFontMgr->colors[FNT_FORE_COLOR] = CI_BLACK;
	pFontMgr->colors[FNT_BACK_COLOR] = CI_WHITE;
	pDialog->AddLabel(40, 10, "Quit?  REALLY?!");

	pFontMgr->colors[FNT_FORE_COLOR] = CI_WHITE;
	pFontMgr->colors[FNT_BACK_COLOR] = CI_BLACK;

	pDialog->AddButton(10, 30, "No way", 1, NULL, TRUE);
	pDialog->AddButton(70, 30, "Way", 2, NULL, TRUE);
	pDialog->SetPos(20, 20);
	pDialog->Activate(M_MODELESS);

	// move dialog to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)pDialog->gSelf, L_FRONT, NULL);
	
	pDialog->GetWindowRect(&newMouseLimits);
	pMouse->SetLimits(&newMouseLimits);
//	pMouse->SetLimits(&(ADerefAs(Window, pDialog->gWindow)->scrim.rect));

	retVal = pDialog->EventLoop(NULL);
	pDialog = ADerefAs(Dialog, gDialog);
	pDialog->Deactivate();
	ADelete(gDialog);
	
	switch(retVal)
	{
		case 1:
			break;
		case 2:
			pContextMgr->Quit();
			break;
	}

	pMouse->SetLimits(&oldMouseLimits);
*/

//	#define	CONFIRMED	1
//	#define	DENIED		(-1)
//	Dialog	d;
//	int		oldCursor;
//	int		oldHideCount;
//	DS9Guy	quitCel;
//
//	oldCursor = pMouse->GetCel();
//	pMouse->SetAnim(1, ARROW_CUR);
//	oldHideCount = pMouse->ForceShow(0);
//
//	pDS9->clock.Suspend();
//	pFontMgr->foreColor(TEXT_DLG);
//
//	d.Initialize(SkipDrawProc);
//	d.AddButton(55, 30, "      ", DENIED, K_N, TRUE);
//	d.AddButton(-10, 30, "        ", CONFIRMED, K_Y, TRUE);
//	d.SetControlDrawProc(NULL);
//	d.Activate(M_MODAL, DIALOG_PRI);
//
//	quitCel.SetContext(d.gSelf);
//	quitCel.SetAnim(9615, 1);
//	quitCel.SetPos(-110,-30);
//	quitCel.SetPri(CONTROLS_PRI);
//	quitCel.SetRect();
//	quitCel.Setup();
//
//	// approximately centered
//	d.SetPos(260,200);
//
//	pFontMgr->foreColor(TEXT_DEFAULT);
//
//	// synchronous dialog waits for event
//	if (d.EventLoop(NULL) == CONFIRMED)
//	{
//		// delete dynamics
//		Dialog*	pDlg;
//
//		if (cControls.gTurnDlg)
//		{
//			pDlg = ADeref(cControls.gTurnDlg);
//			pDlg->Deactivate();
//			delete pDlg;
//		}
//
//		if (cControls.gLocDlg)
//		{
//			pDlg = ADeref(cControls.gLocDlg);
//			pDlg->Deactivate();
//			delete pDlg;
//		}
//
//		// send quit message to contextMgr
//		APost(N_QUIT, pContextMgr->gSelf);
//	}
//	else
//	{
//		if (pDS9->locationMod.curLocation == LOC_SPACE_1)
//		{
//			pMouse->Hide();
//			pDS9->clock.Resume();
//		}
//		else
//		{
//			pDS9->clock.Resume();
//		}
//	}
//
//	d.Deactivate();
//	pMouse->ForceShow(oldHideCount);
//	pMouse->SetAnim(1, oldCursor);
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
					return(TRUE);
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
							return(TRUE);

						#ifndef NDEBUG
						case K_Z:
							//saveNum = 1;
							if(pMono->suspended)
								pMono->Resume();
							else pMono->Suspend();
							break;

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
									TRACK_MEM("Snap");	pSnap = new Snap;
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

							return(TRUE);

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
						//	return(TRUE);

						case K_F10:
							SetFontColors(CI_SKIP,93,90);
							// teleport
							TRACK_MEM("teleport"); pTeleport = new BAM_TeleportPopup;
							pTeleport->Setup(OPTION_SQB,38);
							return(TRUE);

						case K_F11:
							SetFontColors(CI_SKIP,93,90);
							// RUN CINEMATIC NUMBER?
							//can only use this from main menu
							if(bGlobal.roomMgr.curRoomNum == BR_MENU)
							{
								bGlobal.roomMgr.prevRoomMode = 0;
								bGlobal.roomMgr.newRoomMode = 1;
								TRACK_MEM("telecine"); pTeleport = new BAM_TeleportPopup;
								pTeleport->Setup(OPTION_SQB,53);
							}
							return(TRUE);

						case	K_PRINT_SCREEN:
//							if (!ki_debugging)
//							{
//								#pragma aux ENTER_DEBUG =		\
//									"int	3";
//								ENTER_DEBUG();
//							}
							return(TRUE);
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
	if(rSound1 && !ALoadDebug(__FILE__, __LINE__, RES_DAC, rSound1, TRUE))
	{
		return;
	}

	//IF there's a second sound, validate and add it
	if(rSquib2 && cel2)
	{
//		rSound2 = (rSquib2 + language) * 10000 + cel2;
		rSound2 = rSquib2 * 10000 + cel2;
		if(rSound2 && !ALoadDebug(__FILE__, __LINE__, RES_DAC, rSound2, TRUE))
		{
			return;
		}
	}

	//IF there's a third sound, validate and add it
	if(rSquib2 && cel2)
	{
//		rSound3 = (rSquib3 + language) * 10000 + cel3;
		rSound3 = rSquib3 * 10000 + cel3;
		if(rSound3 && !ALoadDebug(__FILE__, __LINE__, RES_DAC, rSound3, TRUE))
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
			return(NULL);
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
	*pBuf = NULL;
	char	*pStr;

	if(sq1 && cel1)
	{
		pStr = squib1.Load(sq1, cel1, TRUE);
		if(pStr)
			strcat(pBuf, pStr);
	}
	if(sq2 && cel2)
	{
		if(*pBuf)
			strcat(pBuf, " ");
		pStr = squib1.Load(sq2, cel2, TRUE);
		if(pStr)
			strcat(pBuf, pStr);
	}
	if(sq3 && cel3)
	{
		if(*pBuf)
			strcat(pBuf, " ");
		pStr = squib1.Load(sq3, cel3, TRUE);
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
ARandomDebug(int range, int line, char *file)
{
	#ifndef NDEBUG
	BamDebug.Out("#%d@%d ARand(%d)l%d %s\n",
		bGlobal.randGenCalls, ATicks(), range,	line, file);

	bGlobal.randGenCalls++;
	return bGlobal.randGen.GetNumber(range);
	#else
	range = range;
	line = line;
	file = file;
	APanic("ARandomDebug() called w/ #NDEBUG!\n");
	return(0);
	#endif
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
