// UNITLIB.CPP
// Unit librarian
//
#include <ctype.h>

#include "apires.hpp"
#include "tigre.hpp"
#include "debug.hpp"
#include "savebase.hpp"

#include "bam.hpp"
#include "unitlib.hpp"

extern 	World	*pWorld;
#ifdef NDEBUG
//#undef NDEBUG
#endif
Debugger UnitDebug;

//-------------------------------------------------------
// UnitGroup class

UnitGroup::~UnitGroup()
{
}

void
UnitGroup::Init(unitType newType, char *parms)
{
	char		*token, *str, buffer[80];
	uint32	totalTerrains, loop1;
	Terrain	*pTerrain;

	strcpy(buffer, parms);
	type = newType;

	token = GetFirstToken(buffer);
//	token = strtok(buffer, " \t");

	// check start-of-line marker
	if(strcmp("#", token))
		return;

	// get unit name	
//	token = strtok(NULL, " \t");
	token = GetNextToken();
	strncpy(unitNameInternal, token, sizeof(unitNameInternal) - 1);
	strncpy(unitName, pWorld->squib1.Load(SQUIB_UNIT_NAMES, (int)type),
		sizeof(unitName) - 1);

	str = unitName;
	while(*str)
	{
		if(*str == '_')	
			*str = ' ';
		str++;
	}

	// get movement rate
//	token = strtok(NULL, " \t");
	token = GetNextToken();
	baseMoveRate = atoi(token);

	// get movement type
//	token = strtok(NULL, " \t");
	token = GetNextToken();
	switch(toupper(*token))
	{
		case 'W':
			movement = WALKER;
			break;
		case 'S':
			movement = SWIMMER;
			break;
		case 'F':
			movement = FLIER;
			break;
		case 'L':
			movement = FLOATER;
			break;
		default:
			return;
	}

	token = GetNextToken();
	maxHP = atoi(token);

	token = GetNextToken();
	baseAttack = atoi(token);

	token = GetNextToken();
	baseDefense = atoi(token);

	token = GetNextToken();
	baseAttackRange = atoi(token);

	token = GetNextToken();
	baseAnim = atoi(token);

	token = GetNextToken();
	portAnim = atoi(token);

	token = GetNextToken();
	skillRange = atoi(token);

	token = GetNextToken();
	skillCost = atoi(token);

	token = GetNextToken();
	if(token)
		transformCost = atoi(token);

	token = GetNextToken();
	if(token)
		transformTime = atoi(token);

	token = GetNextToken();
	if(token)
	{
		expRequired = atoi(token);

		// any units with EXP cost == 0, no need to research
		if(!expRequired)
			bGlobal.unitsResearched[type] = TRUE;
	}

	token = GetNextToken();
	if(token)
		expGained = atoi(token);

	token = GetNextToken();
	if(token)
		researchTime = atoi(token);

	// pre-compute base move rates for all terrains	
	totalTerrains = AMin(MAX_TILE_GROUPS, TERRAINMAXTYPES);
	totalTerrains = AMin(totalTerrains, pWorld->pTileLib->totalTerrains);

	int32	attBonus, defBonus, rangeBonus;

	for(loop1 = 0; loop1 < totalTerrains; loop1++)
	{
		pTerrain = &(pWorld->pTileLib->terrains[loop1]);

		if(FreeMovementIn((terrainType)loop1))		// if immune to movement effects of this terrain
			moveRates[loop1] = baseMoveRate;				// then use base vals
		else if(NoMovementIn((terrainType)loop1))
			moveRates[loop1] = 0;
		else
		{
			// calc our move rate for this terrain type
			moveRates[loop1] = baseMoveRate - pTerrain->moveRate;
		}

		// get bonus/penalties specific to this unit type in this terrain
		if(movement == WALKER)
		{
			// unit-specific terrain bonuses override universal terrain bonuses/penalties
			GetTerrainBonuses((terrainType)loop1, attBonus, rangeBonus, defBonus);
//			attack[loop1] = baseAttack + (attBonus? attBonus: -pTerrain->attackMod);
			attack[loop1] = baseAttack;
			if(attBonus)
				attack[loop1] += attBonus;
			
			if(!attBonus || !pTerrain->attackMod)
				attack[loop1] -= pTerrain->attackMod;
			else if(attBonus / abs(attBonus) != pTerrain->attackMod /
				abs(pTerrain->attackMod))
			{
				// both are either positive or negative - let them be cumulative
				attack[loop1] += pTerrain->attackMod;
			}

			// range no longer supported!
			// attackRange[loop1] = baseAttackRange + (rangeBonus? rangeBonus: -pTerrain->attackRangeMod);
			attackRange[loop1] = baseAttackRange;

			defense[loop1] = baseDefense;
			if(defBonus)
				defense[loop1] += defBonus;
			
			if(!defBonus || !pTerrain->defenseMod)
				defense[loop1] += pTerrain->defenseMod;
			else if(defBonus / abs(defBonus) == pTerrain->defenseMod /
				abs(pTerrain->defenseMod))
			{
				// both are either positive or negative - let them be cumulative
				defense[loop1] += pTerrain->defenseMod;
			}

		}
		else	// fliers, floaters, swimmers
		{
			attack[loop1] = baseAttack;
			attackRange[loop1] = baseAttackRange;
			defense[loop1] = baseDefense;
		}
	}		
}

