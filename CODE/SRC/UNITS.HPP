// UNITS.HPP
// various creatures, critters, and creepy-crawlies for BAM

#ifndef UNITS_HPP
#define UNITS_HPP

#include "tigre.hpp"
#include "periodic.hpp"

#include "ai.hpp"
#include "alldefs.hpp"
#include "maps.hpp"
#include "pather.hpp"
#include "tilelib2.hpp"
#include "viewport.hpp"

#define AVOID_DISTANCE	5
#define MAX_PATH_SIZE	40		// max number of steps of a path that are stored for use

#define MAX_PACK_SIZE	4

#define DMG_WYRM_FLAME		2
#define DMG_BANSHEE_WAIL	5
#define MASON_MIX_HP			40
#define BOAR_BURGER_HP		10
#define TRANSFORM_TIME	(TICKS_PER_SEC * 20)

#define SQUIB_UNIT_NAMES	1120
#define SQUIB_UNIT_SKILL	1110
#define SKILL_TRANSFER		100
#define SKILL_IMMITATE		101

enum worldMsgs
{
	MSG_ENEMY_DIED = 100, MSG_FRIEND_DIED, MSG_NEUTRAL_DIED, MSG_ENEMY_DESTROYED, MSG_FRIEND_DESTROYED, MSG_ITEM_LOST,
	MSG_COMPONENT_CAPTURED, MSG_NO_MANA, MSG_RANDOM, MSG_TRANSFORM, MSG_EXIT_MENU, MSG_EXIT, MSG_TRANSFORM_INTO,
	MSG_TRANSFER, MSG_ABORT_TRANSFORM, MSG_AI_TAKEOVER, MAX_MSGS
};

enum unitState
{
	ST_NULL = 0, ST_OBELISKING, ST_OBELISK, ST_ATTACKING, ST_WAIT2ATTACK, ST_DYING, ST_DEAD, ST_DISBANDING,
	ST_DISBANDED, ST_BERSERKING, ST_BUILDING, ST_TRANSFORMING
};

// movement types
enum moveType
{
	WALKER = 0, FLIER, SWIMMER, FLOATER, MOVEMAXTYPES
};

enum direction
{
	DOWN = 0, DOWN_RIGHT, RIGHT, UP_RIGHT, UP, UP_LEFT, LEFT, DOWN_LEFT, STANDING, DNULL, MAX_DIRECTIONS
};

// offsets into each set of unit Anims
#define ANIM_WALK		0
#define ANIM_ATTACK	10
#define ANIM_GUARD	20
#define ANIM_IS_HIT	25
#define ANIM_DIES		30
#define ANIM_STONED	31
#define ANIM_PROJECTILE	32
#define ANIM_TO_OBELISK	34
#define ANIM_IMPACT	38
#define ANIM_ATTACK2 40

