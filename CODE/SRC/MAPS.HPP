// MAPS.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Desc: ?
//
// Date Started: ?
//

#ifndef MAPS_HPP
#define MAPS_HPP

#include "tigre.hpp"
#include "api.hpp"
#include "list.hpp"

#include "alldefs.hpp"
#include "mapsbase.hpp"
#include "tiles.hpp"
#include "tilelib2.hpp"
#include "viewport.hpp"

#define MAP_SWAP_SERIES_LENGTH	15
#define MAP_TOTAL_SWAP_SERIES		10

#define MISS1_PLAYER_USEFUL	70
#define MISS1_PLAYER_FOUND		10
#define MISS1_PLAYER_NEUTRAL	20
#define MISS1_COMP_USEFUL		10
#define MISS1_COMP_FOUND		70
#define MISS1_COMP_NEUTRAL		20
#define MISS15_PLAYER_USEFUL	10
#define MISS15_PLAYER_FOUND	70
#define MISS15_PLAYER_NEUTRAL	20
#define MISS15_COMP_USEFUL		70
#define MISS15_COMP_FOUND		10
#define MISS15_COMP_NEUTRAL	20

class Unit;

class MapSpace : public MapSpaceBase
{
	public:
				MapSpace(funcType spaceType);
				MapSpace(FILE	*infile);
				MapSpace(grip gSpaceBase);
		int32	TakeAHit(BAM_Ani *pAttacker, unitType attacker, uint32 attackerX, uint32 attackerY, int32 damage,
					int hitX, int hitY), HealThyself(grip gHealer, unitType healer, uint32 healVal);
		void	Die(Unit *pAttacker), SetTracking(bool fNewTracking), FoundationToBuilding(int player);
		bool	BuildFromFoundation(funcType newType = FUNC_MISC), CheckFoundationFilled(void),
					IsFoundation(void), IsSpecialBuilding(void), IsSurrounded(void);
		void	AddTile(int tileX, int tileY), DeleteTile(int tileX, int tileY), AddDefender(Unit *),
				RemoveDefender(Unit *);
		int	CountDefenders(void);
		Unit	*ChooseDefender(void);
		void	RunTrigger(bool fSwap = TRUE);
		unitType	GetRelatedUnitType(int rank, funcType testFunc = FUNC_MISC);
		virtual bool Save(uint16 state, FILE *pFile = NULL);

		char		mapSpaceDataStart;
		uint32	defenders[MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE];
		uint32	lastAttackerSerial;		// who last attacked?
		ticks_t	lastAttackTicks;	// and when?

		bool		fFoundationFilled;	// are all foundation spots covered by acolytes?
		bool		fCursorTracking;
		uint32	size, serialNum;
		int8		tiles[MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE][2];
		int32		oldTileNums[MAX_CLUSTER_SIZE * MAX_CLUSTER_SIZE];
		funcType newBldgType;
		grip		gKillAni;
		char		mapSpaceDataEnd;
};

class Map : public MapBase
{
	public:
				Map(void);
				~Map();
		bool	Load(uint rNewMapNum);

	// map interrogation routines
		MapSpace		*FindStructure(funcType function, uint32 side);
		MapSpace		*GetStructure(uint32 mapX, uint32 mapY);		// get ptr to MapSpace struct at X,Y if any
		funcType		StringToFunction(char *string);
		void			SetStructure(int32 structNum, uint32 mapX, uint32 mapY);
		uint32		GetTerrainColor(uint32 MapX, uint32 MapY);
		bool			IsTerrainHidden(uint32 MapX, uint32 MapY), FireCanBurnAt(int32 x, int32 y),
						IsTypeAllowed(unitType type, uint32 playerSide), FindBurnableXY(uint32 *resX, uint32 *resY),
						FindNearestTerrain(int startX, int startY, int *pDestX, int *pDestY,
							terrainType terr1, terrainType terr2 = UNDEFINED);
		uint32		GetTerrainNum(uint32 xPos, uint32 yPos);
		terrainType	GetTerrainType(char *szName);
		uint32		GetTileNum(uint32 xPos, uint32 yPos);
		tile *		GetTile(int	xPos, int yPos);
		Terrain		*GetTerrain(int xPos, int yPos);
		TileGroup	*GetTerrainType(uint32 xPos, uint32 yPos);
		void			FixupClusters(void), UpdateAllTurretTiles(MapSpace *pWall), ApplyRandomChanges(void),
							ClearFoundationOwnerships(void);
		int			ApplyRandomChangesForSide(int newSide, KeyList *pSites, int minCount, int maxCount);
		bool			GetSurroundingCoords(int targetX, int targetY, int radius, int &returnX, int &returnY,
							bool fStart = FALSE);
		virtual bool Save(uint16 state, FILE *pFile = NULL);

		List		lBuildings[TOTAL_SIDES];		// map spaces (grips) with HPs

		char		mapDataStart;
		uint32	serialNums[MAX_SPACES];
		int		portalCoords[TOTAL_SIDES][2];
		grip		gHotSpots[100];
		int		totalHotSpots;
		uint		rMapNum;
		int		swapSeries[MAP_TOTAL_SWAP_SERIES][MAP_SWAP_SERIES_LENGTH], totalSwapSeries;
		bool		fLegendary;
		char		mapDataEnd;
};

#endif


