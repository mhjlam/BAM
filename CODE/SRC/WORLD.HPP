#ifndef WORLD_HPP
#define WORLD_HPP

#include <time.h>

#include "api.hpp"
#include "tigre.hpp"
#include "scrimage.hpp"
#include "text.hpp"

#include "ai.hpp"
#include "bam.hpp"
#include "bamroom.hpp"
#include "maps.hpp"
#include "items.hpp"
#include "pather.hpp"
#include "units.hpp"
#include "unitlib.hpp"
#include "viewport.hpp"
#include "worldmap.hpp"

#define MAX_WINCONS			4
#define SQUIB_WORLD_MSGS	1100
#define SQUIB_WORLD_RESEARCH 116
#define SQUIB_WORLD_YOUR	 120
#define SQUIB_WORLD_ENEMY	 121
#define SQUIB_WORLD_HASDIED 122
#define SQUIB_WORLD_HASBEENDESTROYED	123
#define SQUIB_WORLD_HASBEENLOST			124
#define SQUIB_WORLD_ISREADY				125
#define SQUIB_WORLD_RESEARCHCOMPLETED	126
#define SQUIB_WORLD_CANNOTATTACK			135
#define SQUIB_WORLD_CANNOTHEAL			136
#define SQUIB_WORLD_CANNOTREPAIR			137
#define SQUIB_WORLD_CANNOTCONVERT		138
#define SQUIB_WORLD_CANNOTNET				139
#define SQUIB_WORLD_STRUCTURESINDANGER	140
#define SQUIB_WORLD_UNITSINDANGER		141
#define SQUIB_WORLD_NOSMOKING				142
#define SQUIB_WORLD_PSYCHO					144
#define SQUIB_WORLD_LOVESICK				145
#define SQUIB_WORLD_AFRAID					146
#define SQUIB_WORLD_STONER					147
#define SQUIB_WORLD_GROUP					148
#define SQUIB_WORLD_ATTACK					149
#define SQUIB_WORLD_DEFENSE				150
#define SQUIB_WORLD_MOVE					151
#define SQUIB_WORLD_DROP					152
#define SQUIB_WORLD_XP						153
#define SQUIB_WORLD_CANNOTBERSERK		154
#define SQUIB_WORLD_CANNOTTELEPORT		155
#define SQUIB_WORLD_CANNOTWARD			156

#define SQUIB_METAPOD		143

#define SQUIB_WORLD_TERRAIN	1140
#define SQUIB_TERRAIN_UNKNOWN	100


// mana adjustments
#define MANA_SACRED_URN		20
#define MANA_SUMMON			40
#define MANA_MEDITATE		1
#define MANA_ACOLYTE_MAX	20
#define MANA_TRANSFORM		20
#define MANA_TRANSFORM_BACK 10
#define MANA_DISBAND			20
#define MANA_BOOST			60
#define MANA_BUILD_SITE		0
#define MANA_TELEPORT		5
#define MANA_WEIRD_WARD		5
#define MANA_CAULDRON		20

// MASON MIX repair value 40, no cost
#define MANA_MAX				300

#define	ACTION_POOL_SIZE	40		// size of action pool

#define PACKET_MAX_ACTIONS	10

#define ATTACK_ALARM_TIMER (TICKS_PER_SEC * 30)

struct BAMActionPacketCel
{
	uint32	unitSerial;
	int32		newAction, coordX, coordY, aux1;
};

struct BAMActionPacket
{
	ticks_t	currTicks;
	int32		frameNum, randTest, currMana, totalActions, aux1;
	struct	BAMActionPacketCel actions[PACKET_MAX_ACTIONS];
};

struct WinningCondition
{
	int32			ownerSide, targetSide, targetCount;
	unitType		unitSpec;
	funcType		structSpec;
	itemType		itemSpec;
	terrainType	terrainSpec;
	TargetType	targetSpec;
	bool			fSatisfied;
	int			rSatisfiedAni, aniX, aniY;
};

enum mainButtons 
{
	MAINBTN_FREEZE = 0, MAINBTN_ENCYCLO, MAINBTN_WINLOSE, MAINBTN_SETTINGS, MAINBTN_QUESTION,
	MAINBTN_SEND, MAINBTN_TOTAL
};

