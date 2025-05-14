//
//	Copyright 1994, Tachyon, Inc.
//
//
// Desc: Basic map stuff for both BAM and MAPEDIT
//
// Date Started: 12-16-94
//

#include <ctype.h>

#include "api.hpp"
#include "apimem.hpp"
#include "tigre.hpp"

#include "alldefs.hpp"
#include "mapsbase.hpp"
#include "mono.hpp"
#include "tilelib2.hpp"

char szFuncNames[][20] = {"MISC", "ARBOR LODGE", "BARRACKS", "CRYPT", "RUNESTONE", "TEMPLE", 
	"FLAME SPOUT", "MINESHAFT", "PORTAL", "WHIRLPOOL", "HOME", "ARBL FOUND", "BARR FOUND", "CRYP FOUND",
	"RUNE FOUND", "TEMP FOUND", "KEEP", "AQUEDUCT", "GATE", "CAMP", "CAULDRON", "TRIPSW", "PRESSUREPL", "IMM GATE",
	"RANDOMIZER", "GEN FOUND", "BANISH STONE", "PRISONER PIT", "ANY"};

// Map constructor - initialize to safe defaults
MapBase::MapBase(void)
{
	int	loop1;

	for(loop1 = 0; loop1 < WORLD_SIZE; loop1++)
		MapTiles[loop1] = -1;		// default = no tile specified
	memset(MapSpaces, 0, WORLD_SIZE * sizeof(MapSpaces[0]));	// default = no space info specified
	h.LibName[0] = NULL;
	h.totalSpaces = 0;
	memset(gSpaces, NULL, sizeof(grip) * MAX_SPACES);
}

MapBase::~MapBase()
{
	uint16	loop1;
	grip		gSpace;

	for(loop1 = 0; loop1 < h.totalSpaces; loop1++)
	{
//		pMono->Out("[%d]==%d\n", loop1, gSpaces[loop1]);
		if(gSpaces[loop1])
		{
			gSpace = gSpaces[loop1];
			ADelete(gSpace);
			gSpaces[loop1] = NULL;
		}
	}
}

void
MapBase::Report(void)
{
	FILE 		*report;
	uint32	loop1;
	MapSpaceBase	*pSpace;
	char		string1[255], string2[10];

	sprintf(string1, "%s.MAP", h.LibName);
	pMono->Out("\nWriting report list %s\n", string1);
	report = fopen(string1, "w");
	fprintf(report, "// MAPEDIT stuff report for map file \"%s\"\n", h.LibName);
	fprintf(report, "// <*>=---------------------------------------------=<*>\n");
	fprintf(report, "// Cluster list (%d groups):\n//\n#CLUSTERS\n", h.totalSpaces);

	for(loop1 = 0; loop1 < h.totalSpaces; loop1++)
	{
		pSpace = ADerefAs(MapSpaceBase, gSpaces[loop1]);
		fprintf(report, "#%03d Func%02d:%-11s X%02d Y%02d HotSpot:%s Item%03d Rand:%s \
SpecLoc:%s Dest%02d/%02d Owner%d MaxHP%04d\n",
			loop1, pSpace->h.func, &szFuncNames[pSpace->h.func][0], pSpace->h.xPos, pSpace->h.yPos,
			pSpace->h.hotSpot? "YES" : "no ", pSpace->h.presetItem,
			pSpace->h.fPickRandomItem? "YES": "no ", pSpace->h.fSpecialItemDrop? "YES": "no ",
			pSpace->h.destX, pSpace->h.destY, pSpace->h.owner, pSpace->h.maxHP);
	}

	string1[0] = NULL;
	fprintf(report, "\n   ");
	for(loop1 = 0; loop1 < WORLD_WIDTH; loop1++)
	{
		sprintf(string2, " %3d", loop1);
		strcat(string1, string2);
	}
	strcat(string1, "\n");
	fprintf(report, string1);

	string1[0] = NULL;
	for(loop1 = 0; loop1 < WORLD_SIZE; loop1++)
	{
		if(!(loop1 % WORLD_WIDTH))
		{
			sprintf(string2, "%3d ", loop1 / WORLD_WIDTH);
			strcat(string1, string2);
		}

		if(MapSpaces[loop1])
			sprintf(string2, " %03d", MapSpaces[loop1]);
		else sprintf(string2, " ---");
		strcat(string1, string2);
		if(!((loop1 + 1) % WORLD_WIDTH))
		{
			strcat(string1, "\n");
			fprintf(report, string1);
			string1[0] = NULL;
		}
	}
	fclose(report);	
}

