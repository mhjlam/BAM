//
// MAKEMIF.CPP
//
// May 30rd, 1995
// (C) 1995, Tachyon, Inc.  All rights reserved.
//
//	Create one *.MIF map info file (a tigre resource) from all related *.UNI, *.CFG, *.ITM, and *.SCE
//
//----[]--------------------------------------------------------------------------

#include <ctype.h>
#include <conio.h>

#include "apimem.hpp"
#include "mem.hpp"
#include "os.hpp"

#include "alldefs.hpp"
#include "tilelib2.hpp"
#include "tiles.hpp"

char		configLines[MAX_CONFIG_LINES][80], szBuffer1[256], szBuffer2[256];
int		totalConfigLines, mapResNum, tileResNum;

void		ReadAllDataLines(char *szFileName), Usage(void), ReadMapSCE(void);
char *	CompressString(char *szSrc, char *szDest);
bool		SeekToMark(char *marker, FILE *pFile, char *lineBuf);
int16		mapTiles[WORLD_SIZE], mapSpaces[WORLD_SIZE];
struct MapHeader	mapHeader;
MapSpaceHeader	*pOldSpaces, *pOldSpace1;

void
main(int argc, char *argv[])
{
	FILE		*pDestFile;
	uint32	allocSize, loop1;
	MapInfoRes	*pMapInfo;
	bool		fBool1;
	uint16	wordData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	char		szFileName[40];
	spaceInfo	*pSpace;

	printf("\nMAKEMIF Version 1.0\nCopyright 1995 Tachyon, All Rights Reserved.\nCompiled %s %s\n",
		__DATE__, __TIME__);
	if(argc != 2)
	{
		printf("Error: invalid argument(s).\n");
		Usage();
		exit(1);
	}

	mapResNum = atoi(argv[1]);
	tileResNum = mapResNum - mapResNum % 20;

	totalConfigLines = 0;
	ReadAllDataLines("DEFAULT.UNI");
	sprintf(szFileName, "%d.UNI", mapResNum);
	ReadAllDataLines(szFileName);
	sprintf(szFileName, "%d.CFG", mapResNum);
	ReadAllDataLines(szFileName);
	sprintf(szFileName, "%d.CFG", tileResNum);
	ReadAllDataLines(szFileName);
	sprintf(szFileName, "%d.ITM", mapResNum);
	ReadAllDataLines(szFileName);
	ReadMapSCE();

	allocSize = sizeof(MapInfoRes) + sizeof(spaceInfo) * (mapHeader.totalSpaces - 1);
	printf("Resource %d.MIF will be %ld bytes\n", mapResNum, allocSize);

	TRACK_MEM("MapInfoResMem");	pMapInfo = (MapInfoRes *)mem_malloc(allocSize);
	if(!pMapInfo)
	{
		printf("\nError: memory allocation failure\n");
	}
	else
	{
		// resource allocated - now fill it.
		pDestFile = fopen("MAKEMIF.TXT", "w");

		// fill header
		pMapInfo->genID = NULL;

		memcpy(pMapInfo->configLines, configLines, sizeof(configLines));

		pMapInfo->totalConfigLines = totalConfigLines;

		memset(pMapInfo->pad, NULL, sizeof(pMapInfo->pad));

		memcpy(pMapInfo->mapTiles, mapTiles, sizeof(mapTiles));

		memcpy(pMapInfo->mapSpaces, mapSpaces, sizeof(mapSpaces));

		pMapInfo->totalSpaces = mapHeader.totalSpaces;

		for(loop1 = 0;	loop1 < mapHeader.totalSpaces; loop1++)
		{
			pSpace = &pMapInfo->spaces[loop1];
			pOldSpace1 = pOldSpaces + loop1;

			pSpace->func = (funcType)pOldSpace1->func;
			pSpace->xPos = pOldSpace1->xPos;
			pSpace->yPos = pOldSpace1->yPos;
			pSpace->destX = pOldSpace1->destX;
			pSpace->destY = pOldSpace1->destY;
			pSpace->owner = pOldSpace1->owner;
			pSpace->maxHP = pOldSpace1->maxHP;
			pSpace->presetItem = pOldSpace1->presetItem;
			pSpace->fHotSpot = (bool)pOldSpace1->hotSpot;
			pSpace->fPickRandomItem = (bool)pOldSpace1->fPickRandomItem;
			pSpace->fSpecialItemDrop = (bool)pOldSpace1->fSpecialItemDrop;

			pSpace->gLastAttacker = NULL;
			pSpace->lastAttackTicks = NULL;
			pSpace->fFoundationFilled = FALSE;
			pSpace->fCursorTracking = FALSE;
			pSpace->fInUse = FALSE;
			pSpace->size = 0;
			pSpace->serialNum = 0;
			pSpace->currHP = 0;
			memset(pSpace->tiles, NULL, sizeof(pSpace->tiles));
		}

		fprintf(pDestFile, "MAKEMIF output file\nContents of %d.MIF\n------------------------\n", mapResNum);
		fprintf(pDestFile, "genID = %ld\ntotalConfigLines = %ld\ntotalSpaces = %ld\n",
			pMapInfo->genID, pMapInfo->totalConfigLines, pMapInfo->totalSpaces);
		for(loop1 = 0;	loop1 < pMapInfo->totalConfigLines; loop1++)
		{
			fprintf(pDestFile, "\"%s\"\n", &pMapInfo->configLines[loop1][0]);
		}

		// res data created - now dump to disk.
		printf("\nCreating resource %d.MIF\n", mapResNum);
		fBool1 = CreateGeneric(RES_MAP_INFO, mapResNum, allocSize, pMapInfo, wordData, FALSE);
		if(!fBool1)
		{
			printf("CreateGeneric() failed.  Valid resource not created.\n");
		}
		mem_free((char *)pMapInfo);
	}
	free((char *)pOldSpaces);
	fclose(pDestFile);
}

