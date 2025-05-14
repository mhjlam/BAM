// MAPS.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Desc: ?
//
// Date Started: ?
//


//--[Map class]-------------------------------------------------------------
#include "dos.h"	// for sleep()

#include "apires.hpp"
#include "tigre.hpp"
#include "eventmgr.hpp"
#include "savebase.hpp"

#include "alldefs.hpp"
#include "tilelib2.hpp"
#include "maps.hpp"
#include "units.hpp"
#include "world.hpp"
#include "viewport.hpp"

extern World	*pWorld;
extern char		szFuncNames[][20];
extern int		offsets[][2];

//extern int debugLine;
//extern char *debugFile;

int HPPerTile[TOTALFUNCTIONS] =
{40, 60, 90, 75, 75, 60, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 111};

// Map constructor - initialize to safe defaults
Map::Map(void)
{
	totalHotSpots = 0;

	lBuildings[SIDE0].resizable = TRUE;
	lBuildings[SIDE1].resizable = TRUE;
	lBuildings[SIDE2].resizable = TRUE;
	lBuildings[SIDE3].resizable = TRUE;
}

Map::~Map()
{
	int loop1;

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		lBuildings[loop1].Release();

	AFlush(ALoad(RES_MAP_INFO, rMapNum));
}

bool
Map::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&mapDataStart, 1, (int)&mapDataEnd -
				(int)&mapDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&mapDataStart, 1, (int)&mapDataEnd -
				(int)&mapDataStart, pFile);
			break;
	}
	return(TRUE);
}

bool
Map::IsTerrainHidden(uint32 MapX, uint32 MapY)
{
	// obviously this still needs to be written
	return((pWorld->vPort.fog[MapY][MapX] & FOG_CENTER) ? TRUE : FALSE);
}

bool
Map::Load(uint rNewMapNum)
{
	int		loop1, loop2;
	grip		gMap;
	struct	MapInfoRes	*pMapInfo;
	MapSpace	*pSpace;
	struct	spaceInfo	*pSpaceInfo;

	for(loop1 = 0; loop1 < MAP_TOTAL_SWAP_SERIES; loop1++)
		for(loop2 = 0; loop2 < MAP_SWAP_SERIES_LENGTH; loop2++)
			swapSeries[loop1][loop2] = 0;
	totalSwapSeries = 0;

	rMapNum = rNewMapNum;
	if(rMapNum % 10 == 2)
		fLegendary = TRUE;
	else fLegendary = FALSE;

	if(rMapNum == 9430)
		bGlobal.storyLine = TUTORIAL;

	// this is a rather large allocation, so take some extra care.
	gMap = ALoadDebug(__FILE__, __LINE__, RES_MAP_INFO, rNewMapNum, TRUE);
	if(!gMap)
	{
		pMono->Out("Map::Load(%d) - ALoadDebug(__FILE__, __LINE__, ) failed!\n", rMapNum);
		return(FALSE);
	}

	pMapInfo = (MapInfoRes *)AGetResData(gMap);

	memcpy(MapTiles, pMapInfo->mapTiles, sizeof(MapTiles));
	memcpy(MapSpaces, pMapInfo->mapSpaces, sizeof(MapSpaces));

	// load MapSpace's with info from resource
	memset(gSpaces, NULL, sizeof(gSpaces));

	h.totalSpaces = (uint16)pMapInfo->totalSpaces;
	for(loop1 = 0; loop1 < h.totalSpaces; loop1++)
	{
		TRACK_MEM("MapSpace");	pSpace = new MapSpace(FUNC_MISC);
		gSpaces[loop1] = pSpace->gSelf;
		pSpaceInfo = &pMapInfo->spaces[loop1];

		pSpace->h.func = (uint32)pSpaceInfo->func;
		pSpace->h.xPos = pSpaceInfo->xPos;		// if these are invalid, they will be correected
		pSpace->h.yPos = pSpaceInfo->yPos;		// in Map::FixupClusters() for run-time initialization
		pSpace->h.hotSpot = pSpaceInfo->fHotSpot;
		pSpace->h.presetItem = pSpaceInfo->presetItem;
		pSpace->h.fPickRandomItem = pSpaceInfo->fPickRandomItem;
		pSpace->h.fSpecialItemDrop = pSpaceInfo->fSpecialItemDrop;
		pSpace->h.destX = pSpaceInfo->destX;
		pSpace->h.destY = pSpaceInfo->destY;
		pSpace->h.owner = pSpaceInfo->owner;
		pSpace->h.maxHP = pSpaceInfo->maxHP;
		pSpace->h.currHP = 0;

		if(pSpace->IsFoundation())
		{
			pSpace->h.func = FUNC_GEN_FOUND;
		}

		if(pSpace->h.maxHP > 0 || pSpace->IsFoundation() ||
			pSpace->h.func == FUNC_IMM_WELL || pSpace->h.func == FUNC_CAULDRON || pSpace->h.owner == SIDE1 ||
			pSpace->h.owner == SIDE2 || pSpace->h.func == FUNC_MINESHAFT || pSpace->h.func == FUNC_PRISON_PIT)
		{
			// assign a serial # for targeting purposes
			pSpace->serialNum = pWorld->GetNextSerialNum(BUILDING, pSpace->gSelf);

			// add the bldg to someone's list
			switch(pSpace->h.owner)
			{
				case SIDE2:
				case SIDE1:
					lBuildings[pSpace->h.owner].Add((void *)pSpace->gSelf);
					break;

				default:
					lBuildings[SIDE0].Add((void *)pSpace->gSelf);
					break;
			}
		}
	}
	return(TRUE);
}

void
Map::UpdateAllTurretTiles(MapSpace *pWall)
{
	WorldMap	*pWM = ADerefAs(WorldMap, pWorld->gWorldMap);
	int		totalTiles, loop1, tempX, tempY, viewX, viewY, turretType;
	tile		*pTile;

	totalTiles = pWorld->pTileLib->totalTiles;
	viewX = pWorld->vPort.ViewX;
	viewY = pWorld->vPort.ViewY;

	for(loop1 = 0; loop1 < pWall->size; loop1++)
	{
		tempX = pWall->tiles[loop1][0];
		tempY = pWall->tiles[loop1][1];

		// check if tile is freshly converting to a turret
		pTile = pWorld->map.GetTile(tempX, tempY);
		if(!ABetween(pTile->terrain, TURRET, TURRET15))
		{
			// store original tile for restoration upon turret death
			pWall->oldTileNums[loop1] = GetTileNum(tempX, tempY);
		}

		turretType = FOG_NULL;
		if(GetStructure(tempX - 1, tempY) == pWall)
			turretType |= FOG_WEST;
		if(GetStructure(tempX + 1, tempY) == pWall)
			turretType |= FOG_EAST;
		if(GetStructure(tempX, tempY - 1) == pWall)
			turretType |= FOG_NORTH;
		if(GetStructure(tempX, tempY + 1) == pWall)
			turretType |= FOG_SOUTH;

		pWorld->map.SetTile(pWorld->vPort.turretTiles[turretType], (uint16)tempX, (uint16)tempY);
		if(pWorld->vPort.InViewPort(tempX, tempY, NOTARGET))
			pWorld->vPort.Draw(tempX - viewX, tempY - viewY, tempX - viewX, tempY - viewY);
		pWM->Draw(tempX, tempY);
	}
}