bool
UnitGroup::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&uGroupDataStart, 1, (int)&uGroupDataEnd -
				(int)&uGroupDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&uGroupDataStart, 1, (int)&uGroupDataEnd -
				(int)&uGroupDataStart, pFile);
			break;
	}
	return(TRUE);
}

UnitGroup::UnitGroup(void)
{
	uint32	loop1;

	type = NOUNIT;
	movement = WALKER;
	baseMoveRate = 0;
	baseAttack = 0;
	baseDefense = 0;
	baseAttackRange = 0;
	maxHP = 0;
	baseAnim = 0;
	unitName[0] = NULL;
	for(loop1 = 0; loop1 < TERRAINMAXTYPES; loop1++)
	{
		attack[loop1] = 0;
		attackRange[loop1] = 0;
		defense[loop1] = 0;
		moveRates[loop1] = 0;
	}
	transformCost = 0;
	transformTime = 0;
}

//----------------------------------------------------------------------
//UnitLib class
UnitLib::UnitLib(void)
{
	int	loop1;

	#ifndef NDEBUG
	UnitDebug.OpenWindow(0, 0, 79, 6);
	UnitDebug.SetScrollCount(2);
	#endif

	totalUnits = 0;
	memset(gUnitGroups, NULL, sizeof(gUnitGroups));
//	for(loop1 = 0; loop1 < UNITMAXTYPES; loop1++)
//		gUnitGroups[loop1] = NULL;

	// no units in the world yet
	memset(gUnitMap, NULL, sizeof(gUnitMap));

	for(loop1 = 0;	loop1 < 5; loop1++)
	{
		lUnits[loop1].resizable = TRUE;
		lUnits[loop1].fDuplicates = FALSE;
	}

	// neutrals
	SetEnemyFlag(SIDE0, SIDE0, FALSE);
	SetEnemyFlag(SIDE0, SIDE1, FALSE);
	SetEnemyFlag(SIDE0, SIDE2, FALSE);
	SetEnemyFlag(SIDE0, SIDE3, TRUE);
	
	// player's side
	SetEnemyFlag(SIDE1, SIDE0, FALSE);
	SetEnemyFlag(SIDE1, SIDE1, FALSE);
	SetEnemyFlag(SIDE1, SIDE2, TRUE);
	SetEnemyFlag(SIDE1, SIDE3, TRUE);
	
	// opponent's feelings
	SetEnemyFlag(SIDE2, SIDE0, FALSE);
	SetEnemyFlag(SIDE2, SIDE1, TRUE);
	SetEnemyFlag(SIDE2, SIDE2, FALSE);
	SetEnemyFlag(SIDE2, SIDE3, TRUE);

	// wild animals
	SetEnemyFlag(SIDE3, SIDE0, TRUE);
	SetEnemyFlag(SIDE3, SIDE1, TRUE);
	SetEnemyFlag(SIDE3, SIDE2, TRUE);
	SetEnemyFlag(SIDE3, SIDE3, TRUE);
}

