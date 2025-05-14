//
// March 27, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
//	AI for Computer controlled units.
//
//----[]-------------------------------------------------------------

#include "stdarg.h"
#include "eventmgr.hpp"
#include "debug.hpp"

#include "ai.hpp"
#include "world.hpp"

#ifdef NDEBUG
//#undef NDEBUG
#endif

extern void			MouseHandler( int draw_mouse );
extern Debugger	BamDebug;
extern char		pVerbNames[][30];
extern char 	szFuncNames[][20];
char				szMissionNames[AI_MAX][30] =
{"AINONE", "AIGUARD", "AIATTUNIT", "AIATTBLDG", "AITRANSFORM", "AIDEFUNIT",
"AIDEFBLDG", "AITRANSFOUND", "AIHEAL", "AIGOTOHOT", "AIGETITEM", "AIMANA",
"AISUMMON", "AIRETREAT"};

// direction indexes
//
//     0 1 2
//		  \|/
//		 7- -3
//		  /|\
//		 6 5 4

int	offsets[8][2] =
{
	-1,-1,
	 0,-1,
	 1,-1,
	 1, 0,
	 1, 1,
	 0, 1,
	-1, 1,
	-1, 0
};

void ClearAIMission(Unit *pUnit)
{
	pUnit->ai.action = AI_NONE;
	pUnit->ai.data = 0;
}

static FILE *pDebugAI = NULL;
char globalDebugString[160];
void AILog(int line, char *pFile, char *fmt, ...)
{
	va_list args;
  	char msg[256];

	sprintf(msg, "%5d %8s ", line, pFile);
	strcpy(globalDebugString, msg);
	strncpy(globalDebugString + 15, fmt, sizeof(globalDebugString) - 16);
	globalDebugString[sizeof(globalDebugString) - 1] = NULL;

	BamDebug.Put(fmt);	// BUGBUG - temp stack-crash catcher

	va_start (args, fmt);
	#ifndef NDEBUG
	vsprintf (msg, fmt, args);
	#endif
	va_end (args);

	#ifndef NDEBUG
	if(!pDebugAI)
	{
		pDebugAI = fopen("debug.ai", "w");
	}
	fwrite(msg, strlen(msg), 1, pDebugAI);
	#endif
}

int AiDistanceCompare(void const *pparm1, void const *pparm2);

AI::AI()
{
	int	loop1;

	// clear the timer array
	memset(nextAITimer, 0, sizeof(nextAITimer));

	memset(cheatChances, NULL, sizeof(cheatChances));
	cheatChances[AICHEAT_TRANSFORM_NEED_FIGHTERS] = 25;	// default
	cheatChances[AICHEAT_RECYCLE_NEED_MANA] = 20;	// default
	cheatChances[AICHEAT_SUMMON_NEED_MANA] = 33;	// default
	cheatChances[AICHEAT_AGGRSKILL_NEED_MANA] = 50;	// default

	eAggression = 99;
	eWits = 99;
	eCaution = 99;
	eTerrainIQ = 99;
	eItemIQ = 99;
	eSpellIQ = 99;

	memset(totalTransformPrefs, NULL, sizeof(totalTransformPrefs));
	memset(transformPrefs, NULL, sizeof(transformPrefs));
	memset(nextAITimer, NULL, sizeof(nextAITimer));
	memset(delayedActionCount, NULL, sizeof(delayedActionCount));

	for(loop1 = 0;	loop1 < TOTAL_SIDES; loop1++)
	{
		aiTable[loop1].portalX = 0;
		aiTable[loop1].portalY = 0;
		attackNow[loop1] = FALSE;
		attackTimer[loop1] = 0;
	}
}

AI::~AI()
{
	
}

// lItems - Item list

// convert a building function type to one of our ai enums.
// if no enum, return -1
int
AI::BldgFuncToAIEnum(funcType bldgFunc)
{
	int		bldgType = -1;
	
	switch(bldgFunc)
	{
		case FUNC_GEN_FOUND:
			bldgType = AIT_FOUNDATION;
			break;

		case FUNC_PORTAL:
			bldgType = AIT_PORTAL;
			break;

		case FUNC_ARBORLODGE:
//		case FUNC_ARBOR_FOUND:
			bldgType = AIT_ARBOR;
			break;

		case FUNC_BARRACKS:
//		case FUNC_BARRACKS_FOUND:
			bldgType = AIT_BARRACKS;
			break;

		case FUNC_CRYPT:
//		case FUNC_CRYPT_FOUND:
			bldgType = AIT_CRYPT;
			break;

		case FUNC_RUNESTONE:
//		case FUNC_RUNE_FOUND:
			bldgType = AIT_RUNESTONE;
			break;

		case FUNC_TEMPLE:
//		case FUNC_TEMPLE_FOUND:
			bldgType = AIT_TEMPLE;
			break;

		case FUNC_CAULDRON:
			bldgType = AIT_CAULDRON;
			break;
	}

	return bldgType;
}

void
AI::Setup(int side)
{
	grip		gTarget;
	MapSpace	*pBldg;
	int		loop1, int1, count;

	AILog(__LINE__, __FILE__, "AI::Setup(%d)\n", side);

	// pre-set these to possibly-inaccurate, but safe, defaults
	friendlySide = side;
	enemySide = (side == SIDE1)? SIDE2: SIDE1;

	if(bGlobal.aiOveride)
	{
		SetAggression(bGlobal.aiOveride);
		SetWits(bGlobal.aiOveride);
		SetCaution(bGlobal.aiOveride);
		SetTerrainIQ(bGlobal.aiOveride);
		SetItemIQ(bGlobal.aiOveride);
		SetSpellIQ(bGlobal.aiOveride);
	}

	// setup the aiTables w/ safe defaults
	aiTable[side].absMinAcolytes = 3;
	aiTable[side].minAcolytes = aiTable[side].absMinAcolytes + 1;
	aiTable[side].manaReserve = MANA_SUMMON;
	aiTable[side].minFighters = 3;

	aiTable[side].portalX = pWorld->map.portalCoords[side][0];
	aiTable[side].portalY = pWorld->map.portalCoords[side][1];

	if(!totalTransformPrefs[side])
	{
		// failsafe - if no transform prefs assigned, just go by the bldgs
		// we have.
		gTarget = (grip) pWorld->map.lBuildings[side].FirstValue();
		while(gTarget)
		{
			pBldg = ADerefAs(MapSpace, gTarget);
			// is this a transformer building of ours?
			if(pBldg->h.owner == side && pBldg->IsSpecialBuilding())
			{
				// add its associated unit types to our transform prefs
				int 		rank = 0;
				unitType type1;

				type1 = pBldg->GetRelatedUnitType(rank);
				while(type1)
				{
					if(pWorld->map.IsTypeAllowed(type1, side))
						AddUnitPref(side, type1, 20);
					rank++;
					type1 = pBldg->GetRelatedUnitType(rank);
				}
			}
			gTarget = (grip)pWorld->map.lBuildings[side].NextValue();
		}
	}

	// randomly pick a few other transform prefs to add to the mix
	if(pWorld->map.fLegendary)
	{
		// add 3 more prefs
		for(loop1 = 0; loop1 < 3;)
		{
			int1 = ARandom((int)ZOMBIE) + 1;
			if(pWorld->map.IsTypeAllowed((unitType)int1, side))
			{
				AddUnitPref(side, (unitType)int1, 20);
				loop1++;
			}
		}
	}

	if(totalTransformPrefs[side] < 1)
	{
		// still no transformation prefs? - make some defaults then
		AddUnitPref(side, CLERIC, 20);
		AddUnitPref(side, RANGER, 20);
		AddUnitPref(side, WARRIOR,20);
		AddUnitPref(side, WIZARD, 20);
		AddUnitPref(side, ZOMBIE, 20);
	}

	// refresh hot spot array for everyone
	hotSpotCount = 0;
	count = AMin(AI_MAX_GUARD, pWorld->map.totalHotSpots);
	for(loop1 = 0; loop1 < count; loop1++)
	{
		gTarget = pWorld->map.gHotSpots[loop1];

		pBldg = ADerefAs(MapSpace, gTarget);

		if(pWorld->map.GetStructure(pBldg->h.xPos, pBldg->h.yPos) != pBldg)
		{
			// forget it, struct is old news.
			continue;
		}
			
		hotSpotArray[loop1].gUnit = pBldg->gSelf;
		hotSpotArray[loop1].x = pBldg->h.xPos;
		hotSpotArray[loop1].y = pBldg->h.yPos;
		hotSpotCount++;
	}

	// failsafe in case activation fails
	nextAITimer[side] = 0;

	delayedActionCount[side] = 0;

	// only do AI every once in a while
	timerInterval = TICKS_PER_SEC * 8;

	attackNow[side] = FALSE;
	attackTimer[side] = ATicks() + ((100 - eAggression) / 5) * (TICKS_PER_SEC * 60);

	#ifndef NDEBUG
	char	string1[80];
	sprintf(string1, "%d:Ag%d Wi%d Ca%d Te%d It%d Sp%d\n", pWorld->map.rMapNum,
		eAggression, eWits, eCaution, eTerrainIQ, eItemIQ, eSpellIQ);
	pWorld->MsgBoxAdd(string1, pWorld->map.portalCoords[SIDE2][0], pWorld->map.portalCoords[SIDE2][1]);
	pWorld->MsgBoxDraw();
	#endif
}

// Find if an action is in the action array.  Only find
// ones that are specific (not ALL_UNITS).
// If notthing is found, return NULL

ai_action_struct*
AI::FindAction(ai_action aiAction)
{
	int	i;
	ai_action_struct	*action = NULL;

	for(i = 0; i < actionCount && !action; i++)
	{
		if (actions[i].action == aiAction)
		{
			switch(actions[i].action)
			{
				case AI_ATTACK_UNIT:
				case AI_DEFEND_UNIT:
					if (actions[i].actionSpec != UNITMAXTYPES)
					{
						action = &actions[i];
					}
					break;

				case AI_ATTACK_BUILDING:
				case AI_DEFEND_BUILDING:
					if (actions[i].actionSpec != TOTALFUNCTIONS)
					{
						action = &actions[i];
					}
					break;
			}
		}
	}

	return action;
}

void
AI::DeleteAction(int actionNum)
{
	int	loop1;

	if(actionNum >= actionCount)
	{
		// error, # too high
		return;
	}

	for(loop1 = actionNum; loop1 < (actionCount - 1); loop1++)
		memcpy((void *)&actions[loop1], (void *)&actions[loop1 + 1], sizeof(actions[0]));
	actionCount--;
}

// add an action to action array.  chance is 0 (never) to 100 (always)
void
AI::AddAIAction(ai_action aiAction, int actionSpec, int sideSpec, int chance)
{
	int	loop1;
	ai_action_struct	*pAIArray;

	switch(aiAction)
	{
		case AI_SUMMON:
			// prevent unnecessary duplicates
			for(loop1 = 0, pAIArray = &actions[0]; loop1 < actionCount; loop1++, pAIArray++)
				if(pAIArray->action == AI_SUMMON)
					return;
			break;
	}

	if (actionCount < MAX_ACTIONS)
	{
		actions[actionCount].action = aiAction;
		actions[actionCount].actionSpec = actionSpec;
		actions[actionCount].sideSpec = sideSpec;

		// make sure that chance is in range
		if (chance < 0)
		{
			chance = 0;
		}
		else
		{
			if (chance > 100)
			{
				chance = 100;
			}
		}
		actions[actionCount].chance = chance;
		actionCount++;
	}
}

// take all the chances in the action array and make them all
// exist in one range so that we only have to call random once.

void
AI::FixActionPercents()
{
	int	total = 0;
	int	i;
	int	previousChance = 0;
	int	newChance;

	if (actionCount)
	{
		// total all the chances
		for(i = 0; i < actionCount; i++)
		{
			total += actions[i].chance;
		}

		// now fill in the computedChance
		for(i = 0; i < actionCount; i++)
		{
			newChance = (actions[i].chance * 100)/total;
			previousChance = actions[i].computedChance = previousChance + newChance;
		}

		// make sure that the last one is 100 
		actions[actionCount - 1].computedChance = 100;
	}
}

// return the index to the action picked.
// NOTE - FixActionPercents should be called before this routine is called.
int
AI::PickAction(Unit *pSubject)
{
	int	chance;
	int	i;

	chance = ARandom(100) + 1;

	for(i = 0; i < actionCount; i++)
 	{
		if (actions[i].computedChance >= chance)
		{
			if(actions[i].action == AI_TRANSFORM && pSubject && pSubject->type != ACOLYTE)
				;	// not allowed - continue searching
					// BUGBUG - This assumes that there are other actions available!
			else
			{
				// this is the one
				break;
			}
		}
	}

	if (i == actionCount)
	{
		// sorry kid, we dont need ya
		return(-1);

//		APanic("AI Action out of range");
	}

	return i;
}

void
AI::ReportUnit(Unit *pUnit, char *pString, bool fLog)
{
	#ifndef NDEBUG
	char	string1[256];

	sprintf(string1, "AI:%s%dc%s@%d/%d p%s %s@%d/%d %s %s %sm%s:%d\n",
		pUnit->unitName, pUnit->serialNum, pVerbNames[pUnit->currAction],
		pUnit->tileX, pUnit->tileY, pVerbNames[pUnit->primaryAction],
		(pUnit->primaryTargetType == UNIT)? "UNIT": ((pUnit->primaryTargetType == BUILDING)? "BLDG": "TERR"),
		pUnit->primaryX, pUnit->primaryY, 
		pUnit->fPrimarySatisfied? "fpDONE": "fpBUSY",
		pUnit->fCursorTracking? "TRACK": "", pString,
		szMissionNames[pUnit->ai.action], pUnit->ai.data);
	if(fLog)
		AILog(__LINE__, __FILE__, string1);
	pMono->Out(string1);
	#endif
}

// several funcs need access to these, but they do NOT need saveGame protection
void
AI::Cycle(int side)
{
	ticks_t	currTicks = ATicks();

	if (currTicks < nextAITimer[side])
	{
		// not time to do ai yet.
		return;
	}

	AILog(__LINE__, __FILE__, "AI::Cycle(%d) @ %d ticks\n", side, currTicks);


	#ifdef DEBUG_BIG_NET
	pMono->Out("AI::Cycle(%d) - ticks%d\n", side, currTicks);
	#endif

	// if first cycle
	if(nextAITimer[side] == 0)
	{
		// put it off a sec, let the game start quickly
		nextAITimer[side] = currTicks + TICKS_PER_SEC * 2;
		return;
	}

  	AILog(__LINE__, __FILE__, "\nAI::Cycle(t%d) Wits=%d Caut=%d Aggr=%d Spel=%d Terr=%d Item=%d\n",
		currTicks, eWits, eCaution, eAggression, eSpellIQ, eTerrainIQ, eItemIQ);
	AILog(__LINE__, __FILE__, "attNow=%s attTime=%d\n", attackNow[side]? "TRUE": "FALSE", attackTimer[side] - currTicks);

	// setup the timer for next time to do ai
	// value (side) is added so multiple-ai games will not have both ai::cycle()'s
	// occuring on same tick
	nextAITimer[side] = currTicks + timerInterval + side;

	if (attackNow[side] == FALSE)
	{
		// check timer
		if (currTicks >= attackTimer[side])
		{
			// time to attack!
			attackNow[side] = TRUE;	
		}
	}

	int		dActionCount, i, builders;
	Unit		*pUnit, *pEUnit, *pBuilder;
	grip		gUnit;
	int		otherSide;
	List		*plUnits, *pList1;
	MapSpace	*pBldg, *pFoundationToBuildOn;
	bool		useMasonMix, useGnomes,	useHealSpells, useItems;
	ticks_t	recentAttackTicks;
//	bool		defendJuggernaut = FALSE;
	ai_delay_action	*delayAction;
	int		loop1, int1;	// unitX, unitY
//	TargetType	targetType;

	recentAttackTicks = currTicks - (timerInterval * 2);
	aiTable[side].manaReserve = MANA_SUMMON;	// may be over-ridden later

	if (side == SIDE1)
	{
		otherSide = SIDE2;
	}
	else
	{
		otherSide = SIDE1;
	}

	// these two are avail to all member funcs
	friendlySide = side;
	enemySide = otherSide;

	guardCount = 0;
	healCount = 0;
	repairCount = 0;
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		aiTable[loop1].currFighters = 0;
		aiTable[loop1].currAcolytes = 0;
	}

	plUnits = &pWorld->unitLib.lUnits[side];
