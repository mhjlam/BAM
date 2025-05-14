//
// AI.HPP
//
// March 27, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
//	AI for Computer controlled units.
//
//----[]-------------------------------------------------------------

#ifndef AI_HPP
#define AI_HPP

#include "tigre.hpp"

// fPrimarySatisfied, AI_TRANSFORM, currFighters < minFighters, CheckWits(),
// but insufficient mana.  Currently 25%
#define AICHEAT_TRANSFORM_NEED_FIGHTERS	0

// !fPrimarySatisfied, AI_TRANSFORM, recycling unit, guarding, near PORTAL,
// but insufficient mana.  Currently 20%
#define AICHEAT_RECYCLE_NEED_MANA			1

// AI_SUMMON, CheckWits(), CheckSpellIQ(), currAcolytes < minAcolytes,
// but insufficient mana.  Currently 33%
#define AICHEAT_SUMMON_NEED_MANA				2

#define AICHEAT_AGGRSKILL_NEED_MANA			3

#define AICHEAT_TOTAL							4

enum	ai_action
{
	AI_NONE = 0,
	AI_GUARD,
	AI_ATTACK_UNIT, 
	AI_ATTACK_BUILDING,
	AI_TRANSFORM, 
	AI_DEFEND_UNIT, 
	AI_DEFEND_BUILDING,
	AI_TRANS_FOUNDATION,
	AI_HEAL,
	AI_GOTO_HOT_SPOT,
	AI_GET_ITEM,
	AI_GATHER_MANA,
	AI_SUMMON,
	AI_RETREAT,
	AI_MAX
};

#define	MAX_ACTIONS	(AI_MAX + 10)

struct ai_unit_data
{
	ai_action	action;
	uint32		data;
	coord			x, y, lastSwitchX, lastSwitchY, lastSwitchPrimaryX, lastSwitchPrimaryY;
};

#include "alldefs.hpp"
#include "maps.hpp"
#include "units.hpp"
#include "viewport.hpp"

struct ai_action_struct
{
	ai_action	action;
	int			actionSpec;
	int			chance;
	int			sideSpec;
	int			computedChance;
};

#define	MAX_ABUILDINGS	5

struct ai_build_entry
{
	coord		x,y;
	uint32	owner;
	grip		gBuilding;
};

struct ai_build_list
{
	int				count;
	ai_build_entry	bData[MAX_ABUILDINGS];
};

enum	ai_transform
{
	AIT_FOUNDATION = 0,
	AIT_ARBOR,
	AIT_BARRACKS,
	AIT_CRYPT,
	AIT_RUNESTONE,
	AIT_TEMPLE,
	AIT_IMM_WELL,
	AIT_ANY_TRANSFORMER,
	AIT_PORTAL,
	AIT_CAULDRON,
	AIT_MAX
};

#define	AI_NUM_SIDES	5

struct ai_table
{
	int	minAcolytes;		// desired minimum number of acolytes 
	int	absMinAcolytes;	// absolute minimum
	int	maxAcolytes;		// dont bother making/maintaining more than this
	int	minFighters, currFighters;		// desired minimum number of tough guys (no aggressive offense otherwise)
	int	currAcolytes;		// current count, no matter what they're doing
	int	manaReserve;		// desired minimum amount of mana stored
	int	portalX, portalY;
};

#define	AI_MAX_GUARD	40

struct ai_guard
{
	grip		gUnit;
	Unit		*pUnit;
	unitType	type;
	int		x,y;
	int		distance;
	itemType	item;
};

#define	AI_MAX_ITEMS	30

enum ai_delay_type
{
	ADT_NONE = 0,
	ADT_BLDG,
	ADT_UNIT
};

struct ai_delay_action
{
	ai_delay_type	gotHitType;
	uint32			gotHitSerialNum;
	ai_delay_type	hitByType;
	uint32			hitBySerialNum;
};

#define	AI_MAX_DELAY	40

// forward declaration
class Unit;

class AI
{
	public:

						AI();
						~AI();

		void			Cycle(int side);	
		void			Setup(int side);
		void			Cleanup(); 

		bool			BumpUnit(Unit *pUnit);

		bool			HitUnit(Unit *pHitUnit, Unit *pAttacker);
		bool			HitBldg(MapSpace *pHitBldg, Unit *pAttacker);
		bool			HitTerrain(Unit *pHitUnit, TerrainUnit *pAttacker);

