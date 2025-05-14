//
// RLE.CPP
//
// January 14, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Tagged (i.e. line indexed) Run Length Encoder
//
//----[]-------------------------------------------------------------

#include <string.h>

#include "writeres.hpp"
#include "rle.hpp"



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
//	Pack bytes, unpack bytes encoding is identical to that in
//	ilbm files.

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