//	eUnitsList	= &pWorld->unitLib.lUnits[otherSide];
  
	actionCount = 0;

	// -------------------------------------------
	// do some preprocessing

	int acsTransforming = 0, acsBuilding = 0, acsBusy = 0;
	pFoundationToBuildOn = NULL;
	builders = 0;
	pBuilder = NULL;
	gUnit = (grip)plUnits->FirstValue();
	while(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
		gUnit = (grip) plUnits->NextValue();

		// count our guarding acolytes
		if(pUnit->type == ACOLYTE)
		{
			// is he trying to make a building?
			if(pUnit->ai.action == AI_TRANS_FOUNDATION)
			{
				// try to count how many helpers he already has
				pBldg = pWorld->map.GetStructure(pUnit->primaryX, pUnit->primaryY);
				if(pFoundationToBuildOn == pBldg)
				{
					builders++;
				}
				else
				{
					pFoundationToBuildOn = pBldg;
					builders = 1;
					pBuilder = pUnit;	// remember him for later, in case he needs reinforcements
				}
				acsBuilding++;
				// ReportUnit(pUnit, "BUILDING");
			}
//			else if(pUnit->currAction == TRANSFORM || (pUnit->ai.action == AI_TRANSFORM && ARandom(2)))
			else if(pUnit->currAction == TRANSFORM)
			{
				// ReportUnit(pUnit, "TRNSFRMNG");
				acsTransforming++;
				// trying to transform
			}
//			else if(!pUnit->fPrimarySatisfied)
			else if(pUnit->currAction != GUARD)
			{
				ReportUnit(pUnit, "BUSY:REPORT");
				acsBusy++;
				// whatever he's doing, we cant count on him for mana
			}
			else
			{
				// ReportUnit(pUnit, "AVAIL");
				// we only count the rest, because those other guys are unreliable
				aiTable[side].currAcolytes++;
			}
		}
		else
		{
			if(pUnit->type == ENCHANTER)
			{
				// as long as we have an enchanter around, we want to aim for
				// more mana
				aiTable[side].manaReserve = 60;	// may be over-ridden later
			}

			if(pUnit->currHP < 6)
			{
				// unit is almost dead, and in need of recycling (if we can
				// heal him before he gets recycled, then he'll be saved)

				// if unit is not en-route for recycling
				if(!pUnit->fAutoControl && pUnit->ai.action != AI_TRANSFORM && CheckWits())
				{
					// Note: we do this during preprocessing so he wont get assigned to do something else

					// if not already at the portal.  Dont mess w/ AI_RETREAT
					// condition because that flag keeps him from trying to defend
					// himself.  He'll already be enroute to the portal anyway.
					if(pUnit->nearbyTransformer == FUNC_PORTAL || pUnit->ai.action != AI_RETREAT)
					{
						// get ye to a nunnery!
						pUnit->SetPrimaryAction(MOVE_TO, pWorld->map.portalCoords[side][0], 
							pWorld->map.portalCoords[side][1]);
						pUnit->ai.action = AI_TRANSFORM;
						pUnit->ai.data = (uint32)ACOLYTE;
						AILog(__LINE__, __FILE__, "AI Prep: Unit almost dead: recycle@portal\n");
						ReportUnit(pUnit, "HURT:RECYCLE", TRUE);
					}
				}

				// preparing for re-cycle.  don't add him to avail list
				continue;
			}

			switch(pUnit->primaryAction)
			{
				case ATTACK:
				case SKILL:
					break;

				default:
					if(pUnit->fAutoControl)
						break;
					aiTable[side].currFighters++;

					if(pUnit->type == FURY)
						attackNow[friendlySide] = TRUE;
				break;
			}
		}

		switch(pUnit->type)
		{
			case CLERIC:
			case PALADIN:
				if(!useHealSpells)
					break;

				if(pUnit->fAutoControl)
					break;
				if(pUnit->currAction == GUARD || pUnit->currAction == ATTACK || pUnit->currAction == MOVE_TO)
					AddToHealArray(pUnit);
				break;

			case HARPY:
				if(pUnit->item != HARPY_NET || pUnit->primaryAction == DROP)
					break;

				// got one!  go dunk him!
				AILog(__LINE__, __FILE__, "AI Prep: HARPY has VICTIM, choosing dump site\n");
				pBldg = FindClosestBldg(FUNC_CAULDRON, pUnit->player, pUnit->tileX, pUnit->tileY);
				if(pBldg)
				{
					// we have a cauldron. 
					// move to the cauldron and drop
					pUnit->SetPrimaryAction(DROP, pBldg->h.xPos, pBldg->h.yPos);
					ClearAIMission(pUnit);
					ReportUnit(pUnit, "DROPNET@CAULD", TRUE);
				}
				else
				{
					// no cauldron.
					// move to nearest chasm or water and drop.

					int tempX, tempY;
					if(pWorld->map.FindNearestTerrain(pUnit->tileX, pUnit->tileY,
						&tempX, &tempY, WATER, CHASM))
					{
						pUnit->SetPrimaryAction(DROP, tempX, tempY);
						ClearAIMission(pUnit);
						ReportUnit(pUnit, "DROPNET@WATERORCHASM", TRUE);
					}
					else
					{
						pUnit->SetPrimaryAction(DROP, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
						ClearAIMission(pUnit);
						ReportUnit(pUnit, "DROPNET@RAND", TRUE);
					}
				}
				break;
		}

		if(!pUnit->fAutoControl && pUnit->currAction == GUARD)
		{
			if ((pUnit->type == GNOME && useGnomes) || (pUnit->item == MASON_MIX && useMasonMix))
			{
				// add to our array of units that can repair
				if (repairCount < AI_MAX_GUARD)
				{
					repairArray[repairCount].gUnit = pUnit->gSelf;
					repairArray[repairCount].type = pUnit->type;
					repairArray[repairCount].x = pUnit->tileX;
					repairArray[repairCount].y = pUnit->tileY;
					repairArray[repairCount].item = pUnit->item;
					repairCount++;
				}
			}

			if(pUnit->type == HARPY && pUnit->item == HARPY_NET)
			{
				// leave him alone - handled already
			}
			else switch(pUnit->ai.action)
			{
				case AI_RETREAT:
					AILog(__LINE__, __FILE__, "AI Prep: AI_RETREAT satisfied, clearing mission\n");
					ClearAIMission(pUnit);
					ReportUnit(pUnit, "CLR_AIRETREAT", TRUE);
					break;

				case AI_GUARD:
					if(currTicks > pUnit->ai.data)
					{
						// we have guarded long enough
						AILog(__LINE__, __FILE__, "AI Prep: AI_GUARD satisfied && timer expired, clearing mission\n");
						ClearAIMission(pUnit);
						ReportUnit(pUnit, "END_AIGUARD", TRUE);
					}
					else if(pUnit->ai.data == 0)
					{
						// we just arrived somewhere.
						// set timer for guard time
						AILog(__LINE__, __FILE__, "AI Prep: AI_GUARD satisfied, setting timer\n");
						pUnit->ai.data = currTicks + (TICKS_PER_SEC * 60) + ARandom(TICKS_PER_SEC * 60);
						ReportUnit(pUnit, "START_AIGUARD", TRUE);
					}
					// fall-thru

				case AI_GOTO_HOT_SPOT:
					// add to our array of guarding guys, ready for action.
					AILog(__LINE__, __FILE__, "AI Prep: AI_HOTSPOT satisfied, unit avail\n");
					ReportUnit(pUnit, "HOTSPOT:DONE", TRUE);
					AddToGuardArray(pUnit);
					break;

				case AI_NONE:
					// add to our array of guarding guys, ready for action.
					AILog(__LINE__, __FILE__, "AI Prep: AI_NONE satisfied, unit avail\n");
					ReportUnit(pUnit, "AINONE:DONE", TRUE);
					if(!pUnit->fStandGuard)		
						AddToGuardArray(pUnit);
					break;

				case AI_GET_ITEM:
					// add to our array of guarding guys, ready for action.
					AILog(__LINE__, __FILE__, "AI Prep: AI_GET_ITEM satisfied, unit avail\n");
					ReportUnit(pUnit, "GETITEM:DONE", TRUE);
					AddToGuardArray(pUnit);
					break;

				case AI_DEFEND_BUILDING:
				case AI_DEFEND_UNIT:
					AILog(__LINE__, __FILE__, "AI Prep: DEFEND_BUILDING/UNIT satisfied\n");

					if(pUnit->currAction != ATTACK && pUnit->primaryAction != ATTACK)
					{
						AILog(__LINE__, __FILE__, ".. unit available\n");
						ReportUnit(pUnit, "DEF_BLDG/UNIT:DONE/AVAIL", TRUE);
						AddToGuardArray(pUnit);
					}
					else
					{
						AILog(__LINE__, __FILE__, ".. unit busy though\n");
						ReportUnit(pUnit, "DEF_BLDG/UNIT:ATTACKING", TRUE);
					}
					break;
//				case AI_ATTACK_UNIT:
//					pEUnit = (Unit *) pWorld->DerefSerial(pUnit->ai.data);
//					if (pEUnit)
//					{
//						// why has he stopped attacking?  Well, restart it.
//						pMono->Out("AI@%d: %s(%d,%d) RESUME ATTACK on %s(%d,%d)\n", currTicks,
//							pUnit->unitName, pUnit->tileX, pUnit->tileY,
//							pEUnit->unitName,	pEUnit->tileX, pEUnit->tileY);
//						pUnit->SetPrimaryAction(ATTACK, pEUnit->tileX, pEUnit->tileY);
//					}
//					pUnit->ai.action = AI_NONE;
//					break;
//
//				case AI_ATTACK_BUILDING:
//					pEBldg = (MapSpace *) pWorld->DerefSerial(pUnit->ai.data);
//					if (pEBldg)
//					{
//						// why has he stopped attacking?  Well, restart it.
//						pMono->Out("AI@%d: %s(%d,%d) RESUME ATTACK on bldg(%d,%d)\n", currTicks,
//							pUnit->unitName, pUnit->tileX, pUnit->tileY, pUnit->ai.x, pUnit->ai.y);
//						pUnit->SetPrimaryAction(ATTACK, pUnit->ai.x, pUnit->ai.y);
//					}
//					pUnit->ai.action = AI_NONE;
//					break;
			}
		}
	}

	// count # of acolytes on enemy side
	pList1 = &pWorld->unitLib.lUnits[otherSide];
	gUnit = (grip)pList1->FirstValue();
	while(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
		if(pUnit->type == ACOLYTE)
			aiTable[otherSide].currAcolytes++;

		gUnit = (grip)pList1->NextValue();
	}

	// absolute minimum, do not dip into this reserve
	aiTable[side].absMinAcolytes = eWits / 25 + 1;

	if(bGlobal.storyLine != SHOW_OFF)
	{
		// set our " comfortable minimum" as (caution formula) or close to what
		// the enemy has, whichever is higher
		aiTable[side].minAcolytes = AMax((aiTable[otherSide].currAcolytes * 75) / 100,
			(eCaution / 25) + 1);
	}
	// else it's AI vs. AI, so don't over-escalate

	// for consistency's sake..
	aiTable[side].minAcolytes = AMax(aiTable[side].minAcolytes, aiTable[side].absMinAcolytes + 2);
	// comfortable minimum MUST be >=absolute+1 to insure excess acolyte(s) are available

	// even if we have excess mana, don't get carried away
	aiTable[side].maxAcolytes = AMax(aiTable[side].minAcolytes + 1, aiTable[otherSide].currAcolytes * (eAggression / 25));

	// comfortable minimum # of fighting units - dont send any out for offense otherwise
	aiTable[side].minFighters = eCaution / 7 + 1;

	AILog(__LINE__, __FILE__, "AI::Cycle(%d) ATicks()==%d mana=%d absMinAc=%d comfMinAc=%d minFig=%d curFig=%d\n",
		side, currTicks, pWorld->GetManaLevel(side), aiTable[side].absMinAcolytes, aiTable[side].minAcolytes,
		aiTable[side].minFighters, aiTable[side].currFighters);

	// so, did we find any acolyte(s) trying to make a building, but dont have enough help?
	if(pFoundationToBuildOn && builders < 4)
	{
		// send help
		AILog(__LINE__, __FILE__, "AI:FOUND team incomplete, need help\n");
		AddAIAction(AI_TRANS_FOUNDATION, pBuilder->ai.data, side, 100);
	}

	loop1 = 0;
	int chanceCounter = 0, chance = ARandom(100) + 1;
	if((CheckWits() || aiTable[side].currFighters < aiTable[side].minFighters) && totalTransformPrefs[side] > 0)
	{
		// we want to transform a unit - pick a type
		do
		{
			// if array end somehow reached
			if(loop1 >= totalTransformPrefs[side])
			{
				// just randomly pick one
				int1 = ARandom(totalTransformPrefs[side]);
				AddAIAction(AI_TRANSFORM, (unitType)transformPrefs[side][int1][0], side, 100);
				chance = 0;
			}
			else
			{
				chanceCounter += transformPrefs[side][loop1][1];
				if(chance <= chanceCounter)
				{
					AddAIAction(AI_TRANSFORM, (unitType)transformPrefs[side][loop1][0], side, 100);
					chance = 0;
				}
			}
			loop1++;
		} while(chance);
	}

	useMasonMix = CheckItemIQ();
	useGnomes = CheckSpellIQ();
	useHealSpells = CheckSpellIQ();
	useItems = CheckItemIQ();

	dActionCount = delayedActionCount[side];

	// when we are done here, the list will be clear. 
	delayedActionCount[side] = 0;

	// process the list of delayed actions
	for (i= 0; i < dActionCount; i++)
	{
		delayAction = &delayedActions[side][i];
		if (delayAction->gotHitType == ADT_UNIT)
		{
			// must be unit attacking unit
			// validate the serial numbers
			pUnit = (Unit *) pWorld->DerefSerial(delayAction->gotHitSerialNum);
			pEUnit = (Unit *) pWorld->DerefSerial(delayAction->hitBySerialNum);

			if(pUnit)
			{
				// do we need healing?
				if(pUnit->currHP < pUnit->maxHP)
				{
					// we aren't doing so good.

					switch(pUnit->type)
					{
						case JUGGERNAUT:
			 			case GUARDIAN:
						case GOLEM:
							HelpRepair(NULL, pUnit);
							break;

						default:
//							pMono->Out("AI Delayed: %s(%d,%d) needs HEALING\n", pUnit->unitName, pUnit->tileX, pUnit->tileY);
//							HelpHealUnit(pUnit);	
							break;
					}
				}
			}

			if (pEUnit)
			{
				// ignore the juggernaut
				if (pEUnit->type != JUGGERNAUT)
				{
					// attack this sucker
					AttackUnit(pEUnit, 10);
				}
			}
		}
		else
		{
			// must be unit attacking building			
			// validate the serial numbers
			pBldg = (MapSpace *) pWorld->DerefSerial(delayAction->gotHitSerialNum);
			pEUnit = (Unit *) pWorld->DerefSerial(delayAction->hitBySerialNum);

			if (pBldg)
			{
				// see if it needs to be repaired
				if (pBldg->h.currHP < (pBldg->h.maxHP >> 1))
				{
					// we have lost 50% of the hit points.  try to repair
					HelpRepair(pBldg);
				}
				AddAIAction(AI_DEFEND_BUILDING, pBldg->h.func, side, 100);
			}

			if (pEUnit)
			{
				// ignore the juggernaut
				if (pEUnit->type != JUGGERNAUT)
				{
					// attack this sucker
//					AttackUnit(pEUnit, 10);
//					AttackUnit(pEUnit);
				}
			}
		}
	}

	// defensive
//	BUGBUG - what good is the AI_GUARD mission?
//	AddAIAction(AI_GUARD, 0, side, 50);

	if(CheckWits())
		AddAIAction(AI_GATHER_MANA, UNITMAXTYPES, side, 100);

	// offensive

	if(attackNow[side])
	{
		AddAIAction(AI_ATTACK_UNIT, ENCHANTER, otherSide, 100);
		AddAIAction(AI_ATTACK_UNIT, HARPY, otherSide, 100);
		AddAIAction(AI_ATTACK_UNIT, ACOLYTE, otherSide, 100);
	}

	if(attackNow[side])
	{
		AddAIAction(AI_ATTACK_BUILDING, TOTALFUNCTIONS, otherSide, 100);
	}

	if(attackNow[side])
	{
		AddAIAction(AI_ATTACK_UNIT, UNITMAXTYPES, otherSide, 100);
	}
 
	// examine all win/lose conditions, and maybe act on those too
	MonitorWinCons(side);
	
	if(CheckCaution())
		AddAIAction(AI_DEFEND_BUILDING, TOTALFUNCTIONS, side, 100);

	if(CheckTerrainIQ())
		AddAIAction(AI_GOTO_HOT_SPOT, UNITMAXTYPES, side, 60);

	if(CheckItemIQ())
		AddAIAction(AI_GET_ITEM, ENDOFITEMTYPES, side, 40);

	AILog(__LINE__, __FILE__, "AI: Acolytes=%d (trans=%d build=%d busy=%d)\n",
		aiTable[side].currAcolytes, acsTransforming, acsBuilding, acsBusy);

	// if below our comfortable minimum # of acolytes
	if(aiTable[side].currAcolytes <= aiTable[side].minAcolytes && CheckWits() && CheckWits())
	{
		AILog(__LINE__, __FILE__, "AI: ACOLYTE count(%d) below comfMin(%d)\n", aiTable[side].currAcolytes,
			aiTable[side].minAcolytes);
		AddAIAction(AI_SUMMON, ACOLYTE, side, 100);
	}
	// else do we have excess mana?
	else if(pWorld->mana[side] >=	40 + (eCaution * 4))
	{
		AILog(__LINE__, __FILE__, "AI: excess mana, ");

		if(aiTable[side].currAcolytes >= aiTable[side].maxAcolytes)
		{
			AILog(__LINE__, __FILE__, "but max acolytes\n");
		}
		else
		{
			AILog(__LINE__, __FILE__, "SUMMON ACOLYTE\n");
			AddAIAction(AI_SUMMON, ACOLYTE, side, 50);
		}
	}

	// okay, here's where we start writing AI code THE WAY IT SHOULD HAVE BEEN HANDLED!!!
	ExamineAllUnitsImportant();

	MouseHandler(FALSE);

	if(actionCount)
	{
		// now that all the actions are in place, fix the percentages for selection
		FixActionPercents();
		ProcessMissionPool(side);
		MouseHandler(FALSE);
	}

	ExamineAllUnits();

	MouseHandler(FALSE);

	if(attackNow[side])
	{
		// delay further aggression until later
		AILog(__LINE__, __FILE__, "AI: End of AGGRESSION phase\n");
		attackNow[side] = FALSE;
		attackTimer[side] = ATicks() + ((100 - eAggression) / 5) * (TICKS_PER_SEC * 60);
	}

#ifdef DEBUG_BIG_NET
	pMono->Out("AI::Cycle(%d) - done @ ticks%d\n", side, currTicks);
#endif
}

// some actions need to occur before ProcessMissionPool(), to make sure they
// get first pick of avail resources
void
AI::ExamineAllUnitsImportant(void)
{
	ticks_t currTicks = ATicks();
	grip	gUnit;
	List	*plUnits = &pWorld->unitLib.lUnits[friendlySide];
	Unit	*pUnit;
	unitType	unit2, unit1;
	MapSpace	*pBldg;
	int	loop1, loop2, loopX;

	gUnit = (grip)plUnits->FirstValue();
	while(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
		gUnit = (grip)plUnits->NextValue();

		// don't mess with units that are under auto control
		if(pUnit->fAutoControl)
			continue;

		if(pUnit->fPrimarySatisfied)
		{
			switch(pUnit->ai.action)
			{
				case AI_TRANSFORM:
					// is he able to transform now?
					AILog(__LINE__, __FILE__, "AI Ex:AI_TRANSFORM satisfied..");

					if(pUnit->type == ACOLYTE)
					{
						if(pUnit->ai.data == (uint32)ACOLYTE)
						{
							// this guy already did his transform.
							ReportUnit(pUnit, "ERR:ALREADY DONE", TRUE);
							ClearAIMission(pUnit);
							break;
						}
					}
					else if(pUnit->ai.data != (uint32)ACOLYTE)
					{
						// this guy already did his transform.
						ReportUnit(pUnit, "ERR:ALREADY DONE", TRUE);
						ClearAIMission(pUnit);
						break;
					}

					if(pUnit->type == ACOLYTE)
					{
						// if attacked in last 3 seconds
						if(currTicks - pUnit->lastAttackTicks < TICKS_PER_SEC * 3)
						{
							// dont worry about checking other things
						}
						else
						{
							// if currently short on acolytes, wait..
							if(aiTable[pUnit->player].currAcolytes < aiTable[pUnit->player].minAcolytes - 1)
							{
								// if some acolytes, but short on fighters
								if((aiTable[pUnit->player].currAcolytes > aiTable[pUnit->player].minAcolytes / 2) &&
									aiTable[pUnit->player].currFighters < (aiTable[pUnit->player].minFighters / 2) &&
									CheckCaution() && CheckCaution())
									;	// dont skip TRANSFORM, cuz we need military too
								else
								{
									// skip TRANSFORM, wait til another replacement is made (to keep the mana flowing)
									AILog(__LINE__, __FILE__, "Waiting for more acolytes\n");
									AddAIAction(AI_SUMMON, ACOLYTE, friendlySide, 100);
									continue;
								}
							}

							// if acolyte has mana to transfer, do that first
							if(pUnit->currMana > 3 && CheckSpellIQ() && CheckSpellIQ() && !CheckAggression())
							{
								pUnit->SetAction(SKILL);
								ReportUnit(pUnit, "TRANSFER MANA", TRUE);
								continue;
							}
						}
					}
					else
					{
						// unit is being recycled
						// has he since been fixed?
						if(pUnit->currHP > pUnit->maxHP / 3)
						{
							// nevermind, wait for a healer to continue healing
							ReportUnit(pUnit, "cancel, WAIT4HEAL", TRUE);
							ClearAIMission(pUnit);
							pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
							continue;
						}
					}

					// if low on mana, but we need fighters..
					if(pWorld->mana[pUnit->player] < MANA_SUMMON && aiTable[friendlySide].currFighters <
						aiTable[friendlySide].minFighters && CheckWits() &&
							CheckCheat(AICHEAT_TRANSFORM_NEED_FIGHTERS))
					{
						// cheat!
						pWorld->mana[pUnit->player] = MANA_SUMMON;
					}

					if(pUnit->type == ACOLYTE)
					{
						// verify that our destination bldg is still standing
						if(!pUnit->nearbyTransformer)
						{
							pBldg = pWorld->map.GetStructure(pUnit->primaryX, pUnit->primaryY);
							if(!pBldg || !pBldg->IsSpecialBuilding() || pBldg->h.owner != friendlySide)
							{
								ReportUnit(pUnit, "ERR:BLDG GONE", TRUE);
								// I guess the building was destroyed.  Abort mission.
								ClearAIMission(pUnit);
								continue;
							}
						}
					}

					if (pUnit->Transform((unitType)(pUnit->ai.data)))
					{
						// we've begun the transformation
						ReportUnit(pUnit, "TRANS:START", TRUE);
						aiTable[pUnit->player].currAcolytes--;
						continue;
					}

					ReportUnit(pUnit, "ERR:UNKNOWN", TRUE);
					ClearAIMission(pUnit);
					// inexplicable failure.  Forget it, release unit.
					break;

				case AI_TRANS_FOUNDATION:
					AILog(__LINE__, __FILE__, "AI Ex:AI_TRANS_FOUND satisfied..");
					if(pUnit->currTerrain != FOUND_GENERIC)
					{
						ReportUnit(pUnit, "ERR:NO_FOUND", TRUE);
						break;
					}

					pBldg = pWorld->map.GetStructure(pUnit->tileX, pUnit->tileY);
					if(pBldg && pBldg->h.func == FUNC_GEN_FOUND)	// failsafe
					{
						// we are here, waiting to transform
						if (pBldg->CheckFoundationFilled())
						{
							// the foundation is filled.					
							if(pBldg->BuildFromFoundation((funcType)pUnit->ai.data))
							{
								// build successful!
								ReportUnit(pUnit, "BUILD:DONE", TRUE);
								continue;
							}
						}
					}
					ReportUnit(pUnit, "ERR:UNKNOWN", TRUE);
					ClearAIMission(pUnit);
					break;
			}// end of switch()
		}// end of if(fPrimarySatisfied)
	
		switch(pUnit->currAction)
		{
			case ATTACK:
				if(pUnit->type == ACOLYTE && pUnit->nearbyTransformer &&
					CheckWits() && CheckWits())
				{
					// we're near a transformer, and we're being beat on.
					// try to pull a fast one

					pUnit->FindNearbyTransformer(&pBldg);

					if(!pBldg)
					{
						// um - what?  Whatever.
						pUnit->CheckForTransformer();
						break;
					}

					// find a transformation type that matches the bldg we're near
					for(loop1 = 0, loop2 = ARandom(totalTransformPrefs[pUnit->player]);
						loop1 < totalTransformPrefs[pUnit->player]; loop1++,
						loop2 = (++loop2 % totalTransformPrefs[pUnit->player]))
					{
						unit1 = (unitType)transformPrefs[pUnit->player][loop2][0];
						loopX = 0;
						do
						{
							unit2 = pBldg->GetRelatedUnitType(loopX++);
						}
						while(unit2 && unit2 != unit1);
						if(unit2)
						{
							if(pWorld->mana[pUnit->player] < MANA_SUMMON &&
								CheckCheat(AICHEAT_TRANSFORM_NEED_FIGHTERS))
									pWorld->mana[pUnit->player] = MANA_SUMMON;

							pUnit->Transform(unit2);
							break;
						}
					}
				}
				break;
		}
	}// end of while(gUnit)
}