bool
UnitLib::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&uLibDataStart, 1, (int)&uLibDataEnd -
				(int)&uLibDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&uLibDataStart, 1, (int)&uLibDataEnd -
				(int)&uLibDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
UnitLib::SetEnemyFlag(int32 sideA, int32 sideB, bool fNewState)
{
	fEnemyFlags[sideA][sideB] = fNewState;
}

bool
UnitLib::CheckEnemyFlag(int32 sideA, int32 sideB)
{
	return(fEnemyFlags[sideA][sideB]);
}

void
UnitLib::Cleanup(void)
{
	uint32	loop1;
	grip		gUnit;

	// de-alloc all units, removing them from the Scrimage list as well
	for(loop1 = 0;	loop1 < 5; loop1++)
	{
		gUnit = (grip)lUnits[loop1].FirstValue();
		while(gUnit)
		{
			DeleteUnit(gUnit);		// de-alloc
			gUnit = (grip)lUnits[loop1].NextValue();
		}
		lUnits[loop1].Release();
	}

	for(loop1 = 0; loop1 < UNITMAXTYPES; loop1++)
		if(gUnitGroups[loop1])
			ADelete(gUnitGroups[loop1]);
}

UnitLib::~UnitLib(void)
{
	#ifndef NDEBUG
	UnitDebug.CloseWindow();
	#endif
}

unitType
UnitLib::GetUnitType(char *unitName)
{
	uint32		loop1;
	UnitGroup	*pUnitGroup;

	for(loop1 = 0; loop1 < UNITMAXTYPES; loop1++)
	{
		pUnitGroup = GetUnitGroup((unitType)loop1);
		if(!memcmp(unitName, pUnitGroup->unitNameInternal, strlen(unitName)))
			return((unitType)loop1);
	}
	return(NOUNIT);
}

UnitGroup *
UnitLib::GetUnitGroup(unitType type)
{
	UnitGroup	*pGroup;
	grip			gGroup;

	if(type >= UNITMAXTYPES)
		return(NULL);
	gGroup = gUnitGroups[type];
	if(!gGroup)
		return(NULL);
	pGroup = ADerefAs(UnitGroup, gGroup);
	return(pGroup);
}

void
UnitLib::PlaceUnitGrip(grip gUnit, int32 x, int32 y)
{
	// do NOT check for previous/current grip at X,Y
	// because the tortoise's grip must be overwritten by its rider
	gUnitMap[y][x] = gUnit;
}

// return grip of Unit located at x,y if any
grip
UnitLib::GetUnitGrip(int32 x, int32 y)
{
	if(y < 0 || y >= WORLD_HEIGHT || x < 0 || x >= WORLD_WIDTH)		// if invalid coords
		return(NULL);

	return(gUnitMap[y][x]);
}

// return ptr to Unit located at x,y if any
Unit *
UnitLib::GetUnit(int32 x, int32 y)
{
	grip		gUnit;
	Unit		*pUnit;

	if(y < 0 || y >= WORLD_HEIGHT || x < 0 || x >= WORLD_WIDTH)		// if invalid coords
		return(NULL);

	gUnit = gUnitMap[y][x];
	if(!gUnit)
		return(NULL);
	pUnit = ADerefAs(Unit, gUnit);
	return(pUnit);
}

// count the # of units on a given within [Z] tiles of coord X/Y
int
UnitLib::CountUnits(int xCenter, int yCenter, int side, int radius)
{
	int	x1, x2, y1, y2, loopX, loopY, unitCount = 0;
	Unit	*pUnit;
	grip	gUnit;

	if(radius > 0)
		radius--;

	x1 = AMax(0, xCenter - radius); x2 = AMin(WORLD_WIDTH - 1, xCenter + radius);
	y1 = AMax(0, yCenter - radius); y2 = AMin(WORLD_HEIGHT - 1, yCenter + radius);

	for(loopY = y1; loopY <= y2; loopY++)
		for(loopX = x1; loopX <= x2; loopX++)
		{
			gUnit = gUnitMap[loopY][loopX];
			if(!gUnit)
				continue;
			pUnit = ADerefAs(Unit, gUnit);
			if(pUnit->player == side)
				unitCount++;
		}
	return(unitCount);
}