void
Map::FixupClusters(void)
{
	int		loop1, loop2, loopX, loopY;
	MapSpace	*pSpace;
	terrainType	terrain;
	grip		gSpace;

	memset(portalCoords, (uchar)-1, sizeof(portalCoords));

	// fill each cluster with list of X,Ys it owns, and its own total size
	// note: done in reverse order, so that top-left coords will be left in x.hPos and y.yPos
	for(loopY = WORLD_HEIGHT - 1;	loopY >= 0; loopY--)
		for(loopX = WORLD_WIDTH - 1; loopX >= 0; loopX--)
		{
			pSpace = GetStructure(loopX, loopY);
			if(pSpace)
			{
				pSpace->AddTile(loopX, loopY);
				pSpace->h.xPos = loopX;
				pSpace->h.yPos = loopY;

				// note these for later reference
				if(pSpace->h.func == FUNC_PORTAL)
				{
					portalCoords[pSpace->h.owner][0] = pSpace->h.xPos;
					portalCoords[pSpace->h.owner][1] = pSpace->h.yPos;
				}
			}
		}

	for(loop1 = 0;	loop1 < h.totalSpaces; loop1++)
	{
		if(!gSpaces[loop1])
			continue;

		pSpace = ADerefAs(MapSpace, gSpaces[loop1]);
		
		// some hardcoded Vas features
		switch(rMapNum)
		{
			case 9312:
			case 9315:
				// the following clusters (trip switches or pressure plates)
				// replace themselves with different clusters upon activation
				switch(loop1)
				{
					case 4:
						pSpace->h.swapClusterNum = 126;
						break;
					case 126:
						pSpace->h.swapClusterNum = 127;
						break;
					case 127:
						pSpace->h.swapClusterNum = 128;
						break;
					case 128:
						pSpace->h.swapClusterNum = 4;
						break;
					default:
						pSpace->h.swapClusterNum = 0;
						break;
				}
				break;

			case 9310:
				// the following clusters (trip switches or pressure plates)
				// replace themselves with different clusters upon activation
				switch(loop1)
				{
					case 4:
						pSpace->h.swapClusterNum = 130;
						break;
					case 130:
						pSpace->h.swapClusterNum = 131;
						break;
					case 131:
						pSpace->h.swapClusterNum = 132;
						break;
					case 132:
						pSpace->h.swapClusterNum = 4;
						break;
					default:
						pSpace->h.swapClusterNum = 0;
						break;
				}
				break;

			default:
				pSpace->h.swapClusterNum = 0;
				break;
		}

		if(pSpace->h.hotSpot)
		{
			// add it to list of strategic spots
			gHotSpots[totalHotSpots++] = pSpace->gSelf;
		}

		if(pSpace->h.func == FUNC_MISC)
		{
			// auto-code function as necessary, add it to buildings list
			for(loop2 = 0; loop2 < pSpace->size; loop2++)
			{
				terrain = (terrainType)GetTerrainNum(pSpace->tiles[loop2][0], pSpace->tiles[loop2][1]);
				switch(terrain)
				{
					case WELL_OF_IMMORTALS:
						pSpace->serialNum = pWorld->GetNextSerialNum(BUILDING, pSpace->gSelf);
						pSpace->h.func = FUNC_IMM_WELL;
						lBuildings[SIDE0].Add((void *)pSpace->gSelf);
						break;

					case FOUND_ARBORLODGE:
					case FOUND_BARRACKS:
					case FOUND_CRYPT:
					case FOUND_RUNESTONE:
					case FOUND_TEMPLE:
					case FOUND_GENERIC:
						pSpace->h.func = FUNC_GEN_FOUND;
						break;

					case KEEP:
						pSpace->h.func = FUNC_KEEP;
						break;

					case GATE:
						pSpace->h.func = FUNC_GATE;
						break;

					case AQUEDUCT:
						pSpace->h.func = FUNC_AQUEDUCT;
						break;

					case CAULDRON:
						pSpace->h.func = FUNC_CAULDRON;
						pSpace->serialNum = pWorld->GetNextSerialNum(BUILDING, pSpace->gSelf);
						lBuildings[pSpace->h.owner].Add((void *)pSpace->gSelf);		// add it to the Building list
						break;

					case CAMP:
						pSpace->h.func = FUNC_CAMP;
						pSpace->serialNum = pWorld->GetNextSerialNum(BUILDING, pSpace->gSelf);
						lBuildings[pSpace->h.owner].Add((void *)pSpace->gSelf);		// add it to the Building list
						break;
				}
				if(pSpace->h.func != FUNC_MISC)
					break;	// stop searching
			}
		}
	}

	// clean up lBuildings - remove any invalid clusters
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		gSpace = (grip)lBuildings[loop1].FirstValue();
		while(gSpace)
		{
			pSpace = ADerefAs(MapSpace, gSpace);
			if(GetStructure(pSpace->h.xPos, pSpace->h.yPos) != pSpace)
			{
				lBuildings[loop1].Delete((void *)gSpace);
			}
			else
			{
				// set initial HP level
				if(pSpace->h.maxHP && !pSpace->IsFoundation())
					pSpace->h.currHP = pSpace->h.maxHP;
				else pSpace->h.currHP = 0;
				memset(pSpace->defenders, NULL, sizeof(pSpace->defenders));
			}
			gSpace = (grip)lBuildings[loop1].NextValue();
		}
	}
}

void
Map::ClearFoundationOwnerships(void)
{
	// this was done in Map::Load() before, but now Legendary maps require pre-coded ownership
	// of foundations for run-time randomization.  So we just clean up owned foundations here instead,
	// after any Legendary randomization may have occured.
	int		loop1;
	grip		gSpace;
	MapSpace	*pSpace;

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		gSpace = (grip)lBuildings[loop1].FirstValue();
		while(gSpace)
		{
			pSpace = ADerefAs(MapSpace, gSpace);
			if(pSpace->h.func == FUNC_GEN_FOUND)
			{
				if(loop1 != SIDE0)
				{
					lBuildings[pSpace->h.owner].Delete((void *)pSpace->gSelf);
					lBuildings[SIDE0].Add((void *)pSpace->gSelf);
				}
				pSpace->h.owner = SIDE0;
			}
			gSpace = (grip)lBuildings[loop1].NextValue();
		}
	}
}

void
Map::ApplyRandomChanges(void)
{
	grip		gSpace;
	MapSpace	*pSpace;
	int		loop1, maxPlayerUseful, totalPlayerUseful, totalNeutral;
	KeyList	lSites[TOTAL_SIDES];

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		lSites[loop1].resizable = TRUE;

	totalNeutral = 0;

	// first, collect all available sites, sorted by distance to portal
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		gSpace = (grip)lBuildings[loop1].FirstValue();
		while(gSpace)
		{
			pSpace = ADerefAs(MapSpace, gSpace);
			switch(pSpace->h.func)
			{
				case FUNC_GEN_FOUND:
				// add to list of sites for that side, keyed by distance to home portal
				if(loop1 != SIDE2 && loop1 != SIDE1)
				{
					totalNeutral++;
				}
				
				if(loop1 != SIDE2)
				{
					lSites[SIDE1].Add((void *)pSpace, AMax(abs(pSpace->h.xPos - portalCoords[SIDE1][0]),
					abs(pSpace->h.yPos - portalCoords[SIDE1][1])));
				}

				if(loop1 != SIDE1)
				{
					lSites[SIDE2].Add((void *)pSpace, AMax(abs(pSpace->h.xPos - portalCoords[SIDE2][0]),
					abs(pSpace->h.yPos - portalCoords[SIDE2][1])));
				}
				break;
			}
			gSpace = (grip)lBuildings[loop1].NextValue();
		}
	}
	
	if(bGlobal.storyLine == NETGAME)
	{
		ApplyRandomChangesForSide(SIDE1, &lSites[SIDE1], 1, 1);
		ApplyRandomChangesForSide(SIDE2, &lSites[SIDE2], 1, 1);
	}
	else
	{
		// player cannot have more than computer can have
		maxPlayerUseful = lSites[SIDE2].count - totalNeutral / 2 - totalNeutral % 2;
		totalPlayerUseful = 0;

		totalPlayerUseful = ApplyRandomChangesForSide(SIDE2, &lSites[SIDE2], 1, lSites[SIDE2].count);
		if(totalPlayerUseful > 1)
			ApplyRandomChangesForSide(SIDE1, &lSites[SIDE1], 0, totalPlayerUseful - 1);
	}

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		lSites[loop1].Release();
}

bool
Map::GetSurroundingCoords(int targetX, int targetY, int radius, int &returnX, int &returnY, bool fStart)
{
	static int	startOffset, currOffset, linePos, lineMaxPos;
	int	testX, testY, cornerX, cornerY;

	if(fStart)
	{
		if(radius == 0)
		{
			returnX = targetX;
			returnY = targetY;
			return(TRUE);
		}

		currOffset = ARandom(4) * 2 + 1;
		startOffset = currOffset;
		lineMaxPos = radius * 2;
		linePos = 0;
	}
	else if(radius == 0)
	{
		return(FALSE);
	}

	while(linePos >= lineMaxPos)
	{
		// we're at a corner - change direction
		currOffset = (currOffset + 2) % 8;
		if(currOffset == startOffset)
		{
			// we've gone around the world - no more options
			return(FALSE);
		}
		linePos = 0;
	}

	cornerX = targetX + offsets[(currOffset + 5) % 8][0] * radius;
	cornerY = targetY + offsets[(currOffset + 5) % 8][1] * radius;
	testX = cornerX + offsets[currOffset][0] * linePos;
	testY = cornerY + offsets[currOffset][1] * linePos;
	linePos++;

	// clip to world edges
	testX = AMin(WORLD_WIDTH - 1, testX);	testX = AMax(0, testX);
	testY = AMin(WORLD_HEIGHT - 1, testY);	testY = AMax(0, testY);

	returnX = testX;
	returnY = testY;
	return(TRUE);
}