// see how everyone is doing, and if they need a hand
void
AI::ExamineAllUnits(void)
{
	grip	gUnit;
	bool	busy, aiDone, fSneak, fMove;
	Unit	*pUnit, *pUnit2, *pEUnit, *pEUnit2;
	unitType	unit1;
	MapSpace	*pBldg, *pEBldg;
	ticks_t currTicks = ATicks();
	int	tempX, tempY, loopX, loopY, loop1, int1, int2;
	TerrainUnit	*pTUnit;
	List	*plUnits = &pWorld->unitLib.lUnits[friendlySide];

	gUnit = (grip)plUnits->FirstValue();
	aiDone = FALSE;
	while(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
		gUnit = (grip)plUnits->NextValue();
		busy = FALSE;

		// if wounded (and more than just a scratch)
		if(pUnit->maxHP - pUnit->currHP >= 5)
		{
			// unit has taken damage, determine course of action

			// if almost dead, and very recently attacked
			if (pUnit->currHP <= 4 && CheckWits() && !pUnit->fAutoControl &&
				(currTicks - pUnit->lastAttackTicks) < TICKS_PER_SEC * 2)
			{
				// this guy is too far gone.  disband for mana
				ReportUnit(pUnit, "AI Ex2: WASTED:DISBAND", TRUE);
				pUnit->SetPrimaryAction(DISBAND);
				continue;
			}
			else
			{
				AILog(__LINE__, __FILE__, "AI: %s(%d,%d) calling for HEALER\n", pUnit->unitName, pUnit->tileX, pUnit->tileY);
				HelpHealUnit(pUnit);	
			}
		}

		// don't mess with units that are under auto control
		if(pUnit->fAutoControl)
			continue;

		// inspect unit's mission - do we have another step for him?
		if(pUnit->fPrimarySatisfied)
		{
			switch(pUnit->ai.action)
			{
				case AI_TRANSFORM:
				case AI_TRANS_FOUNDATION:
					// do nothing here, next mission step is assigned elsewhere
					continue;

				case AI_GOTO_HOT_SPOT:
					AILog(__LINE__, __FILE__, "AI Ex2: GOTO_HOTSPOT satisfied..");
					ReportUnit(pUnit, "GOT_HOTSPOT:DONE", TRUE);
					ClearAIMission(pUnit);

					continue;

				case AI_DEFEND_BUILDING:
					// just chill
					ReportUnit(pUnit, "DEF_BLDG:DONE", TRUE);
					// dont clear mission - bldg may get attacked at any moment
//					ClearAIMission(pUnit);
					continue;

				case AI_ATTACK_UNIT:
					// okay, so is the unit dead?
					AILog(__LINE__, __FILE__, "AI Ex2: ATTACK_UNIT satisfied..");
					if(!pUnit->ai.data)
					{
						// umm, whatever.  Clean up.
						ReportUnit(pUnit, "ERR:NO_TARG", TRUE);
						ClearAIMission(pUnit);
						continue;
					}
					pEUnit = (Unit *)pWorld->DerefSerial(pUnit->ai.data);
					if(!pEUnit || !pEUnit->currHP)
					{
						// well done!
						ReportUnit(pUnit, "DONE", TRUE);
						ClearAIMission(pUnit);
						continue;
					}
				
					// is target unit still known to unitLib?
					if(!pWorld->unitLib.lUnits[pEUnit->player].Find((void *)pEUnit->gSelf))
					{
						// serial # is valid, but unit grip is unlisted.  Maybe target got
						// harpy-netted.  Forget him.
						ReportUnit(pUnit, "ERR:!IN_UNITLIB", TRUE);
						ClearAIMission(pUnit);
						continue;
					}

					// target is still valid, so why is he fPrimarySatisfied?
					if(pUnit->primaryAction == MOVE_TO)
					{
						// must have been told to stop at a waypoint.
						// Is target unit moving or recently attacked?  If so, forget
						// waypoints and rush him!
						if(pEUnit->currAction == MOVE_TO || pEUnit->lastAttackTicks >
							(currTicks - TICKS_PER_SEC * 3))
						{
							pUnit->SetPrimaryAction(ATTACK, pEUnit->tileX, pEUnit->tileY);
							ReportUnit(pUnit, "RUSH", TRUE);
							continue;
						}

						// okay, target unit is still stationary
						if(FindWayPoint(pUnit, pEUnit->tileX, pEUnit->tileY, &tempX, &tempY))
						{
							pUnit->SetPrimaryAction(MOVE_TO, tempX, tempY);
							ReportUnit(pUnit, "SNEAK2NEXT", TRUE);
							continue;
						}
						// no way-points available - is everyone else ready to attack?
						if(IsTeamReady(pUnit))
						{
							pUnit->SetPrimaryAction(ATTACK, pEUnit->tileX, pEUnit->tileY);
							ReportUnit(pUnit, "ATTACK!", TRUE);
							continue;
						}
						else
						{
							ReportUnit(pUnit, "WAIT4TEAM", TRUE);
							continue;
						}
					}
					else if(pUnit->primaryAction == SKILL)
					{
						ReportUnit(pUnit, "ERR:SKILL DONE?", TRUE);
						ClearAIMission(pUnit);
						// skill complete - what next?
						break;
					}

					ReportUnit(pUnit, "ERR:UNKNOWN", TRUE);
					break;

				case AI_ATTACK_BUILDING:
					// okay, so is the building dead?
					AILog(__LINE__, __FILE__, "AI Ex2: ATTACK_BLDG satisfied..");
					if(!pUnit->ai.data)
					{
						// umm, whatever.  Clean up.
						ReportUnit(pUnit, "ERR:NO_TARG", TRUE);
						ClearAIMission(pUnit);
						continue;
					}
					pBldg = (MapSpace *)pWorld->DerefSerial(pUnit->ai.data);
					if(!pBldg || !pBldg->h.currHP)
					{
						// well done!
						ReportUnit(pUnit, "DONE", TRUE);
						ClearAIMission(pUnit);
						continue;
					}
				
					// okay, so why is he stopped?
					if(pUnit->primaryAction == MOVE_TO)
					{
						// unit stopped out-of-range, must have been travling to a way point.

						fSneak = TRUE;
						// is our target already under attack?
						if(pBldg->lastAttackTicks > currTicks - TICKS_PER_SEC * 10)
						{
							// no point in trying to be sneaky now
							pUnit->SetPrimaryAction(ATTACK, pBldg->h.xPos, pBldg->h.yPos);
							ReportUnit(pUnit, "RUSH", TRUE);
							continue;
						}
					
						if(FindWayPoint(pUnit, pBldg->h.xPos, pBldg->h.yPos, &tempX, &tempY))
						{
							pUnit->SetPrimaryAction(MOVE_TO, tempX, tempY);
							ReportUnit(pUnit, "SNEAK2NEXT", TRUE);
							continue;
						}

						// no way-points available - is everyone else ready to attack?
						if(IsTeamReady(pUnit))
						{
							pUnit->SetPrimaryAction(ATTACK, pBldg->h.xPos, pBldg->h.yPos);
							ReportUnit(pUnit, "ATTACK!", TRUE);
							continue;
						}
						else
						{
							#ifndef NDEBUG
							pMono->Out("AI:ATTACK_BLDG - Waiting for team member(s)\n");
							#endif
							continue;
						}
					}

					ReportUnit(pUnit, "ERR:UNKNOWN", TRUE);
					break;
			}

			// end of fPrimarySatisfied analysis
			// Since fPrimarySatisfied, and we got this far, then unit
			// does not have a next step to follow, so force termination
			// of mission.

			ReportUnit(pUnit, "FPRIMS:CLEAR_AI", TRUE);
			ClearAIMission(pUnit);
		}
		else
		{
			// fPrimarySatisfied = FALSE, he's busy doing something

			switch(pUnit->ai.action)
			{
				case AI_TRANS_FOUNDATION:
					// is he still needed?
					pBldg = pWorld->map.GetStructure(pUnit->primaryX, pUnit->primaryY);
					if(!pBldg || pBldg->h.func != FUNC_GEN_FOUND)
					{
						// whatever happened, the foundation is gone.  Might as well quit.
						AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND working..");
						ReportUnit(pUnit, "ERR:FOUND_GONE", TRUE);
						ClearAIMission(pUnit);
						pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
						continue;
					}
					break;

				case AI_TRANSFORM:
					// is he able to transform now?
					if(pUnit->type != ACOLYTE && pUnit->ai.data != (uint32)ACOLYTE)
					{
						// this guy already did his transform.
						AILog(__LINE__, __FILE__, "AI Ex2: TRANSFORM working..");
						ReportUnit(pUnit, "ERR: ALREADY DONE", TRUE);
						pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
						ClearAIMission(pUnit);
						continue;
					}

					// if attempting to recycle unit..
					if(pUnit->type != ACOLYTE && pUnit->ai.data == ACOLYTE)					
					{
						if(pUnit->currAction == GUARD && pUnit->nearbyTransformer == FUNC_PORTAL)
						{
							if(pWorld->mana[pUnit->player] < MANA_SUMMON &&
								CheckCheat(AICHEAT_RECYCLE_NEED_MANA))
							{
								// cheat
								pWorld->mana[pUnit->player] = MANA_SUMMON;
							}

							AILog(__LINE__, __FILE__, "AI Ex2: TRANSFORM working..");
							if(pUnit->Transform((unitType)(pUnit->ai.data)))
							{
								// we've begun the transformation
								ClearAIMission(pUnit);
								ReportUnit(pUnit, "RECYCLE STARTED", TRUE);
								continue;
							}
							else
							{
								ReportUnit(pUnit, "RECYCLE FAILED", TRUE);
							}
						}
					}

					if(pUnit->currAction == GUARD && ATicks() - pUnit->tCurrActStarted > TICKS_PER_SEC * 90)
					{
						AILog(__LINE__, __FILE__, "AI Ex2: TRANSFORM working, gridlock..");
						// unit has been guarding for too long to accomplish something - give up.
						// in case of traffic gridlock, try this:
						pBldg = FindClosestBldg(TOTALFUNCTIONS, friendlySide,
							pUnit->tileX, pUnit->tileY);
						if(pBldg)
						{
							ReportUnit(pUnit, "GUARD NEAREST BLDG", TRUE);
							int1 = ARandom(pBldg->size);
							pUnit->tCurrActStarted = currTicks;	// just in case
							ClearAIMission(pUnit);
							pUnit->SetPrimaryAction(MOVE_TO, pBldg->tiles[int1][0],
								pBldg->tiles[int1][1]);
							continue;
						}
					}
					break;
			}

			switch(pUnit->currAction)
			{
				case GUARD:
					// he's sitting around, but he's not happy.
					// maybe we can solve his dilemma?
					if(pUnit->ai.action == AI_GOTO_HOT_SPOT)
					{
						// is he close enough?
						if(pUnit->GetDistanceTo(pUnit->primaryX, pUnit->primaryY) <= 2)
						{
							AILog(__LINE__, __FILE__, "AI Ex2: GOTO_HOTSPOT/GUARDING working..");
							// be happy where you are.
							pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileY, pUnit->tileX);
							ReportUnit(pUnit, "CLOSE ENOUGH", TRUE);
							continue;
						}
					}

					if(pUnit->ai.action == AI_TRANSFORM && pUnit->nearbyTransformer)
					{
						// he's next to some kind of building, is it the correct type?
						// maybe he just couldn't reach the exact X/Y he wanted.
						pUnit->FindNearbyTransformer(&pBldg);
						loop1 = 0;
						if(!pBldg)
						{
							// umm.. yeah, whatever.
							unit1 = NOUNIT;
							pUnit->CheckForTransformer();
						}
						else do
						{
							// try next unit type
							unit1 = pBldg->GetRelatedUnitType(loop1++);
							if(unit1 == pUnit->ai.data || pBldg->h.func == FUNC_IMM_WELL)
							{
								// that's the right one - stop here
								AILog(__LINE__, __FILE__, "AI Ex2: TRANSFORM ?/GUARD..");
								ReportUnit(pUnit, "SETTLE", TRUE);
								pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
								break;
							}
						}
						while(unit1);

						if(pUnit->primaryAction == MOVE_TO && pUnit->primaryX == pUnit->tileX &&
							pUnit->primaryY == pUnit->tileY)
						{
							// it worked!  no more AI for him
							continue;
						}
					}

					// maybe we need some traffic mgmt?
					if(ARandom(2) == 0 && CheckWits() && CheckWits())
					{
						pUnit->curPathPosition = 0;
						pUnit->pathLength = pUnit->FindPathTo(pUnit->primaryX, pUnit->primaryY, pUnit->pathArray,
							pUnit->fStopAdjacent, TRUE, FALSE, NULL, NULL, TRUE);

						// if a better path is avail, it's possibly thru friendlies.  Look for
						// them and bump them out of our way
						int1 = AMin(5, pUnit->pathLength);
						for(loop1 = 0; loop1 < int1; loop1++)
						{
							pUnit->PeekNextTarget(tempX, tempY);
							pUnit->curPathPosition++;
							pUnit2 = pWorld->unitLib.GetUnit(tempX, tempY);

							// if a unit (anyone's unit) isn't found on the 1st tile,
							// dont worry about it - it wont help us.
							if(!pUnit2)
							{
								if(loop1 == 0)
									break;
								else continue;
							}

							if(pUnit2->player == pUnit->player && (pUnit2->currAction == GUARD || pUnit2->currAction == NO_ACTION))
							{
								// get outta the way, stupid!
								if(BumpUnit(pUnit2))
								{
									// success!  wait and try again later
									break;
								}
							}
						}						
					}
					break;

				case ATTACK:
					// is there something special we can try?

					if(pUnit->item == BERSERKER_BREW && CheckItemIQ() &&
						pUnit->type != FURY && pUnit->type != GOLEM && pUnit->type != GUARDIAN &&
						pUnit->type != JUGGERNAUT && pUnit->type != TROLL && pUnit->type != WRAITH &&
						pUnit->type != ROC_EGG)
					{
						AILog(__LINE__, __FILE__, "AI Ex2: ?/ATTACKING working..");
						// beserk and kill!
						pUnit->UseItem();
						ReportUnit(pUnit, "BERSERK", TRUE);
						break;
					}

					// okay, if he's currently attacking, then WHAT is he attacking?
					switch(pUnit->secondaryTargetType)
					{
						case UNIT:
							pEUnit = (Unit *)pWorld->DerefSerial(pUnit->secondaryTargetSerial);
							pEBldg = NULL;
							break;

						case BUILDING:
							pEUnit = NULL;
							pEBldg = (MapSpace *)pWorld->DerefSerial(pUnit->secondaryTargetSerial);
							break;

						default:
							pEUnit = NULL;
							pEBldg = NULL;
							break;
					}

					// do we have a skill we can use to help the fight?
					switch(pUnit->type)
					{
						// this is done because I like being able to BREAK at multiple points
						default:
							if(pUnit->skillType != SKILL_AGGRESSIVE)
							{
								// nope, not a skill we can use here
								break;
							}

							if(pUnit->skillCost > pWorld->GetManaLevel(friendlySide))
							{
								// nope, too expensive
								break;
							}

							if(!CheckSpellIQ())
							{
								// nope, AI is too stupid this turn
								break;
							}									

							// is target a unit && immune to us?
							if(pEUnit && pEUnit->ImmuneToSkillFrom(pUnit) && pUnit->skillRange <= 1)
							{
								// yep, no point
								break;
							}

							if(!ShouldWeUseSkill(pUnit))
							{
								// extra thinking says no - nevermind
								break;
							}

							// let's show our target a trick
							AILog(__LINE__, __FILE__, "AI Ex2: ?/ATTACKING working..");
							pUnit->SetAction(SKILL, pUnit->secondaryX, pUnit->secondaryY);
							ReportUnit(pUnit, "SKILL", TRUE);
							continue;
					}
					break;	// end of CASE ATTACK:
					
				default:
					// all other currActions here
					break;
			}// end of switch(currAction);

		}// end of fPrimarySatisfied = FALSE

		if(pUnit->type == NYMPH)
		{
			aiDone = FALSE;
			tempX = pUnit->tileX;
			tempY = pUnit->tileY;
			// is she near enemies already under her spell?
			for(loopY = tempY - 2; loopY <= tempY + 2 && !aiDone; loopY++)
				for(loopX = tempX - 2; loopX <= tempX + 2 && !aiDone; loopX++)
				{
					pEUnit = pWorld->unitLib.GetUnit(loopX, loopY);
					if(!pEUnit || pEUnit == pUnit)
					{
						continue;
					}

					if(pEUnit->primaryAction == FOLLOW && pEUnit->primaryTargetSerial == pUnit->serialNum)
					{
						// we have at least one admirer.. should we string him along?
						if(CheckWits())
						{
							// run for it!
							pUnit->SetPrimaryAction(MOVE_TO, ARandom(WORLD_WIDTH), ARandom(WORLD_HEIGHT));
							pUnit->ai.action = AI_RETREAT;
							AILog(__LINE__, __FILE__, "AI Ex2: NYMPH..");
							ReportUnit(pUnit, "LURE:RUNAWAY", TRUE);

							// call some friends for a party (heh heh!)
							AssignUnits(pUnit->player, MOVE_TO, pUnit->primaryX, pUnit->primaryY, AI_DEFEND_UNIT,
								pUnit->serialNum, eCaution / 10 + 2, TRUE, FALSE);
							aiDone = TRUE;
						}
					}
				}

			if(aiDone)
			{
				// she's running away, so leave her alone now
				continue;
			}
		}

		if(pUnit->currAction == GUARD)	// regardless of fPrimarySatisfied state
		{
			// well, he's just standing around.

			fMove = FALSE;
			// make sure he's not blocking our portal, or getting fried by the enemy portal
			if(pUnit->currTerrain == PORTAL)
			{
				// bad boy - step aside
				fMove = TRUE;
			}
			// or standing on a dangerous spot (like a moron)
			else if(pUnit->currTerrain == FLAME_SPOUT)
				fMove = TRUE;
			else
			{
				pTUnit = pWorld->vPort.GetTerrainUnit(pUnit->tileX, pUnit->tileY);
				if(pTUnit && (pTUnit->activeType == ACTIVE_FIRE ||
					pTUnit->activeType == ACTIVE_ARCH_FIRE))
					fMove = TRUE;
			}
				
			if(fMove)
			{
				AILog(__LINE__, __FILE__, "AI Ex2: ?/GUARD..");
				if(pUnit->currTerrain == PORTAL && pUnit->primaryAction == TRANSFORM)
				{
					// unit should only be moved adjacent to portal, not any further away
					tempX = pUnit->tileX + (ARandom(3) - 1);
					tempX = AMin(WORLD_WIDTH - 1, tempX);	tempX = AMax(0, tempX);
					tempY = pUnit->tileY + (ARandom(3) - 1);
					tempY = AMin(WORLD_HEIGHT - 1, tempY);	tempY = AMax(0, tempY);
				}
				else
				{
					tempX = pUnit->tileX + (ARandom(3) - 1) * 2;
					tempX = AMin(WORLD_WIDTH - 1, tempX);	tempX = AMax(0, tempX);
					tempY = pUnit->tileY + (ARandom(3) - 1) * 2;
					tempY = AMin(WORLD_HEIGHT - 1, tempY);	tempY = AMax(0, tempY);
				}

				if(pUnit->fPrimarySatisfied)
				{
					pUnit->SetPrimaryAction(MOVE_TO, tempX, tempY);
					ReportUnit(pUnit, "PRIM MOVE ASIDE", TRUE);

				}
				else
				{
					// dont override primary action
					pUnit->SetAction(MOVE_TO, tempX, tempY);
					ReportUnit(pUnit, "SEC MOVE ASIDE", TRUE);
				}
				continue;
			}

			// AI checkup on unit's completion of its mission, whether or not
			// it's currently moving
			switch(pUnit->primaryAction)
			{
				case MOVE_TO:
				case ATTACK:
					// this unit is trying to get somewhere, and has stopped.

					switch(pUnit->ai.action)
					{
						case AI_GET_ITEM:
							// is the item already gone?
							aiDone = FALSE;

							switch(pUnit->primaryTargetType)
							{
								case UNIT:
									// is he still packin'?
									pUnit2 = (Unit *)pWorld->DerefSerial(pUnit->primaryTargetSerial);
									if(pUnit2 && pUnit2->item)
										break;

									// okay, what if he dropped it?
									if(pUnit->CanIMoveTo(pUnit->primaryX, pUnit->primaryY))
									{
										if(!pWorld->itemMgr.CheckItem(pUnit->primaryX, pUnit->primaryY))
										{
											aiDone = TRUE;
											// didn't drop it - oh well
										}
										else
										{
											// go get the item he dropped
											pUnit->SetPrimaryAction(MOVE_TO, pUnit->primaryX, pUnit->primaryY);
										}
										break;
									}
									else
									{
										// wouldn't matter if he did, we cant go there.
										aiDone = TRUE;
									}
									break;

								case TERRAIN:
									if(!pWorld->itemMgr.CheckItem(pUnit->primaryX, pUnit->primaryY))
										aiDone = TRUE;
									break;

								case BUILDING:
									pBldg = pWorld->map.GetStructure(pUnit->primaryX,
										pUnit->primaryY);
									if(!pBldg || !pBldg->h.currHP)
									{
										// took it down - now get the item
										pUnit->SetPrimaryAction(MOVE_TO, pUnit->primaryX,
											pUnit->primaryY);
									}
									break;
							}

							if(aiDone)
							{
								// return to previous pos
								AILog(__LINE__, __FILE__, "AI Ex2: GET_ITEM ?/GUARD..");
								ClearAIMission(pUnit);
								pUnit->SetPrimaryAction(MOVE_TO, pUnit->ai.x, pUnit->ai.y);
								ReportUnit(pUnit, "GOHOME", TRUE);
								continue;
							}
							break;
					}

					if(pUnit->ai.action == AI_DEFEND_UNIT)
					{
						// is target unit still alive?
						pUnit2 = (Unit *)pWorld->DerefSerial(pUnit->ai.data);
						if(!pUnit2 || !pUnit2->currHP)
						{
							// nevermind
							AILog(__LINE__, __FILE__, "AI Ex2: DEF_UNIT ?/GUARD..");
							ReportUnit(pUnit, "LOST", TRUE);
							ClearAIMission(pUnit);
							pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
							continue;
						}

						// was target recently attacked?
						if(currTicks - pUnit2->lastAttackTicks < TICKS_PER_SEC * 3)
						{
							// get 'em!
							pEUnit = (Unit *)pWorld->DerefSerial(pUnit2->lastAttackerSerial);
							if(pEUnit)
							{
								AILog(__LINE__, __FILE__, "AI Ex2: DEF_UNIT ?/GUARD..");
								ReportUnit(pUnit, "DEFEND", TRUE);
								pUnit->SetPrimaryAction(MOVE_TO, pEUnit->tileX, pEUnit->tileY);
								continue;
							}
						}
							
						// else just stick by them
						if(pUnit->GetDistanceTo(pUnit2, UNIT) > 2)
						{
							AILog(__LINE__, __FILE__, "AI Ex2: DEF_UNIT ?/GUARD..");
							ReportUnit(pUnit, "FOLLOW", TRUE);
							pUnit->SetPrimaryAction(MOVE_TO, pUnit2->tileX, pUnit2->tileY);
							continue;
						}
						break;
					}

					if(!pUnit->WithinRangeForPrimary())
					{
						// this unit has gotten stopped along the way, and is waiting.
						// Let's see if we can help him.

						// is he trying to build a building?
						if(pUnit->ai.action == AI_TRANS_FOUNDATION)
						{
							// has the foundation already been transformed?
							pBldg = pWorld->map.GetStructure(pUnit->primaryX, pUnit->primaryY);
							if(pBldg->h.func != FUNC_GEN_FOUND)
							{
								AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND ?/GUARD..");
								// nevermind, they didnt need us.
								ReportUnit(pUnit, "FOUND GONE", TRUE);
								ClearAIMission(pUnit);
								pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
								continue;
							}

							// alright then, any enemies on the foundation?
							pEUnit = NULL;
							pEUnit2 = NULL;
							for(loop1 = 0; loop1 < pBldg->size; loop1++)
							{
								pEUnit = pWorld->unitLib.GetUnit(pBldg->tiles[loop1][0],
									pBldg->tiles[loop1][1]);
								if(pEUnit)
								{
									// is there some smart-ass enemy on our foundation?
									if(pUnit->DoIHate(pEUnit))
									{
										// move close enough to start a fight
										AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND ?/GUARD..");
										ReportUnit(pUnit, "ENEMY ON FOUND", TRUE);
										pUnit->SetAction(MOVE_TO, pEUnit->tileX, pEUnit->tileY);
										break;
									}
									// else is it another one of our units then?
									else if(pEUnit->player == pUnit->player)
									{
										if(pEUnit->type != ACOLYTE && pEUnit->currAction == GUARD)
										{
											// hmmm, what's he doing there?  make a note for later
											pEUnit2 = pEUnit;
										}
									}
								}
							}
							if(pEUnit)
							{
								// if an enemy was found, we're after him now so
								// no need to process further
								continue;
							}

							// okaaaay, maybe one of our non-acolytes is there? (big dummy)
							if(pEUnit2)
							{
								AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND ?/GUARD..");
								ReportUnit(pUnit, "MOVING FRIEND", TRUE);
								// if he's just standing around, move him aside and keep waiting
								if(CheckWits() && pEUnit2->IsTrapped())
								{
									// one sure way to fix a traffic jam!
									pEUnit2->SetPrimaryAction(DISBAND);
									ClearAIMission(pEUnit2);
									ReportUnit(pEUnit2, "", TRUE);
									continue;
								}
								else
								{
									tempX = pEUnit2->tileX + 2 * (ARandom(3) - 1);
									tempY = pEUnit2->tileY + 2 * (ARandom(3) - 1);
									pEUnit2->SetPrimaryAction(MOVE_TO, tempX, tempY);
									ClearAIMission(pEUnit2);
									ReportUnit(pEUnit2, "LEAVE FOUND", TRUE);
									continue;
								}
							}

							// is the exact tileX/Y he's trying to get to, occupied?
							if(pWorld->unitLib.GetUnitGrip(pUnit->primaryX, pUnit->primaryY))
							{
								// is he already on a different tile of the same foundation?
								if(pBldg == pWorld->map.GetStructure(pUnit->tileX, pUnit->tileY))
								{
									// tell him to stop worrying.
									AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND ?/GUARD..");
									pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
									ReportUnit(pUnit, "SETTLE", TRUE);
									continue;
								}

								// pick a different, empty tile - it doesnt matter.
								for(loop1 = 0; loop1 < pBldg->size; loop1++)
								{
									if(!pWorld->unitLib.GetUnitGrip(pBldg->tiles[loop1][0], pBldg->tiles[loop1][1]))
									{
										pUnit->SetPrimaryAction(MOVE_TO, pBldg->tiles[loop1][0],
											pBldg->tiles[loop1][1]);
										AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND ?/GUARD..");
										ReportUnit(pUnit, "MOVETO ALT", TRUE);
										continue;
									}
								}
							}
						}

						// assume no path currently open.  Any other ideas?

						// is unit trying to build?
						aiDone = FALSE;
						if(pUnit->type == ACOLYTE && pUnit->ai.action == AI_TRANS_FOUNDATION)
						{
							// are we real, real close already?
							pBldg = pWorld->map.GetStructure(pUnit->primaryX, pUnit->primaryY);
							if(pUnit->GetDistanceTo((void *)pBldg, BUILDING) <= 2)
							{
								// find a friendly unit adjacent to us, on the foundation,
								// and make him move out of our way!
								int2 = pBldg->size;
								int1 = ARandom(int2);
								pUnit2 = NULL;
								for(loop1 = 0; loop1 < int2 && !pUnit2; loop1++, int1 = (int1 + 1) % int2)
								{
									tempX = pBldg->tiles[int1][0];
									tempY = pBldg->tiles[int1][1];
									pUnit2 = pWorld->unitLib.GetUnit(tempX, tempY);
									if(!pUnit2 || pUnit2->player != pUnit->player || pUnit2->fAutoControl
										|| pUnit2->currAction != GUARD || pUnit->GetDistanceTo(tempX, tempY) > 1)
									{
										pUnit2 = NULL;
										continue;
									}

									// found a candidate
									break;
								}

								if(pUnit2)
								{
									// now find him a new spot
									int1 = ARandom(int2);
									for(loop1 = 0; loop1 < int2; loop1++, int1 = (int1 + 1) % int2)
									{
										tempX = pBldg->tiles[int1][0];
										tempY = pBldg->tiles[int1][1];
										if(pUnit2->CanIMoveTo(tempX, tempY, TRUE) &&
											(pUnit2->tileX != tempX || pUnit2->tileY != tempY))
										{
											// move him aside, to make room for us.
											// We're assuming here that the tile we're sending him
											// to, the first unit can not presently reach (traffic jam)
											AILog(__LINE__, __FILE__, "AI Ex2: TRANS_FOUND ?/GUARD..");
											ReportUnit(pUnit, "MOVING FRIEND", TRUE);

											pUnit2->SetPrimaryAction(MOVE_TO, tempX, tempY);
											ReportUnit(pUnit2, "MOVETO FOUND ALT", TRUE);
											aiDone = TRUE;
											break;
										}
									}
								}
							}
						}
						if(aiDone)
							continue;

						// Is a path available that goes
						// through a neutral or enemy building or unit? (path w/ BULLDOZER==TRUE)
						aiDone = FALSE;
						pUnit->curPathPosition = 0;
						pUnit->pathLength = pUnit->FindPathTo(pUnit->primaryX, pUnit->primaryY, pUnit->pathArray,
							pUnit->fStopAdjacent, FALSE, TRUE);
						if(pUnit->pathLength > 0)
						{
							// a better path is avail, but only in BULLDOZER mode.  Find the obstacle and attack it.
							do
							{
								pUnit->PeekNextTarget(tempX, tempY);
								pUnit->curPathPosition++;
								pEUnit = pWorld->unitLib.GetUnit(tempX, tempY);
								if(pEUnit)
								{
									// is this the unit blocking us?
									if(pEUnit->player != friendlySide && pEUnit->currAction != MOVE_TO)
									{
										AILog(__LINE__, __FILE__, "AI Ex2: ?/GUARD..");
										// go pick a fight
										if(pUnit->curPathPosition == 1)
										{
											// he's in our face already!
											pUnit->SetAction(ATTACK, tempX, tempY);
											ReportUnit(pUnit, "BULLD_UNIT::SEC", TRUE);
										}
										else
										{
											// dont throw away primary, we'll get there before periodic repath
											// (w/o BULLDOZER mode) interrupts us
											pUnit->SetAction(MOVE_TO, tempX, tempY);
											ReportUnit(pUnit, "BULLD_UNIT::SEC2", TRUE);
										}

										// call the homies too
										AILog(__LINE__, __FILE__, "AI: %s@(%d,%d) needs BULLDOZER thru %s@(%d,%d)\n", pUnit->unitName,
											pUnit->tileX, pUnit->tileY, pEUnit->unitName, tempX, tempY);
										AssignUnits(friendlySide, ATTACK, tempX, tempY, AI_NONE, NULL, eCaution / 25, TRUE);
										aiDone = TRUE;
										break;									
									}
								}
								else if((bool)(pEBldg = pWorld->map.GetStructure(tempX, tempY)) &&
									pUnit->DoIHate(pEBldg) && !pUnit->CanIMoveTo(tempX, tempY))
								{
									// a building, eh?  Take it down!
									if(pUnit->curPathPosition == 1)
									{
										pUnit->SetAction(ATTACK, tempX, tempY);
										ReportUnit(pUnit, "BULLD_BLDG::SEC", TRUE);
									}

									// send a whole wrecking crew
									AILog(__LINE__, __FILE__, "AI: %s@(%d,%d) needs BULLDOZER thru BLDG@(%d,%d)\n", pUnit->unitName,
										pUnit->tileX, pUnit->tileY, tempX, tempY);
									AssignUnits(friendlySide, ATTACK, tempX, tempY, AI_NONE, NULL, eCaution / 25, TRUE);
									aiDone = TRUE;
									break;
								}
							} while(pUnit->curPathPosition < pUnit->pathLength);
						}
						if(aiDone)
							continue;
					}
					break;
					// end of "case MOVE_TO:"

				default:
					break;

			}// end of switch(primaryAction)
		}
		else if(pUnit->currAction == ATTACK)
		{
			if(pUnit->secondaryTargetType == BUILDING)
			{
				pEBldg = (MapSpace *)pUnit->ValidateSecondaryTarget();
				if(pEBldg && pEBldg->h.owner == pUnit->player)
				{
					// bad!  Leave our buildings alone!
					if(pUnit->primaryTargetSerial == pEUnit->serialNum)
					{
						pUnit->SetPrimaryAction(GUARD);
					}
					else
					{
						pUnit->SetAction(GUARD);
					}
					continue;
				}
			}
			else if(pUnit->secondaryTargetType == UNIT)
			{
				pEUnit = (Unit *)pUnit->ValidateSecondaryTarget();
				if(pEUnit && pEUnit->type == JUGGERNAUT)
				{
					if(pWorld->map.rMapNum == 9330)
					{
						// bad!  Leave the JUGGERNAUT alone, it's technically on our side.
						// Just go somewhere random nearby.

						tempX = pUnit->tileX + (5 + ARandom(5)) * (ARandom(2)? 1: -1);
						tempY = pUnit->tileY + (5 + ARandom(5)) * (ARandom(2)? 1: -1);
						tempX = AMax(tempX, 0); tempX = AMin(tempX, WORLD_WIDTH - 1);
						tempY = AMax(tempY, 0); tempY = AMin(tempY, WORLD_HEIGHT - 1);
						pUnit->SetPrimaryAction(MOVE_TO, tempX, tempY);
						continue;
					}
				}
				if(pEUnit && pEUnit->player == pUnit->player)
				{
					// fighting amongst themselves?  Stop that!
					if(pUnit->primaryTargetSerial == pEUnit->serialNum)
					{
						pUnit->SetPrimaryAction(GUARD);
					}
					else
					{
						pUnit->SetAction(GUARD);
					}
				}
			}
		}
	}
	// dont forget structures!
}