// load unit statistics from *.UNI (reads only #UNITLIB section, not #UNITS section)
bool
UnitLib::Load(void)
{
	uint32		loop1;
	UnitGroup	*pUnitGroup;
	char			string1[256], *configLine;
	MapInfoRes	*pMapInfo;

	pMapInfo = (MapInfoRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_MAP_INFO, pWorld->mapResNum));

	configLine = &pMapInfo->configLines[0][0];
	for(loop1 = 0;	loop1 < pMapInfo->totalConfigLines; loop1++)
	{
		if(!memcmp(configLine, "#UNITLIB", 8))
			break;
		configLine += CONFIG_LINE_MAX_LEN;
	}

	configLine += CONFIG_LINE_MAX_LEN * 2;

	for(loop1 = 1; loop1 < UNITMAXTYPES; loop1++)
	{
		strcpy(string1, configLine);
		if(string1[0] != '#' || string1[1] != ' ')
		{
			break;
		}
		strupr(string1);

		TRACK_MEM("UnitGroup");		pUnitGroup = new UnitGroup;
		gUnitGroups[loop1] = pUnitGroup->gSelf;
		pUnitGroup->Init((unitType)loop1, string1);

		configLine += CONFIG_LINE_MAX_LEN;
	}
	return(TRUE);
}

void									// add a unit to the master unit list
UnitLib::AddUnit(grip gUnit, bool fPrevSerialNum)
{
	Unit	*pUnit;
	List	*pList;
	int	side;

	pUnit = ADerefAs(Unit, gUnit);
	side = pUnit->player;
	pList = &lUnits[side];

	// if unit had no previous serial number, assign one
	if(!fPrevSerialNum)
		pUnit->serialNum = pWorld->GetNextSerialNum(UNIT, pUnit->gSelf);

	pList->Add((void *)gUnit);
}

void									// cut a unit from the master unit list
UnitLib::CutUnit(grip gUnit, bool fLeaveSerialNum)
{
	Unit	*pUnit;
	List	*pList;
	int	side;

	pUnit = ADerefAs(Unit, gUnit);
	side = pUnit->player;
	pList = &lUnits[side];

	// if unit doesnt wish to retain serial number
	if(!fLeaveSerialNum)
		pWorld->FreeSerialNum(pUnit->serialNum);

	pList->Delete((void *)gUnit);
}

void									// delete an alloc'ed unit, and cut him from master List
UnitLib::DeleteUnit(grip gUnit)
{
	Unit	*pUnit;
	pUnit = ADerefAs(Unit, gUnit);
	pUnit->Cleanup();
	ADelete(gUnit);									// de-alloc unit
}

grip				// alloc a new unit and add to master list, and set up all parameters
UnitLib::NewUnit(unitType newType, uint32 newSide, uint32 newX, uint32 newY, itemType newItem,
	action newAction)
{
	grip	gUnit;
	Unit	*pUnit;

	gUnit = NewUnit();
	pUnit = ADerefAs(Unit, gUnit);

	pUnit->SetType(newType);
	
	pUnit->SetSide(newSide);

	// set clipping to viewport dimensions
	pUnit->SetTilePos(newX, newY);
	pUnit->AcquireItem(newItem);
	pUnit->RunAnimation(ST_ANIM_WALK, 0, FALSE, pUnit->currDir);
	pUnit->SetPrimaryAction(newAction);
//	pUnit->Cycle();		// cause first cel of animation to display

	pWorld->vPort.AddAni(gUnit);
	pWorld->vPort.CheckAni(gUnit);

	return(gUnit);
}