// misc animations (absolute vals)
#define ANIM_SHADOW_CURSOR		102
#define ANIM_HP_GUAGE			132
#define ANIM_COLLAPSE			302
#define ANIM_FIRE					304
#define ANIM_ARCH_FIRE			305
#define ANIM_BIG_SPLASH			306
#define ANIM_FLAME_SPOUT		312
#define ANIM_WARD_SHATTERS		316
#define ANIM_WARD_SPINS			318
#define ANIM_PORTAL				322
#define ANIM_SPARKY_POOF		334
#define ANIM_CAULDRON_SPLASH  337
#define ANIM_ENCHANTMENT		338
#define ANIM_HARPY_NET			340
#define ANIM_METAPOD				342
#define ANIM_METAPOD_HIT		343
#define ANIM_METAPOD_DIES		344
#define ANIM_SPLASH_FOLIAGE	350
//#define ANIM_SPLASH_MARSH		352
//#define ANIM_SPLASH_SWAMP		354
//#define ANIM_SPLASH_SWAMP		356
#define ANIM_SPLASH_WATER		356
#define ANIM_TRANSFORM			358
#define ANIM_HEAL_REPAIR		360
#define ANIM_WAILED				362
#define ANIM_GENERIC_STONED	364
#define ANIM_GENERIC_UNSTONED	366
#define ANIM_BERSERK				360
#define ANIM_FEAR					360
//#define ANIM_FEAR					372
#define ANIM_CRUSHED				368
//#define ANIM_BERSERK				374
#define ANIM_DOPPLEGANGER_DIES 376
#define ANIM_OBELISK_IS_HIT	378
#define ANIM_OBELISK_DIES		380
#define ANIM_ROC_EGG_HIT		382
#define ANIM_ROC_EGG_DIES		384
#define ANIM_DISBAND				386
#define ANIM_TELEPORT			388
#define ANIM_NET_BREAK			390
#define ANIM_STRUCTURE_HIT		392
#define ANIM_SPLASH_HOTBED		394
#define ANIM_SW_TRIP				396
#define ANIM_MANA_TRANSFER		398
#define ANIM_MANA_RECEIVE		400
#define ANIM_EXPLODE				1840
#define PORTRAIT_DOPPLEGANGER 2562

class Unit : public BAM_Ani
{
	public:
						Unit(bool fJustTesting = FALSE);
						~Unit();
		virtual void SmartCycle(bool fEssentialOnly), CycleUseItem(void), Cleanup(void);
		virtual void SetPos(coord L, coord T);
		virtual void SetTilePos(uint32 newX, uint32 newY, uint32 newXOff = TILE_WIDTH / 2 - 1,
							uint32 newYOff = TILE_HEIGHT / 2 - 1), SetCurrTerrain(terrainType newTerrain);
		virtual void SetTilePosOffset(uint32 newXOff, uint32 newYOff);
		bool			SetType(unitType newType);				// set unit's type
		void			SetSide(uint32 newSide);
		void			SetDir(int32 xDif, int32 yDif), SetDir(direction newDir),
							GetXYOffsets(direction dir, int32 &xOff, int32 &yOff);
		direction	GetDir(int32 xDif, int32 yDif);
		uint32		GetDistanceTo(uint32 xPos, uint32 yPos), GetDistanceTo(void *pTarget, TargetType targType);

		//	PrimaryAction decides what individual actions are performed.  Either can be used.
		void			SetPrimaryAction(action newAction, int32 xPos = -1, int32 yPos = -1),
							SetActionAvailable(action action1, bool fState), ClearLastTripSwitch(void);
		bool			CheckActionAvailable(action action1);
		void			SetAction(action newAction, int32 xPos = -1, int32 yPos = -1),
							StandGuard(bool fNewState);
		void			CheckPrimaryAction(void);
		void			*UpdatePrimaryTarget(void);
		void			SetPrimaryTarget(TargetType targType, void *pTarget,
							int targX = -1, coord targY = -1),
						SetSecondaryTarget(TargetType targType, void *pTarget,
							int targX = -1, coord targY = -1);
		void			*ValidatePrimaryTarget(void), *ValidateSecondaryTarget(void);	
		