bool
AI::ShouldWeUseSkill(Unit *pUnit, Unit *pTarget)
{
	Unit	*pUnit2, *pTarget2;
	int	tileX, tileY, loop1, tempX, tempY;
	grip	gUnit;

	switch(pUnit->type)
	{
		case CLERIC:
		case PALADIN:
			if(pWorld->mana[pUnit->player] < aiTable[pUnit->player].manaReserve)
			{
				if(!CheckSpellIQ())
					return(FALSE);
			}

			if(pTarget && pTarget->currHP > 15)
			{
				// not in dire need
				if(!CheckSpellIQ())
					return(FALSE);
			}
			return(TRUE);

		case ENCHANTER:
		case WYRM:
			if(pUnit->currHP < 15)
				return(TRUE);
			return(TRUE);

		case WRAITH:
			if(pUnit->currHP > 15)
			{
				// nah, dont need it yet.  Dont want to scare them away while we beat on them.
				return(FALSE);
			}
			return(TRUE);

		case HARPY:
			if(pUnit->item)
				return(FALSE);

			// make sure that no one other than our current target is attacking us
			if(pUnit->secondaryTargetType == UNIT)
				pTarget2 = (Unit *) pWorld->DerefSerial(pUnit->secondaryTargetSerial);
			else pTarget2 = NULL;

			tileX = pUnit->tileX;
			tileY = pUnit->tileY;
			for(loop1 = 0; loop1 < 8; loop1++)
			{
				tempX = tileX + offsets[loop1][0];
				tempY = tileY + offsets[loop1][1];
				gUnit = pWorld->unitLib.gUnitMap[tempY][tempX];
				if(!gUnit)
					continue;

				pUnit2 = ADerefAs(Unit, gUnit);
				if(pUnit2->secondaryTargetSerial != pUnit->serialNum)
				{
					// this is not attacking us, so he doesnt matter
					continue;
				}

				if(pUnit2->player == pUnit->player)
				{
					// friendly unit, dont worry
					continue;
				}

				if(pUnit2->serialNum == pUnit->secondaryTargetSerial)
				{
					// he's our target, ignore him
					continue;
				}

				if(pUnit2 == pUnit)
				{
					// ignore ourselves
					continue;
				}

				// this guy is a threat - dont bother trying to net (hey, that rhymes!)
				return(FALSE);
			}

			// all clear!
			return(TRUE);
	}

	return(TRUE);
}

