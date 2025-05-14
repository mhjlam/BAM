//
// TRLE.CPP
//
// December 9, 1994
// WATCOM: September 20, 1994  CLS
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Tagged (i.e. line indexed) Run Length Encoder
//
//----[]-------------------------------------------------------------

#include <string.h>
#include <limits.h>

#include "apimem.hpp"
#include "trle.hpp"

//-------------------------------------------------------------
//	Pack bytes, unpack bytes encoding is identical to that in
//	ilbm files.

uchar*
PackRLE(uchar* pDst, uchar* pSrc, short width)
{
	char b[128];
	unsigned int bdex = 0, i = 0, j = 0, t = 0;

	do
	{
		i = 0;
		while ((pSrc[t + i] == pSrc[t + i + 1]) &&
		        i < 127 &&
		        i < (width - 1) &&
		        ((t + i + 1) < width))
		{
			++i;
		}

		if (i > 0 || bdex >= 127)
		{
			if (bdex)
			{
				// fputc(((bdex-1) & 0x7f),fp);
				*pDst++ = (uchar) ((bdex - 1) & 0x7f);
				++j;
				// fwrite(b, 1, bdex, fp);
				memcpy(pDst, b, bdex);
				pDst += bdex;
				j += bdex;
				bdex = 0;
			}
			if (i)
			{
				// fputc((~i+1),fp);
				*pDst++ = (uchar) (~i + 1);

				// fputc(pSrc[t+i],fp);
				*pDst++ = pSrc[t + i];
				j += 2;
				t += (i + 1);
			}
		}
		else
		{
			b[bdex++] = pSrc[t++];
		}
	} while(t < width);

	if (bdex)
	{
		// fputc(((bdex-1) & 0x7f),fp);
		*pDst++ = (uchar) ((bdex - 1) & 0x7f);
		++j;

		// fwrite(b,1,bdex,fp);
		memcpy(pDst, b, bdex);
		pDst += bdex;
		j += bdex;
	}

	// padding
	// if((j & 0x0001)) fputc(0x80,fp);
	if ((j & 0x0001))
	{
		*pDst++ = 0x80;
	}

	return pDst;
}



//-------------------------------------------------------------
//	Unpack bytes, encoding is identical to that in ilbm files.

uchar*
ExpandRLE(uchar* pDst, uchar* pSrc, coord width)
{
	int c, i, n = 0;

	do
	{
		c = (*pSrc++) & 0xff;
		if (c & 0x80)
		{
			if (c != 0x80)
			{
				i = ((~c) & 0xff) + 2;
				c = *pSrc++;
				while (i--)
				{
					pDst[n++] = (uchar) c;
				}
			}
		}
		else
		{
			i = (c & 0xff) + 1;
			while (i--)
			{
				pDst[n++] = *pSrc++;
			}
		}
	} while (n < width);

	return pSrc;
}




//-------------------------------------------------------------
//	Advances one RLE line without unpacking bytes into a buffer,
//	slightly faster than ExpandRLE.

uchar*
AdvanceRLE(uchar* pSrc, short width)
{
	int c, i, n = 0;

	do
	{
		c = (*pSrc++) & 0xff;
		if (c & 0x80)
		{
			if (c != 0x80)
			{
				i = ((~c) & 0xff) + 2;
				pSrc++;
				n += i;
			}
		}
		else
		{
			i = (c & 0xff) + 1;
			n += i;
			pSrc += i;
		}
	} while (n < width);

	return pSrc;
}

uchar* 
ScanAddress(ScanOffsetTable* pTable, coord row)
{
	uint32	offset;
	unsigned	i;

	offset = pTable->offset[row];

	for (i = 0; i < MAX_TRLE_PAGES; i++)
	{
		if (row > pTable->lastYofPage[i])
		{
			offset += USHRT_MAX;
		}
		else
		{
			break;
		}
	}

	return (((uchar*) pTable) + offset);
}