void
MapBase::ParseNewMapInfo(void)
{
	FILE 	*report;
	int32	loop1, clusterNum;
	char	buffer[256], *str;
	uint32 mapTime, txtTime;
	MapSpaceBase 	*pSpace;

	sprintf(buffer, "%s.SCE", h.LibName);
	mapTime = AFileLastModTime(buffer);

	sprintf(buffer, "%s.MAP", h.LibName);
	txtTime = AFileLastModTime(buffer);

	pMono->Out("Parsing new map info from %s\n", buffer);

	if(txtTime <= mapTime)
	{
		pMono->Out("Map info file %s is not newer than .SCE - ignoring\n", buffer);
		return;
	}

	report = fopen(buffer, "r");
	if(!report)
		return;

	do
	{
		fgets(buffer, 255, report);
		strupr(buffer);
	} while(memcmp(buffer, "#CLUSTERS", 9) && !feof(report));

	fgets(buffer, 255, report);
	loop1 = 0;
	while(buffer[0] == '#' && !feof(report))
	{
		str = strtok(buffer, " \r\n\t");

		clusterNum = atoi(str + 1);
		if(!gSpaces[clusterNum])		// if no existing cluster in this slot, make new one
		{
			TRACK_MEM("MapSpaceBase"); pSpace = new MapSpaceBase(FUNC_MISC);
			gSpaces[clusterNum] = pSpace->gSelf;
			h.totalSpaces = (uint16)AMax(h.totalSpaces, (uint32)(clusterNum + 1));
		}
		pSpace = ADerefAs(MapSpaceBase, gSpaces[clusterNum]);
		
		str = strtok(NULL, " \r\n\t");
		pSpace->h.func = atoi(str + 4);

		str = strtok(NULL, " \r\n\t");
		pSpace->h.xPos = atoi(str + 1);

		str = strtok(NULL, " \r\n\t");
		pSpace->h.yPos = atoi(str + 1);

		str = strtok(NULL, " \r\n\t");
		pSpace->h.hotSpot = (toupper(*(str + 8)) == 'Y')? TRUE: FALSE;

		str = strtok(NULL, " \r\n\t");
		pSpace->h.presetItem = atoi(str + 4);

		str = strtok(NULL, " \r\n\t");
		pSpace->h.fPickRandomItem = (toupper(*(str + 5)) == 'Y')? TRUE: FALSE;

		str = strtok(NULL, " \r\n\t");
		pSpace->h.fSpecialItemDrop = (toupper(*(str + 8)) == 'Y')? TRUE: FALSE;

		str = strtok(NULL, " \r\n\t");
		pSpace->h.destX = atoi(str + 4);
		pSpace->h.destY = atoi(str + 7);

		str = strtok(NULL, " \r\n\t");
		pSpace->h.owner = atoi(str + 5);

		str = strtok(NULL, " \r\n\t");
		pSpace->h.maxHP = atoi(str + 5);

		fgets(buffer, 255, report);
		loop1++;
	}
	fclose(report);
}

void
MapBase::SetTile(int tileNum, int xPos, int yPos)
{
	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
		return;	

	MapTiles[xPos + WORLD_WIDTH * yPos] = (int16)tileNum;
}

//Tile *
//MapBase::GetOldTile(uint32 xPos, uint32 yPos)
//{
//	uint32		MapPos, TileNum;
//	Tile			*pTile;
//	
//	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
//		return(NULL);	
//	MapPos = xPos + WORLD_WIDTH * yPos;
//	TileNum = MapTiles[MapPos];
//
//	if(TileNum >= TileLib.h.totalTiles)
//		return(NULL);
//	pTile = ADerefAs(Tile, TileLib.gTile[TileNum]);
//	return(pTile);
//}

