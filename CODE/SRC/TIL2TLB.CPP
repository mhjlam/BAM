//
// til2tlb.cpp
//
// May 2nd, 1995
// (C) 1995, Tachyon, Inc.  All rights reserved.
//
//	Create one *.TLB tile library resource from one *.TIL
//
//----[]-------------------------------------------------------------

#include <ctype.h>

#include "apimem.hpp"
#include "mem.hpp"

#include "alldefs.hpp"
#include "tilelib2.hpp"
#include "tiles.hpp"

void
Usage(void)
{
	printf("Format:	TIL2TLB <resNum>\n");
	printf("Source filename == <resNum>.TIL, destination == <resNum>.TLB\n");
}

void
main(int argc, char *argv[])
{
	FILE		*pSrcFile, *pDestFile;
	char		szSrcName[40];
	struct	TileLibraryHeader	tLHeader;
	struct	TileGroupHeader	tGHeader;
	struct	TileHeader			tileHeader;
	int		loop1, resNum;
	uint32	allocSize;
	TileLibRes	*pTileLib;
	Terrain	*pTerrain;
	tile		*pTile;
	bool		fBool1;
	uint16	wordData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	printf("\nTIL2TLB Version 1.0\nCopyright 1995 Tachyon, All Rights Reserved.\n\n");
	if(argc != 2)
	{
		printf("Error: invalid argument(s).\n");
		Usage();
		exit(1);
	}

	resNum = atoi(argv[1]);
	resNum -= resNum % 20;
	sprintf(szSrcName, "%d.TIL", resNum);
	pSrcFile = fopen(szSrcName, "rb");
	if(!pSrcFile)
	{
		printf("Error: unable to open source file \"%s\".\n", szSrcName);
		Usage();
		exit(1);
	}

	fread(&tLHeader, sizeof(tLHeader), 1, pSrcFile);
	printf("Source file \"%s\" contains:\n%4d tiles\n%4d terrains\n", szSrcName, tLHeader.totalTiles,
		tLHeader.totalTileGroups);

	allocSize = sizeof(TileLibRes) + sizeof(tile) * (tLHeader.totalTiles - 1);
	printf("Resource %d.TLB will be %ld bytes", resNum, allocSize);

	TRACK_MEM("TileLibResMem");	pTileLib = (TileLibRes *)mem_malloc(allocSize);
	if(!pTileLib)
	{
		printf("\nError: memory allocation failure\n");
	}
	else
	{
		pDestFile = fopen("TIL2TLB.TXT", "w");

		// process header
		pTileLib->genID = NULL;
		pTileLib->totalTiles = tLHeader.totalTiles;
		pTileLib->totalTerrains = tLHeader.totalTileGroups;
		memset(pTileLib->pad, NULL, sizeof(pTileLib->pad));
		fprintf(pDestFile, "TIL2TLB output file\nContents of %d.TLB\n------------------------\n", resNum);
		fprintf(pDestFile, "genID = %ld\ntotalTiles = %ld\ntotalTerrains = %ld\n", pTileLib->genID, pTileLib->totalTiles,
			pTileLib->totalTerrains);

		// process terrain types
		for(loop1 = 0;	loop1 < tLHeader.totalTileGroups; loop1++)
		{
			printf(".");
			fread(&tGHeader, 1, sizeof(tGHeader), pSrcFile);
			pTerrain = &pTileLib->terrains[loop1];
			memcpy(pTerrain->szName, tGHeader.descr, 19);
			pTerrain->szName[19] = NULL;
			pTerrain->moveRate = tGHeader.moveRate;
			pTerrain->attackMod = tGHeader.attackMod;
			pTerrain->attackRangeMod = tGHeader.attackRangeMod;
			pTerrain->defenseMod = tGHeader.defenseMod;
			pTerrain->damageVal = tGHeader.damageVal;
			pTerrain->color = tGHeader.miniColor;
			pTerrain->burnable = tGHeader.burnable;
			pTerrain->portraitNum = tGHeader.portrait;
			fprintf(pDestFile, "\nterrain %d:\"%s\"\nmoveRate = %ld\nattackMod = %ld\nattackRangeMod = %ld\n", loop1,
				pTerrain->szName, pTerrain->moveRate, pTerrain->attackMod, pTerrain->attackRangeMod);
			fprintf(pDestFile, "defenseMod = %ld\ndamageVal = %ld\ncolor = %ld\nburnable = %ld\nportraitNum = %ld\n",
				pTerrain->defenseMod, pTerrain->damageVal, pTerrain->color, pTerrain->burnable, pTerrain->portraitNum);
		}
		printf("\n");

		// process tiles
		for(loop1 = 0;	loop1 < tLHeader.totalTiles; loop1++)
		{
			printf(".");
			fread(&tileHeader, 1, sizeof(tileHeader), pSrcFile);
			pTile = &pTileLib->tiles[loop1];
			pTile->terrain = (terrainType)tileHeader.GroupNum;
			pTile->swapTile = tileHeader.swapTile;
			pTile->aniRes = tileHeader.animTile;
			pTile->aniDelay = tileHeader.animDelay;
			memcpy(pTile->pBitMap, tileHeader.BitMap, sizeof(pTile->pBitMap));
			fprintf(pDestFile, "\ntile %d\nterrain = %ld\nswapTile = %ld\naniRes = %ld\naniDelay = %ld\n",
				loop1, (long)pTile->terrain, pTile->swapTile, pTile->aniRes, pTile->aniDelay);
		}

		printf("\nCreating resource %d.TLB\n", resNum);
		fBool1 = CreateGeneric(RES_TILELIB, resNum, allocSize, pTileLib, wordData, FALSE);
		if(!fBool1)
		{
			printf("CreateGeneric() failed.  Valid resource not created.\n");
		}
		mem_free((char *)pTileLib);
	}

	fclose(pDestFile);
	fclose(pSrcFile);
}