//------------------------------------------------------------------
// expands an entire cel
void
ExpandTRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader)
{
	int16			i;
	uchar*		theBuff;
	uchar*		tempBuffer;
	uchar*		startData;

// ALLOCATE FOR DATA STORAGE
	theBuff = (uchar*)malloc((celHeader->width * celHeader->height) + 150);
	if (!theBuff)
	{
		printf("\n\nCOULD NOT ALLOCATE %d BYTES!", celHeader->width * celHeader->height + 150);
		exit(0);
	}
	else
	{
		printf("\n\nALLOCATED %d BYTES!", celHeader->width * celHeader->height + 150);
	}
	tempBuffer = theBuff;

// MOVE POINTER TO START OF DATA
	startData = pSrc + celHeader->offset;
	startData = (uchar*)ScanAddress((ScanOffsetTable*)startData, 0);

// EXPAND LINE BY LINE INTO BUFFER
	for (i = 0; i < celHeader->height; i++)
	{
		startData = ExpandRLE(tempBuffer, startData, celHeader->width);
		tempBuffer += celHeader->width;
	}
	printf("\nUsed up %d bytes", tempBuffer - theBuff);
	memcpy(pDst, theBuff, tempBuffer - theBuff);
	free(theBuff);
}

//-----------------------------------------------------------
// pack an entire cel
uint32
PackTRLECel(uchar* pSrc, uchar* pDst, CelHeader* celHeader)
{
	size_t				bufSize = 0;			
	uint32				encodedCelSize;
	CelArrayElement*	elem;

	bufSize = celHeader->width * celHeader->height;

	TRACK_MEM("CelArrElm");	elem = new CelArrayElement;
	elem->celHeader.orgX = celHeader->orgX;
	elem->celHeader.orgY = celHeader->orgY;
	elem->celHeader.width = celHeader->width;
	elem->celHeader.height = celHeader->height;
	elem->celHeader.priority = celHeader->priority;
	elem->celHeader.offset = celHeader->offset;
	elem->celData = (uchar*)malloc(bufSize * 2);
	memcpy(elem->celData, pSrc, bufSize);

	encodedCelSize = CopyTRLE(pDst, elem);

	if (encodedCelSize > bufSize)
	{
		// This would be bigger than if we didn't compress!!!
		// Let's not compress this way
		// tell the user just to be nice
		printf("\nTRLE produced output larger than the source.");
		printf("\nTRLE won't be used as requested.\n");
		printf("original: %d; TRLE: %d", bufSize, encodedCelSize);
		return (-1);
	}
	else
	{
		return (encodedCelSize);
	}
}

uint32
CopyTRLE(uchar* pResData, CelArrayElement* pElem)
{
	CelHeader			celHeader;
	uchar*				pSrc;
	uchar*				pDst;
	uchar*				pBase;
	unsigned				i;
	ScanOffsetTable*	pTable;
	unsigned				iPage = 0;	

	pSrc = pElem->celData;
	uchar* pTest = pSrc + 780;

	celHeader = pElem->celHeader;

	// calc distance past variable-length scan table
	pTable = (ScanOffsetTable*) pResData;
	pBase = (uchar*) pTable;
	pDst = pResData + sizeof(ScanOffsetTable) + (sizeof(uint16) * (celHeader.height-1));

	// init table lastYofPage
	pTable->lastYofPage[0] = USHRT_MAX;
	pTable->lastYofPage[1] = USHRT_MAX;
	pTable->lastYofPage[2] = USHRT_MAX;
	pTable->lastYofPage[3] = USHRT_MAX;

	// pack each scan line
	for (i=0; i<celHeader.height; i++)
	{
		// if beyond the range of uint16, mark previous y as last of that page
		if ((pDst - pBase) > USHRT_MAX)
		{
			pTable->lastYofPage[iPage++] = (uint16) (i - 1);

			if (iPage > MAX_TRLE_PAGES)
			{
				printf ("\nERROR: Run Length Encoded cels can't be greater 327,000 bytes\n");
				exit(1);
			}
			pBase += USHRT_MAX;
		}

		pTable->offset[i] = (uint16) (pDst - pBase);

		pDst = PackRLE(pDst, pSrc, celHeader.width);
		pSrc += celHeader.width;

	}
	
	return (pDst - pResData);
}