int
Map::ApplyRandomChangesForSide(int newSide, KeyList *pSites, int minCount, int maxCount)
{
	MapSpace	*pSpace, *pSpace2;
	grip		gSpace;
	int		totalUseful, tileNum, loop1, loop2, seriesNum, seriesStart, percUseful, percFound, percNeutral;
	
	totalUseful = 0;

	// scaled to generate a trend from high percUseful (70) down to low percUseful (10) for SIDE1,
	//	mirrored by percFound,
	// and the opposite for SIDE2
	percNeutral = 20;
	percUseful = 70 - (60 * ((bGlobal.missionsDone * 100) / 15)) / 100;
	percFound = 100 - percUseful - percNeutral;
	if(newSide == SIDE2)
	{
		int temp;

		// swap to give SIDE2 an opposing trend
		temp = percUseful;
		percUseful = percFound;
		percFound = temp;
	}

	pSpace = (MapSpace *)pSites->FirstValue();
	while(pSpace && totalUseful < maxCount)
	{
		// validate pSpace
		if(pSpace->h.func != FUNC_GEN_FOUND)
		{
			// skip to next
			pSpace = (MapSpace *)pSites->NextValue();
			continue;
		}

		// find the swap series entry for the tile set used by this cluster
		tileNum = GetTileNum(pSpace->h.xPos, pSpace->h.yPos);
		for(loop1 = 0; loop1 < totalSwapSeries; loop1++)
		{
			if(swapSeries[loop1][0] == tileNum)
			{
				// decide what we'll swap it to
				if(bGlobal.storyLine == NETGAME && newSide == SIDE2)
				{
					// use same type of bldg that SIDE1 already has
					gSpace = (grip)lBuildings[SIDE1].FirstValue();
					do
					{
						if(!gSpace)
							pSpace2 = NULL;
						else
						{
							pSpace2 = ADerefAs(MapSpace, gSpace);
						}
						gSpace = (grip)lBuildings[SIDE1].NextValue();
					} while(pSpace2 && !pSpace2->IsSpecialBuilding());
					
					if(!pSpace2)
						break;	// something went wrong

					switch(pSpace2->h.func)
					{
						case FUNC_ARBORLODGE:
							seriesNum = 1;
							break;

						case FUNC_BARRACKS:
							seriesNum = 2;
							break;

						case FUNC_CRYPT:
							seriesNum = 3;
							break;

						case FUNC_RUNESTONE:
							seriesNum = 4;
							break;

						case FUNC_TEMPLE:
							seriesNum = 5;
							break;

						default:	// error
							seriesNum = 3;	// safety default
							break;
					}

				}
				else do
				{
					if(totalUseful < minCount)
					{
						seriesNum = ARandom(5) + 1;	// only useful buildings
					}
					else
					{
						seriesNum = ARandom(100);
						if(seriesNum < percNeutral)
						{
							seriesNum = 6 + ARandom(3);	// only neutral structures
						}
						else if(seriesNum < percNeutral + percUseful)
						{
							seriesNum = ARandom(5) + 1;	// only useful blgs
						}
						else
						{
  							// leave as foundation (not grouped with NEUTRAL)
							seriesNum = 0;
						}
						seriesNum = ARandom(10);
					}
				} while(!swapSeries[loop1][seriesNum]);

				switch(seriesNum)
				{
					case 0:
						continue;	// leave alone
					case 1:
						totalUseful++;
						pSpace->h.func = FUNC_ARBORLODGE;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
					case 2:
						totalUseful++;
						pSpace->h.func = FUNC_BARRACKS;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
					case 3:
						totalUseful++;
						pSpace->h.func = FUNC_CRYPT;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
					case 4:
						totalUseful++;
						pSpace->h.func = FUNC_RUNESTONE;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
					case 5:
						totalUseful++;
						pSpace->h.func = FUNC_TEMPLE;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
					case 6:
						if(pSpace->h.owner == SIDE3 && newSide == SIDE2)
						{
							// can be allocated to either side, so let other side have a crack at it too
							seriesNum = 0;
							break;
						}
						pSpace->h.func = FUNC_MISC;
						pSpace->h.maxHP = 0;
						break;
					case 7:
						if(pSpace->h.owner == SIDE3 && newSide == SIDE2)
						{
							// can be allocated to either side, so let other side have a crack at it too
							seriesNum = 0;
							break;
						}
						pSpace->h.func = FUNC_MISC;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
					case 8:
						if(pSpace->h.owner == SIDE3 && newSide == SIDE2)
						{
							// can be allocated to either side, so let other side have a crack at it too
							seriesNum = 0;
							break;
						}
						pSpace->h.func = FUNC_HOME;
						pSpace->h.maxHP = HPPerTile[pSpace->h.func];
						break;
				}

				if(seriesNum > 0)
				{
					lBuildings[pSpace->h.owner].Delete((void *)pSpace->gSelf);
	
					if(seriesNum == 6)	// change to OPEN LAND
					{
						// do nothing here
					}
					else	// change to bldg type
					{
						// set HP
						pSpace->h.maxHP = pSpace->h.maxHP * pSpace->size;
						pSpace->h.currHP = pSpace->h.maxHP;
						if(pSpace->h.func == FUNC_MISC || pSpace->h.func == FUNC_HOME)
						{
							pSpace->h.owner = SIDE0;
						}
						else
						{
							pSpace->h.owner = newSide;
						}
						lBuildings[pSpace->h.owner].Add((void *)pSpace->gSelf);
					}

					// swap all tiles to new form (oh boy, wont this make the game fun!)
					seriesStart = swapSeries[loop1][seriesNum];
					for(loop2 = 0; loop2 < pSpace->size; loop2++)
					{
						SetTile(seriesStart + loop2, pSpace->tiles[loop2][0], pSpace->tiles[loop2][1]);
						if(seriesNum == 6)
						{
							// open land - can remove structure entirely
							SetStructure(NULL, pSpace->tiles[loop2][0], pSpace->tiles[loop2][1]);
						}
					}
				}
				// swap series found - can stop looking, so break out of loop
				break;
			}
		}
		pSpace = (MapSpace *)pSites->NextValue();
	}
	return(totalUseful);
}

void
Map::SetStructure(int32 structNum, uint32 mapX, uint32 mapY)
{
	uint32	mapPos;

	if(mapX >= WORLD_WIDTH || mapY >= WORLD_HEIGHT)
		return;	
	mapPos = mapX + WORLD_WIDTH * mapY;
	MapSpaces[mapPos] = (int16)structNum;
	return;
}

funcType
Map::StringToFunction(char *string)
{
	int	loop1;
	char	*str;

	str = string;
	while(*str)
	{
		if(*str == '_')
			*str = ' ';
		str++;
	}

	for(loop1 = 0; loop1 < TOTALFUNCTIONS; loop1++)
	{
		if(!strcmp(string, &szFuncNames[loop1][0]))
			return((funcType)loop1);
	}
	return((funcType)TOTALFUNCTIONS);
}

// return * to cluster occupying X,Y
MapSpace *
Map::GetStructure(uint32 mapX, uint32 mapY)
{
	uint32	mapPos, spaceNum;
	MapSpace	*pSpace;

	if(mapX >= WORLD_WIDTH || mapY >= WORLD_HEIGHT)
		return(NULL);	
	mapPos = mapX + WORLD_WIDTH * mapY;
	spaceNum = MapSpaces[mapPos];
	if(!spaceNum)
		return(NULL);
	pSpace = ADerefAs(MapSpace, gSpaces[spaceNum]);
	return(pSpace);
}

tile *
Map::GetTile(int xPos, int yPos)
{
	int	tileNum;

	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
		return(NULL);	

	tileNum = MapTiles[xPos + yPos * WORLD_WIDTH];
	return(&(pWorld->pTileLib->tiles[tileNum]));
}

terrainType
Map::GetTerrainType(char *szName)
{
	int	totalTerrains, loop1;
	char	str[80];

	strcpy(str, szName);
	strupr(str);

	if(!strcmp(str, "ANY") || !strcmp(str, "ALL"))
		return(TERRAINMAXTYPES);

	totalTerrains = pWorld->pTileLib->totalTerrains;
	for(loop1 = 0; loop1 < totalTerrains; loop1++)
	{
		strupr(pWorld->pTileLib->terrains[loop1].szName);
		if(!strcmp(str, pWorld->pTileLib->terrains[loop1].szName))
			return((terrainType)loop1);
	}
	return(UNDEFINED);
}

uint32
Map::GetTerrainNum(uint32 xPos, uint32 yPos)
{
	int			tileNum;

	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
		return(NULL);	
	tileNum = MapTiles[xPos + WORLD_WIDTH * yPos];
	if(tileNum >= pWorld->pTileLib->totalTiles)
		return(NULL);
	return((uint32)pWorld->pTileLib->tiles[tileNum].terrain);
}

uint32
Map::GetTileNum(uint32 xPos, uint32 yPos)
{
	uint32		mapPos, tileNum;
	//uint32		GroupNum;
	//Tile			*pTile;

	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
		return(NULL);	
	mapPos = xPos + WORLD_WIDTH * yPos;
	tileNum = MapTiles[mapPos];
	if(tileNum >= pWorld->pTileLib->totalTiles)
		return(NULL);
	return(tileNum);
}

// find x,y closest to startx,y w/ given terrain type
bool
Map::FindNearestTerrain(int startX, int startY, int *pDestX, int *pDestY,
	terrainType terr1, terrainType terr2)
{
	int	loop1, tileNum, distance, bestDistance = 999;
	int16	*pMapTiles = &MapTiles[0];
	tile	*pTile = &pWorld->pTileLib->tiles[0];
	for(loop1 = 0; loop1 < WORLD_HEIGHT * WORLD_WIDTH; loop1++, pMapTiles++)
	{
		tileNum = *pMapTiles;
		if((pTile + tileNum)->terrain == terr1 || (pTile + tileNum)->terrain == terr2)
		{
			distance = AMax(abs(startX - loop1 % WORLD_HEIGHT),
				abs(startY - loop1 / WORLD_HEIGHT));
			if(distance < bestDistance)
			{
				bestDistance = distance;
				*pDestX = loop1 % WORLD_HEIGHT;
				*pDestY = loop1 / WORLD_WIDTH;
			}
		}
	}
	if(bestDistance == 999)
		return(FALSE);
	else return(TRUE);
}