// Map loader
bool
MapBase::Load(char *Name, bool fStripClusters)
{
	grip		*tempGrip = &gSpaces[158];
	FILE		*infile;
	char		pFileName[40], pLibName[20];
	int		loop1, loopX, loopY;
	MapSpaceBase *pSpace;
	funcType	func;

	strcpy(pLibName, Name);
	sprintf(pFileName, "%s.SCE", pLibName);
	infile = fopen(pFileName, "rb");

	if(!infile)
		return(FALSE);
	fread(MapTiles, sizeof(int16), WORLD_SIZE, infile);

	fread(MapSpaces, sizeof(int16), WORLD_SIZE, infile);

	fread(&h, sizeof(h), 1, infile);
	if(fStripClusters)
	{
		memset(MapSpaces, NULL, sizeof(MapSpaces));
		h.totalSpaces = 0;
	}

	strcpy(h.LibName, pLibName);
	if(h.totalSpaces > 0 && !fStripClusters)
	{
		for(loop1 = 0; loop1 < h.totalSpaces; loop1++)
		{
			TRACK_MEM("MapSpaceBase");		pSpace = new MapSpaceBase(infile);
			func = (funcType)pSpace->h.func;
			gSpaces[loop1] = pSpace->gSelf;

			if(loop1 > 0)	// skip over cluster #0, it's a dummy
			{
				bool fClusterFound = FALSE;
				// scan through map to find where this MapSpace (cluster) is assigned, and make sure it points there
				for(loopY = 0; loopY < WORLD_HEIGHT && !fClusterFound; loopY++)
					for(loopX = 0; loopX < WORLD_WIDTH && !fClusterFound; loopX++)
						if(MapSpaces[loopY * WORLD_WIDTH + loopX] == loop1)
						{
							pSpace->h.xPos = loopX;
							pSpace->h.yPos = loopY;
							fClusterFound = TRUE;
						}
			}
		}
	}
	fclose(infile);
	ParseNewMapInfo();	// read old report file, if any, for overrides/updates
	return(TRUE);
}

// Save with new name
bool
MapBase::Save(char *Name)
{
	strcpy(h.LibName, Name);
	Save();
	return(TRUE);
}

// save with old name
bool
MapBase::Save(void)
{
	FILE			*pFile;
	char			pFileName[40];
//	SmartDialog	*pDialog;
//	grip			grip1;
	uint32		loop1;
//	Tile			*pTile;
//	TileGroup	*pGroup;
	MapSpaceBase *pSpace;

//	TRACK_MEM("SaveDialog"); pDialog = new SmartDialog(0, 0, DG_NO_WINDOW, 0);
//	grip1 = pDialog->AddLabel(100, 200, "Saving map, please wait...");
//	pDialog->Activate();
//	pDialog->SetPos(0, 0);
//	pGraphMgr->Animate();

	Report();

	sprintf(pFileName, "%s.SCE", h.LibName);
	pFile = fopen(pFileName, "wb");
	if(!pFile)
	{
//		pDialog->Deactivate();
//		ADelete(pDialog->gSelf);
		return(FALSE);
	}

	pMono->Out("Writing tile map..\n");
	fwrite(MapTiles, sizeof(MapSpaces), 1, pFile);
	pMono->Out("Writing space map..\n");
	fwrite(MapSpaces, sizeof(MapSpaces), 1, pFile);
	pMono->Out("Writing header..\n");
	fwrite(&h, sizeof(h), 1, pFile);
	for(loop1 = 0; loop1 < h.totalSpaces; loop1++)
	{
		pSpace = ADerefAs(MapSpaceBase, gSpaces[loop1]);
		pSpace->Write(pFile);
//		pMono->Out("Spaces[%d] func==%d\n", loop1, pSpace->h.func);
	}
	fclose(pFile);

//	pDialog->Deactivate();
//	ADelete(pDialog->gSelf);
//	sleep(1);
	return(TRUE);
}