void
AI::ProcessMissionPool(int side)
{
	int		loop1, loop2, distance, bestDistance, int1, int2, index,
					tempX, tempY;
	unitType	unit1;
	Unit		*pUnit, *pUnit2;
	ai_guard	*pGuardUnit, *pGuardUnit2;
	ai_action_struct	*pAction;
	List		*pList1;
	BAMItem	*pItem1;
	grip		gItem, gUnit, gBldg;
	MapSpace	*pBldg;
	ticks_t	currTicks = ATicks();
	funcType	bldgType;
	unitType	unitType1;
	action	action1;
	ai_action	aiAction1;
	UnitGroup	*pGroup;
	bool		fSneak;
	itemType	item1;

	// examine mission pool, and make some assignments
	for(loop1 = 0; loop1 < actionCount; loop1++)
	{
		pAction = &actions[loop1];
		if(ARandom(100) > pAction->chance)
		{
			// nevermind, not important enough this cycle
			continue;
		}

		// alright, what's the mission?
		switch(pAction->action)
		{
			case AI_GOTO_HOT_SPOT:
				AILog(__LINE__, __FILE__, "AI Proc: GOTO HOTSPOT..");
				if(!hotSpotCount)
				{
					AILog(__LINE__, __FILE__, "none in map!\n");
					// nevermind
					break;
				}

				// find an idle non-acolyte who isnt at a hotspot
				pGuardUnit = guardArray;
				pUnit = NULL;
				for(loop2 = 0; loop2 < guardCount; loop2++, pGuardUnit++)
				{
					if(!pGuardUnit->pUnit)
					{
						// this entry already re-assigned
						continue;
					}
					if(pGuardUnit->type == ACOLYTE)
					{
						// dont send ACOLYTEs
						continue;
					}

					if(pGuardUnit->pUnit->ai.action == AI_GOTO_HOT_SPOT)
					{
						// he's already at one
						continue;
					}

					pUnit = pGuardUnit->pUnit;
					break;
				}

				if(!pUnit)
				{
					AILog(__LINE__, __FILE__, "no units available!\n");
					break;
				}
 				AILog(__LINE__, __FILE__, "sending %s@(%d,%d), ", pUnit->unitName, pUnit->tileX, pUnit->tileY);

				// send this guy, pick an hotspot
				pGuardUnit2 = NULL;
				if(CheckCaution())
				{
					AILog(__LINE__, __FILE__, "CAUTION: want safe hotspot\n");

					// try to find one with friends near it first
					for(loop2 = 0; loop2 < hotSpotCount; loop2++)
					{
						pGuardUnit2 = &hotSpotArray[loop2];

						// make sure we're not already at this one
						if(AMax(abs(pUnit->tileX - pGuardUnit2->x),
							abs(pUnit->tileY - pGuardUnit2->y)) < 5)
						{
							// too close to where we already are
							continue;
						}

						int1 = pWorld->unitLib.CountUnits(pGuardUnit2->x, pGuardUnit2->y, side, 3);

						// if enough friendlies but not too many
						if(int1 >= 1 && int1 <= (eCaution / 33 + 1))
						{
							// okay, stop looking
							break;
						}
						else pGuardUnit2 = NULL;
					}
					if(!pGuardUnit2)
					{
						AILog(__LINE__, __FILE__, "  Cant find one!\n");
						break;
					}
					else
					{
						AILog(__LINE__, __FILE__, "found %d friends..", int1);
					}
				}
				if(!pGuardUnit2)
				{
					// pick any old hotspot
					pGuardUnit2 = &hotSpotArray[ARandom(hotSpotCount)];
				}

				pUnit->SetPrimaryAction(MOVE_TO, pGuardUnit2->x, pGuardUnit2->y);
				pUnit->ai.action = AI_GOTO_HOT_SPOT;
				pUnit->ai.data = 0;
				ReportUnit(pUnit, "HOTSPOT::START", TRUE);

				// remove unit from availability	
				pGuardUnit->pUnit = NULL;
				break;

			case AI_ATTACK_UNIT:
				// how many attackers do we want?
				if(!CheckCaution() || !CheckCaution() || !CheckCaution())
				{
					int1 = 1;
				}
				else
				{
					int1 = eCaution / 25 + 1;
				}

				unitType1 = (unitType)pAction->actionSpec;

				if(unitType1 == UNITMAXTYPES)
					pGroup = NULL;
				else pGroup = pWorld->unitLib.GetUnitGroup(unitType1);

				if(pAction->sideSpec == SIDE3 || unitType1 == JUGGERNAUT)
				{
					// leave these for the player to fight
					break;
				}
				
				AILog(__LINE__, __FILE__, "AI Proc: ATTACK_UNIT %sw/%d.. ", pGroup? pGroup->unitName: "ANY", int1);

				// search for best possible target based on distance from our portal, and relative health
				pList1 = &pWorld->unitLib.lUnits[pAction->sideSpec];
				bestDistance = 9999;
				pUnit2 = NULL;
				gUnit = (grip)pList1->FirstValue();
				while(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					gUnit = (grip)pList1->NextValue();

					if(pUnit->type == JUGGERNAUT)
					{
						// leave him for the player to fight
						continue;
					}

					if(pUnit->type == unitType1 || unitType1 == UNITMAXTYPES)
					{
						distance = MapDistance(pUnit->tileX, pUnit->tileY, aiTable[side].portalX,
							aiTable[side].portalY);

						// increase weight for damaged units (pick off the weak!)
						distance = (distance * ((pUnit->currHP * 10) / pUnit->maxHP)) / 10;

						// increase weight for HARPIES carrying NETs
						if(pUnit->type == HARPY && pUnit->item == HARPY_NET)
							distance -= 15;

						if(distance < bestDistance)
						{
							pUnit2 = pUnit;
							bestDistance = distance;
						}
					}
				}

				if(!pUnit2)
				{
					AILog(__LINE__, __FILE__, "NOTARG\n");
					break;
				}
				AILog(__LINE__, __FILE__, "%s@(%d,%d)\n", pUnit2->unitName, pUnit2->tileX, pUnit2->tileY);

				// if target unit is on the move	or is currently being attacked by us
				if(pUnit2->currAction == MOVE_TO || pUnit2->lastAttackTicks > currTicks - TICKS_PER_SEC * 10)
				{
					fSneak = FALSE;
				}
				else
				{
					// try to sneak up on him
					fSneak = TRUE;
				}
				AssignUnits(side, ATTACK, pUnit2->tileX, pUnit2->tileY, AI_ATTACK_UNIT, pUnit2->serialNum, int1,
					FALSE, fSneak);
				break;

			case AI_ATTACK_BUILDING:
				bldgType = (funcType)pAction->actionSpec;
				AILog(__LINE__, __FILE__, "AI Proc: ATTACK_BLDG %s.. ", &szFuncNames[bldgType][0]);

				// find a building matching desired type
				pList1 = &pWorld->map.lBuildings[pAction->sideSpec];
				gBldg = (grip)pList1->FirstValue();
				while(gBldg)
				{
					pBldg = ADerefAs(MapSpace, gBldg);
					gBldg = (grip)pList1->NextValue();

					// is pBldg what we want?
					if(pBldg->h.currHP && (pBldg->h.func == bldgType || bldgType == TOTALFUNCTIONS))
					{
						// found one!
						// how many attackers do we want?
						if(CheckCaution())
							int1 = eCaution / 25 + 1;
						else int1 = 1;
						AILog(__LINE__, __FILE__, "%s@(%d,%d) w/%d\n", szFuncNames[pBldg->h.func],
							pBldg->h.xPos, pBldg->h.yPos, int1);

						// if building was recently attacked already
						if(pBldg->lastAttackTicks > currTicks - TICKS_PER_SEC * 10)
							fSneak = FALSE;
						else fSneak = TRUE;

//						AssignUnits(side, ATTACK, pBldg->h.xPos, pBldg->h.yPos, AI_ATTACK_BUILDING, pBldg->serialNum, int1,
//							(bool)!fSneak, fSneak);
						AssignUnits(side, ATTACK, pBldg->h.xPos, pBldg->h.yPos, AI_ATTACK_BUILDING, pBldg->serialNum, int1,
							FALSE, fSneak);
						gBldg = NULL;
						pBldg = NULL;
						break;
					}
				}
				if(pBldg || !pList1->count)
					AILog(__LINE__, __FILE__, "NOTARG\n");
				break;

  			case AI_DEFEND_BUILDING:
				bldgType = (funcType)pAction->actionSpec;
				AILog(__LINE__, __FILE__, "AI Proc: DEF_BLDG %s..", &szFuncNames[bldgType][0]);
				pBldg = FindClosestBldg(bldgType, friendlySide, aiTable[friendlySide].portalX,
					aiTable[friendlySide].portalY);
				if(!pBldg)
				{
					// try looking in the neutral list
					pBldg = FindClosestBldg(bldgType, SIDE0, aiTable[friendlySide].portalX,
						aiTable[friendlySide].portalY);

					if(!pBldg)
					{
						AILog(__LINE__, __FILE__, "NOTARG\n");
						break;
					}
				}

				if(CheckCaution())
					int1 = eCaution / 25 + 1;
				else int1 = 1;

				action1 = GUARD;
				aiAction1 = AI_DEFEND_BUILDING;
				int2 = ARandom(pBldg->size);
				tempX = pBldg->tiles[int2][0];
				tempY = pBldg->tiles[int2][1];

				// if not recently attacked
				if(pBldg->lastAttackTicks < currTicks - TICKS_PER_SEC * 3)
				{
					// if already well-defended
					for(loop2 = 0, int2 = 0; loop2 < MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE; loop2++)
						if(pBldg->defenders[loop2] && pWorld->DerefSerial(pBldg->defenders[loop2]))
							int2++;
					if(int2 >= eCaution / 15)
					{
						// are we feeling paranoid?
						if(!CheckCaution() || !CheckCaution())
						{
							// nah, dont sweat it
							AILog(__LINE__, __FILE__, "bldg already has %d defenders\n", int2);
							break;
						}
					}
				}
//				else if(CheckWits() && CheckWits())
//				{
//					// go straight for the last attacker
//					pUnit2 = (Unit *)pWorld->DerefSerial(pBldg->lastAttackerSerial);
//					if(pUnit2)
//					{
//						tempX = pUnit2->tileX;
//						tempY = pUnit2->tileY;
//						action1 = MOVE_TO;
//						aiAction1 = AI_NONE;
//					}
//				}

				AILog(__LINE__, __FILE__, "%s@(%d,%d) w/%d\n", szFuncNames[pBldg->h.func], pBldg->h.xPos, pBldg->h.yPos, int1);
				AssignUnits(side, action1, tempX, tempY, aiAction1, pBldg->serialNum, int1,
					TRUE, FALSE);
				break;

			case AI_SUMMON:
				AILog(__LINE__, __FILE__, "AI Proc: SUMMON..");
				if (aiTable[side].portalX == -1)
				{
					AILog(__LINE__, __FILE__, "no portal\n");
					break;
				}

				// find if a unit is on the portal
				pUnit = pWorld->unitLib.GetUnit(aiTable[side].portalX, aiTable[side].portalY);
				if(pUnit)
				{
					AILog(__LINE__, __FILE__, "PORTAL blocked\n");
					if(pUnit->player == side && pUnit->currAction == GUARD)
					{
						// this is our player.  try and move him off the portal

						if(pUnit->type == ACOLYTE && CheckWits())
						{
							// well he's an acolyte, let's transform him
							TransformUnit(pUnit,
							(unitType)transformPrefs[pUnit->player][ARandom(totalTransformPrefs[pUnit->player])][0]);
						}
						else
						{
							// just side-step him, so as not to create a traffic jam at the portal

							// find an opening
							index = ARandom(8);
							for (loop2 = 0; loop2 < 8; loop2++)
							{
								tempX = aiTable[side].portalX + offsets[index][0] * ARandom(3) + 1;
								tempY = aiTable[side].portalY + offsets[index][1] * ARandom(3) + 1;

								if(!pWorld->unitLib.gUnitMap[tempY][tempX] && pUnit->CanIMoveTo(tempX,tempY))
								{
									pUnit->SetPrimaryAction(MOVE_TO, tempX, tempY);
									if(pUnit->ai.action != AI_RETREAT)
										ClearAIMission(pUnit);
									ReportUnit(pUnit, "LEAVE PORTAL", TRUE);
									break;
								}
								index = (index + 1) % 8;
							}
						}
						// try again next cycle
						break;
					}
				}

				if(pWorld->mana[side] < MANA_SUMMON)
				{
					AILog(__LINE__, __FILE__, "not enough mana\n");
					// hmmm.  we need an acolyte..and don't have enough mana.
					// maybe we should cheat and just get some mana :) hahahaha!				
					if(CheckWits() && CheckSpellIQ() &&
						aiTable[friendlySide].currAcolytes <= aiTable[friendlySide].minAcolytes
						&& CheckCheat(AICHEAT_SUMMON_NEED_MANA))
					{
						AILog(__LINE__, __FILE__, "CHEATING! ");
						pWorld->mana[side] = MANA_SUMMON;
					}
					else
					{
						// guess we'll be honest
						break;
					}
				}

				AILog(__LINE__, __FILE__, "SUMMONING\n");
				pWorld->Summon(ACOLYTE, aiTable[side].portalX, aiTable[side].portalY, side);
				break;

			case AI_TRANS_FOUNDATION:
				AILog(__LINE__, __FILE__, "AI Proc: TRANS_FOUND..");
				// okay, we want to make a building of type X on a foundation.
				// First - do we already have unit(s) trying to do so?

				pList1 = &pWorld->unitLib.lUnits[side];
				int1 = 0;
				pBldg = NULL;
				gUnit = (grip)pList1->FirstValue();
				while(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					gUnit = (grip)pList1->NextValue();

					// is this unit trying to make the same type of building as we want to?
					if(pUnit->ai.action == AI_TRANS_FOUNDATION && pUnit->ai.data == pAction->actionSpec)
					{
						if(!pBldg)
						{
							// note for later
							pBldg = pWorld->map.GetStructure(pUnit->primaryX, pUnit->primaryY);
							if(!pBldg || (funcType)(pBldg->h.func) != FUNC_GEN_FOUND)
							{
								// misguided units - ignore
								pBldg = NULL;
								continue;
							}
						}
						int1++;
					}
				}

				// if project not even started yet
				if(!int1)
				{
					// pick a foundation
					// build as close to our portal as we can
					pBldg = FindClosestBldg(FUNC_GEN_FOUND, SIDE0, aiTable[side].portalX, aiTable[side].portalY);
					if(!pBldg)
					{
						AILog(__LINE__, __FILE__, " no FOUNDATION avail.\n");
						break;
					}
					AILog(__LINE__, __FILE__, " FOUNDATION@(%d,%d)\n", (int)pBldg->h.xPos, (int)pBldg->h.yPos);
				}
				else
				{
					if(!pBldg)	// for whatever reason
					{
						AILog(__LINE__, __FILE__, "ERROR: Found misdirected builder!\n");
						break;
					}

					AILog(__LINE__, __FILE__, "join TEAMof%d@FOUND(%d,%d) w/%d\n", int1, (int)pBldg->h.xPos,
						(int)pBldg->h.yPos, 4 - int1);
				}

				// figure how many to send
				int2 = aiTable[side].currAcolytes - aiTable[side].absMinAcolytes;	// how many free?
				int2 = AMin(int2, 4);	// no more than 4
				int2 = AMax(int2, 0);	// no less than 0
				int2 -= int1;				// subtract units already sent

				if(int2 < 1)
				{
					AILog(__LINE__, __FILE__, "No spare ACOLYTES!\n");
					break;
				}

				// if not enough people already working on it
				// we know what we want to build, and where.
				// Now gather a crew

				#ifndef NDEBUG
				pMono->Out("AI: sending crew of %d\n", int2);
				#endif
				int1 = ARandom(pBldg->size);
				AssignUnits(side, MOVE_TO, pBldg->tiles[int1][0],
					pBldg->tiles[int1][1], AI_TRANS_FOUNDATION,
					pAction->actionSpec, int2, FALSE);
				break;

			case AI_TRANSFORM:
				AILog(__LINE__, __FILE__, "AI Proc: TRANSFORM..");
				// okay, we wanna transform ONE unit to another type.

				// if currently short on acolytes, wait..
				if(aiTable[side].currAcolytes < aiTable[side].minAcolytes - 1)
				{
					// if also short on fighters
					if(aiTable[side].currAcolytes > aiTable[side].minAcolytes / 2 &&
						aiTable[side].currFighters < (aiTable[side].minFighters / 2) &&
						CheckCaution() && CheckCaution())
						;	// dont skip TRANSFORM, cuz we need military too
					else
					{
						// nevermind, wait til another replacement is made, to keep the mana flowing
						AILog(__LINE__, __FILE__, "insuf acolytes\n");
						break;
					}
				}

				unit1 = (unitType)pAction->actionSpec;

				// find the most wounded acolyte we have
				pGuardUnit = guardArray;
				pGuardUnit2 = NULL;
				int1 = 0;
				pUnit = NULL;
				for(loop2 = 0; loop2 < guardCount; loop2++, pGuardUnit++)
				{
  					if(!pGuardUnit->pUnit)
						continue;

					if(pGuardUnit->type != ACOLYTE)
						continue;

					if(pGuardUnit->pUnit->ai.action == AI_TRANSFORM ||
						pGuardUnit->pUnit->ai.action == AI_TRANS_FOUNDATION ||
						pGuardUnit->pUnit->currTerrain == FOUND_GENERIC)
						continue;

					if(!pGuardUnit2 || (pGuardUnit->pUnit->currHP < pGuardUnit2->pUnit->currHP && CheckWits()))
					{
						pGuardUnit2 = pGuardUnit;
					}
				}
				
				if(pGuardUnit2)
				{
					pUnit = pGuardUnit2->pUnit;
					pGuardUnit->pUnit = NULL;
					AILog(__LINE__, __FILE__, "\n");
					TransformUnit(pUnit, unit1);
				}
				else AILog(__LINE__, __FILE__, "could not find ACOLYTE\n");
				break;

			case AI_GET_ITEM:
				AILog(__LINE__, __FILE__, "AI Proc: GET_ITEM..");
				pUnit2 = NULL;
				pBldg = NULL;
				tempX = -1;
				tempY = -1;

				item1 = (itemType)pAction->actionSpec;
				if(item1 == ENDOFITEMTYPES)
				{
					// Pick an item, any item.
					pList1 = &(pWorld->itemMgr.lItems);
					if(!pList1->count)
					{
						// nothing to go after
						AILog(__LINE__, __FILE__, "no items!\n");
						break;
					}
					int1 = ARandom(pList1->count) + 1;
					gItem = (grip)(pList1->At(int1));
					pItem1 = ADerefAs(BAMItem, gItem);
					tempX = pItem1->tileX;
					tempY = pItem1->tileY;
					item1 = pItem1->type;
				}
				else
				{
					// search for an instance of the specified item

					// search open space
					pList1 = &(pWorld->itemMgr.lItems);
					pItem1 = NULL;
					gItem = (grip)pList1->FirstValue();
					while(gItem && !pItem1)
					{
						pItem1 = ADerefAs(BAMItem, gItem);
						gItem = (grip)pList1->NextValue();

						if(!pItem1)
							continue;

						if(pItem1->type == item1)
						{
							pBldg = pWorld->map.GetStructure(pItem1->tileX, pItem1->tileY);
							if(pBldg && pBldg->h.func == FUNC_CAMP && pBldg->h.owner == side)
							{
								// leave it there
							}	
							else
							{
								tempX = pItem1->tileX;
								tempY = pItem1->tileY;
								break;
							}
						}
						pItem1 = NULL;
					}

					if(tempX == -1)
					{
						// can't find one on the ground - search non-friendly units
						pUnit2 = NULL;
						for(loop2 = 0; loop2 < TOTAL_SIDES && !pUnit2; loop2++)
						{
							if(loop2 == side)
							{
								continue;
								// we're not gonna attack our own guys
							}

							pList1 = &pWorld->unitLib.lUnits[loop2];
							gUnit = (grip)pList1->FirstValue();
							while(gUnit && !pUnit2)
							{
								pUnit2 = ADerefAs(Unit, gUnit);
								if(pUnit2->item != item1)
								{
									pUnit2 = NULL;
									gUnit = (grip)pList1->NextValue();
								}
								else
								{
									tempX = pUnit2->tileX;
									tempY = pUnit2->tileY;
								}
							}
						}
					}

					if(tempX == -1)
					{
						// search the buildings
						pBldg = NULL;
						for(loop2 = 0; loop2 < TOTAL_SIDES && !pBldg; loop2++)
						{
							if(loop2 == side)
								continue;
						
							pList1 = &pWorld->map.lBuildings[loop2];
							gBldg = (grip)pList1->FirstValue();
							while(gBldg && !pBldg)
							{
								pBldg = ADerefAs(MapSpace, gBldg);
								if(pBldg && pBldg->h.currHP && pBldg->h.presetItem == item1)
								{
									tempX = pBldg->h.xPos;
									tempY = pBldg->h.yPos;
								}
								else
								{
									gBldg = (grip)pList1->NextValue();
									pBldg = NULL;
								}
							}
						}
					}
				}

				// now, find nearest avail unit to get it
				pGuardUnit = NULL;
				pGuardUnit2 = guardArray;
				bestDistance = 9999;
				for(loop2 = 0; loop2 < guardCount; loop2++, pGuardUnit2++)
				{
					pUnit = pGuardUnit2->pUnit;
					if(!pUnit)
					{
						// invalid entry
						continue;
					}

					if(pUnit->item)
					{
						// already has an item				
						continue;
					}

					if(!pUnit->CanHaveItem())
					{
						// kinda tough without hands
						continue;
					}
					distance = MapDistance(pUnit->tileX, pUnit->tileY, tempX, tempY);

					if(pUnit->type == ACOLYTE && distance > 7)
					{
						// dont risk acolytes
						continue;
					}

					if(distance < bestDistance)
					{
						// new candidate
						pGuardUnit = pGuardUnit2;
						bestDistance = distance;
					}
				}

				// if someone was picked
				if(!pGuardUnit)
				{
					AILog(__LINE__, __FILE__, "no avail units\n");
					break;
				}

				// go pick it up
				pUnit = pGuardUnit->pUnit;
				if(pUnit2 || pBldg)
					pUnit->SetPrimaryAction(ATTACK, tempX, tempY);
				else pUnit->SetPrimaryAction(MOVE_TO, tempX, tempY);
				pUnit->ai.action = AI_GET_ITEM;

				// return-to-base coords when he's done
				pUnit->ai.x = pUnit->tileX;
				pUnit->ai.y = pUnit->tileY;
				ReportUnit(pUnit, "GET ITEM:START", TRUE);
				
				// make sure he doesnt get picked to do anything else this cycle
				pGuardUnit->pUnit = NULL;
				break;
			
			case AI_GATHER_MANA:
				// find guarding acolyte with most mana stored
				AILog(__LINE__, __FILE__, "AI Proc: MANA..");

				pList1 = &pWorld->unitLib.lUnits[side];
				gUnit = (grip)pList1->FirstValue();
				pUnit2 = NULL;
				while(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					gUnit = (grip)pList1->NextValue();
					if(pUnit->type == ACOLYTE && pUnit->currAction == GUARD && !pUnit->fAutoControl)
					{
						// if this unit doesnt have much mana, AND we have enough in reserve already
						if(pUnit->currMana < 5 && pWorld->mana[side] >= aiTable[side].manaReserve)
						{
							// ignore this entry
							continue;
						}

						if(!pUnit2 || pUnit->currMana > pUnit2->currMana)
						{
							// this guy is an improvement - take him now
							pUnit2 = pUnit;
						}
					}
				}

				// if someone was found
				if(pUnit2)
				{
					pUnit2->SetAction(SKILL);
					ReportUnit(pUnit2, "MANA:START", TRUE);
				}
				else
				{
					AILog(__LINE__, __FILE__, "no one avail.\n");
				}
				break;
		}
	}
}