enum transformButtons
{
	TRANSFORM_1 = 0, TRANSFORM_2, TRANSFORM_3, TRANSFORM_4, TRANSFORM_5, TRANSFORM_TOTAL
};

class World : public BAM_Room
{
	public:
							World(void);
							~World();

		virtual void	Setup(void), SetupEditMode(void), ConstructEditMode(void);
		virtual void	Cleanup(void), EndScenario(int winningSide);
		virtual bool	HandleMsg(Message* pMsg);
		virtual void	Cycle(void);
		void				SetupItems(void), SetupUnits(void), GenerateWinLose(void);
		funcType			GetBldgType(unitType unit1, bool fAlternate = FALSE);
		bool				ProcessTileConfig(void), ProcessSceneConfig(int pDefogArray[][4], int *pPreDefogCount),
								ProcessUnitLine(char *configLine, List *plSpots, List *plMaster);
		int				StringToSide(char *string);
		void				DrawBanner(uint32 player);
		void				DrawItemPortrait(itemType item);
		void				SetStatusBoxMode(TargetType), DrawStatusBox(void), DrawUnitStatus(uint32 serialNum,
								bool fMinimum = FALSE), DrawManaGuage(int32 manaCount);
		void				SetVerbButtons(TargetType target, uint32 targetSerial),
								SetTransformButtons(uint32 targetSerial), SetTransformingButtons(uint32 targetSerial);
		void				SetupVerbButtons(void);
		void				DrawAttDefMovGuages(int32 attack, int32 maxAttack, int32 defense, int32 maxDefense,
								int32 movement, int32 maxMovement, bool fShowSign), ClearAttDefMovGuages(void);
		void				DrawHPGuage(int32 currHP, int32 maxHP, int ownerSide), ClearHPGuage(void),
								UpdateHPGuageOnly(int32 currHP, int32 maxHP, int ownerSide),
								UpdateAttDefMoveOnly(Unit *pUnit), DrawGuage(char *szLabel, int curr, int max,
								uint rAniRes, bool fShowSign, int xPos, int yPos, CelHeader *pDestCel);
		void				SetCurrentControl(grip gNewControl);
		virtual void	PauseRoom(bool fState);
		void				Summon(unitType newUnitType, uint32 newX, uint32 newY, uint32 newSide), CheatSummon(unitType unit1);
		void				DropItem(void), UseItem(void), *DerefSerial(uint32, TargetType *targ = NULL);

		void				GoSee(void);
		void				Note(void), NoteSend(void), NoteExit(void), NoteCleanup(void);
		void				MsgBoxAdd(char *msg, uint32 newMapX, uint32 newMapY), MsgBoxDraw(void);
		void				DeselectAllVerbButtons(void);
		int32				GetManaLevel(uint32 side), AdjustManaLevel(uint32 side, int32 manaAdj);
		int				AdjustXP(uint32 side, int xpAdjust);
		bool				StartResearch(unitType subject, int researchSecs);
		void				SetManaLevel(uint32 side, int32 newMana); //, ScanForNewMana(uint32 side);
		bool				SpendMana(int player, int manaNeeded, int tileX, int tileY);
		void				CheckWinLose(int eventX, int eventY);
		bool				LaunchSound(int soundNum, int xLoc = 999, int yLoc = 999,
								bool fLoop = FALSE, int volPercent = 100),
							Announce(int tileX, int tileY, int squibNum1, int cel1,
								int squibNum2 = NULL, int cel2 = NULL,
								int squibNum3 = NULL, int cel3 = NULL);
		void				UpdateAllSoundVolumes(void);
		int				CalcSoundVolAndPan(int soundX, int soundY, int16 *pNewVol, int16 *pNewPan);
		void				AddLocalAction(uint32 targetSerial, action newAction,
								int newX, int newY, int aux1 = NULL),
							AddRemoteAction(uint32 targetSerial, action newAction,
								int newX, int newY, int aux1 = NULL),
							AddAction(uint32 targetSerial, action newAction,
								int newX, int newY, int aux1 = NULL),
							AddCosmeticAni(int rAni, int tileX, int tileY,
								int tileXOff, int tileYOff), ProcessActions(void),
								SwapActionPools(void);
		bool				SyncActionsWithRemote(void), SyncSend(void), SyncReceive(void);
		void				AITakeOver(void);
		void				*ValidateTarget(uint32 serialNum, TargetType *targ = NULL, int targX = -1, int targY = -1);
		void				FakeMouseClick(grip gButton);
		void				AllowMouseDragging(bool fNewState, bool fForce = FALSE);
		void				SetTutorialGoal(tutorialGoals goal, bool fState);