		void			ChooseNextTarget(void), PeekNextTarget(int &nextX, int &nextY),
							AddToPath(int8 *pPathArray, int x, int y, bool fInsertAtTop = FALSE);
		bool			IsXYBlocked(int32 xPos, int32 yPos);
		int32			GetMoveRate(void);						// ask unit's speed on current terrain
		bool			ImmuneToSkillFrom(Unit *pSkillUser), ImmuneToItem(itemType item1);
		bool			CanIMoveTo(uint32 xPos, uint32 yPos, bool fCheckForUnits = FALSE);
		bool			IsTrapped(void);
		int32			GetMoveRate(terrainType terrain);	// ask unit's speed on terrain X
		bool			CanHaveItem(itemType newItem = NO_ITEM), AcquireItem(itemType newItem);
		bool			PickupItem(void);		// remove item from current X,Y in world, calls AcquireItem()
		itemType		DropItem(bool fTrade = TRUE);		// drop object held, call LoseItem()
		itemType		LoseItem(void);							// modify personal stats accordingly
		void			UseItem(void);
		bool			ApplyItemEffects(itemType item1);
							// use current item (sometimes called automatically)
		void			SetTracking(bool fNewTracking);
		int32			TakeAHit(BAM_Ani *pAttacker, unitType attacker, uint32 attackerX, uint32 attackerY, 
							int32 damage, bool fSkillDmg = FALSE), ComputeFireDamage(int32 damage);
		int32			HealThyself(grip gHealer, unitType healer, uint32 healVal);
		void			Die(Unit *pAttacker = NULL, unitType specialDeath = NOUNIT), Disband(void), Crush(Unit *pCrusher);
		void			SetupForAttack(void), StopTerrainAni(void), CheckTerrainAni(void);
		void			StartGuardMode(void), CycleGuardMode(void), CycleDeathMode(void), CycleMoveTo(void),
							CycleAttack(void), StartMoveToMode(void), StopMoveToMode(void), CycleDisbandMode(void),
							CycleBerserk(void), CycleHeal(void), EndGuardMode(void), CycleTransformToBldgMode(void),
							CycleFlameAttack(void), CheckForMount(void), CycleScare(void), CycleLure(void),
							CycleWail(void), CycleVenom(void), CycleSteal(void),
							CycleGetStoned(void), EndStonedMode(void), CycleEnchantment(void), CycleHarpyNet(void),
							CycleTurret(void), CycleDropMode(void), CycleBlowBack(void), CycleSendMana(void),
								CycleTransform(void);
		bool			ProtectionFromAttacksBy(unitType attacker), ImmuneToAttacksBy(unitType attacker);
		unitType		GetTransformationType(funcType bldgFunction), Transform(unitType newType = ACOLYTE);
		bool			FindNearestPrimaryTarget(void),	WithinRangeForPrimary(void *pTarget = NULL,
							action action1 = NO_ACTION, int tempX = -1, int tempY = -1), SearchForItems(void),
							SearchForItems(int &itemX, int &itemY);
		Unit 			*SearchForEnemies(void), *SearchForPatients(void);
		MapSpace 	*SearchForBuildings(bool fEnemy, bool fSwitches = FALSE);
		void			SearchForEnemiesPreset(Unit *pUnit);
		bool			DoIHate(Unit *pUnit), DoIHate(MapSpace *pSpace);
		uint32		FindPathTo(coord xPos, coord yPos, int8 *pathArray, bool fStopAdjacent,
							bool fThroughFriendlies = TRUE, bool fBulldozer = FALSE, int *pFinalX = NULL,
							int *pFinalY = NULL, bool fThroughStillFriendlies = FALSE), GetPathLength(int8 *pathArray);
		funcType		FindNearbyTransformer(MapSpace **pTransformer = NULL);
		void			CheckForTransformer(void), EnterMineShaft(void), BeAfraidOf(grip gScarer);							
		void			FindALeader(void), BecomeALeader(void), AddFollower(Unit	*pFollower), UpdateAllFollowers(void),
						UpdateFollower(Unit *pFollower), RemoveFollower(Unit *pFollower);
		bool			FollowTheLeader(void);
		virtual void RunAnimation(animState_t newAnimState, int32 newSpeed = 0, bool fNewCycle = FALSE,
							direction newDir = DOWN, bool fImmediate = TRUE, bool fNewFwd = TRUE);
//		void			SetRider(Unit *pNewRider), UpdateRider(void), MountMoved(void);
		void			GroupSelection(bool fState);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char			unitDataStart;
		grip			gGroupCursor;

  		unitType		type, metaPodType;		// index into UnitLib[] for grip to unitGroup
//		bool			fDoppleganger;

