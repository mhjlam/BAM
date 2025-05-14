//
// SRLE.CPP
//
// December 9, 1994
// WATCOM: September 20, 1994  CLS
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include <assert.h>
#include <limits.h>
#include <string.h>

#include "api.hpp"
#include "comp.hpp"
#include "trle.hpp"
#include "toolmem.hpp"
#include "srle.hpp"

uint32
PackSRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader)
{
	int		i, j;
	bool		testSkip = TRUE;
	uchar*	startWrite;
	uchar*	startRead;
	uchar*	dataBuff;
	uchar*	startCelData;
	size_t	buffSize;
	size_t	totalSize = 0;
	SRLEline	lineData;
  
// ALLOCATE FINAL BUFFER AND SET UP POINTERS	
	dataBuff = (uchar*)malloc(celHeader->width * celHeader->height);
	if (!dataBuff)
	{
		printf("COULD NOT ALLOCATE DATA BUFFER!");	
		exit(0);
	}
	startCelData = dataBuff;
	startRead = pSrc + celHeader->offset;

// LOOK AT EVERY LINE IN THE CEL
	for (i = 0; i < celHeader->height; i++)
	{
	// SET UP STRUCTURE TO HOLD INFO
		lineData.skipNum = 0;
		lineData.runLength = 0;
		lineData.runData = (uchar*)malloc(celHeader->width + 50);
		if (!lineData.runData)
		{
			printf("COULD NOT ALLOCATE LINE BUFFER!");	
			exit(0);
		}
		startWrite = lineData.runData;
		testSkip = TRUE;

	// COMPRESS
		for (j = 0; j < celHeader->width; j++)
		{
			char		theChar;

			theChar = *(pSrc);
			if (testSkip && (theChar == 0xFE))
			{
				lineData.skipNum++;
			}
			if (!testSkip && (theChar == 0xFE))
			{
				int 		k;
				bool		centerSkip = FALSE;
				uchar*	tempLine;
			
				tempLine = pSrc;

			// LOOK AT THE REST OF THE LINE TO SEE IF IT'S ALL SKIP
				for (k = j; k < celHeader->width; k++)
				{
					char		tempChar;
	
					tempChar = *tempLine;
					if (tempChar != 0xFE)
					{
// THIS STUFF ISN'T RIGHT. THE ENTIRE BUFFER WON't BE WRITTEN TO RUNDATA
						printf("\nfound center skip");
						lineData.runLength += k;
						*lineData.runData = theChar;
						lineData.runData++;
						k = celHeader->width;
						centerSkip = TRUE;
					}
					tempLine++;
				}
				if (! centerSkip)
				{
					j = celHeader->width;
					pSrc = startRead + celHeader->width * (i + 1);
				}
			}
			if (theChar != 0xFE)
			{
				testSkip = FALSE;
				lineData.runLength++;
				*lineData.runData = theChar;
				lineData.runData++;
			}
			pSrc++;
		}

// WRITE LINE TO BUFFER
		buffSize = lineData.runData - startWrite + sizeof(char) + sizeof(int16);
		totalSize += buffSize;
		memcpy(dataBuff, &lineData, buffSize);
		dataBuff += buffSize;
		free(startWrite);
	}

// WRITE BUFFER TO CEL
	buffSize = dataBuff - startCelData;
	memcpy(pDst, startCelData, buffSize);
	free(startCelData);
	return (totalSize);
}

void
ExpandSRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader)
{
	int		i, j;
	uchar*	travel;
	uchar*	miniBuff;
	uchar*	theBuff;
	uchar*	startWrite;
	SRLEline	lineData;
	int		leftoverSkip;

// ALLOCATE BUFFER FOR FINISHED DATA AND HOLDER FOR RUN LENGTH
	theBuff = (uchar*)malloc(celHeader->width * celHeader->height);
	if (!theBuff)
	{
		printf("COULD NOT ALLOCATE EXPAND BUFFER!");	
		exit(0);
	}
	miniBuff = (uchar*)malloc(sizeof(int16));
	if (!miniBuff)
	{
		printf("COULD NOT ALLOCATE RUN LENGTH!");	
		exit(0);
	}

// SET UP POINTERS
	travel = pSrc;
	startWrite = theBuff;

// LOOK AT EACH LINE IN THE CEL
	for (i = 0; i < celHeader->height; i++)
	{
	// PULL OFF SKIP NUM AND RUN LENGTH
		lineData.skipNum = *travel;
		travel++;
		memcpy(miniBuff, travel, sizeof(int16));
		lineData.runLength = atoi((char*)miniBuff);
		travel += sizeof(int16);

	// UNCOMPRESS
		for (j = 0; j < lineData.skipNum; j++)
		{
			*theBuff = 0xFE;
			theBuff++;
		}
		for (j = 0; j < lineData.runLength; j++)
		{
			*theBuff = *travel;
			theBuff++;
			travel++;
		}
	// FILL IN THE REST WITH SKIP
		leftoverSkip = celHeader->width - lineData.runLength - lineData.skipNum;
		for (j = 0; j < leftoverSkip; j++)
		{
			*theBuff = 0xFE;
			theBuff++;
		}
	}	

// WRITE CEL
	memcpy(pDst, startWrite, celHeader->width * celHeader->height);
	free(theBuff);
	free(miniBuff);
}