grip				// alloc a new unit and add to master list
UnitLib::NewUnit(void)
{
	Unit	*pUnit;
//	BAM_Ani	*pAni;
//	grip		gAni;
	
	TRACK_MEM("Unit");		pUnit = new Unit;												// create a new unit
	AddUnit(pUnit->gSelf);

	pUnit->SetLimits(0, 0, TILE_WIDTH * VPORT_WIDTH - 1, TILE_HEIGHT * VPORT_HEIGHT - 1);

	// create ani for terrain visual effects, but with null settings
//	TRACK_MEM("TerrainAni");	gAni = pWorld->vPort.NewAni(RES_ANIM,
//			NULL, NULL, PRI_WALKER + 2, FALSE, FALSE, 0, 0, 0, TRUE);
//	pUnit->gTerrainAni = gAni;
//	pAni = ADerefAs(BAM_Ani, gAni);
//	pAni->selfType = TERRAIN_ANI;			
//	pAni->fAniHideWhenDone = TRUE;
	return(pUnit->gSelf);
}

void
UnitLib::Report(void)
{
/*	uint32		loop1, loop2;
	UnitGroup	*pUnitGroup;

	for(loop1 = 0; loop1 < UNITMAXTYPES; loop1++)
		if(UnitGroups[loop1])
		{
			pUnitGroup = ADerefAs(UnitGroup, UnitGroups[loop1]);
			pMono->Out("UnitGroup[%d]:%s Type%02d Player%02d TileX%02d TileY%02d\n", loop1, pUnitGroup->unitName, pUnitGroup->);
			pMono->Out("Movement rates (base rate==%d)\n", pUnitGroup->baseMoveRate);
			for(loop2 = 0; loop2 < pWorld->pTileLib->totalTerrains; loop2++)

		}*/
}

//----------------------------------------------
// Projectile class (something fired by a unit)

Projectile::Projectile(Unit *pNewOwner)
{
	targetSerial = NULL;
	ownerSerial = pNewOwner->serialNum;
	ownerType = pNewOwner->type;
	priorityLevel = PRI_PROJECTILE;
	baseAnim = pNewOwner->baseAnim;
}

Projectile::~Projectile()
{
}

bool
Projectile::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&projectileDataStart, 1, (int)&projectileDataEnd -
				(int)&projectileDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&projectileDataStart, 1, (int)&projectileDataEnd -
				(int)&projectileDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
Projectile::Launch(uint32 newTargetSerial, int32 targX, int32 targY, int32 targXOff, int32 targYOff,
	int32 newStepX, int32 newStepY, int32 newDelay, int32 newDamage)
{
	grip	gNewTarget;
	Unit	*pUnit;

	launchX = tileX;
	launchY = tileY;
	damage = newDamage;
//	pMono->Out("Proj(g%d x%d y%d)::Launch(s%d x%d y%d xo%d yo%d sx%d sy%d)\n",
//		launchX, launchY, newTargetSerial, targX, targY, targXOff, targYOff, newStepX, newStepY);

	SetContext(pWorld->gSelf);
	pWorld->vPort.AddAni(gSelf);
	pWorld->vPort.CheckAni(gSelf);

	// fireball will travel to specified X,Y but upon impact if gTarget==(valid unit) then
	// unit's x,y will be used for impact location (in case a target unit crosses a tile boundary
	// while projectile is traveling)
	if(!newTargetSerial)
	{
		gNewTarget = pWorld->unitLib.GetUnitGrip(targX, targY);
		if(gNewTarget)
		{
			pUnit = ADerefAs(Unit, gNewTarget);
			targetSerial = pUnit->serialNum;
		}
	}
	else
		targetSerial = newTargetSerial;

	MoveAnim(targX, targY, targXOff, targYOff, newStepX, newStepY, newDelay);
}