		moveType		movement;
		uint32		player;
		// current (item-modified) && regular stats
		int32			currAttack, baseAttack, currRange, baseRange, currDefense, baseDefense, currMove, baseMove;
		int32			currHP, currMana, maxHP, skillRange, skillCost, skillType;
		bool			fSkillNeedsTarget;
		itemType  	item;					// item in possession, if any
		grip			glBackpack;			// bunch of items (for brigand)
		terrainType currTerrain;		// what we're currently standing on
		bool			fTerrainAni;		// is there an environmental ani for curr terrain?
		grip			gTerrainAni;					// environmental ani for current terrain
		grip			gAuxAni;
		unitType		unitSpecialDeath;
		funcType		nearbyTransformer;		// function of nearby transformer building (FUNC_MISC if none)

		uint32		portAnim;
		action		currAction, primaryAction, defendTargetAction;
		int32			actionsAvail;

		// used so PrimaryAction knows when to re-think
		bool			fInAction, fActionDone, fActionEnding, fPrimarySatisfied,
							fStandGuard, fKamikaze;

		int32			unitWidth, unitHeight;
	
		// used for step-by-step movement
 		int32			targetTileX, targetTileY, targetTileXOff, targetTileYOff;
		int32			stepToXOff, stepToYOff;
		int32			currStep;		// 20 steps per tile-to-tile move
		bool			fReturnToCenter, fEnteringMineShaft, fStopAdjacent;

		int32			primaryX, primaryY, primaryXOff, primaryYOff, lastPrimaryX, lastPrimaryY;
		int32			secondaryX, secondaryY, secondaryXOff, secondaryYOff;
		uint32		secondaryTargetSerial, primaryTargetSerial, defendTargetSerial, defendTargetX, defendTargetY,
							killerSerial;
		TargetType	secondaryTargetType, primaryTargetType, defendTargetType;

		direction	currDir, lastDir, wanderDir;
		ticks_t		tNextStateCheck, tBerserkTimer, tEnemyPortalTimer, tFearLureStonedTimer, tAutoActionTimer;
		unitState	currState;
		char			unitName[20];
		grip			glFollowers;		// if we're a leader - grip list of units in our pack
		bool			fPackLeader, fPackFollower, fAutoControl;
		uint32		packLeaderSerial, searchForEnemiesPresetSerial, riderSerial, mountSerial, mountingSerial,
							prisonerSerial;

		// pather stuff
		int8			pathArray[MAX_PATH_SIZE * 2];
		int32			curPathPosition, pathLength;
		int32			pathDestX, pathDestY;

		ticks_t		tUnitBirth, tPrimaryActStarted, tCurrActStarted, tLastSkillTicks;

		// stuff for AI
		ai_unit_data	ai;

		uint32		serialNum;

		uint32		lastAttackerSerial;						// who last attacked?
		ticks_t		lastAttackTicks, lastOurAttackTicks, lastOurAttackTicks2;// and when?
		char			unitDataEnd;

		private:
		bool			CanIMoveToSingle(uint32 xPos, uint32 yPos, bool fCheckForUnits = FALSE);
};

class Projectile : public BAM_Ani
{
	public:
						Projectile(Unit *pNewOwner);
						~Projectile();
		virtual void Launch(uint32 newTargetSerial, int32 targX, int32 targY, int32 targXOff, int32 targYOff,
							int32 newStepX, int32 newStepY, int32 newDelay, int32 newDamage);
		virtual void SmartCycle(bool fEssentialOnly);
		virtual void RunAnimation(int32 newFPS, bool fNewCycle, int32 baseAnim, direction newDir, bool fNewFwd);
		virtual bool Save(uint16 state, FILE *pFile = NULL);

		char			projectileDataStart;
		int32			launchX, launchY, damage;
		uint32		targetSerial, ownerSerial;
		unitType		ownerType;
		char			projectileDataEnd;
};
#endif
