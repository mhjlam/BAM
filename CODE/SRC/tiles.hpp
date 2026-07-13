//
// TILES.HPP
//
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
// 05-02-1995
//
//	Tile resource management
//
//----[]-------------------------------------------------------------

#ifndef	TILES_HPP
#define	TILES_HPP

#include "rect.hpp"
#include "resmgr.hpp"
#include "mapsbase.hpp"

#define MAX_CLUSTER_SIZE	5

struct tile
{
	uint8			terrain;	// stored as 1 byte in binary (Watcom default: small enum = smallest type, no padding)
	uint32		swapTile, aniRes, aniDelay;
	uchar			pBitMap[(TILE_WIDTH) * TILE_HEIGHT];
} __attribute__((packed));
static_assert(sizeof(tile) == 773, "tile size mismatch: must match Watcom-packed binary layout");

struct Terrain
{
	char		szName[20];
	int32		moveRate, attackMod, attackRangeMod, defenseMod, damageVal;
	uint32	color, burnable, portraitNum;
};

struct TileLibRes
{
	uint32	genID;
	uint32	totalTiles, totalTerrains;
	char		pad[64];
	Terrain	terrains[128];	// NOTE! if TERRAINMAXTYPES grows to >thisNum then struct of resource must change!
	tile		tiles[1];
};

struct spaceInfo
{
	uint8		func;	// was funcType (Watcom small enum = 1 byte; max value TOTALFUNCTIONS=28 fits in uint8)
	int32		xPos, yPos, destX, destY, owner, maxHP, presetItem;
	uint8		fHotSpot, fPickRandomItem, fSpecialItemDrop;	// Watcom bool = enum bool { false,true } = 1 byte

	// the following are initialized at, and maintained during, run-time
	grip		gLastAttacker;		// who last attacked?
	ticks_t	lastAttackTicks;	// and when?
	uint8		fInUse, fFoundationFilled, fCursorTracking;	// Watcom bool = 1 byte
	int32		size, serialNum, currHP;
	int32		tiles[MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE][2];
	char		pad[16];
} __attribute__((packed));
static_assert(sizeof(spaceInfo) == 271, "spaceInfo size mismatch: must match Watcom-packed binary layout");

#define MAX_CONFIG_LINES	130
#define CONFIG_LINE_MAX_LEN 80
struct MapInfoRes
{
	uint32				genID;
	char					configLines[MAX_CONFIG_LINES][CONFIG_LINE_MAX_LEN];
	uint32				totalConfigLines;
	char					pad[128];

	int16					mapTiles[WORLD_SIZE], mapSpaces[WORLD_SIZE];
	uint32				totalSpaces;
	struct spaceInfo	spaces[1];
};
#endif