Terrain *
Map::GetTerrain(int xPos, int yPos)
{
	int			tileNum, terrainNum;

	tileNum = MapTiles[xPos + yPos * WORLD_WIDTH];
	terrainNum = (int)pWorld->pTileLib->tiles[tileNum].terrain;
	return(&pWorld->pTileLib->terrains[terrainNum]);
}

uint32
Map::GetTerrainColor(uint32 MapX, uint32 MapY)
{
	int			tileNum, terrainNum, color;
	TileLibRes	*pTileLib = (TileLibRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_TILELIB, pWorld->tileResNum));
	
	// if X or Y is out of bounds
	if(MapX >= WORLD_WIDTH || MapY >= WORLD_HEIGHT)
		return(CI_BLACK);

	tileNum = MapTiles[MapY * WORLD_WIDTH + MapX];
	terrainNum = (int)pTileLib->tiles[tileNum].terrain;
	color = pTileLib->terrains[terrainNum].color;

	// watch for colors that need shifting
	if(ABetween(color, CLUT_TEAM_COLORS, CLUT_TEAM_COLORS + CLUT_SIZE - 1))
	{
		MapSpace *pSpace = GetStructure(MapX, MapY);
		if(pSpace)
			color = *(ADerefAs(uchar, pWorld->clut[pSpace->h.owner]) + (color - CLUT_TEAM_COLORS));
	}
	return(color);
}

bool
Map::FindBurnableXY(uint32 *resX, uint32 *resY)
{
	terrainType terrain1;
	int			randX, randY, loop1;
	bool			fFound;

	randX = ARandom(WORLD_WIDTH);
	randY = ARandom(WORLD_HEIGHT);

	fFound = FALSE;
	for(loop1 = 0; loop1 < WORLD_SIZE; loop1++)
	{
		terrain1 = (terrainType)GetTerrainNum(randX, randY);
		switch(terrain1)
		{
			case FOLIAGE:
			case HIGH_FOLIAGE:
			case STRUCTURE:
			case WALL:
			case MONUMENT:
			case STOCKPILE:
				*resX = randX;
				*resY = randY;
				return(TRUE);

			case KEEP:
				if(rMapNum != 9290)
					break;
				*resX = randX;
				*resY = randY;
				return(TRUE);				
		}
		randX = (randX + 1) % WORLD_WIDTH;
		if(!randX)
			randY = (randY + 1) % WORLD_HEIGHT;
	}
	return(FALSE);
}

bool
Map::IsTypeAllowed(unitType type, uint32 playerSide)
{
	int missionCheck = 999;

	if(pBam->playerTypes[playerSide] == PLAYER_COMPUTER && bGlobal.storyLine == LEGEND)
		missionCheck = bGlobal.missionsDone + 1;

	#ifdef INTERACTIVE_DEMO
	switch(type)
	{
		case GUARDIAN:
		case JUGGERNAUT:
		case NYMPH:
		case PALADIN:
		case WRAITH:
		case WYRM:
			return(FALSE);
	}
	#endif

	switch(type)
	{
		case NOUNIT:
			return(FALSE);

		case BANSHEE:
		case BRIGAND:
		case GORGON:
		case JUGGERNAUT:
		case PEASANT:
		case ROC:
		case ROC_EGG:
		case TORTOISE:
		case TROLL:
		case WOLF:
			if(playerSide == SIDE1 || playerSide == SIDE2)
				return(FALSE);
			break;

		case ENCHANTER:
		case GOBLIN:
		case HARPY:
			switch(rMapNum)
			{
				case 9350:
				case 9370:
				case 9390:
					if(playerSide == SIDE2)
						return(TRUE);
					return(FALSE);

				case 9355:
				case 9375:
				case 9395:
					if(playerSide == SIDE1)
						return(TRUE);
					return(FALSE);

				case 9372:
				case 9352:
				case 9392:
					// both sides allowed
					return(TRUE);

				default:
					return(FALSE);
			}
	}

	switch(type)
	{
		case FURY:
		case GARGOYLE:
		case GNOME:
		case RANGER:
			if(missionCheck < 3)
				return(FALSE);
			break;

		case GHOUL:
		case GOLEM:
		case GRIFFIN:
		case PALADIN:
			if(missionCheck < 6)
				return(FALSE);
			break;

		case NYMPH:
		case WRAITH:
		case WYRM:
			if(missionCheck < 9)
				return(FALSE);
			break;
	}			
	return(TRUE);
}

bool
Map::FireCanBurnAt(int32 x, int32 y)
{
	terrainType terrain;

	terrain = (terrainType)GetTerrainNum(x, y);
	switch(terrain)
	{
		case CHASM:
		case PRISON_PIT:
		case FLAME_SPOUT:
//		case HOTBED:
		case MARSH:
		case MINE:
		case PORTAL:
		case SWAMP:
		case WATER:
		case WHIRLPOOL:
		case OBSTRUCTION:
		case CAULDRON:
			return(FALSE);

		case FOLIAGE:
		case HIGH_FOLIAGE:
		case BRAMBLES:
			if(rMapNum % 9170 < 10)
				return(FALSE);
			break;
	}
	return(TRUE);
}

//-------------------------------------------------------------------------------
//	MapSpace class

void
MapSpace::AddDefender(Unit *pDefender)
{
	int loop1, emptySlot, serial1;

	if(pDefender->type == FURY)
		return;

	serial1 = pDefender->serialNum;
	emptySlot = -1;
	for(loop1 = 0; loop1 < MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE; loop1++)
	{
		if(!defenders[loop1])
		{
			emptySlot = loop1;
		}
		else if(defenders[loop1] == serial1)
		{
//			pMono->Out("Bldg(s%d): Unit s%d already in defender list!\n", serialNum, serial1);
			return;
		}
	}

	if(emptySlot == -1)
		pMono->Out("Bldg(s%d): Cant find open defender slot for unit s%d\n", serialNum, serial1);
	else defenders[emptySlot] = pDefender->serialNum;
}

int
MapSpace::CountDefenders(void)
{
	int		count = 0, loop1;
	uint32	*pDefenders;
	Unit		*pDefender;

	for(loop1 = 0, pDefenders = defenders; loop1 < MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE;
		loop1++, pDefenders++)
	{
		if(*pDefenders)
		{
			pDefender = (Unit *)pWorld->DerefSerial(*pDefenders);
			if(pDefender && pDefender->currAction == GUARD && !pDefender->fAutoControl)
				count++;
		}
	}
	return(count);
}

Unit *
MapSpace::ChooseDefender(void)
{
	Unit		*pDefender;
	int		loop1;
	uint32	*pDefenders;

	for(loop1 = 0, pDefenders = defenders; loop1 < MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE;
		loop1++, pDefenders++)
	{
		if(*pDefenders)
		{
			pDefender = (Unit *)pWorld->DerefSerial(*pDefenders);
			if(pDefender && pDefender->currAction == GUARD && !pDefender->fAutoControl)
				return(pDefender);				
		}
	}
	return(NULL);
}

void
MapSpace::RemoveDefender(Unit *pDefender)
{
	int 		loop1, serial1;
	uint32	*pDefenders;

	serial1 = pDefender->serialNum;
	for(loop1 = 0, pDefenders = defenders; loop1 < MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE;
		loop1++, pDefenders++)
		if(*pDefenders == serial1)
		{
			*pDefenders = NULL;
			return;
		}
	pMono->Out("Bldg(s%d): Cant find defender's slot for unit s%d\n", serialNum, serial1);
}

// Cause all units on a foundation to go into BUILD mode.
// Units will trigger actual cluster change when appropriate.
bool
MapSpace::BuildFromFoundation(funcType newType)
{
	int32		loop1, x, y;
	Unit		*pUnit;

	pMono->Out("randGenCalls == %d\n", bGlobal.randGenCalls);

	if(!CheckFoundationFilled())
		return(FALSE);

	pUnit = pWorld->unitLib.GetUnit(h.xPos, h.yPos);
	if(!pUnit)
		return(FALSE);

	if(!pWorld->SpendMana(pUnit->player, size * MANA_BUILD_SITE, pWorld->vPort.CursorX, pWorld->vPort.CursorY))
		return(FALSE);

	if(newType == FUNC_MISC)
		switch(ARandom(5))
		{
			case 0:
				newType = FUNC_ARBORLODGE;
				break;
			case 1:
				newType = FUNC_BARRACKS;
				break;
			case 2:
				newType = FUNC_CRYPT;
				break;
			case 3:
				newType = FUNC_RUNESTONE;
				break;
			case 4:
				newType = FUNC_TEMPLE;
				break;
		}
	
	newBldgType = newType;

	memset(defenders, NULL, sizeof(defenders));
	// send all units involved into bldg transform mode
	for(loop1 = 0; loop1 < size; loop1++)
	{
		x = tiles[loop1][0];
		y = tiles[loop1][1];

		// get rid of unit here
		pUnit = pWorld->unitLib.GetUnit(x, y);
		if(pUnit)
			pUnit->SetPrimaryAction(TRANSFORM_TO_BLDG);	// will cause actual transformation

	}
	return(TRUE);
}

