//	BAM units (lions, tigres, bears, etc)
//

#include <ctype.h>

#include "apires.hpp"
#include "tigre.hpp"
#include "eventmgr.hpp"
#include "savebase.hpp"

#include "items.hpp"
#include "tilelib2.hpp"
#include "units.hpp"
#include "viewport.hpp"
#include "world.hpp"
#include "ai.hpp"
#include "debug.hpp"

#ifdef NDEBUG
//#undef NDEBUG
#endif

extern World			*pWorld;
extern char 			szFuncNames[][20], pVerbNames[][30];
extern int				offsets[][2];
extern Debugger		BamDebug;
extern bool	fShowCurrentUnitPath;
extern void ShowUnitPath(Unit *pUnit);

#define FURY_DMG_INTERVAL	(TICKS_PER_SEC * 4)

#undef ARandom
#define ARandom(x) ARandomDebugUnit((x), __LINE__, __FILE__, serialNum, unitName)

//char	szSkillNames[UNITMAXTYPES][15] =
//{"",	"TURRET",	"WAIL",	"",	"HEAL",	"", "CONVERT",	"", "",
//"", "REPAIR", "", "", "STONE", "", "", "NET", "",
//"LURE", "", "HEAL", "", "", "", "", "", "",
//"", "", "", "FEAR", "FLAME", "",
//"", "", "", ""};

// pixel offset for each of 20 steps from tile-to-tile
uint32 stepOffSets[TILE_WIDTH][2] =
{ 1, 1, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
	1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 1}; //DOWN-RIGHT

bool	fRepathFlags[MAX_PATH_SIZE] = {
TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE,
TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE
};		// points at which to repath

extern Debugger UnitDebug;
//---------------------------------------
// Unit class

struct t_pathCache
{
	// args to last pather call
	ticks_t	tLastPathCall;
	uint32	sLastUnitToPath;
	int	startX, startY, destX, destY;
	bool	fStopAdjacent, fThroughFriendlies, fBulldozer, fThroughStillFriendlies;
	int8	pathArray[MAX_PATH_SIZE * 2];
} pathCache;

int
ARandomDebugUnit(int range, int line, char *file, uint32 serialNum, char *pUnitName)
{
	if(bGlobal.storyLine == NETGAME)
	{
		Unit	*pUnit = (Unit *)pWorld->DerefSerial(serialNum);
		BamDebug.Out("U(s%d%s)", serialNum, pUnitName);
		if(pUnit)
		{
			BamDebug.Out(" p%s@%d/%d c%s@%d/%d fAD%d ",
				pVerbNames[pUnit->primaryAction], pUnit->primaryX, pUnit->primaryY,
				pVerbNames[pUnit->currAction], pUnit->secondaryX, pUnit->secondaryY,
				pUnit->fActionDone? 1: 0);
		}

		return(ARandomDebug(range, line, file));
	}
	else
	{
		return(random(range));
	}
}

Unit::Unit(bool fJustTesting)
{
 	fKamikaze = FALSE;
	fStandGuard = FALSE;
	gGroupCursor = NULL;
	glBackpack = NULL;
	glFollowers = NULL;
 	ai.lastSwitchPrimaryX = 999;
	ai.lastSwitchPrimaryY = 999;
	lastAttackerSerial = 0;
	lastAttackTicks = 0;
	lastOurAttackTicks = 0;
	lastOurAttackTicks2 = 0;
	serialNum = 0;
	defendTargetAction = NO_ACTION;	// temp storage for "ulterior motive" primary
	unitSpecialDeath = NOUNIT;
	killerSerial = NULL;
	memset(&ai, 0, sizeof(ai_unit_data));
	fPrimarySatisfied = FALSE;
	fActionDone = FALSE;
	fAutoControl = FALSE;					// if a unit is under self-direction
	tLastSkillTicks = 0;
	currState = ST_NULL;
	unitWidth = 0;		unitHeight = 0;	// # of tiles a unit occupies
	// unit riding on us, or that we are riding on
	riderSerial = NULL;		mountSerial = NULL;		mountingSerial = NULL;

	prisonerSerial = NULL;
	if(fJustTesting)
		tUnitBirth = 0;
	else tUnitBirth = ATicks();
	searchForEnemiesPresetSerial = NULL;// unit to auto-find on next SearchForEnemies() regardless of range
	skillRange = 0;							// radius of effect
	fPackFollower = FALSE;					// following a leader?
	packLeaderSerial = NULL;				// that leader's serial#
	fPackLeader = FALSE;						// being followed by others?
	tPrimaryActStarted = 0;
	tCurrActStarted = 0;
	fEnteringMineShaft = FALSE;
	fStopAdjacent = FALSE;
	gAuxAni = NULL;
	tBerserkTimer = NULL;
	tEnemyPortalTimer = NULL;
	tFearLureStonedTimer = NULL;					// when effects of NYMPH/WRAITH skill will end
	tAutoActionTimer = NULL;
	tNextStateCheck = NULL;
	nearbyTransformer = FUNC_MISC;	// no transformer buildings nearby
	unitName[0] = NULL;
	movement = WALKER;
	selfType = UNIT;		// for self-identification
	type = ACOLYTE;
//	fDoppleganger = FALSE;
	player = SIDE0;
	if(fJustTesting)
		player = SIDE0;
	else SetSide(SIDE0);		// force adding to lUnits[side]
	gTerrainAni	= NULL;
	fTerrainAni = FALSE;

	currStep = 0;		// 20 steps per tile-to-tile move
	fReturnToCenter = FALSE;	// if already moving from curr X,Y to next X,Y and then player says move back to
										// curr X,Y, used to reverse step counting

	currAction = NO_ACTION;		// single actions
	primaryAction = NO_ACTION;	// used to choose individual actions for currAction
	actionsAvail = (int)NO_ACTION;
	fInAction = FALSE;			// when Cycle() sees FALSE here, chooses next action from primaryAction
	primaryX = -1;			primaryY = -1;			primaryXOff = 0;		primaryYOff = 0;
	lastPrimaryX = -1;	lastPrimaryY = -1;
	secondaryTargetSerial = 0;

	currAttack = 0;	baseAttack = 0;		// current (item-modified) && normal stats
	currRange = 0;		baseRange = 0;
	currDefense = 0;	baseDefense = 0;
	currMove = 0;		baseMove = 0;
	currHP = 0;			maxHP = 0;
	currMana = 0;
	pathLength = 0;
	curPathPosition = 0;

	currTerrain = UNDEFINED;
	currDir = DNULL, lastDir = DNULL, wanderDir = DOWN;
	tileX = -1;
	tileY = -1;
	baseAnim = 0;
	portAnim = 0;
	secondaryX = 0;	secondaryY = 0;	secondaryXOff = 0;	secondaryYOff = 0;
	primaryTargetSerial = 0;	primaryTargetType = NOTARGET;
	targetTileX = 0;
	targetTileY = 0;	// used if currAction == MOVE_TO
	targetTileXOff = 0;
	targetTileYOff = 0;
	item = NO_ITEM;
	SetTracking(FALSE);		// viewport cursor is not presently locked to us
}

Unit::~Unit()
{
	List	*pList;

	if(glFollowers)
	{
		pList = ADerefAs(List, glFollowers);
		pList->Release();
		ADelete(glFollowers);
		glFollowers = NULL;
	}
	if(glBackpack)
	{
		pList = ADerefAs(List, glBackpack);
		pList->Release();
		ADelete(glBackpack);
		glBackpack = NULL;
	}
	PurgeAnimQeue();
	if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
		pWorld->vPort.DeleteAni(gAuxAni);
	else gAuxAni = NULL;
}
		
bool
Unit::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&unitDataStart, 1, (int)&unitDataEnd -
				(int)&unitDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&unitDataStart, 1, (int)&unitDataEnd -
				(int)&unitDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
Unit::Cleanup(void)
{
	int		loopX, loopY;
	MapSpace	*pSpace;
	Unit		*pUnit;

	WorldMap	*pWM = ADerefAs(WorldMap, pWorld->gWorldMap);

	if(gGroupCursor)
	{
		pWorld->vPort.RemoveUnitFromGroup(serialNum);
	}

	if(type == ACOLYTE && currTerrain == FOUND_GENERIC)
	{
		pSpace = pWorld->map.GetStructure(tileX, tileY);
		if(pSpace->h.func == FUNC_GEN_FOUND && pSpace->fFoundationFilled)
			pSpace->CheckFoundationFilled();
	}

	for(loopY = tileY - unitHeight + 1; loopY <= tileY; loopY++)
		for(loopX = tileX - unitWidth + 1; loopX <= tileX; loopX++)
		{
			// clear spot in grip map
			if(pWorld->unitLib.GetUnitGrip(loopX, loopY) == gSelf)
				pWorld->unitLib.PlaceUnitGrip(NULL, loopX, loopY);
			pWM->Draw(loopX, loopY);
		}

	if(currTerrain == PRESSURE_PLATE)
	{
		pWorld->vPort.SwapMapTile(tileX, tileY);

		pSpace = pWorld->map.GetStructure(tileX, tileY);
		if(pSpace)
			pSpace->RunTrigger();
	}

	if(fTerrainAni)
	{
		StopTerrainAni();
	}

	pWorld->vPort.CutAni(gSelf);				// cut self from animation system
	pWorld->unitLib.CutUnit(gSelf);			// cut self from unit library

	// if carrying a unit, he's out of the system so de-alloc him here
	if(prisonerSerial)
	{
		pUnit = (Unit *)pWorld->DerefSerial(prisonerSerial);
		ADelete(pUnit->gSelf);
	}
}

bool
Unit::PickupItem(void)
{
	BAMItem	*pItem;

	// am I standing on an item?
	pItem = pWorld->itemMgr.CheckItem(tileX, tileY);
	if(!pItem)
		return(FALSE);

	if(!AcquireItem(pItem->type))	// if failed to get item (maybe no hands)
		return(FALSE);

	pWorld->LaunchSound(SND_GET_ITEM, tileX, tileY);
	pWorld->iteiMgr.RemoveItem(tileX, tileY);				// remove item from x,y map

	// are we spotlighted right now?
	if(fCursorTracking || pWorld->lastUnitSerialDrawn == gSelf)
	{
		pWorld->DrawItemPortrait(item);		// then update the status box
		pWorld->DrawStatusBox();
	}
	return(TRUE);
}

bool
Unit::CanHaveItem(itemType newItem)
{
	switch(type)
	{
	 	case HARPY:
			if(newItem == HARPY_NET)
				break;
			return(FALSE);

		case GRIFFIN:
		case JUGGERNAUT:
		case ROC_EGG:
		case ROC:
		case TORTOISE:
		case UNIT_WEIRD_WARD:
		case WOLF:
		case WYRM:
			return(FALSE);		// cant carry any items
	}
	return(TRUE);		
}

bool
Unit::AcquireItem(itemType newItem)
{
	itemType restoreItem = NO_ITEM;

	if(newItem > MAXITEMTYPES)
	{
		UnitDebug.Out("\nUnit::AcquireItem(%d) - invalid item #", newItem);
		return(FALSE);
	}

	if(!CanHaveItem(newItem))
		return(FALSE);

	if(item)
	{
		// already have an item in-hand
		if(type == BRIGAND)
		{
			List	*pList = ADerefAs(List, glBackpack);
			pList->Add((void *)item);
			LoseItem();
		}
		else
		{
			if(pWorld->itemMgr.GetItemUsage(newItem) == ITEM_AUTOMATIC
				&& !pWorld->itemMgr.IsItemReusable(newItem))
	 		{
				restoreItem = item;
				item = NO_ITEM;
			}
			else
			{
//				UnitDebug.Out("\nUnit:: cannot pick up item - already carrying %d", newItem);
				return(FALSE);
			}
		}
	}

	item = newItem;
	if(item)
	{
		SetActionAvailable(DROP, TRUE);
		if(pWorld->itemMgr.GetItemUsage(item) != ITEM_AUTOMATIC)
			SetActionAvailable(USE_ITEM, TRUE);
		else SetActionAvailable(USE_ITEM, FALSE);
	}
	else
	{
		SetActionAvailable(DROP, FALSE);
		SetActionAvailable(USE_ITEM, FALSE);
	}

	// some items affect player when picked up
	if(pWorld->itemMgr.GetItemUsage(item) == ITEM_AUTOMATIC)
	{
		if(bGlobal.storyLine == TUTORIAL)
		{
			if(player == pWorld->playerSide && item == DRIFT_DISC)
			{
				pWorld->SetTutorialGoal(TUT_ITEM_FOUND, TRUE);	
			}
		}
	
		UseItem();

		if(restoreItem)
		{
			item = NO_ITEM;
			// restore previous item
			AcquireItem(restoreItem);
		}
	}
	return(TRUE);
}

void
Unit::UseItem(void)	// use item currently in possession
{
	bool fItemApplied;

	// some units CAN carry items, they just cant use them
	switch(type)
	{
		case FURY:
		case BRIGAND:
			return;
	}

	// only side 1 and 2 can use items
	switch(player)
	{
		case SIDE1:
		case SIDE2:
			break;
		default:
			return;
	}

	switch(item)
	{
		case WEIRD_WAND:
			if(!pWorld->SpendMana(player, MANA_WEIRD_WARD, tileX, tileY))
				return;
			break;

		case TELEPORT_TOME:
			if(!pWorld->SpendMana(player, MANA_TELEPORT, tileX, tileY))
				return;
			gAuxAni = NULL;	// forget about teleport ani - it shouldnt travel with us
			break;
	}

	fItemApplied = ApplyItemEffects(item);
	if(fItemApplied && !pWorld->itemMgr.IsItemReusable(item))
	{
		// these items are one-time
		LoseItem();
	}
}
				
bool
Unit::ApplyItemEffects(itemType item1)
{
	bool			fFail;
	uint32		x1, y1, x2, y2, loop1, loopX, loopY;
	UnitGroup	*pGroup;
	grip			gAni;
	MapSpace		*pSpace;
	Unit			*pUnit;

	if((primaryAction == DIE || primaryAction == DISBAND) && item1 != SACRED_URN)
	{
		// units in one of these states are immune to all items
		return(FALSE);
	}

	if(type == TORTOISE || type == UNIT_WEIRD_WARD)
	{
		// these guys are immune to all items
		return(FALSE);
	}

	if(type == ACOLYTE && currState == ST_OBELISK)
		pGroup = pWorld->unitLib.GetUnitGroup(OBELISK);
	else pGroup = pWorld->unitLib.GetUnitGroup(type);

	switch(item1)
	{
		case NO_ITEM:
			return(FALSE);

		case BOAR_BURGER:
			if(ImmuneToItem(item1))
				return(FALSE);

			HealThyself(gSelf, type, BOAR_BURGER_HP);
			break;

		case HARPY_NET:
			if(gAuxAni)
			{
				return(FALSE);
				// presume already showing net ani
			}
			TRACK_MEM("NetAni");	gAuxAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_HARPY_NET, 1, PRI_PROJECTILE,
				TRUE, TRUE, 3, tileX, tileY, TRUE, tileXOff, tileYOff);
			if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
				pWorld->vPort.CheckAni(gAuxAni);
			else gAuxAni = NULL;
			break;

		case TELEPORT_TOME:
			// for multi-player reasons, fog checking is done in ViewPort::ProcessVerb()
			// and not here.
			if(CanIMoveTo(secondaryX, secondaryY, TRUE))
			{
				SetTilePos(secondaryX, secondaryY);
				if(fTerrainAni)
				{
					// dont want to carry splash anims through a teleport
					StopTerrainAni();
				}

				if(currTerrain == MINE && movement != FLIER)
				{
					// mineshaft must have both mine tile && cluster with FUNC_MINESHAFT
					pSpace = pWorld->map.GetStructure(tileX, tileY);
					if(pSpace && pSpace->h.func == FUNC_MINESHAFT)
						EnterMineShaft();
				}
			}
			else return(FALSE);
			break;

		case WEIRD_WAND:
			if(pWorld->unitLib.GetUnitGrip(secondaryX, secondaryY))
			{
				// whoops!  Someone beat us to it!
				fFail = TRUE;
			}
			else fFail = FALSE;

			pSpace = pWorld->map.GetStructure(secondaryX, secondaryY);
			if(pSpace && pSpace->h.currHP > 0)	// blocked by building
				return(FALSE);

			// find previous WARD (if any) and destroy
			gAni = (grip)pWorld->unitLib.lUnits[SIDE3].FirstValue();
			while(gAni)
			{
				pUnit = ADerefAs(Unit, gAni);
				if(pUnit->type == UNIT_WEIRD_WARD && pUnit->primaryAction != DIE)
				{
					pUnit->Die();
					break;
				}		
				gAni = (grip)pWorld->unitLib.lUnits[SIDE3].NextValue();
			}

			TRACK_MEM("Unit");	gAni = pWorld->unitLib.NewUnit(UNIT_WEIRD_WARD, SIDE3, 
				secondaryX, secondaryY, NO_ITEM, GUARD);
			pWorld->vPort.CheckAni(gAni);
			pWorld->LaunchSound(SND_HEAL_REPAIR, secondaryX, secondaryY);

			if(fFail)
			{
				pUnit = ADerefAs(Unit, gAni);
				pUnit->Die();
			}
			break;
	 		
		case MANA_ORB:
			pWorld->AdjustManaLevel(player, MANA_BOOST);
			break;

		case MAP:	// show a random area of the map
			if(player == pWorld->playerSide)		// only the player cares, not the computer
			{
				// search for a fogged spot
				loop1 = 0;
				do
				{
					x1 = 3 + ARandom2(WORLD_WIDTH - 6);
					y1 = 3 + ARandom2(WORLD_HEIGHT - 6);
				} while(!(pWorld->vPort.fog[y1][x1] & FOG_CENTER) && ++loop1 < 100);

				if(loop1 >= 100)	// if too many tries
				{
					loop1 = 0;
					for(loopY = 0; loopY < WORLD_HEIGHT && loop1 != 100; loopY++, y1 = (y1 + 1) % WORLD_HEIGHT)
						for(loopX = 0; loopX < WORLD_WIDTH && loop1 != 100; loopX++, x1 = (x1 + 1) % WORLD_WIDTH)
							if(pWorld->vPort.fog[y1][x1] & FOG_CENTER)	// if found a spot
							{
								x2 = x1;
								y2 = y1;
								loop1 = 100;	// stop search
							}
					if(loop1 == 100)	// if search successful
					{
						x1 = x2;
						y1 = y2;
					}
					else break;		// no fog to be found
				}
				x2 = AMin(x1 + 6, WORLD_WIDTH - 1);
				y2 = AMin(y1 + 6, WORLD_HEIGHT - 1);
				pWorld->vPort.Defogger(x1, y1, x2, y2);
			}
			else return(FALSE);
			break;

		case BERSERKER_BREW:
			switch(type)
			{
				case FURY:
				case GOLEM:
				case JUGGERNAUT:
				case GUARDIAN:
				case ROC_EGG:
				case TROLL:
				case WRAITH:
					return(FALSE);

				default:
					fAutoControl = TRUE;
					SetPrimaryAction(BERSERK);

					// cause stat changes to be applied
					terrainType oldTerrain = currTerrain;
					SetCurrTerrain(UNDEFINED);
					SetCurrTerrain(oldTerrain);

					TRACK_MEM("BerserkAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_BERSERK, 1,
						priorityLevel + 2, TRUE, FALSE,
						4, tileX, tileY, FALSE);	// poof!
					pWorld->LaunchSound(SND_HEAL_REPAIR, tileX, tileY);
					pWorld->vPort.CheckAni(gAni);
					break;
			}
			break;

		case MASON_MIX:
			if(ImmuneToItem(item1))
				return(FALSE);

			HealThyself(gSelf, type, MASON_MIX_HP);
			break;

		case HEALING_SALVE:
			if(ImmuneToItem(item1))
				return(FALSE);

			HealThyself(gSelf, type, maxHP - currHP);
			break;

		case TRAILFINDER:
			switch(currTerrain)
			{
				case ROUGH_LAND:
				case BROKEN_LAND:
					currMove = baseMove;
					break;

				default:
					return(FALSE);
			}
			break;

		case SHRUB_SPRITE:
			switch(currTerrain)
			{
				case FOLIAGE:
				case BRAMBLES:
					currMove = baseMove;
					break;

				default:
					return(FALSE);
			}
			break;

		case DRIFT_DISC:
			switch(currTerrain)
			{
				case WATER:
				case WHIRLPOOL:
					currMove = baseMove;
					break;

				default:
					return(FALSE);
			}
			break;

		case BOG_BOOTS:
			switch(currTerrain)
			{
				case MARSH:
				case SWAMP:
					currMove = baseMove;	// negates penalties
					break;

				default:
					return(FALSE);
			}
			break;

		case STORM_BRACERS:
			switch(type)
			{
				case WIZARD:		// cant wear 'em
					return(FALSE);

				default:
					currAttack = pGroup->attack[currTerrain] + 1;
					break;
			}
			break;

		case MIGHT_MANTLE:
			currDefense = pGroup->defense[currTerrain] + 1;
			break;

		case SACRED_URN:
			if(primaryAction == DISBAND || primaryAction == DIE)
				pWorld->AdjustManaLevel(player, MANA_SACRED_URN);
			break;
	}
	if(fCursorTracking)
		pWorld->DrawStatusBox();

	// if using item is worth XPs
	if(item1 && (pWorld->itemMgr.GetItemUsage(item1) != ITEM_AUTOMATIC || !pWorld->itemMgr.IsItemReusable(item1)))
		pWorld->AdjustXP(player, XP_USE_ITEM);

	return(TRUE);
}

int searchOffSets[4][2] = {1,0, 0,1, -1,0, 0, -1};
itemType
Unit::LoseItem(void)
{
	Unit	*pUnit;
	int	loop1, newX, newY;
	grip	gAni;
	UnitGroup	*pGroup;
	itemType oldItem;

	#ifndef NDEBUG
	if(type == HARPY)
		BamDebug.Out("LoseItem()..");
	#endif

	if(item == SACRED_URN && (primaryAction == DIE || primaryAction == DISBAND))
	{
		UseItem();
	}
	
	oldItem = item;
	if(type == ACOLYTE && currState == ST_OBELISK)
		pGroup = pWorld->unitLib.GetUnitGroup(OBELISK);
	else pGroup = pWorld->unitLib.GetUnitGroup(type);

	item = NO_ITEM;

	// update curr stats
	terrainType oldTerrain = currTerrain;
	SetCurrTerrain(UNDEFINED);
	SetCurrTerrain(oldTerrain);

	#ifndef NDEBUG
	if(type == HARPY)
		BamDebug.Out("L1");
	#endif

	if(oldItem == HARPY_NET && prisonerSerial)
	{
		// release prisoner from the net
		pUnit = (Unit *)(pWorld->DerefSerial(prisonerSerial));

		// determine if we drop him under us, or beside us
		newX = -1;

		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.1(%d)", (int)currTerrain);
		#endif
		switch(currTerrain)
		{
			case CAULDRON:
				if(pUnit->movement != WALKER)
				{
					// only walkers will drown here
					break;
				}
				// fall-through
			case WATER:
			case CHASM:
			case PRISON_PIT:
				// unit will die
				newX = tileX;
				newY = tileY;
				break;
		}

		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.2");
		#endif
		// if not dropping here
		if(newX == -1)
		{
			// search for an open spot nearby to place unit in
			for(loop1 = 1; loop1 < 5 && newX == -1; loop1++)	// search outward to max radius
			{
				pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, newX, newY, TRUE);
				do
				{
					if(!CanIMoveTo(newX, newY, TRUE))
					{
						newX = -1;
						continue;
					}
					if(!pUnit->CanIMoveTo(newX, newY, TRUE))
					{
						newX = -1;
						continue;
					}
				} while(newX == -1 && pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, newX, newY));
			}
		}

		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.3");
		#endif
		if(pUnit->tBerserkTimer || pUnit->tFearLureStonedTimer)
		{
			// leave fAutoControl true	
		} else pUnit->fAutoControl = FALSE;

		pUnit->currTerrain = UNDEFINED;
		pUnit->currAction = NO_ACTION;
		pUnit->currState = ST_NULL;
		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.4");
		#endif
		pUnit->SetTilePos(newX, newY);
		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.5");
		#endif
		if(pUnit->primaryAction != DIE)
		{
			TRACK_MEM("NetBreakAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_NET_BREAK, 1, 
				PRI_SKY, TRUE, FALSE, 3, newX, newY, FALSE);
			pWorld->vPort.CheckAni(gAni);
		}

//		presumably the net'ing process cleaned up the victim's terrain ani
//		pUnit->StopTerrainAni();		// if any

		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.6");
		#endif
		pUnit->SetSide(pUnit->player);
		pUnit->fInAction = FALSE;
		pUnit->fActionDone = TRUE;
		pUnit->fActionEnding = FALSE;
		pUnit->fInMotion = FALSE;
		pUnit->fAniDone = TRUE;
		pWorld->unitLib.AddUnit(pUnit->gSelf);
		pWorld->vPort.AddAni(pUnit->gSelf);
		pWorld->vPort.CheckAni(pUnit->gSelf);
		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.7");
		#endif
		pUnit->SmartCycle(FALSE);

		// in case unit walked over our grip, restore ourselves.
		// The unit will have died from something anyway.
		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.8");
		#endif
		SetTilePos(tileX, tileY, tileXOff, tileYOff);

		prisonerSerial = NULL;
		SetActionAvailable(SKILL, TRUE);
		SetActionAvailable(ATTACK, TRUE);
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.DeleteAni(gAuxAni);	// net animation
		gAuxAni = NULL;
		oldItem = NO_ITEM;

		#ifndef NDEBUG
		if(type == HARPY)
			BamDebug.Out("L1.9");
		#endif
	}

	SetActionAvailable(DROP, FALSE);
	SetActionAvailable(USE_ITEM, FALSE);
	if(fCursorTracking)
		pWorld->DrawStatusBox();

	#ifndef NDEBUG
	if(type == HARPY)
		BamDebug.Out("L2");
	#endif
	return(oldItem);
}

itemType
Unit::DropItem(bool fTrade)
{
	itemType oldItem, newItem = NO_ITEM;
	bool		fItemPlaced;
	MapSpace	*pSpace;

	#ifndef NDEBUG
	BamDebug.Out("\nDropItem(%s)..", unitName);
	#endif
	oldItem = item;

	if(primaryAction == USE_ITEM)
	{
		// cancel primary, otherwise possible item trade would cause wrong
		// item to be used
		SetPrimaryAction(MOVE_TO, primaryX, primaryY);
	}

	#ifndef NDEBUG
	BamDebug.Out("D2");
	#endif

	if(item)
	{
		// if there is an item already at this X,Y then trade.
		// But BRIGANDs arent allowed to trade.
		if(fTrade && type != BRIGAND && primaryAction != DIE &&
			primaryAction != DISBAND)
			newItem = pWorld->itemMgr.RemoveItem(tileX, tileY);

		// restore our usual settings
		oldItem = LoseItem();

		#ifndef NDEBUG
		BamDebug.Out("D3");
		#endif
		if(oldItem == DRIFT_DISC && currTerrain == WATER && movement == WALKER)
		{
			// re-register current terrain (start the drowning process)
			currTerrain = UNDEFINED;
			SetTilePos(tileX, tileY, tileXOff, tileYOff);
		}

		if(oldItem)
		{
			// place the item we're dropping on the ground

			#ifndef NDEBUG
			BamDebug.Out("D4");
			#endif
			if(currTerrain == CAULDRON && oldItem == BOAR_BURGER)
			{
				// stew like life!  no, life like stew!
				pSpace = pWorld->map.GetStructure(tileX, tileY);
				pWorld->AdjustManaLevel(pSpace->h.owner, MANA_CAULDRON);
				fItemPlaced = FALSE;
			}
			else if((currAction == MOVE_TO && !fReturnToCenter) || type == BRIGAND)
			{
				// we're moving into the current tileX,Y still - dont place the item there, or
				// else we'll just pick it back up again
				#ifndef NDEBUG
				BamDebug.Out("\nPlaceItem %s %d %s", unitName, __LINE__, __FILE__);
				#endif
				fItemPlaced = pWorld->itemMgr.PlaceItem(tileX, tileY, oldItem, 3, TRUE);
			}
			else
			{
				#ifndef NDEBUG
				BamDebug.Out("\nPlaceItem %s %d %s", unitName, __LINE__, __FILE__);
				#endif
				fItemPlaced = pWorld->itemMgr.PlaceItem(tileX, tileY, oldItem, 3);
			}

			#ifndef NDEBUG
			BamDebug.Out("D5");
			#endif
			if(!fItemPlaced)
			{
				pWorld->Announce(tileX, tileY, SQUIB_ITEM_NAMES, oldItem,
					SQUIB_WORLD_MSGS, SQUIB_WORLD_HASBEENLOST);

				if(currTerrain == WATER || currTerrain == CAULDRON)
					pWorld->LaunchSound(SND_SMALL_SPLASH, tileX, tileY);
			}
			else pWorld->LaunchSound(SND_DROP_ITEM, tileX, tileY);
		}
		// else item was destroyed in process of losing it (like HARPY NET), so nothing to drop
	}

	#ifndef NDEBUG
	BamDebug.Out("D6");
	#endif
	if(newItem)		// if we're picking up something new in the process
	{
		AcquireItem(newItem);
		if(fCursorTracking)	// do this here, since LoseItem() un-did it
			pWorld->DrawStatusBox();
	}
	else if(type == BRIGAND)
	{
		List	*plPack = ADerefAs(List, glBackpack);
		if(plPack->count > 0)		// look inside our backpack
		{
			AcquireItem((itemType)plPack->LastValue());
			plPack->Delete((void *)item);
		}

//	if(fCursorTracking)
//		pWorld->DrawStatusBox();	handled by LoseItem() now.
	}

	#ifndef NDEBUG
	BamDebug.Out("done\n");
	#endif
	return(oldItem);
}

// does unit have any adjacent tiles he can move to?
bool
Unit::IsTrapped(void)
{
	int	loopX, loopY, loopX2, loopY2;

	for(loopY = tileY - 1, loopY2 = tileY + 1; loopY <= loopY2; loopY++)
		for(loopX = tileX - 1, loopX2 = tileX + 1; loopX <= loopX2; loopX++)
		{
			if(loopX == tileX && loopY == tileY)
				continue;
			if(CanIMoveToSingle(loopX, loopY, TRUE))
				return(FALSE);
		}
	return(TRUE);
}

bool
Unit::CanIMoveTo(uint32 xPos, uint32 yPos, bool fCheckForUnits)
{
	if(type != JUGGERNAUT)
		return(CanIMoveToSingle(xPos, yPos, fCheckForUnits));
	else
	{
		// check four corners of unit
		if(!CanIMoveToSingle(xPos, yPos, fCheckForUnits))
			return(FALSE);
		if(!CanIMoveToSingle(xPos - unitWidth + 1, yPos, fCheckForUnits))
			return(FALSE);
		if(!CanIMoveToSingle(xPos, yPos - unitHeight + 1, fCheckForUnits))
			return(FALSE);
		if(!CanIMoveToSingle(xPos - unitWidth + 1, yPos - unitHeight + 1, fCheckForUnits))
			return(FALSE);
		return(TRUE);
	}
}

bool
Unit::CanIMoveToSingle(uint32 xPos, uint32 yPos, bool fCheckForUnits)
{
	terrainType	terrain;
	bool			fCanMoveTo;
	Unit			*pUnit;
	MapSpace		*pBuilding;
	grip			gUnit;

	// if out of range
	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
		return(FALSE);

	terrain = (terrainType)pWorld->map.GetTerrainNum(xPos, yPos);
	fCanMoveTo = (bool)(GetMoveRate(terrain) > 0);

	// if terrain is unpassable normally
	if(!fCanMoveTo)
	{
//		// if blocked by terrain movement val, some walkers have another option
//		if(movement == WALKER && type != JUGGERNAUT)
//		{
//			// check for a TORTOISE to walk on
//			pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
//			if(pUnit && pUnit->type == TORTOISE)
//			{
//				// walk on the tortoise instead
//				fCanMoveTo = TRUE;
//			}
//			else if(xPos == tileX && yPos == tileY)
//			{
//				// already standing here, maybe need to re-center.
//				fCanMoveTo = TRUE;
//			}
//		}
	}
	else
	{
		if(fCheckForUnits)
		{
			gUnit = pWorld->unitLib.gUnitMap[yPos][xPos];
			if(gUnit)
			{
				if(gUnit != gSelf)
				{
					pUnit = ADerefAs(Unit, gUnit);
					if(type == JUGGERNAUT)
					{
						if(pUnit->movement == FLIER || pUnit->type == JUGGERNAUT ||
							pUnit->type == UNIT_WEIRD_WARD)
						{
							// cant crush this unit
							fCanMoveTo = FALSE;
						}
					}
					else if(pUnit->type != JUGGERNAUT && (pUnit->tileX != xPos || pUnit->tileY != yPos))
					{
						// leftover grip!  clean up and treat as empty
						pWorld->unitLib.PlaceUnitGrip(NULL, xPos, yPos);
					}
					else
					{
						fCanMoveTo = FALSE;
					}
				}
			}
		}
	
		// if not blocked by units either
		if(fCanMoveTo)
		{
			// check for buildings
			pBuilding = pWorld->map.GetStructure(xPos, yPos);
			if(pBuilding && pBuilding->h.currHP > 0)
			{
				// if building is there
				if(movement != FLIER && type != GNOME)
				{
					// cant get over it - blocked
					fCanMoveTo = FALSE;
				}
			}
		}

		if(fCanMoveTo && type == WYRM && player == SIDE0 && currTerrain == WIND_WALL)
		{
			// another Vas exception
			fCanMoveTo = FALSE;
		}
	}
	return(fCanMoveTo);
}

void
UnitGroup::GetTerrainBonuses(terrainType thisTerrain, int32 &attackBonus, int32 &rangeBonus, int32 &defenseBonus)
{
	attackBonus = 0;
	rangeBonus = 0;
	defenseBonus = 0;

	switch(type)
	{
		case ACOLYTE:
			switch(thisTerrain)
			{
				case PORTAL:
					// warning : should check portal alignment first, no bonus on enemy portal
					attackBonus = 1;
					defenseBonus = 1;
					break;
			}
			break;

		case BANSHEE:
		case CLERIC:
		case GRIFFIN:
		case PALADIN:
			break;	// no special terrain bonuses for these guys

		case DRUID:
			switch(thisTerrain)
			{
				case BRAMBLES:
				case MARSH:
					defenseBonus = 1;
					break;
			}
			break;

		case GHOUL:
		case GOBLIN:
		case ZOMBIE:
			switch(thisTerrain)
			{
				case MARSH:
				case SWAMP:
					defenseBonus = 1;
					break;
			}
			break;

		case GNOME:
			switch(thisTerrain)
			{
				case RUINS:
				case MINE:
				case ARBOR_LODGE:
				case BARRACKS:
				case CAMP:
				case CRYPT:
				case GATE:
				case KEEP:
				case RUNESTONE:
				case TEMPLE:
				case STRUCTURE:
				case WALL:
				case MONUMENT:
				case STOCKPILE:
				case GREAT_WALL:
				case TURRET:
					defenseBonus = 1;
					break;
			}
			break;

		case NYMPH:
			switch(thisTerrain)
			{
				case BRAMBLES:
				case MARSH:
				case SWAMP:
					defenseBonus = 1;
					break;
			}
			break;

		case WIZARD:
			switch(thisTerrain)
			{
				case FOLIAGE:
				case BRAMBLES:
				case ROUGH_LAND:
				case MINE:
				case BROKEN_LAND:
					rangeBonus = -1;
					break;
			}
			break;
		
		case GARGOYLE:
		case FURY:
		case RANGER:
		case ROC:
		case ROC_EGG:
		case TORTOISE:
		case WRAITH:
		case WYRM:
		case UNIT_WEIRD_WARD:
			break;
	}

	// override range modifiers - no longer supported!
	rangeBonus = 0;
}

unitType
Unit::GetTransformationType(funcType bldgFunction)
{
	if(type != ACOLYTE && bldgFunction == FUNC_PORTAL)
		return(ACOLYTE);

	switch(pWorld->map.rMapNum)
	{
		case 9350:
		case 9370:
		case 9390:
			if(bldgFunction == FUNC_RUNESTONE && type != ENCHANTER && player == SIDE2)
				return(ENCHANTER);
			break;

		case 9355:
		case 9375:
		case 9395:
			if(bldgFunction == FUNC_RUNESTONE && type != ENCHANTER && player == SIDE1)
				return(ENCHANTER);
			break;

		// legendary maps - both sides are allowed
		case 9372:
		case 9352:
			if(bldgFunction == FUNC_RUNESTONE && type != ENCHANTER)
				return(ENCHANTER);
	}

	if(bldgFunction == FUNC_IMM_WELL)
	{
		switch(ARandom(16))
		{
			case 0:
				return(DRUID);
			case 1:
				return(GARGOYLE);
			case 2:
				return(WARRIOR);
			case 3:
				return(CLERIC);
			case 4:
				return(WIZARD);
			case 5:
				return(PALADIN);
			case 6:
				return(ZOMBIE);
			case 7:
				return(GOLEM);
			case 8:
				return(RANGER);
			case 9:
				return(GHOUL);
			case 10:
				return(GNOME);
			case 11:
				return(GRIFFIN);
			case 12:
				return(WYRM);
			case 13:
				return(FURY);
			case 14:
				return(WRAITH);
			case 15:
				return(NYMPH);
		}
	}

	switch(type)
	{
		case PEASANT:
			break;

		case ACOLYTE:
			switch(bldgFunction)
			{
				case FUNC_ARBORLODGE:
					return(DRUID);
				case FUNC_CRYPT:
					return(GARGOYLE);
				case FUNC_BARRACKS:
					return(WARRIOR);
				case FUNC_TEMPLE:
					return(CLERIC);
				case FUNC_RUNESTONE:
					return(WIZARD);
			}
			break;
		case WARRIOR:
			switch(bldgFunction)
			{
				case FUNC_TEMPLE:
					return(PALADIN);
				case FUNC_CRYPT:
					return(ZOMBIE);
				case FUNC_RUNESTONE:
					return(GOLEM);
				case FUNC_ARBORLODGE:
					return(RANGER);
			}
			break;
		case CLERIC:
			switch(bldgFunction)
			{
				case FUNC_CRYPT:
					return(GHOUL);
				case FUNC_BARRACKS:
					return(PALADIN);
				case FUNC_RUNESTONE:
					return(GNOME);
			}
			break;
		case GARGOYLE:
			switch(bldgFunction)
			{
				case FUNC_ARBORLODGE:
					return(GRIFFIN);
				case FUNC_RUNESTONE:
					return(WYRM);
				case FUNC_TEMPLE:
					return(FURY);
			}
			break;
		case WIZARD:
			switch(bldgFunction)
			{
				case FUNC_BARRACKS:
					return(GOLEM);
				case FUNC_CRYPT:
					return(WRAITH);
				case FUNC_TEMPLE:
					return(GNOME);
				case FUNC_ARBORLODGE:
					return(NYMPH);
			}
			break;
		case DRUID:
			switch(bldgFunction)
			{
				case FUNC_RUNESTONE:
					return(NYMPH);
				case FUNC_BARRACKS:
					return(RANGER);
			}
			break;
	}
	return(NOUNIT);
}

bool
Unit::ImmuneToAttacksBy(unitType attacker)
{
	switch(type)	// what am I?
	{
		case FURY:
			switch(attacker)
			{
				case FURY:
					return(FALSE);
			}
			return(TRUE);

		case WRAITH:
			switch(attacker)
			{
				case CLERIC:
				case FURY:
				case PALADIN:
				case WIZARD:
				case WRAITH:
					return(FALSE);
			}
			return(TRUE);

		case JUGGERNAUT:
			switch(attacker)
			{
				case UNIT_FIRE:
					return(TRUE);
			}
			return(FALSE);

//		case TORTOISE:
		case UNIT_WEIRD_WARD:
			return(TRUE);
	}
	return(FALSE);
}

bool
Unit::ProtectionFromAttacksBy(unitType attacker)
{
	switch(type)		// what am I?
	{
		case ACOLYTE:
		case FURY:
		case GHOUL:
		case GRIFFIN:
		case WARRIOR:
		case PEASANT:
		case WRAITH:
		case WYRM:
			break;

		case BANSHEE:
			switch(attacker)
			{
				case DRUID:
				case GRIFFIN:
				case NYMPH:
				case RANGER:
				case ROC:
				case TROLL:
				case WOLF:
					return(TRUE);
			}
			break;

		case CLERIC:
		case PALADIN:	// same for both
			switch(attacker)
			{
				case ENCHANTER:
				case GARGOYLE:
				case GHOUL:
				case GOBLIN:
				case HARPY:
				case WRAITH:
				case ZOMBIE:
					return(TRUE);
			}
			break;

		case DRUID:
			switch(attacker)	//protection from the following
			{
				case BANSHEE:
				case GRIFFIN:
				case NYMPH:
				case RANGER:
				case ROC:
				case TROLL:
				case WOLF:
					return(TRUE);
			}
			break;

		case GARGOYLE:
			switch(attacker)
			{
				case WRAITH:
					return(TRUE);
			}
			break;

		case GNOME:
			switch(attacker)
			{
				case GOLEM:
				case JUGGERNAUT:
					return(TRUE);
			}
			break;

		case GOLEM:
			switch(attacker)
			{
//				case GNOME:
				// case lava_flow
//					break;

				default:
					return(TRUE);
			}
//			break;

		case GUARDIAN:
			return(TRUE);

		case JUGGERNAUT:
			switch(attacker)
			{
				case GNOME:
					break;

				default:
					return(TRUE);
			}
			break;

		case NYMPH:
			switch(attacker)
			{
				case BANSHEE:
				case DRUID:
				case GRIFFIN:
				case RANGER:
				case ROC:
				case TROLL:
				case WOLF:
					return(TRUE);
			}
			break;

		case WIZARD:
			switch(attacker)
			{
				case WIZARD:
					return(TRUE);
			}
			break;

		case ZOMBIE:
			switch(attacker)
			{
				case CLERIC:
				case FURY:
				case PALADIN:
				case WIZARD:
				// note: add other cases here for immunity to fire damage (not implemented yet)
					break;

				default:
					return(TRUE);
			}
			break;
	}

	// alright, try items
	switch(item)
	{
		case VIRTUE_VEIL:
			switch(attacker)
			{
				case ENCHANTER:
				case GARGOYLE:
				case GHOUL:
				case GOBLIN:
				case HARPY:
				case WRAITH:
				case ZOMBIE:
					return(TRUE);
			}
			break;

		case VERDANT_SHIELD:
			switch(attacker)
			{
				case BANSHEE:
				case DRUID:
				case GRIFFIN:
				case NYMPH:
				case RANGER:
				case ROC:
				case TROLL:
				case WOLF:
					return(TRUE);
			}
			break;
		case DAEMONS_BANE:
			switch(attacker)
			{
				case CLERIC:
				case FURY:
				case PALADIN:
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}

bool
UnitGroup::NoMovementIn(terrainType terrain)
{
	switch(movement)
	{
		case SWIMMER:
			switch(terrain)
			{
				case WATER:
					return(FALSE);
			}
			return(TRUE);
	}
	return(FALSE);
}

bool
UnitGroup::FreeMovementIn(terrainType terrain)
{
	bool	freedom = FALSE;

	// first - check motion type
	switch(movement)
	{
		case WALKER:
			freedom = FALSE;
			break;
		case SWIMMER:
			switch(terrain)
			{
				case WATER:
					freedom = TRUE;
					break;
			}
			break;
		case FLOATER:
			// dont need to list structures here - their HP value establishes them as obstacles
			switch(terrain)
			{
				case HIGH_FOLIAGE:
				case OBSTRUCTION:
				case GREAT_WALL:
				case WALL:
				case MONUMENT:
				case CHASM:
				case PRISON_PIT:
				case STRUCTURE:
				case WIND_WALL:
				case WELL_OF_IMMORTALS:
					freedom = FALSE;
					break;
				default:
					freedom = TRUE;
			}
			break;

		case FLIER:
			freedom = TRUE;
			break;
	}

	// some units have additional overrides, based on their own type
	switch(type)
	{
		case BANSHEE:
			switch(terrain)
			{
//				case WATER:
				case MINE:
				case RUINS:
					freedom = FALSE;
					break;
			}
			break;

//		case WRAITH:
//			switch(terrain)
//			{
//				case WATER:
//					freedom = FALSE;
//					break;
//			}
//			break;

		case DRUID:
		case RANGER:
			switch(terrain)
			{
				case FOLIAGE:
				case BRAMBLES:
				case ROUGH_LAND:
				case MARSH:
					freedom = TRUE;
			}
			break;

		case GHOUL:
		case GOBLIN:
		case ZOMBIE:
			switch(terrain)
			{
				case MARSH:
				case SWAMP:
					freedom = TRUE;
					break;
			}
			break;

		case GNOME:
			switch(terrain)
			{
				case RUINS:
				case MINE:
				// can also hop over buildings - must have valid move rate for those terrains though
				case ARBOR_LODGE:
				case BARRACKS:
				case CRYPT:
				case GATE:
				case KEEP:
				case RUNESTONE:
				case TEMPLE:
				case STRUCTURE:
				case WALL:
				case MONUMENT:
				case BRIDGE:
				case STOCKPILE:
				case GREAT_WALL:
				case WIND_WALL:
					freedom = TRUE;
					break;
			}
			break;
		
		case GORGON:
			switch(terrain)
			{
				case RUINS:
				case MINE:
					freedom = TRUE;
					break;
			}
			break;

		case GOLEM:
		case TROLL:
			switch(terrain)
			{
				case HIGH_FOLIAGE:
				case CHASM:
				case PRISON_PIT:
				case WATER:
				case OBSTRUCTION:
				case WALL:
				case GREAT_WALL:
				case STRUCTURE:
				case MONUMENT:
				case WIND_WALL:
				case WELL_OF_IMMORTALS:
					freedom = FALSE;
					break;

				default:
					freedom = TRUE;
					break;
			}
			break;

		case JUGGERNAUT:
			switch(terrain)
			{
				case WATER:
				case HIGH_FOLIAGE:
				case OBSTRUCTION:
				case GREAT_WALL:
				case WALL:
				case MONUMENT:
				case CHASM:
				case PRISON_PIT:
				case STRUCTURE:
				case WIND_WALL:
				case WELL_OF_IMMORTALS:
					freedom = FALSE;
					break;

				default:
					freedom = TRUE;
					break;
			}
			break;

		case NYMPH:
			switch(terrain)
			{
				case RUINS:
				case MINE:
					freedom = FALSE;
					break;
			}
			break;
	}
	return(freedom);			
}

bool
Unit::ImmuneToSkillFrom(Unit *pSkillUser)
{
	unitType skillUser;

	if(!pSkillUser)
		return(FALSE);
	else skillUser = pSkillUser->type;	

//	if(pSkillUser->fDoppleganger)
//	{
//		switch(type)
//		{
//			case FURY:
//			case GOLEM:
//			case GHOUL:
//			case GUARDIAN:
//			case WRAITH:
//			case ZOMBIE:
//				return(TRUE);
//			default:
//				if(fDoppleganger)
//					return(FALSE);
//				break;
//		}
//	}

	// units while stoned are immune to all skills
	switch(primaryAction)
	{
		case GET_STONED:
		case DIE:
		case DISBAND:
			return(TRUE);
	}

	if(currAction == TRANSFORM)
	{
		// certain things cant be done to metapods
		switch(pSkillUser->type)
		{
			case WRAITH:
			case NYMPH:
			case HARPY:
			case GORGON:
				return(TRUE);			
		}
	}

	switch(pSkillUser->type)
	{
		case FURY:
			if(item == FROST_CAPE)
				return(TRUE);
			break;

		case ENCHANTER:
			if((player != SIDE1 && player != SIDE2) || player == pSkillUser->player)
				return(TRUE);
			break;

		case HARPY:
			if(movement == FLIER)
				return(TRUE);
			break;

		case WRAITH:
			if(item == VIRTUE_VEIL)
				return(TRUE);
			// fall-thru

		case CLERIC:
		case PALADIN:
			if(tBerserkTimer)
			{
				// immune to these spells while BERSERK
				return(TRUE);
			}
			break;

		case BANSHEE:
		case NYMPH:
			if(item == VERDANT_SHIELD)
				return(TRUE);
			break;
	}

	switch(type)	// what are we?
	{
		case ACOLYTE:
			switch(skillUser)
			{
	 			case GNOME:
					return(TRUE);

				case GORGON:
					if(currAction == GUARD)
					{
						// cant affect an obelisk
						return(TRUE);
					}
					break;
			}
			break;

		case BANSHEE:
			switch(skillUser)
			{
				case BANSHEE:
				case ENCHANTER:
				case NYMPH:
				case WRAITH:
				case GNOME:
					return(TRUE);
			}
			break;

		case BRIGAND:
			switch(skillUser)
			{
				case BRIGAND:
				case GNOME:
				case ENCHANTER:
					return(TRUE);
			}
			break;

		case CLERIC:
			switch(skillUser)
			{
				case GNOME:
				case WRAITH:
					return(TRUE);
			}
			break;

		case DRUID:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
				case NYMPH:
					return(TRUE);
			}
			break;

		case ENCHANTER:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
				case ENCHANTER:
				case NYMPH:
				case WRAITH:
					return(TRUE);
			}
			break;

		case FURY:
			switch(skillUser)
			{
				case BRIGAND:
				case WYRM:
					break;
					
				default:
					return(TRUE);
			}
			break;

		case GARGOYLE:
			switch(skillUser)
			{
				case GNOME:
				case WRAITH:
					return(TRUE);
			}
			break;

		case GHOUL:
			switch(skillUser)
			{
				case CLERIC:
				case GNOME:
				case GHOUL:
				case PALADIN:
				case WRAITH:
					return(TRUE);
			}
			break;

		case GNOME:
			switch(skillUser)
			{
				case GNOME:
				case GORGON:
					return(TRUE);
			}
			break;

		case GOBLIN:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
				case WRAITH:
//				case ENCHANTER:
					return(TRUE);
			}
			break;

		case GOLEM:
			switch(skillUser)
			{
				case CLERIC:
				case PALADIN:
				case GORGON:
				case GHOUL:
				case HARPY:
					return(TRUE);
			}
			break;

		case GORGON:
			switch(skillUser)
			{
				case GORGON:
				case GNOME:
				case WRAITH:
				case ENCHANTER:
					return(TRUE);
			}
			break;

		case GRIFFIN:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
					return(TRUE);
			}
			break;

		case GUARDIAN:
			if(skillUser != GNOME)
				return(TRUE);
			break;

		case HARPY:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
				case BRIGAND:
//				case ENCHANTER:
				case WRAITH:
				case HARPY:
					return(TRUE);
			}
			break;

		case JUGGERNAUT:
			switch(skillUser)
			{
				case GNOME:
					break;
					
				default:
					return(TRUE);
			}
			break;

		case NYMPH:
			switch(skillUser)
			{
				case BANSHEE:
				case ENCHANTER:
				case GNOME:
				case NYMPH:
					return(TRUE);
			}
			break;

		case PALADIN:
			switch(skillUser)
			{
				case GNOME:
				case WRAITH:
					return(TRUE);
			}
			break;

		case PEASANT:
			switch(skillUser)
			{
				case GNOME:
				case ENCHANTER:
					return(TRUE);
			}
			break;

		case RANGER:
			switch(skillUser)
			{
				case GNOME:
					return(TRUE);
			}
			break;

		case ROC:
			switch(skillUser)
			{
				case BANSHEE:
				case ENCHANTER:
				case GNOME:
					return(TRUE);
			}
			break;

		case ROC_EGG:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
				case CLERIC:
				case ENCHANTER:
				case PALADIN:
				case WYRM:
				case JUGGERNAUT:
					break;
					
				default:
					return(TRUE);
			}
			break;

		case TORTOISE:
			switch(skillUser)
			{
				case BANSHEE:
					return(TRUE);
			}
			break;

		case UNIT_WEIRD_WARD:
			return(TRUE);

//		case TRITON:
//			switch(skillUser)
//			{
//				case GORGON:
//				case WYRM:
//				case JUGGERNAUT:
//					return(TRUE);
//			}
//			return(FALSE);

		case TROLL:
			switch(skillUser)
			{
				case BANSHEE:
				case GNOME:
				case CLERIC:
				case ENCHANTER:
				case PALADIN:
//				case GHOUL:
					return(TRUE);
			}
			break;

		case WARRIOR:
			switch(skillUser)
			{
				case GNOME:
					return(TRUE);
			}
			break;

		case WIZARD:
			switch(skillUser)
			{
				case GNOME:
					return(TRUE);
			}
			break;

		case WOLF:
			switch(skillUser)
			{
				case BANSHEE:
				case ENCHANTER:
				case GNOME:
					return(TRUE);
			}
			break;

		case WRAITH:
			switch(skillUser)
			{
				case BANSHEE:
				case CLERIC:
				case JUGGERNAUT:
				case HARPY:
				case PALADIN:
				case GNOME:
				case WRAITH:
				case GHOUL:
					return(TRUE);
			}
			break;

		case WYRM:
			switch(skillUser)
			{
				case WYRM:
				case JUGGERNAUT:
				case GNOME:
					return(TRUE);
			}
			break;

		case ZOMBIE:
			switch(skillUser)
			{
				case BANSHEE:
				case CLERIC:
				case GNOME:
				case GHOUL:
				case PALADIN:
//				case JUGGERNAUT:
				case WRAITH:
//				case WYRM:		// BUG! need to return 0.5 protection factor, not total immunity
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}

bool
Unit::ImmuneToItem(itemType item1)
{
	switch(item1)
	{
		case BOAR_BURGER:
			if(tBerserkTimer)
				return(TRUE);

			switch(type)
			{
				case GOLEM:
				case GUARDIAN:
				case JUGGERNAUT:
				case WRAITH:
				case ZOMBIE:
					return(TRUE);
			}
			return(FALSE);

		case MASON_MIX:
			switch(type)
			{
				case JUGGERNAUT:
				case GOLEM:
				case GUARDIAN:
					return(FALSE);
			}
			return(TRUE);

		case HEALING_SALVE:
			if(tBerserkTimer)
				return(TRUE);

			switch(type)
			{
				case FURY:
				case GHOUL:
				case GOLEM:
				case GUARDIAN:
				case JUGGERNAUT:
				case TROLL:
				case WRAITH:
				case ZOMBIE:
					return(TRUE);
			}
			return(FALSE);
	}
	return(FALSE);
}

int32
Unit::GetMoveRate(terrainType terrain)
{
	UnitGroup	*pUnitGroup;
	int32			terrainNum = (int32)terrain, moveRate = 0, realBaseMove = baseMove;
		
	if(type == ACOLYTE)
	{
		// ignore OBELISK stats
		pUnitGroup = pWorld->unitLib.GetUnitGroup(ACOLYTE);
		realBaseMove = pUnitGroup->baseMoveRate;
	}

	if(terrain >= 0 && terrain < TERRAINMAXTYPES)
	{
		pUnitGroup = pWorld->unitLib.GetUnitGroup(type);
		moveRate = pUnitGroup->moveRates[terrainNum];

		switch(item)
		{
			case TRAILFINDER:
				switch(terrain)
				{
					case ROUGH_LAND:
					case BROKEN_LAND:
						moveRate = realBaseMove;
						break;
				}
				break;
			case SHRUB_SPRITE:
				switch(terrain)
				{
					case FOLIAGE:
					case BRAMBLES:
						moveRate = realBaseMove;
						break;
				}
				break;
			case DRIFT_DISC:
				switch(terrain)
				{
					case WATER:
					case WHIRLPOOL:
						moveRate = realBaseMove;
						break;
				}
				break;
			case BOG_BOOTS:
				switch(terrain)
				{
					case MARSH:
					case SWAMP:
						moveRate = realBaseMove;
						break;
				}
				break;
		}
	}
	return(moveRate);
}

// calculate total length of current path
uint32
Unit::GetPathLength(int8 *pathArray)
{
	int		xPos, yPos, loop1;
	int8		*pData = pathArray;

	for(loop1 = 0; loop1 < MAX_PATH_SIZE; loop1++)
	{
		xPos = *(pData++);
		yPos = *(pData++);
		if(xPos == -1)
			return(loop1);
	}
	return(loop1);
}

void
Unit::GroupSelection(bool fState)
{
	BAM_Ani	*pAni;
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	if(gGroupCursor)
	{
		if(!fState)
		{
			pAni = ADerefAs(BAM_Ani, gGroupCursor);
			pAni->fDeleteMe = TRUE;
			gGroupCursor = NULL;
			pWorldMap->Draw(tileX, tileY);
		}
	}
	else
	{
		if(fState)
		{
			TRACK_MEM("GroupCursor");	gGroupCursor = pWorld->vPort.NewAni(RES_ANIM,
				9002, 1, PRI_INTERFACE, TRUE, FALSE, 0, tileX, tileY, TRUE,
					tileXOff, tileYOff);
			pAni = ADerefAs(BAM_Ani, gGroupCursor);
			pWorld->vPort.SetCursorTarget(UNIT, serialNum, (Guy *)pAni);
			pWorld->vPort.CheckAni(gGroupCursor);
			pWorldMap->Draw(tileX, tileY, TRUE, TRUE);
		}
	}
}

void
Unit::SetTracking(bool fNewTracking)
{
	fCursorTracking = fNewTracking;
}

void
Unit::PeekNextTarget(int &nextX, int &nextY)
{
	int8	*pData;
	
	if(pathLength > 0)
	{
		pData = pathArray + (sizeof(int8) * (curPathPosition * 2));
		nextX = (*pData);
		nextY = *(pData + sizeof(int8));
	}		
}

void
Unit::AddToPath(int8 *pPathArray, int x, int y, bool fInsertAtTop)
{
	int8	*pData;
	
	pData = pPathArray;

	if(fInsertAtTop)
	{
		memcpy(pData + sizeof(int8) * 2, pData, sizeof(pathArray) - 2);
	}
	else
	{
		pData += (sizeof(int8) * (curPathPosition * 2));
	}

	*pData = (int8)x;
	pData++;
	*pData = (int8)y;
	pathLength++;
}

// when walking, unit moves from tile to tile.  Each tile is a new target
void
Unit::ChooseNextTarget(void)
{
	int32	currX, currY, targetX, targetY;

	// calculate exact current location
	currX = tileX * TILE_WIDTH + tileXOff;
	currY = tileY * TILE_HEIGHT + tileYOff;

	// if end of path buffer reached
	if(curPathPosition == MAX_PATH_SIZE)
	{
		// repath from current x,y and continue on
		pathLength = FindPathTo((coord)secondaryX, (coord)secondaryY, pathArray, fStopAdjacent);
		curPathPosition = 0;

		if(fShowCurrentUnitPath && fCursorTracking)
			ShowUnitPath(this);
	}

	// get the next position to move to
	int8	*pData = pathArray + (sizeof(int8) * (curPathPosition * 2));

	if(*pData == -1)	// if no more path left
		curPathPosition = MAX_PATH_SIZE;

	// if not at end of path buffer
	if(curPathPosition != MAX_PATH_SIZE)
	{
		// path data still available.  get next tile step.
		targetX = (*pData);
		pData++;
		targetY = (*pData);
		if(!CanIMoveTo(targetX, targetY, TRUE))
		{
			// next step no longer accessible (maybe a unit moved in the way)
			curPathPosition = MAX_PATH_SIZE;	// trigger repath
		}
		else
		{
			targetX = targetX * TILE_WIDTH + secondaryXOff;	// expand out to pixel count
			targetY = targetY * TILE_HEIGHT + secondaryYOff;
			curPathPosition++;
			targetTileX = targetX / TILE_WIDTH;
			targetTileXOff = targetX % TILE_WIDTH;
			targetTileY = targetY / TILE_HEIGHT;
			targetTileYOff = targetY % TILE_HEIGHT;
			return;
		}
	}

	// end of path buffer reached.
	// Path buffer used up and no further path available,
	// or path cut short by obstacle having moved into it
	return;
}

void
Unit::SetActionAvailable(action action1, bool fState)
{
	int32	flagVal = 1 << (int32)action1;

	if(fState)
		actionsAvail |= flagVal;
	else actionsAvail &= ~flagVal;
}

bool
Unit::CheckActionAvailable(action action1)
{									 
	int32	flagVal = 1 << (int32)action1;

	if(fAutoControl)
	{
		switch(type)
		{
			case BANSHEE:
			case BRIGAND:
				// these types are always under "auto" control, for their own
				// reasons.  So they can still use their skills usually.

				if(tFearLureStonedTimer || tBerserkTimer)
					return(FALSE);
				break;

			default:
				return(FALSE);
		}
	}

	return((actionsAvail & flagVal)? TRUE: FALSE);
}

void
Unit::ClearLastTripSwitch(void)
{
	// for AI tripswitch management
	if(pBam->playerTypes[player] == PLAYER_COMPUTER && (primaryX != ai.lastSwitchPrimaryX ||
		primaryY != ai.lastSwitchPrimaryY))
	{
		// new primary goal - forget about what tripswitch we tried last, so that it might be
		// tried again
		ai.lastSwitchX = 999;
		ai.lastSwitchY = 999;
		ai.lastSwitchPrimaryX = primaryX;
		ai.lastSwitchPrimaryY = primaryY;
	}
}

// overall action control, for complex actions
void
Unit::SetPrimaryAction(action newAction, int32 xPos, int32 yPos)
{
	grip		gNewTarget;
	Unit		*pUnit;
	MapSpace	*pSpace;
	ticks_t	currTicks = ATicks();
	bool		fActionRefused;

//	UnitDebug.Out("\nSetPrimaryAction(%d,%d,%d)", (int)newAction, xPos, yPos);
//	BamDebug.Out("Unit(%d)@%d::SetPrim(%d,%d,%d)\n", serialNum, (int)currTicks,
//		(int)newAction, (int)xPos, (int)yPos);

//	FILE	*pFile = fopen("BamDebug.tmp", "a");
//	fprintf(pFile, "Unit(%s s%d)::SetPrimary(%s, %d, %d);\n", unitName, serialNum,
//		&pVerbNames[(int)newAction][0], xPos, yPos);
//	fclose(pFile);

	// if default X,Y
	if(xPos == -1)
	{
		xPos = tileX;
		yPos = tileY;
	}

	if(type == FURY)
	{
		// FURYs are very picky about what they'll do

		fActionRefused = FALSE;
		
		if(currAction != BLOWN_BACK)
		switch(newAction)
		{
			case GUARD:
				if(xPos != tileX || yPos != tileY)
					fActionRefused = TRUE;
				break;

			case ATTACK:
				if(!pWorld->unitLib.GetUnit(xPos, yPos))
				{
					// only units can be targeted
					fActionRefused = TRUE;
				}
				break;

			case DIE:
			case DISBAND:
				break;

			default:
				fActionRefused = TRUE;
				break;
		}


		if(fActionRefused)
		{
			// if AI loaded us with some mission info, forget it
			if(pBam->playerTypes[player] == PLAYER_COMPUTER)
			{
				ai.action = AI_NONE;
				ai.data = NULL;
				pWorld->ai.ReportUnit(this, "ACTION_REFUSED", TRUE);
			}
			return;
		}
	}

	// some modes dont like to be interrupted, or need to do cleanup
	switch(currAction)
	{
		case GUARD:
			// guard mode needs cleanup first
			switch(newAction)
			{
				case GUARD:
					if(xPos == tileX && yPos == tileY)
					{
						// dont cause image to fidget
						break;
					}
					// fall-through

				case DROP:
				case DIE:					
				case DISBAND:
				case TRANSFORM_TO_BLDG:
				case GET_STONED:
				case MOVE_TO:	// needs to check path availability before ending guard mode
				case TRANSFORM:
					// no guard mode cleanup in these cases though
					if(type == ACOLYTE && currTerrain == FOUND_GENERIC)
					{
						pSpace = pWorld->map.GetStructure(tileX, tileY);
						if(pSpace->h.func == FUNC_GEN_FOUND && pSpace->fFoundationFilled)
							pSpace->CheckFoundationFilled();
					}
					if(newAction == MOVE_TO)
						EndGuardMode();
					break;

				case SKILL:
					if(type == ACOLYTE)
						break;	// skip cleanup
					// else fall-through to DEFAULT:
				default:
					EndGuardMode();
					break;
			}
	}

	// check current primary
	switch(primaryAction)
	{
		case TRANSFORM_TO_BLDG:
			if(fActionDone)
			{
				// transform failed - allow reset
				break;
			}
			// fall-thru

		case DIE:					
		case DISBAND:
			// these modes cannot be over-ridden
			return;

		case BERSERK:
			// BERSERK mode can only be over-ridden by certain things
			if(newAction != DIE && newAction != DISBAND)
				return;
			break;

		case FOLLOW:		// might be NYMPH lure
		case AVOID:			// might be WRAITH fear
			if(!fAutoControl || !tFearLureStonedTimer)
			{
				// We're under voluntary control.  Interruption OK.
				break;
			}

			// fall-through intentional
		case GET_STONED:	// GORGON venom
			// it's possible to be FOLLOWing another unit by choice, so need to check fAutoControl flag too
			if(fAutoControl && newAction != DIE && newAction != DISBAND && newAction != GET_STONED)
			{
				return;
			}
			if(primaryAction == GET_STONED)	// if breaking free
				;
			break;
	}

	if(newAction == primaryAction && xPos == primaryX && yPos == primaryY)
	{
		// redundant, do nothing
      if(!ValidatePrimaryTarget())
      {
         primaryAction = NO_ACTION;
         SetPrimaryAction(MOVE_TO, tileX, tileY);
      }
		return;
	}

	if(currAction != SKILL)
	{
		// okay to cut short any existing wait period
		tNextStateCheck = 0;
	}

	tPrimaryActStarted = currTicks;
//	SetPrimaryTarget(NOTARGET, NULL);
	searchForEnemiesPresetSerial = NULL;	// in case there was one, it no longer matters

	// set up new primary action
	switch(newAction)
	{
		case NO_ACTION:
			primaryAction = newAction;
			tNextStateCheck = currTicks + ARandom(3);
			break;

		case AVOID:
		case FOLLOW:
			// first check target X,Y for source unit
			gNewTarget = pWorld->unitLib.GetUnitGrip(xPos, yPos);
			if(!gNewTarget)
			{
				SetPrimaryAction(GUARD);
				return;
			}

			// safety net #2 - not allowed to follow/avoid yourself
			if(gNewTarget == gSelf)
			{	 
				SetPrimaryAction(GUARD);
				return;
			}

			pUnit = ADerefAs(Unit, gNewTarget);

			if(!pWorld->ValidateTarget(pUnit->serialNum))
			{
				SetPrimaryAction(GUARD);
				return;
			}

			lastPrimaryX = primaryX;
			lastPrimaryY = primaryY;	// needed for wander mode, to create a "general direction" trend
			primaryAction = newAction;
			SetPrimaryTarget(UNIT, pUnit);
			ClearLastTripSwitch();
			if(currTicks > tNextStateCheck)
				tNextStateCheck = currTicks + ARandom(3);
			if(fPackLeader)
				UpdateAllFollowers();
			break;

		case TRANSFORM_TO_BLDG:
			SetPrimaryTarget(NOTARGET, NULL);
			fAutoControl = TRUE;
			tNextStateCheck = currTicks + ARandom(3);
			primaryAction = newAction;
			break;

		case BERSERK:
		case DIE:
		case GET_STONED:
			SetPrimaryTarget(NOTARGET, NULL);
			primaryAction = newAction;
			tNextStateCheck = currTicks + ARandom(3);
			break;

		case DROP:
		case TRANSFORM:
			primaryAction = newAction;
			if(currTicks > tNextStateCheck)
				tNextStateCheck = currTicks + ARandom(3);
			SetPrimaryTarget(TERRAIN, NULL, xPos, yPos);
			ClearLastTripSwitch();
			break;

		case DISBAND:
			SetPrimaryTarget(NOTARGET, NULL);
			primaryAction = newAction;
			tNextStateCheck = currTicks + ARandom(3);
			if(fPackLeader)
				UpdateAllFollowers();
			break;

		case GUARD:
			// MOVE_TO at X,Y will be run first, to insure tile centering before guard mode
			
			// if already guarding specified location
			if(currAction == newAction && primaryX == xPos && primaryY == yPos)
			{
				// no change of primary, so do nothing.
				// We check for this because it happens often, and we dont want the GUARD
				// animation to react
				return;
			}

	 		if(xPos < 0 || yPos < 0 || xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)		// if any parms invalid
			{
				// take current location as default
				xPos = tileX;
				yPos = tileY;
			}

			primaryAction = newAction;
			if(currTicks > tNextStateCheck)
				tNextStateCheck = currTicks + ARandom(3);
	 		lastPrimaryX = primaryX;
			lastPrimaryY = primaryY;	// needed for wander mode, to create a "general direction" trend

			pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
			if(pUnit && pWorld->ValidateTarget(pUnit->serialNum))
			{
				SetPrimaryTarget(UNIT, pUnit);
			}
			else
			{
				pSpace = pWorld->map.GetStructure(xPos, yPos);
				if(pSpace && pWorld->ValidateTarget(pSpace->serialNum))
					SetPrimaryTarget(BUILDING, pSpace);
				else SetPrimaryTarget(TERRAIN, NULL, xPos, yPos);
			}
			ClearLastTripSwitch();

			if(fPackLeader)
				UpdateAllFollowers();
			break;

		case SKILL:
//		case VENOM:
//		case IMMITATE:					// identical logic for all, at this level anyway
	 		lastPrimaryX = primaryX;
			lastPrimaryY = primaryY;	// needed for wander mode, to create a "general direction" trend

			// first check dest loc for a unit to pursue
			// all can target units
			pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
			if(pUnit && pWorld->ValidateTarget(pUnit->serialNum))
			{
				SetPrimaryTarget(UNIT, pUnit);
			}
			else
			{
				MapSpace *pSpace = pWorld->map.GetStructure(xPos, yPos);
				if(pSpace && pWorld->ValidateTarget(pSpace->serialNum))
					SetPrimaryTarget(BUILDING, pSpace);
				else
					SetPrimaryTarget(TERRAIN, NULL, xPos, yPos);
			}

			// some targeting exceptions
			if(newAction == SKILL)
			{
				switch(type)
				{
					case BANSHEE:
					case WRAITH:
					case ACOLYTE:
						// these units have a skill with no target, but targeting themselves would be wrong
						SetPrimaryTarget(TERRAIN, NULL, xPos, yPos);
						break;
				}
			}

			if(type != ACOLYTE)
			{
				// dont want tripswitch to be forgotten because of ACOLYTE mana discharge, since he does that
				// automatically too.
				ClearLastTripSwitch();
			}

			primaryAction = newAction;
			ClearLastTripSwitch();
			if(currTicks > tNextStateCheck)
				tNextStateCheck = currTicks + ARandom(3);
			break;

		case ATTACK:
			if(type == FURY)
				fAutoControl = TRUE;

			// safety net!  No computer vs computer attacks
			if(pBam->playerTypes[player] == PLAYER_COMPUTER)
			{
				// error check: is target unit friendly?
				pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
				if(pUnit->player == player)
				{
					// error!  catch and repair here
					SetPrimaryAction(MOVE_TO, tileX, tileY);
					ai.action = AI_NONE;
					pWorld->ai.ReportUnit(this, "ERR:ATT_FRIEND", TRUE);
					break;
				}
			}

			if(xPos == tileX && yPos == tileY)	// not allowed to attack yourself
			{
				SetPrimaryAction(MOVE_TO, tileX, tileY);
				return;
			}
			// fall-through intentional

		case USE_ITEM:
		case MOVE_TO:
	 		lastPrimaryX = primaryX;
			lastPrimaryY = primaryY;	// needed for wander mode, to create a "general direction" trend

			// first check dest loc for a unit to target
		  	pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
			if(newAction != MOVE_TO && pUnit && pWorld->ValidateTarget(pUnit->serialNum))
			{
				SetPrimaryTarget(UNIT, pUnit);
			}
			else // no unit - look for a building
			{
				MapSpace *pSpace = pWorld->map.GetStructure(xPos, yPos);
				if(pSpace && pWorld->ValidateTarget(pSpace->serialNum) && newAction != MOVE_TO)
				{
					SetPrimaryTarget(BUILDING, pSpace);
				}
				else
				{
					// no bldg found
					if(newAction == ATTACK)
					{
						// no valid target - go there anyway, I guess.
						SetPrimaryAction(MOVE_TO, xPos, yPos);
						return;
					}
					else
					{
						// target the X,Y itself
						SetPrimaryTarget(TERRAIN, NULL, xPos, yPos);
					}
				}
			}
			primaryAction = newAction;
			ClearLastTripSwitch();

			if(currTicks > tNextStateCheck)
				tNextStateCheck = currTicks + ARandom(3);
			if(fPackLeader)
				UpdateAllFollowers();

			// after FURY is given primary command, its avail commands change	
			if(type == FURY && fCursorTracking)
				pWorld->DrawStatusBox();
			break;
	}

	fInAction = FALSE;	// cause Cycle() to make next decision based on primaryAction
	fActionDone = FALSE;
	fActionEnding = FALSE;
	currAction = NO_ACTION;
	fPrimarySatisfied = FALSE;

	// debug test
	if(primaryTargetType != NOTARGET && !ValidatePrimaryTarget())
		UnitDebug.Out("\nUnit(g%d,%s)::SetPrimaryAction() - invalid target selected!", gSelf, unitName);
	CheckPrimaryAction();
	if(fKamikaze)
		fAutoControl = TRUE;
}

void
Unit::SetPrimaryTarget(TargetType targType, void *pTarget, coord targX, coord targY)
{
	Unit		*pUnit;
	MapSpace	*pBldg;
	int		int1;
	bool		fInvalidTarget = FALSE;

	primaryTargetType = targType;

	switch(targType)
	{
		case UNIT:
			pUnit = (Unit *)pTarget;
			if(!pTarget || pUnit->selfType != UNIT)
			{
				fInvalidTarget = TRUE;
				break;
			}

			primaryTargetSerial = pUnit->serialNum;
			primaryX = pUnit->tileX;
			primaryY = pUnit->tileY;

			// aim at center of target's tile
			primaryXOff = TILE_HORZ_CENTER;
			primaryYOff = TILE_VERT_CENTER;
			break;

		case BUILDING:
			pBldg = (MapSpace *)pTarget;
			if(!pTarget)
			{
				fInvalidTarget = TRUE;
				break;
			}

			primaryTargetSerial = pBldg->serialNum;
			if(targX == -1 || pWorld->map.GetStructure(targX, targY) != pBldg)
			{
				int1 = ARandom(pBldg->size);
				primaryX = pBldg->tiles[int1][0];
				primaryY = pBldg->tiles[int1][1];
			}
			else
			{
				primaryX = targX;
				primaryY = targY;
			}
			primaryXOff = TILE_HORZ_CENTER;
			primaryYOff = TILE_VERT_CENTER;
			break;

		default:
			primaryTargetSerial = NULL;
			primaryX = targX;
			primaryY = targY;
			primaryXOff = TILE_HORZ_CENTER;
			primaryYOff = TILE_VERT_CENTER;
			break;
	}

	if(targType != NOTARGET && (primaryX < 0 || primaryX >= WORLD_WIDTH))
		fInvalidTarget = TRUE;

	if(fInvalidTarget)
	{
		UnitDebug.Out("\nUnit::SetPrimaryTarget(t_%d, 0x%x, x%d, y%d) invalid target!",
			(int)targType, (int)pTarget, targX, targY);
		primaryTargetType = NOTARGET;
		primaryTargetSerial = NULL;
		primaryX = 0;
		primaryY = 0;
	}
}

void
Unit::SetSecondaryTarget(TargetType targType, void *pTarget, coord targX, coord targY)
{
	Unit		*pUnit;
	MapSpace	*pBldg;
	int		loop1, tempX, tempY, tempDistance, tempDistance2;

	secondaryTargetType = targType;

	switch(targType)
	{
		case UNIT:
			pUnit = (Unit *)pTarget;	
			secondaryTargetSerial = pUnit->serialNum;
			secondaryX = pUnit->tileX;
			secondaryY = pUnit->tileY;
			secondaryXOff = pUnit->tileXOff;
			secondaryYOff = pUnit->tileYOff;
			break;

		case BUILDING:
			pBldg = (MapSpace *)pTarget;
			secondaryTargetSerial = pBldg->serialNum;

			if(targX == -1)
			{
				// pick closest x,y of building
				tempDistance = 9999;
				secondaryX = -1;
				for(loop1 = 0; loop1 < pBldg->size; loop1++)
				{
					tempX = pBldg->tiles[loop1][0];
					tempY = pBldg->tiles[loop1][1];
					tempDistance2 = GetDistanceTo(tempX, tempY);
					if(!tempDistance2 || tempDistance2 > tempDistance)
						continue;
					if(pWorld->unitLib.GetUnitGrip(tempX, tempY))
					{
						// tile is occupied, cannot target underlying building there
						continue;
					}

					if(tempDistance > 0 && tempDistance2 < tempDistance)
					{
						secondaryX = tempX;
						secondaryY = tempY;
						tempDistance = tempDistance2;
					}
				}

				// if no suitable target found
				if(secondaryX == -1)
				{
					// error out
					secondaryTargetSerial = NULL;
					secondaryTargetType = NOTARGET;
					return;
				}
			}
			else
			{
				secondaryX = targX;
				secondaryY = targY;
			}
			secondaryXOff = TILE_HORZ_CENTER;
			secondaryYOff = TILE_VERT_CENTER;
			break;

		default:
			secondaryTargetSerial = NULL;
			secondaryX = targX;
			secondaryY = targY;
			secondaryXOff = TILE_HORZ_CENTER;
			secondaryYOff = TILE_VERT_CENTER;
			break;
	}	
}

// insure that primary grip is still in use, object is still valid, and return ptr
void *
Unit::ValidatePrimaryTarget(void)
{
	void	*pTarget;

	switch(primaryTargetType)
	{
		case UNIT:
		case BUILDING:
			if(!primaryTargetSerial)
			{
				// invalid circumstance - target probably died and
				// we haven't fully reset ourselves.  -Van
				return(FALSE);
			}
			break;
	}
	pTarget = pWorld->ValidateTarget(primaryTargetSerial, NULL, primaryX, primaryY);
	return(pTarget);
}

// insure that secondary grip is still in use, object is still valid, and return ptr
void *
Unit::ValidateSecondaryTarget(void)
{
	void	*pTarget;

	switch(secondaryTargetType)
	{
		case UNIT:
		case BUILDING:
			if(!secondaryTargetSerial)
			{
				// invalid circumstance - target probably died and
				// we haven't fully reset ourselves.  -Van
				return(FALSE);
			}
			break;
	}

	pTarget = pWorld->ValidateTarget(secondaryTargetSerial, NULL, secondaryX, secondaryY);
	return(pTarget);
}

// update primaryX,Y associated with primary target, in case target has moved
void *
Unit::UpdatePrimaryTarget(void)
{
	Unit		*pUnit;
	MapSpace *pSpace;
	int		int1;

	switch(primaryTargetType)
	{
		case UNIT:
			pUnit = (Unit *)ValidatePrimaryTarget();
			if(!pUnit)
			{
				return(FALSE);
			}
			primaryX = pUnit->tileX;
			primaryY = pUnit->tileY;
			return(pUnit);

		case BUILDING:
			pSpace = (MapSpace *)ValidatePrimaryTarget();
			if(!pSpace)
			{
				return(FALSE);
			}
			if(pWorld->map.GetStructure(primaryX, primaryY) != pSpace)
			{
				// somehow we got invalid coords for a valid bldg
				// repair coords and go on with life
				int1 = ARandom(pSpace->size);
				primaryX = pSpace->tiles[int1][0];
				primaryY = pSpace->tiles[int1][1];
			}
			return(pSpace);

		case TERRAIN:
			if(!ValidatePrimaryTarget())
			{
				return(FALSE);
			}
			return((void *)TRUE);

		default:
			UnitDebug.Out("\nUnit::CheckPrimaryTarget() - unknown target type %d(serial %d)",
				(int)primaryTargetType, (int)primaryTargetSerial);
			return(FALSE);
	}
}

void
Unit::CheckPrimaryAction(void)	// choose new currAction based on primaryAction
{
	Unit		*pUnit;
	terrainType terrain1;
	MapSpace	*pSpace;
	void		*pTarget;
	ticks_t	currTicks;
	bool		fValidTarget;
	int		tempX, tempY, distance, distance2, int1, loop1, tempX2, tempY2;
	action	action1;

//	currTicks = ATicks();
//	BamDebug.Out("\nUnit(%d)@%d::CheckPrim() - pri=%d, curr=%d", serialNum,
//		(int)currTicks, (int)primaryAction,	(int)currAction);

	if(fInAction && !fActionDone)
		return;
	currTicks = ATicks();
	if(currTicks < tNextStateCheck)
	{
//		BamDebug.Out("\nUnit(%d) !tNextStateCheck", serialNum);
		return;
	}

	// is primary still valid?
	pTarget = ValidatePrimaryTarget();
	fValidTarget = pTarget? TRUE: FALSE;

	if(type == BRIGAND && currAction == ATTACK)
	{
		// safety net - BRIGAND should never attack
		UnitDebug.Out("\nUnit::CheckPrimary() - warning, BRIGAND in ATTACK mode.  Overriding.");
		SetPrimaryAction(MOVE_TO, tileX, tileY);
		return;
	}

	switch(primaryAction)
	{
		case NO_ACTION:
			break;

		case TRANSFORM_TO_BLDG:
			if(fInAction && fActionDone)
			{
				// why do we still exist?  Something must have failed.
				fAutoControl = FALSE;
				SetPrimaryAction(MOVE_TO, tileX, tileY);
				return;
			}

		case DIE:
		case DISBAND:
			if(currAction != primaryAction)
				SetAction(primaryAction);
			break;

		case GET_STONED:
			if(tNextStateCheck > currTicks)				// if wait for some predetermined reason
			{
				return;
			}

			if(!fInAction)
			{
				SetAction(primaryAction);
				return;
			}
			else	// presume scare/wail completed
			{
				SetPrimaryAction(MOVE_TO, tileX, tileY);
				return;
			}
//			break;

		// the following are grouped together so they can share target pursuit logic
		// with only minor differences
		case SKILL:
		case DROP:
		case BERSERK:
		case MOVE_TO:
		case GUARD:
		case FOLLOW:
		case ATTACK:
		case AVOID:
//		case IMMITATE:
		case USE_ITEM:
		case TRANSFORM:
//		case VENOM:
			if(tNextStateCheck > currTicks)				// if wait for some predetermined reason
			{
				return;
			}

			// re-check target pos && update primaryX,Y if necessary
			switch(primaryTargetType)
			{
				case UNIT:
				case BUILDING:
					UpdatePrimaryTarget();
					break;
			}

			if(type == FURY && !fValidTarget)
			{
				// FURYs change back to ACOLYTEs when their target is dead/gone
				grip	gAni;

				SetType(ACOLYTE);
				TRACK_MEM("TransformAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1,
					priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY, FALSE);
				pWorld->vPort.CheckAni(gAni);

				pWorld->LaunchSound(SND_TRANSFORM, tileX, tileY);
				// re-register current terrain because it might affect us differently now
				currTerrain = UNDEFINED;
				SetTilePos(tileX, tileY);

				// if we are standing somewhere we shouldnt be, and it wont kill us
				if(!CanIMoveTo(tileX, tileY) && primaryAction != DIE)
				{
					// cant be here - better move aside
					tempX = -1;
					for(loop1 = 1; loop1 < 3 && tempX == -1; loop1++)
					{
						pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, tempX, tempY, TRUE);
						do
						{
							// outside of world bounds
							if(tempX < 0 || tempX >= WORLD_WIDTH || tempY < 0 || tempY >= WORLD_HEIGHT)
							{
								tempX = -1;
								continue;
							}

							if(!CanIMoveTo(tempX, tempY, TRUE))
							{
								tempX = -1;
								continue;
							}

							// found one!
							break;
						}
						while(pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, tempX, tempY));
					}

					// if an open spot found nearby
					if(tempX != -1)
					{
						// move over there instead
						SetTilePos(tempX, tempY);
						TRACK_MEM("TransformAni2");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1,
							priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY, FALSE);
						pWorld->vPort.CheckAni(gAni);						
					}
				}

				// no point in continuing towards an invalid target
				SetPrimaryAction(MOVE_TO, tileX, tileY);

				if(fCursorTracking)
					pWorld->DrawStatusBox();
			}

			// if not doing anything
			if(!fInAction)
			{
				if(primaryAction == BERSERK)
				{
					// if havent started berserk mode yet
					if(!tBerserkTimer)
					{
						SetAction(BERSERK);	// first, spin cycle!
						return;
					}
					else
					{
						// select a new target
						if(!FindNearestPrimaryTarget())
						{
							SetPrimaryTarget(TERRAIN, NULL, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
						}
						fValidTarget = ValidatePrimaryTarget()? TRUE: FALSE;
					}
				}
			
				// init / re-init search for target

				if(!fValidTarget)
				{
					// Target has died.

					if(pBam->playerTypes[player] == PLAYER_COMPUTER)
					{
						// just stop here.  Let AI take control.
						SetPrimaryAction(MOVE_TO, tileX, tileY);
					}
					else if(GetDistanceTo(primaryX, primaryY) <= currRange + 1)
					{
						// we were already in range of target - just sit tight
						SetPrimaryAction(MOVE_TO, tileX, tileY);
					}
					else
					{			
						// proceed to last known X,Y
						SetPrimaryAction(MOVE_TO, primaryX, primaryY);
					}
					return;
				}		

				// are we within range to do what we wanna do yet?
				if(WithinRangeForPrimary())
				{
					// within range of target's last known X,Y

					if(fValidTarget)
					{
						// target is still valid, and now within range.  Respond accordingly.

						switch(primaryAction)
						{
							case MOVE_TO:		// we've arrived.
							case GUARD:
							case FOLLOW:		// we've caught up to them.
							case AVOID:			// we're far enough away to be happy
								// no path left (because we're here).  Act like we just cant move,
								// so enemy search will occur.
								break;

//								SetAction(GUARD);
//								return;

							case BERSERK:
								switch(primaryTargetType)
								{
									case UNIT:
									case BUILDING:
										SetAction(ATTACK, primaryX, primaryY);
										return;

									case TERRAIN:
									default:
										SetAction(GUARD);
										return;
								}

							default:								
								// primary translates directly to appropriate secondary
								if(primaryTargetType == BUILDING)
								{
									// try to select an open tile
									pSpace = (MapSpace *)pTarget;
									if(pSpace)
										for(loop1 = 0, int1 = ARandom(pSpace->size);
											loop1 < pSpace->size; loop1++, int1 = ++int1 % pSpace->size)
										{										
											tempX = pSpace->tiles[loop1][0];
											tempY = pSpace->tiles[loop1][1];
											if(GetDistanceTo(tempX, tempY) > currRange)
												continue;

											if(pWorld->unitLib.GetUnitGrip(tempX, tempY))
												continue;

											SetAction(primaryAction, tempX, tempY);
											return;
										}
								}

								SetAction(primaryAction, primaryX, primaryY);
								return;
						}
					}
				}
				// end of if(WithinRangeForPrimary())

				// else still out of range from target - need to do something			
	 			if(primaryAction == AVOID && !WithinRangeForPrimary())
				{
					// pick a spot away from target unit
					tempX = 0;
					tempX2 = WORLD_WIDTH - 1;
					if(tileX < primaryX)
					{
						tempX2 = AMin(tileX, primaryX - AVOID_DISTANCE);
					}
					else if(tileX > primaryX)
					{
						tempX = AMax(tileX, primaryX + AVOID_DISTANCE);
					}
					if(tempX > WORLD_WIDTH - 1)
					{
						tempX = 0;
						tempX2 = primaryX - AVOID_DISTANCE;
					}
					else if(tempX2 < 0)
					{
						tempX = primaryX + AVOID_DISTANCE;
						tempX2 = WORLD_WIDTH - 1;
					}

					tempY = 0;
					tempY2 = WORLD_HEIGHT - 1;
					if(tileY < primaryY)
					{
						tempY2 = AMin(tileY, primaryY - AVOID_DISTANCE);
					}
					else if(tileY > primaryY)
					{
						tempY = AMax(tileY, primaryY + AVOID_DISTANCE);
					}
					if(tempY > WORLD_HEIGHT - 1)
					{
						tempY = 0;
						tempY2 = primaryY - AVOID_DISTANCE;
					}
					else if(tempY2 < 0)
					{
						tempY = primaryY + AVOID_DISTANCE;
						tempY2 = WORLD_HEIGHT - 1;
					}
					SetAction(MOVE_TO, tempX + ARandom(tempX2 - tempX + 1),
						tempY + ARandom(tempY2 - tempY + 1));
					return;
				}
				else
				{
					// can we get any closer to primary at this moment?
					if(!WithinRangeForPrimary())
					{
						pathLength = FindPathTo(primaryX, primaryY, pathArray, FALSE);
						curPathPosition = 0;
						if(pathLength > 0)
						{
							// is first step open?
							PeekNextTarget(tempX, tempY);
						
							// if open right now
							if(CanIMoveTo(tempX, tempY, TRUE))
							{
								// back on the road
								SetAction(MOVE_TO, primaryX, primaryY);
								fStopAdjacent = FALSE;
								return;
							}
						}
					}
				}

				if(fPackLeader)
				{
					// whether or not we can reach him, give followers the same chance
					UpdateAllFollowers();
				}

				// no path available at all, at this time.

				// well then - look around for an enemy
				pUnit = SearchForEnemies();

				if(pUnit)
				{
					if(type == BRIGAND)
					{
						// if enemy has no item, or one we dont want
						if(!pUnit->item || pUnit->item == HARPY_NET)
						{
							// run away!
							SetPrimaryAction(MOVE_TO, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
							return;
						}
					}

					// attack it
					if(GetDistanceTo(pUnit, UNIT) <= currRange)
					{
						if(fPackLeader)	// pack leader? then change primary so pack will follow
						{
							SetPrimaryAction(ATTACK, pUnit->tileX, pUnit->tileY);
							// we're overriding whatever PrimaryAction there was before, but since
							// only NPCs move in packs anyway, it's forgivable
						}
						else	// just attack him
						{
							if(type == BRIGAND)
							{
								// target must be carrying an item (but not for long)
								SetAction(STEAL, pUnit->tileX, pUnit->tileY);
							}
							else
								SetAction(ATTACK, pUnit->tileX, pUnit->tileY);
						}
						return;
					}
					else
					{
						if(fPackLeader)
						{
							// he must have attacked one of our followers - get him!
							SetPrimaryAction(ATTACK, pUnit->tileX, pUnit->tileY);
							return;
						}
							// Evaluate its threat level.
						switch(primaryAction)
						{
							case GUARD:
								// also get distance to our primary target, which we are to defend.
									switch(primaryTargetType)
								{
									case UNIT:
									case BUILDING:
										distance = pUnit->GetDistanceTo(pTarget, primaryTargetType);
										break;
									case TERRAIN:
									default:
										distance = pUnit->GetDistanceTo(primaryX, primaryY);
										break;
								}
								break;

							default:
								distance = 9999;
								break;									
						}

						// also get target unit's distance to us
						distance2 = pUnit->GetDistanceTo(this, UNIT);

						// is target unit within range to threaten either us or our primary goal (if we have
						// one to defend)?
						if(distance <= pUnit->currRange || distance2 <= pUnit->currRange)
						{
							// go get him.
							// alright, how do we get to there?
							pathLength = FindPathTo(pUnit->tileX, pUnit->tileY, pathArray, FALSE);
							curPathPosition = 0;
							if(pathLength > 0)
							{
								// is first step open?
								PeekNextTarget(tempX, tempY);
					
								// if open right now
								if(CanIMoveTo(tempX, tempY, TRUE))
								{
									// go there and start a fight
									if(pBam->playerTypes[player] == SIDE3)
									{
										// go get 'im
										SetPrimaryAction(ATTACK, pUnit->tileX, pUnit->tileY);
									}
									else
									{
										// go defend ourselves w/o forgetting our primary objective
										SetAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
									}
									fStopAdjacent = TRUE;
									return;
								}
							}
						}
						return;
					}
				}
				// end of if(pUnit)

				// alright, no enemy units to attack.  Maybe an enemy building then?
				pSpace = SearchForBuildings(TRUE);
				if(pSpace)
				{
					// find an open spot w/i range and attack
					for(loop1 = 0, int1 = ARandom(pSpace->size); loop1 < pSpace->size;
						loop1++, int1 = ++int1 % pSpace->size)
					{
						tempX = pSpace->tiles[int1][0];
						tempY = pSpace->tiles[int1][1];
						if(pWorld->unitLib.GetUnit(tempX, tempY))
							continue;

						if(GetDistanceTo(tempX, tempY) > currRange)
							continue;

						SetAction(ATTACK, tempX, tempY);
						return;
					}
				}

				if(primaryAction == GUARD && WithinRangeForPrimary())
				{
					SetAction(GUARD, primaryX, primaryY);
				}
				else SetAction(GUARD);
				return;
			}
			// end of !fInAction
			
			// else secondary action already begun, and terminated. Figure out why.

			if(type == BANSHEE)
			{
				// time to scream?
				if(currTicks >= tAutoActionTimer && CheckActionAvailable(SKILL))
				{
					SetAction(SKILL);
					return;
				}
			}
			// else what were we doing, and why have we stopped it?
			switch(currAction)
			{
				// any actions not covered below will default to SetPrim(MOVE, here) to terminate them.

				case BERSERK:
					// doesnt search for current nearest enemy and
					// do whatever's appropriate
				
					fInAction = FALSE;
					CheckPrimaryAction();
					return;
				
				case MOVE_TO:
					// first - some exceptions, before running general MOVE_TO logic.
					switch(type)
					{
//						case TORTOISE:
//							// react dumbly.
//							SetAction(GUARD);
//							return;

						case BRIGAND:
							// look for stuff to grab
							int32 itemX, itemY;

							if(SearchForItems(itemX, itemY))
							{
								if(itemX == tileX && itemY == tileY)
								{
									// it's right under our nose
									PickupItem();
									break;
								}

								// alright, how do we get to it?
								pathLength = FindPathTo(itemX, itemY, pathArray, FALSE);
								curPathPosition = 0;
								if(pathLength > 0)
								{
									// is first step open?
									PeekNextTarget(tempX, tempY);
						
									// if open right now
									if(CanIMoveTo(tempX, tempY, TRUE))
									{
										// nevermind why we stopped - get that item!

										SetPrimaryAction(MOVE_TO, itemX, itemY);
										fStopAdjacent = FALSE;
										return;
									}
								}
							}
							break;
					}

					if(!fValidTarget || WithinRangeForPrimary())
					{
						// restart target search
						fInAction = FALSE;
						CheckPrimaryAction();
						return;
					}
					else	// something must have got in our way
					{
						// alternate route avail?
						if(FindPathTo((coord)primaryX, (coord)primaryY, NULL, FALSE) > 0)
						{
							// re-init travel
							fInAction = FALSE;
							CheckPrimaryAction();
							return;
						}

						if(primaryTargetType == BUILDING)
						{
							// try targeting a different tile of the building
							pSpace = (MapSpace *)pTarget;
							if(pSpace->size > 1)
							{
								int slot = ARandom(pSpace->size);

								tempX = pSpace->tiles[slot][0];
								tempY = pSpace->tiles[slot][1];

								if(!pWorld->unitLib.GetUnitGrip(tempX, tempY) &&
									FindPathTo((coord)tempX, (coord)tempY, NULL, FALSE) > 0)
								{
									// better alternative discovered! re-init travel
									primaryX = tempX;
									primaryY = tempY;
									fInAction = FALSE;
									CheckPrimaryAction();
									return;
								}
							}
						}

						// just sit and wait - GUARD mode will rescan for an open path
						SetAction(GUARD);	// will automatically attack if enemy within range
						return;
					}
//					return;


				case STEAL:
					if(type == BRIGAND)	// should always be true
					{
						SetPrimaryAction(MOVE_TO, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
						return;
					}
					break;

				case ATTACK:
					if(type == GORGON && primaryTargetType == UNIT)
					{
						pUnit = (Unit *)pTarget;
						if(fValidTarget && pUnit->primaryAction == GET_STONED)
						{
							// target is stoned - dont attack anymore
							SetPrimaryAction(MOVE_TO, tileX, tileY);
							return;
						}
					}
					// target died or moved, either primary or secondary target.
					// re-init search process for primary target
					fInAction = FALSE;
					CheckPrimaryAction();
					return;

				case GUARD:
					// these units end guard mode for their own non-standard reasons,
					// so their decisions are made here.
					switch(type)
					{
						case BRIGAND:
							int32 itemX, itemY;
		
							if(SearchForItems(itemX, itemY))
							{
								SetAction(MOVE_TO, itemX, itemY);
								fStopAdjacent = FALSE;
								return;
							}
							break;

						case GORGON:
							// did we wake up because we found an enemy?
							pUnit = SearchForEnemies();
							if(pUnit)
							{
								// let normal process deal with it
								SearchForEnemiesPreset(pUnit);
								break;
							}

							// did we wake up to return to an ulterior motive?
							if(defendTargetAction != NO_ACTION)
							{
								// let normal process deal with it
								break;
							}

							if(tFearLureStonedTimer)
							{
								// we're more interested in our target unit for now.
								// bypass normal GORGON bldg-search logic.
								break;
							}

							// no enemy present, must be periodic wakeup.  Time to wander.
							// Pick new dest X,Y (with special GORGON considerations)
							tempX = 0;
							while(tempX != -1)
							{
								tempX = ARandom(WORLD_WIDTH);
								tempY = ARandom(WORLD_HEIGHT);

								terrain1 = (terrainType)pWorld->map.GetTerrainNum(tempX, tempY);
								if(terrain1 == RUINS || terrain1 == ROAD)
								{
									SetPrimaryAction(MOVE_TO, tempX, tempY);
									return;
								}
								pSpace = pWorld->map.GetStructure(tempX, tempY);
								if(pSpace && pSpace->h.currHP)
								{
									SetPrimaryAction(MOVE_TO, tempX, tempY);
									return;
								}
							}
//							break;

						case TORTOISE:
//							if(WithinRangeForPrimary())
//							{
//								// pick a new destination
//								pSpace = pWorld->map.GetStructure(tileX, tileY);
//								if(!pSpace || !pSpace->h.hotSpot || (!pSpace->h.destX && !pSpace->h.destY))
//								{
//									tempX = ARandom(WORLD_WIDTH);
//									tempY = ARandom(WORLD_HEIGHT);
//								}
//								else
//								{
//									tempX = pSpace->h.destX;
//									tempY = pSpace->h.destY;
//								}
//								SetPrimaryAction(MOVE_TO, tempX, tempY);
//								return;
//							}

//							loop1 = ARandom(pWorld->map.totalHotSpots);
//							for(loop2 = 0;	loop2 < pWorld->map.totalHotSpots;
//								loop2++, loop1 = (loop1 + 1) % pWorld->map.totalHotSpots)
//							{
//								pSpace = ADerefAs(MapSpace, pWorld->map.gHotSpots[loop1]);
//								if(CanIMoveTo(pSpace->h.xPos, pSpace->h.yPos))
//								{
//									// terrain is passable to us - check for available path
//									curPathPosition = 0;
//									pathLength = FindPathTo(pSpace->h.xPos, pSpace->h.yPos, pathArray, FALSE);
//									if(pathLength > 0)
//									{
//										SetPrimaryAction(MOVE_TO, pSpace->h.xPos, pSpace->h.yPos);
//										return;
//									}
//								}
//							}
//							UnitDebug.Out("Unit::CheckPrimaryAction(g%d, %s) - error, couldnt locate available hotspot.\n",
//								gSelf, unitName);
//
//							SetPrimaryAction(MOVE_TO, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
//							return;

						case JUGGERNAUT:
							switch(ARandom(3))
							{
								case 2:
								case 1:	// ATTACK
									pUnit = SearchForEnemies();
									if(pUnit && GetDistanceTo(pUnit, UNIT) <= currRange)
									{
										SetAction(ATTACK, pUnit->tileX, pUnit->tileY);
										return;
									}
			
									pSpace = SearchForBuildings(TRUE);
									if(pSpace)
									{
										int1 = ARandom(pSpace->size);
										SetAction(ATTACK, pSpace->tiles[int1][0], pSpace->tiles[int1][1]);
										return;
									}
									// if no enemies, move instead.
									// fall-through intentional.

								case 0:	// MOVE
									// pick a new direction, or continue in same dir?
									if(ARandom(2))
									{
										tempX = tileX + ARandom(3) - 1;
										tempY = tileY + ARandom(3) - 1;
									}
									else
									{
										// try to continue in same dir
										GetXYOffsets(currDir, tempX, tempY);
										tempX += tileX;
										tempY += tileY;
									}
									if(tempX < 0)	tempX = 0;
									else if(tempX >= WORLD_WIDTH)	tempX = WORLD_WIDTH - 1;
								
									if(tempY < 0)	tempY = 0;
									else if(tempY >= WORLD_HEIGHT)	tempY = WORLD_HEIGHT - 1;
									SetPrimaryAction(MOVE_TO, tempX, tempY);
									return;
							}
							break;
					}

					// Either waiting for obstacle/target to move, or already there and
					// something woke us up.
					// Run general logic.
					
					if(!WithinRangeForPrimary())
					{
						// what woke us up doesnt matter, we still
						// have some traveling to do.
						// restart search for primary X,Y
						fInAction = FALSE;
						CheckPrimaryAction();
					}
					else
					{
						// check for an ulterior motive to return to
						if(defendTargetAction)
						{
							tempX = defendTargetX;
							tempY = defendTargetY;
							action1 = defendTargetAction;

							// restore ulterior target specs to primary position
							defendTargetType = NOTARGET;	defendTargetSerial = NULL; defendTargetAction = NO_ACTION;
							SetPrimaryAction(action1, tempX, tempY);
							return;
						}

						// within range of primary.  But certain primaries are do-nothing once target is reached.
						switch(primaryAction)
						{
							case MOVE_TO:
							case GUARD:
							case FOLLOW:
								// currAction == GUARD and we woke up.. why?
								pUnit = SearchForEnemies();
								if(pUnit)
								{
									// is enemy outside of our attack range?
									distance = GetDistanceTo(pUnit, UNIT);
									if(distance > currRange && primaryAction != FOLLOW)
									{
										// since our primary is to be where we are, we cant travel
										// to a distant enemy without possibly contradicting our primary,
										// which will override such travel.
										// Need to change primary, but not forget that we have an "ulterior
										// motive".

										// remember our ulterior motive here.  CycleMove() and CycleAttack() will
										// keep it in mind.
										defendTargetX = primaryX;
										defendTargetY = primaryY;
										defendTargetAction = primaryAction;

										defendTargetSerial = NULL;
										defendTargetType = NOTARGET;
										if(WithinRangeForPrimary())
										{
											pSpace = SearchForBuildings(FALSE);
											if(pSpace)
											{
												defendTargetType = BUILDING;
												defendTargetSerial = pSpace->serialNum;
											}
										}

										SetPrimaryAction(ATTACK, pUnit->tileX, pUnit->tileY);
										return;
									}
									
									if(fPackLeader)	// pack leader has to change his primary for pack to follow
									{
										SetPrimaryAction(ATTACK, pUnit->tileX, pUnit->tileY);
										// we're overriding our primary, but since only NPCs move in packs anyway
										// then it's forgivable
										return;
									}
									else	// just attack him
									{
										if(type == BRIGAND)
											SetAction(STEAL, pUnit->tileX, pUnit->tileY);
										else
											SetAction(ATTACK, pUnit->tileX, pUnit->tileY);
										return;
									}
								}							
								break;
						}

						// primary target still valid?
						if(fValidTarget)
						{
							// re-search will find him, and act accordingly
							fInAction = FALSE;
							CheckPrimaryAction();
						}
						else
						{
							// if primary target has died, our only remaining "purpose" is just to be here
							// so enter that mode and let it deal with whoever just woke us up
							SetPrimaryAction(MOVE_TO, tileX, tileY);
						}
					}
				 	return;

				case SKILL:
					// was skill completed upon primary target?
					if(primaryAction == SKILL)
					{	
						switch(primaryTargetType)
						{
							case UNIT:
							case TERRAIN:
								if(secondaryX == primaryX && secondaryY == primaryY)
								{
									// okay, we're done
									SetPrimaryAction(MOVE_TO, tileX, tileY);
									return;
								}
								break;

							case BUILDING:
								if(pWorld->map.GetStructure(secondaryX, secondaryY)
									== ValidatePrimaryTarget())
								{
									// okay, we're done
									SetPrimaryAction(MOVE_TO, tileX, tileY);
									return;
								}
								break;
						}
					}
					SetAction(GUARD);
					return;

				case TRANSFORM:
					// undo OBELISK stats from TRANSFORM mode
					SetCurrTerrain(currTerrain);
					if(fCursorTracking)
						pWorld->DrawUnitStatus(serialNum, FALSE);

					SetPrimaryAction(GUARD, tileX, tileY);		// mission complete
					return;

				default:
					SetPrimaryAction(MOVE_TO, tileX, tileY);		// mission complete
					return;
			}

			// catch-all here, in case something slips through
			SetAction(NO_ACTION);
			fInAction = FALSE;	// re-init search process
			CheckPrimaryAction();
			break;
	}
}

bool
Unit::WithinRangeForPrimary(void *pTarget, action action1, int tempX, int tempY)
{
	int32			distance;
	Unit			*pUnit;
	MapSpace		*pSpace;
	TargetType	targ;

	if(pTarget == NULL)
		pTarget = ValidatePrimaryTarget();
	pUnit = (Unit *)pTarget;
	pSpace = (MapSpace *)pTarget;
	if(!pTarget)
		targ = TERRAIN;
	else targ = primaryTargetType;
	if(action1 == NO_ACTION)
		action1 = primaryAction;

	if(tileXOff != TILE_HORZ_CENTER || tileYOff != TILE_VERT_CENTER)
	{
//		if(action1 != FOLLOW || pUnit->type != TORTOISE)
			return(FALSE);
	}

	switch(targ)
	{
		case UNIT:
		case BUILDING:
			distance = GetDistanceTo(pTarget, targ);
			break;

		default:
			if(tempX == -1)
			{
				tempX = primaryX;
				tempY = primaryY;
			}
			distance = GetDistanceTo(tempX, tempY);
			if(type == JUGGERNAUT && primaryAction == MOVE_TO)
			{
				// tweak for JUGGERNAUT targeting TERRAIN for MOVE
				if(tempX < tileX || tempY < tileY)
					distance++;
			}
			break;
	}

	switch(primaryAction)
	{
		case ATTACK:
		case BERSERK:
			if(distance <= currRange)	// regular attack range
				return(TRUE);
			return(FALSE);

		case FOLLOW:
			// distance == 0 is possible if FOLLOW'ing a TORTOISE
//			if(targ == UNIT && pUnit->type == TORTOISE)
//			{
//				if(distance == 0)
//					return(TRUE);
//				else return(FALSE);
//			}

			if(distance == 1)
				return(TRUE);
			return(FALSE);

	 	case AVOID:
			if(distance < AVOID_DISTANCE)
				return(FALSE);	// primary not satisfied until OUTSIDE of range10
			return(TRUE);

		// skills go here
		case SKILL:
//		case IMMITATE:
//		case VENOM:
			if(distance <= skillRange)
				return(TRUE);
			return(FALSE);

		case USE_ITEM:
			if(distance <= pWorld->itemMgr.GetItemRange(item))
				return(TRUE);
			return(FALSE);

		case MOVE_TO:	
		case GUARD:
		case TRANSFORM:
		case DROP:
			switch(targ)
			{
				case UNIT:
				case BUILDING:
					// if next to, or on top of, destination
					if(distance < 2)
						return(TRUE);
					else return(FALSE);

				case TERRAIN:
				default:
					if(distance < 1)	// if on top
						return(TRUE);
					return(FALSE);
			}
			// break; unnecessary

//		case DROP:
//			if(distance < 1)
//				return(TRUE);
//			else return(FALSE);
	}
	return(FALSE);
}

int32
Unit::HealThyself(grip gHealer, unitType healer, uint32 healVal)
{
	grip	gAni;
	int	loopX, loopY;

	#ifndef NDEBUG
	BamDebug.Out("Us%d(%s):HealThyself(g%d, %d)\n", serialNum, unitName,
		gHealer, healVal);
	#endif

	gHealer = gHealer;
  	healer = healer;
	currHP = AMin(currHP + healVal, maxHP);
	if(fCursorTracking)
		pWorld->UpdateHPGuageOnly(currHP, maxHP, player);


	for(loopY = tileY - (unitHeight - 1); loopY <= tileY; loopY++)
		for(loopX = tileX - (unitWidth - 1); loopX <= tileX; loopX++)
		{
			TRACK_MEM("HealAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_HEAL_REPAIR, 1, PRI_SKY,
				TRUE, FALSE, 3, loopX, loopY, FALSE, tileXOff, tileYOff);
			pWorld->vPort.CheckAni(gAni);
		}

	pWorld->LaunchSound(SND_HEAL_REPAIR, tileX, tileY);
	return(currHP);
}

// some units have protection/immunity to fire
int32
Unit::ComputeFireDamage(int32 damage)
{
	switch(type)
	{
		case GOLEM:
			damage /= 2;	// protection, not immunity
			damage = AMax(damage, 1);
			break;
		
		case GUARDIAN:
		case JUGGERNAUT:
			damage = 0;		// immune
			break;

		case WYRM:
			damage = 0;
			break;
	}

	if(item == FROST_CAPE)
	{
		damage = 0;
	}
	return(damage);
}

int32
Unit::TakeAHit(BAM_Ani *pAttacker, unitType attacker, uint32 attackerX,
	uint32 attackerY, int32 damage, bool fSkillDmg)
{
	terrainType terrain = UNDEFINED;
	uint32		attackerSerial = 0;
	ticks_t		currTicks = ATicks();
	Unit			*pUnit, *pUnit2;
	UnitGroup	*pGroup;

	#ifndef NDEBUG
	BamDebug.Out("Unit(s%d g%d %s @%d/%d)::Hit(%d/%d)..", serialNum,
		gSelf, unitName, tileX, tileY, attackerX, attackerY);
	#endif

	if(!pWorld->ValidateTarget(serialNum))
	{
		// we're in an un-attackable state
		return(0);
	}
	if(type == UNIT_WEIRD_WARD)
	{
		// immune to all damage
		BamDebug.Out(" warning: undamageable unit attacked\n");
		return(0);
	}

	// if attacked by another unit..
	if(pAttacker && pAttacker->selfType == UNIT)
	{
		if(ImmuneToAttacksBy(attacker))
		{
			if(type == FURY)
			{
				if(pAttacker != ValidatePrimaryTarget())
					damage = 0;
				// else ignore immunity - it doesn't apply in thise case
			}
			else damage = 0;
		}

		pUnit = (Unit *)pAttacker;
		attackerSerial = pUnit->serialNum;
		if(pUnit->player != player)
		{	
			pWorld->unitLib.SetEnemyFlag(player, pUnit->player, TRUE);
			if(pBam->playerTypes[player] == PLAYER_LOCAL)
			{
				// player's unit attacked - sound the alarm?
				if(!pWorld->tUnitAttackTimer || (pWorld->tUnitAttackTimer < (currTicks - ATTACK_ALARM_TIMER)
					&& currTicks >= ATTACK_ALARM_TIMER))
				{
					pWorld->Announce(tileX, tileY, SQUIB_WORLD_MSGS, SQUIB_WORLD_UNITSINDANGER);
				}
				// reset timer
				pWorld->tUnitAttackTimer = ATicks();
			}
		}
		lastAttackerSerial = attackerSerial;
		lastAttackTicks = currTicks;
	}
	else pUnit = NULL;

	// damage modifiers
	if(primaryAction == BERSERK)
	{
		if(tileX != attackerX || tileY != attackerY)
			damage = 0;			// no one else can harm us right now
//		else			// berserker brew damage!
	}
	else if(type == FURY)	// special conditions
	{
		if(attacker == UNIT_TERRAIN)
		{
			if(currTerrain == PORTAL || currTerrain == GUARDIAN_PORTAL)
				;	// PORTALs can hurt us
			else damage = 0;
		}
		else if(pUnit)	// attacked by another unit?
		{
			// if we dont have a target yet, or we do and this guy isnt it
			if(!primaryTargetSerial || (attackerSerial != primaryTargetSerial))
			{
				// he cant hurt us
				damage = 0;
			}

			if(fSkillDmg && ImmuneToSkillFrom(pUnit))
			{
				damage = 0;
			}
		}

		// if damage wasnt negated by something
		if(damage)
		{
			if(attacker == WIZARD || (attacker == BANSHEE && fSkillDmg))
				;	// ignore currDefense, dmg is constant
			else
			{
				// use usual math
				damage = AMax(damage - currDefense, 1);
			}
		}
	}
	else if(attacker == WIZARD || (attacker == BANSHEE && fSkillDmg))
	{
		; // ignore currDefense - damage from these is not reduced by armor
	}
	else if(attacker == UNIT_TERRAIN)	// if suffering terrain damage
	{
		terrain = (terrainType)pWorld->map.GetTerrainNum(attackerX, attackerY);
		if(terrain == PORTAL || terrain == GUARDIAN_PORTAL)
			;	// no change
		else if(movement == FLIER || movement == SWIMMER)
		{
			// not affected by terrain
			damage = 0;
//			UnitDebug.Out("Unit::TakeAHit(s%d, %s) - unaffected by terrain dmg\n", serialNum, unitName);
		}
		else
		{
			// if unit is allowed free movement in this terrain, no damage
		
			pGroup = pWorld->unitLib.GetUnitGroup(type);
			if(pGroup->FreeMovementIn(terrain))
				damage = 0;
		}

		// item immunities are handled later.
	}
	else if(attacker == UNIT_FIRE)	// if suffering fire damage
	{
		damage = ComputeFireDamage(damage);	// apply immunity or protection
	}
	else if(attacker == UNIT_ARCH_FIRE)
	{
		if(type == GUARDIAN)
			damage = 0;
		// no one else is immune
	}
	else if(pUnit && fSkillDmg && ImmuneToSkillFrom(pUnit))
	{
		damage = 0;
	}
	else // normal condition
	{
		// if damage hasn't been negated
		if(damage > 0)
		{
			damage -= currDefense;		// our defense absorbs some of the blow
			if(damage < 1)
				damage = 1;					// but 1 pt damage is minimum

			if(pAttacker && pAttacker->selfType == UNIT)
			{
				if(ProtectionFromAttacksBy(attacker))
					damage = AMax(1, damage / 2);
			}
		}
	}

	// if we're NYMPH-lured and she has attacked us
	if(primaryAction == FOLLOW && tFearLureStonedTimer && primaryTargetSerial == attackerSerial)
	{
		// break the spell
		tFearLureStonedTimer = NULL;
		fAutoControl = FALSE;
		SetPrimaryAction(MOVE_TO, tileX, tileY);
	}

	// check for immunity, other special circumstances
	if(!fSkillDmg && damage > 0)
	switch(item)
	{
		case HARPY_NET:
			LoseItem();
			break;

		case FROST_CAPE:
			// immunity to these attackers (damage == 0)
			if(attacker == UNIT_FIRE)
				damage = 0;
			break;
		case SHRUB_SPRITE:
			// immunity to these attackers (damage == 0)
			if(attacker == UNIT_TERRAIN && terrain == BRAMBLES)
				damage = 0;
			break;
	}

	bool	fHitAniAlreadyRunning = FALSE;
	if(currAnimState == ST_ANIM_HIT)
	{
		fHitAniAlreadyRunning = TRUE;
	}

	if(primaryAction == DIE || primaryAction == DISBAND || primaryAction == BERSERK || damage < 1)
	{
		// these animations have priority
	}
	else if(currAnimState == ST_ANIM_ATTACK && !fAniDone && scrim.cel < 3)
	{
		// attack anim has priority until cel #3
	}
	else if(primaryAction == GET_STONED && fAutoControl)
	{
		// cause STONED process to end immediately
		EndStonedMode();
	}
	else
	{
		// run hit animation
		direction	attackDir;
		
		if(tileX != attackerX || tileY != attackerY)
			attackDir = GetDir(attackerX - tileX, attackerY - tileY);
		else attackDir = currDir;

		if(type == ACOLYTE && currAction == GUARD)				// if acolyte (obelisk), use special anim
		{
			if(attacker == WIZARD)	// show "shock" cel first, then run hit anim delayed instead
			{
				RunAnimation(ST_ANIM_DIE, 0, FALSE, DNULL, TRUE, TRUE);		// single cel "shock"
				RunAnimation(ST_ANIM_HIT, 3, FALSE, attackDir, FALSE);		// delayed hit anim
			}
			else
				RunAnimation(ST_ANIM_HIT, 3);
		}
		else
		{ 
			if(attacker == WIZARD)	// show "shock" cel first, then run hit anim delayed insteasd
			{
				RunAnimation(ST_ANIM_DIE, 0, FALSE, DNULL, TRUE, TRUE);
				RunAnimation(ST_ANIM_HIT, 3, FALSE, attackDir, FALSE);

			}
			else
			{
				RunAnimation(ST_ANIM_HIT, 3, FALSE, attackDir);
			}
		}
	}

	currHP -= damage;
//	UnitDebug.Out("Unit(g%d, %s) hit for %d points. %d HP left\n", gSelf, unitName, damage, currHP);

	// BRIGANDs drop items when hit, sometimes
	if(type == BRIGAND && damage > 0 && item && pUnit)
	{
		DropItem();
	}

	if(currHP < 1)		// if we're dead
	{
		if(currAction != DIE)
		{
			if(attacker == UNIT_FIRE || (attacker == WYRM && fSkillDmg))
				Die(NULL, UNIT_FIRE);
			else if(attacker == UNIT_ARCH_FIRE)
				Die(NULL, UNIT_ARCH_FIRE);
			else Die(pUnit);
		}
	}
	else if(pUnit)	// if attacked by another unit
	{
		// notify AI, in case it cares
		pWorld->ai.HitUnit(this, pUnit);

		if(fPackFollower)	// gang retalliation
		{
			Unit	*pLeader = (Unit *)(pWorld->ValidateTarget(packLeaderSerial));

			if(pLeader && pLeader->currAction == GUARD)					// if leader is at rest
				pLeader->SetPrimaryAction(ATTACK, attackerX, attackerY);	// send the whole mob after 'em!
		}
		else // we're solo, now choose our own reaction
		{
			// if we're just guarding
			if(damage > 0)
			{
				if(currAction == GUARD)
				{
					if(type == BRIGAND)
					{
						// run away!
						SetPrimaryAction(MOVE_TO, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
						tNextStateCheck = currTicks + TICKS_PER_SEC / 2;
					}
					else
					{
						// force search to "find" this enemy on next enemy search
						if((player == SIDE1 || player == SIDE2) && ((Unit *)pAttacker)->player == player)
						{
							// same-side attack, ignore it
						}
						else if(pBam->playerTypes[player] == PLAYER_COMPUTER &&
							((ai.action == AI_RETREAT) || (type == ACOLYTE && nearbyTransformer &&
							AMax(abs(tileX - pUnit->tileX), abs(tileY - pUnit->tileY)) > currRange)))
						{
							// AI unit busy w/ something more important than retalliation
						}
						else
							SearchForEnemiesPreset(pUnit);

						// if hit anim running
						if(currAnimState == ST_ANIM_HIT && !fHitAniAlreadyRunning)
							tNextStateCheck = currTicks + TICKS_PER_SEC;	// give hit ani time to run
					}
				}
				else if(currAction == MOVE_TO && player == SIDE3 && DoIHate(pUnit))
				{
					// BRIGANDs are non-violent - dont interrupt their current action to retalliate.
					if(type != BRIGAND && GetDistanceTo(pUnit, UNIT) < GetDistanceTo(primaryX, primaryY))
					{
						// go get him
						SearchForEnemiesPreset(pUnit);
						SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
					}
				}
			}

			// if currently fighting someone
			if((currAction == ATTACK || primaryAction == ATTACK) &&
				(pBam->playerTypes[player] != PLAYER_COMPUTER ||
				(pWorld->ai.CheckAggression() && pWorld->ai.CheckAggression())))
			{
				bool fGoGetHim = FALSE;
				// should we change targets?
		
				// exception cases first
				if(type == GORGON && pUnit)
				{

					// if we're not attacking a unit
					if(secondaryTargetType != UNIT)
					{
						fGoGetHim = TRUE;
					}
					else
					{
						// is current target immune to stoning?

						pUnit2 = (Unit *)ValidateSecondaryTarget();
						if(!pUnit2 || pUnit2->ImmuneToSkillFrom(this))
						{
							// is our attacker succeptable?
							if(!pUnit->ImmuneToSkillFrom(this))
							{
								fGoGetHim = TRUE;
							}
						}
					}
				}

				// if current target is not our primary target
				if(!fGoGetHim && secondaryTargetSerial != primaryTargetSerial)
				{

					// temp storage, we don't really assume this is a unit
					pUnit2 = (Unit *)pWorld->ValidateTarget(secondaryTargetSerial);

					if(!pUnit2)
					{
						fGoGetHim = TRUE;
					}
					else // if new attacker is closer than current targ
					if(GetDistanceTo(pUnit, UNIT) < GetDistanceTo((void *)pUnit2,
						secondaryTargetType))
					{
						// get him!
						fGoGetHim = TRUE;
					}
				}

				if(fGoGetHim)
				{
					// get him!
					tNextStateCheck = currTicks;

					#ifndef NDEBUG
					BamDebug.Out("\n ");
					#endif

					if(currAction == MOVE_TO)
					{
						// allow for re-centering
						SearchForEnemiesPreset(pUnit);
						SetAction(MOVE_TO, tileX, tileY);
					}
					else if(GetDistanceTo((void *)pUnit, UNIT) <= currRange)
					{
						// just beat on him
						SetAction(ATTACK, pUnit->tileX, pUnit->tileY);
					}
				}
			}
		}
	}
	else if(pAttacker)	// if hit by something else (non-unit)
	{
		// notify AI
		pWorld->ai.HitTerrain(this, (TerrainUnit *)pAttacker);
	}

	if(fCursorTracking || pWorld->lastUnitSerialDrawn == serialNum)
	{
		pWorld->UpdateHPGuageOnly(currHP, maxHP, player);
	}

	if(currAction == GUARD && movement == FLIER)
	{
		// we'll want to wake back up immediately afterwards to restore our
		// normal animation
		if(tNextStateCheck > currTicks + TICKS_PER_SEC / 2)
			tNextStateCheck = currTicks + TICKS_PER_SEC - 5;
	}
	#ifndef NDEBUG
	BamDebug.Out(" done\n");
	#endif

	return(currHP);
}

void
Unit::StartGuardMode(void)
{
	Unit		*pUnit;
	MapSpace	*pBldg;
	bool		fAlreadyGuarding;

	fInAction = TRUE;
	fActionDone = FALSE;
	fActionEnding = FALSE;

	if(type != JUGGERNAUT)
	{
		// not for JUGGERNAUT so that he responds less quickly - makes him look dumber.

		// look around for someone we should be killing first
		pUnit = SearchForEnemies();
		if(pUnit)
		{
			SearchForEnemiesPreset(pUnit);
			EndGuardMode();
			return;
		}
		pBldg = SearchForBuildings(TRUE);
		if(pBldg)
		{
			if(pBldg->h.owner == SIDE0 && primaryTargetSerial != pBldg->serialNum)
			{
				// dont go after neutral buildings unless explicitly told to
			}
			else
			{
				EndGuardMode();
				return;
			}
		}
	}

	// check for possible mount/rider relationship
//	if(mountSerial && type == WALKER)
//	{
//		pUnit = (Unit *)(pWorld->ValidateTarget(mountSerial));
//		pUnit->SetRider(this);
//		pUnit->UpdateRider();
//	}

	if(type == ACOLYTE)
	{
		fAlreadyGuarding = FALSE;

		// if already in/going to GUARD mode visually
		if(currAnimState == ST_ANIM_OBELISK || (currAnimState == ST_ANIM_HIT && currState == ST_OBELISK))
		{
			if(fAniFwd)
			{
				// dont cause an anim hiccup
				if(fAniDone)
					currState = ST_OBELISK;
				else currState = ST_OBELISKING;
				fAlreadyGuarding = TRUE;

				// make sure all foundation partners know we are available again
				pBldg = pWorld->map.GetStructure(tileX, tileY);
				if(pBldg && pBldg->IsFoundation())
				{
					// in case one of our partners was in transformation selection menu
					pBldg->CheckFoundationFilled();
				}
			}
			else
			{
				if(!fAniDone)
				{
					// ani running backwards - turn it around
					fAniFwd = TRUE;
					currState = ST_OBELISKING;
				}
				else
				{
					// run guard anim fwd
					RunAnimation(ST_ANIM_GUARD, 3);
					currState = ST_OBELISKING;
				}
			}
		}
		else
		{
			// run obelisk anim fwd
			RunAnimation(ST_ANIM_GUARD, 3);
			currState = ST_OBELISKING;
		}
		if(fCursorTracking && !fAlreadyGuarding)	// are we being watched?
			pWorld->DrawStatusBox();	// update portrait
		tAutoActionTimer = ATicks() + TICKS_PER_SEC * 6;
	}
	else
	{
		if(type == WRAITH || type == TORTOISE)
		{
			// cycle fwd then stop
			RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir);
		}
		else if(type == BRIGAND)
		{
			// cycle fwd then stop
			RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir);
			tAutoActionTimer = ATicks() + TICKS_PER_SEC * 20;
		}
		else if(type == WIZARD || type == JUGGERNAUT || type == GUARDIAN || movement == FLIER)
		{
			// cycle fwd and loop
			RunAnimation(ST_ANIM_GUARD, 3, TRUE, currDir);
		}
		else if(type == UNIT_WEIRD_WARD)
		{
			// run death ani backwards
			RunAnimation(ST_ANIM_DIE, 3, FALSE, currDir, TRUE, FALSE);
		}
		else
		{
			// strike a pose - we'll animate occasionally
			RunAnimation(ST_ANIM_GUARD, 0, FALSE, currDir, TRUE, FALSE);	// hold on last cel
		}
		currState = ST_NULL;
	}

	// in some cases, when a UNIT guards it becomes a guarding of the nearby building while in GUARD mode
	if(primaryAction == GUARD || primaryAction == MOVE_TO)
	{
		if(WithinRangeForPrimary())
		{
			if(player == SIDE1 || player == SIDE2 || type == GORGON || type == TROLL)
			{
				if(type == ACOLYTE && pBam->playerTypes[player] == PLAYER_COMPUTER)
				{
					// don't allow AI acolytes to be wasted on bldg defense.					
				}
				else
				{
					pBldg = SearchForBuildings(FALSE);
					if(pBldg)
					{
						pBldg->AddDefender(this);
					}
				}
			}
		}
	}
	if(fPackLeader)
		tAutoActionTimer = ATicks() + TICKS_PER_SEC * 20;
}

bool
Unit::DoIHate(MapSpace *pSpace)
{
	if(!pSpace || pSpace->h.currHP < 1)
		return(FALSE);

	if(type == JUGGERNAUT)
	{
		// JUGGERNAUTs attack all structures
		return(TRUE);
	}
	else if(player != SIDE1 && player != SIDE2)
	{
		// neutrals dont care
		return(FALSE);
	}
	else return(pWorld->unitLib.CheckEnemyFlag(player, pSpace->h.owner));
}

bool
Unit::DoIHate(Unit *pUnit)
{
	bool	fHate;
	
	if(!pUnit)
		return(FALSE);

	switch(type)
	{
		case ROC_EGG:
		case UNIT_WEIRD_WARD:
			// we dont hate anyone
			return(FALSE);

//		case BRIGAND:
//			// dont attack other BRIGANDs
//			if(pUnit->type == BRIGAND)
//				return(FALSE);
//			break;

		case GORGON:
			if(pUnit->primaryAction == GET_STONED)
				return(FALSE);
			break;

		case NYMPH:
			if(pUnit->primaryAction == FOLLOW && pUnit->fAutoControl && pUnit->primaryTargetSerial == serialNum)
			{
				// dont break the spell
				return(FALSE);
			}
			break;

		case FURY:
			// FURY's only attack other FURY's, and their own primary target
			if(pUnit->type != FURY && pUnit->serialNum != primaryTargetSerial)
				return(FALSE);
			break;

		case WOLF:
		case TROLL:
			// game play tweak - during single player mode, wolves don't care
			// about non-hostile AI players in fog.  (Yes, Im CHEATING! - V)
			if(bGlobal.storyLine == NETGAME)
				break;
			if(!(pWorld->vPort.fog[pUnit->tileY][pUnit->tileX] & FOG_CENTER))
				break;
			if(pUnit->currAction == ATTACK || pBam->playerTypes[pUnit->player]
				!= PLAYER_COMPUTER)
				break;

			// okay, ignore him. He's inconsequential.	
			return(FALSE);
	}

	switch(pUnit->type)
	{
		case WOLF:
		case TROLL:
			// game play tweak - during single player mode, if we're the AI then
			// ignore passive fogged wolves. (Yes, Im CHEATING! - V)
			if(bGlobal.storyLine == NETGAME)
				break;
			if(!(pWorld->vPort.fog[pUnit->tileY][pUnit->tileX] & FOG_CENTER))
				break;
			if(pUnit->currAction == ATTACK || pBam->playerTypes[player]
				!= PLAYER_COMPUTER)
				break;

			// okay, ignore him. He's inconsequential.	
			return(FALSE);

		case UNIT_WEIRD_WARD:
			// these units are never hated
			return(FALSE);

		case NYMPH:
			if(primaryAction == FOLLOW && tFearLureStonedTimer && primaryTargetSerial == pUnit->serialNum)
			{
				// dont attack our sweetheart!
				return(FALSE);
			}
			break;
	}

	switch(pUnit->primaryAction)
	{
		case DIE:
		case DISBAND:
		case TRANSFORM_TO_BLDG:
			// ignore units already doing this
			return(FALSE);
	}

	// SIDE3 units dont attack other SIDE3s of same class
	if(player == SIDE3 && pUnit->player == SIDE3 && pUnit->type == type)
		return(FALSE);

	fHate = pWorld->unitLib.CheckEnemyFlag(player, pUnit->player);

	return(fHate);
}

// Specify a unit for SearchForEnemies() to find that it would not find otherwise.
// Used when a distant enemy attacks us with a range weapon outside our own attack
// range, so that the next Search() will find him
void
Unit::SearchForEnemiesPreset(Unit *pUnit)
{
	searchForEnemiesPresetSerial = pUnit->serialNum;

	if(fStandGuard)
		StandGuard(FALSE);
}

bool
Unit::SearchForItems(void)
{
	int itemX, itemY;

	return(SearchForItems(itemX, itemY));
}

bool
Unit::SearchForItems(int &itemX, int &itemY)
{
	int	searchRange, x1, x2, y1, y2, loopX, loopY;
 
	switch(type)
	{
		case BRIGAND:
			searchRange = 2;
			break;
		default:
			searchRange = 1;
			break;
	}

	itemX = -1;
	itemY = -1;

	x1 = AMax(tileX - searchRange, 0);	x2 = AMin(tileX + searchRange, WORLD_WIDTH - 1);
	y1 = AMax(tileY - searchRange, 0);	y2 = AMin(tileY + searchRange, WORLD_HEIGHT - 1);

	for(loopY = y1; loopY <= y2; loopY++)
		for(loopX = x1; loopX <= x2; loopX++)
		{
			if(pWorld->itemMgr.CheckItem(loopX, loopY) && CanIMoveTo(loopX, loopY, TRUE))
			{
				if(itemX == -1 || GetDistanceTo(loopX, loopY) < GetDistanceTo(itemX, itemY))
				{
					itemX = loopX;
					itemY = loopY;
				}
			}
		}
	return(itemX == -1? FALSE: TRUE);
}

#define MAX_TARGETS	50
MapSpace *
Unit::SearchForBuildings(bool fEnemy, bool fSwitches)
{
	int		loopX, loopY, loop1, x1, y1, x2, y2, targetCount = 0, shortest,
					current, viewingRange;
	MapSpace* pTargets[MAX_TARGETS], *pSpace, *pSpace2;
	grip		gSpace, *pgSpaces;
	int16		*pMapSpaces, *pMapSpaces2;

	if(fSwitches)
	{
		viewingRange = 4;
	}
	else
	{
		viewingRange = AMax(currRange, 0);	// safety net, in case of negative range
		if(tBerserkTimer)
			viewingRange = AMax(WORLD_HEIGHT, WORLD_WIDTH);
	}
	x1 = AMax(tileX - viewingRange, 0);	x2 = AMin(tileX + viewingRange, WORLD_WIDTH - 1);
	y1 = AMax(tileY - viewingRange, 0); y2 = AMin(tileY + viewingRange, WORLD_HEIGHT - 1);

	pgSpaces = pWorld->map.gSpaces;
	pMapSpaces = pWorld->map.MapSpaces + x1 + y1 * WORLD_WIDTH;

	for(loopY = y1; loopY <= y2; loopY++, pMapSpaces += WORLD_WIDTH)
	{
		pMapSpaces2 = pMapSpaces;
		for(loopX = x1; loopX <= x2; loopX++, pMapSpaces2++)
		{
			gSpace = pgSpaces[*pMapSpaces2];
			if(!gSpace)
				continue;

			pSpace = ADerefAs(MapSpace, gSpace);

			if(fSwitches)
			{
				// looking for trip switches
				if(pSpace->h.func == FUNC_SWTRIP || pSpace->h.func == FUNC_SWPRESSURE)
				{
					pTargets[targetCount++] = pSpace;
				}
			}
			else if(pSpace->h.currHP)
			{
				if(fEnemy)
				{
					if(!DoIHate(pSpace))
						continue;

					// no point in caring about enemy buildings that are covered	
					if(pWorld->unitLib.GetUnitGrip(loopX, loopY))
						continue;

					if(pSpace->h.owner == SIDE0 && (type != JUGGERNAUT && player != SIDE2))
						continue;	// no one else attacks SIDE0 structs just for the hell of it
				}
				else
				{
					if(type == TROLL || type == GORGON)
					{
						// any building will do
					}
					else if(pSpace->h.owner != player)
					{
						// only friendly buildings will do
						continue;
					}
				}

				for(loop1 = 0; loop1 <= targetCount; loop1++)
				{
					if(loop1 == targetCount)
					{
						// end of list reached
						// added on to end of list
						pTargets[targetCount++] = pSpace;
						break;
					}
					else if(pTargets[loop1] == pSpace)
					{
						// prevent duplicate entries
						break;
					}
				}
			}
		}
	}

	if(!targetCount)
		return(NULL);

	if(fStandGuard && fEnemy)
	{
		// time to wake up!
		StandGuard(FALSE);
	}

	if(targetCount == 1)
		return(pTargets[0]);

	if(fSwitches)
	{
		// randomly pick one
		pSpace = pTargets[ARandom(targetCount)];
	}
	else
	{
		// find nearest
		pSpace = pTargets[0];
		shortest = GetDistanceTo(pSpace, BUILDING);

		for(loopX = 1; loopX < targetCount; loopX++)
		{
			pSpace2 = pTargets[loopX];
			current = GetDistanceTo(pSpace2, BUILDING);
			if(current == 0)
			{
				// ignore
				continue;
			}

			if(current < shortest || (current == shortest && !ARandom(3)))
			{
				pSpace = pSpace2;
				shortest = current;
			}
		}
	}
	return(pSpace);
}

#define SEARCH_MAX_PATIENTS	10
Unit *
Unit::SearchForPatients(void)
{
	int		x1, y1, x2, y2, viewingRange, loopX, loopY, targetCount, targetCount2, 
					bestDistance, distance2;
	Unit		*pUnit, *pUnit2, *pTargets[SEARCH_MAX_PATIENTS], *pTargets2[SEARCH_MAX_PATIENTS];
	grip		gUnit, *pgUnitMap, *pgUnitMap2;

	viewingRange = 1;
	targetCount = 0;

	x1 = AMax(tileX - viewingRange, 0);	x2 = AMin(tileX + viewingRange, WORLD_WIDTH - 1);
	y1 = AMax(tileY - viewingRange, 0); y2 = AMin(tileY + viewingRange, WORLD_HEIGHT - 1);

	pgUnitMap = &pWorld->unitLib.gUnitMap[y1][x1];
	for(loopY = y1; loopY <= y2; loopY++, pgUnitMap += WORLD_WIDTH)
	{
		for(pgUnitMap2 = pgUnitMap, loopX = x1; loopX <= x2; loopX++, pgUnitMap2++)
		{
			if(!*pgUnitMap2)
				continue;

			gUnit = *pgUnitMap2;
			if(gUnit == gSelf)
				continue;
			pUnit = ADerefAs(Unit, gUnit);

			if(pUnit->player != player || pUnit->maxHP - pUnit->currHP < 5)
				continue;

			if(pUnit->ImmuneToSkillFrom(this))
				continue;

			pTargets[targetCount++] = pUnit;
		}
	}

	switch(targetCount)
	{
		case 0:
			return(NULL);

		case 1:
			return(pTargets[0]);
		
		default:
			// pick best
			pUnit = (Unit *)pTargets[0];		// temp ptr to best unit
			bestDistance = GetDistanceTo(pUnit, UNIT);
			pTargets[targetCount++] = pUnit;

			targetCount2 = 0;
			for(loopX = 1;	loopX < targetCount;	loopX++)
			{
				pUnit2 = (Unit *)pTargets[loopX];

				// is next closer than best?
				distance2 = GetDistanceTo(pUnit2, UNIT);
				if(distance2 < bestDistance)
				{
					targetCount2 = 0;
					// new unit is closer than best - restart final viable targets list
				}
				if(distance2 <= bestDistance)
				{
					pUnit = pUnit2;
					bestDistance = distance2;
					pTargets2[targetCount2++] = pUnit;
				}
			}

			if(targetCount2 == 1)
				return(pTargets2[0]);

			// pick weakest from remaining units
			pUnit = pTargets2[0];
			for(loopX = 1; loopX < targetCount2; loopX++)
			{
				if(pUnit2->currHP < pUnit->currHP)
					pUnit = pUnit2;
			}
			return(pUnit);
	}
}

#define SEARCH_MAX_ENEMIES	100
Unit	*pTargets[SEARCH_MAX_ENEMIES + 1], *pTargets2[SEARCH_MAX_ENEMIES + 1];
Unit *
Unit::SearchForEnemies(void)
{
	int		x1, y1, x2, y2, viewingRange, loopX, loopY, targetMaxHP, targetCount, targetCount2, 
					bestDistance, distance2;
	Unit		*pUnit, *pUnit2;
	grip		gUnit, *pgUnitMap, *pgUnitMap2;
	uint32	unitSerialNum;
	List		*pList;

	viewingRange = AMax(currRange, 0);	// safety net, in case of negative range
	switch(type)
	{
		case ROC_EGG:
		case UNIT_WEIRD_WARD:
			// for these, dont even search.  Speed optimization.
			return(NULL);

		case WOLF:
			if(!fPackLeader)
				return(NULL);		// only wolf leaders look for targets

			// wolfpack leader looks beyond his attack range for targets
	
			viewingRange = 3;
			if(!fPackLeader)
			{
				pUnit = (Unit *)(pWorld->ValidateTarget(packLeaderSerial));
				if(!pUnit)
				{
					// umm, error.  Deal with it.
					break;
				}
			}
			else pUnit = this;	// look at self
		
			// calculate total HP of our pack
			targetMaxHP = pUnit->currHP;

			pList = ADerefAs(List, pUnit->glFollowers);
			unitSerialNum = (uint32)pList->FirstValue();
			while(unitSerialNum)
			{
				pUnit = (Unit *)(pWorld->ValidateTarget(unitSerialNum));
				if(pUnit)
					targetMaxHP += pUnit->currHP;

				unitSerialNum = (uint32)pList->NextValue();
			}
			targetMaxHP /= 2;		// take half
			break;
		
		case BRIGAND:
			viewingRange = 2;
			break;

		default:
			targetMaxHP = 9999;	// any target
			break;
	}
	if(primaryAction == BERSERK)
		viewingRange = AMax(WORLD_HEIGHT, WORLD_WIDTH);

	targetCount = 0;
	// if we have a preset enemy (presumably outside search range)
	if(searchForEnemiesPresetSerial)
	{
		pUnit = (Unit *)(pWorld->ValidateTarget(searchForEnemiesPresetSerial));
		if(pUnit)
		{
			pTargets[targetCount++] = pUnit;
			// cut search short
			viewingRange = 0;
		}
		searchForEnemiesPresetSerial = NULL;
	}

	x1 = AMax(tileX - viewingRange, 0);	x2 = AMin(tileX + viewingRange, WORLD_WIDTH - 1);
	y1 = AMax(tileY - viewingRange, 0); y2 = AMin(tileY + viewingRange, WORLD_HEIGHT - 1);

	pgUnitMap = &pWorld->unitLib.gUnitMap[y1][x1];
	for(loopY = y1; loopY <= y2; loopY++, pgUnitMap += WORLD_WIDTH)
	{
		for(pgUnitMap2 = pgUnitMap, loopX = x1; loopX <= x2; loopX++, pgUnitMap2++)
		{
			if(!*pgUnitMap2)
				continue;

			gUnit = *pgUnitMap2;
			if(gUnit == gSelf)
				continue;
			pUnit = ADerefAs(Unit, gUnit);

			if(!DoIHate(pUnit))
			{
				continue;
			}
			
			if(pUnit->currHP > targetMaxHP)
			{
				continue;
			}

			// AI will ignore SIDE3 units if they're out of striking range
			if(pUnit->player == SIDE3 && pBam->playerTypes[player] == PLAYER_COMPUTER)
			{
				if(type == JUGGERNAUT)
				{
					if(GetDistanceTo(pUnit, UNIT) > pUnit->currRange)
						continue;
				}
				else if(AMax(abs(tileX - pUnit->tileX), abs(tileY - pUnit->tileY))
				  > pUnit->currRange)
					continue;

				// if we're sleeping, ignore him anyway.  If he attacks us, he'll
				// be our preset and this search loop won't run anyway.
				if(fStandGuard)
					continue;
			}

			pTargets[targetCount++] = pUnit;
			if(targetCount >= SEARCH_MAX_ENEMIES)
			{
				// prevent array overflow, stop search
				loopY = 9999;
				break;
			}
		}
	}

	if(targetCount && fStandGuard)
	{
		// time to wake up!
		StandGuard(FALSE);
	}

	switch(targetCount)
	{
		case 0:
			return(NULL);

		case 1:
			return(pTargets[0]);
		
		default:
			// pick best
			pUnit = (Unit *)pTargets[0];		// temp ptr to best unit
			bestDistance = GetDistanceTo(pUnit, UNIT);
			pTargets[targetCount++] = pUnit;

			targetCount2 = 0;
			for(loopX = 1;	loopX < targetCount;	loopX++)
			{
				pUnit2 = (Unit *)pTargets[loopX];

				switch(type)
				{
					case BRIGAND:
						// if 'best' has an item
						if(pUnit->item)
						{
							// but next doesnt
							if(!pUnit2->item)
							{
								// over-ride distance check - item is more important
								continue;
							}
						}
						// else if best doesnt have an item, and next does
						else if(pUnit2->item)
						{
							// over-ride distance check - item is more important
							pUnit = pUnit2;
							pTargets2[targetCount2++] = pUnit;
							continue;
						}
						break;

					case GUARDIAN:
						// if 'best' is burning or cant be burnt
						if(!pWorld->map.FireCanBurnAt(pUnit->tileX, pUnit->tileY) ||
							(pWorld->vPort.GetTerrainUnit(pUnit->tileX, pUnit->tileY))->activeType == ACTIVE_ARCH_FIRE)
						{
							// and next isnt burning and CAN be burnt
							if(pWorld->map.FireCanBurnAt(pUnit2->tileX, pUnit2->tileY) &&
								pWorld->vPort.GetTerrainUnit(pUnit2->tileX, pUnit2->tileY)->activeType != ACTIVE_ARCH_FIRE)
							{
								// prefer new unit (cuz he can be torched)
								pUnit = pUnit2;
								targetCount2 = 0;
								pTargets2[targetCount2++] = pUnit;
								continue;
							}
						}
						break;
				}

				// is next closer than best?
				distance2 = GetDistanceTo(pUnit2, UNIT);
				if(distance2 < bestDistance)
				{
					targetCount2 = 0;
					// new unit is closer than best - restart final viable targets list
				}
				if(distance2 <= bestDistance)
				{
					pUnit = pUnit2;
					bestDistance = distance2;
					pTargets2[targetCount2++] = pUnit;
				}
			}

			if(targetCount2 == 1)
				return(pTargets2[0]);

			// pick weakest from remaining units
			pUnit = pTargets2[0];
			for(loopX = 1; loopX < targetCount2; loopX++)
			{
				if(pUnit2->currHP < pUnit->currHP)
					pUnit = pUnit2;
			}
			return(pUnit);
	}
}

void
Unit::EndGuardMode(void)
{
	MapSpace	*pSpace;

	// if OBELISK, transform to walker first
	if(type == ACOLYTE)
	{
		SetActionAvailable(SKILL, FALSE);

		if(currState == ST_OBELISK)
		{
			RunAnimation(ST_ANIM_OBELISK, 3, FALSE, DNULL, TRUE, FALSE);
			currState = ST_OBELISKING;
			SetCurrTerrain(currTerrain);	// cause regular (non-OBELISK) attribs to be set
			if(fTerrainAni)
				StopTerrainAni();		// in case the above line started one by mistake
			if(fCursorTracking)
				pWorld->DrawStatusBox();

			tNextStateCheck = ATicks() + TICKS_PER_SEC;
		}
		else if(currState == ST_OBELISKING)
		{
			if(currAnimState == ST_ANIM_OBELISK)
			{
				if(fAniFwd)	// if going wrong dir
				{
					if(fAniDone)
						RunAnimation(ST_ANIM_OBELISK, 3, FALSE, DNULL, TRUE, FALSE);
					else fAniFwd = FALSE;
				}
				// else allow to continue
			}
			else if(currAnimState == ST_ANIM_HIT)
			{
				// hit while obelisk'ing
				if(scrim.resNum == ANIM_OBELISK_IS_HIT)
				{
					// full visual obelisk state.  Reverse out.
					RunAnimation(ST_ANIM_OBELISK, 3, FALSE, DNULL, TRUE, FALSE);
				}
				// else visual acolyte state, assume guard mode already over
			}
		}
	}
	else if(type == TORTOISE)
	{
 		// reverse guard anim
		RunAnimation(ST_ANIM_GUARD, 5, FALSE, currDir, TRUE, FALSE);
		currState = ST_OBELISKING;
		tNextStateCheck = ATicks() + TICKS_PER_SEC * 2;
	}
	else if(type == BRIGAND)
	{
 		// reverse guard anim
		RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir, TRUE, FALSE);
		currState = ST_OBELISKING;
		tNextStateCheck = ATicks() + TICKS_PER_SEC;
	}
	else
	{
		currState = ST_NULL;
		tNextStateCheck = ATicks();
	}
	fActionEnding = TRUE;

	pSpace = pWorld->map.GetStructure(tileX, tileY);
	if(pSpace)
	{
		pSpace->CheckFoundationFilled();
	}

	pSpace = SearchForBuildings(FALSE);
	if(pSpace)
	{
		pSpace->RemoveDefender(this);
	}
}

void
Unit::CycleGuardMode(void)
{
	ticks_t	currTicks = ATicks();
	bool		fStartToWander;
	int		tempX, tempY, loop1, int1;
	MapSpace	*pSpace;
	Unit		*pUnit;

//	BamDebug.Out("Unit(%d)@%d::CycleGuardMode() - name=%s pri=%d curr=%d\n", serialNum, (int)currTicks,
//		unitName, (int)primaryAction,	(int)currAction);

	if(!fInAction)		// if GUARD mode not started yet
	{
//		BamDebug.Out("Unit::!fInAction\n");

		StartGuardMode();
//		BamDebug.Out("Unit::!fInAction2\n");
		if(type == TORTOISE)
			tNextStateCheck = currTicks + TICKS_PER_SEC * 5 + ARandom(5);
		else
			tNextStateCheck = currTicks + TICKS_PER_SEC * 1 + ARandom(5);
		return;
	}
	else
	{
//		BamDebug.Out("Unit::fInAction == TRUE\n");
	}

	switch(type)
	{
		case ACOLYTE:
			// if transformation-to-obelisk is running
			if(currState == ST_OBELISKING && fAniDone && !fActionEnding)
			{
				// if they got or or something during transform-to-obelisk
				if(currAnimState != ST_ANIM_OBELISK)
				{
					RunAnimation(ST_ANIM_GUARD);
					SetCel(scrim.numCels);	// jump to last cel
				}

				currState = ST_OBELISK;			// transformation complete
				SetCurrTerrain(currTerrain);	// cause OBELISK attributes to be set
				SetActionAvailable(SKILL, TRUE);
				if(fCursorTracking)
					pWorld->DrawStatusBox();

				// check for foundation
				MapSpace	*pSpace = pWorld->map.GetStructure(tileX, tileY);
				if(pSpace)
					pSpace->CheckFoundationFilled();
			}

			if(tAutoActionTimer && currTicks >= tAutoActionTimer)
			{
				// mana bonus from meditation
				if((pBam->playerTypes[player] == PLAYER_COMPUTER && currMana < MANA_ACOLYTE_MAX / 2) ||
					currMana < MANA_ACOLYTE_MAX)
				{
					currMana = AMin((currMana + MANA_MEDITATE), MANA_ACOLYTE_MAX);
					if(fCursorTracking && player == pWorld->playerSide)
						pWorld->SetVerbButtons(pWorld->vPort.targType, pWorld->vPort.targSerial);
				}
				else if(CheckActionAvailable(SKILL))
				{
					// auto-transfer mana, trying not to override primaryAction
					SetAction(SKILL);
					return;
				}

				tAutoActionTimer = currTicks + TICKS_PER_SEC * 6;
			}
			break;

		case UNIT_WEIRD_WARD:
			if(fAniDone)
			{
				// birth ani must have finished - start regular cycle ani
				RunAnimation(ST_ANIM_GUARD, 3, TRUE, currDir);
			}
			break;

		case WRAITH:	
			if(fAniDone && fAniFwd && currAnimState == ST_ANIM_GUARD)
			{
				// if fwd cycle has ended, run reverse
				RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir, TRUE, FALSE);
			}
			break;
	}

	if(fActionEnding && fAniDone)
	{
//		BamDebug.Out("Unit::ActionEnding && fAniDone\n");
		fActionDone = TRUE;
		return;
	}

	if(movement == FLIER && fAniDone)
	{
		// whoops!  cant allow animation to stop on these guys!
		// cycle fwd and loop
		RunAnimation(ST_ANIM_GUARD, 3, TRUE, currDir);
	}

	if(currTicks < tNextStateCheck)		// if not time for re-think
	{
//		BamDebug.Out("Unit::currTicks < tNextStateCheck\n");
		return;
	}

	if(fActionEnding)
	{
		// delay over - presume cleanup to be done
//			BamDebug.Out("Unit::fActionEnding\n");
			fActionDone = TRUE;
			return;
	}

	// most periodic re-think code goes here
	tNextStateCheck = currTicks + TICKS_PER_SEC * (ARandom(3) + 1) + ARandom(TICKS_PER_SEC);

	// kludgy bug patch
	// has our entry in the grip map been walked on?
	if(pWorld->unitLib.gUnitMap[tileY][tileX] != gSelf)
	{
		UnitDebug.Out("\n%s(%d,%d) repairing grip map", unitName, tileX, tileY);
		pWorld->unitLib.PlaceUnitGrip(gSelf, tileX, tileY);
	}

	switch(type)
	{
		case BANSHEE:
			if(currTicks > tAutoActionTimer)
			{
				// time to wail
				fActionDone = TRUE;
				return;
			}
			break;

		case UNIT_WEIRD_WARD:
//			if(currTicks > tUnitBirth + WEIRDWARD_LIFESPAN)
//			{
//				Die();
//				return;
//			}
			break;

		case JUGGERNAUT:
			EndGuardMode();
			return;	// JUGGERNAUT never sits still long

		case GORGON:
			if(!ARandom(12))
			{
				// get up and run around
				EndGuardMode();
				return;
			}
			break;

		case CLERIC:
		case PALADIN:
			// AI code
			if(pBam->playerTypes[player] != PLAYER_COMPUTER)
				break;

			// we're a healer - does a neighor need us?	
			if(pWorld->ai.ShouldWeUseSkill(this) && CheckActionAvailable(SKILL))
			{
				pUnit = SearchForPatients();
				if(pUnit)
				{
					SetAction(SKILL, pUnit->tileX, pUnit->tileY);
					return;
				}
			}
			break;
	}

	fStartToWander = FALSE;
	if(fPackFollower)
	{
//		BamDebug.Out("Unit::fPackFollower\n");
		if(FollowTheLeader())	// if change in our current action
		{
//			UnitDebug.Out("Guard():: guard mode ended - following leader\n");
			return;
		}
		// else no change - staying in guard mode
	}
	// else if we're a pack leader && at rest
	else if(fPackLeader && (currTicks > tAutoActionTimer))
	{
		tAutoActionTimer = NULL;
//		BamDebug.Out("Unit::fPackLeader 20 sec roam\n");
		// every 20 seconds, start roaming
		fStartToWander = TRUE;
	}
	else switch(type)
	{
		case BANSHEE:
			// only wanders after having been hit once.
			// Kludgy but effective.
			if(currHP != maxHP || lastAttackTicks)
				if((currTicks % (TICKS_PER_SEC * 10)) < TICKS_PER_SEC)
					fStartToWander = TRUE;
			break;

		case BRIGAND:
			if(currTicks > tAutoActionTimer)
			{
				tAutoActionTimer = 0;
				fStartToWander = TRUE;
			}
			break;

		case ROC:
			if(primaryAction == GUARD && pWorld->itemMgr.CheckItem(primaryX, primaryY))
				fStartToWander = FALSE;
			else fStartToWander = TRUE;
			break;
	}

	if(fStartToWander)
	{
//		BamDebug.Out("Unit::fStartToWander\n");

		bool	fWanderTarget = FALSE;
		int32	destX, destY, deltaX, deltaY, temp1, newDir, loop1, quadrantLoop;

		if(lastPrimaryX < 0)	// if havent wandered before
		{
			wanderDir = (direction)ARandom(8);		// just pick a random direction
		}

		quadrantLoop = 0;
		while(!fWanderTarget && quadrantLoop < 8)
		{
			newDir = wanderDir;

			// randomly pick a course change, with pro-forward tendencies
			temp1 = ARandom(100);
			if(ABetween(temp1, 0, 29))				// straight, no change in direction
				newDir = newDir;
			else if(ABetween(temp1, 30, 49))		// bank left
				newDir++;
			else if(ABetween(temp1, 50, 69))		// bank right
				newDir--;
			else if(ABetween(temp1, 70, 77))		// hard left
				newDir += 2;
			else if(ABetween(temp1, 78, 85))		// hard right
				newDir -= 2;
			else if(ABetween(temp1, 85, 90))		// back-left
				newDir += 3;
			else if(ABetween(temp1, 91, 95))		// back-right
				newDir -= 3;
			else // 96 - 99							// reverse
				newDir += 4;

			newDir = ((wanderDir + newDir + 8) % 8);	// direction wrapping
				// new trial direction chosen - now find avail dest in that direction
			loop1 = 0;
			while(loop1 < 20 && !fWanderTarget)	// pick random X/Ys until one found within target area
			{
				loop1++;
				destX = ARandom(WORLD_WIDTH);
				destY = ARandom(WORLD_HEIGHT);
				deltaX = destX - tileX;
				deltaY = destY - tileY;
				// no need to worry about Y/0 problem - slopes only computed for diagonals
				switch(newDir)
				{
					// validate by quadrants
					case DOWN_RIGHT:
						if(deltaX > 0 && deltaY > 0)
							fWanderTarget = TRUE;	// target is valid - stop searching
						break;
					case UP_RIGHT:
						if(deltaX > 0 && deltaY < 0)
							fWanderTarget = TRUE;
						break;
					case UP_LEFT:
						if(deltaX < 0 && deltaY < 0)
							fWanderTarget = TRUE;
						break;
					case DOWN_LEFT:
						if(deltaX < 0 && deltaY > 0)
							fWanderTarget = TRUE;
						break;
					case DOWN:
						if(deltaY > abs(deltaX))
							fWanderTarget = TRUE;
						break;
					case UP:
						if(deltaY < -abs(deltaX))
							fWanderTarget = TRUE;
						break;
					case RIGHT:
						if(deltaX > abs(deltaY))
							fWanderTarget = TRUE;
						break;
					case LEFT:
						if(deltaX < -abs(deltaY))
							fWanderTarget = TRUE;
						break;
					default:
						APanic("Unit::CycleGuardMode() - error, invalid dir for wander!\n");
						break;
				}
				if(fWanderTarget)	// if potential dest found in valid quadrant
				{
					if(!CanIMoveTo(destX, destY, TRUE))		// if terrain not passable
						fWanderTarget = FALSE;	// keep searching
				}
			}
			quadrantLoop++;	// only check so many quadrants before giving up
		}
		if(fWanderTarget)
		{	
			SetPrimaryAction(MOVE_TO, destX, destY);
			wanderDir = (direction)newDir;	// to maintain directional trend
		}
		else
		{
			// else continue guarding
			UnitDebug.Out("\nUnit(g%d)::CycleGuardMode() - warning, no wander targets", gSelf);
		}
	}	
	else
	{
	}

	// sometimes a unit has to stop and guard, because his path is blocked.
	// We are guarding now, but if we still have an ultimate destination, here we
	// try again (and again, and again) to reach target
		
	switch(primaryAction)
	{
		case GUARD:
		case MOVE_TO:
	 		if(type == TORTOISE)
			{
				// TORTOISEs can change their primary X,Y while guarding, regardless
				// of having reached their current primary X,Y first.
				// Gives them a dumber, wandering look.
				if(!ARandom(6))
				{
					do
					{
						tempX = ARandom(WORLD_WIDTH);
						tempY = ARandom(WORLD_HEIGHT);
					} while(!CanIMoveTo(tempX, tempY));
					SetPrimaryAction(MOVE_TO, tempX, tempY);
					return;
				}
				break;
			}

			// if not where we ultimately want to be
			fPrimarySatisfied = WithinRangeForPrimary();
	 			
			if(!fPrimarySatisfied && !ARandom(3))
			{
				fStopAdjacent = FALSE;
				#ifndef NDEBUG
				if(fCursorTracking)
					UnitDebug.Out("\nUnit(%s s%d)::CycleGuard() - test repath",
						unitName, serialNum);
				#endif

				pathLength = FindPathTo((coord)primaryX, (coord)primaryY, pathArray, fStopAdjacent, TRUE);
				curPathPosition = 0;
				if(pathLength > 0)	// if at least a partial path is avail
				{
					PeekNextTarget(tempX, tempY);
					// wide open?
					if(CanIMoveTo(tempX, tempY, TRUE))
					{
						EndGuardMode();
						return;
					}
					// blocked by one of ours?
//					if(CanIMoveTo(tempX, tempY, FALSE))
//					{
//						// a friendly is blocking us - is he just sitting around?
//						pUnit = pWorld->unitLib.GetUnit(tempX, tempY);
//						if(pUnit->currAction == GUARD && !pUnit->fAutoControl)
//						{			
//							// give him a kick, maybe we'll be able to move on next cycle
//							for(loop1 = 0, loop2 = ARandom(8); loop1 < 8; loop1++, loop2 = (loop2 + 1) % 8)
//							{
//								tempX = pUnit->tileX + offsets[loop2][0];
//								tempY = pUnit->tileY + offsets[loop2][0];
//								if(pUnit->CanIMoveTo(tempX, tempY, TRUE))
//								{
//									// step aside!
//									pUnit->SetAction(MOVE_TO, tempX, tempY);
//									break;
//								}
//							}
//						}
//					}
				}
				
				if(pBam->playerTypes[player] == PLAYER_COMPUTER)
				{
					if(pWorld->map.rMapNum % 9310 < 10)
					{
						// look for a tripswitch to try
						pSpace = SearchForBuildings(FALSE, TRUE);
						if(pSpace && pSpace->h.xPos != ai.lastSwitchX && pSpace->h.yPos != ai.lastSwitchY)
						{
							// remember which one, so if we can't path to it, we wont re-try if it's the only
							// switch we can find.
							ai.lastSwitchX = pSpace->h.xPos;
							ai.lastSwitchY = pSpace->h.yPos;

							// found a new one, but can we get to it?
							curPathPosition = 0;
							pathLength = FindPathTo((coord)pSpace->h.xPos, (coord)pSpace->h.yPos, pathArray,
								FALSE, FALSE, FALSE, &tempX, &tempY);
							if(pathLength > 0 && tempX == pSpace->h.xPos && tempY == pSpace->h.yPos)
							{
								// found one, and a complete path is avail - try it
								SetAction(MOVE_TO, tempX, tempY);
								return;
							}
						}
					}
				}

				// okay, there's nothing we can do about accomplishing our goal.
				// Is it time to give up yet?
				if(currTicks - tCurrActStarted > TICKS_PER_SEC * 120)
				{
					if(pBam->playerTypes[player] == PLAYER_COMPUTER)
					{
						if(ai.action == AI_TRANS_FOUNDATION ||	ai.action == AI_TRANSFORM)
						{
							// these missions may require some waiting time - s'okay
							;
						}
						else
						{
							// make ourselves available to AI again, and quit trying to find a path
							SetPrimaryAction(GUARD, tileX, tileY);
							ai.action = AI_NONE;
							ai.data = 0;
							pWorld->ai.ReportUnit(this, "GUARD-TIMEOUT ABANDON", TRUE);
							return;
						}
					}
				}
			}
			else
			{
//				if(type == TORTOISE)
//				{
//					// stop guarding and pick next "bus stop"
//					EndGuardMode();
//					return;
//				}
			}

			if(primaryAction == MOVE_TO)
			{
				// if we actually have an ulterior target and can restore that target
				if(defendTargetAction)
				{
					// return to that ulterior target
					EndGuardMode();
					return;
				}
			}

			// alright, so no enemies in sight.

			// ** OLD bldg guard mechanism below - now handled via pBldg->AddDefender()
			// is anyone attacking our guard subject?
//			if(type == GORGON && primaryAction == GUARD)
//				switch(primaryTargetType)
//				{
//					case BUILDING:
//						pSpace = (MapSpace *)ValidatePrimaryTarget();
//						// if building was attacked recently
//						if(pSpace && (currTicks - pSpace->lastAttackTicks < TICKS_PER_SEC * 5))
//						{
//							// ah ha!  Get him!
//							SearchForEnemiesPreset((Unit *)(pWorld->ValidateTarget(pSpace->lastAttackerSerial)));
//							EndGuardMode();
//							return;
//						}
//						break;
//				}
			break;

		case AVOID:
			if(!ValidatePrimaryTarget())
				EndGuardMode();
			
			// if not far enough away
			UpdatePrimaryTarget();
			if(!WithinRangeForPrimary())
			{
				EndGuardMode();
				return;
			}
			break;

		case ATTACK:
		case BERSERK:
		case USE_ITEM:
		case FOLLOW:
		case SKILL:
			if(!ValidatePrimaryTarget())
			{
				EndGuardMode();
				return;
			}

			UpdatePrimaryTarget();

			if(ARandom(2) && !WithinRangeForPrimary())
			{
				// try to get through
				if(primaryTargetType == BUILDING)
				{
					loop1 = 2;	// try twice
					pSpace = (MapSpace *)ValidatePrimaryTarget();
					if(!pSpace)	// safety net
						loop1 = 1;
				}
				else loop1 = 1;	// try once

				// can we get any closer?
				for(tempX = primaryX, tempY = primaryY; loop1 > 0; loop1--)
				{
					pathLength = FindPathTo(tempX, tempY, pathArray, TRUE, TRUE);
					curPathPosition = 0;
					PeekNextTarget(tempX, tempY);
					if(pathLength > 0 && CanIMoveTo(tempX, tempY, TRUE))
					{
						// is this target different from our original target?
						if(tempX != primaryX || tempY != primaryY)
						{
							// then it must be preferable - switch it
							primaryX = tempX;
							primaryY = tempY;
						}
						EndGuardMode();
						return;
					}
					else if(loop1 > 1)	// any more tries after current one?
					{
						// pick another tile of the building to try instead
						int1 = ARandom(pSpace->size);
						tempX = pSpace->tiles[int1][0];
						tempY = pSpace->tiles[int1][1];
					}
				}
				
				// alright, that failed.  Can we find a trip switch to play with?
				if(pWorld->map.rMapNum == 9310 || pWorld->map.rMapNum == 9315)
				{
					// look for a tripswitch nearby to try
					pSpace = SearchForBuildings(FALSE, TRUE);
					if(pSpace)
					{
						// can we get to it?
						curPathPosition = 0;
						pathLength = FindPathTo((coord)pSpace->h.xPos, (coord)pSpace->h.yPos, pathArray,
							FALSE, FALSE, FALSE, &tempX, &tempY);
						if(pathLength > 0 && tempX == pSpace->h.xPos && tempY == pSpace->h.yPos)
						{
							// found one, and a complete path is avail - try it
							SetAction(MOVE_TO, tempX, tempY);
							return;
						}
					}
				}

				// okay, there's nothing we can do about accomplishing our goal.
				// Is it time to give up yet?
				if(currTicks - tCurrActStarted > TICKS_PER_SEC * 60)
				{
					// make ourselves available to AI again, and quit trying to find a path
					SetPrimaryAction(GUARD, tileX, tileY);
					
					if(pBam->playerTypes[player] == PLAYER_COMPUTER)
					{
						ai.action = AI_NONE;
						ai.data = 0;
						pWorld->ai.ReportUnit(this, "GUARD:TIMEOUT_ABANDON", TRUE);
					}
					return;
				}
			}
			break;
	}

	// look around for enemy units
	int32	itemX, itemY;

	pUnit = SearchForEnemies();
	if(type != JUGGERNAUT && pUnit)
	{
		bool	fAttack = TRUE;

		if(pBam->playerTypes[player] == PLAYER_COMPUTER && pUnit->player == SIDE3)
		{
			if(GetDistanceTo(pUnit, UNIT) > 1)
			{
				// nevermind - let it live
				fAttack = FALSE;
			}
		}

		if(fAttack)
		{
			// if we're AI and the JUGGERNAUT is after us
			if(pUnit->type == JUGGERNAUT && pBam->playerTypes[player] == PLAYER_COMPUTER)
			{
				// run away!
				pSpace = pWorld->ai.FindClosestBldg(TOTALFUNCTIONS, player, tileX, tileY);
				if(pSpace)
				{
					// go guard a bldg
					tempX = pSpace->h.xPos;
					tempY = pSpace->h.yPos;
				}
				else
				{
					// go home
					tempX = pWorld->map.portalCoords[player][0];
					tempY = pWorld->map.portalCoords[player][1];
				}
				// the JUGGERNAUT is (sorta) on our side - go somewhere else
				pUnit->ai.action = AI_NONE;
				SetPrimaryAction(MOVE_TO, tempX, tempY);
				return;
			}

			//found an enemy unit - goooo get 'em Flash!  Kyu! Kyu! Kyu!
			EndGuardMode();	// - will cause CheckPrimary() to search for enemies

			// put unit into preset pos for when re-search occurs at next level up
			SearchForEnemiesPreset(pUnit);
			return;
		}
	}

	pSpace = SearchForBuildings(TRUE);
	if(pSpace)
	{
		if(pSpace->h.owner == SIDE0 && primaryTargetSerial != pSpace->serialNum)
		{
			// just another neutral building - dont attack it, even if we do hate them
		}
		else
		{
			// attack enemy building
			EndGuardMode();
			return;
		}
	}

	if(type == BRIGAND && SearchForItems(itemX, itemY))
	{
		EndGuardMode();
		return;
	}

	// nothing to do?  Well, maybe fidget around
	switch(movement)
	{
		case WALKER:
			if(fAniDone)
	 		{
				// some units, having stopped their guard animation, should re-start it
				switch(type)
				{
					// should wizards be in this list too?
					case ACOLYTE:
					case BRIGAND:
						break;
	
					case JUGGERNAUT:
						RunAnimation(ST_ANIM_GUARD, 3, TRUE, currDir);
						break;

					default:
//						BamDebug.Out("CycleGuardMode(%s)\n", unitName);	
						if(!ARandom(3))
						{
							RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir);
							// when anim is done, last cel is the hold cel for walkers anyway,
							// so we can just leave it there
						}
						break;
				}
			}
			else
			{
				// some units, while running their guard animation (which cycles), should
				// pick a new direction to face.
				switch(type)
				{
					case WIZARD:
					case GUARDIAN:
						if(!ARandom(3))
							RunAnimation(ST_ANIM_GUARD, 3, TRUE, currDir);
						break;
				}
			}
			break;

		case FLOATER:
			if(fAniDone)
				switch(type)
				{
					case NYMPH:
					case WRAITH:
						if(!ARandom(3))
							RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir, TRUE, TRUE);
						break;

//					case BANSHEE:	// handled by autoActionTimer now
//						if(!ARandom(5))
//						{
//							SetPrimaryAction(SKILL);
//							return;
//						}
//						break;
  				}
  			break;

		case FLIER:
			if(!ARandom(3))
			{
				// change dir in mid-flight.  Looks smart.
				// cycle fwd and loop
				RunAnimation(ST_ANIM_GUARD, 3, TRUE, currDir);
			}
			break;			
	}
	return;
}

const int move2AttackTicks[] = 
	{0, 5, 4, 3, 2, 2, 2, 2, 2, 2, 2};
const int move2ticks[] =
	{0, 4, 3, 3, 2, 1, 1, 1, 1, 1, 1};
// 0  1  2  3  4  5  6
const int move2steps[] =
	{0, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4};
void
Unit::CycleBlowBack(void)
{
	ticks_t	currTicks = ATicks();

	if(fActionDone)
		return;

	if(currTicks < tNextStateCheck)	// if not time for rethink yet
		return;

	if(!fInAction)
	{
		// start psycho mode
		fInAction = TRUE;
		MoveAnim(tileX, tileY, TILE_HORZ_CENTER, TILE_VERT_CENTER,
			move2steps[currMove], move2steps[currMove], move2ticks[currMove]);
		pWorld->LaunchSound(SND_GUST_O_WIND, tileX, tileY);
		return;
	}
	else
	{
		if(fInMotion)
		{
			// spin around, strike a pose only - dont animate
			currDir = (direction)((currDir + 1) % 8);
			RunAnimation(ST_ANIM_WALK, 0, TRUE, currDir);
			tNextStateCheck = currTicks + 1;
			return;
		}
		else
		{
			fActionDone = TRUE;	// end of spin
			SetPrimaryAction(MOVE_TO, tileX, tileY);
			if(type == FURY)
			{
				SetActionAvailable(ATTACK, TRUE);
				fAutoControl = FALSE;
				if(fCursorTracking)
					pWorld->DrawStatusBox();
			}
			return;
		}
	}
}

void
Unit::CycleBerserk(void)
{
	ticks_t	currTicks = ATicks();

	if(fActionDone)
		return;

	if(fInAction && currState == ST_BERSERKING)
	{
		// jump around like an idiot several times

		if(!(currTicks % (TICKS_PER_SEC * 10 / 4) / 10))	// 4 hits per sec
		{
			// strike a pose only - dont animate
			currDir = (direction)((currDir + 1) % 8);
			RunAnimation(ST_ANIM_WALK, 0, TRUE, currDir);
		}
	}

	if(currTicks < tNextStateCheck)	// if not time for rethink yet
		return;

	if(!fInAction)
	{
		// start psycho mode
		fInAction = TRUE;
		currState = ST_BERSERKING;
//		TRACK_MEM("BerserkAni");	grip gAni = pWorld->vPort.NewAni(ANIM_BERSERK,
//			1, PRI_SKY, TRUE, FALSE, 4, tileX, tileY, FALSE);		// poof!
//		pWorld->vPort.CheckAni(gAni);
		tNextStateCheck = currTicks + TICKS_PER_SEC * 2;	// shake for 2 secs
		return;
	}
	else
	{
		fActionDone = TRUE;	// end of nervous breakdown - now go kill someone
		tBerserkTimer = currTicks + TICKS_PER_SEC * 2;			// 1hp damage per sec
	}
}

void
Unit::SmartCycle(bool fEssentialOnly)
{
	ticks_t currTicks = ATicks();

	#ifndef NDEBUG
	UnitDebug.Out("\nUs%dg%d%s):C(p%s@%d/%dc%s@%d/%dfAD%d)", serialNum,
		gSelf, unitName, pVerbNames[primaryAction], primaryX, primaryY,
		pVerbNames[currAction],	secondaryX, secondaryY, fActionDone? 1: 0);
	#endif

	if(fAutoControl)
	{
//		BamDebug.Out("Unit(%d) fAutoControl==TRUE\n", serialNum);

		// periodic damage from BERSERK
		if(primaryAction == BERSERK && currTicks >= tBerserkTimer)
		{
			tBerserkTimer = currTicks + TICKS_PER_SEC * 2;
			TakeAHit(NULL, NOUNIT, tileX, tileY, 1);
		}
	
		// if fear/lure/stoned active && time's up
		if(tFearLureStonedTimer && (currTicks >= tFearLureStonedTimer))
		{
			// end of fear/lure
			tFearLureStonedTimer = NULL;
			fAutoControl = FALSE;
			SetSide(player);	// in case unit was under auto-control by virtue of alignment
			SetPrimaryAction(MOVE_TO, tileX, tileY);
			if(fCursorTracking)
				pWorld->DrawStatusBox();
		}
	}

	#ifndef NDEBUG
	UnitDebug.Out("C2");
	#endif

	// if standing on enemy portal
	if(tEnemyPortalTimer)
	{
		if(currTicks >= tEnemyPortalTimer)
		{
			#ifndef NDEBUG
			UnitDebug.Out("\nUnit(%d) tEnemyPortalTimer==TRUE", serialNum);
			#endif
			TakeAHit(NULL, UNIT_TERRAIN, tileX, tileY, 1);
			tEnemyPortalTimer = currTicks + TICKS_PER_SEC;		// wait another sec
		}
	}	

	// if unit has an auto-action to process
	if(tAutoActionTimer && currTicks > tAutoActionTimer)
	{
		#ifndef NDEBUG
		UnitDebug.Out("\nUnit(%d) tAutoActionTimer ready", serialNum);
		#endif
		switch(type)
		{
			case TROLL:
				// auto heal
				if((currAction != DISBAND && currAction != DIE) && currHP < maxHP)
				{
					currHP++;
					if(fCursorTracking)
						pWorld->DrawStatusBox();
				}
				tAutoActionTimer = currTicks + TICKS_PER_SEC * 2;
				break;

			case FURY:
				// it aint easy being furious
				TakeAHit(NULL, NOUNIT, tileX, tileY, 1);
				tAutoActionTimer = currTicks + FURY_DMG_INTERVAL;
				break;
		}
	}

	#ifndef NDEBUG
	UnitDebug.Out("C3");
	#endif

	// if current action complete
	if(fActionDone && !fEssentialOnly)
	{
		// make next decision (sets new currAction)
		CheckPrimaryAction();
	}

	// allow no other activity while acolyte is de-obelisking (caused by EndGuardMode())
	if(currState == ST_OBELISKING && type == ACOLYTE)
	{
		if(fAniDone)
		{
			// if leaving guard mode
			if(!fAniFwd)
			{
				// transformation complete
				currState = ST_NULL;
				if(fCursorTracking)
					pWorld->DrawStatusBox();
			}
			// else entering guard mode - let CycleGuard() deal with it.
			// We only worry about it here, so that ALL other primary actions dont need code to
			// wait for de-obelisk process
		}
		else 
		{
			BAM_Ani::SmartCycle(FALSE);

			#ifndef NDEBUG
			UnitDebug.Out("EOC-OB",
				pVerbNames[primaryAction], pVerbNames[currAction]);
			#endif
			return;	// do nothing else during obelisk transformation
		}
	}
  
	#ifndef NDEBUG
	UnitDebug.Out("C4()p%sc%s", pVerbNames[primaryAction], pVerbNames[currAction]);
	#endif

	if(!fEssentialOnly)
	switch(currAction)
	{
		case NO_ACTION:
			fActionDone = TRUE;
			break;

		case BLOWN_BACK:
			CycleBlowBack();
			break;

//		case IMMITATE:
//			CycleImmitate();
//			break;

		case GET_STONED:
			CycleGetStoned();
			break;

//		case VENOM:
//			CycleVenom();
//			break;

		case DROP:
			CycleDropMode();
			break;

		case STEAL:
			CycleSteal();
			break;

		case TRANSFORM_TO_BLDG:
			if(currTerrain == FOUND_GENERIC)
				CycleTransformToBldgMode();
			else CycleTurret();
			break;

		case TRANSFORM:
			CycleTransform();
			break;

		case DISBAND:
			CycleDisbandMode();
			break;

		case DIE:
			CycleDeathMode();
			break;

		case GUARD:
//			BamDebug.Out("Unit(%d)@%d::Cycle3() - pri=%d, curr=%d\n", serialNum, (int)currTicks,
//				(int)primaryAction,	(int)currAction);

			CycleGuardMode();
			break;

		case MOVE_TO:
			CycleMoveTo();
			break;

		case USE_ITEM:
			CycleUseItem();
			break;

		case ATTACK:
			CycleAttack();
			break;

		case BERSERK:	// temporary insanity state - only lasts a sec
			CycleBerserk();
			break;

		case SKILL:
			switch(type)
			{
				case WYRM:
					CycleFlameAttack();
					break;

				case ENCHANTER:
					CycleEnchantment();
					break;

				case HARPY:
					CycleHarpyNet();
					break;

				case NYMPH:
					CycleLure();
					break;

				case WRAITH:
					CycleScare();
					break;

				case ACOLYTE:
					CycleSendMana();
					break;

				case GNOME:
					CycleHeal();
					break;

			 	case CLERIC:
				case PALADIN:
					CycleHeal();
					break;

				case BANSHEE:
					CycleWail();
					break;
			}
			break;

	}
	#ifndef NDEBUG
	UnitDebug.Out("C5", pVerbNames[primaryAction], pVerbNames[currAction]);
	#endif

	BAM_Ani::SmartCycle(FALSE);

	#ifndef NDEBUG
	UnitDebug.Out("done");
	#endif
}

void
Unit::CycleDeathMode(void)
{
	ticks_t	currTicks = ATicks();
	Unit		*pUnit;
	grip		gAni, gUnit;
	int		aniNum, loop1;
	List		*pList;

	if(currTicks < tNextStateCheck)
		return;

	if(!fInAction)						// if not dying yet
	{
		#ifndef NDEBUG
		BamDebug.Out("Us%d(%s):CycleDeathMode()\n", serialNum, unitName);
		#endif
		pWorld->CheckWinLose(tileX, tileY);

	 	if(movement == WALKER || movement == SWIMMER || type == NYMPH)
			priorityLevel = PRI_FLOOR;	// look like you're on the ground

		fInAction = TRUE;
	 	fInMotion = FALSE;
		fAutoControl = TRUE;

		tAutoActionTimer = 0;
		tFearLureStonedTimer = NULL;
		tBerserkTimer = NULL;
		tEnemyPortalTimer = NULL;


		// remove self from grip map to allow other units immediate entry into our tile
		if(type != ROC_EGG)
			if(pWorld->unitLib.GetUnitGrip(tileX, tileY) == gSelf)	// failsafe
				pWorld->unitLib.PlaceUnitGrip(NULL, tileX, tileY);

		if(fCursorTracking)
		{
			// clear status box of dead unit
			pWorld->vPort.MoveCursor(pWorld->vPort.CursorX, pWorld->vPort.CursorY);
		}

		// death message
		if(pWorld->playerSide == player)
		{
			if(type == ACOLYTE && currState == ST_OBELISK)
				loop1 = (int)OBELISK;
			else loop1 = (int)type;
			pWorld->Announce(tileX, tileY, SQUIB_WORLD_MSGS, SQUIB_WORLD_YOUR,
				SQUIB_UNIT_NAMES, loop1, SQUIB_WORLD_MSGS, SQUIB_WORLD_HASDIED);
		}
		else if(pWorld->unitLib.CheckEnemyFlag(pWorld->playerSide, player) &&
			player != SIDE3)
		{
			if(type == ACOLYTE && currState == ST_OBELISK)
				loop1 = (int)OBELISK;
			else loop1 = (int)type;
			pWorld->Announce(tileX, tileY, SQUIB_WORLD_MSGS, SQUIB_WORLD_ENEMY,
				SQUIB_UNIT_NAMES, loop1, SQUIB_WORLD_MSGS, SQUIB_WORLD_HASDIED);
		}
		else if(type != UNIT_WEIRD_WARD)
		{
			pWorld->Announce(tileX, tileY, SQUIB_UNIT_NAMES, (int)type, SQUIB_WORLD_MSGS, SQUIB_WORLD_HASDIED);
		}

		// done in a while() loop in case of backpacking
		while(item)
			DropItem();

		switch(type)
		{
			case NYMPH:
			case WRAITH:
				// release any victims
				for(loop1 = 0, pList = &pWorld->unitLib.lUnits[loop1]; loop1 < TOTAL_SIDES; loop1++, pList++)
				{
					gUnit = (grip)pList->FirstValue();
					while(gUnit)
					{
						pUnit = ADerefAs(Unit, gUnit);
						gUnit = (grip)pList->NextValue();
						if(pUnit->fAutoControl && (pUnit->primaryAction == FOLLOW || pUnit->primaryAction == AVOID) &&
							pUnit->primaryTargetSerial == serialNum)
						{
							// release unit
							pUnit->fAutoControl = FALSE;
							pUnit->tFearLureStonedTimer = NULL;
							pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
						}
					}
				}
				break;
		}

		// pick animation
		if(unitSpecialDeath == GORGON)	// will be UNIT_TERRAIN if over WATER or CHASM
		{
			// do nothing, visually
			Hide();
			scrim.resNum = NULL;
			fAniDone = TRUE;
			currState = ST_DYING;
			tNextStateCheck = currTicks + 1;
			return;
		}
		else if(unitSpecialDeath == UNIT_TERRAIN)
		{
			switch(currTerrain)
			{
				case WATER:
				case WHIRLPOOL:
				case CAULDRON:
					if(currTerrain == CAULDRON)
						aniNum = ANIM_CAULDRON_SPLASH;
					else aniNum = ANIM_BIG_SPLASH;
					TRACK_MEM("DrownAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, aniNum, 1,
						priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff);
					pWorld->vPort.CheckAni(gAni);

					Hide();
					scrim.resNum = NULL;
					fAniDone = TRUE;
					currState = ST_DYING;
					tNextStateCheck = currTicks;
					return;

				case CHASM:
				case PRISON_PIT:
					if(currAnimState != ST_ANIM_STONED && type != UNIT_WEIRD_WARD
						&& type != ACOLYTE)
					{
						RunAnimation(ST_ANIM_ATTACK, 1, TRUE, DOWN, TRUE, TRUE);
//						RunAnimation(ST_ANIM_ATTACK, 0, FALSE, DOWN, TRUE, TRUE);
//						SetCel(2);
					}
					MoveAnim(tileX, tileY, tileXOff, TILE_VERT_BOTTOM - 2, 0, 1, 2);
					tNextStateCheck = currTicks + 1;
					currState = ST_DYING;
					return;
			}
		}

		RunAnimation(ST_ANIM_DIE, 3);
		tNextStateCheck = currTicks + TICKS_PER_SEC / 2;
		currState = ST_DYING;
		return;
	}
	
	if(currState == ST_DYING)
	{
		if(unitSpecialDeath == UNIT_TERRAIN && (currTerrain == CHASM || currTerrain == PRISON_PIT))
		{
			if(scrim.scale > 64)
			{
				SetScale(scrim.scale - 16);
				tNextStateCheck = currTicks + 1;
				return;
			}
			else
			{
				currState = ST_DEAD;
				Hide();
				scrim.resNum = 0;
			}
		}
		else if(fAniDone)
		{
			currState = ST_DEAD;
			tNextStateCheck = currTicks + TICKS_PER_SEC * 2;

			// hold on last cel of death, or clear immediately?
			if(movement != WALKER)
			{
				Hide();
				scrim.resNum = 0;
			}
			else switch(type)
			{
				case ROC_EGG:
					// cheat death - become a ROC instead
					SetType(ROC);
					primaryAction = NO_ACTION;
					currAction = NO_ACTION;
					SetPrimaryAction(GUARD);
					return;

				case PALADIN:
				case JUGGERNAUT:
				case ENCHANTER:
				case GUARDIAN:
					Hide();
					scrim.resNum = 0;
					break;
	 		}
			return;
		}
	}

	if(currState == ST_DEAD)
	{
		// if we were riding on another unit, tell them they're free
//		if(mountSerial)
//		{
//			pMount = (Unit *)(pWorld->ValidateTarget(mountSerial));
//			if(pMount)
//				pMount->SetRider(NULL);
//			mountSerial = NULL;
//		}
		fDeleteMe = TRUE;

		if(pWorld->mapResNum % 9310 < 10 && currTerrain == CHASM) 
		{
			BAM_Ani	*pAni;
			TRACK_MEM("FlameSpout");	pAni = pWorld->vPort.NewTerrainUnit(ACTIVE_FLAME_SPOUT,
				tileX, tileY);
			if(pAni)
			{
				pAni->SetTilePosOffset(tileXOff, tileYOff);
				pWorld->vPort.RepositionAni(pAni);
			}
			pWorld->LaunchSound(SND_FLAME_SPURT, tileX, tileY);
		}
	}
}

void
Unit::CycleSendMana(void)
{
	ticks_t	currTicks = ATicks();
	grip		gAni;
	BAM_Ani	*pAni;

	if(currTicks < tNextStateCheck)
		return;

	if(fActionDone)
		return;

	if(!fInAction)
	{
		fInAction = TRUE;
		if(!currMana || (currState != ST_OBELISK && currState != ST_OBELISKING))
		{
			fActionDone = TRUE;
			return;
		}

		if(fCursorTracking && pWorld->statBoxCurrMode == TERRAIN)
			pWorld->SetVerbButtons(pWorld->vPort.targType, pWorld->vPort.targSerial);

		TRACK_MEM("ManaAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_MANA_TRANSFER, 1, PRI_WALKER + 1, TRUE,
			FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff);
		pAni = ADerefAs(BAM_Ani, gAni);
		pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[player]);
		pWorld->vPort.CheckAni(gAni);
		pWorld->LaunchSound(SND_MAGICAL_ENERGY, tileX, tileY, FALSE, 25);

		pWorld->AdjustManaLevel(player, AMin(currMana, MANA_ACOLYTE_MAX / 2));
		if(pBam->playerTypes[player] == PLAYER_COMPUTER && pWorld->ai.CheckSpellIQ())
		{
			// cheat
			pWorld->AdjustManaLevel(player, 3);
		}
		currMana = 0;

		// update TRANSFER button if necessary
		if(fCursorTracking && player == pWorld->playerSide)
			pWorld->SetVerbButtons(pWorld->vPort.targType, pWorld->vPort.targSerial);

		tNextStateCheck = currTicks + TICKS_PER_SEC;

		return;
	}

	TRACK_MEM("ManaAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_MANA_RECEIVE, 1, PRI_SWIMMER, TRUE,
		FALSE, 3, pWorld->map.portalCoords[player][0], pWorld->map.portalCoords[player][1], FALSE);
	pAni = ADerefAs(BAM_Ani, gAni);
	pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[player]);
	pWorld->vPort.CheckAni(gAni);
	pWorld->LaunchSound(SND_MAGICAL_ENERGY, pAni->tileX, pAni->tileY, FALSE, 25);

	tNextStateCheck = currTicks + TICKS_PER_SEC;
	fActionDone = TRUE;
}

// ACOLYTE changes into TURRET (wall)
void
Unit::CycleTurret(void)
{
	ticks_t	currTicks = ATicks();
	BAM_Ani	*pAni;
	int		loop1, loopX, loopY;
	tile		*pTile;
	terrainType	terrain1;
	TerrainUnit	*pTUnit;
  	MapSpace	*pSpace;
	itemType	item1;

	if(currTicks < tNextStateCheck)
		return;

	if(!fInAction)
	{
		fInAction = TRUE;

		// can we create a turret here?
		terrain1 = (terrainType)pWorld->map.GetTerrainNum(tileX, tileY);
		switch(terrain1)
		{
			case PORTAL:
			case MINE:
			case CAMP:
			case CAULDRON:
			case PRESSURE_PLATE:
				fActionDone = TRUE;
				return;

			case BRIDGE:
				// if puzzle palace
				if(pWorld->mapResNum / 9310 < 10)
				{
					fActionDone = TRUE;
					return;
				}
				break;
		}
		if(pWorld->pTileLib->terrains[terrain1].moveRate > 1)
		{
			// denied!
			fActionDone = TRUE;
			return;
		}

		if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		{
			// denied!
			fActionDone = TRUE;
			return;
		}

		if(item)
		{
			// drop the item somewhere, but not under us
			#ifndef NDEBUG
			BamDebug.Out("\nPlaceItem %s %d %s", unitName, __LINE__, __FILE__);
			#endif
			pWorld->itemMgr.PlaceItem(tileX, tileY, item, 2, TRUE);
			LoseItem();
		}

	 	fAutoControl = TRUE;	// eliminate player control
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.DeleteAni(gAuxAni);
		gAuxAni = NULL;
		// poof!
		TRACK_MEM("TransformAni");	gAuxAni = pWorld->vPort.NewAni(RES_ANIM,
			ANIM_TRANSFORM, 1, priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY,
			FALSE, tileXOff, tileYOff + 3);
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.CheckAni(gAuxAni);
		else gAuxAni = NULL;
		pWorld->LaunchSound(SND_TRANSFORM, tileX, tileY);
		return;
	}

	if(fActionDone)
	{
		// we're already finished - just havent changed state yet, I guess.
		return;
	}

	bool fMakeTurret = FALSE;

	if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
	{
		pAni = ADerefAs(BAM_Ani, gAuxAni);
		if(pAni->scrim.cel == 3 || pAni->fAniDone)
			fMakeTurret = TRUE;
	}
	else fMakeTurret = TRUE;

	if(fMakeTurret)
	{
		// if existing terrain unit (fire, flame spout, etc), kill it
		pTUnit = pWorld->vPort.GetTerrainUnit(tileX, tileY);
		if(pTUnit)
		{
			pTUnit->fDeleteMe = TRUE;
			pTUnit->Hide();
		}

		// remove unit
		fDeleteMe = TRUE;
		Hide();				// force image removal this cycle, instead of waiting
		gAuxAni = NULL;	// allow poof ani to finish cycling after we're gone

		List	lpTurrets(4, TRUE);
		MapSpace	*pWall, *pOldWall;
		int	x1, x2, y1, y2, structNum;

		lpTurrets.fDuplicates = FALSE;

		// build list of nearby existing TURRET structure(s) for possible link up.
		// If more than one is found, and they are different structures (MapSpace *), they will
		// be merged
		
		x1 = AMax(0, tileX - 1);	x2 = AMin(WORLD_WIDTH - 1, tileX + 1);
		y1 = AMax(0, tileY - 1);	y2 = AMin(WORLD_HEIGHT - 1, tileY + 1);
		for(loopY = y1; loopY <= y2; loopY++)
			for(loopX = x1; loopX <= x2; loopX++)
			{
				if(loopX != tileX && loopY != tileY)	// avoid diagonals
					continue;

				pTile = pWorld->map.GetTile(loopX, loopY);
				if(ABetween(pTile->terrain, TURRET, TURRET15))
				{
					pOldWall = pWorld->map.GetStructure(loopX, loopY);
					if(pOldWall->h.owner == player && pOldWall->size <
					 (MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE))
						lpTurrets.Add((void *)pOldWall);
				}
			}

		if(lpTurrets.count > 0)
		{
			// add ourselves to 1st structure detected
			pWall = (MapSpace *)lpTurrets.FirstValue();
			structNum = pWorld->map.MapSpaces[pWall->h.yPos * WORLD_WIDTH + pWall->h.xPos];

		}
		else
		{
			//	create new structure
			TRACK_MEM("NewTurretSpace");	pWall = new MapSpace(FUNC_MISC);
 			pWall->serialNum = pWorld->GetNextSerialNum(BUILDING, pWall->gSelf);
			structNum = pWorld->map.h.totalSpaces++;
			pWorld->map.gSpaces[structNum] = pWall->gSelf;
			pWall->h.xPos = (uint32)tileX;
			pWall->h.yPos = (uint32)tileY;
			pWall->h.owner = player;
			pWorld->map.lBuildings[player].Add((void *)pWall->gSelf);
		}

		// if we're standing on an hot spot, copy that info into the new wall structure
		pSpace = pWorld->map.GetStructure(tileX, tileY);
		if(pSpace->h.hotSpot)
			pWall->h.hotSpot = TRUE;
	
		// add our X,Y to this wall struct, and place its # in the struct map
		pWall->tiles[pWall->size][0] = (int8)tileX;
		pWall->tiles[pWall->size][1] = (int8)tileY;
		pWall->size++;
		pWorld->map.MapSpaces[tileY * WORLD_WIDTH + tileX] = (int16)structNum;
		
		// integrate any other adjoining wall structs with this one
		pOldWall = (MapSpace *)lpTurrets.NextValue();
		while(pOldWall)
		{
			if(pWall->size + pOldWall->size > (MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE))
			{
				// too big!  skip it
				pOldWall = (MapSpace *)lpTurrets.NextValue();
				continue;
			}

			for(loop1 = 0; loop1 < pOldWall->size; loop1++)
			{
				pWall->tiles[pWall->size][0] = (int8)pOldWall->tiles[loop1][0];
				pWall->tiles[pWall->size][1] = (int8)pOldWall->tiles[loop1][1];
				pWall->oldTileNums[pWall->size] = pOldWall->oldTileNums[loop1];
				pWall->size++;
				pWorld->map.MapSpaces[pOldWall->tiles[loop1][1] * WORLD_WIDTH +
					pOldWall->tiles[loop1][0]] = (int16)structNum;
			}
			pWorld->map.lBuildings[pOldWall->h.owner].Delete((void *)pOldWall->gSelf);
			// leave grip in gSpaces[] list, for de-allocation later

			// preserve any hotspots we might be assimilating (to serve the collective)
			if(pOldWall->h.hotSpot)
				pWall->h.hotSpot = TRUE;

			pOldWall = (MapSpace *)lpTurrets.NextValue();
		}
		lpTurrets.Release();

		pWall->h.maxHP = pWall->size * 35 + (2 * pWall->size * pWall->size);
		pWall->h.maxHP = ((pWall->h.maxHP + 4) / 5) * 5;		// round up to nearest 5
		pWall->h.currHP = pWall->h.maxHP;

		// change actual tile to TURRET now
		pWorld->map.UpdateAllTurretTiles(pWall);

		
		// preserve any item we're standing on
		item1 = pWorld->itemMgr.RemoveItem(tileX, tileY);
		if(item1)
		{
			if(!pWorld->itemMgr.PlaceItem(tileX, tileY, item1, 10, TRUE))
			{
				pWorld->Announce(tileX, tileY, SQUIB_ITEM_NAMES, (int)item1, SQUIB_WORLD_MSGS,
					SQUIB_WORLD_HASBEENLOST);
			}
		}
	}
}

void
Unit::CycleTransformToBldgMode(void)
{
	ticks_t	currTicks = ATicks();
	BAM_Ani	*pAni = NULL;
	itemType item1;

	if(!fInAction)		// if not transforming yet
	{
		#ifndef NDEBUG
		BamDebug.Out("\nCycleTransformToBldg(%s)", unitName);
		#endif
		fInAction = TRUE;
		currState = ST_BUILDING;
		fAniDone = TRUE;
		tNextStateCheck = currTicks + TICKS_PER_SEC * 0;

		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.DeleteAni(gAuxAni);
		gAuxAni = NULL;
		TRACK_MEM("TransformAni");	gAuxAni = pWorld->vPort.NewAni(RES_ANIM,
			ANIM_TRANSFORM, 1, priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY,
			TRUE);				// poof!
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.CheckAni(gAuxAni);
		else gAuxAni = NULL;
		pWorld->LaunchSound(SND_TRANSFORM, tileX, tileY);

		// certain items must be dropped if you're gonna transform
		if(item == HARPY_NET)
		{
			// gotta let him go
			LoseItem();
		}
		return;
	}

	bool	fMakeBldg = FALSE;

	if(currState == ST_BUILDING)
	{
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
		{
			pAni = ADerefAs(BAM_Ani, gAuxAni);
			if(pAni->scrim.cel == 3 || pAni->fAniDone)	// frame to remove unit on
				fMakeBldg = TRUE;
		}
		else fMakeBldg = TRUE;

		if(fMakeBldg)
		{
			MapSpace	*pSpace;
			pSpace = pWorld->map.GetStructure(tileX, tileY);
			if(pSpace && pSpace->IsFoundation())	// if transformation hasnt occured yet
			{
				// then we get to trigger it (we came up first)
				pSpace->FoundationToBuilding(player);
			}

	 		if(item)
			{
				DropItem(TRUE);
			}

			item1 = pWorld->itemMgr.RemoveItem(tileX, tileY);
			if(item1)
			{
				if(!pWorld->itemMgr.PlaceItem(tileX, tileY, item1, 10, TRUE))
				{
					pWorld->Announce(tileX, tileY, SQUIB_ITEM_NAMES, (int)item1, SQUIB_WORLD_MSGS,
						SQUIB_WORLD_HASBEENLOST);
				}
			}

			if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			{
				if(pAni->fAniDone)	// safety net - shouldnt happen
					pWorld->vPort.DeleteAni(gAuxAni);
				else
					pAni->fAniPermanent = FALSE;	// cause to self-delete
			}
			gAuxAni = NULL;

			currState = ST_NULL;
			pWorld->unitLib.PlaceUnitGrip(NULL, tileX, tileY);
			ADerefAs(WorldMap, pWorld->gWorldMap)->Draw(tileX, tileY);
			fDeleteMe = TRUE;
			Hide();		// force image removal this cycle, instead of waiting
		}
	}

	if(currTicks < tNextStateCheck)
		return;
}

void
Unit::CycleDropMode(void)
{
	ticks_t	currTicks = ATicks();

	if(!fInAction)		// if not disbanding yet
	{
		#ifndef NDEBUG
		BamDebug.Out("\nCycleDropMode(%s)", unitName);
		#endif
		fInAction = TRUE;
		fActionDone = TRUE;
		tNextStateCheck = currTicks;

		DropItem();
	}
}

void
Unit::CycleDisbandMode(void)
{
	ticks_t	currTicks = ATicks();
	BAM_Ani	*pAni = NULL;
	Unit		*pUnit;
	int		loop1;
	List		*pList;
	grip		gUnit;

	if(!fInAction)		// if not disbanding yet
	{
		#ifndef NDEBUG
		BamDebug.Out("CycleDisbandMode(%s) start..", unitName);
		#endif
		pWorld->CheckWinLose(tileX, tileY);

		fInAction = TRUE;
		currState = ST_DISBANDING;
		fAniDone = TRUE;
		fInMotion = FALSE;
		tNextStateCheck = currTicks + TICKS_PER_SEC * 0;

		switch(type)
		{
			case NYMPH:
			case WRAITH:
				// release any victims
				for(loop1 = 0, pList = &pWorld->unitLib.lUnits[loop1]; loop1 < TOTAL_SIDES; loop1++, pList++)
				{
					gUnit = (grip)pList->FirstValue();
					while(gUnit)
					{
						pUnit = ADerefAs(Unit, gUnit);
						gUnit = (grip)pList->NextValue();
						if(pUnit->fAutoControl && (pUnit->primaryAction == FOLLOW || pUnit->primaryAction == AVOID) &&
							pUnit->primaryTargetSerial == serialNum)
						{
							// release unit
							pUnit->fAutoControl = FALSE;
							pUnit->tFearLureStonedTimer = NULL;
							pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
						}
					}
				}
				break;
		}

		// poof!
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.DeleteAni(gAuxAni);
		gAuxAni = NULL;

		// done in a while() loop in case of backpacking
		while(item)
			DropItem();

		TRACK_MEM("DisbandAni");	gAuxAni = pWorld->vPort.NewAni(RES_ANIM,
			ANIM_DISBAND, 1, priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY,
			TRUE, tileXOff, tileYOff);
		pAni = ADerefAs(BAM_Ani, gAuxAni);
		pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[player]);
		pWorld->vPort.CheckAni(gAuxAni);
		pWorld->LaunchSound(SND_DISBAND, tileX, tileY);

		pWorld->unitsLost[player]++;
		#ifndef NDEBUG
		BamDebug.Out("done\n");
		#endif
		return;
	}

	bool	fDisband = FALSE;

	if(currState == ST_DISBANDING)
	{
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
		{
			pAni = ADerefAs(BAM_Ani, gAuxAni);
			if(pAni->scrim.cel == 6 || pAni->fAniDone)	// frame to remove unit on
				fDisband = TRUE;
		}
		else fDisband = TRUE;

		if(fDisband)
		{
			if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			{
				if(pAni->fAniDone)	// shouldnt happen)
					pWorld->vPort.DeleteAni(pAni->gSelf);
				else pAni->fAniPermanent = FALSE;
			}
			gAuxAni = NULL;

			currState = ST_DISBANDED;

			// if we were riding on another unit, tell them they're free
//			if(mountSerial)
//			{
//				pMount = (Unit *)(pWorld->ValidateTarget(mountSerial));
//				if(pMount)
//					pMount->SetRider(NULL);
//				mountSerial = NULL;
//			}
			fDeleteMe = TRUE;
			Hide();		// force image removal this cycle, instead of waiting
			pWorld->AdjustManaLevel(player, MANA_DISBAND);
		}
	}

//	if(currTicks < tNextStateCheck)
//		return;
}

// DOPPLEGANGER skill
//void
//Unit::CycleImmitate(void)
//{
//	Unit	*pUnit;
//
//	if(fActionDone)
//		return;
//
//	fInAction = TRUE;
//	fActionDone = TRUE;
//
//	if(secondaryTargetType != UNIT)
//		return;
//
//	pUnit = (Unit *)ValidateSecondaryTarget();
//	if(!pUnit)
//		return;
//
//	// if not close enough
//	if(GetDistanceTo(pUnit->tileX, pUnit->tileY) > skillRange)
//		return;
//
//	// if not on side1 or side2
//	if(!(pUnit->player & (SIDE1 | SIDE2)))
//		return;
//
//	// if immune to immitation
//	if(pUnit->ImmuneToSkillFrom(this))
//		return;
//
//	if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
//		return;
//
//	SetType(pUnit->type);
//	TRACK_MEM("ImmitateAni");	grip gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1,
//		priorityLevel + 2, TRUE, FALSE, 4, tileX, tileY, FALSE);
//	pWorld->vPort.CheckAni(gAni);
//	pWorld->LaunchSound(SND_TRANSFORM, tileX, tileY);
//}

void
Unit::CycleEnchantment(void)
{
	Unit		*pUnit, *pUnit2;
	ticks_t	currTicks;
//	BAM_Ani	*pAni;
	grip		gUnit, gAni;

	if(fActionDone)
		return;

	currTicks = ATicks();
	if(currTicks < tNextStateCheck)
		return;

	if(!fInAction)
	{
		fInAction = TRUE;
		if(secondaryTargetType != UNIT)
		{
			fActionDone = TRUE;
			return;
		}

		pUnit = (Unit *)ValidateSecondaryTarget();
		if(!pUnit)
		{
			fActionDone = TRUE;
			return;
		}
		if(pUnit->ImmuneToSkillFrom(this))
		{
			fActionDone = TRUE;
			return;
		}

		if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		{
			fActionDone = TRUE;
			return;
		}

		RunAnimation(ST_ANIM_ATTACK, 0, FALSE, GetDir(secondaryX - tileX, secondaryY - tileY), TRUE, FALSE);

//		if(gAuxAni)
//			pWorld->vPort.DeleteAni(gAuxAni);
		TRACK_MEM("EnchantAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_ENCHANTMENT, 1,
			pUnit->priorityLevel + 2, TRUE, FALSE, 3, pUnit->tileX, pUnit->tileY, FALSE,
			pUnit->tileXOff, pUnit->tileYOff);	// poof!
		pWorld->vPort.CheckAni(gAni);
		pUnit->tNextStateCheck = currTicks + TICKS_PER_SEC;	// suspended animation
		if(!pUnit->fAniDone)
			pUnit->cycleNext = currTicks + TICKS_PER_SEC;
		if(pUnit->fInMotion)
			pUnit->tMotionNextStep = currTicks + TICKS_PER_SEC;
		tNextStateCheck = currTicks + TICKS_PER_SEC / 2;
		return;
	}

	pWorld->AdjustXP(player, XP_USE_SKILL);
	fActionDone = TRUE;
	pUnit = (Unit *)ValidateSecondaryTarget();
	if(pUnit && pUnit->player != player)
	{
		pWorld->LaunchSound(SND_GIGGLE, tileX, tileY);
		pWorld->unitsLost[pUnit->player]++;
		pUnit->SetSide(player);

		// cause unit's current terrain to re-register, in case of acolytes on portals or other weirdness
		pUnit->currTerrain = UNDEFINED;
		pUnit->SetTilePos(pUnit->tileX, pUnit->tileY, pUnit->tileXOff, pUnit->tileYOff);

		ADerefAs(WorldMap, pWorld->gWorldMap)->Draw(pUnit->tileX, pUnit->tileY);
		pUnit->currHP /= 2;
		if(!pUnit->currHP)
			pUnit->currHP = 1;
		pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);

		if(pUnit->fCursorTracking || pWorld->lastUnitSerialDrawn == serialNum)
		{
			pWorld->DrawStatusBox();
//			pWorld->vPort.SetCursorTarget(UNIT, pUnit->gSelf);	- why was this being done? It causes trouble.
  		}

		// notify any friendlies attacking him, to stop
		gUnit = (grip)pWorld->unitLib.lUnits[player].FirstValue();
		while(gUnit)
		{
			pUnit2 = ADerefAs(Unit, gUnit);
			if(pUnit2->primaryTargetSerial == secondaryTargetSerial &&
				(pUnit2->primaryAction == ATTACK || (pUnit2->primaryAction == SKILL
				&& pUnit2->skillType == SKILL_AGGRESSIVE)))
			{
				pUnit2->SetPrimaryAction(GUARD);
			}
			else if(pUnit2->secondaryTargetSerial == secondaryTargetSerial &&
				pUnit2->currAction == ATTACK)
			{
				// leave him alone!
				pUnit2->SetAction(GUARD);
			}

			if(pUnit2->searchForEnemiesPresetSerial == secondaryTargetSerial)
				pUnit2->searchForEnemiesPresetSerial = NULL;
			gUnit = (grip)pWorld->unitLib.lUnits[player].NextValue();
		}
	}
}

void
Unit::CycleHarpyNet(void)
{
	Unit		*pUnit;
	ticks_t	currTicks;

	if(fActionDone)
		return;

	currTicks = ATicks();
	if(currTicks < tNextStateCheck)
		return;

	if(!fInAction)
	{
		fInAction = TRUE;
		if(secondaryTargetType != UNIT)
		{
			fActionDone = TRUE;
			return;
		}

		pUnit = (Unit *)ValidateSecondaryTarget();
		if(!pUnit)
		{
			fActionDone = TRUE;
			return;
		}
	 	if(pUnit->ImmuneToSkillFrom(this))
		{
			fActionDone = TRUE;
			return;
		}
		if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		{
			fActionDone = TRUE;
			return;
		}
		
		RunAnimation(ST_ANIM_ATTACK, 3, FALSE, GetDir(secondaryX - tileX, secondaryY - tileY), TRUE, TRUE);
		return;
	}

	if(scrim.cel == 3 && !fActionEnding)
	{
		pWorld->AdjustXP(player, XP_USE_SKILL);

		// capture unit
		pUnit = (Unit *)ValidateSecondaryTarget();
		if(pUnit && !pUnit->ImmuneToSkillFrom(this))
		{
//			if(pUnit->mountSerial)
//			{
//				Unit	*pMount = (Unit *)(pWorld->ValidateTarget(pUnit->mountSerial));
//				if(pMount)
//					pMount->SetRider(NULL);
//				pUnit->mountSerial = NULL;
//			}
//			else

			// make our victim disappear
			pWorld->unitLib.PlaceUnitGrip(NULL, pUnit->tileX, pUnit->tileY);

	 		if(pUnit->gGroupCursor)
			{
				pUnit->GroupSelection(FALSE);
			}

			pUnit->Hide();
			pUnit->fAutoControl = TRUE;
			pUnit->fPrimarySatisfied = FALSE;
	 		if(pUnit->fTerrainAni)
				pUnit->StopTerrainAni();
			if(pUnit->gAuxAni && pMemMgr->CheckGrip(pUnit->gAuxAni))
			{
				pWorld->vPort.DeleteAni(pUnit->gAuxAni);
				pUnit->gAuxAni = NULL;
			}
			pWorld->vPort.CutAni(pUnit->gSelf);
	 		pWorld->unitLib.CutUnit(pUnit->gSelf, TRUE);

			prisonerSerial = pUnit->serialNum;
			AcquireItem(HARPY_NET);
			SetActionAvailable(SKILL, FALSE);
			SetActionAvailable(ATTACK, FALSE);

			ADerefAs(WorldMap, pWorld->gWorldMap)->Draw(pUnit->tileX, pUnit->tileY);
			if(pUnit->fCursorTracking || pWorld->lastUnitSerialDrawn == serialNum)
			{
				pWorld->SetStatusBoxMode(TERRAIN);
				pWorld->DrawStatusBox();
			}
			fActionEnding = TRUE;

			if(pBam->playerTypes[pUnit->player] == PLAYER_COMPUTER)
			{
				// give the AI a nudge for faster response
				pWorld->ai.nextAITimer[pUnit->player] = currTicks;
			}
		}
	}
	if(currAnimState != ST_ANIM_ATTACK || fAniDone)
		fActionDone = TRUE;
}

void
Unit::CycleHeal(void)		// also handles REPAIR
{
	Unit		*pUnit;
	MapSpace	*pBldg;

	if(fActionDone)
		return;		// safety net

	fInAction = TRUE;
	fActionDone = TRUE;

	switch(secondaryTargetType)
	{
		case BUILDING:
			pBldg = (MapSpace*) ValidateSecondaryTarget();

			// if not next to target
			if(!pBldg || GetDistanceTo(pBldg, BUILDING) > skillRange)
				return;

			switch(type)		// what are we?
			{
				case GNOME:
					if(pBldg->h.maxHP > 0)	// is it a structure at all?
					{
						if(pWorld->SpendMana(player, skillCost, pBldg->h.xPos, pBldg->h.yPos))
							pBldg->HealThyself(gSelf, type, 5);
						break;
					}
					return;
			}
			break;

		case UNIT:
			pUnit = (Unit *)ValidateSecondaryTarget();

			// if not next to target
			if(!pUnit || GetDistanceTo(pUnit, UNIT) > skillRange)
				return;

			switch(type)		// what are we?
			{
				case GNOME:
					if(!pUnit->ImmuneToSkillFrom(this))
					{
						if(pWorld->SpendMana(player, skillCost, pUnit->tileX, pUnit->tileY))
							pUnit->HealThyself(gSelf, type, 5);
					}
					break;

				case PALADIN:
				case CLERIC:
					if(pUnit->currHP == pUnit->maxHP)
					{
						// unit does not need healing.  Are we AI-driven?
						if(pBam->playerTypes[player] == PLAYER_COMPUTER)
						{
							// skip it, save mana
							break;
						}
					}

					if(!pWorld->SpendMana(player, skillCost, pUnit->tileX, pUnit->tileY))
						break;
					pUnit->HealThyself(gSelf, type, 5);

					if(bGlobal.storyLine == TUTORIAL && player == pWorld->playerSide &&
						type == CLERIC && pUnit->type == PEASANT && pUnit->currHP >= pUnit->maxHP)
							pWorld->SetTutorialGoal(TUT_UNIT_HEALED, TRUE);
					break;
			}
	}
	pWorld->AdjustXP(player, XP_USE_SKILL);
}

void
Unit::CycleUseItem(void)
{
	Unit		*pUnit;
	MapSpace	*pBldg;
	int		distance;
	grip		gUnit;
	ticks_t	currTicks = ATicks();
	bool		fItemWorked = FALSE;

	if(fActionDone)
		return;

	if(!fInAction)
	{
		#ifndef NDEBUG
		BamDebug.Out("Us%d(%s):CycleUseItem() %s\n", serialNum, unitName,
			pWorld->itemMgr.itemNames[item]);
		#endif
		fInAction = TRUE;

		// check if target out of range
		distance	= AMax(abs(secondaryX - tileX), abs(secondaryY - tileY));
		if(distance > pWorld->itemMgr.GetItemRange(item))
		{
			fActionDone = TRUE;
			return;
		}

		switch(item)
		{
			// some items are used on the user instead of the target
			case TELEPORT_TOME:
				if(pWorld->GetManaLevel(player) < MANA_TELEPORT)
				{
					fActionDone = TRUE;
					break;
				}

//				if(gAuxAni)
//					pWorld->vPort.DeleteAni(gAuxAni);
				// poof!
				TRACK_MEM("TeleportAni");	gUnit = pWorld->vPort.NewAni(RES_ANIM, ANIM_TELEPORT, 1,
					priorityLevel + 1000, TRUE, FALSE, 3, secondaryX, secondaryY, FALSE, tileXOff, tileYOff - 12);
				pWorld->vPort.CheckAni(gUnit);
				pWorld->LaunchSound(SND_MAGICAL_ENERGY, secondaryX, secondaryY);

				TRACK_MEM("TeleportAni");	gUnit = pWorld->vPort.NewAni(RES_ANIM, ANIM_TELEPORT, 1,
					priorityLevel + 1000, TRUE, FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff - 12);	// poof!
				pWorld->vPort.CheckAni(gUnit);
				pWorld->LaunchSound(SND_MAGICAL_ENERGY, tileX, tileY);
				tNextStateCheck = currTicks + TICKS_PER_SEC;
				break;
	
			case WEIRD_WAND:
				if(pWorld->GetManaLevel(player) < MANA_WEIRD_WARD)
				{
					fActionDone = TRUE;
					break;
				}
				break;

			default:
				fItemWorked = FALSE;

				switch(secondaryTargetType)
				{
					case UNIT:
						pUnit = (Unit *)ValidateSecondaryTarget();
						if(pUnit)
						{
							fItemWorked = pUnit->ApplyItemEffects(item);
						}
						break;

					case BUILDING:
						pBldg = (MapSpace *)ValidateSecondaryTarget();
						if(pBldg)
							switch(item)
							{
								case MASON_MIX:
									pBldg->HealThyself(gSelf, type, MASON_MIX_HP);
									pWorld->AdjustXP(player, XP_USE_ITEM);
									fItemWorked = TRUE;
									break;
							}
						break;
				}

				if(fItemWorked && !pWorld->itemMgr.IsItemReusable(item))
				{
					LoseItem();
					if(type == BRIGAND)
					{
						List	*plPack = ADerefAs(List, glBackpack);
						if(plPack->count > 0)		// look inside our backpack
						{
							AcquireItem((itemType)plPack->LastValue());
							plPack->Delete((void *)item);
						}
					}
				}
				if(fInAction == TRUE)	// if item usage didnt cause a primaryAction change
					fActionDone = TRUE;
				// else - new primary set - we wont be in USE_ITEM mode anymore anyway,
				// so dont worry about backing out of it normally with fActionDone
				break;
		}
		return;
	}

	if(currTicks < tNextStateCheck)
		return;

	switch(item)
	{
		case TELEPORT_TOME:
//			pAni = ADerefAs(BAM_Ani, gAuxAni);
//			if(pAni->scrim.cel == 6)
//			{
				UseItem();
				fActionDone = TRUE;
//				gAuxAni = NULL;
//			}
			break;

		case WEIRD_WAND:
			UseItem();
			fActionDone = TRUE;
			break;
	}
}

// BANSHEE wail skill
void
Unit::CycleWail(void)
{
	Unit		*pUnit;
	int32		startX, startY, endX, endY;
	uint32	loopX, loopY;
	grip		gAni;

	if(fActionDone)
		return;

	if(!fInAction)
	{
		fInAction = TRUE;

		if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		{
			fActionDone = TRUE;
			return;
		}

		pWorld->AdjustXP(player, XP_USE_SKILL);

		startX = tileX - skillRange;	startX = AMax(0, startX);
		startY = tileY - skillRange;	startY = AMax(0, startY);
		endX = tileX + skillRange; endX = AMin(WORLD_WIDTH - 1, endX);
		endY = tileY + skillRange; endY = AMin(WORLD_HEIGHT - 1, endY);

		for(loopY = startY; loopY <= endY; loopY++)
			for(loopX = startX; loopX <= endX; loopX++)
			{
				pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
				if(pUnit)
				{
					if(pUnit != this && !pUnit->ImmuneToSkillFrom(this))
					{
						pUnit->TakeAHit(this, type, tileX, tileY, DMG_BANSHEE_WAIL, TRUE);
						TRACK_MEM("WailAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_HEAL_REPAIR, 1,
							pUnit->priorityLevel + 2, TRUE, FALSE, 4, pUnit->tileX, pUnit->tileY, FALSE);
						pWorld->vPort.CheckAni(gAni);
					}

					if(pUnit->mountSerial)
					{
						pUnit = (Unit *)(pWorld->ValidateTarget(pUnit->mountSerial));
						if(pUnit && pUnit != this && !pUnit->ImmuneToSkillFrom(this))
							pUnit->TakeAHit(this, type, tileX, tileY, DMG_BANSHEE_WAIL, TRUE);
						TRACK_MEM("WailAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_HEAL_REPAIR, 1,
							pUnit->priorityLevel + 2, TRUE, FALSE, 4, pUnit->tileX, pUnit->tileY, FALSE);
						pWorld->vPort.CheckAni(gAni);
					}
				}
			}

		RunAnimation(ST_ANIM_GUARD, 3, FALSE, currDir, TRUE, TRUE);
		tNextStateCheck = ATicks() + TICKS_PER_SEC;

		pWorld->LaunchSound(SND_FEMALE_WAIL, tileX, tileY);
	}
	else
	{
		if(fAniDone)
		{
			// WAIL completed - reset
			tAutoActionTimer = ATicks() + TICKS_PER_SEC * (ARandom(10) + 4);

			// if not attacked recently..
			if(lastAttackTicks > ATicks() - (TICKS_PER_SEC * 2))
				tAutoActionTimer += TICKS_PER_SEC * 5;
			fActionDone = TRUE;
		}
		else tNextStateCheck = ATicks() + TICKS_PER_SEC;
	}
}

// WRAITH fear skill
void
Unit::CycleScare(void)
{
	Unit		*pUnit;
	int32		startX, startY, endX, endY;
	uint32	loopX, loopY;

	if(fActionDone)
		return;

	fInAction = TRUE;
	fActionDone = TRUE;

	if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		return;

	pWorld->AdjustXP(player, XP_USE_SKILL);

	startX = tileX - skillRange;	startX = AMax(0, startX);
	startY = tileY - skillRange;	startY = AMax(0, startY);
	endX = tileX + skillRange; endX = AMin(WORLD_WIDTH - 1, endX);
	endY = tileY + skillRange; endY = AMin(WORLD_HEIGHT - 1, endY);

	for(loopY = startY; loopY <= endY; loopY++)
		for(loopX = startX; loopX <= endX; loopX++)
		{
			pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
			if(pUnit)
			{
				// if an enemy unit
				if(pUnit != this && DoIHate(pUnit) && !pUnit->ImmuneToSkillFrom(this))
					pUnit->BeAfraidOf(gSelf);

//				if(pUnit->mountSerial)
//				{
//					pUnit = (Unit *)(pWorld->ValidateTarget(pUnit->mountSerial));
//					if(pUnit && pUnit != this && DoIHate(pUnit))
//						pUnit->BeAfraidOf(gSelf);				
//				}
			}
		}

	RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
	pWorld->LaunchSound(SND_CACKLE, tileX, tileY);
	tNextStateCheck = ATicks() + (TICKS_PER_SEC / 2);
}

// NYMPH lure skill
void
Unit::CycleLure(void)
{
	Unit		*pUnit;
	int32		startX, startY, endX, endY;
	uint32	loopX, loopY;
	grip		gAni;

	if(fActionDone)
		return;

	fInAction = TRUE;
	fActionDone = TRUE;

	if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		return;

	pWorld->AdjustXP(player, XP_USE_SKILL);

	// figure search X,Y boundaries
	startX = tileX - skillRange;	startX = AMax(0, startX);
	startY = tileY - skillRange;	startY = AMax(0, startY);
	endX = tileX + skillRange; endX = AMin(WORLD_WIDTH - 1, endX);
	endY = tileY + skillRange; endY = AMin(WORLD_HEIGHT - 1, endY);

	// search for units within those boundaries
	for(loopY = startY; loopY <= endY; loopY++)
		for(loopX = startX; loopX <= endX; loopX++)
		{
			pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
			if(!pUnit)
				continue;
			if(pUnit->gSelf == gSelf)
				continue;
			if(pUnit->player == player)
				continue;
			if(pUnit->ImmuneToSkillFrom(this))
				continue;

			// instruct unit to follow us around
	 		pUnit->fAutoControl = TRUE;
			pUnit->searchForEnemiesPresetSerial = NULL;
	 		pUnit->tFearLureStonedTimer = ATicks() + LURE_DURATION;
			pUnit->SetPrimaryAction(FOLLOW, tileX, tileY);
			// charmed animation
			TRACK_MEM("LureAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_HEAL_REPAIR, 1,
				pUnit->priorityLevel + 2, TRUE, FALSE, 3, pUnit->tileX, pUnit->tileY, FALSE,
				pUnit->tileXOff, pUnit->tileYOff);
			pWorld->vPort.CheckAni(gAni);

			if(pUnit->fCursorTracking)
			{
				pWorld->DrawUnitStatus(pUnit->serialNum, TRUE);
			}
		}

	// strike a pose
	RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
	pWorld->LaunchSound(SND_FEMALE_SONG, tileX, tileY);
	tNextStateCheck = ATicks() + (TICKS_PER_SEC / 2);
}

void
Unit::BeAfraidOf(grip gScarer)
{
	grip	gAni;

	Unit	*pScarer = ADerefAs(Unit, gScarer);

	// run away from target unit
	fAutoControl = TRUE;
	searchForEnemiesPresetSerial = NULL;
	tFearLureStonedTimer = ATicks() + FEAR_DURATION;
	SetPrimaryAction(AVOID, pScarer->tileX, pScarer->tileY);

	TRACK_MEM("FearAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_FEAR, 1,
		priorityLevel + 2, TRUE, FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff);
	pWorld->vPort.CheckAni(gAni);

	if(fCursorTracking)
	{
		pWorld->DrawUnitStatus(serialNum, TRUE);
	}
}

// GORGON venom skill
//void
//Unit::CycleVenom(void)
//{
//	Unit	*pUnit;
//
//	if(fActionDone)
//		return;	// we're already done - sit and wait
//
//	if(!fInAction)
//	{
//		fInAction = TRUE;
//
//		if(secondaryTargetType != UNIT)
//		{
//			fActionDone = TRUE;
//			return;
//		}
//		if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
//		{
//			fActionDone = TRUE;
//			return;
//		}
//
//		currDir = GetDir(secondaryX - tileX, secondaryY - tileY);
//		RunAnimation(ST_ANIM_ATTACK2, 4, FALSE, currDir, TRUE, TRUE);
//		return;
//	}
//
//	if(currAnimState == ST_ANIM_ATTACK && scrim.cel == 3)
//	{
//		pUnit = (Unit *)ValidateSecondaryTarget();
//		if(!pUnit)
//			return;
//
//		// if target not immune and not already stoned
//		if(!pUnit->ImmuneToSkillFrom(this))
//		{
//			pUnit->SetPrimaryAction(GET_STONED);
//		}
//	}
//	if(fAniDone)
//		fActionDone = TRUE;
//}

void
Unit::EndStonedMode(void)
{
	Unit		*pUnit;
	grip		gAni;

	TRACK_MEM("StonedAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_GENERIC_UNSTONED, 1,
		priorityLevel + 2, TRUE, FALSE, 4, tileX, tileY, FALSE, tileXOff, tileYOff);
	pWorld->vPort.CheckAni(gAni);
	pWorld->LaunchSound(SND_CRUMBLE, tileX, tileY);

	fAutoControl = FALSE;

	SetSide(player);	// in case  unit belongs to auto-control side, restore that setting

	// strike some kind of pose, in case SetPrimary() doesnt call SetAnim() right away
	RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
	pWorld->vPort.CheckAni(gSelf);
	primaryAction = NO_ACTION;

	if(fPackFollower)
	{
		pUnit = (Unit *)pWorld->ValidateTarget(packLeaderSerial);
		if(pUnit)
		{
			pUnit->AddFollower(this);	// in case they dropped us
			pUnit->UpdateFollower(this);
		}
		else
			FindALeader();
	}
	else SetPrimaryAction(MOVE_TO, tileX, tileY);

	if(fCursorTracking)
		pWorld->DrawStatusBox();
}

// GORGON venom state (stoned unit)
void
Unit::CycleGetStoned(void)
{
	ticks_t	currTicks = ATicks();

	if(currTicks < tNextStateCheck)
		return;

	if(fActionDone)
		return;	// sitting and waiting - we've already executed previously

	if(!fInAction)
	{
		// start stoning process

		fInAction = TRUE;
		fInMotion = FALSE;
		if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
			pWorld->vPort.DeleteAni(gAuxAni);
		gAuxAni = NULL;

		// stoned units should not appear up in the air still
		if(priorityLevel > PRI_WALKER)	
			priorityLevel = PRI_WALKER;

		RunAnimation(ST_ANIM_STONED, 0, FALSE, currDir);
//		RunAnimation(ST_ANIM_STONED, 0, FALSE, RIGHT);	// BUGBUG! Kludge to avoid scale+mirror crash bug
		fAutoControl = TRUE;

		if(fCursorTracking)
		{
			pWorld->DrawStatusBox();
		}

		if(currTerrain == CHASM || currTerrain == WATER || currTerrain == CAULDRON
			|| currTerrain == PRISON_PIT)
		{
			// hold before dying
			tNextStateCheck = currTicks + TICKS_PER_SEC / 2;
			return;
		}
		else if(movement == FLIER)
		{
			// begin death fall to bottom of current tile
			MoveAnim(tileX, tileY, tileXOff, TILE_VERT_BOTTOM, 2, 2, 2);
			tNextStateCheck = currTicks + 1;
		}
		else
		{
			// wait a while before coming out
			tNextStateCheck = currTicks + STONED_DURATION;	
		}
		return;
	}

	// stoning process already started - monitor as necessary
	if(currTerrain == CHASM || currTerrain == WATER || currTerrain == CAULDRON || currTerrain == PRISON_PIT)
	{
		// plummet to your death
		Die(NULL, UNIT_TERRAIN);
		return;
	}

	if(movement == FLIER)
	{
		// are we done falling yet?
		if(!fInMotion)
		{
			fActionEnding = TRUE;

			// run crumble animation
			EndStonedMode();
			// hide self
			Hide();
			scrim.resNum = 0;

			Die(NULL, GORGON);
			return;
		}
		else return;
	}
	else
	{
		// wake up
		EndStonedMode();
		return;
	}
}

// old version, before individual stoned ani's existed
//
//void
//Unit::CycleGetStoned(void)
//{
//	BAM_Ani	*pAni;
//
//	if(ATicks() < tNextStateCheck)
//		return;
//
//	if(fActionDone)
//		return;	// sitting and waiting - we've already executed previously
//
//	if(!fInAction)
//	{
//		if(fCursorTracking)
//			pWorld->SetVerbButtons(UNIT, serialNum);
//
//		fInAction = TRUE;
//		fInMotion = FALSE;
//		if(gAuxAni)
//			pWorld->vPort.DeleteAni(gAuxAni);
//		TRACK_MEM("StonedAni");	gAuxAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_GENERIC_STONED, 1,
//			priorityLevel + 2, TRUE, FALSE, 4, tileX, tileY, TRUE, tileXOff, tileYOff);
//		pWorld->vPort.CheckAni(gAuxAni);
//		fAutoControl = TRUE;
//		return;
//	}
//	else if(!fActionEnding)
//	{
//		pAni = ADerefAs(BAM_Ani, gAuxAni);
//		if(pAni->scrim.cel == 3 && scrim.resNum)
//		{
//			// remove unit ani, leaving only rock
////			Hide();
////			scrim.resNum = 0;
//		}
//
//		if(pAni->fAniDone)
//		{
//			Hide();
//			scrim.resNum = 0;
//			currAnimState = ST_ANIM_NULL;
//			// stoning process complete
//
//			if(movement == FLIER)
//			{
//				pAni = ADerefAs(BAM_Ani, gAuxAni);
//				if(!pAni->fInMotion)
//				{
// 					if(pAni->tileYOff != TILE_VERT_BOTTOM)
//					{
//						// begin death fall
//						pAni->MoveAnim(tileX, tileY, tileXOff, TILE_VERT_BOTTOM, 2, 2, 2);
//					}
//					else
//					{
//						// death fall finished.
//						// Start breakup.
//						fActionEnding = TRUE;
//					}
//				}
//			}
//			else
//			{
//				// delay until crumble and release
//				tNextStateCheck = ATicks() + TICKS_PER_SEC * 20;
//				fActionEnding = TRUE;
//			}
//		}
//	}
//	else
//	{
//		EndStonedMode();
//
//		if(movement == FLIER)
//		{
//			Die();
//		}
//		return;
//	}
//}

// BRIGAND theft skill
void
Unit::CycleSteal(void)
{
	Unit	*pUnit;

	if(fActionDone)
		return;	// sitting and waiting - we've already executed previously

	fInAction = TRUE;
	fActionDone = TRUE;

	if(secondaryTargetType != UNIT)
		return;

	pUnit = (Unit *)ValidateSecondaryTarget();
	if(!pUnit)
		return;

	currDir = GetDir(secondaryX - tileX, secondaryY - tileY);
	// stand on last frame
	RunAnimation(ST_ANIM_ATTACK, 3, FALSE, currDir, TRUE, TRUE);
	tNextStateCheck = ATicks() + TICKS_PER_SEC;

	if(pUnit->item && pUnit->item != HARPY_NET)
	{
		AcquireItem(pUnit->item);
		if(fCursorTracking)
			pWorld->DrawItemPortrait(item);
		pUnit->LoseItem();
	}
}

void
Unit::CycleFlameAttack(void)
{
	Unit		*pUnit;
	MapSpace	*pSpace;
	direction attackDir;
	int32		pixelX, pixelY;
	uint32	newTargetSerial;
	ticks_t	currTicks = ATicks();

	if(fActionDone)
		return;	// sitting and waiting - we've already executed previously

	if(currTicks < tNextStateCheck)
	{
		// not time to recycle yet
		return;
	}

	if(!fInAction)
	{
		fInAction = TRUE;
		if(!pWorld->SpendMana(player, skillCost, tileX, tileY))
		{
			fActionDone = TRUE;
			return;
		}

		attackDir = GetDir(secondaryX - tileX, secondaryY - tileY);
		currDir = attackDir;
		// stand on last frame
		RunAnimation(ST_ANIM_ATTACK, 3, FALSE, attackDir, TRUE, TRUE);
		return;
	}

	// time to fire?
	if(currAnimState == ST_ANIM_ATTACK && scrim.cel < 2)
	{
		// not yet
		return;
	}

	// validate target (safety net)
	newTargetSerial = NULL;

	switch(secondaryTargetType)
	{
		case UNIT:
			pUnit = (Unit *)ValidateSecondaryTarget();
			if(!pUnit)
			{
				fActionDone = TRUE;
				return;
			}
//			UnitDebug.Out("Flame UNIT s%d ", secondaryTargetSerial);
			newTargetSerial = pUnit->serialNum;
			pixelX = pUnit->tileX * TILE_WIDTH + pUnit->tileXOff;
			pixelY = pUnit->tileY * TILE_HEIGHT + pUnit->tileYOff;
			break;
	
		case BUILDING:
			pSpace = (MapSpace *)ValidateSecondaryTarget();
			if(!pSpace || pSpace->h.currHP < 1)
			{
				fActionDone = TRUE;
				return;
			}
			newTargetSerial = pSpace->serialNum;
			pixelX = secondaryX * TILE_WIDTH + TILE_WIDTH / 2;
			pixelY = secondaryY * TILE_HEIGHT + TILE_HEIGHT / 2;
			break;

		case TERRAIN:
			pixelX = secondaryX * TILE_WIDTH + TILE_WIDTH / 2;
			pixelY = secondaryY * TILE_HEIGHT + TILE_HEIGHT / 2;
			break;
	}

	// mana penalty for skill use
	switch(type)
	{
		case WYRM:
			pWorld->LaunchSound(SND_HISSING_SHOT, tileX, tileY);
			break;
	}

	pWorld->AdjustXP(player, XP_USE_SKILL);

	tNextStateCheck = ATicks() + (TICKS_PER_SEC / 2);

	TRACK_MEM("Projectile");	Projectile *pFireball = new Projectile(this);
	pFireball->RunAnimation(5, TRUE, baseAnim, currDir, TRUE);
	pFireball->SetTilePos(tileX, tileY, tileXOff, tileYOff - 19);
	pWorld->vPort.CheckAni(pFireball->gSelf);
//	UnitDebug.Out("FireBall(g%d x%d y%d) s%d x%d y%d xo%d y%d\n",
//		pFireball->gSelf, pFireball->tileX, pFireball->tileY,
//		newTargetSerial, pixelX / TILE_WIDTH, pixelY / TILE_HEIGHT,
//		pixelX % TILE_WIDTH, pixelY % TILE_HEIGHT);
	pFireball->Launch(newTargetSerial, pixelX / TILE_WIDTH, pixelY / TILE_HEIGHT,
		pixelX % TILE_WIDTH, pixelY % TILE_HEIGHT, 4, 4, 1, DMG_WYRM_FLAME);
	fActionDone = TRUE;
}

void
Unit::CycleAttack(void)
{
	Unit		*pUnit;
	ticks_t	currTicks = ATicks(), int1;
	MapSpace	*pBuilding;
	int		tempX, tempY;
	grip		gAni;

	if(!fInAction)	// if attack hasnt started yet
	{
		#ifndef NDEBUG
		UnitDebug.Out("CA()1");
		#endif

		fInAction = TRUE;

		if(secondaryTargetType != UNIT && secondaryTargetType != BUILDING)
		{
			fActionDone = TRUE;
			return;
		}

		if(type == BRIGAND)
		{
			// a mistake has occured - BRIGANDs steal, not attack
			fActionDone = TRUE;
			return;
		}
		else if(type == ACOLYTE && baseAttack == 0)
		{
			// error!  skater mode!  Repair it.
			currTerrain = UNDEFINED;
			currState = ST_NULL;
			SetTilePos(tileX, tileY);
		}

		// begin pre-attack animation

		tempX = secondaryX - tileX;
		tempY = secondaryY - tileY;
		if(type == JUGGERNAUT)
		{
			// adjust for JUGGERNAUT's lower-right awareness
			if(tempX == -1)
				tempX = 0;
			if(tempY == -1)
				tempY = 0;
		}
		currDir = GetDir(tempX, tempY);
		if(movement == WALKER || movement == FLOATER)
		{
			// strike a pose, no animation
			switch(type)
			{
				case JUGGERNAUT:
					if((GetDistanceTo(secondaryX, secondaryY) > 1) || secondaryTargetType == TERRAIN)
						RunAnimation(ST_ANIM_ATTACK2, 0, FALSE, currDir, TRUE, FALSE);
					else RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
					break;

				case RANGER:
				case GOBLIN:
					if(GetDistanceTo(secondaryX, secondaryY) > 1)
						RunAnimation(ST_ANIM_ATTACK2, 0, FALSE, currDir, TRUE, FALSE);
					else RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
					break;

				case GORGON:
//					pUnit = (Unit *)ValidateSecondaryTarget();
//					if(pUnit && !pUnit->ImmuneToSkillFrom(this))
//						RunAnimation(ST_ANIM_ATTACK2, 0, FALSE, currDir, TRUE, FALSE);
//					else
					RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
					break;

				default:
					RunAnimation(ST_ANIM_ATTACK, 0, FALSE, currDir, TRUE, FALSE);
					break;
			}
		}
		else
		{
			// hold in walk anim
			RunAnimation(ST_ANIM_WALK, move2ticks[currMove], TRUE, currDir);
		}

		// first-swing wind up
		if(pBam->playerTypes[player] != PLAYER_COMPUTER &&
			lastOurAttackTicks > currTicks - TICKS_PER_SEC * 2 &&
			!tBerserkTimer)
		{
			// extend delay to prevent double-attacking
			tNextStateCheck = lastOurAttackTicks + TICKS_PER_SEC * 3;
		}
		else tNextStateCheck = currTicks + TICKS_PER_SEC / 2;
		tNextStateCheck += ARandom(2);
		lastOurAttackTicks = currTicks;

		#ifndef NDEBUG
		UnitDebug.Out("CA()2");
		#endif
		return;
	}

	// if non-walker ani finishes
	if((movement == FLIER || movement == SWIMMER) && fAniDone)
	{
		// restart flying/swimming anim in cycle mode (wont finish)
		currDir = GetDir(secondaryX - tileX, secondaryY - tileY);
		RunAnimation(ST_ANIM_WALK, 2, TRUE, currDir);
	}

	if(currTicks >= tNextStateCheck)
	{
		#ifndef NDEBUG
		UnitDebug.Out("CA()3");
		#endif
		// kludgy bug patch
		// Periodic inspection: has our entry in the grip map been walked on?
		if(pWorld->unitLib.GetUnitGrip(tileX, tileY) != gSelf)
		{
			UnitDebug.Out("\n%s(%d,%d) repairing grip map", unitName, tileX, tileY);
			pWorld->unitLib.PlaceUnitGrip(gSelf, tileX, tileY);
		}

		if(type == BANSHEE && currTicks >= tAutoActionTimer)
		{
			fActionDone = TRUE;
			return;
		}
		else
		{
			// any other reason to break off attack?
			if(secondaryTargetSerial != primaryTargetSerial)
			{
				// we're supposed to be doing something else.  Are we not close enough for it?
				if(!WithinRangeForPrimary())
				{
					// nope.. so, should we resume travel?
					int tempX, tempY, pathLength = FindPathTo((coord)primaryX, (coord)primaryY,
						pathArray, fStopAdjacent);

					curPathPosition = 0;
					if(pathLength > 0)
					{
						PeekNextTarget(tempX, tempY);
						if(CanIMoveTo(tempX, tempY, TRUE))
						{
							// a path is avail, so stop screwing around and get back to work
							fActionDone = TRUE;
							return;
						}
						// else path is blocked, nevermind.
					}
					// else path is blocked, nevermind.
				}

				// if we're just attacking a bldg then it's just on general principals, so keep an eye
				// out for enemy units too.
				if(secondaryTargetType == BUILDING)
				{
					pUnit = SearchForEnemies();
					if(pUnit)
					{
						// go get him
						SearchForEnemiesPreset(pUnit);
						fActionDone = TRUE;
						return;
					}
				}
			}

			if(tBerserkTimer && ARandom(4) == 0)
			{
				// periodic re-search for better target
				fActionDone = TRUE;
				return;
			}

			if(type == GUARDIAN && ARandom(2))
			{
				// if current targ already on fire and another target available is not,
				// we'll find him and change here.  (Evil tactic.)
				pUnit = SearchForEnemies();
				if(pUnit && pUnit->serialNum != secondaryTargetSerial)
				{
					SearchForEnemiesPreset(pUnit);
					fActionDone = TRUE;
					return;
				}
			}

			// AI code
			if(pBam->playerTypes[player] == PLAYER_COMPUTER && CheckActionAvailable(SKILL))
			{
				// before we attack, should we use our skill first?
				if(skillType == SKILL_AGGRESSIVE)
				{
					if((secondaryTargetType == UNIT || type == WYRM) &&
						(pWorld->ai.CheckSpellIQ() && !ARandom(3)))
					{
						if(pWorld->mana[player] < skillCost &&
							pWorld->ai.CheckCheat(AICHEAT_AGGRSKILL_NEED_MANA))
						{
							pWorld->mana[player] = skillCost;
						}

						if(pWorld->mana[player] >= skillCost)
						{
							// razzle dazzle!
							if(fSkillNeedsTarget)
								SetAction(SKILL, secondaryX, secondaryY);
							else SetAction(SKILL, tileX, tileY);
							return;
						}
					}
				}

				if(type == CLERIC || type == PALADIN)
				{
					// we're a healer - does a neighor need us?	
					if(pWorld->ai.ShouldWeUseSkill(this))
					{
						pUnit = SearchForPatients();
						if(pUnit)
						{
							SetAction(SKILL, pUnit->tileX, pUnit->tileY);
							return;
						}
					}
				}
			}

			// okay, go for it
			#ifndef NDEBUG
			UnitDebug.Out("CA()4");
			#endif
			SetupForAttack();
			return;
		}
	}

	if(currTicks >= cycleNext)	// if time for next frame of swing
	{
		if(fAniDone && currState == ST_ATTACKING)		// if last cel of swing
		{
			currState = ST_WAIT2ATTACK;

			if(type == JUGGERNAUT)
			{
				// only attacks each target once
				SetAction(GUARD);
			}
			return;
		}
		// the following is necessary because after cel#2, ANIM_HIT might be run,
		// which also has a cel#2
		else if(currAnimState == ST_ANIM_ATTACK && scrim.cel == 1)
		{
			#ifndef NDEBUG
			UnitDebug.Out("CA()5");
			#endif

//			// to watch for double-attack
//			if(lastOurAttackTicks + TICKS_PER_SEC * 3 > currTicks && pBam->playerTypes[player] == PLAYER_LOCAL
//				|| bGlobal.storyLine == NETGAME)
//			{
//				// someone's using the double-attack bug!  Payback time.
//				switch(ARandom(6))
//				{
//					case 0:
//					case 1:
//					case 2:
//						if(pWorld->map.FireCanBurnAt(tileX, tileY))
//						{
//							TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_FIRE,
//								tileX, tileY);
//							break;
//						}
//
//					case 3:
//					case 4:	
//						TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_ARCH_FIRE,
//							tileX, tileY);
//						break;
//
//					case 5:
//						pWorld->AdjustManaLevel((pBam->playerSide == SIDE2)? SIDE1: SIDE2, MANA_SUMMON);
//						pWorld->Summon(GUARDIAN, tileX - 2 + ARandom(5), tileY - 2 + ARandom(5),
//							(pBam->playerSide == SIDE2)? SIDE1: SIDE2);
//						break;
//				}
//			}
			lastOurAttackTicks = currTicks;

			int	damageDone, oldBldgOwner, shotXOff, shotYOff;

			switch(secondaryTargetType)
			{
				case UNIT:
					#ifndef NDEBUG
					UnitDebug.Out(" 5.1");
					#endif
					pUnit = (Unit *)ValidateSecondaryTarget();
					if(!pUnit)
					{
						fActionDone = TRUE;
						return;
					}
					#ifndef NDEBUG
					if((int)pUnit == 1)
					{
						APanic("CA()5: pUnit == 1!");
					}
					#endif

					if(type == GORGON && pUnit->primaryAction == GET_STONED)
					{
						// dont attack again, it would free him
						fActionDone = TRUE;
						return;
					}

					// in case target has moved
					secondaryX = pUnit->tileX;
					secondaryY = pUnit->tileY;
					shotXOff = pUnit->tileXOff;
					shotYOff = pUnit->tileYOff;
				
					#ifndef NDEBUG
					UnitDebug.Out(" 5.2");
					#endif
					break;

				case BUILDING:
					#ifndef NDEBUG
					UnitDebug.Out(" 5.3");
					#endif

					// look at target structure
					pBuilding = (MapSpace *)ValidateSecondaryTarget();
					if(!pBuilding)
					{
						fActionDone = TRUE;
						return;
					}

					#ifndef NDEBUG
					if((int)pBuilding == 1)
					{
						APanic("CA()5: pBldg == 1!");
					}
					#endif
					if(pBuilding->h.currHP < 1)	// if bldg is dead
					{
						fActionDone = TRUE;
						return;
					}
					oldBldgOwner = pBuilding->h.owner;
					shotXOff = TILE_HORZ_CENTER;
					shotYOff = TILE_VERT_CENTER;
					#ifndef NDEBUG
					UnitDebug.Out(" 5.4");
					#endif
					break;

				case TERRAIN:
					shotXOff = TILE_HORZ_CENTER;
					shotYOff = TILE_VERT_CENTER;
					break;
			}

			// some units fire projectiles, instead of direct melee
			bool	fFireProjectile = FALSE;
			switch(type)
			{
				case WIZARD:
					fFireProjectile = TRUE;
					break;

				case JUGGERNAUT:
					if(secondaryTargetType == TERRAIN)
					{
						fFireProjectile = TRUE;
						break;
					}
					// fall-through intentional

				case RANGER:
				case GOBLIN:
					if(GetDistanceTo(secondaryX, secondaryY) > 1)
						fFireProjectile = TRUE;
					break;

				case GUARDIAN:
					if(GetDistanceTo(secondaryX, secondaryY) > 1)
						fFireProjectile = TRUE;
					break;
			}

			if(fFireProjectile)
			{
				#ifndef NDEBUG
				UnitDebug.Out(" 5.5");
				#endif
				Projectile	*pShot;

				TRACK_MEM("Projectile");	pShot = new Projectile(this);

				if(type == JUGGERNAUT)
					pShot->baseAnim = 1800;	// kludge - steal projectile ani from Wyrm

				pShot->RunAnimation(5, TRUE, pShot->baseAnim, currDir, TRUE);
	 			if(type == JUGGERNAUT)
				{
					switch(currDir)
					{
						case DOWN:
							tempX = tileX - 1;
							tempY = tileY;
							break;

						case UP:
							tempX = tileX;
							tempY = tileY - 1;
							break;

						case UP_LEFT:
						case LEFT:
						case DOWN_LEFT:
							tempX = tileX - 1;
							tempY = tileY;
							break;

						case DOWN_RIGHT:
						case RIGHT:
						case UP_RIGHT:
						default:
							tempX = tileX;
							tempY = tileY;
							break;

					}
				}
				else
				{
					tempX = tileX;
					tempY = tileY;
				}

				#ifndef NDEBUG
				UnitDebug.Out(" 5.6(%x %d %d %d %d)", (int)pShot, tempX, tempY, tileXOff, tileYOff - 19);
				#endif
				pShot->SetTilePos(tempX, tempY, tileXOff, tileYOff - 19);
				UnitDebug.Out(" 5.61(%d)", (int)pShot->gSelf);
				pWorld->vPort.CheckAni(pShot->gSelf);
				UnitDebug.Out(" 5.62(%d %d)", tileX, tileY);

				// sound for projectile launch
				switch(type)
				{
					case WIZARD:
						pWorld->LaunchSound(SND_ZAP, tileX, tileY);
						int1 = 5;
						break;

					case JUGGERNAUT:
					case GUARDIAN:
						pWorld->LaunchSound(SND_HISSING_SHOT, tileX, tileY);
						int1 = 5;
						break;

					case RANGER:
						pWorld->LaunchSound(SND_BOW_TWANG, tileX, tileY);
						int1 = 6;
						break;

					case GOBLIN:
						pWorld->LaunchSound(SND_SPEAR_CHUCK, tileX, tileY);
						int1 = 4;
						break;

					default:
						pWorld->LaunchSound(SND_GIGGLE, tileX, tileY);
						int1 = 5;
						break;
				}
				UnitDebug.Out(" 5.63(%d %d %d %d %d %d %d)", (int)secondaryTargetSerial,
					secondaryX, secondaryY, shotXOff, shotYOff - 9, int1, (int)currAttack);
				pShot->Launch(secondaryTargetSerial, secondaryX, secondaryY,
					shotXOff, shotYOff - 9, int1, int1, 1, currAttack);
				#ifndef NDEBUG
				UnitDebug.Out(" 5.7");
				#endif
			}
			else
			{
				#ifndef NDEBUG
				UnitDebug.Out(" 5.8(%d,%d)", secondaryX, secondaryY);
				#endif
				// regular melee attack (range==1)

				// attack sound	
				switch(type)
				{
					case GUARDIAN:
						pWorld->LaunchSound(SND_METALLIC_STRIKE, secondaryX, secondaryY);
						break;

					case GORGON:
					case BANSHEE:
					case DRUID:
					case GARGOYLE:
					case GHOUL:
					case GRIFFIN:
					case HARPY:
					case ROC:
					case TORTOISE:
					case WYRM:
					case WOLF:
						pWorld->LaunchSound(SND_SCRAPE, secondaryX, secondaryY);
						break;

					case FURY:
					case GNOME:
					case PALADIN:
					case WARRIOR:
					case ZOMBIE:
						pWorld->LaunchSound(SND_SWORD_CLASH, secondaryX, secondaryY);
						break;

					case ACOLYTE:
					case CLERIC:
					case ENCHANTER:
					case GOLEM:
					case NYMPH:
					case PEASANT:
					case TROLL:
					  pWorld->LaunchSound(SND_WHACK, secondaryX, secondaryY);
					  break;

					case RANGER:
					case GOBLIN:
					case WRAITH:
						pWorld->LaunchSound(SND_CHOPPING_STRIKE, secondaryX, secondaryY);
						break;

					case JUGGERNAUT:
						pWorld->LaunchSound(SND_SAWBLADE_STRIKE, secondaryX, secondaryY);
						break;

					default:
					  pWorld->LaunchSound(SND_GIGGLE, secondaryX, secondaryY);
					  break;
				}

				//HEYYYY BATTERBATTERBATTERBATTER-SWING!!!
				switch(secondaryTargetType)
				{
					case UNIT:		
						#ifndef NDEBUG
						UnitDebug.Out(" 5.91");
						#endif

						damageDone = pUnit->currHP;
						damageDone -= pUnit->TakeAHit(this, type, tileX, tileY, currAttack);

						switch(type)
						{
							case GHOUL:		// ghouls suck
								if(!pUnit->ImmuneToSkillFrom(this))
								{
									currHP = AMin(maxHP, currHP + damageDone);
									if(fCursorTracking)
										pWorld->UpdateHPGuageOnly(currHP, maxHP, player);
								}
								break;

							case GORGON:
								if(!pUnit->ImmuneToSkillFrom(this))
								{
									pUnit->SetPrimaryAction(GET_STONED);
								}
								break;

							case GUARDIAN:
								TRACK_MEM("ExplodeAni");	gAni = pWorld->vPort.NewAni(RES_ANIM,
									baseAnim + ANIM_IMPACT, 1, PRI_SKY,	TRUE, FALSE, 3,
									secondaryX, secondaryY, FALSE);
								pWorld->vPort.CheckAni(gAni);

								TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_ARCH_FIRE, secondaryX,
									secondaryY);
								break;
						}
						#ifndef NDEBUG
						UnitDebug.Out(" 5.92");
						#endif
						break;

					case BUILDING:
						#ifndef NDEBUG
						UnitDebug.Out(" 5.93");
						#endif
						pBuilding->TakeAHit(this, type, tileX, tileY, currAttack, secondaryX, secondaryY);
						switch(type)
						{
							case GUARDIAN:
								TRACK_MEM("ExplodeAni");	gAni = pWorld->vPort.NewAni(RES_ANIM,
									baseAnim + ANIM_IMPACT, 1, PRI_SKY,	TRUE, FALSE, 3,
									secondaryX, secondaryY, FALSE);
								pWorld->vPort.CheckAni(gAni);

								TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_ARCH_FIRE, secondaryX,
									secondaryY);
								break;
						}
						#ifndef NDEBUG
						UnitDebug.Out(" 5.94");
						#endif
						break;
				}
			}
			#ifndef NDEBUG
			UnitDebug.Out("CA()6");
			#endif
	  	}
	}
}

void		// setup anim parameters for unit taking a swing at target pos
Unit::SetupForAttack(void)
{
	Unit			*pUnit;
	MapSpace		*pBuilding;
	bool			fAbortAttack = FALSE;
	ticks_t		currTicks = ATicks();

	// in case target has moved recently, get new target pos
	switch(secondaryTargetType)
	{
		case UNIT:
			pUnit = (Unit *)ValidateSecondaryTarget();
			if(pUnit && GetDistanceTo(pUnit, UNIT) <= currRange)
			{
				secondaryX = pUnit->tileX;		// make sure we're pointing the right direction
				secondaryY = pUnit->tileY;		// in case target has moved
				break;
			}
			fAbortAttack = TRUE;
			break;

		case BUILDING:
			pBuilding = (MapSpace *)ValidateSecondaryTarget();			
			if(!pBuilding || GetDistanceTo(pBuilding, BUILDING) > currRange)
				fAbortAttack = TRUE;

			// if a unit has moved on top of the tile we want to attack
			if(pWorld->unitLib.GetUnitGrip(secondaryX, secondaryY))
			{
				// have to pick another tile
				fAbortAttack = TRUE;
			}
			break;

		default:
			if(type != JUGGERNAUT)
			{
				// only JUGGERNAUT can ATTACK terrain
				fAbortAttack = TRUE;
			}
			break;
	}

	if(fAbortAttack)
	{
		fActionDone = TRUE;
		tNextStateCheck = currTicks;	// trigger re-think immediately
		return;
	}

	currDir = GetDir(secondaryX - tileX, secondaryY - tileY);
	if((type == JUGGERNAUT || type == RANGER || type == GOBLIN) && GetDistanceTo(secondaryX, secondaryY) > 1)
	{
		RunAnimation(ST_ANIM_ATTACK2, 3, FALSE, currDir);
	}
	// GORGON now using normal attack ani again
//	else if(type == GORGON && secondaryTargetType == UNIT && !pUnit->ImmuneToSkillFrom(this))
//	{
//		RunAnimation(ST_ANIM_ATTACK2, 3, FALSE, currDir);
//	}
	else
	{
		RunAnimation(ST_ANIM_ATTACK, move2AttackTicks[currMove], FALSE, currDir);
	}
	if(primaryAction == BERSERK)
		tNextStateCheck = currTicks + (TICKS_PER_SEC * 15) / 10;	// next re-think in .66 seconds
	else if(type == RANGER && GetDistanceTo(secondaryX, secondaryY) > 1)
	{
		// extended wind-up
		tNextStateCheck = currTicks + TICKS_PER_SEC * 5;	// next re-think in 5 seconds
	}
	else tNextStateCheck = currTicks + TICKS_PER_SEC * 2;	// next re-think in 2 seconds
	currState = ST_ATTACKING;
}


uint32
Unit::FindPathTo(coord xPos, coord yPos, int8 *tempPathArray, bool fStopAdjacent, bool fThroughFriendlies,
	bool fBulldozer, int *pFinalX, int *pFinalY, bool fThroughStillFriendlies)
{
	int16			*pStructureMap, *pTileMap;
	int			tempX, tempY, int1, int2;
	int32			xDif, yDif, *pFogMap;
	uint32		i, j, tempPathLength;
	char			mapData[WORLD_WIDTH * WORLD_HEIGHT], *pData = mapData, moveRate1;
	UnitGroup	*pUGroup = pWorld->unitLib.GetUnitGroup(type);
	bool			tempBuffer, fSeeFog;
	int			numSteps;
	Unit			*pUnit;
	grip			g, *pUnitMap;
	MapSpace		*pBldg;
	tile			*pTiles;
	terrainType terrain1;

	#ifndef NDEBUG
	if(fCursorTracking)
	UnitDebug.Out("\n(s%d %s) path @%dt %d/%d-%d/%d fSA%d fTF%d fB%d fTSF%d ",
		serialNum, unitName, ATicks(), tileX, tileY, xPos, yPos,
		(int)fStopAdjacent, (int)fThroughFriendlies, (int)fBulldozer,
		(int)fThroughStillFriendlies);
	#endif

	// if actual path is not desired
	if(!tempPathArray)
	{
		// allocate some temp space
		tempBuffer = TRUE;
		TRACK_MEM("tempPathArray");	tempPathArray = new int8[MAX_PATH_SIZE * 2];
	}
	else
 	{
		tempBuffer = FALSE;
	}

	if(pathCache.tLastPathCall == ATicks() && pathCache.sLastUnitToPath ==
		serialNum && pathCache.startX == tileX && pathCache.startY == tileY &&
		pathCache.destX == xPos && pathCache.destY == yPos &&
		pathCache.fStopAdjacent == fStopAdjacent && pathCache.fThroughFriendlies
		== fThroughFriendlies && pathCache.fBulldozer == fBulldozer &&
		pathCache.fThroughStillFriendlies == fThroughStillFriendlies)
	{
		#ifndef NDEBUG
		if(fCursorTracking)
			UnitDebug.Out("cache hit!");
		#endif

		memcpy(tempPathArray, pathCache.pathArray, sizeof(pathCache.pathArray));
		tempPathLength = GetPathLength(tempPathArray);

		if(tempBuffer)
			delete tempPathArray;
		return(tempPathLength);
	}
	else
	{
		pathCache.tLastPathCall = ATicks();
		pathCache.sLastUnitToPath = serialNum;
		pathCache.startX = tileX;
		pathCache.startY = tileY;
		pathCache.destX = xPos;
		pathCache.destY = yPos;
		pathCache.fStopAdjacent = fStopAdjacent;
		pathCache.fThroughFriendlies = fThroughFriendlies;
		pathCache.fBulldozer = fBulldozer;
		pathCache.fThroughStillFriendlies = fThroughStillFriendlies;
	}

	if(type == JUGGERNAUT)
	{
		#ifndef NDEBUG
		if(fCursorTracking)
			UnitDebug.Out("\n");
		#endif

		// JUGGERNAUT code
		curPathPosition = 0;
		pathLength = 0;
	 	memset(tempPathArray, -1, MAX_PATH_SIZE * 2);

		// determine direction in terms of x/y offsets
		xDif = xPos - tileX;	xDif = (xDif? xDif / abs(xDif): 0);
		yDif = yPos - tileY;	yDif = (yDif? yDif / abs(yDif): 0);

		// calc only one tile step and add to path
		pathDestX = tileX + xDif;
		pathDestY = tileY + yDif;

		// if there is a change of pos and we can go there
		if((pathDestX != tileX || pathDestY != tileY) && CanIMoveTo(pathDestX, pathDestY))
		{
			AddToPath(tempPathArray, pathDestX, pathDestY);
		}
		else
		{
//			AddToPath(tempPathArray, tileX, tileY);
//			secondaryX = tileX;
//			secondaryY = tileY;	// BUGBUG - this was causing problems

			// force termination of motion - crude method, so we'll ONLY do this for
			// the JUGGERNAUT, which has very simple behavioral rules anyway.
		}

		tempPathLength = GetPathLength(tempPathArray);
	}
	else if(AMax(abs(xPos - tileX), abs(yPos - tileY)) == 1)
	{
		// just a single-step, here's a hardcoded shortcut

		#ifndef NDEBUG
		if(fCursorTracking)
			UnitDebug.Out("single step\n");
		#endif

		// pre-fill the first 2 locs in the buffer w/ -1, since some
		// areas of the code look for -1 as end-of-path

		memset(tempPathArray, -1, sizeof(*tempPathArray) * 4);
		tempPathLength = 0;
		if(CanIMoveTo(xPos, yPos))
		{
			pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
			if(!pUnit)
			{
				// road is clear
				AddToPath(tempPathArray, xPos, yPos, TRUE);
				tempPathLength++;
			}
			else if(pUnit->player == player && fThroughFriendlies)
			{
				// he's friendly, so maybe it's okay
				if(pUnit->currAction != GUARD || fThroughStillFriendlies)
				{
					// either he's moving (so it's okay), or we just dont care
					AddToPath(tempPathArray, xPos, yPos, TRUE);
					tempPathLength++;
				}
			}
		}
		if(tileXOff != TILE_HORZ_CENTER || tileYOff != TILE_VERT_CENTER)
		{
			// we're not at the center of our tile.
			// Should we recenter first, or continue towards where we were headed?
			
			// if we're off-center and no path was found, forcibly re-center
			if(!tempPathLength)
			{
				AddToPath(tempPathArray, tileX, tileY, TRUE);
				tempPathLength++;
			}
		}
	}
	else
	{
//		if(player == SIDE1 && pBam->playerTypes[SIDE2] == PLAYER_COMPUTER)
//			fSeeFog = TRUE;
//		else
		fSeeFog = FALSE;

		#ifndef NDEBUG
		if(fCursorTracking)
			UnitDebug.Out("\n");
		#endif

		// first build obstacle & travel rate map
		pUnitMap = &pWorld->unitLib.gUnitMap[0][0];
		pFogMap = &(pWorld->vPort.fog[0][0]);
		pStructureMap = &(pWorld->map.MapSpaces[0]);
		pTileMap = &(pWorld->map.MapTiles[0]);
		pTiles = &pWorld->pTileLib->tiles[0];

		for(j = 0; j < WORLD_HEIGHT; j++)
		{
			pFogMap = &(pWorld->vPort.fog[j][0]);
			pUnitMap = &(pWorld->unitLib.gUnitMap[j][0]);

			for(i = 0; i < WORLD_WIDTH; i++, pUnitMap++, pFogMap++, pData++, pStructureMap++, pTileMap++)
			{
				if(fSeeFog && (*pFogMap == FOG_CENTER))
				{
					*pData = 1;
					continue;
				}
				
				// if blocked by a unit
				g = *pUnitMap;

				if(g)
				{	
					pUnit = ADerefAs(Unit, g);
//					if(pUnit->type == TORTOISE)
//					{
//						if(movement == WALKER && type != JUGGERNAUT)
//						{
//							// walkers can step on tortoises
//							*pData = (char)pUGroup->baseMoveRate;
//						}
//						else
//						{
//							// no one else can walk on a TORTOISE
//							*pData = 0;
//						}
//					}
//					else

					if(DoIHate(pUnit))
					{
						// enemy unit
						if(fBulldozer)
						{
							// go on through, no matter what
						}
						else if(pBam->playerTypes[player] == PLAYER_COMPUTER)
						{
							// can we take him?

							int1 = pUnit->currHP / AMax(1, currAttack - pUnit->currDefense);
							int2 = currHP / AMax(1, pUnit->currAttack - currDefense);

							if(int1 > int2)
							{
								// wimp-out

								*pData = 0;
								continue;
							}
							// else dont worry about him
						}
						else
						{
							// avoid conflict
							*pData = 0;
							continue;
						}
					}
					else
					{
						// friendly unit
						if(pUnit == this)
						{
							// cant be blocked by self (JUGGERNAUT)
						}
						else if(!fThroughFriendlies)
						{
							// dont allow pathing through friendlies
							*pData = 0;
							continue;
						}
						else
						{
							// pathing through friendlies allowed

							if(pUnit->currAction == MOVE_TO || fThroughStillFriendlies)
							{
								// presume he'll have moved away by the time we get there
							}
							else
							{
								// stationary friendly, dont try it.
								*pData = 0;
								continue;
							}
						}
					}
				}

				// check for building
				if(*pStructureMap)
				{
					pBldg = pWorld->map.GetStructure(i, j);
					if(pBldg && pBldg->h.currHP > 0)
					{
						// building is there, but can we go over it?
						if(movement == FLIER || type == GNOME)
						{
							// these can pass over buildings
								// if not blocked by unimportant unit
							if(!g)
								*pData = (char)pUGroup->baseMoveRate;
							continue;
						}
						else
						{
							if(fBulldozer && pBldg->h.owner == SIDE0 && pBldg->h.func == FUNC_MISC)
							{
								// path through buildings - AI will probably attack it when he gets there
								*pData = 1;	// least desirable, but possible
								continue;
							}
							else
							{
								// blocked by structure
								*pData = 0;
								continue;
							}
						}
					}
				}

				if(g)					// if unit in way but not a fatal obstacle
					*pData = 1;		// minimum movement rate - possible but least desirable
				else
				{
					// figure our movement rate for this spot (for choosing not only shortest,
					// but quickest route)
					terrain1 = pTiles[*pTileMap].terrain;
					moveRate1 = (char)GetMoveRate(terrain1);
					if(i == xPos && j == yPos)
					{
						// this is our target.  Anything special about it?
						if(terrain1 == PORTAL && pBam->playerTypes[player] == PLAYER_COMPUTER)
						{
							pBldg = pWorld->map.GetStructure(i, j);
							if(DoIHate(pBldg))
							{
								// computer will not path to an enemy portal
								moveRate1 = 0;	// dont path to it
							}
						}
					}
					else
					{
						// this is not our target.  Anything special about it?
						if(terrain1 == CAULDRON && movement == WALKER)
						{
							moveRate1 = 0;	// dont path across it
						}
					}
					*pData = (char)moveRate1;
				}
//				pData++;
			}
		}

		if(fStopAdjacent)		// if they only want a path UP to X,Y and not ON to X,Y
			mapData[yPos * WORLD_WIDTH + xPos] = 0;		// cant walk there, no matter what

		// make sure that we can walk where we are currently standing
		// (get rid of the unit on this square).
		pData = mapData + tileX + (tileY * WORLD_WIDTH);
//		if(mountSerial)
//			moveRate1 = (char)baseMove;
//		else
		*pData = (char)GetMoveRate(currTerrain);

		int	tempFinalX, tempFinalY;
		if(!pFinalX)
		{
			pFinalX = &tempFinalX;
			pFinalY = &tempFinalY;
		}
		*pFinalX = xPos;
		*pFinalY = yPos;
		numSteps = pWorld->pather.MakePath(mapData, tileX, tileY, pFinalX, pFinalY, tempPathArray);
		tempPathLength = numSteps;

		if(tileXOff != TILE_HORZ_CENTER || tileYOff != TILE_VERT_CENTER)
		{
			// we're not at the center of our tile.
			// Should we recenter first, or continue towards where we were headed?
			
			// determine where we were headed by where we are currently
//			int destX = tileX, destY = tileY;
//			if(tileXOff > TILE_HORZ_CENTER)
//				destX++;
//			else if(tileXOff < TILE_HORZ_CENTER)
//				destX--;
//			if(tileYOff > TILE_VERT_CENTER)
//				destY++;
//			else if(tileYOff < TILE_VERT_CENTER)
//				destY--;
//
//			// peek at first tile in path - 

			// if we're off-center and no path was found, forcibly re-center
			if(!tempPathLength)
			{
				AddToPath(tempPathArray, tileX, tileY, TRUE);
				tempPathLength++;
			}
		}
	}

//	tempPathLength = GetPathLength(tempPathArray);

	if(fThroughFriendlies && tempPathLength > 0 && !fThroughStillFriendlies)
	{
		// validate first step
		tempX = tempPathArray[0];
		tempY = tempPathArray[1];

		// trying to cut down on overhead - only try the following some of the time
		if(!CanIMoveTo(tempX, tempY) && ARandom(3))
		{
			// blocked by friendly!
			// look for alternate (no-friendlies) route after all
			TRACK_MEM("tempPathArray");	int8	*tempPathArray2 = new int8[MAX_PATH_SIZE * 2];

			int tempPathLength2 = FindPathTo(xPos, yPos, tempPathArray2, fStopAdjacent, FALSE);

			// if alt route is no more than old route + 5
//			if(tempPathLength2 < tempPathLength + 5)
			if(tempPathLength2 < (tempPathLength + (tempPathLength >> 1)))
			{
				// substitute this path instead
				tempPathLength = tempPathLength2;
				memcpy(tempPathArray, tempPathArray2, MAX_PATH_SIZE * 2);
			}
			delete tempPathArray2;
		}
	}

	// store path results in cache, in case we can re-use it later
	memcpy(pathCache.pathArray, tempPathArray, sizeof(pathCache.pathArray));

	// de-alloc buffer if it was temp
	if(tempBuffer)
		delete tempPathArray;

/*	// debug stuff
  	FILE*	pFile;
  	// append to any existing file
  	pFile = fopen ("path.dmp", "at");
  	pData = mapData;
  	fprintf(pFile, "    ");
  	for (i = 0; i < WORLD_WIDTH; i++)
  	{
  		fprintf(pFile, "%d", i%10);	
  	}
  	fprintf(pFile, "\n\n");
	if (mapData && WORLD_WIDTH > 0 && WORLD_HEIGHT > 0)
	{
		for (j = 0; j < WORLD_HEIGHT; j++)
		{
			fprintf(pFile, "%03d ", j);	
			for (i = 0; i < WORLD_WIDTH; i++)
			{
				if (*pData < 0)
				{
					fprintf(pFile, "-");
				}
				else
				{
					fprintf(pFile, "%d", *pData);
				}
				pData++;
			}
			fprintf(pFile, "\n");
		}
	}

	fprintf(pFile, "Start %d,%d  Dest %d,%d\n", tileX, tileY, xPos, yPos);
	if (tempPathLength)
	{
		// show the path
		int8	*pArrayTemp = pathArray;
		i = 0;
		fprintf(pFile, "path\n");
		while(*pArrayTemp != -1 && i < MAX_PATH_SIZE)
		{
			fprintf(pFile, "%d", *pArrayTemp);
			pArrayTemp++;
			fprintf(pFile, ",%d\n", *pArrayTemp);
			pArrayTemp++;
			i++;
		}
	}
	else
	{
		fprintf(pFile, "We are as close as possible\n");
	}

	fprintf(pFile, "\n\n\n\n");
	fclose(pFile);
	// end debug stuff
*/

	return(tempPathLength);
}

void
Unit::StartMoveToMode(void)
{
	fInAction = TRUE;
	fActionDone = FALSE;

	// break rider/mount relationship if necessary
//	if(mountSerial)
//	{
//		pUnit = (Unit *)pWorld->ValidateTarget(mountSerial);
//		if(pUnit)	//safety
//			pUnit->SetRider(NULL);
//		mountSerial = NULL;
//	}
//	CheckForMount();	replaced by above

	// re-start appropriate splash ani for terrain, if any
	CheckTerrainAni();

	curPathPosition = 0;
	pathDestX = secondaryX;
	pathDestY = secondaryY;

	pathLength = FindPathTo((coord)pathDestX, (coord) pathDestY, pathArray, fStopAdjacent);
	if(pathLength > 0)
	{
		// if ACOLYTE was in OBELISK
		if(type == ACOLYTE)
		{
			if(currState == ST_OBELISK || currState == ST_OBELISKING)
			{
				EndGuardMode();
				// begun de-obelisk process.  Movement will be held off until then.
				return;
			}
			if(currAttack == 0)
			{
				// error!  skater mode!  Try and prevent it from occuring.
				SetTilePos(tileX, tileY);
			}
		}

//		// check for immediate next step blocked by friendlies
//		PeekNextTarget(testX, testY);
//		if(!CanIMoveTo(testX, testY, TRUE))	// if blocked
//		{
//			// repath without pathing through friendlies
//			pathLength = FindPathTo((coord)pathDestX, (coord) pathDestY, pathArray, fStopAdjacent, FALSE);
//		}
	}

	cycleNext = ATicks();	// cause next rethink immediately
//	UnitDebug.Out("Unit::StartMoveToMove() - targ %d/%d, %d steps to reach %d/%d from %d/%d\n", secondaryX, secondaryY,
//		pathLength, pathDestX, pathDestY, tileX, tileY);
}

void
Unit::StopMoveToMode(void)
{
	MapSpace	*pSpace;

	fActionDone = TRUE;
	fAniDone = TRUE;
	if(fTerrainAni)
	{
		StopTerrainAni();
	}

	if(fKamikaze)
	{
		// kaBOOM!
		int		x1, x2, y1, y2, loopX, loopY, damage = currHP;
		Unit		*pUnit;
		BAM_Ani	*pAni;

		// introducing.. the cobalt bomb!
		x1 = AMax(tileX - 2, 0);
		x2 = AMin(tileX + 2, WORLD_WIDTH - 1);
		y1 = AMax(tileY - 2, 0);
		y2 = AMin(tileY + 2, WORLD_HEIGHT - 1);

		for(loopY = y1; loopY <= y2; loopY++)
			for(loopX = x1; loopX <= x2; loopX++)
			{
				if((loopX == x1 || loopX == x2) && (loopY == y1 || loopY == y2))
				{
					// trim the corners off
					continue;
				}
				TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_FIRE,
					loopX, loopY);
				pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
				if(pUnit)
					pUnit->TakeAHit(NULL, UNIT_FIRE, loopX, loopY, damage, FALSE);
				pSpace = pWorld->map.GetStructure(loopX, loopY);
				if(pSpace && pSpace->h.currHP > 0)
					pSpace->TakeAHit(NULL, UNIT_FIRE, loopX, loopY, damage, loopX, loopY);
			}

		TRACK_MEM("FlameSpout");	pAni = pWorld->vPort.NewTerrainUnit(ACTIVE_FLAME_SPOUT,
			tileX, tileY);
		if(pAni)
		{
			pAni->SetTilePosOffset(tileXOff, tileYOff);
			pWorld->vPort.RepositionAni(pAni);
		}
		pWorld->LaunchSound(SND_FLAME_SPURT, tileX, tileY);
		Die();
	}
	// check if we've stopped on top of another unit
//	CheckForMount();

	// if tile center reached
	if(tileXOff == TILE_HORZ_CENTER && tileYOff == TILE_VERT_CENTER)
	{
		switch(currTerrain)
		{
			case MINE:
				if(movement == FLIER)
					break;		// fliers cant traverse mineshafts

				// mineshaft must have both mine tile && cluster with FUNC_MINESHAFT
				pSpace = pWorld->map.GetStructure(tileX, tileY);
				if(!pSpace || pSpace->h.func != FUNC_MINESHAFT)
					break;
				EnterMineShaft();
				break;
		}
	}
}

//void
//Unit::CheckForMount(void)
//{
//	Unit	*pUnit;
//	grip	gNewMount;
//	List	*pList;
//
//	if(!mountSerial && movement == WALKER)
//	{
//		gNewMount = pWorld->unitLib.GetUnitGrip(tileX, tileY);
//
//		if(gNewMount == gSelf)
//		{
//			// double-check
//			if(currTerrain == WATER)
//			{
//				pList = &pWorld->unitLib.lUnits[SIDE3];
//				gNewMount = (grip)pList->FirstValue();
//				while(gNewMount)
//				{
//					if(gNewMount != gSelf)
//					{
//						pUnit = ADerefAs(Unit, gNewMount);
//						if(pUnit->type == TORTOISE && pUnit->tileX == tileX && pUnit->tileY == tileY)
//							break;
//					}
//					gNewMount = (grip)pList->NextValue();
//				}
//			}
//		}
//
//		if(gNewMount && gNewMount != gSelf)
//		{
//			pUnit = ADerefAs(Unit, gNewMount);
//			if(pUnit->type == TORTOISE)
//			{
//				mountSerial = pUnit->serialNum;
//				UnitDebug.Out("* Unit g%d(\"%s\") - new mount g%d\n", gSelf, unitName, mountSerial);
//				pUnit = (Unit *)(pWorld->ValidateTarget(mountSerial));
//				pUnit->SetRider(this);
//			}
//		}
//	}
//	else if(mountSerial)
//	{
//		// check if we've stepped away from our mount's X,Y
//
//		// have to check from pUnit-> because a mount's grip is not placed in the grip map
//		pUnit = (Unit *)(pWorld->ValidateTarget(mountSerial));
//		if(!pUnit || tileX != pUnit->tileX || tileY != pUnit->tileY)
//		{
//			// break the rider/mount relationship
//			if(pUnit)
//				pUnit->SetRider(NULL);
//			mountSerial = NULL;
//		}
//	}
//}

void
Unit::StopTerrainAni(void)
{
	if(gTerrainAni)
	{
		pWorld->vPort.CutAni(gTerrainAni);
		ADelete(gTerrainAni);
		gTerrainAni = NULL;
	}

	fTerrainAni = FALSE;
}

void
Unit::CheckTerrainAni(void)
{
	uint32	aniNum;
	BAM_Ani	*pAni;

	if(movement != WALKER || mountSerial)
	{
		// only non-riding walkers splash through terrain
		aniNum = 0;
	}
	else switch(currTerrain)
	{
		case HOTBED:
			aniNum = ANIM_SPLASH_HOTBED;
			break;

		case FOLIAGE:
			aniNum = ANIM_SPLASH_FOLIAGE;
			break;
	
//		case MARSH:
//			aniNum = ANIM_SPLASH_MARSH;
//			break;
		
//		case SWAMP:
//			aniNum = ANIM_SPLASH_SWAMP;
//			break;

		case SWAMP:
		case MARSH:
		case WHIRLPOOL:
		case WATER:
			aniNum = ANIM_SPLASH_WATER;
			break;
			
		default:
			aniNum = 0;
			break;
	}

	// if there's an ani to show
	if(aniNum && currAction == MOVE_TO)
	{
		// allocate new one as necessary
		if(!gTerrainAni)
		{
			TRACK_MEM("TerrainAni");	gTerrainAni = pWorld->vPort.NewAni(RES_ANIM,
				NULL, NULL, PRI_WALKER + 2, FALSE, FALSE, 0, 0, 0, TRUE);
			pAni = ADerefAs(BAM_Ani, gTerrainAni);
			pAni->selfType = TERRAIN_ANI;			
			pAni->fAniHideWhenDone = TRUE;
		}
		else
		{
			// re-use existing ani
			pAni = ADerefAs(BAM_Ani, gTerrainAni);
		}

		pAni->SetTilePos(tileX, tileY, tileXOff, tileYOff);
		pAni->RunAnim(aniNum, TRUE, TRUE, 4);
		fTerrainAni = TRUE;
		pWorld->vPort.CheckAni(gTerrainAni);
	}
	else
	{
		// no terrain ani to run
		
		if(fTerrainAni)
		{
			// just stop it
			StopTerrainAni();
		}
	}
}

void
Unit::CycleMoveTo(void)
{
	Unit		*pUnit;
	MapSpace	*pSpace;
	int		tempX, tempY;
	action	action1;
	ticks_t	currTicks = ATicks();

	#ifndef NDEBUG
	UnitDebug.Out("CM1:%d ", fInAction? 1: 0);
	#endif

	if(!fInAction)	// if we haven't started moving yet
	{
		StartMoveToMode();		// run pathing, etc
		if(fShowCurrentUnitPath && fCursorTracking)
			ShowUnitPath(this);
		return;
	}

	// have we reached our dest yet?
	if(!fInMotion && tileX == secondaryX && tileY == secondaryY &&
		tileXOff == secondaryXOff && tileYOff == secondaryYOff)
	{
		#ifndef NDEBUG
		UnitDebug.Out("CM2 done\n");
		#endif
		StopMoveToMode();
		return;
	}

	if(currTicks < cycleNext)
	{
		#ifndef NDEBUG
		UnitDebug.Out("CM3 done\n");
		#endif
		return;		// not time for unit's next step yet
	}
			
	if(targetTileX == -1)	// if next (or first) step not chosen yet, choose it
	{
		if(currState == ST_OBELISKING)
		{
			// WHOOPS!  This flag should have been cleared by now.
			currState = ST_NULL;
		}

		switch(type)
		{
			case BANSHEE:
				if(currTicks >= tAutoActionTimer)
				{
					StopMoveToMode();
//					Die(NULL, UNIT_TERRAIN);	- why Die() instead of wailing?
					return;
				}
				break;

			case BRIGAND:
				// if no item @ destX,Y but item within range
				if(!ARandom(5) && !pWorld->itemMgr.CheckItem(secondaryX, secondaryY) && SearchForItems())
				{
					// stop movement.  BRIGAND will do an item search from guard mode
					StopMoveToMode();
					return;
				}
				break;
		}
	
		// if chasing a moving target, rather than end MOVE_TO mode just for repath, do it here.
		bool	fTargetMoved = FALSE;

		// if primary target has moved
		switch(primaryTargetType)
		{
			case UNIT:
				if(primaryAction == AVOID)
				{
					// don't worry about it, we're not trying to catch up with
					// him anyway
				}
				else if(ARandom(2))	// to reduce CPU stress
				{
					// update primary X,Y to reflect target's new pos
					pUnit = (Unit *)ValidatePrimaryTarget();
					if(pUnit && (pUnit->tileX != primaryX || pUnit->tileY != primaryY))
					{
						fTargetMoved = TRUE;
				
						// update primary X/Y
						primaryX = pUnit->tileX;
						primaryY = pUnit->tileY;			
					}
				}
				break;

			case BUILDING:
				if(primaryAction == GUARD && searchForEnemiesPresetSerial)
				{
					// auxilary target - MOVE to target's X,Y and cause SearchForEnemies() to find
					// target once there.  Used for special targetting cases (defending buildings from attackers
					// outside of defender's normal range, etc)
					pUnit = (Unit *)(pWorld->ValidateTarget(searchForEnemiesPresetSerial));
					if(pUnit && pUnit->currAction == MOVE_TO)
					{
						secondaryX = pUnit->tileX;
						secondaryY = pUnit->tileY;
						curPathPosition = MAX_PATH_SIZE;	// trigger repath

						// this is handled this way because the unit we are pursuing is NOT the primary
						// target, which would require 3 layers of unit logic otherwise.
					}
				}
				break;
		}
				
		if(WithinRangeForPrimary())
		{
			// end move_to mode
			StopMoveToMode();
			return;		
		}

		if(fTargetMoved)
		{
			if(defendTargetAction && primaryTargetType == UNIT)
			{
				pUnit = (Unit *)ValidatePrimaryTarget();

				// are we defending a building?
				pSpace = (MapSpace *)pWorld->DerefSerial(defendTargetSerial);
				if(pSpace && pSpace->h.currHP && pSpace->h.owner == player)
				{
					// we are pursuing a target unit for the purpose of defending a building.
					// Target unit has moved.  Verify if building is still in danger.

					// make sure enemy unit is out of range of both building, and 1-tile border surrounding it (so
					// that we have a safe spot to which we can return)
					if(pUnit->GetDistanceTo(pSpace, BUILDING) > pUnit->currRange + 1)
					{
						// target unit cannot attack our building from there.
						// Stop chasing enemy - restore ulterior action as primary action

						tempX = defendTargetX;
						tempY = defendTargetY;
						action1 = defendTargetAction;

						defendTargetSerial = NULL;	defendTargetType = NOTARGET;	defendTargetAction = NO_ACTION;
						SetPrimaryAction(action1, tempX, tempY);
						return;
					}
				}
				else
				{
					// is target unit within threat range of our old X,Y?
					if(pUnit->GetDistanceTo(defendTargetX, defendTargetY) > pUnit->currRange + 1)
					{
						// target unit cannot attack our building from there.
						// Stop chasing enemy - restore ulterior action as primary action

						tempX = defendTargetX;
						tempY = defendTargetY;
						action1 = defendTargetAction;

						defendTargetSerial = NULL;	defendTargetType = NOTARGET;	defendTargetAction = NO_ACTION;
						SetPrimaryAction(action1, tempX, tempY);
						return;
					}
				}
			}	

			// did target move itself within range?

			// if we're close to end of path then repath here for accuracy
			if((pathLength - curPathPosition) < 5 && pathLength != MAX_PATH_SIZE)
			{
				// repath
				curPathPosition = MAX_PATH_SIZE;
				
				// update dest X,Y because target has moved
				secondaryX = primaryX;
				secondaryY = primaryY;
			}
			// else ignore unit's X,Y change and continue same path - at this distance
			// from the target, it doesnt matter and isnt worth the overhead of repathing
		}

		// periodic repath, in case obstacles have moved
		if(curPathPosition > 3 && !ARandom(15))
		{
	 		// repath
			curPathPosition = MAX_PATH_SIZE;
				
			// update dest X,Y in case target X,Y (primary target) has changed
			secondaryX = primaryX;
			secondaryY = primaryY;

			if(pBam->playerTypes[player] == PLAYER_COMPUTER)
			{
				// we're an AI unit - extra occasional thinking here

				// Are we chasing another unit?
				if(secondaryTargetType == UNIT)
				{
					// okay, have we been chasing him for too long?
					if(tCurrActStarted - currTicks > TICKS_PER_SEC * 60 && pWorld->ai.CheckWits() && pWorld->ai.CheckWits())
					{
						// nevermind, we're wasting our time.
						SetPrimaryAction(MOVE_TO, tileX, tileY);
						ai.action = AI_NONE;
						ai.data = NULL;
						pWorld->ai.ReportUnit(this, "MOVE:CHASE_TOO_LONG", TRUE);
						return;
					}
				}
			}
		}

		ChooseNextTarget();

		if(curPathPosition == 1)
		{
			// taking first step
			if(!fTerrainAni)
			{
				CheckTerrainAni();
			}
		}

		if(curPathPosition == MAX_PATH_SIZE)
		{
			// end of path
			StopMoveToMode();
			return;
		}
		else if(targetTileX == tileX && targetTileY == tileY)
		{
			// first step == currX/Y?  Unit must want re-centering first.
			currDir = GetDir(TILE_HORZ_CENTER - tileXOff, TILE_VERT_CENTER - tileYOff);

			RunAnimation(ST_ANIM_WALK, 1, TRUE, currDir);
	 		CycleMotion();		// skip first cycle of movement - starting x,y
			MoveAnim(targetTileX, targetTileY, TILE_HORZ_CENTER, TILE_VERT_CENTER,
				move2steps[currMove], move2steps[currMove], move2ticks[currMove]);
			fReturnToCenter = TRUE;
		}
		else	// else moving to new tile
		{
			lastDir = currDir;
		
			// what direction will we be walking in to reach this target?
			currDir = GetDir(targetTileX - tileX, targetTileY - tileY);
			RunAnimation(ST_ANIM_WALK, 1, TRUE, currDir);

			// walk only to edge of current tile bordering new tile, then verify that target
			// space is still empty before actually entering
			switch(currDir)
			{
				case DOWN:
				case UP:
					stepToXOff = TILE_HORZ_CENTER;
					break;
				case RIGHT:
				case UP_RIGHT:
				case DOWN_RIGHT:
					stepToXOff = TILE_HORZ_RIGHT;
					break;
				case LEFT:
				case UP_LEFT:
				case DOWN_LEFT:
					stepToXOff = TILE_HORZ_LEFT;
					break;
			}
			switch(currDir)
			{
				case UP:
				case UP_LEFT:
				case UP_RIGHT:
					stepToYOff = TILE_VERT_TOP;
					break;
				case LEFT:
				case RIGHT:
					stepToYOff = TILE_VERT_CENTER;
					break;
				case DOWN:
				case DOWN_LEFT:
				case DOWN_RIGHT:
					stepToYOff = TILE_VERT_BOTTOM;
					break;
			}
			MoveAnim(tileX, tileY, stepToXOff, stepToYOff, move2steps[currMove],
				move2steps[currMove], move2ticks[currMove]);
	 		CycleMotion();		// skip first cycle of movement - starting x,y
			fReturnToCenter = FALSE;
		}
	}

	// if motion has stopped
	if(!fInMotion)
	{
		#ifndef NDEBUG
		UnitDebug.Out("CM4 ");
		#endif
		if(fReturnToCenter)
		{
			// returned from edge (target tile was blocked)
			// re-initiate pathing
			targetTileX = -1;	// need next target
			curPathPosition = MAX_PATH_SIZE;
		}
		else if(tileX == targetTileX && tileY == targetTileY)
		{
			// arrived at center of target tile.  Need next target.

			targetTileX = -1;	// need next target
			CycleMoveTo();

			// help anim from hiccuping
			tLastCycle = 0;
			cycleNext = 0;
			BAM_Ani::SmartCycle(FALSE);
		}
		else
		{
			// arrived at edge - check new tile for blockage before
			// proceeding in
			if(CanIMoveTo(targetTileX, targetTileY, TRUE))
			{
//				// if preparing to mount a TORTOISE
//				if(primaryAction == FOLLOW && primaryTargetType == UNIT && targetTileX == primaryX &&
//					targetTileY == primaryY)
//				{
//					pUnit = (Unit *)ValidatePrimaryTarget();
//					if(pUnit)	// safety
//					{
//						// inform turtle to grab us if it is, or starts, moving away
//						pUnit->mountingSerial = serialNum;
//					}
//				}

				if(pWorld->map.GetTile(targetTileX, targetTileY)->terrain == WIND_WALL)
				{
					// stop travel

					StopMoveToMode();
					fInMotion = FALSE;
					SetAction(BLOWN_BACK);
					return;
				}

				// if unit is moving diagonally
				switch(currDir)
				{
					case UP_LEFT:
					case UP_RIGHT:
					case DOWN_LEFT:
				 	case DOWN_RIGHT:
						// place unit inside of new tile by hand - cant always trust
						// Bresnham to not clip an adjacent tile otherwise
						SetTilePos(targetTileX, targetTileY, (tileXOff == TILE_HORZ_RIGHT)? TILE_HORZ_LEFT: TILE_HORZ_RIGHT,
							(tileYOff == TILE_VERT_TOP)? TILE_VERT_BOTTOM: TILE_VERT_TOP);
						break;
				}

				// proceed towards center of new tile
				MoveAnim(targetTileX, targetTileY, TILE_HORZ_CENTER, TILE_VERT_CENTER,
					move2steps[currMove], move2steps[currMove], move2ticks[currMove]);
		 		CycleMotion();		// skip first cycle of movement - starting x,y
			}
			else if(currTerrain == PRISON_PIT && fEnteringMineShaft)
			{
				// unit trying to exit a prison pit, but failed.  Down the tubes he goes!
				StopMoveToMode();
				return;
			}
			else
			{
				// return to center of current tile
				currDir = (direction)((currDir + 4) % 8);		// turn around 180
				RunAnimation(ST_ANIM_WALK, 1, TRUE, currDir);
				MoveAnim(tileX, tileY, TILE_HORZ_CENTER, TILE_VERT_CENTER, 
					move2steps[currMove], move2steps[currMove], move2ticks[currMove]);
				fReturnToCenter = TRUE;
			}
		}
	}

	if(fAniDone)
	{
		// movement ani stopped?  Maybe we took a hit or something.  Restart it.
		RunAnimation(ST_ANIM_WALK, 1, TRUE, currDir);
	}
	#ifndef NDEBUG
	UnitDebug.Out("CM5 done\n");
	#endif
}

/*
void
Unit::SetRider(Unit *pNewRider)
{
	pNewRider = (Unit *)pWorld->ValidateTarget(pNewRider->serialNum);		// debug test

	if(pNewRider)
		riderSerial = pNewRider->serialNum;
	else riderSerial = NULL;

	UnitDebug.Out("Unit %s(g%d) has new rider (g%d)\n", unitName, gSelf, riderSerial);

	if(riderSerial)
	{
//		if(currAction == GUARD)
//			EndGuardMode();
	}
	else
	{
		// restore our own grip to the grip map, instead of our rider's
		SetTilePos(tileX, tileY, tileXOff, tileYOff);
	}
}

void
Unit::UpdateRider(void)
{
//	Unit	*pRider;
//
//	pRider = (Unit *)(pWorld->ValidateTarget(riderSerial));
//	if(pRider)
//	{
//		pRider->MountMoved();
//	}
//	else
//	{
//		UnitDebug.Out("Unit(g%d)::UpdateRider() - rider serial %d invalid, disconnecting\n", gSelf, riderSerial);
//		riderSerial = NULL;
//	}
}

void
Unit::MountMoved(void)
{
//	Unit	*pUnit;
//	int32	newX, newY, newXOff, newYOff;
//
//	pUnit = (Unit *)(pWorld->ValidateTarget(mountSerial);
//	newX = pUnit->tileX;
//	newY = pUnit->tileY;
//	newXOff = pUnit->tileXOff;
//	newYOff = pUnit->tileYOff;
//
////	UnitDebug.Out("Unit g%d(\"%s\") mount s%d moved - ", gSelf, unitName, mountSerial);
//
//	if(newX != tileX || newY != tileY)
//	{
////		UnitDebug.Out(" newX%d newY%d newXOff%d newYOff%d\n", newX, newY, newXOff, newYOff);
//		SetTilePos(newX, newY, newXOff, newYOff);
//	}
//	else
//	{
////		UnitDebug.Out(" newXOff%d newYOff%d\n", newXOff, newYOff);
//		SetTilePosOffset(newXOff, newYOff);
//	}
//	pWorld->vPort.RepositionAni(this);
}
*/

void
Unit::SetPos(coord L, coord T)
{
	if(type == JUGGERNAUT)
	{
		// push JUGGERNAUT ani up & left by 1/2 tile, because it is a 2x2 tile unit
		L -= TILE_WIDTH / 2;
//		T -= TILE_HEIGHT / 2;
	}
	BAM_Ani::SetPos(L, T);
}

void
Unit::SetCurrTerrain(terrainType newTerrain)
{
	UnitGroup	*pGroup;
	bool			fIgnoreTerrain = FALSE;

	if(currAction == TRANSFORM)
	{
		if(!fActionDone)
		{
			// METAPOD stats same as OBELISK
			pGroup = pWorld->unitLib.GetUnitGroup(OBELISK);
		}
		else
		{
			// restore original stats
			pGroup = pWorld->unitLib.GetUnitGroup(type);
		}

		// base stats, necessary here because ACOLYTE's shift between 2 base modes.
		baseAttack = pGroup->baseAttack;
		baseRange = pGroup->baseAttackRange;
		baseDefense = pGroup->baseDefense;
		baseMove = pGroup->baseMoveRate;
	}
	else if(movement != WALKER && (newTerrain != PORTAL && newTerrain != GUARDIAN_PORTAL))
	{
		fIgnoreTerrain = TRUE;
		pGroup = pWorld->unitLib.GetUnitGroup(type);
	}
	else if(type == ACOLYTE)
	{
		if(currState == ST_OBELISK)
		{
			pGroup = pWorld->unitLib.GetUnitGroup(OBELISK);
		}
		else pGroup = pWorld->unitLib.GetUnitGroup(type);

		// base stats, necessary here because ACOLYTE's shift between 2 base modes.
		baseAttack = pGroup->baseAttack;
		baseRange = pGroup->baseAttackRange;
		baseDefense = pGroup->baseDefense;
		baseMove = pGroup->baseMoveRate;
	}
	else pGroup = pWorld->unitLib.GetUnitGroup(type);

	currTerrain = newTerrain;
	if(fIgnoreTerrain)
	{
		// go unaffected by terrain for some reason
		currAttack = pGroup->baseAttack;
		currDefense = pGroup->baseDefense;
		currRange = pGroup->baseAttackRange;

		// another Vas exception
		if(type == WYRM && currTerrain == WIND_WALL && player == SIDE0)
			currMove = 0;

		currMove = pGroup->baseMoveRate;
	}
	else
	{
		CheckTerrainAni();

		currAttack = pGroup->attack[newTerrain];
		currDefense = pGroup->defense[newTerrain];
		currRange = pGroup->attackRange[newTerrain];
		currMove = pGroup->moveRates[newTerrain];
		if(currMove < 1)
		{
			// standing somewhere we shouldnt have been able to reach.
			// maybe we moved off a tortoise
			currMove = pGroup->baseMoveRate;
		}

		if(newTerrain == PORTAL)
		{
			// assume we're standing on the portal - check ownership
			MapSpace	*pPortal = pWorld->map.GetStructure(tileX, tileY);
			if(pPortal && pPortal->h.owner != player)
			{
				// restore defaults
				currDefense = pGroup->baseDefense;
				currAttack = pGroup->baseAttack;
				currRange = pGroup->baseAttackRange;
				currMove = pGroup->baseMoveRate;
			}
		}

	}

	// some items modify currStats - these changes need to be reset now.
	// Presumably these are all re-usable items, since a one-shot automatic
	// item would have been used up at pickup time.
	if(item)
		if(pWorld->itemMgr.GetItemUsage(item) == ITEM_AUTOMATIC)
			UseItem();

	if(primaryAction == BERSERK)
	{
		currAttack += AMax((currAttack / 2), 1);		// attack better
		currMove++;												// move faster
		currRange = (currRange > 1)? 1: currRange;	// but restricted viewing range
	}
}

void
Unit::SetTilePosOffset(uint32 newXOff, uint32 newYOff)
{
	BAM_Ani	*pAni;

	BAM_Ani::SetTilePosOffset(newXOff, newYOff);

	// if tile center reached
	if(tileXOff == TILE_HORZ_CENTER && tileYOff == TILE_VERT_CENTER)
	{
		// check for item to pick up at new location
		BAMItem	*pItem = pWorld->itemMgr.CheckItem(tileX, tileY);
		if(pItem)
			PickupItem();
	}

	// move terrain ani along with us
	if(fTerrainAni)
	{
		BAM_Ani	*pAni = ADerefAs(BAM_Ani, gTerrainAni);
		pAni->SetTilePosOffset(tileXOff, tileYOff);
		if(!CheckState(S_HIDDEN))
			pWorld->vPort.RepositionAni(pAni);	// make it show
	}

	// move cursor ani along with us
	if(gGroupCursor)
	{
		BAM_Ani	*pAni = ADerefAs(BAM_Ani, gGroupCursor);
		pAni->SetTilePosOffset(tileXOff, tileYOff);
		if(!CheckState(S_HIDDEN))
			pWorld->vPort.RepositionAni(pAni);	// change on-screen pos
	}

	if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
	{
		pAni = ADerefAs(BAM_Ani, gAuxAni);
		pAni->SetTilePosOffset(tileXOff, tileYOff);
		if(!CheckState(S_HIDDEN))
			pWorld->vPort.RepositionAni(pAni);
	}
	else gAuxAni = NULL;

	// is another unit riding on us?
//	if(riderSerial)
//		UpdateRider();
//	else if(mountingSerial)	// if someone trying to mount us
//	{
//		// if not at tile center now
//		if(tileXOff != TILE_HORZ_CENTER || tileYOff != TILE_VERT_CENTER)
//		{
//			// grab unit and place them on us (assisted mount, prevents drowning)
//			pUnit = (Unit *)pWorld->ValidateTarget(mountingSerial);
//			if(pUnit)
//			{
//				pUnit->StopMoveToMode();
//				pUnit->fInMotion = FALSE;
//				pUnit->mountSerial = serialNum;
//				SetRider(pUnit);
//				UpdateRider();
//			}
//		}
//		mountingSerial = NULL;	// cancel mounting assist process either way
//	}

	if(fCursorTracking)
	{
		pWorld->vPort.UpdateTracking();
	}
}

// overloaded from BAM_Ani so that we can update the unitMap[] accordingly
void
Unit::SetTilePos(uint32 newX, uint32 newY, uint32 newXOff, uint32 newYOff)
{
	WorldMap *pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	BAM_Ani	*pAni;
	grip		gAni, gOldUnit;
	Unit		*pUnit;
	MapSpace *pSpace;

	gOldUnit = NULL;
	if(newX >= WORLD_WIDTH || newY >= WORLD_HEIGHT)
	{
		UnitDebug.Out("\nUnit::SetTilePos(%d, %d, %d, %d) - error, invalid coords.  Relocating to world\ncenter.",
			newX, newY, newXOff, newYOff);
		newX = WORLD_WIDTH / 2 - 1;
		newY = WORLD_HEIGHT / 2 - 1;
	}

	// if already somewhere else
	if(tileX > -1)
	{
		// remove our grip from old location in grip map before moving into new loc
		if(type == JUGGERNAUT)
		{
			pWorld->unitLib.PlaceUnitGrip(NULL, tileX, tileY);
			pWorld->unitLib.PlaceUnitGrip(NULL, tileX - 1, tileY);
			pWorld->unitLib.PlaceUnitGrip(NULL, tileX, tileY - 1);
			pWorld->unitLib.PlaceUnitGrip(NULL, tileX - 1, tileY - 1);
			pWorldMap->Draw(tileX, tileY);
			pWorldMap->Draw(tileX - 1, tileY);
			pWorldMap->Draw(tileX, tileY - 1);
			pWorldMap->Draw(tileX - 1, tileY - 1);
		}
		else
		{
			if(pWorld->unitLib.GetUnitGrip(tileX, tileY) == gSelf)
			{
				// not there no' mo'
				pWorld->unitLib.PlaceUnitGrip(NULL, tileX, tileY);
			}
			else
			{
				UnitDebug.Out("\nUnit::SetTilePos(g%d, x%d y%d) - gSelf already gone!",
					gSelf, tileX, tileY);
			}
			pWorldMap->Draw(tileX, tileY);
		}
	}
	if(currTerrain == PRESSURE_PLATE)
	{
		pSpace = pWorld->map.GetStructure(tileX, tileY);
		if(pSpace)
			pSpace->RunTrigger();
		pWorld->vPort.SwapMapTile(tileX, tileY);
	}

	BAM_Ani::SetTilePos(newX, newY, newXOff, newYOff);
	if(gAuxAni && pMemMgr->CheckGrip(gAuxAni))
	{
		pAni = ADerefAs(BAM_Ani, gAuxAni);
		pAni->SetTilePos(newX, newY, newXOff, newYOff);
		if(!CheckState(S_HIDDEN))
			pWorld->vPort.RepositionAni(pAni);
	}
	else gAuxAni = NULL;

	// tileX and tileY will now contain new values

//	if(type != JUGGERNAUT)
//		CheckForMount();

	if(!riderSerial)	// if no one is riding us
	{
		if(type == JUGGERNAUT)
		{
			pUnit = pWorld->unitLib.GetUnit(tileX, tileY);
			if(pUnit && pUnit != this)
				pUnit->Crush(this);
			pUnit = pWorld->unitLib.GetUnit(tileX - 1, tileY);
			if(pUnit && pUnit != this)
				pUnit->Crush(this);
			pUnit = pWorld->unitLib.GetUnit(tileX, tileY - 1);
			if(pUnit && pUnit != this)
				pUnit->Crush(this);
			pUnit = pWorld->unitLib.GetUnit(tileX - 1, tileY - 1);
			if(pUnit && pUnit != this)
				pUnit->Crush(this);
			pWorld->unitLib.PlaceUnitGrip(gSelf, tileX, tileY);
			pWorld->unitLib.PlaceUnitGrip(gSelf, tileX - 1, tileY);
			pWorld->unitLib.PlaceUnitGrip(gSelf, tileX, tileY - 1);
			pWorld->unitLib.PlaceUnitGrip(gSelf, tileX - 1, tileY - 1);
			pWorldMap->Draw(tileX, tileY, TRUE, gGroupCursor? TRUE: FALSE);
			pWorldMap->Draw(tileX - 1, tileY, TRUE, gGroupCursor? TRUE: FALSE);
			pWorldMap->Draw(tileX, tileY - 1, TRUE, gGroupCursor? TRUE: FALSE);
			pWorldMap->Draw(tileX - 1, tileY - 1, TRUE, gGroupCursor? TRUE: FALSE);
		}
		else
		{
			gOldUnit = pWorld->unitLib.GetUnitGrip(tileX, tileY);
			if(gOldUnit)
			{
				pUnit = ADerefAs(Unit, gOldUnit);
				UnitDebug.Out("\nUnit(s%d, %s):: tried walking on unit @%d:%d!",
					serialNum, unitName, tileX, tileY);
			}
			else pWorld->unitLib.PlaceUnitGrip(gSelf, tileX, tileY);
			pWorldMap->Draw(tileX, tileY, TRUE, gGroupCursor? TRUE: FALSE);
		}
	}


	// is another unit riding us?
//	if(riderSerial)
//	{
//		UpdateRider();
//	}

	// get modifiers for new terrain, and pre-calc current unit settings
	terrainType newTerrainType;
	newTerrainType = (terrainType)pWorld->map.GetTerrainNum(tileX, tileY);

	// if terrain type has changed
	if(newTerrainType != currTerrain)
	{
		// set current stats according to terrain type
		SetCurrTerrain(newTerrainType);

		// dont need to worry about incidental targeting (lastUnitSerialDrawn) here
		// because ViewPort::Cycle() will draw all unit stats later
		if(fCursorTracking)
			pWorld->UpdateAttDefMoveOnly(this);
	}

	CheckForTransformer();
									  
	int32 viewingRange;
	if(movement == FLIER)
		viewingRange = AMax(currRange, 3);
	else viewingRange = AMax(currRange, 2);

	if(tEnemyPortalTimer)
	{
		// if we're moving to / still on an enemy portal, the timer will be re-activated
		tEnemyPortalTimer = NULL;
	}

	switch(currTerrain)
	{
		case PRESSURE_PLATE:			
			if(movement != WALKER)
				break;

			pWorld->vPort.SwapMapTile(tileX, tileY);
			pSpace = pWorld->map.GetStructure(tileX, tileY);
			if(pSpace)
				pSpace->RunTrigger();
			break;

		case TRIP_SWITCH:
			if(movement != WALKER)
				break;

			TRACK_MEM("TripSwitchAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_SW_TRIP, 1,
				PRI_FLOOR, TRUE, FALSE,	4, tileX, tileY, FALSE);	// poof!
			pWorld->vPort.CheckAni(gAni);
			pSpace = pWorld->map.GetStructure(tileX, tileY);
			if(pSpace)
				pSpace->RunTrigger();
			break;

		case HOTBED:
			if(movement != WALKER)
				break;

			TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_FIRE, tileX, tileY);
			pWorld->LaunchSound(SND_LAVA, tileX, tileY);
			break;

		case BRAMBLES:
			if(movement == WALKER)
				TakeAHit(NULL, UNIT_TERRAIN, tileX, tileY, 2);
//			viewingRange--;
			break;

		case FOLIAGE:
		case BROKEN_LAND:
		case RUINS:
		case SWAMP:
//			viewingRange--;	/ No sight-range limiting anymore
			break;

		case PRISON_PIT:
			if(fEnteringMineShaft == TRUE)
				break;
			// fall-through

		case CHASM:
			if(movement != FLIER)
			{
				Die(NULL, UNIT_TERRAIN);
			}
			break;

		case CAULDRON:
			if(movement == WALKER)
			{
//				TRACK_MEM("DrownAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_CAULDRON_SPLASH, 1,
//					priorityLevel + 2, TRUE, FALSE, 4, tileX, tileY, FALSE, tileXOff, tileYOff);
//				pWorld->vPort.CheckAni(gAni);
				Die(NULL, UNIT_TERRAIN);
			}
			break;

		case WATER:
			if(movement != WALKER)
				break;
			if(mountSerial || item == DRIFT_DISC)
				break;
//			if(gOldUnit)
//			{
//				pUnit = ADerefAs(Unit, gOldUnit);
//				if(pUnit->type == TORTOISE)
//					break;
//			}
//			TRACK_MEM("DrownAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_BIG_SPLASH, 1,
//				priorityLevel + 2, TRUE, FALSE, 4, tileX, tileY, FALSE, tileXOff, tileYOff);
//			pWorld->vPort.CheckAni(gAni);
			Die(NULL, UNIT_TERRAIN);
			break;

		case GUARDIAN_PORTAL:
			// everyone gets hurt by this portal
			tEnemyPortalTimer = ATicks();
			break;

		case PORTAL:
			// check for enemy portal
			pSpace = pWorld->map.GetStructure(tileX, tileY);

			// if enemy-controlled
			if(pSpace && pSpace->h.owner != player)
				tEnemyPortalTimer = ATicks();
			break;
	}

	if(player == pWorld->playerSide)
	{
		pWorld->vPort.Defogger(tileX - viewingRange, tileY - viewingRange,
			tileX + viewingRange, tileY + viewingRange);
	}

	// set position of terrain ani (if any) to match us
	if(fTerrainAni)
	{
		pAni = ADerefAs(BAM_Ani, gTerrainAni);
		pAni->SetTilePos(tileX, tileY, tileXOff, tileYOff);
	}

	if(gGroupCursor)
	{
		pAni = ADerefAs(BAM_Ani, gGroupCursor);
		pAni->SetTilePos(tileX, tileY, tileXOff, tileYOff);
	}
}

void
Unit::EnterMineShaft(void)
{
	uint32	destX, destY, searchSize;
	int32		loopX, loopY, searchX1, searchY1, searchX2, searchY2;
	bool		fDestFound;	
	
	MapSpace *pSpace = pWorld->map.GetStructure(tileX, tileY);
	if(!pSpace || pSpace->h.func != FUNC_MINESHAFT)
	{
		fEnteringMineShaft = FALSE;	// failsafe
		return;			// this isnt a mineshaft - do nothing
	}	

	if(fEnteringMineShaft)	// if we came through the mine
	{
		fEnteringMineShaft = FALSE;
		return;					// dont re-enter, just sit here
	}
	fEnteringMineShaft = TRUE;	// so other end doesnt re-enter us (prevents ping pong)

	// go into the mine - spooky!
	// pWorld->LaunchSound(cavern moan)

	// find destination spot
	destX = pSpace->h.destX;
	destY = pSpace->h.destY;

	// scan for closest open && traversable spot
	fDestFound = FALSE;
	for(searchSize = 1; searchSize < 2 && !fDestFound; searchSize++)
	{
		searchX1 = AMax(destX - searchSize, 0);	searchX2 = AMin(destX + searchSize, WORLD_WIDTH - 1);
		searchY1 = AMax(destY - searchSize, 0);	searchY2 = AMin(destY + searchSize, WORLD_HEIGHT - 1);
		for(loopY = searchY1; loopY <= searchY2 && !fDestFound; loopY++)
			for(loopX = searchX1; loopX <= searchX2 && !fDestFound; loopX++)
			{
				if(loopX == pSpace->h.destX && loopY == pSpace->h.destY)
					continue;		// skip actual mine entrance spot

				if(CanIMoveTo(loopX, loopY, TRUE))
				{
					fDestFound = TRUE;
					destX = loopX;
					destY = loopY;
				}
			}
	}

	if(!CanIMoveTo(pSpace->h.destX, pSpace->h.destY, TRUE))	// if mineshaft opening blocked
	{
		if(fDestFound)
		{
			SetTilePos(destX, destY);	// jump straight to open spot
			SetPrimaryAction(GUARD, destX, destY);
		}
		else 
		{
//			if(pWorld->playerSide == player)	// if unit belonged to player
//			{
//				char	string1[80];
//				sprintf(string1, "%s LOST IN THE MINES", unitName);
//				pWorld->MsgBoxAdd(string1, tileX, tileY);
//				pWorld->MsgBoxDraw();
//			}
			DropItem();
			Hide();
			fDeleteMe = TRUE;
			return;
		}
	}
	else		// mineshaft opening not blocked - start there
	{
		SetTilePos(pSpace->h.destX, pSpace->h.destY);
		if(fDestFound)	// but can we move aside?
		{
			SetPrimaryAction(MOVE_TO, destX, destY);		// move there
		}
		else SetPrimaryAction(GUARD, tileX, tileY);		// we're trapped - sit tight
	}
	fEnteringMineShaft = FALSE;
}

void
Unit::CheckForTransformer(void)
{
	bool 	fNewState;

	// search for nearby transformer buildings, for verb button setting
	funcType	newTransformer = FindNearbyTransformer();
	if(newTransformer != nearbyTransformer)	// if a new transformation type is now available
	{
		nearbyTransformer = newTransformer;
		if(nearbyTransformer == FUNC_MISC)
		{
			SetActionAvailable(TRANSFORM, FALSE);
		}
		else
		{
			SetActionAvailable(TRANSFORM, TRUE);
		}
		if(fCursorTracking)
		{
			pWorld->SetVerbButtons(UNIT, serialNum);
			if(type == ACOLYTE && pWorld->statBoxCurrMode == UNIT)
				pWorld->SetTransformButtons(serialNum);
		}
	}

	if(type == ACOLYTE)
	{
		fNewState = CheckActionAvailable(TRANSFORM);
		if(fNewState != (currTerrain != PORTAL && currTerrain != BRIDGE))
		{
			// acolytes can transform anywhere (except on portals)
			SetActionAvailable(TRANSFORM, (bool)!fNewState);
			if(fCursorTracking)
				pWorld->SetVerbButtons(UNIT, serialNum);
		}
	}
}
void
Unit::Disband(void)
{
	uint32	manaBonus, serial1;
	Unit		*pUnit;

	#ifndef NDEBUG
	BamDebug.Out("\nDisband(%s)", unitName);
	#endif

	if(primaryAction == DIE || primaryAction == DISBAND)
		return;
	manaBonus = MANA_DISBAND;

	pWorld->AdjustManaLevel(player, manaBonus);

	if(fPackFollower)		// notify our pack leader we're leaving
	{
		pUnit = (Unit *)(pWorld->ValidateTarget(packLeaderSerial));
		if(pUnit)
			pUnit->RemoveFollower(this);		// we arent following him no mo'
		packLeaderSerial = NULL;
		fPackFollower = FALSE;
	}
	else if(fPackLeader)		// notify followers to find new leader
	{
		fPackLeader = FALSE;
		List	*pList = ADerefAs(List, glFollowers);
		serial1 = (uint32)pList->FirstValue();
		while(serial1)
		{
			pUnit = (Unit *)(pWorld->ValidateTarget(serial1));
			if(pUnit && pUnit->packLeaderSerial == serialNum)
				pUnit->FindALeader();
			serial1 = (uint32)pList->NextValue();
		}
		pList->Release();
	}
	SetPrimaryAction(DISBAND);
	DropItem();
}

void
Unit::Crush(Unit *pCrusher)
{
	grip	gAni;

	Die(pCrusher);

	TRACK_MEM("CrushedAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_CRUSHED, 1,
		PRI_FLOOR, TRUE, FALSE, 5, tileX, tileY, FALSE, tileXOff, tileYOff);
	pWorld->vPort.CheckAni(gAni);
	
	pWorld->unitLib.DeleteUnit(gSelf);
	// WARNING: (Unit *)this now invalid!
}

// pretty self explanatory
void
Unit::Die(Unit	*pAttacker, unitType specialDeath)
{
	Unit		*pUnit;
	UnitGroup *pGroup;
	int		serial1;
	MapSpace	*pSpace;

	if(primaryAction == DIE)
	{
		return;
	}

	#ifndef NDEBUG
	BamDebug.Out("Us%d(%s):Die()\n", serialNum, unitName);
	#endif

	pWorld->unitsLost[player]++;

	// store these for later reference, if needed
	unitSpecialDeath = specialDeath;
	if(pAttacker)
		killerSerial = pAttacker->serialNum;
	else killerSerial = NULL;

	// extra sound, in addition to death scream
	switch(specialDeath)
	{
		case UNIT_FIRE:
		case UNIT_ARCH_FIRE:
			pWorld->LaunchSound(SND_DEATH_FIRE, tileX, tileY);
			break;

		case UNIT_TERRAIN:
			switch(currTerrain)
			{
				case HOTBED:
				case FLAME_SPOUT:
					pWorld->LaunchSound(SND_DEATH_FIRE, tileX, tileY);
					break;

				case CAULDRON:
					pSpace = pWorld->map.GetStructure(tileX, tileY);
					if(pSpace && pSpace->h.func == FUNC_CAULDRON)
					{
						pWorld->AdjustManaLevel(pSpace->h.owner, MANA_CAULDRON);
					}
					// fall-through

				case WHIRLPOOL:
				case WATER:
					pWorld->LaunchSound(SND_DROWNING, tileX, tileY);
					pWorld->LaunchSound(SND_WATER_SPLASH, tileX, tileY);
					break;

				case CHASM:
				case PRISON_PIT:
					break;
			}
			break;
	}

	if(!(specialDeath == UNIT_TERRAIN && (currTerrain == WATER || currTerrain == CAULDRON)))
	{
		// pick a death scream
		int	deathScream = 0;

//		if(fDoppleganger)
//		{
//			deathScream = SND_DEATH_SQUEAL;
//		}
//		else
		
		switch(type)
		{
			case ACOLYTE:
				if(currAction == GUARD)
					deathScream = SND_COLLAPSING_STONES;
				else deathScream = SND_MASCULINE_SCREAM;
				break;

			case BRIGAND:
			case CLERIC:
			case DRUID:
			case PALADIN:
		 	case PEASANT:
			case RANGER:
			case WARRIOR:
			case WIZARD:
				deathScream = SND_DEATH_MALE;
				break;
	
			case DOPPLEGANGER:
			case GNOME:
				deathScream = SND_DEATH_SQUEAL;
				break;
	
			case GHOUL:
			case TROLL:
			case WRAITH:
			case ZOMBIE:
				deathScream = SND_DEATH_UNDEAD;
				break;
	
			case FURY:
			case NYMPH:
				deathScream = SND_DEATH_FEMALE1;
				break;

			case BANSHEE:
			case HARPY:
				deathScream = SND_MONSTROUS_FEMALE_SCREAM;
				break;

			case GORGON:
				deathScream = SND_DEATH_FEMALE2;
				break;
	
			case GARGOYLE:
				deathScream = SND_DEMONIC_WAIL;
				break;

			case GRIFFIN:
				deathScream = SND_AVIAN_ROAR;
				break;

			case WYRM:
				deathScream = SND_ROAR;
				break;
	
			case ROC_EGG:
			case ROC:
				deathScream = SND_BIRD_CRY;
				break;
	
			case GUARDIAN:
			case JUGGERNAUT:
				deathScream = SND_METALLIC_ROAR;
				break;

			case TORTOISE:
				deathScream = SND_WATER_SPLASH;
				break;

			case GOBLIN:
			case WOLF:
				deathScream = SND_BESTIAL_ROAR;
				break;

			case UNIT_WEIRD_WARD:
				deathScream = SND_SHATTER;
				break;

			case GOLEM:
				deathScream = SND_IF_A_TREE_FALLS_AND_NO_ONE;
				break;
		}
		if(deathScream)
			pWorld->LaunchSound(deathScream, tileX, tileY);
	}

	SetPrimaryAction(DIE);
	
	if(pAttacker && pAttacker != this)		// if killed by another unit
	{
		pWorld->unitsSlain[player][pAttacker->player]++;

		// no XPs for friendly fire
		if(pAttacker->player != player)
		{
			pGroup = pWorld->unitLib.GetUnitGroup(type);
			pWorld->AdjustXP(pAttacker->player, pGroup->expGained);
		}
	}

	while(item)		// if we have an item to give up
	{
		// if he didnt already have an item
		if(pAttacker && !pAttacker->item && GetDistanceTo(pAttacker, UNIT) < 2)
		{
			if(pAttacker->AcquireItem(item))	// if attacker is allowed to have this item
			{
				pWorld->LaunchSound(SND_GET_ITEM, pAttacker->tileX, pAttacker->tileY);
				LoseItem();	// not that it really matters at this point

				if(type == BRIGAND)
				{
					List	*plPack = ADerefAs(List, glBackpack);
					if(plPack->count > 0)		// look inside our backpack																									 	
					{
						// place next item in our hands (not that it really matters)
						AcquireItem((itemType)plPack->LastValue());
						plPack->Delete((void *)item);
					}
				}
			}
			else DropItem();
		}
		else DropItem();
	}

	if(fPackFollower)
	{
		// inform our pack leader not to update us any longer

		pUnit = (Unit *)(pWorld->ValidateTarget(packLeaderSerial));
		if(pUnit)
			pUnit->RemoveFollower(this);		// we arent following him no mo'

		packLeaderSerial = NULL;
		fPackFollower = FALSE;
	}
	else if(fPackLeader)
	{
		// instruct all members of pack to pick a new leader

		fPackLeader = FALSE;
		List	*pList = ADerefAs(List, glFollowers);
		serial1 = (uint32)pList->FirstValue();
		while(serial1)
		{
			pUnit = (Unit *)(pWorld->ValidateTarget(serial1));
			if(pUnit && pUnit->packLeaderSerial == serialNum)
			{
				pUnit->FindALeader();
			}
			serial1 = (uint32)pList->NextValue();
		}
		pList->Release();
	}
}

void
Unit::StandGuard(bool fNewState)
{
	if(fNewState)
		fStandGuard = TRUE;
	else
	{
		fStandGuard = FALSE;

		int tempX, tempY, loop1;
		Unit	*pUnit;

		for(loop1 = 1; loop1 <= 2; loop1++)
		{
			pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, tempX,
				tempY, TRUE);
			do
			{
				pUnit = pWorld->unitLib.GetUnit(tempX, tempY);
				if(pUnit && pUnit->player == player && pUnit->fStandGuard)
				{
					// wake up, AI might need you too!
					pUnit->StandGuard(FALSE);
				}				
			}
			while(pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, tempX,
				tempY));
		}
	}
}

#define	AI_SKILL_RECHARGE	(TICKS_PER_SEC / 2)
void
Unit::SetAction(action newAction, int32 xPos, int32 yPos)
{
	#ifndef NDEBUG
	BamDebug.Out("Unit(s%d g%d %s @%d/%d)::SetAction(%s %d/%d)..",
		serialNum, gSelf, unitName, tileX, tileY, pVerbNames[newAction],
		xPos, yPos);
	#endif

	// AI hack - to prevent AI from over-using healing skills, we have a timer
	if((type == CLERIC || type == PALADIN) && pBam->playerTypes[player] == PLAYER_COMPUTER
		&& newAction == SKILL && ATicks() < tLastSkillTicks + AI_SKILL_RECHARGE)
	{
		#ifndef NDEBUG
		BamDebug.Out("AI skill recharging\n");
		#endif
		return;
	}

	MapSpace	*pBuilding;
	targetTileX = -1;
	targetTileY = -1;
	secondaryX = -1;
	secondaryY = -1;
	tCurrActStarted = ATicks();
	SetSecondaryTarget(NOTARGET, NULL);
	fActionDone = TRUE;	// in case a new action cant be chosen
	Unit		*pUnit;
	fPrimarySatisfied = FALSE;

	if(xPos == -1)
	{
		xPos = tileX;
		yPos = tileY;
	}

	// check old action first, for cleanup etc.
	switch(currAction)
	{
//		case SKILL:
//			if(type == ENCHANTER)
//			{
//				// if target was not properly enchanted, make sure he is freed
//				pUnit = (Unit *)ValidateSecondaryTarget();
//				if(pUnit && pUnit->primaryAction == NO_ACTION)
//				{
//					// we stopped when we realized it was stupid - return his freedom
//					pUnit->tNextStateCheck = ATicks();
//					pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
//				}
//			}
//			break;
//
		case GUARD:
			if(newAction == SKILL && type == ACOLYTE)
			{
				// dont break meditation mode
			}
			else if(newAction == GUARD)
			{
				// dont cause guard hiccup
				break;
			}
			else if(!fActionDone || !fActionEnding)
			{
				EndGuardMode();
			}
			break;
	}

	switch(newAction)
	{
		case NO_ACTION:
			break;

		case DIE:
	 	case DISBAND:
		case BERSERK:
		case GET_STONED:
		case BLOWN_BACK:
			fInAction = FALSE;
			fActionDone = FALSE;
			break;

		case MOVE_TO:
		case GUARD:
		case TRANSFORM:
		case TRANSFORM_TO_BLDG:
			fInAction = FALSE;
			fActionDone = FALSE;
			SetSecondaryTarget(TERRAIN, NULL, xPos, yPos);
			break;

		case ATTACK:
		case STEAL:
//		case IMMITATE:
//		case VENOM:
			if(xPos == tileX && yPos == tileY)
			{
				// error!  Not allowed to do these to yourself
				fInAction = TRUE;
				fActionDone = TRUE;
				break;
			}
			// fall-thru

		case SKILL:
		case USE_ITEM:
			fInAction = FALSE;
			fActionDone = FALSE;
			// all these commands can accept units as targets, so no need to check here.


			pUnit = pWorld->unitLib.GetUnit(xPos, yPos);
			// if unit was found
			if(pUnit)
			{
				SetSecondaryTarget(UNIT, pUnit);
			}
			else
			{
				pBuilding = pWorld->map.GetStructure(xPos, yPos);
				if(pBuilding && pBuilding->h.currHP > 0)		// if building was found
				{
					SetSecondaryTarget(BUILDING, pBuilding);
				}
				else
				{
					SetSecondaryTarget(TERRAIN, NULL, xPos, yPos);
				}
			}
			break;

	}

	currAction = newAction;
	#ifndef NDEBUG
	BamDebug.Out("done\n");
	#endif
}

uint32
Unit::GetDistanceTo(uint32 xPos, uint32 yPos)
{
	int	shortest, current, loopX, loopY;

	if(type != JUGGERNAUT)
	{
		shortest = AMax(abs(xPos - tileX), abs(yPos - tileY));
		return(shortest);
	}

	shortest = 9999;

	for(loopY = tileY - unitHeight + 1; loopY <= tileY; loopY++)
		for(loopX = tileX - unitWidth + 1; loopX <= tileX; loopX++)
		{
			current = AMax(abs(xPos - loopX), abs(yPos - loopY));
			if(current < shortest)
				shortest = current;
		}
	return(shortest);
}

uint32
Unit::GetDistanceTo(void *pTarget, TargetType targType)
{
	Unit		*pUnit;
	MapSpace	*pSpace;
	int		distance, tempDistance, x, y, loop1, targetTiles;
	int8		*pTiles;

	switch(targType)
	{
		case UNIT:
			pUnit = (Unit *)pTarget;
			if(pUnit->selfType != UNIT)
				return(0);
			if(pUnit->type == JUGGERNAUT)
			{
				distance = GetDistanceTo(pUnit->tileX, pUnit->tileY);
				distance = AMin(distance, GetDistanceTo(pUnit->tileX - 1, pUnit->tileY));
				distance = AMin(distance, GetDistanceTo(pUnit->tileX, pUnit->tileY - 1));
				distance = AMin(distance, GetDistanceTo(pUnit->tileX - 1, pUnit->tileY - 1));
			}
			else
				distance = GetDistanceTo(pUnit->tileX, pUnit->tileY);
			return(distance);

		case BUILDING:
			pSpace = (MapSpace *)pTarget;
			targetTiles = pSpace->size;

			pTiles = &pSpace->tiles[0][0];
			x = 0;	y = 0;	distance = 9999;
			for(loop1 = 0;	loop1 < targetTiles; loop1++, pTiles += 2)
			{
				x = *pTiles;
				y = *(pTiles + 1);

				// is this tile occupied?
				if(pWorld->unitLib.gUnitMap[y][x])
				{
					// nevermind, tile is inaccessible.  Pretend it doesnt exist.
					continue;
				}

				if(type != JUGGERNAUT)
				{
					tempDistance = AMax(abs(tileX - x), abs(tileY - y));
				}
				else tempDistance = GetDistanceTo(x, y);

				if(tempDistance < distance)
				{
					distance = tempDistance;

					// OPTIMIZATION!
					// if distance > 5 (arbitrary #) then it's safe to say
					// the exact distance doesn't matter, because we're so far
					// away, so let's stop thinking and just return this #
					if(distance > 5 && pSpace->size > 5)
						break;
				}
				if(tempDistance == 0)
				{
					// directly beneath us - ignore
					continue;
				}
			}
			return(distance);

		default:
			return(0);

	}
}

int32 xyOffsets[MAX_DIRECTIONS][2] = {0, 1,	1, 1,		1,0,	1,-1,	0,-1,		-1,-1,	-1,0,	-1,1};
void
Unit::GetXYOffsets(direction dir, int32 &xOff, int32 &yOff)
{
	xOff = xyOffsets[dir][0];
	yOff = xyOffsets[dir][1];
}

direction		// figure proper new direction based on xDif && yDif being used this cycle
Unit::GetDir(int32 xDif, int32 yDif)
{
	if(xDif > 0)
	{
		if(yDif > 0)
			return(DOWN_RIGHT);
		else if(yDif < 0)
			return(UP_RIGHT);
		else
			return(RIGHT);
	}
	else if(xDif < 0)
	{
		if(yDif > 0)
			return(DOWN_LEFT);
		else if(yDif < 0)
			return(UP_LEFT);
		else
			return(LEFT);
	}
	else
	{
		if(yDif > 0)
			return(DOWN);
		else if(yDif < 0)
			return(UP);
		else
			return(DOWN);		// standing still
	}
//	return((direction)NULL);
}

// set walking anim, towards calculated direction
void
Unit::SetDir(int32 xDif, int32 yDif)
{
	SetDir(GetDir(xDif, yDif));
}

/*void
Unit::RunAttackAnim(direction attackDir)
{
	if(newDir > DOWN_LEFT)	// safety check
		return;

	if(!baseAnim)
		return;					// safety check

	switch(newDir)
	{
		case DOWN:
		case DOWN_RIGHT:
		case RIGHT:
		case UP_RIGHT:
		case UP:
			RunAnim(baseAnim + ANIM_ATTACK + newDir, TRUE, TRUE, 3);
			Flip(S_FM_HORZ, FALSE);
			break;

			// following 3 directions are mirrors of other directions
		case UP_LEFT:
			RunAnim(baseAnim + ANIM_ATTACK + UP_RIGHT, TRUE, TRUE, 3);
			Flip(S_FM_HORZ, TRUE);
			break;
		case LEFT:
			RunAnim(baseAnim + ANIM_ATTACK + RIGHT, TRUE, TRUE, 3);
			Flip(S_FM_HORZ, TRUE);
			break;
		case DOWN_LEFT:
			RunAnim(baseAnim + ANIM_ATTACK + DOWN_RIGHT, TRUE, TRUE, 3);
			Flip(S_FM_HORZ, TRUE);
			break;
	}
	currDir = attackDir;
}*/

// set walking anim, towards given direction
void
Unit::SetDir(direction newDir)
{
	int32 newAnimDelay;

	if(newDir > DOWN_LEFT)		// safety check
		return;

	if(baseAnim)		// failsafe
	{
//		if(currDir != newDir || TRUE)	// debug test
		if(currDir != newDir)
		{
			if(currMove < 0 || currMove > 6)
				newAnimDelay = 10;	// safety - very slow, but noticable.
			else newAnimDelay = move2ticks[currMove];
			switch(newDir)
			{
				case DOWN:
				case DOWN_RIGHT:
				case RIGHT:
				case UP_RIGHT:
				case UP:
					RunAnim(baseAnim + newDir, TRUE, TRUE, newAnimDelay);
					Flip(S_FM_HORZ, FALSE);
					break;

				// following 3 directions are mirrors of other directions
				case UP_LEFT:
					RunAnim(baseAnim + UP_RIGHT, TRUE, TRUE, newAnimDelay);
					Flip(S_FM_HORZ, TRUE);
					break;
				case LEFT:
					RunAnim(baseAnim + RIGHT, TRUE, TRUE, newAnimDelay);
					Flip(S_FM_HORZ, TRUE);
					break;
				case DOWN_LEFT:
					RunAnim(baseAnim + DOWN_RIGHT, TRUE, TRUE, newAnimDelay);
					Flip(S_FM_HORZ, TRUE);
					break;
			}
		}
	}
	lastDir = currDir;
	currDir = newDir;
}

// set which player, if any, controls this unit
void
Unit::SetSide(uint32 newSide)
{
	if(newSide > 5)
	{
		BamDebug.Out("Unit(grip %d)::SetSide(%d) - error, illegal side setting.  Assuming NEUTRAL.\n");
		newSide = SIDE0;
	}

	pWorld->unitLib.CutUnit(gSelf, TRUE);

	player = newSide;

	if(fAutoControl && (tFearLureStonedTimer || primaryAction == BERSERK || primaryAction == GET_STONED))
	{
		;	// auto-controlled already, for a special reason. Do not over-ride it.
	}
	else
	{
		if(player == SIDE0 || player == SIDE3)
			fAutoControl = TRUE;
		else
			fAutoControl = FALSE;
	}

	pWorld->unitLib.AddUnit(gSelf, TRUE);
	pWorld->unitsCreated[player]++;

	SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[player]);
}

bool
Unit::FindNearestPrimaryTarget(void)
{
	Unit	*pUnit;
	MapSpace	*pBldg;
	primaryTargetType = NOTARGET;

	pUnit = SearchForEnemies();
	pBldg = SearchForBuildings(TRUE);
	if(pUnit && (!pBldg || GetDistanceTo(pUnit, UNIT) < GetDistanceTo(pBldg, BUILDING) + 2))
	{
		SetPrimaryTarget(UNIT, pUnit);
		return(TRUE);
	}
	if(pBldg)
	{
		SetPrimaryTarget(BUILDING, pBldg);
		return(TRUE);
	}
	return(FALSE);
}

// search immediate area for a friendly building w/ Transform capacity
funcType
Unit::FindNearbyTransformer(MapSpace **pTransformer)
{
	uint32	misc = (uint32)FUNC_MISC;
	int32		loopX, loopY;
	MapSpace	*pSpace;
	funcType	function;

	// first: search for a nearby transformation building
	for(loopY =	tileY - 1; loopY < tileY + 2; loopY++)
	{
		for(loopX = tileX - 1; loopX < tileX + 2; loopX++)
		{
			pSpace = pWorld->map.GetStructure(loopX, loopY);
			if(!pSpace)								// if no building here
				continue;

			// if not accessable for transformation purposes
			if(pSpace->h.owner != player && pSpace->h.func != FUNC_IMM_WELL)
				continue;

			if(pSpace->h.func != misc)
			{
				function = (funcType)pSpace->h.func;			
				switch(function)
				{
					case FUNC_PORTAL:
						if(type == ACOLYTE)
							break;	// doesnt count

						if(pTransformer)
							*pTransformer = pSpace;
						return(function);

					case FUNC_ARBORLODGE:
					case FUNC_BARRACKS:
					case FUNC_CRYPT:
					case FUNC_TEMPLE:
					case FUNC_RUNESTONE:
					case FUNC_IMM_WELL:
						if(type != ACOLYTE)
							break;

						if(pTransformer)
							*pTransformer = pSpace;
						return(function);
				}
			}
		}
	}
	if(pTransformer)
		*pTransformer = NULL;
	return(FUNC_MISC);	// no transformers found
}

unitType
Unit::Transform(unitType newType)
{
	MapSpace		*pTransformer;
	UnitGroup	*pGroup;

	if(type != ACOLYTE && newType != ACOLYTE)
	{
		// not allowed
		return(NOUNIT);
	}

	if(nearbyTransformer == FUNC_IMM_WELL)
   {
		// randomly select a type

		// get WELL pointer
		FindNearbyTransformer(&pTransformer);

		if(!pTransformer)
		{
			// umm.. yeah, whatever.
			CheckForTransformer();
			return(NOUNIT);
		}
		else do
		{
			newType = pTransformer->GetRelatedUnitType(random(5),
				(funcType)(FUNC_ARBORLODGE + random(5)));
		} while(newType == NOUNIT);			
	}
	else if(!newType)
		return(NOUNIT);

	if(pBam->playerTypes[player] == PLAYER_COMPUTER)
	{
		// check for mana here so we can return success/failure
		pGroup = pWorld->unitLib.GetUnitGroup(newType);
		if(pWorld->GetManaLevel(player) < pGroup->transformCost)
			return(NOUNIT);
	}

	metaPodType = newType;

	SetPrimaryAction(TRANSFORM, tileX, tileY);
	return(newType);
}

void
Unit::CycleTransform(void)
{
	ticks_t	currTicks = ATicks();
	grip		gAni;
	int		currPhase;
	UnitGroup	*pGroup;
	BAM_Ani	*pAni;

	if(currTicks < tNextStateCheck)
		return;

	if(fActionDone)
		return;

	if(!fInAction)
	{
		fInAction = TRUE;

		if(!metaPodType)
		{
			// error, abort cleanly.
			fActionEnding = TRUE;
			return;
		}

		if(metaPodType == ACOLYTE)
		{
			if(nearbyTransformer != FUNC_PORTAL)
			{
				metaPodType = NOUNIT;
				fActionEnding = TRUE;
				return;
			}
		}
		else
		{
			// transforming to something else

			// if there is no transformer building nearby
			if(nearbyTransformer == FUNC_MISC)
			{
				metaPodType = NOUNIT;
				fActionEnding = TRUE;
				return;
			}
		}

		pGroup = pWorld->unitLib.GetUnitGroup(metaPodType);

		// if cant afford it
		if(!pWorld->SpendMana(player, pGroup->transformCost, tileX, tileY))
		{
			metaPodType = NOUNIT;
			fActionEnding = TRUE;
			return;
		}
		fAutoControl = TRUE;
		tAutoActionTimer = currTicks;

		TRACK_MEM("TransformAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_METAPOD, 1, priorityLevel + 2,
			TRUE, FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff);
		pAni = ADerefAs(BAM_Ani, gAni);
		pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[player]);
		pWorld->vPort.CheckAni(gAni);

 		pWorld->LaunchSound(SND_METAPOD_START, tileX, tileY);

		// cause metapod stats to take over
		SetCurrTerrain(currTerrain);

		if(item == HARPY_NET)
		{
			// can't keep this!
			DropItem();
		}

		tNextStateCheck = currTicks + TICKS_PER_SEC / 2;
		return;
	}
	
	// if abort flag set externally
	if(fActionEnding)
	{
		// nevermind, go back to normal
		fActionDone = TRUE;
		fAutoControl = FALSE;

		// if transformation was allowed to initiate
		if(metaPodType)
		{
			// poof!
			TRACK_MEM("TransformAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1, priorityLevel + 2,
				TRUE, FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff);
			pWorld->vPort.CheckAni(gAni);	
			metaPodType = NOUNIT;
		}
		currState = ST_NULL;
		currAnimState = ST_ANIM_NULL;	// force restoration of guard anim

		SetCurrTerrain(currTerrain);
		if(fCursorTracking)
		{
			// cause Transform button text to be updated as appropriate
			pWorld->SetStatusBoxMode(TERRAIN);
			pWorld->DrawStatusBox();
		}

		return;
	}
	else if(currState != ST_TRANSFORMING)
	{
		// now that the cover ani has run over us, change our own image to match it before it goes away
		currState = ST_TRANSFORMING;
		RunAnimation(ST_ANIM_METAPOD, 0, FALSE, DOWN, TRUE, FALSE);
	}


	pGroup = pWorld->unitLib.GetUnitGroup(metaPodType);

	if(pGroup->transformTime)
	{
		currPhase = ((int)currTicks - tAutoActionTimer) / ((pGroup->transformTime * TICKS_PER_SEC * 10) / 70) + 1;
	}
	else currPhase = 8;

	currPhase = AMin(currPhase, 8);
	if(currPhase < 8)
	{
		if(fCursorTracking)
		{
			if(pWorld->statBoxCurrMode != TRANSFORMING)
				pWorld->SetStatusBoxMode(TRANSFORMING);
			pWorld->SetTransformingButtons(serialNum);
			pWorld->DrawUnitStatus(serialNum, (bool)(currPhase > 1));
		}

		// calc ticks til next phase change
//		tNextStateCheck = currTicks + (pGroup->transformTime * TICKS_PER_SEC * 8) / 70;
		tNextStateCheck = currTicks + (pGroup->transformTime * TICKS_PER_SEC * 10) / 70;
	}
	else
	{
		// perform transformation
		currState = ST_NULL;
		currAnimState = ST_ANIM_NULL;
		fActionDone = TRUE;
		fAutoControl = FALSE;

		SetType(metaPodType);
		metaPodType = NOUNIT;
//		AcquireItem(LoseItem());		// re-register item stat modifiers

		// creation bonus
		pWorld->AdjustXP(player, pWorld->unitLib.GetUnitGroup(type)->expGained);

		if(bGlobal.storyLine == TUTORIAL && player == pWorld->playerSide)
		{
			if(type == CLERIC)
				pWorld->SetTutorialGoal(TUT_UNIT_TRANSFORMED, TRUE);
		}

		// poof!
		TRACK_MEM("TransformAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1, priorityLevel + 2,
			TRUE, FALSE, 3, tileX, tileY, FALSE, tileXOff, tileYOff);
		pWorld->vPort.CheckAni(gAni);

		if(fCursorTracking)
		{
			// cause Transform button text to be updated as appropriate
			pWorld->SetStatusBoxMode(TERRAIN);
			pWorld->DrawStatusBox();
		}

		pWorld->LaunchSound(SND_TRANSFORM, tileX, tileY);

		if(pBam->playerTypes[player] == PLAYER_LOCAL)
		{
			pWorld->Announce(tileX, tileY, SQUIB_WORLD_MSGS, SQUIB_WORLD_YOUR,
				SQUIB_UNIT_NAMES, type, SQUIB_WORLD_MSGS, SQUIB_WORLD_ISREADY);
		}
	}
	return;
}

bool
Unit::SetType(unitType newType)
{
	UnitGroup	*pUnitGroup;
	grip			gUnitGroup;
	unitType		oldType = type;
	itemType		item1;
	MapSpace		*pBldg;
	List			*pList;

	type = newType;

	switch(type)
	{
		case BRIGAND:
			// Brigands can carry lots of items
			if(glBackpack)
				break;
			pList = new List(10, TRUE);
			glBackpack = pList->gSelf;		
			break;

		case WOLF:
		case PEASANT:
		case GOBLIN:
			// pack animals
			if(glFollowers)
				break;
			pList = (List *)new List(MAX_PACK_SIZE - 1, TRUE);
			glFollowers = pList->gSelf;
			break;
	}

	gUnitGroup = pWorld->unitLib.gUnitGroups[type];
	if(!gUnitGroup)
	{
		BamDebug.Out("Unit::SetType(%d) - error, invalid type!\n");
		return(FALSE);
	}

	if(type == JUGGERNAUT)
	{
		unitWidth = 2;
		unitHeight = 2;
	}
	else
	{
		unitWidth = 1;
		unitHeight = 1;
	}

	pUnitGroup = ADerefAs(UnitGroup, gUnitGroup);
	movement = pUnitGroup->movement;	// movement type (walker, flier, etc)
	baseAnim = pUnitGroup->baseAnim;
	portAnim = pUnitGroup->portAnim;
	strcpy(unitName, pUnitGroup->unitName);

	// current && base stats.
	baseAttack = pUnitGroup->baseAttack;
	baseRange = pUnitGroup->baseAttackRange;
	baseDefense = pUnitGroup->baseDefense;
	baseMove = pUnitGroup->baseMoveRate;

	SetCurrTerrain(currTerrain);		// reset stats according to terrain type

	if(type == UNIT_WEIRD_WARD)
	{
		maxHP = 0;	// doesnt need them
		currHP = 0;
	}
//	else if((type == ACOLYTE && oldType == FURY) || fDoppleganger)
	else if(type == ACOLYTE && oldType == FURY)
	{
		maxHP = pUnitGroup->maxHP;

		// retain old HP, within limit
		if(currHP > maxHP)
			currHP = maxHP;

		if(type == ACOLYTE && oldType == FURY)
		{
			// end of FURY's obsession mode
			fAutoControl = FALSE;
			SetSide(player);
		}
	}
	else
	{
		maxHP = pUnitGroup->maxHP;
		currHP = maxHP;
	}

	if(type == FURY)
	{
		// remove from any bldg's defender list - we wont help them now
		pBldg = SearchForBuildings(FALSE);
		if(pBldg)
			pBldg->RemoveDefender(this);
	}

	// re-acquire current item here, in case it affects currStats
	item1 = item;
	LoseItem();
	if(!AcquireItem(item1))
	{
		// new type cant hold that item.  Restore it, then drop it.
		item = item1;
		DropItem();
	}

	RunAnimation(ST_ANIM_GUARD, 3);

	// get cost and range of effect of unit's skill, based on new type
	fSkillNeedsTarget = FALSE;
//	if(fDoppleganger)
//	{
//		skillRange = 1;
//		skillCost = 10;
//	}
//	else
	{
		skillRange = pUnitGroup->skillRange;
		skillCost = pUnitGroup->skillCost;
	}

	skillType = SKILL_NONE;

	if(skillRange > 0)
	{
		switch(type)
		{
			case BANSHEE:
			case ENCHANTER:
			case GORGON:
			case HARPY:
			case NYMPH:
			case WRAITH:
			case WYRM:
				skillType = SKILL_AGGRESSIVE;
				break;

			case GNOME:
			case CLERIC:
			case PALADIN:
				skillType = SKILL_PASSIVE;
				break;
		}

		switch(type)
		{
			case BANSHEE:
			case WRAITH:
			case ACOLYTE:
			case NYMPH:
				fSkillNeedsTarget = FALSE;
				break;

			default:
				fSkillNeedsTarget = TRUE;
				break;
		}
		
		if(type == ACOLYTE)
			SetActionAvailable(SKILL, FALSE);
		else SetActionAvailable(SKILL, TRUE);
	}
	else SetActionAvailable(SKILL, FALSE);

	SetActionAvailable(NO_ACTION, TRUE);	// seems silly, but it's necessary
	SetActionAvailable(ATTACK, TRUE);
	SetActionAvailable(GUARD, TRUE);
	SetActionAvailable(MOVE_TO, TRUE);
	SetActionAvailable(DISBAND, TRUE);
	switch(type)
	{
		case ROC_EGG:
			SetActionAvailable(ATTACK, FALSE);
			SetActionAvailable(GUARD, FALSE);
			SetActionAvailable(MOVE_TO, FALSE);
			SetActionAvailable(DISBAND, FALSE);
			break;
	 	
		case FURY:
			SetActionAvailable(GUARD, FALSE);
			SetActionAvailable(MOVE_TO, FALSE);
			break;
	}

	switch(type)
	{
		case BANSHEE:
			tAutoActionTimer = ATicks() + TICKS_PER_SEC * (ARandom(15) + 15);
			break;

		case TROLL:
			tAutoActionTimer = ATicks() + TICKS_PER_SEC * 2;
			break;

		case FURY:
			tAutoActionTimer = ATicks() + FURY_DMG_INTERVAL;
			break;

		default:
			tAutoActionTimer = 0;
			break;
	}

	if(type == ACOLYTE)
		SetActionAvailable(TRANSFORM, TRUE);
	else SetActionAvailable(TRANSFORM, FALSE);

	switch(movement)
	{
		case SWIMMER:
			priorityLevel = PRI_SWIMMER;
			break;

		case FLIER:
			priorityLevel = PRI_FLIER;
			break;

		case WALKER:
		case FLOATER:
		default:
			priorityLevel = PRI_WALKER;
			break;
	}

	return(TRUE);
}

void
Unit::FindALeader(void)
{
	grip	gUnit;
	Unit	*pUnit, *pLeader, *pLeaders[50];
	int	leaderCount = 0, leaderDistance, newDistance, loop1;

	if(type != PEASANT && type != WOLF && type != GOBLIN)
	{
		BamDebug.Out("Unit::FindALeader(g%d, type=%d) - error, not PEASANT, WOLF, or GOBLIN!\n",
			(int)gSelf, (int)type);
		SetPrimaryAction(GUARD, tileX, tileY);
		return;
	}

	// search through unit list, looking for units of our type, who are already pack leaders
	gUnit = (grip)pWorld->unitLib.lUnits[player].FirstValue();
	while(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
		if(pUnit->type == type && pUnit->fPackLeader)
		{
			List	*pList = ADerefAs(List, pUnit->glFollowers);
			if(pList->count < MAX_PACK_SIZE - 1)
				pLeaders[leaderCount++] = pUnit;
		}
		gUnit = (grip)pWorld->unitLib.lUnits[player].NextValue();
	}
	if(!leaderCount)		// if no avail leaders found
	{
		BecomeALeader();
		return;
	}

	// find closest leader
	pLeader = NULL;
	leaderDistance = 9999;
	for(loop1 = 0; loop1 < leaderCount; loop1++)
	{
		pUnit = pLeaders[loop1];

		// is this unit closer than best so far?
		newDistance = GetDistanceTo(pUnit, UNIT);
		if(!pLeader || newDistance < leaderDistance)
		{
			pLeader = pUnit;
			leaderDistance = newDistance;
		}
	}
	fPackFollower = TRUE;	// we have a leader now
 	packLeaderSerial = pLeader->serialNum;
	pLeader->AddFollower(this);		// let leader know about us.  He'll give us new orders.
}

void
Unit::BecomeALeader(void)
{
	List	*pList;

	fPackLeader = TRUE;
	fPackFollower = FALSE;
	packLeaderSerial = NULL;	// not following another leader

	pList = ADerefAs(List, glFollowers);
	pList->Release();		// make sure list of followers is empty
	SetPrimaryAction(GUARD);		// all leaders make periodic decisions from GUARD mode
}

void
Unit::RemoveFollower(Unit *pFollower)
{
	List	*pList = ADerefAs(List, glFollowers);
	pList->Delete((void *)pFollower->serialNum);
}

void
Unit::AddFollower(Unit *pFollower)
{
	List	*pList;

	pList = ADerefAs(List, glFollowers);
	pList->Add((void *)pFollower->serialNum);
	UpdateFollower(pFollower);
}

void
Unit::UpdateAllFollowers(void)
{
	uint32	followerSerial;
	Unit		*pUnit;
	List		*pList;

	if(!fPackLeader)
	{
		return;
	}

	pList = ADerefAs(List, glFollowers);
	if(!pList->count)
		return;

	followerSerial = (uint32)pList->FirstValue();
	while(followerSerial)
	{
		pUnit = (Unit *)(pWorld->ValidateTarget(followerSerial));
		if(pUnit)
			UpdateFollower(pUnit);
		else pList->Delete((void *)followerSerial);
		followerSerial = (uint32)pList->NextValue();
	}
}

void
Unit::UpdateFollower(Unit *pUnit)
{
	if(pUnit)
	{
		pUnit->SetPrimaryAction(primaryAction, primaryX, primaryY);
		pUnit->tNextStateCheck = ATicks() + ARandom(TICKS_PER_SEC * 2);
	}
}

// adjust primaryAction && currAction to match leader, if necessary
// only called when follower is cycling GUARD mode
bool
Unit::FollowTheLeader(void)
{
	Unit	*pLeader;

	pLeader = (Unit *)(pWorld->ValidateTarget(packLeaderSerial));
	if(!pLeader)
	{
		FindALeader();		// safety net
		return(TRUE);
	}

	switch(pLeader->primaryAction)		// what is the leader doing?
	{
		case ATTACK:
			fActionDone = TRUE;	// maybe we were blocked en-route.  Stop guarding, try to reach target again
			return(TRUE);

		case GUARD:
		case MOVE_TO:
		default:
			if(!WithinRangeForPrimary(pLeader->ValidatePrimaryTarget(), pLeader->primaryAction, pLeader->primaryX,
			  pLeader->primaryY))	// if not where leader wants to be
				if(currAction != MOVE_TO)										// and not moving
				{
					fActionDone = TRUE;	// not where we should be - stop guarding, start walking
					return(TRUE);
				}
			break;
	}
	return(FALSE);
}

void
Unit::RunAnimation(animState_t newAnimState, int32 newSpeed, bool fNewCycle, direction newDir, bool fImmediate,
	bool fNewFwd)
{
//	FILE	*fAnimLog = fopen("RUNANIM.LOG", "a");
//	if(fAnimLog)
//		fprintf(fAnimLog, "@%8d Unit g%d(%10s) newAnimState==%d\n", ATicks(), gSelf, unitName, newAnimState);
//	fclose(fAnimLog);

	struct AnimInfo	*pNewInfo;
	uint32	resNum;

	if(newDir == DNULL)
		newDir = DOWN;	// init failsafe

	if(fImmediate)	// anim must be run immediately
	{
		PurgeAnimQeue();

		fAniDone = TRUE;			// and halt current animation, if any
		tLastCycle = NULL;		// In this case, a re-cycle within this tick is okay
	}

	pNewInfo = &animInfo[pendingAnims];
 	pNewInfo->state = ST_ANIM_NULL;
	pNewInfo->fHFlip = FALSE;
	pNewInfo->fVFlip = FALSE;
	pNewInfo->fFwd = TRUE;
	pNewInfo->fCycle = TRUE;
	pNewInfo->resNum = 0;
	pNewInfo->speed = 0;

	// certain anim states have default or computed animation speeds
	// which we handle here
	switch(newAnimState)
	{
		case ST_ANIM_WALK:	
			if(currMove < 0 || currMove > 6)
			{
				BamDebug.Out("Unit(g%d)::RunAnimation() - currMove == %ld\n", gSelf, currMove);
				newSpeed = move2ticks[1];	// failsafe - very slow, but noticable.
			}
			else
			{
				// anim speed based on unit movement rate
				newSpeed = move2ticks[currMove];
			}
			break;
	}

	// load new anim info

	// certain directions are flips of other directions
	switch(newAnimState)
	{
		case ST_ANIM_WALK:
			switch(type)
			{
				case ROC_EGG:
					resNum = ANIM_ROC_EGG_DIES;
					fNewFwd = TRUE;	// first cel
					newSpeed = 0;
					fNewCycle = FALSE;
					break;

				case UNIT_WEIRD_WARD:
					// run death ani backwards
					resNum = ANIM_WARD_SHATTERS;
					fNewFwd = FALSE;
					fNewCycle = TRUE;
					break;

				default:
					resNum = baseAnim + ANIM_WALK;

					switch(newDir)
					{
						case UP_LEFT:
							newDir = UP_RIGHT;
							pNewInfo->fHFlip = TRUE;
							break;
						case LEFT:
							newDir = RIGHT;
							pNewInfo->fHFlip = TRUE;
							break;
						case DOWN_LEFT:
							newDir = DOWN_RIGHT;
							pNewInfo->fHFlip = TRUE;
							break;
					}
					resNum += newDir;
					break;
			}
			break;

		case ST_ANIM_METAPOD:
			resNum = ANIM_METAPOD;
			break;

		case ST_ANIM_GUARD:
			if(currAction == TRANSFORM && currState == ST_TRANSFORMING)
			{
				resNum = ANIM_METAPOD;
				newAnimState = ST_ANIM_METAPOD;
				newSpeed = 3;
			}
			else switch(type)
			{
				case ACOLYTE:
					resNum = baseAnim + ANIM_TO_OBELISK;
					newSpeed = 2;
			 		newAnimState = ST_ANIM_OBELISK;
					break;

				case ROC_EGG:
					resNum = ANIM_ROC_EGG_HIT;
					fNewFwd = FALSE;	// last cel
					newSpeed = 0;		// and only first cel
					fNewCycle = FALSE;
					break;

				case UNIT_WEIRD_WARD:
					resNum = ANIM_WARD_SPINS;
					fNewFwd = TRUE;
					fNewCycle = TRUE;
					break;

				default:
					resNum = baseAnim + ANIM_GUARD;
		
					switch(newDir)
					{
						case UP:
		 				case DOWN:
						default:
//							BamDebug.Out("%s (g%d, %d/%d)\n", unitName, gSelf, tileX, tileY);
							pNewInfo->fHFlip = (ARandom(2)? TRUE: FALSE);
							break;

//						case UP_LEFT:
//						case LEFT:
//						case DOWN_LEFT:
//							pNewInfo->fHFlip = TRUE;
//							break;
					}
					break;
			}
			break;

		case ST_ANIM_OBELISK:
			if(type != ACOLYTE)	// probably transforming from ACOLYTE in OBELISK mode
			{
				resNum = baseAnim + ANIM_GUARD;
				newAnimState = ST_ANIM_GUARD;
			}
			else
			{
				resNum = baseAnim + ANIM_TO_OBELISK;
			}
			newSpeed = 2;
			break;
		
		case ST_ANIM_ATTACK2:
		case ST_ANIM_ATTACK:
			if(newAnimState == ST_ANIM_ATTACK2)
			{
				newAnimState = ST_ANIM_ATTACK;
				resNum = baseAnim + ANIM_ATTACK2;
			}
			else
			{
				resNum = baseAnim + ANIM_ATTACK;
			}
			switch(newDir)
			{
				case UP_LEFT:
					newDir = UP_RIGHT;
					pNewInfo->fHFlip = TRUE;
					break;
				case LEFT:
					newDir = RIGHT;
					pNewInfo->fHFlip = TRUE;
					break;
				case DOWN_LEFT:
					newDir = DOWN_RIGHT;
					pNewInfo->fHFlip = TRUE;
					break;
			}
			resNum += newDir;
			break;

		case ST_ANIM_HIT:
			resNum = 0;

			if(currAction == TRANSFORM)
				resNum = ANIM_METAPOD_HIT;
			else switch(type)
			{
				case ACOLYTE:
					// if ACOLYTE is transforming TO, or currently in, obelisk mode
					if(currState == ST_OBELISK || (currState == ST_OBELISKING && fAniFwd)
						|| currAnimState == ST_ANIM_GUARD)
						resNum = ANIM_OBELISK_IS_HIT;
					break;

				case ROC_EGG:
					resNum = ANIM_ROC_EGG_HIT;
					break;
			}

			if(!resNum)
			{
				resNum = baseAnim + ANIM_IS_HIT;

				// which dir are we facing?
				switch(newDir)
				{
					case DOWN:
						newDir = (direction)0;
						break;
					case DOWN_RIGHT:
						newDir = (direction)0;
						break;
					case RIGHT:
						newDir = (direction)1;
						break;
					case UP_RIGHT:
						newDir = (direction)1;
						break;
					case UP:
						newDir = (direction)1;
						pNewInfo->fHFlip = TRUE;
						break;
					case UP_LEFT:
						newDir = (direction)1;
						pNewInfo->fHFlip = TRUE;
						break;
					case LEFT:
						newDir = (direction)0;
						pNewInfo->fHFlip = TRUE;
						break;
					case DOWN_LEFT:
						newDir = (direction)0;
						pNewInfo->fHFlip = TRUE;
						break;
				}
				resNum += newDir;
			}
			break;

		case ST_ANIM_STONED:
			resNum = baseAnim + ANIM_STONED;
			switch(newDir)
			{
				case DOWN:
				case UP:
					pNewInfo->fHFlip = ARandom(2)? TRUE: FALSE;
					break;

				case DOWN_RIGHT:
				case RIGHT:
				case UP_RIGHT:
					pNewInfo->fHFlip = FALSE;
					break;

				case UP_LEFT:
				case LEFT:
				case DOWN_LEFT:
					pNewInfo->fHFlip = TRUE;
					break;
			}
			break;

		case ST_ANIM_DIE:
			resNum = 0;

			if(currState == ST_TRANSFORMING)
			{
				resNum = ANIM_METAPOD_DIES;
			}
//			else if(fDoppleganger)
//			{
//				resNum = ANIM_DOPPLEGANGER_DIES;
//			}
			else switch(type)
			{
				case ACOLYTE:
//					if(currAction == TRANSFORM)
//						// allow this to be normal death, regardless of currState
//						;//resNum = ANIM_METAPOD_DIES;
//					else
					if(currState == ST_OBELISK || (currState == ST_OBELISKING && fAniFwd))
						resNum = ANIM_OBELISK_DIES;
					break;

				case ROC_EGG:
					resNum = ANIM_ROC_EGG_DIES;
					break;

				case UNIT_WEIRD_WARD:
					resNum = ANIM_WARD_SHATTERS;
					break;
			}

			if(!resNum)
				resNum = baseAnim + ANIM_DIES;
			break;

		case ST_ANIM_NULL:	// setup condition
			return;

		default:
			BamDebug.Out("Unit::RunAnimation(g%d, %s) - error, unknown animation state given.\n", gSelf, unitName);
			return;
	}

 	pNewInfo->resNum = resNum;
	pNewInfo->state = newAnimState;
	pNewInfo->speed = newSpeed;
	pNewInfo->fCycle = fNewCycle;
	pNewInfo->fFwd = fNewFwd;

	// add it to the anim qeue for this unit
	pendingAnims++;

	if(fImmediate)
	{
		BAM_Ani::SmartCycle(FALSE);	// cause first cel of new ani to display
	}
	else
	{
		// first cel of first entry (FIFO) will be displayed in BAM_Ani::Cycle() 
		// when fAniDone==TRUE
	}
}