void
ReadMapSCE(void)
{
	FILE	*pFile;
	char	szName[40];
	int	allocSize;

	sprintf(szName, "%d.SCE", mapResNum);
	pFile = fopen(szName, "rb");
	if(!pFile)
	{
		printf("Error: unable to open source file \"%s\"\n", szName);
		return;
	}

	printf("Processing %s\n", szName);

	fread(mapTiles, sizeof(int16), WORLD_SIZE, pFile);
	fread(mapSpaces, sizeof(int16), WORLD_SIZE, pFile);
	fread(&mapHeader, sizeof(mapHeader), 1, pFile);

	allocSize = sizeof(struct MapSpaceHeader) * mapHeader.totalSpaces;
	pOldSpaces = (MapSpaceHeader *)malloc(allocSize);
	fread(pOldSpaces, sizeof(struct MapSpaceHeader), mapHeader.totalSpaces, pFile);
	fclose(pFile);
}

void
ReadAllDataLines(char *szFileName)
{
	FILE	*pSrcFile;
	int	newLines = 0;

	pSrcFile = fopen(szFileName, "rb");
	if(!pSrcFile)
	{
		printf("Error: unable to open source file \"%s\".\n", szFileName);
		Usage();
		exit(1);
	}

	// seek to beginning of actual data
	printf("Processing file \"%s\"\n", szFileName);

	fgets(szBuffer1, 255, pSrcFile);
	do
	{
		if(szBuffer1[0] == '#')
		{
			if(totalConfigLines >= MAX_CONFIG_LINES)
			{
				printf("ReadAllDataLines(%s): WARNING! MAX_CONFIG_LINES(%d) exceed!\n",
					szFileName, MAX_CONFIG_LINES);
				break;
			}
			else
			{
				CompressString(szBuffer1, szBuffer2);
				printf("\"%s\"\n", szBuffer2);
				strcpy(&configLines[totalConfigLines][0], szBuffer2);
				totalConfigLines++;
				newLines++;
			}
		}
		fgets(szBuffer1, 255, pSrcFile);
	} while(!feof(pSrcFile));

	fclose(pSrcFile);
	printf("Source file contained %d data lines\n", newLines);
}

void
Usage(void)
{
	printf("Format:	MAKEMIF <mapResNum>\n");
}

char *
CompressString(char *szSrc, char *szDest)
{
	char	*str1, buffer[256];

	*szDest = NULL;
	strcpy(buffer, szSrc);

	str1 = strtok(buffer, " \r\n\t|");

	if(str1)
	{
		while(str1)
		{
			strcat(szDest, str1);
			strcat(szDest, " ");
			str1 = strtok(NULL, " \r\n\t|");
		}
		// chop end space
		*(szDest + strlen(szDest) - 1) = NULL;
	}
	if(strlen(szDest) > 79)
	{
		printf("CompressString() error: \"%s\" is still > 80 chars\nPress (almost) any key...", szDest);
		while(!kbhit());
		getch();
	}
	return(szDest);
}

bool
SeekToMark(char *marker, FILE *pFile, char *lineBuf)
{
	int	markLength = strlen(marker);

	do
	{
		fgets(lineBuf, 255, pFile);
		if(feof(pFile))
		{
			*lineBuf = NULL;
			return(FALSE);
		}

		if(!memcmp(lineBuf, marker, markLength))
		{
			return(TRUE);
		}
	}	while(TRUE);
}