void
Projectile::SmartCycle(bool fEssentialOnly)
{
	int32		impactX, impactY;
	Unit		*pOwner, *pUnit;
	grip		gAni;
	void		*pTarget;
	TargetType targ;
	MapSpace	*pBldg;

	if(fEssentialOnly);

	#ifdef DEBUG_BIG_NET
	pMono->Out("Projectile(g%d)::Cycle()\n", gSelf);
	#endif

	BAM_Ani::SmartCycle(FALSE);

	// if travel complete
	if(!fInMotion)
	{
		fDeleteMe = TRUE;

		pTarget = (void *)(pWorld->ValidateTarget((uint32)targetSerial, (TargetType *)&targ));
		pUnit = (Unit *)pTarget;
		pBldg = (MapSpace *)pTarget;

		if(pTarget && targ == UNIT)
		{
			impactX = pUnit->tileX;
			impactY = pUnit->tileY;
		}
		else
		{
			impactX = motionDestX;
			impactY = motionDestY;
		}

		pOwner = (Unit *)(pWorld->ValidateTarget(ownerSerial));
		// may be NULL at this point, but that's ok

		switch(ownerType)
		{
			case WYRM:
			case JUGGERNAUT:
			case GUARDIAN:
				unitType	damageType;

				if(ownerType == GUARDIAN)
					damageType = UNIT_ARCH_FIRE;
				else damageType = UNIT_FIRE;

				TRACK_MEM("ExplodeAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, baseAnim + ANIM_IMPACT, 1, PRI_SKY,
					TRUE, FALSE, 4, impactX, impactY, FALSE);	// boom!
				pWorld->vPort.CheckAni(gAni);
				pWorld->LaunchSound(SND_LAVA, impactX, impactY);

				if(targ == UNIT)
				{
					if(!pUnit)
						APanic("Projectile::Cycle() - case WYRM pUnit == NULL!\n");
					else pUnit->TakeAHit(pOwner, damageType, launchX, launchY, damage, (bool)(ownerType != GUARDIAN));
				}
				else if(targ == BUILDING)
				{
					if(!pBldg)
						APanic("Projectile::Cycle() - case WYRM pBldg == NULL!\n");
					else pBldg->TakeAHit(pOwner, damageType, launchX, launchY, damage, impactX, impactY);
				}

				if(ownerType == GUARDIAN)
				{
					TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_ARCH_FIRE, impactX, impactY);
				}
				else
				{
					TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_FIRE, impactX, impactY);
				}
				break;

			case RANGER:
			case GOBLIN:
				pWorld->LaunchSound(SND_WOODEN_SMACK, impactX, impactY);
				// fall-thru

			case WIZARD:
			default:
				if(targ == UNIT)
				{
					if(!pUnit)
						APanic("Projectile::Cycle() - case WIZARD pUnit == NULL!\n");
					pUnit->TakeAHit(pOwner, ownerType, launchX, launchY, damage, FALSE);
				}
				else if(targ == BUILDING)
				{
					if(!pUnit)
						APanic("Projectile::Cycle() - case WIZARD pUnit == NULL!\n");
					pBldg->TakeAHit(pOwner, ownerType, launchX, launchY, damage, impactX, impactY);
				}
				break;
		}
	}
	#ifdef DEBUG_BIG_NET
	pMono->Out("Projectile()::Cycle() done\n");
	#endif
}

void
Projectile::RunAnimation(int32 newFPS, bool fNewCycle, int32 baseAnim, direction newDir, bool fNewFwd)
{
	struct AnimInfo *pNewInfo;
	int32	aniNum, aniDelay;

	pNewInfo = &animInfo[pendingAnims];
 	pNewInfo->state = ST_ANIM_NULL;
	pNewInfo->fHFlip = FALSE;
	pNewInfo->fVFlip = FALSE;
	pNewInfo->fFwd = TRUE;
	pNewInfo->fCycle = TRUE;
	pNewInfo->resNum = 0;
	pNewInfo->speed = 0;
 
	aniNum = baseAnim + ANIM_PROJECTILE;
	aniDelay = TICKS_PER_SEC / newFPS;

	// certain directions are flips of other directions
	switch(newDir)
	{
		case UP_RIGHT:
			newDir = DOWN_RIGHT;
			pNewInfo->fVFlip = TRUE;
			break;
		case UP:
			newDir = DOWN;
			pNewInfo->fVFlip = TRUE;
			break;
		case UP_LEFT:
			newDir = DOWN_RIGHT;
			pNewInfo->fHFlip = TRUE;
			pNewInfo->fVFlip = TRUE;
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
	aniNum += newDir;

	pNewInfo->resNum = aniNum;
	pNewInfo->state = ST_ANIM_NULL;
	pNewInfo->speed = aniDelay;
	pNewInfo->fCycle = fNewCycle;
	pNewInfo->fFwd = fNewFwd;

	pendingAnims++;
	BAM_Ani::SmartCycle(FALSE);
}