// find next closest hotspot from us to target
bool
AI::FindWayPoint(Unit *pUnit, int targetX, int targetY, int *pWayX, int *pWayY)
{
	int		subjectToTarget, subjectToHotSpot, subjectX, subjectY, targetToHotSpot, loop1, bestDistance;
	ai_guard	*pCurrSpot, *pBest;

	subjectX = pUnit->tileX;
	subjectY = pUnit->tileY;
	pBest = NULL;
	bestDistance = 0xFFFF;
	subjectToTarget = MapDistance(targetX, targetY, subjectX, subjectY);
	for(loop1 = 0, pCurrSpot = hotSpotArray; loop1 < AI_MAX_GUARD; loop1++, pCurrSpot++)
	{
		subjectToHotSpot = MapDistance(subjectX, subjectY, pCurrSpot->x, pCurrSpot->y);
		if(subjectToHotSpot >= bestDistance)
		{
			// nevermind, location is further from us than our current favorite
			continue;
		}

		// is location significantly closer to target than we are already?
		targetToHotSpot = MapDistance(targetX, targetY, pCurrSpot->x, pCurrSpot->y);
		if((targetToHotSpot + 5) >= subjectToTarget)
		{
			// nevermind - wont get us closer to target
			continue;
		}

		// is location closer to us than our target is (i.e. along the way)?
		if(subjectToHotSpot >= subjectToTarget)
		{
			// nevermind - our target is closer than this location
			continue;
		}

		// is location an unreasonable detour?
		if((subjectToHotSpot + targetToHotSpot) > subjectToTarget + 5)
		{
			// nevermind - that's going too far out of our way
			continue;
		}

		// okay, this one is best so far
		bestDistance = subjectToHotSpot;
		pBest = pCurrSpot;
	}
	if(pBest)
	{
		*pWayX = pBest->x;
		*pWayY = pBest->y;
		return(TRUE);
	}
	else return(FALSE);
}

void
AI::MonitorWinCons(int side)
{
	int					loop1, loop2, otherSide, tempX, tempY;
	WinningCondition	*pWinCon;
	BAMItem				*pItem;

	otherSide = (side == SIDE1)? SIDE2: SIDE1;

	for(loop1 = 0; loop1 < MAX_WINCONS; loop1++)
	{
		pWinCon = &pWorld->winCons[loop1];
		if(pWinCon->ownerSide == side)
		{
			// what are we trying to do?

			if(CheckAggression() && aiTable[side].currFighters >= aiTable[side].minFighters)
			{
				switch(pWinCon->targetSpec)
				{
					case BUILDING:
						AddAIAction(AI_ATTACK_BUILDING, pWinCon->structSpec,
							pWinCon->targetSide, attackNow[side]? 75: 10);
						break;

					case UNIT:
						AddAIAction(AI_ATTACK_UNIT, pWinCon->unitSpec, pWinCon->targetSide,
							attackNow[side]? 75: 10);
						break;

					case ITEM:
						AddAIAction(AI_GET_ITEM, pWinCon->itemSpec, pWinCon->targetSpec,
							100);
						break;
				}
			}

			if(pWinCon->targetSpec == ITEM && pWinCon->itemSpec != ENDOFITEMTYPES)
			{
				MapSpace	*pBldg;
				pBldg = pWorld->map.FindStructure(FUNC_CAMP, side);
				if(!pBldg)
					pBldg = pWorld->map.FindStructure(FUNC_PORTAL, side);

				// we want this item.  Does anyone already have it?
				List	*pList;
				grip	gUnit;
				Unit	*pUnit;

				pList = &pWorld->unitLib.lUnits[side];
				gUnit = (grip)pList->FirstValue();
				while(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					if(pUnit->item == pWinCon->itemSpec)
					{
//						if(pUnit->currTerrain == CAMP)
//							pUnit->DropItem();
//						else
						for(loop2 = 0; loop2 < pBldg->size; loop2++)
						{
							tempX = pBldg->tiles[loop2][0];
							tempY = pBldg->tiles[loop2][1];
							pItem = pWorld->itemMgr.CheckItem(tempX, tempY);
							if(!pItem || pItem->type != pWinCon->itemSpec)
							{
								pUnit->SetPrimaryAction(DROP, tempX, tempY);
								break;
							}
						}
					}
					gUnit = (grip)pList->NextValue();
				}
			}
		}
		else if(pWinCon->ownerSide == otherSide)
		{
			// what's the enemy trying to do?

			if (CheckCaution())
			{
				switch(pWinCon->targetSpec)
				{
					case BUILDING:
						AddAIAction(AI_DEFEND_BUILDING, pWinCon->structSpec,
							pWinCon->targetSide, 100);
						break;

					case UNIT:
						AddAIAction(AI_DEFEND_UNIT, pWinCon->unitSpec,
							pWinCon->targetSide, 50);
						break;

					case ITEM:
						AddAIAction(AI_GET_ITEM, pWinCon->itemSpec, pWinCon->targetSpec,
							100);
						break;
				}
			}

			if(pWinCon->targetSpec == ITEM && pWinCon->itemSpec != ENDOFITEMTYPES)
			{
				MapSpace	*pBldg;
				pBldg = pWorld->map.FindStructure(FUNC_PORTAL, side);

				// we want this item.  Does anyone already have it?
				List	*pList;
				grip	gUnit;
				Unit	*pUnit;

				pList = &pWorld->unitLib.lUnits[side];
				gUnit = (grip)pList->FirstValue();
				while(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					if(pUnit->item == pWinCon->itemSpec)
					{
						for(loop2 = 0; loop2 < pBldg->size; loop2++)
						{
							tempX = pBldg->tiles[loop2][0];
							tempY = pBldg->tiles[loop2][1];
							pItem = pWorld->itemMgr.CheckItem(tempX, tempY);
							if(!pItem || pItem->type != pWinCon->itemSpec)
							{
								pUnit->SetPrimaryAction(DROP, tempX + (ARandom(3) - 1),
									tempY + (ARandom(3) - 1));
								break;
							}
						}
					}
					gUnit = (grip)pList->NextValue();
				}
			}
		}
	}
}

void
AI::DeleteFromGuardArray(Unit *pUnit)
{
	int		loop1;
	ai_guard	*pGuardUnit;

	pGuardUnit = guardArray;
	for(loop1 = 0; loop1 < guardCount; loop1++, pGuardUnit++)
	{
		if(pGuardUnit->pUnit == pUnit)
		{
			pGuardUnit->pUnit = NULL;
			return;

//			// found match - delete and compress array
//			memmove(pGuardUnit, pGuardUnit + 1, sizeof(ai_guard) * ((guardCount - loop1) - 1));
//			guardCount--;
//			return;
		}
	}

	// if we get here, unit was not found in guard array
}

void
AI::AddToGuardArray(Unit *pUnit)
{
	if (guardCount >= AI_MAX_GUARD)
	{
		// no room left, forget him for now
		return;
	}

	guardArray[guardCount].gUnit = pUnit->gSelf;
	guardArray[guardCount].pUnit = pUnit;
	guardArray[guardCount].type = pUnit->type;
	guardArray[guardCount].x = pUnit->tileX;
	guardArray[guardCount].y = pUnit->tileY;
	guardArray[guardCount].item = pUnit->item;
	guardCount++;
}

void
AI::DeleteFromHealArray(Unit *pUnit)
{
	int		loop1;
	ai_guard	*pHealArray;

	pHealArray = healArray;
	for(loop1 = 0; loop1 < healCount; loop1++, pHealArray++)
	{
		if(pHealArray->pUnit == pUnit)
		{
			pHealArray->pUnit = NULL;
			return;
		}
	}

	// if we get here, unit was not found in guard array
}

void
AI::AddToHealArray(Unit *pUnit)
{
	if (healCount >= AI_MAX_GUARD)
	{
		// no room left, forget him for now
		return;
	}

	healArray[healCount].gUnit = pUnit->gSelf;
	healArray[healCount].pUnit = pUnit;
	healArray[healCount].type = pUnit->type;
	healArray[healCount].x = pUnit->tileX;
	healArray[healCount].y = pUnit->tileY;
	healArray[healCount].item = pUnit->item;
	healCount++;
}

void
AI::Cleanup()
{
	if(pDebugAI)
	{
		AILog(__LINE__, __FILE__, "\nAI::Cleanup()\n");
		fclose(pDebugAI);
		pDebugAI = NULL;
	}
}

int
AI::MapDistance(int x, int y, int destX, int destY)
{
	// find the shortest distance between the two points.
	// take walking diagnal into account
	int workX = abs(destX - x);
	int workY = abs(destY - y);
 	int destDist;

	if (workX >= workY)
	{
		destDist = workX;
	}
	else
	{
		destDist = workY;
	}

	return destDist;
}