		virtual bool	Save(uint16 state, FILE	*pFile = NULL);

		UnitLib			unitLib;
		ViewPort			vPort;
		Map				map;
		ItemMgr			itemMgr;
		List				lBackgroundAnis;
		Pather			pather;
		AI					ai;
		TMusic			music1;
		SquibRes			squib1;
		Rectangle		rTemp1;

		char				worldDataStart;
		int				currFPS, framesRun;
		int            dragSuspendCnt;
		bool				aiOn;
		TileLibRes		*pTileLib;
		uint				mapResNum, tileResNum;
		bool				fIsPaused, fPauseViaNet;
		grip				MsgBox[4];
		bool				MsgBoxOn;
		grip				gPal;						// TPalette
		grip				gInterfaceScreen;		// Guy
		uint				rISCel, rAcolyteSkillButton, rAcolyteTransformGuage;
//		grip				gISCel;					// DCEL
//		uint				rNumISCel;
		grip				gBox, gBoxText;
		grip				gVerbButton[9];		// Guys, unit action
		grip				gTransformButton[TRANSFORM_TOTAL]; // Guys, unit transformations (to unit or bldg)
		grip				gMainButton[MAINBTN_TOTAL];		// Guys (pause, settings, etc)
		grip				gGoSeeButton, gTransformExitButton;			// Guy
		grip				gNoteButton, gNoteSend, gNoteExit;			// Guys
		grip				gNoteBoxText, gNoteBox, gSpendXPButton;
//		grip				gCurControl;			// currently active controller, if any
		grip				gPortrait;				// status box portrait button
		grip				gItemPortrait;			// Guy
		clock_t			PauseFlashTimer;
		uint32			lastUnitSerialDrawn;
		grip				gWorldMap;				// WorldMap object

		int32				mana[TOTAL_SIDES], lastMana[TOTAL_SIDES], lastManaCel;	// mana for players
		grip				clut[TOTAL_SIDES];
		uint32			playerSide;
		ticks_t			tNextManaBonus, tNextWinCheck, tMinesRelease, tShowOffTimer,
								tUnitAttackTimer, tBldgAttackTimer;

		struct			t_actionPool
		{
			int				total[TOTAL_SIDES], coords[TOTAL_SIDES][ACTION_POOL_SIZE][2],
									actionAux1[TOTAL_SIDES][ACTION_POOL_SIZE];
			action			actions[TOTAL_SIDES][ACTION_POOL_SIZE];
			grip				gTargets[TOTAL_SIDES][ACTION_POOL_SIZE];
		}	actPool1, actPool2, *pCurrPool, *pAltPool;
		bool				fPoolFlip;

		int32				soundPos[MAX_DIGI_SOUNDS][2];
		int32				musicNum;

		WinningCondition winCons[MAX_WINCONS];
		ticks_t			tWorldEnds;
		int				worldEnder;

		int				unitsCreated[TOTAL_SIDES];
		int				unitsLost[TOTAL_SIDES];
		int				unitsSlain[TOTAL_SIDES][TOTAL_SIDES];
		int				structuresDestroyed[TOTAL_SIDES][TOTAL_SIDES];
		int				sitesControlled[TOTAL_SIDES];
		int				componentsCaptured;

		uint32			nextSerialNum, serialNums[MAX_SERIAL_NUMS][2];	// [0] == targetType, [1] == grip
		uint32			GetNextSerialNum(TargetType, grip);
		void				FreeSerialNum(uint32 serialNum);

		struct sPacket				SPacket1;
		char	 sPacketData[512];
		struct BAMActionPacket	actionPacket1;
		struct BamPacketHdr		BPHdr;
		int32		currFrame;

		TargetType		statBoxCurrMode;

		unitType	currResearchSubject;
		int		currResearchPhase;
		ticks_t	tCurrResearchPhaseEnd, tCurrResearchPhaseDuration;
		char		worldDataEnd;
};

extern World	*pWorld;

#endif
