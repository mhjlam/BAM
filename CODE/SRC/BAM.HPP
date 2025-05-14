//	BAM.hpp
//
//	Copyright 1994, Tachyon, Inc.
//
// Top level of BAM application.
//
//		09-07-94:
//


#ifndef bam_hpp
#define bam_hpp

#include "api.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "fontmgr.hpp"
#include "mono.hpp"
#include "objmgr.hpp"
#include "tigre.hpp"
#include "soundmgr.hpp"
#include "dialog.hpp"
#include "commMgr.hpp"

#include "bamroom.hpp"
#include "tilelib2.hpp"		// tilelib & map structs
//#include "maps.hpp"
#include "bamfuncs.hpp"
#include "world.hpp"

// global constants
const int	PLAYER_LOCAL		= -1;
const int	PLAYER_COMPUTER	= -2;
const int	PLAYER_NONE			= -3;

const uint16	WIND_BACK_COLOR	= 44;
const uint16	WIND_BASE_PRI		= 1000;
const uint16	WIND_BORD_COLOR	= 50;
const uint16	LABEL_FORE_COLOR	= 6;
const uint16	LABEL_BACK_COLOR	= 27;

const uint16	TEXT_DLG				= 1;
const uint16	TEXT_DEFAULT		= 5;
const uint16	TEXT_BLACK			= 0;
const uint16	TEXT_WHITE			= 6;
const uint16	TEXT_OFF_WHITE		= 5;
const uint16	SKIP_COLOR			= 254;

const uint16	CONTROLS_PRI		= 990;
const uint16	DIALOG_PRI			= 1000;

#define	POINTER_RES					9000
#define  CONFIRMATION_ANIS			8997

extern	bool	commCancel;

extern	uint16	saveNum;
extern	uint16	restoreNum;
extern	uint16	netSaveNum;
extern	uint16	netRestoreNum;
extern	int		netSerialNum;

#define	SAVE_MESSAGE_MAX			40
#define	EXIT_MESSAGE_MAX			240
#define	EXIT_AUTHOR_MAX			80
extern	char		saveMessage[SAVE_MESSAGE_MAX];

#define MAX_VOICE_CHAINS	10

//---------[ BAM_Application ]----------------------------------------
//
#define MSGBOX_MAX	1
class BoxMsg : public TMovable
{
	public :
		char text[80];
		uint32	mapX, mapY;
};

// Object to hold various global game objects, and global game methods
class BAM_Application : public Context
{
	public:
		BAM_Application(void);
		~BAM_Application();
		virtual bool	HandleMsg(Message* pMsg), Save(uint16 state, FILE *pFile = NULL);
		virtual void	Cycle(void);
		void		Save(void);
		void		Restore(void);
		void		Restart(void);
		void		GetSaveFiles(void);
		void		Quit(void);
		void		PauseWorld();
		char *	BuildString(char *pBuf, int sq1, int cel1, int sq2, int cel2, int sq3, int cel3);
		void		LaunchVoice(int rSquib1, int cel1, int rSquib2=0, int cel2=0, int rSquib3=0, int cel3=0);
		TSound	*FindAvailTSound(void);

		SquibRes	squib1;
		TSound	sounds[MAX_DIGI_SOUNDS], voice1;

		char	bamAppDataStart;
		bool	fPauseWorld;
	   bool	fWorldEnderPopupExists;
		bool	useTextForDialog, fUseFog, fUseWinLose, fMapEdit, fShowTileNums, fNetworkTest;
		char	scenarioName[9];	// file name - no extension
		uchar	playerSide;	// SIDE1 or SIDE2, only used in World::Setup()
		bool	fNoIntro, fDefaultScenario;
		unitColor_t	sideColors[TOTAL_SIDES];

		int	objAchieved[TOTAL_SIDES];	//holds final count of winning conditions met
		int	unitsCreated[TOTAL_SIDES];
		int	unitsLost[TOTAL_SIDES];//all possible ways to lost a unit -including slain
		int	enemiesSlain[TOTAL_SIDES]; //includes all NPC's slain (except pheasants are a special case)
		int	structuresDestroyed[TOTAL_SIDES];
		int	sitesControlled[TOTAL_SIDES];
		int	playerTypes[TOTAL_SIDES];	// local, computer (AI), or remote ID#
		int	totalFoundations;
		int	rVoiceChains[MAX_VOICE_CHAINS][3], voiceChains;
		int	language;

		char	bamAppDataEnd;
};


class BamGlobal
{
	public:

		BamGlobal();

	BAM_RoomMgr	roomMgr;

	//story.cpp globals
	int	storyLine;
	int	legendStart;
	int	missionsDone;
	int	curPath; // holds legend path index
	int	prevChooseSide;

	uint16	versionNum, versionSubNum, buildID;

	//Some rooms can be entered in various 'modes' which effect their startup.
	int		roomMode;

	bool		replayMap;	//used by LegendOpponent
	int		aiUnitMultiplier, aiOveride;

	//makechar.cpp and netchar.cpp globals
	int		curCharId;
	int		curBodyAnim;
	int		curFaceAnim;
	int		curCoverAnim;
	int		curBanner;
	int		curCat;
	char		curName[140];

	int		enemyCharId;
	int		enemyBodyAnim;
	int		enemyFaceAnim;
	int		enemyCoverAnim;
	int		enemyBanner;
	int		enemyCat;
	char		enemyName[140];

	int		lastEnemyCatNum[15];	//gender catagory
	int		lastEnemyNameNum[15];
	int		lastEnemyFaceAnim[15];

	//for net use
	bool		netWinner;
	bool		netDisconnect;

	//assess.cpp and hall.cpp globals
	int		curLevelScore[16];
	int		lastLevelXP[2];
	int		curXP[TOTAL_SIDES];
	int		curCampaignScore;
	int		curMaxCampaignScore;

	int		lawful,lastLawful;
	int		neutral,lastNeutral;
	int		chaotic,lastChaotic;

	int		evil,lastEvil;
	int		nutral,lastNutral;
	int		good,lastGood;

	//cine.cpp
	int		cinematic;

	//choose.cpp
	int		chooseSide;

	//if this flag is true, then we can write files
	int		writeOut;

	grip		gBam;
	grip		gWorld;
	grip		gSnap;

	TRandomGenerator	randGen;		int	replaySeed;
	int32					randGenCalls;
	TRandomGenerator	randGen2;	int	replaySeed2;
	int		randTestValue;
	bool		fReplayMap;

	bool		unitsResearched[UNITMAXTYPES];
	bool		lastUnitsResearched[2][UNITMAXTYPES];
	bool		fTutorialGoals[TUT_TOTAL_GOALS];

	uint32	antiPiracyCrashFlag;
	int		altMusicNum;	// alt World music, for test purposes
};


extern BAM_Application		*pBam;
extern TCommMgr				*pCommMgr;
extern TComm					*pComm;
extern BamGlobal				bGlobal;

// Van's debug version of ARandom()
//#define ARandom(x) ARandomDebug((x),__LINE__,__FILE__)
int		ARandom(int range);

int		ARandomDebug(int range, int line, char *file);

void		ASeedRandom(uint32 newSeed);
int		ARandom2(int range);
void		ASeedRandom2(uint32 newSeed);

#endif