int
AiDistanceCompare(void const *pparm1,void const *pparm2)
{
	ai_guard	*parm1,*parm2;

	parm1 = (ai_guard *) pparm1;
	parm2 = (ai_guard *) pparm2;

	if(parm1->distance < parm2->distance)
	{
		return -1;
	}
	else
	{
		if (parm1->distance > parm2->distance)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}


// A unit is wounded.  He needs to be healed
void
AI::HelpRepair(MapSpace *pBldg, Unit *pUnit)
{
	ai_guard	*pHealArrayUnit;
	int		i;
	int		bldgX, bldgY;
	Unit		*pHelpUnit;
	int		closestDist;
	int		closestIndex;
	int		distance;
	int		randIndex;

	if(pUnit)
	{
		// we are a unit.  check to see if we can heal ourselves
		if (pUnit->item == MASON_MIX && pUnit->type == GUARDIAN || pUnit->type == JUGGERNAUT ||
 			 pUnit->type == GOLEM)
		{
			// we can heal ourself
			AILog(__LINE__, __FILE__, "AI Help:\n");
			ReportUnit(pUnit, "REPAIR SELF:ITEM", TRUE);
			pUnit->UseItem();
	

			// rather than waste time to take this unit
			// out of the repair array, just set the
			// count to zero.
			repairCount = 0;
		}
	}

	// make sure that we have some other units that can help
	if (repairCount)
	{
		if (pBldg)
		{
			randIndex = ARandom(pBldg->size);
			bldgX = pBldg->tiles[randIndex][0];
			bldgY = pBldg->tiles[randIndex][1];
		}
		else
		{
			// must be the juggernaut or golem
 			bldgX = pUnit->tileX;
 			bldgY = pUnit->tileY;
		}

		// find the distance from the helping units to this building
		// NOTE: This does not take into account actual walking distance.
		pHealArrayUnit = repairArray;
		closestDist = 0xffff;
		closestIndex = -1;
		for(i = 0; i < repairCount; i++)
		{
			distance = MapDistance(bldgX, bldgY,
					pHealArrayUnit->x, pHealArrayUnit->y);		
			if (distance < closestDist)
			{
				closestDist = distance;
				closestIndex = i;
			}
			pHealArrayUnit++;
		}

		if (closestIndex != -1)
		{
			// we have one.
			// make sure that it's still available.

			pHelpUnit = ADerefAs(Unit, repairArray[closestIndex].gUnit);

			if (pHelpUnit->fAutoControl == FALSE && 
					pHelpUnit->fPrimarySatisfied &&
					pHelpUnit->currAction == GUARD)
			{
				if (repairArray[closestIndex].item == MASON_MIX)
				{
					AILog(__LINE__, __FILE__, "AI Help: REPAIR_ITEM @(%d,%d)\n", bldgX, bldgY);
					pHelpUnit->SetPrimaryAction(USE_ITEM, bldgX, bldgY);
					pHelpUnit->ai.action = AI_HEAL;
					pHelpUnit->ai.data = 0;
					ReportUnit(pHelpUnit, "REPAIR:USE ITEM", TRUE);
				}
				else
				{
					// must be a gnome
					AILog(__LINE__, __FILE__, "AI Help: REPAIR_SKILL @(%d,%d)\n", bldgX, bldgY);
					pHelpUnit->SetPrimaryAction(SKILL, bldgX, bldgY);
					pHelpUnit->ai.action = AI_HEAL;
					pHelpUnit->ai.data = 0;
					ReportUnit(pHelpUnit, "REPAIR:USE SKILL", TRUE);
				}
			}

			// this unit has been used.
			// compress the array
			repairCount--;
			if (repairCount)
			{
				repairArray[closestIndex] = repairArray[repairCount];
			}
		}
	}
}


// returns TRUE if the item can be used to heal the unit
bool
AI::CanUseItemToHeal(itemType item, Unit *pUnit)
{
	bool	canBeUsed;

	if(pUnit->primaryAction == BERSERK || pUnit->type == GOLEM || pUnit->type == GUARDIAN ||
		 pUnit->type == JUGGERNAUT || pUnit->type == WRAITH)
	{
		// these units cannot be healed
		canBeUsed = FALSE;
	}
	else
	{
		if(CheckItemIQ() && (item == BOAR_BURGER || (item == HEALING_SALVE && pUnit->type != FURY && 
		 	pUnit->type != ROC_EGG && pUnit->type != TROLL)))
		{
			// this item can be used
			canBeUsed = TRUE;
		}
		else
		{
			// no healing will be done
			canBeUsed = FALSE;
		}
	}

	return canBeUsed;
}

// A unit is wounded.  He needs to be healed
void
AI::HelpHealUnit(Unit *pUnit)
{
	ai_guard	*pHealArrayUnit;
	int		i;
	int		unitX, unitY;
	Unit		*pHelpUnit;
	int		closestDist;
	int		closestIndex;
	int		distance;
	bool		fEmergency;

	if(pUnit->primaryAction == BERSERK)
	{
		// these units cannot be healed
		return;
	}

	// how bad is he?
	fEmergency = (pUnit->currHP < pUnit->maxHP / 2)? TRUE: FALSE;

	// do we have an item to use?
	if(fEmergency)
	{
		switch(pUnit->item)
		{
			case BOAR_BURGER:
			case MASON_MIX:
			case HEALING_SALVE:
				if(pUnit->ImmuneToItem(pUnit->item))
					break;

				// we are carrying an item that can heal ourself
				AILog(__LINE__, __FILE__, "AI Help:\n");
				ReportUnit(pUnit, "HEALSELF:ITEM", TRUE);
				pUnit->UseItem();
				return;
		}
	}

	// can he heal himself?
	if(pUnit->type == CLERIC || pUnit->type == PALADIN)
	{
		if(!pUnit->fAutoControl && pWorld->mana[pUnit->player] >= pUnit->skillCost)
			pUnit->SetAction(SKILL, pUnit->tileX, pUnit->tileY);
		return;
	}

	// make sure that we have some other units that can help
	if(healCount)
	{
		unitX = pUnit->tileX;
		unitY = pUnit->tileY;

		// find the distance from the helping units to this unit
		// NOTE: This does not take into account actual walking distance.
		pHealArrayUnit = healArray;
		closestDist = 0xffff;
		closestIndex = -1;
		for(i = 0; i < healCount; i++)
		{
			pHelpUnit = pHealArrayUnit->pUnit;
			if(!pHelpUnit)
				continue;	// already used up

			distance = MapDistance(unitX, unitY, pHealArrayUnit->x, pHealArrayUnit->y);		
			pHealArrayUnit++;

  			if(pUnit->ImmuneToSkillFrom(pHelpUnit))
			{
				// incompatible
				continue;
			}

			// if travel time would be required
			if(distance > 1)
			{
				if(!fEmergency && pHelpUnit->currAction != GUARD)
				{
					// he's busy and we're not that bad - dont bother him
					continue;
				}
			}

			if(distance < closestDist)
			{
				closestDist = distance;
				closestIndex = i;
			}
		}

		if(closestIndex != -1)
		{
			// we have one.
			// make sure that it's still available.
			pHelpUnit = healArray[closestIndex].pUnit;

			if(pHelpUnit)
			{
				AILog(__LINE__, __FILE__, "AI Help: HEAL_SKILL @(%d,%d)\n", unitX, unitY);
				pHelpUnit->SetPrimaryAction(SKILL, unitX, unitY);
				pHelpUnit->ai.action = AI_HEAL;
				pHelpUnit->ai.data = 0;
				ReportUnit(pUnit, "HEAL:SKILL", TRUE);
			}

			DeleteFromHealArray(pHelpUnit);
		}
	}
}

// are all units assigned to unit's mission ready to proceed?
bool
AI::IsTeamReady(Unit *pUnit)
{
	// find all units with identical ai.action & ai.data (target)
	Unit	*pMember;
	grip	gMember;
	List	*plUnits;
	int	tempX, tempY, teamSize = 0, listNext;
	ai_action	missionType;
	uint32		missionTarget;

	missionType = pUnit->ai.action;
	missionTarget = pUnit->ai.data;

	plUnits = &pWorld->unitLib.lUnits[pUnit->player];
	listNext = plUnits->next;
	gMember = (grip)plUnits->FirstValue();

	while(gMember)
	{
		pMember = ADerefAs(Unit, gMember);
		gMember = (grip)plUnits->NextValue();

		if(pMember->ai.action != missionType || pMember->ai.data != missionTarget)
			continue;
		teamSize++;
		if(FindWayPoint(pMember, pMember->primaryX, pMember->primaryY, &tempX, &tempY))
		{
			// at least one team member still has a stopping point along the way.
			// entire team is not yet ready
			plUnits->next = listNext;
			return(FALSE);
		}
	}

	// no one found not ready, so presume all is well.
	#ifndef NDEBUG
	pMono->Out("size=%d ", teamSize);
	#endif
	plUnits->next = listNext;
	return(TRUE);
}

// determine how many units would be avail to help
int
AI::CountUnitsToHelp(int side)
{
//	ai_guard *pGuardArray;
	int		helpAvail;
//	Unit		*pUnit;
//	grip		gUnit;

	helpAvail = AMax(0, guardCount - aiTable[side].absMinAcolytes);
	return(helpAvail);
}

// send X units to accomplish goal
#define MAX_TEAM_SIZE	20
bool
AI::AssignUnits(int side, action newAction, int targetX, int targetY, ai_action newAIAction, uint32 newAIData,
	int teamSize, bool fCompromise, bool fSneaky)
{
	int 		unitsFound, loop1, tempX, tempY, totalAttackers, totalSneakers,
					sneakerCoords[MAX_TEAM_SIZE][2];
	Unit		*pUnit, *pTargetUnit, *pAttackers[MAX_TEAM_SIZE],
					*pSneakers[MAX_TEAM_SIZE];
	ai_guard	*pGuardUnit;
	bool		fStrongUnitsOnly = FALSE;
	MapSpace	*pTargetBldg;

	if(newAction == ATTACK)
	{
		if(pWorld->ai.aiTable[side].currFighters < pWorld->ai.aiTable[side].minFighters)
		{
			// already dont have enough fighters in reserve - skip it
			// BUGBUG! We should add an emergency over-ride arg to this function

			if(!attackNow[side])
				return(FALSE);
			// else we're in an attack phase - take the chance
		}
	}

  	switch(newAIAction)
	{
		case AI_DEFEND_BUILDING:
			if(aiTable[side].currAcolytes <= aiTable[side].minAcolytes)
			{
				fStrongUnitsOnly = TRUE;
			}
			else if(CheckWits() || CheckWits() || CheckWits())
			{
				fStrongUnitsOnly = TRUE;
			}
			break;
	}

	pTargetUnit = pWorld->unitLib.GetUnit(targetX, targetY);
	pTargetBldg = pWorld->map.GetStructure(targetX, targetY);
	if(newAction == GUARD)
	{
		fStrongUnitsOnly = TRUE;
	}
	else if(newAction == ATTACK)
	{
		if(pTargetUnit)
		{
			if(pTargetUnit->type == HARPY && pTargetUnit->item == HARPY_NET
				&& CheckAggression() && !CheckWits())
			{
				// doesnt take a soldier for this
			}
			else if(pTargetUnit->currHP < 4 && CheckAggression() && !CheckWits())
			{
				// he aint so tough - let acolytes be considered too
			}
			else fStrongUnitsOnly = TRUE;
		}
		else if(pTargetBldg)
		{
			// if bldg is still pretty strong
			if(pTargetBldg->h.currHP > pTargetBldg->h.maxHP / 4)
				fStrongUnitsOnly = TRUE;
		}
		else
		{
			fStrongUnitsOnly = TRUE;
		}
	}

	totalAttackers = 0;	totalSneakers = 0;
	// rate all avail units by distance to target
	unitsFound = 0;
	pGuardUnit = guardArray;
	for(loop1 = 0; loop1 < guardCount; loop1++, pGuardUnit++)
	{
		pGuardUnit->distance = 9999;
		pUnit = pGuardUnit->pUnit;

		if(!pUnit)
		{
			// already taken
			continue;
		}

		if(newAIAction == AI_TRANS_FOUNDATION)
		{
			// only send acolytes
			if(pUnit->type != ACOLYTE)
				continue;

			// dont interrupt other builders/transformers
			if(pUnit->ai.action == AI_TRANS_FOUNDATION || pUnit->ai.action == AI_TRANSFORM)
				continue;
		}
		
  		switch(pUnit->type)
		{
			case ACOLYTE:
				if(fStrongUnitsOnly)
				{
					if(aiTable[friendlySide].currAcolytes < aiTable[friendlySide].absMinAcolytes)
						continue;

					if(!CheckAggression() || !CheckAggression() || CheckWits() || CheckWits())
					{
						// dont send any acolytes
						continue;
					}
				}
				if(pUnit->ai.action == AI_TRANSFORM || pUnit->ai.action == AI_TRANS_FOUNDATION)
					continue;

				if(ARandom(10))	// rarely send ACOLYTEs
					continue;
				break;
		
			case FURY:
				// is target worth sending a FURY?
				if(newAction != ATTACK || !pTargetUnit)
					continue;
				break;

			case HARPY:
				if(pUnit->item == HARPY_NET && newAction != DROP)
				{
					// um, bad idea.
					continue;
				}
				break;
		}

		pGuardUnit->distance = MapDistance(pUnit->tileX, pUnit->tileY, targetX, targetY);
		unitsFound++;

		if(pUnit->type == FURY)
		{
			if(pTargetUnit->type == ACOLYTE || pTargetUnit->currHP < 8)
			{
				// try not to waste a FURY on something trivial
				pGuardUnit->distance += 40;
			}
			else
			{
				pGuardUnit->distance = 1;	// try to use FURY over other units
				teamSize = 1;	// dont send anyone else
				fSneaky = FALSE;	// FURYs cant sneak
			}
		}
	}

	// did we find as many as we wanted?
	if(unitsFound < teamSize)
	{
		// no, should we settle for what we found?
		if(fCompromise)
		{
			teamSize = unitsFound;
		}
		else
		{
			#ifndef NDEBUG
			pMono->Out("AI: ASSIGN() - only %d units avail, aborting.\n", unitsFound);
			#endif
			return(FALSE);
		}
	}

	if(guardCount > 1)
		qsort(guardArray, guardCount, sizeof(ai_guard), AiDistanceCompare);

	// dont send more than requested
	unitsFound = AMin(unitsFound, teamSize);

	// send in the troops!
	for(loop1 = 0, pGuardUnit = guardArray; loop1 < guardCount && unitsFound > 0; loop1++, pGuardUnit++)
	{
		if(pGuardUnit->distance == 9999)
		{
			// draft dodger
			continue;
		}

		pUnit = pGuardUnit->pUnit;
		AILog(__LINE__, __FILE__, "AI Assgn: %s(s%d)@(%d,%d) %s@(%d,%d) %s\n", pUnit->unitName,
			pUnit->serialNum, pUnit->tileX, pUnit->tileY,
			pVerbNames[newAction], targetX, targetY,
			fSneaky? "SNEAKY": "");

		if(fSneaky && pUnit->type != FURY)
		{
			// sneak-up on target instead of going straight to it.
			// If an hotspot exists between this unit and the target, gather there instead.
			// They'll be given further orders when they get there.
			if(FindWayPoint(pUnit, targetX, targetY, &tempX, &tempY))
			{
				AILog(__LINE__, __FILE__, "  SNEAK TO way point @(%d,%d)\n", tempX, tempY);
				sneakerCoords[totalSneakers][0] = tempX;
				sneakerCoords[totalSneakers][1] = tempY;
				pSneakers[totalSneakers++] = pUnit;
				unitsFound--;
			}
			else
			{
				// no way-points available - just go for it
				AILog(__LINE__, __FILE__, "  No way points found\n");
				pAttackers[totalAttackers++] = pUnit;
				unitsFound--;
			}
		}
		else
		{
			pAttackers[totalAttackers++] = pUnit;
			unitsFound--;
		}
	}

	for(loop1 = 0; loop1 < totalSneakers; loop1++)
	{
		pUnit = pSneakers[loop1];
		pUnit->SetPrimaryAction(MOVE_TO, sneakerCoords[loop1][0], sneakerCoords[loop1][1]);
		pUnit->ai.action = newAIAction;
		pUnit->ai.data = newAIData;
		ReportUnit(pUnit, "SNEAK2:START", TRUE);
		DeleteFromGuardArray(pUnit);
	}

	if(fSneaky && !fCompromise)
	{
		// make sure we have enough units ready to actually attack, before pouncing
		if(totalAttackers < teamSize)
		{
			AILog(__LINE__, __FILE__, "AI Assign: %dATT %dSNEAK, WAIT\n", totalAttackers, totalSneakers);
			return(TRUE);
		}
	}
	
	for(loop1 = 0; loop1 < totalAttackers; loop1++)
	{
		pUnit = pAttackers[loop1];

		// if possibility of a traffic jam
		if(CheckWits() && CheckWits() &&
			pWorld->unitLib.CountUnits(pUnit->tileX, pUnit->tileY, pUnit->player, 2) > 1)
		{
			// will travel be involved?
			if(MapDistance(pUnit->tileX, pUnit->tileY, targetX, targetY) > 1)
			{
				// is at least a partial path available?
				int pathLength = pUnit->FindPathTo(targetX, targetY, pUnit->pathArray, TRUE, FALSE, FALSE);
				pUnit->curPathPosition = 0;
				if(pathLength < 1)
				{
					AILog(__LINE__, __FILE__, "AI Assign: %s(%d,%d) gridlock\n", pUnit->unitName,
						pUnit->tileX, pUnit->tileY);
					// nevermind, leave unit avail to avoid permanent traffic jams
					continue;
				}
			}
		}

		pUnit->SetPrimaryAction(newAction, targetX, targetY);
		pUnit->ai.action = newAIAction;
		pUnit->ai.data = newAIData;
		ReportUnit(pUnit, "ASSIGN:START", TRUE);
		DeleteFromGuardArray(pUnit);
	}
	return(TRUE);
}

void
AI::HelpUnit(Unit *pUnit, action newAction, int32 x, int32 y, int helpNeeded, unitType helpUnitType)
{
	int		helpFound, loop1, guardNums[20];
	Unit		*pHelpUnit;
	bool		allUnits;
	uint32	tempGuardCount, oldGuardCount;
	ai_guard	*pGuardArrayUnit;

	if (guardCount)
	{
		if (helpUnitType == UNITMAXTYPES)
		{
			allUnits = TRUE;
		}
		else
		{
			allUnits = FALSE;
		}

		// find the distance from the helping units to this unit
		// NOTE: This does not take into account actual walking distance.
		pGuardArrayUnit = guardArray;
		int	i;
		for(i = 0; i < guardCount; i++)
		{
			pGuardArrayUnit->distance = MapDistance(pUnit->tileX, pUnit->tileY,
					pGuardArrayUnit->x, pGuardArrayUnit->y);			
			if(pGuardArrayUnit->type == ACOLYTE && helpUnitType != ACOLYTE)
			{
				if(!CheckWits() && CheckAggression())
				{
					// discourage but allow it
					pGuardArrayUnit->distance += 40;
				}
				else
				{
					continue;
					// skip adding acolyte altogether - too risky
				}				
			}
			pGuardArrayUnit++;
		}

		// sort by distance
		if (guardCount > 1)
		{
			qsort(guardArray, guardCount, sizeof(ai_guard), AiDistanceCompare);
		}

		oldGuardCount = tempGuardCount = guardCount;
		tempGuardCount = aiTable[pUnit->player].absMinAcolytes;	// dont fall below this many
		pGuardArrayUnit = guardArray;

		// gather helpers
		for(loop1 = 0, helpFound = 0; loop1 < guardCount && helpFound < helpNeeded; loop1++, pGuardArrayUnit++)
		{
			if ((allUnits || pGuardArrayUnit->type == helpUnitType) && pGuardArrayUnit->pUnit)
			{
				pHelpUnit = pGuardArrayUnit->pUnit;
				if (pHelpUnit)
				{
					if(pHelpUnit->type == ACOLYTE)
					{
						if(helpUnitType != ACOLYTE)
						{
							// dont send ACOLYTEs into battle
							continue;
						}
						else if(tempGuardCount > 0)
						{
							--tempGuardCount;		// count off another reservist
							continue;
						}
						// else okay, send him in

					}

					// this unit is valid

					// BUGBUG - is the following check unnecessary perhaps?
					// make sure that he is still available
					if (pHelpUnit->fAutoControl == FALSE && pHelpUnit->fPrimarySatisfied &&
						 pHelpUnit->currAction == GUARD)
					{
						guardNums[helpFound++] = loop1;
					}
				}
			}
		}

		AILog(__LINE__, __FILE__, "AI Help: %s@(%d,%d) needs help %s@(%d,%d)\n", pUnit->unitName,
			pUnit->tileX, pUnit->tileY, pVerbNames[newAction], x, y);

		if(helpFound < helpNeeded)
		{
			AILog(__LINE__, __FILE__, "%dof%d units found\n", helpFound, helpNeeded);
		}

		// send em off!
		for(loop1 = 0; loop1 < helpFound; loop1++)
		{
			pGuardArrayUnit = &guardArray[guardNums[loop1]];
			pHelpUnit = pGuardArrayUnit->pUnit;

			pHelpUnit->SetPrimaryAction(newAction, x, y);
			pHelpUnit->ai.action = pUnit->ai.action;
			pHelpUnit->ai.data = pUnit->ai.data;
			pGuardArrayUnit->pUnit = NULL;	// no longer avail for this cycle
			pGuardArrayUnit->gUnit = NULL;
			ReportUnit(pUnit, "HELPUNIT:START", TRUE);
//			DeleteFromGuardArray(pHelpUnit);
		}
	}
}

void
AI::AttackUnit(Unit *pUnit, int maxDist)
{
	int		numToHelp;
	Unit		*pHelpUnit;
	uint32	tempGuardCount;
	uint32	oldGuardCount;
	int		unitsUsed;
	ai_guard	*pGuardArrayUnit;
	int		i;

	AILog(__LINE__, __FILE__, "AI AttackUnit: \"%s\"@(%d,%d)\n", pUnit->unitName,
		pUnit->tileX, pUnit->tileY);

	if(!guardCount)
	{
		AILog(__LINE__, __FILE__, "  no units avail\n");
		return;
	}

	numToHelp = ARandom(4) + 1;

	// find the distance from the helping units to this unit
	// NOTE: This does not take into account actual walking distance.
	pGuardArrayUnit = guardArray;
	for(i = 0; i < guardCount; i++)
	{
		pGuardArrayUnit->distance = MapDistance(pUnit->tileX, pUnit->tileY,
				pGuardArrayUnit->x, pGuardArrayUnit->y);			
		pGuardArrayUnit++;
	}

	// sort by distance
	if (guardCount > 1)
	{
		qsort(guardArray, guardCount, sizeof(ai_guard), AiDistanceCompare);
	}

	oldGuardCount = tempGuardCount = guardCount;
	pGuardArrayUnit = guardArray;

	while(numToHelp && tempGuardCount && pGuardArrayUnit->distance < maxDist)
	{
		tempGuardCount--;

		pHelpUnit = pGuardArrayUnit->pUnit;
		if (pHelpUnit)
		{
			// this unit is valid

			// make sure that he is still available
			if (pHelpUnit->fAutoControl == FALSE &&
					pHelpUnit->fPrimarySatisfied &&
					pHelpUnit->currAction == GUARD)
			{
				ClearAIMission(pHelpUnit);
				pHelpUnit->SetPrimaryAction(ATTACK, pUnit->tileX, pUnit->tileY);
				ReportUnit(pHelpUnit, "ATTUNIT:START", TRUE);
						
				numToHelp--;
			}

			// this unit has been used or is no longer available
			guardCount--;
			pGuardArrayUnit->pUnit = NULL;
			pGuardArrayUnit->gUnit = 0;
		}
		pGuardArrayUnit++;
	}

	// compress the guard array
	if (guardCount)
	{
		// we still have some units left
		unitsUsed = oldGuardCount - guardCount;
		if (unitsUsed)
		{
			// we have some to move
			memmove(guardArray, &guardArray[unitsUsed], guardCount * sizeof(ai_guard));
		}
	}
}

void
AI::TransformUnit(Unit *pUnit, unitType transToType)
{
	int				count = 0;
	funcType			bldgType, altBldgType;
	MapSpace			*pBldg;
	int				index;
	
	// get rid unref warning
	transToType = transToType;

	// is there a bldg available for making this type of unit?
	bldgType = pWorld->GetBldgType(transToType);
	altBldgType = pWorld->GetBldgType(transToType, TRUE);

	AILog(__LINE__, __FILE__, "AI: %s(%d,%d) TRANSFORM @%s", pUnit->unitName, pUnit->tileX, pUnit->tileY, szFuncNames[bldgType]);

	// find the closest one on our side
	pBldg = FindClosestBldg(bldgType, pUnit->player, pUnit->tileX, pUnit->tileY);
	if(pBldg == NULL && transToType != ACOLYTE)
	{
		// look for alt choice
		pBldg = FindClosestBldg(altBldgType, pUnit->player, pUnit->tileX, pUnit->tileY);

		if(!pBldg)
		{
			// look for a randomizer
			pBldg = FindClosestBldg(FUNC_IMM_WELL, SIDE0, pUnit->tileX, pUnit->tileY);
		}
	}

	if(pBldg == NULL)
	{
		AILog(__LINE__, __FILE__, ", cant find one\n");

		// we didn't find one.
		if (pUnit->type == ACOLYTE)
		{
			// look for a foundation to create one
			pBldg = FindClosestBldg(FUNC_GEN_FOUND, SIDE0, pUnit->tileX, pUnit->tileY);

			if (pBldg != NULL)
			{
				index = ARandom(pBldg->size);

				AILog(__LINE__, __FILE__, "   MAKE BLDG @(%d,%d)\n", pBldg->tiles[index][0], pBldg->tiles[index][1]);

				pUnit->SetPrimaryAction(MOVE_TO, pBldg->tiles[index][0], pBldg->tiles[index][1]);
				pUnit->ai.action = AI_TRANS_FOUNDATION; 
				ReportUnit(pUnit, "BUILD:START", TRUE);

				// if there's an alt bldg type, flip a coin and choose one or the other.
				// otherwise, just go for the standard type
				pUnit->ai.data = altBldgType? (ARandom(2)? bldgType: altBldgType): bldgType;
				DeleteFromGuardArray(pUnit);

				// dont check for avail helpers, just call for help.  If not enough are sent,
				// more will be later.
				HelpUnit(pUnit, MOVE_TO, pBldg->tiles[index][0], pBldg->tiles[index][1], 3, ACOLYTE);
			}
		}

		if (pBldg == NULL)
		{
			// ok, let's try and find one build to destroy

			// BUGBUG - generic foundations, pick ANY enemy transformer bldg to destroy
			pBldg = FindClosestBldg(TOTALFUNCTIONS, (pUnit->player == SIDE1)? SIDE2: SIDE1,
				pUnit->tileX, pUnit->tileY);

			if (pBldg != NULL)
			{
				// destroy enemy building to create a foundation that we can then use

				index = ARandom(pBldg->size);
				ClearAIMission(pUnit);
				pUnit->SetPrimaryAction(ATTACK, pBldg->tiles[index][0], pBldg->tiles[index][1]);
				ReportUnit(pUnit, "BUILD:DESTROY", TRUE);
				DeleteFromGuardArray(pUnit);

				AILog(__LINE__, __FILE__, "AI: MAKE FOUNDATION @(%d,%d)\n", pBldg->tiles[index][0], pBldg->tiles[index][1]);

				if(CheckCaution())
				{
					// send some assistance along
					HelpUnit(pUnit, ATTACK, pBldg->tiles[index][0], pBldg->tiles[index][1], eCaution / 25 + 1);
				}
			}
		}
	}
	else
	{
		AILog(__LINE__, __FILE__, "\n");
		// move next to the building to transform
		index = ARandom(pBldg->size);
		pUnit->SetPrimaryAction(MOVE_TO, pBldg->tiles[index][0], pBldg->tiles[index][1]);
		pUnit->ai.action = AI_TRANSFORM;
		pUnit->ai.data = transToType;
		ReportUnit(pUnit, "TRANSFORM:MOVE_TO", TRUE);
		DeleteFromGuardArray(pUnit);
		if(pUnit->type == ACOLYTE)
			aiTable[friendlySide].currAcolytes--;

		// are there any ways in?
		if((pBldg->IsSurrounded() || pBldg->CountDefenders() > 2) && pWorld->map.totalHotSpots)
		{			
			ai_guard	*pHot;
			Unit		*pDefender;

			// pick one of the guys guarding the building, and move him elsewhere
			pDefender = pBldg->ChooseDefender();
			pHot = ChooseHotSpot(pBldg->h.xPos, pBldg->h.yPos);
			if(pDefender && pHot)
			{
				pBldg->RemoveDefender(pDefender);
				pDefender->SetPrimaryAction(MOVE_TO, pHot->x, pHot->y);
				pDefender->ai.action = AI_GOTO_HOT_SPOT;
				pDefender->ai.data = 0;
				ReportUnit(pUnit, "TRANSF::RELOC2HOT", TRUE);
			}
		}
	}
}

// find the closest building in the building arrays, given the passed side.
// if no building is found, return NULL

ai_guard *
AI::ChooseHotSpot(int startX, int startY)
{
	bool	fCaution;
	int	loop1, loop2, total, totalFriends;
	ai_guard	*pHot = NULL;

	fCaution = CheckCaution();
	total = hotSpotCount;
	if(!total)
		return(NULL);
	loop1 = ARandom(total);
	for(loop2 = 0; loop2 < total; loop2++, loop1 = (++loop1) % total)
	{
		pHot = &hotSpotArray[loop1];

		if(AMax(abs(startX - pHot->x), abs(startY - pHot->y)) < 5)
		{
			continue;
			// too close
		}
		if(fCaution)
		{
			totalFriends = pWorld->unitLib.CountUnits(pHot->x, pHot->y,
				friendlySide, 3);
			if(!totalFriends || totalFriends >= (eCaution / 33 + 1))
			{
				// either not enough, or too many friends
				pHot = NULL;
				continue;
			}
		}
		// okay, take this one
		break;
	}
	return(pHot);
}

MapSpace*
AI::FindClosestBldg(funcType bldgType, int side, int orgX, int orgY)
{
	int		distance, bestDistance;
	MapSpace	*pBestBldg, *pBldg1;
	List		*pList;
	grip		gBldg;

	pList = &pWorld->map.lBuildings[side];

	pBestBldg = NULL;
	bestDistance = 9999;
	gBldg = (grip)pList->FirstValue();
	while(gBldg)
	{
		pBldg1 = ADerefAs(MapSpace, gBldg);
		gBldg = (grip)pList->NextValue();
		if(bldgType != pBldg1->h.func && bldgType != TOTALFUNCTIONS)
			continue;

		// if looking for any building, assume they mean any attackable building	
		if(bldgType == TOTALFUNCTIONS && !pBldg1->h.currHP)
			continue;

		distance = MapDistance(orgX, orgY, pBldg1->h.xPos, pBldg1->h.yPos);
		if(!pBestBldg || (distance < bestDistance && CheckWits()))
		{
			pBestBldg = pBldg1;
			bestDistance = distance;
		}
	}

	return(pBestBldg);
}

// find a random building in the building arrays, given the passed side.
// if no building is found, return NULL
MapSpace*
AI::FindRandomBldg(funcType bldgType, int side)
{
	MapSpace	*pBldg;
	List		*pList;
	grip		gBldg;

	if(bldgType);

	pList = &pWorld->map.lBuildings[side];

	gBldg = (grip)pList->At(ARandom(pList->count));
	pBldg = ADerefAs(MapSpace, gBldg);
	return(pBldg);
}

// Check if the current enemy ai would do this
bool
AI::CheckAggression()
{
	int	hisMod, ourMod, ours, his;
	bool	answer;

	// scale aggressiveness proportionally - if he has twice as many units as we do,
	// double our aggression.
	// and vice versa: if we outnumber him 3-to-1, cut our aggression to 1/3

	ours = aiTable[friendlySide].currFighters;
	his = aiTable[enemySide].currFighters;

	hisMod = AMax(1, ours);
	ourMod = AMax(1, his);
	answer = ((ourMod * ARandom(100)) < (hisMod * eAggression))? TRUE: FALSE;
	return(answer);
}

// Check if the current enemy ai would do this
bool
AI::CheckWits()
{
	int chance = ARandom(100);

	return ((chance < eWits) ? TRUE : FALSE);
}

// Check if the current enemy ai would do this
bool
AI::CheckCaution()
{
	return ((ARandom(100) < eCaution) ? TRUE : FALSE);
}

// Check if the current enemy ai would do this
bool
AI::CheckTerrainIQ()
{
	return ((ARandom(100) < eTerrainIQ) ? TRUE : FALSE);
}

// Check if the current enemy ai would do this
bool
AI::CheckItemIQ()
{
	return ((ARandom(100) < eItemIQ) ? TRUE : FALSE);
}

// Check if the current enemy ai would do this
bool
AI::CheckSpellIQ()
{
	return ((ARandom(100) < eSpellIQ) ? TRUE : FALSE);
}

// decide if we should cheat
bool
AI::CheckCheat(int cheatNum)
{
	return((ARandom(100) < cheatChances[cheatNum])? TRUE: FALSE);
}

void
AI::AddUnitPref(int side, unitType newType, int percentage)
{
	int	loop1;

	if(totalTransformPrefs[side] >= UNITMAXTYPES)
	{
		#ifndef NDEBUG
		pMono->Out("AI:AddUnitPref(%d, %d, %d) - capacity exceeded\n", side, (int)newType, percentage);
		#endif
		return;
	}
	if(!percentage)
		return;

	pMono->Out("AddUnitPref(%d, %s, %d%%)\n", side,
		pWorld->unitLib.GetUnitGroup(newType)->unitName, percentage);

	// see if an entry already exists	
	for(loop1 = 0; loop1 < totalTransformPrefs[side]; loop1++)
	{
		if(transformPrefs[side][loop1][0] == newType)
		{
			// update existing entry
			transformPrefs[side][loop1][1] = percentage;
			return;
		}
	}
	transformPrefs[side][totalTransformPrefs[side]][0] = newType;
	transformPrefs[side][totalTransformPrefs[side]][1] = percentage;
	totalTransformPrefs[side]++;
}

void
AI::SetAggression(int aggression)
{
	eAggression = aggression;
}

void
AI::SetWits(int wits)
{
	eWits = wits;
}

void
AI::SetCaution(int caution)
{
	eCaution = caution;
}

void
AI::SetTerrainIQ(int terrainIQ)
{
	eTerrainIQ = terrainIQ;
}

void
AI::SetItemIQ(int itemIQ)
{
	eItemIQ = itemIQ;
}

void
AI::SetSpellIQ(int spellIQ)
{
	eSpellIQ = spellIQ;
}

bool
AI::BumpUnit(Unit	*pUnit)
{
	int	unitX, unitY, newX, newY;

	unitX = pUnit->tileX;
	unitY = pUnit->tileY;

	pWorld->map.GetSurroundingCoords(unitX, unitY, 2, newX, newY, TRUE);
	do
	{
		if(!pUnit->fAutoControl && pUnit->CanIMoveTo(newX, newY, TRUE))
		{
			switch(ARandom(6))
			{
				case 0:
				case 1:
				case 2:
					// step aside, son.  (He'll come back if his primaryAction desires)
					pUnit->SetAction(MOVE_TO, newX, newY);
					break;

				case 3:
				case 4:
					// just in case it helps
					ClearAIMission(pUnit);
					pUnit->SetPrimaryAction(MOVE_TO, aiTable[pUnit->player].portalX,
						aiTable[pUnit->player].portalY);
					break;

				case 5:
					ClearAIMission(pUnit);
					pUnit->SetPrimaryAction(DISBAND);
					break;

			}
			return(TRUE);
		}
	}
	while(pWorld->map.GetSurroundingCoords(unitX, unitY, 1, newX, newY));
	return(FALSE);
}

// See if the ai wants the unit to do anything based on the attack.
// If action is taken, return TRUE, else return FALSE

bool			
AI::HitUnit(Unit *pHitUnit, Unit *pAttacker)
{
	int	turnsToKill, turnsToDie;
	bool  fCanInterruptUnit;
	Unit	*pCurUnitToHit;
	MapSpace	*pBldg;

	// only take care of computer controlled units
	if(pBam->playerTypes[pHitUnit->player] != PLAYER_COMPUTER)
		return(FALSE);

	// add to a help attack/repair list
	// NOTE: REMEMBER TO CHECK THE SERIAL NUMBERS BEFORE USING.
	// THE UNIT MAY DIE BEFORE THE LIST IS LOOKED AT.
	AddDelayedAction(pHitUnit->player, ADT_UNIT, pHitUnit->serialNum, ADT_UNIT, pAttacker->serialNum);
	// if we are not attacking..then we are now
//	attackNow[pHitUnit->player] = TRUE;	

	if(pHitUnit->currAction == TRANSFORM || pHitUnit->currAction == SKILL || pHitUnit->currHP < 1 ||
		pHitUnit->fAutoControl)
	{
		// certain activities should not be interrupted
		return(FALSE);
	}

	// do we have an item we can use?
	if(pHitUnit->currHP < 10 && CanUseItemToHeal(pHitUnit->item, pHitUnit) &&
		CheckSpellIQ() && CheckSpellIQ() && CheckSpellIQ())
	{
		// we can heal ourself
		pHitUnit->UseItem();
		ReportUnit(pHitUnit, "HIT:ITEM_HEAL", TRUE);
		return(TRUE);
	}
	
	// is AI trying to bulldoze through a neutral building to reach an otherwise unreachable target?
	if(pHitUnit->currAction == MOVE_TO && pHitUnit->primaryAction == ATTACK && pHitUnit->primaryTargetType == BUILDING)
	{
		pBldg = (MapSpace *)pHitUnit->ValidatePrimaryTarget();
		if(pBldg && pBldg->h.owner == SIDE0)
		{
			// looks like it!  Let him continue
			return(FALSE);
		}
	}

	// cowardice check
	if((pHitUnit->currAction == MOVE_TO || pHitUnit->currAction == ATTACK || pHitUnit->currAction == GUARD)
		&& pHitUnit->currHP < 8 && CheckWits() && CheckWits() && ARandom(4) == 1)
	{
		// weak and in danger - is time-to-kill-him greater than time-to-kill-me?
		turnsToKill = pAttacker->currHP / AMax((pHitUnit->currAttack - pAttacker->currDefense), 1);
		turnsToDie = pHitUnit->currHP / AMax((pAttacker->currAttack - pHitUnit->currDefense), 1);
		if(turnsToKill > turnsToDie)
		{
			#ifndef NDEBUG
			pMono->Out("AI:: %s(%d,%d) losing (2kill=%d 2die=%d), ", pHitUnit->unitName, pHitUnit->tileX,
				pHitUnit->tileY, turnsToKill, turnsToDie);
			#endif
			if(pHitUnit->currHP < 3 && CheckWits() && CheckWits())
			{
				// this guy is too far gone.  disband for mana
				pHitUnit->SetPrimaryAction(DISBAND);
				ReportUnit(pHitUnit, "HIT:DISBAND", TRUE);
				return(TRUE);
			}
			// else if not already retreating, and we're not already at the portal
			else if(pHitUnit->ai.action != AI_RETREAT &&
				MapDistance(pHitUnit->tileX, pHitUnit->tileY, aiTable[pHitUnit->player].portalX,
					aiTable[pHitUnit->player].portalY) > 3)
			{
				// run away!  If aid is avail, hopefully it will come to us
				pHitUnit->fStopAdjacent = TRUE;
				pHitUnit->SetPrimaryAction(MOVE_TO, pWorld->map.portalCoords[pHitUnit->player][0],
					pWorld->map.portalCoords[pHitUnit->player][1]);
				pHitUnit->ai.action = AI_RETREAT;
				pHitUnit->ai.data = 0;
				ReportUnit(pHitUnit, "HIT:RETREAT", TRUE);
				return(TRUE);
			}
		}
	}

	// can we heal ourselves with our skill?
	if (pHitUnit->currHP < (pHitUnit->maxHP - 5) && CheckSpellIQ() && ARandom(3) &&
		(pHitUnit->type == CLERIC || pHitUnit->type == PALADIN) && pHitUnit->skillCost <=
		pWorld->mana[pHitUnit->player])
	{
		pHitUnit->SetAction(SKILL, pHitUnit->tileX, pHitUnit->tileY);
		ReportUnit(pHitUnit, "HIT:SKILL_HEAL", TRUE);
		return(TRUE);
	}

	// do we want to retalliate?

	// ignore the juggernaut
	if (pAttacker->type != JUGGERNAUT && pHitUnit->ai.action != AI_RETREAT)
	{
		if ((pHitUnit->type == CLERIC || pHitUnit->type == PALADIN) && pHitUnit->currAction == SKILL)
		{
			// we are trying to heal.  do nothing
		}
		else if(pHitUnit->ai.action != AI_RETREAT)	// if not already retreating
		{
			// are we smart enough to check for tricky stuff?
			if (CheckWits())
			{
				if (pHitUnit->currAction == ATTACK && pHitUnit->secondaryTargetType == UNIT)
				{
					// we are currently attacking someone	
					pCurUnitToHit = (Unit *) pWorld->DerefSerial(pHitUnit->secondaryTargetSerial);
					if (pCurUnitToHit &&	pCurUnitToHit != pAttacker &&
					   (pCurUnitToHit->currHP + pHitUnit->currAttack) > pAttacker->currHP)
					{
						// Hey!  this guy that hit me is weaker.  hit him now :)
						pHitUnit->SetAction(ATTACK, pAttacker->tileX, pAttacker->tileY);
						ReportUnit(pHitUnit, "HIT:RETAL", TRUE);
						return(TRUE);
					}
				}
			}

			// if we are not already fighting a unit, and not already headed for
			// this one, turn and attack
			fCanInterruptUnit = TRUE;

			if (fCanInterruptUnit)
			{
				if(pHitUnit->primaryTargetSerial == pAttacker->serialNum)
				{
					// we're already fighting that guy
					fCanInterruptUnit = FALSE;
				}
				else if(pHitUnit->currAction == ATTACK && pHitUnit->secondaryTargetType == UNIT)
				{
					// we've already got someone to fight
					fCanInterruptUnit = FALSE;
				}
				else if(pHitUnit->primaryTargetType == UNIT && pHitUnit->GetDistanceTo(pHitUnit->primaryX,
					pHitUnit->primaryY) <= pHitUnit->GetDistanceTo(pAttacker->tileX, pAttacker->tileY))
				{
					// we're already almost to our intended target, which is a unit also.
					// dont worry about this other guy
					fCanInterruptUnit = FALSE;
				}
				else if(pHitUnit->type == ACOLYTE && pHitUnit->currAction == MOVE_TO)
				{
					fCanInterruptUnit = FALSE;
				}
			}

			if(fCanInterruptUnit && CheckAggression() && CheckAggression() && CheckAggression())
			{
				// diversion from original goal
				ClearAIMission(pHitUnit);
				pHitUnit->SetPrimaryAction(ATTACK, pAttacker->tileX, pAttacker->tileY);
				ReportUnit(pHitUnit, "HIT:INTERR_RETAL", TRUE);
				return(TRUE);
			}
		}
	}
	return(FALSE);
}


// See if the ai wants the unit to do anything based on the attack.
// If action is taken, return TRUE, else return FALSE
bool			
AI::HitBldg(MapSpace *pHitBldg, Unit *pAttacker)
{
	WinningCondition	*pWinCon;
	int	side = -1;		
	bool	actionTaken = FALSE;
	int	i;

	// one of our buildings?
	if (pBam->playerTypes[pHitBldg->h.owner] == PLAYER_COMPUTER)
	{
		side = pHitBldg->h.owner;
	}
	// else neutral building?
	else if(pHitBldg->h.owner == SIDE0 || pHitBldg->h.owner == SIDE3)
	{
		// check to see if this bldg is specified in the enemy's win conditions.
		// we only care if this is a 3rd party building that we need to defend.
		// process all win conditions
		for(i = 0; i < MAX_WINCONS; i++)
		{
			pWinCon = &pWorld->winCons[i];

			// if wincon[i] owned by AI, skip it
			if(pBam->playerTypes[pWinCon->ownerSide] == PLAYER_COMPUTER)
			{
				// BUGBUG: in AI-vs-AI there will be no response in this case, but it's
				// a fairly obsure case anyway.
				continue;
			}

			if (pWinCon->targetSpec == BUILDING && pWinCon->targetSide == pHitBldg->h.owner &&
				(pWinCon->structSpec == pHitBldg->h.func || pWinCon->structSpec == TOTALFUNCTIONS))
			{
				// find the opposite side.  the one that must defend against the attacker.
				if (pWinCon->ownerSide == SIDE1)
				{
					side = SIDE2;
				}
				else
				{
					side = SIDE1;
				}
			}
		}
	}

	// add to a help attack/repair list?
	// NOTE: REMEMBER TO CHECK THE SERIAL NUMBERS BEFORE USING.
	// THE UNIT MAY DIE BEFORE THE LIST IS LOOKED AT.
	if (side != -1)
	{
		AddDelayedAction(side, ADT_BLDG, pHitBldg->serialNum, ADT_UNIT, pAttacker->serialNum);

		// if we are not attacking..then we are now
//		attackNow[side] = TRUE;
	}

	return actionTaken;	
}


// See if the ai wants the unit to do anything based on the attack.
// If action is taken, return TRUE, else return FALSE
bool			
AI::HitTerrain(Unit *pHitUnit, TerrainUnit *pAttacker)
{
	bool	actionTaken = FALSE;
	pAttacker = pAttacker;
	pHitUnit = pHitUnit;

	// only take care of computer controlled units
	if (pBam->playerTypes[pHitUnit->player] == PLAYER_COMPUTER)
	{
		if (pHitUnit->currHP < 10 && CanUseItemToHeal(pHitUnit->item, pHitUnit))
		{
			// we are carrying an and can heal ourself
			pHitUnit->UseItem();
			ReportUnit(pHitUnit, "HIT_TERR:ITEM HEAL", TRUE);
		}

		if (pHitUnit->currHP < 10 &&
			 CheckSpellIQ() &&
			(pHitUnit->type == CLERIC ||
			 pHitUnit->type == PALADIN)  &&
			 pHitUnit->skillCost <= pWorld->mana[pHitUnit->player] &&
			 pHitUnit->currAction != SKILL)
		{
			pHitUnit->SetAction(SKILL, pHitUnit->tileX, pHitUnit->tileY);
			ReportUnit(pHitUnit, "HIT_TERR:SKILL HEAL", TRUE);
			actionTaken = TRUE;
		}
		else
		{
			if ((pHitUnit->type == CLERIC ||
			 	  pHitUnit->type == PALADIN)  &&
				  pHitUnit->currAction == SKILL)
			{
				// we are trying to heal.  do nothing
			}
			else
			{
				if (pHitUnit->currAction == GUARD &&
					 pHitUnit->fPrimarySatisfied)
				{
					// this idiot is standing on dangerous terrain.
					// get out of here!
//					pHitUnit->SetPrimaryAction(ATTACK, pAttacker->tileX, pAttacker->tileY);
//					pHitUnit->ai.action = AI_NONE;
//					actionTaken = TRUE;
				}
			}
		}
	}

	return actionTaken;	
}

void
AI::AddDelayedAction(int side, ai_delay_type gotHitType, uint32 gotHitSerialNum,
							ai_delay_type hitByType, uint32 hitBySerialNum)
{
	int					i;
	int					count = delayedActionCount[side];
	ai_delay_action	*pAction;

	if (count >= AI_MAX_DELAY)
	{
		#ifndef NDEBUG
		pMono->Out("AI Delayed Action List Full! Increase Size of Array.\n");
		#endif
		return;
	}

	// don't add duplicates.  search
	for(i = 0, pAction = &delayedActions[side][0]; i < count; i++, pAction++)
	{
		if (pAction->gotHitSerialNum == gotHitSerialNum &&
			 pAction->hitBySerialNum == hitBySerialNum)
		{
			// this is already in the list
			return;
		}
	}

	if (count == 0 || i == count)
	{
		// this is a new one
		pAction->gotHitType =		gotHitType;
		pAction->gotHitSerialNum = gotHitSerialNum;
		pAction->hitByType =			hitByType;
		pAction->hitBySerialNum =	hitBySerialNum;
		delayedActionCount[side]++;
	}
}