// change a cluster in foundation mode into operational bldg mode
void
MapSpace::FoundationToBuilding(int player)
{
	if(!CheckFoundationFilled())
		return;

	int32		loop1, loopX, loopY, x, y, x2, y2, minX, minY, maxX, maxY, swapSeries, tileNum;
	tile		*pTile;
	WorldMap	*pWMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	Unit		*pUnit;
	itemType	itemsToPreserve[10], item1;
	int		totalItemsToPreserve = 0, int1;

	minX = 999;	minY = 999; maxX = 0; maxY = 0;
	// locate swap series for this tile set
	tileNum = pWorld->map.GetTileNum(tiles[0][0], tiles[0][1]);
	swapSeries = -1;
	for(loop1 = 0; loop1 < pWorld->map.totalSwapSeries; loop1++)
	{
		if(pWorld->map.swapSeries[loop1][0] == tileNum)
		{
			swapSeries = loop1;
			break;
		}
	}
	if(swapSeries == -1)
	{
		APanic("FoundationToBldg() - couldnt find swap series for tile\n");
		return;
	}
	switch(newBldgType)
	{
		case FUNC_ARBORLODGE:
			swapSeries = pWorld->map.swapSeries[swapSeries][1];
			break;
		case FUNC_BARRACKS:
			swapSeries = pWorld->map.swapSeries[swapSeries][2];
			break;
		case FUNC_CRYPT:
			swapSeries = pWorld->map.swapSeries[swapSeries][3];

			if(bGlobal.storyLine == TUTORIAL)
			{
				if(player == pWorld->playerSide)
				{
					pWorld->SetTutorialGoal(TUT_BUILDING_MADE, TRUE);	
				}
			}
			break;
		case FUNC_RUNESTONE:
			swapSeries = pWorld->map.swapSeries[swapSeries][4];
			break;
		case FUNC_TEMPLE:
			swapSeries = pWorld->map.swapSeries[swapSeries][5];
			break;
	}

	pWorld->map.lBuildings[h.owner].Delete((void *)gSelf);
	h.owner = player;
	pWorld->map.lBuildings[h.owner].Add((void *)gSelf);

	for(loop1 = 0; loop1 < size; loop1++)
	{
		x = tiles[loop1][0];
		y = tiles[loop1][1];

		// build up bldg
		pTile = pWorld->map.GetTile(x, y);
//		pWorld->map.SetTile((int16)pTile->swapTile, (uint16)x, (uint16)y);
		pWorld->map.SetTile(swapSeries + loop1, (uint16)x, (uint16)y);

		// any dropped items will be destroyed
		item1 = pWorld->itemMgr.RemoveItem(x, y);
		if(item1)
		{
//			if(!h.presetItem)
//				h.presetItem = pItem->type;
//			else
//			{
				itemsToPreserve[totalItemsToPreserve++] = item1;
//			}
		}

		if(x < minX)
			minX = x;
		if(x > maxX)
			maxX = x;
		if(y < minY)
			minY = y;
		if(y > maxY)
			maxY = y;

		// if tile was in known territory
		if(!(pWorld->vPort.fog[y][x] & FOG_CENTER))
		{	
			pWMap->Draw(x, y);
					
			// if in viewport
			if(pWorld->vPort.InViewPort(x, y))
			{
				x2 = x - pWorld->vPort.ViewX;
				y2 = y - pWorld->vPort.ViewY;
				pWorld->vPort.Draw(x2, y2, x2, y2);
			}
		}
	}

	h.maxHP = HPPerTile[newBldgType] * size;
	h.currHP = h.maxHP;
	h.func = newBldgType;
	CheckFoundationFilled();	// reset status
	pWorld->LaunchSound(SND_BELL_TOLL, h.xPos, h.yPos);

	// Alert nearby units to presence of new bldg.
	// expand search rect by 1 tile
	minX = AMax(0, minX - 1);
	minY = AMax(0, minY - 1);
	maxX = AMin(WORLD_WIDTH - 1, maxX + 1);
	maxY = AMin(WORLD_HEIGHT - 1, maxY + 1);
	for(loopY = minY; loopY <= maxY; loopY++)
		for(loopX = minX; loopX <= maxX; loopX++)
		{
			pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
			if(pUnit->player == h.owner && pUnit->primaryAction != TRANSFORM_TO_BLDG)
				pUnit->CheckForTransformer();
		}

	if(IsSpecialBuilding())
		pWorld->AdjustXP(h.owner, XP_CREATE_SPECIAL);

	for(loop1 = 0; loop1 < totalItemsToPreserve; loop1++)
	{
		int1 = ARandom(size);
		if(!pWorld->itemMgr.PlaceItem(tiles[int1][0], tiles[int1][1], itemsToPreserve[loop1], 10, TRUE))
		{
			pWorld->Announce(h.xPos, h.yPos, SQUIB_ITEM_NAMES,
				(int)itemsToPreserve[loop1], SQUIB_WORLD_MSGS, SQUIB_WORLD_HASBEENLOST);
		}
	}
}

unitType
MapSpace::GetRelatedUnitType(int rank, funcType testFunc)
{
	int	missionCheck = 0;

	if(testFunc == FUNC_MISC)
		testFunc = (funcType)h.func;

	switch(testFunc)
	{
		case FUNC_TEMPLE:
			switch(rank)
			{
				case 0:
					return(CLERIC);
				case 1:
					return(FURY);
				case 2:
					return(PALADIN);
				case 3:
					if(pWorld->map.IsTypeAllowed(ENCHANTER, h.owner))
						return(ENCHANTER);
					break;
			}
			break;
		case FUNC_ARBORLODGE:
			switch(rank)
			{
				case 0:
					return(DRUID);
				case 1:
					return(RANGER);
				case 2:
					return(GRIFFIN);
				case 3:
					return(NYMPH);
//				case 4:
//					return(DOPPLEGANGER);
			}
			break;
		case FUNC_CRYPT:
			switch(rank)
			{
				case 0:
					return(ZOMBIE);
				case 1:
					if(pWorld->map.IsTypeAllowed(HARPY, h.owner))
						return(HARPY);
					else return(GARGOYLE);

				case 2:
					return(GHOUL);
				case 3:
					return(WRAITH);
			}
			break;
		case FUNC_RUNESTONE:
			switch(rank)
			{
				case 0:
					return(WIZARD);
				case 1:
					return(GNOME);
				case 2:
					return(GOLEM);
				case 3:
					return(WYRM);
			}
			break;
		case FUNC_BARRACKS:
			switch(rank)
			{
				case 0:
					return(WARRIOR);
				case 1:
					return(RANGER);
				case 2:
					return(PALADIN);
				case 3:
					if(pWorld->map.IsTypeAllowed(GOBLIN, h.owner))
						return(GOBLIN);
					break;
			}
			break;

		case FUNC_PORTAL:
			switch(rank)
			{
				case 0:
					return(ACOLYTE);
			}
			break;
	}
	return(NOUNIT);
}

bool
MapSpace::IsSpecialBuilding(void)
{
	switch(h.func)
	{
		case FUNC_ARBORLODGE:
		case FUNC_BARRACKS:
		case FUNC_CRYPT:
		case FUNC_RUNESTONE:
		case FUNC_TEMPLE:
		case FUNC_IMM_WELL:
			return(TRUE);

		default:
			return(FALSE);
	}
}

// is this bldg totally cut off
bool
MapSpace::IsSurrounded(void)
{
	int		loop1, tempX, tempY, checkX, checkY;
	MapSpace	*pBldg;
	bool		fOpening = FALSE;

	// continue to search until an opening is found, or all tiles are tested
	for(loop1 = 0; loop1 < size && !fOpening; loop1++)
	{
		tempX = tiles[loop1][0];
		tempY = tiles[loop1][1];

		fOpening = FALSE;
		// search around each tile in the list, for an opening
		// (note: this search suffers from a lot of redundancy)
		pWorld->map.GetSurroundingCoords(tempX, tempY, 1, checkX, checkY, TRUE);
		do
		{
			// outside of world bounds
			if(checkX < 0 || checkX >= WORLD_WIDTH || checkY < 0 || checkY >= WORLD_HEIGHT)
				continue;

			pBldg = pWorld->map.GetStructure(checkX, checkY);
			if(pBldg && pBldg->h.currHP)
			{
				// blocked by a bldg
				continue;
			}
			if(pWorld->unitLib.GetUnitGrip(checkX, checkY))
			{
				// blocked by a unit
				continue;
			}
			if(pWorld->map.GetTerrain(checkX, checkY)->moveRate <= -6)
			{
				// impassible terrain
				continue;
			}
			fOpening = TRUE;
			break;
		} while(pWorld->map.GetSurroundingCoords(tempX, tempY, 1, checkX, checkY));
	}
	return(fOpening? FALSE: TRUE);
}

bool
MapSpace::IsFoundation(void)
{
	switch(h.func)
	{
		case FUNC_GEN_FOUND:
		case FUNC_ARBOR_FOUND:
		case FUNC_BARRACKS_FOUND:
		case FUNC_CRYPT_FOUND:
		case FUNC_RUNE_FOUND:
		case FUNC_TEMPLE_FOUND:
			return(TRUE);

		default:
			return(FALSE);
	}
}