uint16
MapBase::AddSpace(uint16 spaceType)
{
	MapSpaceBase	*pSpace;
	uint32			totalSpaces, maxSpaces;

	totalSpaces = h.totalSpaces;
	maxSpaces = (uint32)MAX_SPACES;

	if(h.totalSpaces >= MAX_SPACES)
		return((uint16)(totalSpaces - 1));

	TRACK_MEM("MapSpaceBase");		pSpace = new MapSpaceBase((funcType)spaceType);
	gSpaces[totalSpaces] = pSpace->gSelf;	// space #0 means no space info
	return(h.totalSpaces++);
}

// translate Viewport coords into screen pix coords
void
MapBase::ViewToPix(uint32 ViewXPos, uint32 ViewYPos, uint32& PixX, uint32& PixY)
{
	PixX = ViewXPos * (TILE_WIDTH + 1) + 1;
	PixY = ViewYPos * (TILE_HEIGHT + 1) + 1;
}

bool
MapBase::CheckSpaces(void)
{
	uint32 loop1, size, currCRC, oldCRC;

	size = sizeof(h);
	oldCRC = MapSpaceCRC;
	currCRC = 0;
//	for(loop1 = 0; loop1 < size; loop1++)
//		currCRC += *((&h)+loop1);

	for(loop1 = 0; loop1 < WORLD_SIZE; loop1++)
	{
//		currCRC += MapTiles[loop1];
		currCRC += MapSpaces[loop1];
	}

	for(loop1 = 0; loop1 < MAX_SPACES; loop1++)
		currCRC += gSpaces[loop1];
	MapSpaceCRC = currCRC;
	if(currCRC != oldCRC)
		return(FALSE);
	return(TRUE);
}

//-------------------------------------------
//-- MapSpaceBase class

MapSpaceBase::MapSpaceBase()
{
}

MapSpaceBase::MapSpaceBase(FILE *infile)
{
	Read(infile);
}

MapSpaceBase::MapSpaceBase(funcType spaceType)
{
	h.func = (uint32)spaceType;
	h.xPos = 0;
	h.yPos = 0;
	h.hotSpot = FALSE;	h.hotX = 0;		h.hotY = 0;
	h.presetItem = 0;
	h.fPickRandomItem = FALSE;
	h.fSpecialItemDrop = FALSE;
	h.destX = 0;	h.destY = 0;
	h.owner = SIDE0;
	h.maxHP = 0;
	memset(h.data, NULL, sizeof(h.data));
}

void
MapSpaceBase::Read(FILE *infile)
{
	fpos_t pos;

	fgetpos(infile, &pos);
//	pMono->Out("pos==%d\n", (int16)pos);
	fread(&h, sizeof(h), 1, infile);

	// sanity check
	if(h.func > MAX_FUNCS)
		h.func = FUNC_MISC;
	if(h.xPos >= WORLD_WIDTH)
		h.xPos = 0;
	if(h.hotX >= WORLD_WIDTH)
		h.hotX = 0;
	if(h.destX >= WORLD_WIDTH)
		h.destX = 0;
	if(h.yPos >= WORLD_HEIGHT)
		h.yPos = 0;
	if(h.hotY >= WORLD_HEIGHT)
		h.hotY = 0;
	if(h.destY >= WORLD_HEIGHT)
		h.destY = 0;
	
	switch((funcType)h.func)
	{
		case FUNC_MISC:
		case FUNC_ARBORLODGE:
		case FUNC_BARRACKS:
		case FUNC_CRYPT:
		case FUNC_RUNESTONE:
		case FUNC_TEMPLE:
		case FUNC_HOME:
			h.currHP = h.maxHP;
			break;

		default:
			h.currHP = 0;
			break;
	}

	memset(h.data, NULL, sizeof(h.data));
}

void
MapSpaceBase::Write(FILE *outfile)
{
	fpos_t pos;

	fgetpos(outfile, &pos);
//	pMono->Out("pos==%d\n", (int16)pos);
	fwrite(&h, sizeof(h), 1, outfile);
}

MapSpaceBase::~MapSpaceBase()
{
}

