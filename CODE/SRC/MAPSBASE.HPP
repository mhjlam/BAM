// MAPSBASE.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// Desc: Base class for Map - stuff that both BAM and MAPEDIT need access too?
//
// Date Started: 12-16-94
//

#ifndef MAPSBASE_HPP
#define MAPSBASE_HPP

#include "tilelib2.hpp"

#define MAX_SPACES		500
#define MAX_CLUSTER_MARKERS	25

struct MapHeader
{
	char		LibName[40];
	uint16	totalSpaces;
	char		watcomPad[2];
	char pad[1024];
};

struct MapSpaceHeader
{
	uint32	func, xPos, yPos;	// where is this MapSpace, & what does it do
	int8		hotSpot;		// tactical importance?
	int16		swapClusterNum;
	char		watcomPad1[1];
	uint32	presetItem;	// 0 = no preset item
	int8		fPickRandomItem;	// no preset item, but will have randomly picked item
	char		watcomPad2[1];
	int8		fSpecialItemDrop;	// potential location where special item will be randomly placed at startup

	char		watcomPad3[1];
	uint32	hotX, hotY;		// if hotspot, exactly where? (door of temple, etc)
	uint32	destX, destY;	// mineshaft destination
	uint32	owner;		// 0 = no current owner
	uint32	maxHP;		// 0 = not attackable
	uint32	currHP;
	char		data[24];
};

// basic Map stuff used by both BAM and MAPEDIT
class MapSpaceBase : public TMovable
{
	public:
				MapSpaceBase();
				MapSpaceBase(funcType spaceType);
				MapSpaceBase(FILE *infile);
				~MapSpaceBase();
		void	Read(FILE *infile);
		void	Write(FILE *outfile);
		
		struct MapSpaceHeader h;
};

class MapBase : public TMovable
{
	public:
		uint32	MapSpaceCRC;	// for debugging
		struct	MapHeader h;
		int16		MapTiles[WORLD_SIZE];		// -1 = not defined
		int16		MapSpaces[WORLD_SIZE];	// NULL = does not belong to structure
		void		Report(void);
//		TileLibrary TileLib;

						MapBase(void);
						~MapBase();
		void			SetTile(int tileNum, int xPos, int yPos);			// set tile # at X,Y pos
		Tile			*GetOldTile(uint32 MapX, uint32 MapY);
		bool			Save(void);
		bool			Save(char *NewName);
		bool			Load(char *Name, bool fStripClusters = FALSE);
		void			ParseNewMapInfo(void);
		uint16		AddSpace(uint16 spaceType);
		void			ViewToPix(uint32 ViewXPos, uint32 ViewYPos, uint32& PixX, uint32& PixY);
		bool			CheckSpaces(void);

		grip			gSpaces[MAX_SPACES];	
};

#endif