// return TRUE if filled by same-side acolytes in GUARD mode
bool
MapSpace::CheckFoundationFilled(void)
{
	int		loop1, occupants[5];
	Unit		*pUnit, *pUnits[MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE];
	bool		fOldFilled = fFoundationFilled, fUpdateStatusBox = FALSE;

	if(!IsFoundation())
		return(FALSE);

	for(loop1 = 0;	loop1 < 5; loop1++)
		occupants[loop1] = 0;

	for(loop1 = 0;	loop1 < size; loop1++)
	{
		pUnits[loop1] = NULL;
		pUnit = pWorld->unitLib.GetUnit(tiles[loop1][0], tiles[loop1][1]);
		if(!pUnit)
			occupants[0]++;
		else
		{
			if(pUnit->type != ACOLYTE)
				occupants[0]++;
			if((pUnit->currHP > 0 && pUnit->currAction == GUARD && pUnit->currState == ST_OBELISK) ||
			 (pUnit->currAction == TRANSFORM_TO_BLDG))
			{
				occupants[pUnit->player]++;
				pUnits[loop1] = pUnit;
			}
			else occupants[0]++;

			// watch for unit that viewport cursor is on
			if(pUnit->fCursorTracking)
				fUpdateStatusBox = TRUE;
		}
	}

	if(occupants[SIDE1] == size || occupants[SIDE2] == size)
	{
		fFoundationFilled = TRUE;
	}
	else
	{
		fFoundationFilled = FALSE;
	}

	// if one of the units involved is targeted in the viewport, and foundation status has changed
	if(fUpdateStatusBox && fOldFilled != fFoundationFilled)
	{
		pWorld->DrawStatusBox();
	}

	return(fFoundationFilled);
}

// copy into ourselves (a MapSpace) info from a MapSpaceBase
MapSpace::MapSpace(grip gSpaceBase)
{
	MapSpaceBase *pOldSpace;

	pOldSpace = ADerefAs(MapSpaceBase, gSpaceBase);
	memcpy(&h, &pOldSpace->h, sizeof(h));

	size = 0;
	lastAttackerSerial = 0;
	lastAttackTicks = NULL;
	fCursorTracking = FALSE;
	fFoundationFilled = FALSE;
	memset(tiles, -1, sizeof(tiles));
	serialNum = 0;
	gKillAni = NULL;
}

MapSpace::MapSpace(funcType spaceType) : MapSpaceBase(spaceType)
{
	size = 0;
	lastAttackerSerial = 0;
	fCursorTracking = FALSE;
	fFoundationFilled = FALSE;
	memset(tiles, -1, sizeof(tiles));
	serialNum = 0;
	gKillAni = NULL;
}

MapSpace::MapSpace(FILE *infile) : MapSpaceBase(infile)
{
	size = 0;
	lastAttackerSerial = 0;
	fCursorTracking = FALSE;
	fFoundationFilled = FALSE;
	memset(tiles, -1, sizeof(tiles));
	serialNum = 0;
	gKillAni = NULL;
}

// find the 1st structure of function X belonging to side Y
MapSpace *
Map::FindStructure(funcType function, uint32 side)
{
	MapSpace	*pSpace;
	uint32	loop1;

	for(loop1 = 0; loop1 < h.totalSpaces; loop1++)
	{
		if(gSpaces[loop1])
		{
			pSpace = ADerefAs(MapSpace, gSpaces[loop1]);
			if(pSpace->h.func == function && pSpace->h.owner == side)
				return(pSpace);
		}
	}
	return(NULL);
}

void
MapSpace::SetTracking(bool fNewTracking)
{
	fCursorTracking = fNewTracking;
}

// note - not all MapSpace's are buildings.  Some have other purposes.
int32
MapSpace::TakeAHit(BAM_Ani *pAttacker, unitType attacker, uint32 attackerX, uint32 attackerY,
	int32 damage, int hitX, int hitY)
{
	int	loop1;
	grip	gHitAni;
	Unit	*pUnit, *pReactToUnit, *pDefender;
	ticks_t	currTicks = ATicks();

	//---------------
	//---------------
	attacker = attacker;
	attackerX = attackerX;
	attackerY = attackerY;
	//---------------
	//---------------

	pReactToUnit = NULL;
	pUnit = NULL;

	if(pAttacker && pAttacker->selfType == UNIT)
	{
		pUnit = (Unit *)pAttacker;
		if(h.owner == SIDE0 && h.func != FUNC_HOME)
		{
			pReactToUnit = NULL;
			// peasants dont care about statues and barrels
		}
		else
		{
			pReactToUnit = (Unit *)pAttacker;
			lastAttackerSerial = pReactToUnit->serialNum;

			if(pReactToUnit->player != h.owner)
			{	
				pWorld->unitLib.SetEnemyFlag(h.owner, pReactToUnit->player, TRUE);

				if(pBam->playerTypes[h.owner] == PLAYER_LOCAL)
				{
					// player's bldg attacked - sound the alarm?
					if(!pWorld->tBldgAttackTimer || (pWorld->tBldgAttackTimer < (currTicks - ATTACK_ALARM_TIMER)
						&& currTicks >= ATTACK_ALARM_TIMER))
					{
						pWorld->Announce(h.xPos, h.yPos, SQUIB_WORLD_MSGS, SQUIB_WORLD_STRUCTURESINDANGER);
					}
					// reset timer
					pWorld->tBldgAttackTimer = currTicks;
				}
			}
		}
	}

	lastAttackTicks = ATicks();

	// if we're not a building, or we're a dead one that hasnt been cleaned up
	if(h.currHP < 1 || h.maxHP < 1)
		return(0);

//	damage *= 10;	// diagnostic

	if(damage > h.currHP)
		damage = h.currHP;

	h.currHP -= damage;
	if(h.currHP < 1)
		Die(pUnit);
	else if(pUnit)
	{
		pWorld->ai.HitBldg(this, pReactToUnit);
	}
	// else if hit by terrain - well, ignore it for now

	/*	// have we just been seiged?			// seige no longer supported
	else
	{
		percentBefore = (h.currHP * 100) / h.maxHP;
		percentAfter = ((h.currHP - damage) * 100) / h.maxHP;
		if(percentBefore / 25 != percentAfter / 25 && percentAfter < 75)
		{
			Unit			*pReactToUnit = ADerefAs(Unit, gAttacker);
			char			string1[80];
			terrain		*pTerrain = pWorld->map.GetTerrain(h.xPos, h.yPos);

			if(h.owner != pReactToUnit->player)
			{
				if(h.owner == pWorld->playerSide)
				{
					sprintf(string1, "Your %s has been seiged!", pWorld->squib1.Load(tileResNum, (int)terrain1));
					pWorld->MsgBoxAdd(string1, h.xPos, h.yPos);
					pWorld->MsgBoxDraw();
				}
				else if(pReactToUnit->player == pWorld->playerSide)
				{
					sprintf(string1, "You've seiged the %s!", pTerrain->szName);
					pWorld->MsgBoxAdd(string1, h.xPos, h.yPos);
					pWorld->MsgBoxDraw();
				}
				h.owner = pReactToUnit->player;
			}
		}
	}*/

//	pMono->Out("Bldg(%d) hit for %d points. %d HP left\n", gSelf, damage, h.currHP);
	if(fCursorTracking && h.currHP > 0)
	{
		pWorld->UpdateHPGuageOnly(h.currHP, h.maxHP, h.owner);
	}
	// elsewise, dont bother.  We'll have already been removed from the status box.

	TRACK_MEM("HitAni");	gHitAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_STRUCTURE_HIT, 1, PRI_WALKER,
		TRUE, FALSE, 4, hitX, hitY, FALSE);
	pWorld->vPort.CheckAni(gHitAni);
	
	if(pReactToUnit && pReactToUnit->DoIHate(this))
	{
		// send someone to defend us
		for(loop1 = 0; loop1 < MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE; loop1++)
			if(defenders[loop1])
			{
				pDefender = (Unit *)pWorld->ValidateTarget(defenders[loop1]);
				if(!pDefender)
				{
					defenders[loop1] = NULL;
					continue;
				}

				// if he looks available
				if(pDefender->currAction == GUARD && pDefender->fPrimarySatisfied)
				{
					if(pDefender->SearchForBuildings(FALSE) == this)
					{
						// cause enemy to be spotted on next re-check
						pDefender->SearchForEnemiesPreset(pReactToUnit);
						break;
					}
				}
			}
	}

	return(h.currHP);
}

int32
MapSpace::HealThyself(grip gHealer, unitType healer, uint32 healVal)
{
	grip		gAni;
	Unit		*pUnit;
	uint32	loopX, loopY, compoundCoord, coordX, coordY;
	List		lCoords(15, TRUE);
	MapSpace	*pSpace;
	
	healer = healer;

	if(gHealer)
		pUnit = ADerefAs(Unit, gHealer);
	else pUnit = NULL;

	if(h.maxHP < 1)	// we dont even have HP, dummy
		return(-1);

	h.currHP = AMin(h.currHP + healVal, h.maxHP);
	if(fCursorTracking)
		pWorld->UpdateHPGuageOnly(h.currHP, h.maxHP, h.owner);

	for(loopY = 0; loopY < WORLD_HEIGHT; loopY++)
		for(loopX = 0; loopX < WORLD_WIDTH; loopX++)
		{
			pSpace = pWorld->map.GetStructure(loopX, loopY);
			if(pSpace && pSpace->gSelf == gSelf)
			{
				lCoords.Add((void *)((loopX << 8) + loopY));
			}
		}

	// do repair sparkle ani on all building tiles
	compoundCoord = (uint32)lCoords.FirstValue();
	while(compoundCoord)
	{
		coordX = compoundCoord >> 8;
		coordY = compoundCoord & 255;
		TRACK_MEM("RepairAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_HEAL_REPAIR, 1, PRI_WALKER,
			TRUE, FALSE, 4, coordX, coordY, FALSE);
		pWorld->vPort.CheckAni(gAni);

		compoundCoord = (uint32)lCoords.NextValue();
	}
	lCoords.Release();
	pWorld->LaunchSound(SND_HEAL_REPAIR, h.xPos, h.yPos);

	if(bGlobal.storyLine == TUTORIAL && h.owner == SIDE0 && pUnit && 
		pUnit->player == pWorld->playerSide && (funcType)h.func == FUNC_HOME && h.currHP >= h.maxHP)
			pWorld->SetTutorialGoal(TUT_BLDG_REPAIRED, TRUE);

	pWorld->LaunchSound(SND_HEAL_REPAIR, h.xPos, h.yPos);

	return(h.currHP);
}