		// enemy ai checks
		bool			CheckAggression();
		bool			CheckWits();
		bool			CheckCaution();
		bool			CheckTerrainIQ();
		bool			CheckItemIQ();
		bool			CheckSpellIQ();

		void			SetAggression(int aggression);
		void			SetWits(int wits);
		void			SetCaution(int caution);
		void			SetTerrainIQ(int terrainIQ);
		void			SetItemIQ(int itemIQ);
		void			SetSpellIQ(int spellIQ);
		bool			ShouldWeUseSkill(Unit *pUnit, Unit *pTarget = NULL);
		void			ReportUnit(Unit *pUnit, char *pString, bool fLog = FALSE);

		void			AddUnitPref(int side, unitType newType, int percentage);
		uint32		nextAITimer[AI_NUM_SIDES];
		uint32		timerInterval;

		MapSpace*	FindClosestBldg(funcType bldgType, int side,	int orgX, int orgY);
		ai_guard		*ChooseHotSpot(int startX, int startY);
		bool			CheckCheat(int cheatNum);
		int			cheatChances[AICHEAT_TOTAL];

	protected:

		bool					attackNow[TOTAL_SIDES];
		int					attackTimer[TOTAL_SIDES];

		int					actionCount;
		ai_action_struct	actions[MAX_ACTIONS];

		ai_table				aiTable[AI_NUM_SIDES];

		ai_guard				healArray[AI_MAX_GUARD];
		uint32				healCount;

		ai_guard				guardArray[AI_MAX_GUARD];
		uint32				guardCount;

		ai_guard				repairArray[AI_MAX_GUARD];
		uint32				repairCount;

		ai_guard				itemArray[AI_MAX_ITEMS];
		uint32				itemCount;

		ai_guard				hotSpotArray[AI_MAX_GUARD];
		uint32				hotSpotCount;

		uint32				delayedActionCount[TOTAL_SIDES];
		ai_delay_action	delayedActions[TOTAL_SIDES][AI_MAX_DELAY];

		// enemy ai variables
		int		eAggression;
		int		eWits;
		int		eCaution;
		int		eTerrainIQ;
		int		eItemIQ;
		int		eSpellIQ;

		void			AddAIAction(ai_action aiAction, int actionSpec, int sideSpec, int chance),
							DeleteAction(int actionNum), AddToGuardArray(Unit *pUnit), DeleteFromGuardArray(Unit *pUnit),
							ProcessMissionPool(int side), MonitorWinCons(int side), AddToHealArray(Unit *pUnit),
							DeleteFromHealArray(Unit *pUnit), ExamineAllUnits(void), ExamineAllUnitsImportant(void);

		void			TransformUnit(Unit *pUnit, unitType transToType);
		int			transformPrefs[TOTAL_SIDES][UNITMAXTYPES][2], totalTransformPrefs[TOTAL_SIDES];

		int			MapDistance(int x, int y, int destX, int destY);
		MapSpace*	FindRandomBldg(funcType bldgType, int side);
		int			CountUnitsToHelp(int side);
		void			HelpUnit(Unit *pUnit, action newAction, int32 x, int32 y, int helpCount,
							unitType helpUnitType = UNITMAXTYPES);
		void			HelpHealUnit(Unit *pUnit);
		void			HelpRepair(MapSpace *pBldg, Unit *pUnit = NULL);
		bool			AssignUnits(int side, action newAction, int targetX, int targetY, ai_action newAIAction,
							uint32 newAIData, int teamSize, bool fCompromise = FALSE, bool fSneaky = FALSE);
		bool			FindWayPoint(Unit *pUnit, int targetX, int targetY, int *pWayX, int *pWayY),
							IsTeamReady(Unit *pUnit);
		void			AttackUnit(Unit *pUnit, int maxDist = 0xffff);

		void			FixActionPercents();
		int			PickAction(Unit *pSubject = NULL);

		int			BldgFuncToAIEnum(funcType bldgFunc);

		void			AddDelayedAction(int side, ai_delay_type gotHitType, uint32 gotHitSerialNum,
							ai_delay_type hitByType, uint32 hitBySerialNum);

		bool			CanUseItemToHeal(itemType item, Unit *pUnit);

		ai_action_struct*	FindAction(ai_action aiAction);

		int			friendlySide, enemySide;
};

#endif