void
MapSpace::Die(Unit *pAttacker)
{
	int32			loopX, loopY, x, y, minX, minY, maxX, maxY, unitCount, loop1;
	char 			string1[80];
	terrainType terrain1;
	TerrainUnit	*pTUnit;
	tile			*pTile;
	unitType		newUnitType;
	WorldMap		*pWMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	bool			fWallTiles;
	grip			gAni, gUnit;
	Unit			*pUnit;
	MapSpace		*pSpace;
	BAM_Ani		*pAni;

	if(h.maxHP < 1)	// failsafe: if we're not a building
		return;

	h.currHP = 0;	// make sure

	// no XPs for friendly fire
	if(pAttacker && pAttacker->player != h.owner)
	{
		pWorld->structuresDestroyed[h.owner][pAttacker->player]++;
		switch(h.func)
		{
			case FUNC_BANISH_STONE:
				pWorld->AdjustXP(pAttacker->player, 100);
				break;

			case FUNC_ARBORLODGE:
			case FUNC_BARRACKS:
			case FUNC_CRYPT:
			case FUNC_RUNESTONE:
			case FUNC_TEMPLE:
				pWorld->AdjustXP(pAttacker->player, 150);
				break;

			case FUNC_GATE:
			case FUNC_KEEP:
			case FUNC_AQUEDUCT:
			case FUNC_IMM_GATE:
				pWorld->AdjustXP(pAttacker->player, 250);
				break;

			case FUNC_MISC:
			case FUNC_HOME:
			default:
				pWorld->AdjustXP(pAttacker->player, 50);
				break;
		}
	}
	else
		pWorld->structuresDestroyed[h.owner][SIDE3]++;

	// death message
	string1[0] = NULL;
	terrain1 = (terrainType)pWorld->map.GetTerrainNum(h.xPos, h.yPos);
	switch(terrain1)
	{
		case TURRET1:
		case TURRET2:
		case TURRET3:
		case TURRET4:
		case TURRET5:
		case TURRET6:
		case TURRET7:
		case TURRET8:
		case TURRET9:
		case TURRET10:
		case TURRET12:
		case TURRET13:
		case TURRET14:
		case TURRET15:
			terrain1 = TURRET;	// one name, one wav file
			break;
	}

	if(pWorld->playerSide == h.owner)
	{
		pWorld->Announce(h.xPos, h.yPos, SQUIB_WORLD_MSGS, SQUIB_WORLD_YOUR, SQUIB_WORLD_TERRAIN,
			(int)terrain1, SQUIB_WORLD_MSGS, SQUIB_WORLD_HASBEENDESTROYED);
	}
	else if(pWorld->unitLib.CheckEnemyFlag(pWorld->playerSide, h.owner))
	{
		pWorld->Announce(h.xPos, h.yPos, SQUIB_WORLD_MSGS, SQUIB_WORLD_ENEMY, SQUIB_WORLD_TERRAIN,
			(int)terrain1, SQUIB_WORLD_MSGS, SQUIB_WORLD_HASBEENDESTROYED);
	}
	
	fWallTiles = FALSE;
	// find all tiles belonging to this cluster, and swap them
	minX = 999;	minY = 999; maxX = 0; maxY = 0;
	for(loop1 = 0; loop1 < size; loop1++)
	{
		x = tiles[loop1][0];
		y = tiles[loop1][1];

		terrain1 = (terrainType)pWorld->map.GetTerrainNum(x, y);
		if(terrain1 == WALL || terrain1 == GREAT_WALL)
		{
			fWallTiles = TRUE;
			// needs different sound FX
		}

		if(x < minX)
			minX = x;
		if(x > maxX)
			maxX = x;
		if(y < minY)
			minY = y;
		if(y > maxY)
			maxY = y;

		TRACK_MEM("CollapseAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_COLLAPSE, 1,
			PRI_WALKER + 1, TRUE, FALSE, 3, x, y, FALSE);
		pAni = ADerefAs(BAM_Ani, gAni);
		pAni->cycleNext += ARandom(4);
		pWorld->vPort.CheckAni(gAni);

		terrain1 = (pWorld->map.GetTile(x, y))->terrain;
		if(ABetween(terrain1, TURRET, TURRET15))
		{
			// restore original tile
			pWorld->vPort.SwapMapTile(x, y, oldTileNums[loop1]);
		}
		else pWorld->vPort.SwapMapTile(x, y);

		// if fire on this space, extinguish
		pTUnit = pWorld->vPort.GetTerrainUnit(x, y);
		if(pTUnit && (pTUnit->activeType == ACTIVE_FIRE || pTUnit->activeType == ACTIVE_ARCH_FIRE))
		{
			// if spreadable fire (never dies, just moves)
			if(pTUnit->fFireSpread)
				pTUnit->tAniStarted = 0;	// cause premature relocation
			else pTUnit->fAniCycle = FALSE;
		}

		pUnit = pWorld->unitLib.GetUnit(x, y);
		if(pUnit && pUnit->movement == WALKER)
		{
			// unit's defense might change now considering change of terrain
			pUnit->SetTilePos(pUnit->tileX, pUnit->tileY);
		}
	}

	if(fWallTiles)
	{
		pWorld->LaunchSound(SND_COLLAPSING_STONES, tiles[0][0], tiles[0][1]);
	}
	else 
	{
		pWorld->LaunchSound(SND_CRASH, tiles[0][0], tiles[0][1]);
	}

	// release item
	loopX = ARandom(size);
	x = tiles[loopX][0];	y = tiles[loopX][1];

	if(h.presetItem)
	{
		pWorld->itemMgr.PlaceItem(x, y, (itemType)h.presetItem);	
	}
	else
	{
		if(h.fPickRandomItem && ARandom(4))
		{
			itemType newItem;
			newItem = pWorld->itemMgr.PickRandomItem();
			if(newItem)	// should be true, but just in case
				pWorld->itemMgr.PlaceItem(x, y, newItem);
		}
	}

	// release units
	if(pBam->playerTypes[h.owner] == PLAYER_COMPUTER)
	{
		unitCount = 2;
	}
	else
	{
		unitCount = ARandom(AMin(size / 2, 1) + 1);
		// the norm - only overridden in case of FUNC_MISC
	}
	newUnitType = NOUNIT;
	switch((funcType)h.func)
	{
		case FUNC_MISC:
			unitCount = 0;
			newUnitType = NOUNIT;
			break;
		case FUNC_HOME:
			// peasants - hmph!
			if(ARandom(4) > 0)
			{
				newUnitType = NOUNIT;
				unitCount = 0;		// 25% chance of no unit(s)
			}
			else
			{
				if(pWorld->map.rMapNum == 9390 || pWorld->map.rMapNum == 9395)
					newUnitType = GOBLIN;
				else
				{
					newUnitType = PEASANT;

					if(pWorld->unitLib.lUnits[SIDE0].count > 8)
					{
						// nevermind - enough peasants already
						unitCount = 0;
					}
				}
			}
			break;
		case FUNC_BARRACKS:
			newUnitType = WARRIOR;
			break;
		case FUNC_CRYPT:
			newUnitType = GARGOYLE;
			break;
		case FUNC_ARBORLODGE:
			newUnitType = DRUID;
			break;
		case FUNC_TEMPLE:
			newUnitType = CLERIC;
			break;
		case FUNC_RUNESTONE:
			newUnitType = WIZARD;
			break;
		default:
			newUnitType = NOUNIT;
			unitCount = 0;
			break;
	}

	for(loopX = 0; loopX < unitCount; loopX++)
	{
		Unit	*pUnit;
		grip	gUnit;

		TRACK_MEM("Unit!");	gUnit = pWorld->unitLib.NewUnit();
		pUnit = ADerefAs(Unit, gUnit);
		pUnit->SetType(newUnitType);
		pUnit->SetSide(h.owner);
		pUnit->SetDir(DOWN);				// facing South (just to be safe)
		pWorld->vPort.AddAni(pUnit->gSelf);

		// pick location
		for(loopY = 0, loop1 = ARandom(size); loopY < size; loopY++, loop1 = (loop1 + 1) % size)
		{
			x = tiles[loop1][0];	y = tiles[loop1][1];
			if(pUnit->CanIMoveTo(x, y, TRUE))
			{
				pUnit->SetTilePos(x, y);
				loopY = size;
			}
		}
		if(pUnit->tileX == -1)	// if no location ever picked - out of room?
		{
			pUnit->SetPrimaryAction(DIE);		// failsafe
		}
		else if((pUnit->type == PEASANT || pUnit->type == GOBLIN) && pUnit->player == SIDE0)
		{
			pUnit->FindALeader();
			if(pUnit->fPackLeader)	// if unit became a pack leader instead
			{
				if(pAttacker)
				{
					pUnit->SetPrimaryAction(ATTACK, pAttacker->tileX, pAttacker->tileY);
				}
				else
				{
					pUnit->SetPrimaryAction(GUARD, x, y);
				}
			}
		}
		else if(pAttacker && pAttacker->player != h.owner)		// if bldg destroyed by non-friendly
		{
			pUnit->SetPrimaryAction(ATTACK, pAttacker->tileX, pAttacker->tileY);
		}
		else
		{
			pUnit->SetPrimaryAction(GUARD, x, y);
		}
		pWorld->vPort.CheckAni(pUnit->gSelf);
	}

	h.maxHP = 0;
	if(IsSpecialBuilding())
	{
		h.func = FUNC_GEN_FOUND;		// change cluster function to foundation
		CheckFoundationFilled();	// now that we're available for building again
		h.currHP = 0;

		// alert any nearby units that our state has changed,
		// which can affect their transformation options.

		// expand search rect by 1 tile
		minX = AMax(0, minX - 1);
		minY = AMax(0, minY - 1);
		maxX = AMin(WORLD_WIDTH - 1, maxX + 1);
		maxY = AMin(WORLD_HEIGHT - 1, maxY + 1);
		for(loopY = minY; loopY <= maxY; loopY++)
			for(loopX = minX; loopX <= maxX; loopX++)
			{
				pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
				if(pUnit->player == h.owner)
					pUnit->CheckForTransformer();
			}
	}
	else if(h.func == FUNC_BANISH_STONE)
	{
		// leave behind a prisoner pit
		h.func = FUNC_PRISON_PIT;
		h.currHP = 0;
	}
	else if(h.hotSpot)
	{
		h.currHP = 0;
		// need to leave an hotspot marker behind, so dont delete first X,Y entry from map

		for(loop1 = 1; loop1 < size; loop1++)
		{
			// erase us from the map
			pWorld->map.SetStructure(NULL, tiles[loop1][0], tiles[loop1][1]);
		}
	}
	else
	{
		pWorld->FreeSerialNum(serialNum);

		for(loop1 = 0; loop1 < size; loop1++)
		{
			// erase us from the map
			pWorld->map.SetStructure(NULL, tiles[loop1][0], tiles[loop1][1]);
		}

		// leave grip in gSpaces[] list, for de-allocation later
		/*	for(loop1 = 0;	loop1 < pWorld->map.h.totalSpaces; loop1++)
			if(pWorld->map.gSpaces[loop1] == gSelf)
				pWorld->map.gSpaces[loop1] = NULL;*/
	}

	if(h.func == FUNC_IMM_GATE)
	{
		pSpace = pWorld->map.GetStructure(h.destX, h.destY);
		if(pSpace && pSpace->size > 0)
		{
			// summon immortals
			for(loop1 = 0; loop1 < pSpace->size; loop1++)
			{
				x = pSpace->tiles[loop1][0];
				y = pSpace->tiles[loop1][1];

				TRACK_MEM("Unit!");	gUnit = pWorld->unitLib.NewUnit();
				pUnit = ADerefAs(Unit, gUnit);
				pTile = pWorld->map.GetTile(x, y);
				switch(pTile->terrain)
				{
					case GUARDIAN_PORTAL:
						pUnit->SetType(GUARDIAN);
						break;
					case GOLEM_PORTAL:
						pUnit->SetType(GOLEM);
						break;
					case PALADIN_PORTAL:
						pUnit->SetType(PALADIN);
						break;
					case WARRIOR_PORTAL:
						pUnit->SetType(WARRIOR);
						break;
					default:
						pUnit->SetType(GOLEM);
						break;
				}
				pUnit->SetSide(h.owner);
				pUnit->RunAnimation(ST_ANIM_WALK, 0, FALSE, DOWN);
				pWorld->vPort.AddAni(gUnit);

				// cause pedastal to look used
				pWorld->vPort.SwapMapTile(x, y);

				pUnit->SetTilePos(x, y);
				if(pAttacker)
					pUnit->SetPrimaryAction(ATTACK, pAttacker->tileX, pAttacker->tileY);
				else pUnit->SetPrimaryAction(GUARD, x, y);

				TRACK_MEM("TransformAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1,
					pUnit->priorityLevel + 2, TRUE, FALSE, 4, x, y, FALSE);				// poof!
				pWorld->vPort.CheckAni(gAni);		
			}
		}
	}

	pWorld->map.lBuildings[h.owner].Delete((void *)gSelf);

	if(h.hotSpot || h.func == FUNC_GEN_FOUND || h.func == FUNC_PRISON_PIT)
	{
		h.owner = SIDE0;
		pWorld->map.lBuildings[h.owner].Add((void *)gSelf);
	}

	pWorld->CheckWinLose(h.xPos, h.yPos);

	// if we were being watched, cause new cursor target to be set
	if(fCursorTracking)
		pWorld->vPort.MoveCursor(pWorld->vPort.CursorX, pWorld->vPort.CursorY);
	memset(defenders, NULL, sizeof(defenders));

	if(gKillAni)
	{
		// one of Vas's stupid windmills, probably
		pWorld->vPort.DeleteAni(gKillAni);
		pWorld->lBackgroundAnis.Delete((void *)gKillAni);
	}
}

void
MapSpace::AddTile(int tileX, int tileY)
{
	int	loop1;
	int8	tempX, tempY;

	// inclusive sort (single-pass bubble sort at inclusion)
	for(loop1 = 0; loop1 < size; loop1++)
	{
		tempX = tiles[loop1][0];
		tempY = tiles[loop1][1];
		if(tileY < tempY || (tileY == tempY && tileX < tempX))
		{
			tiles[loop1][0] = (int8)tileX;
			tiles[loop1][1] = (int8)tileY;
			tileX = (int8)tempX;
			tileY = (int8)tempY;
		}
	}
	tiles[size][0] = (int8)tileX;
	tiles[size][1] = (int8)tileY;
	size++;
}

void
MapSpace::DeleteTile(int tileX, int tileY)
{
	int	loop1, loop2;

	for(loop1 = 0;	loop1 < size; loop1++)
	{
		if(tiles[loop1][0] == tileX && tiles[loop1][1] == tileY)
		{
			// move rest of list down one element, overwriting this one
			for(loop2 = loop1; loop2 < size - 1; loop2++)
			{
				tiles[loop2][0] = tiles[loop2 + 1][0];
				tiles[loop2][1] = tiles[loop2 + 1][1];
			}
			size--;
			return;
		}
	}

	// if we get here, then tile not found in list
	return;
}

bool
MapSpace::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&mapSpaceDataStart, 1, (int)&mapSpaceDataEnd -
				(int)&mapSpaceDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&mapSpaceDataStart, 1, (int)&mapSpaceDataEnd -
				(int)&mapSpaceDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
MapSpace::RunTrigger(bool fSwap)
{
	MapSpace	*pTarget;
	int		loop1, tileX, tileY;
	Unit		*pUnit;

	if(fSwap)
	{
		switch(h.func)
		{
			case FUNC_SWTRIP:
				pWorld->LaunchSound(SND_TRIPSWITCH, h.xPos, h.yPos);
				break;

			case FUNC_SWPRESSURE:
				pWorld->LaunchSound(SND_PRESSUREPLATE, h.xPos, h.yPos);
				break;
		}
	}

	switch(h.func)
	{
		case FUNC_SWTRIP:
		case FUNC_SWPRESSURE:
			pTarget = pWorld->map.GetStructure(h.destX, h.destY);
			for(loop1 = 0; loop1 < pTarget->size; loop1++)
			{
				tileX = pTarget->tiles[loop1][0];
				tileY = pTarget->tiles[loop1][1];
				pWorld->vPort.SwapMapTile(tileX, tileY);

				// if a unit was there, cause him to re-register
				pUnit = pWorld->unitLib.GetUnit(tileX, tileY);
				if(pUnit)
					pUnit->SetTilePos(pUnit->tileX, pUnit->tileY, pUnit->tileXOff, pUnit->tileYOff);
			}

			if(fSwap && h.swapClusterNum)
			{
				pMono->Out("Swap cluster to %d\n", h.swapClusterNum);
				pTarget = ADerefAs(MapSpace, pWorld->map.gSpaces[h.swapClusterNum]);

				pTarget->h.func = h.func;
				pTarget->h.xPos = tiles[0][0];
				pTarget->h.yPos = tiles[1][0];
				pTarget->size = 0;
				for(loop1 = 0; loop1 < size; loop1++)
				{
					pTarget->AddTile(tiles[loop1][0], tiles[loop1][1]);
					pWorld->map.SetStructure(h.swapClusterNum, tiles[loop1][0], tiles[loop1][1]);
				}

				pTarget->RunTrigger(FALSE);
			}
			break;
	}
}
