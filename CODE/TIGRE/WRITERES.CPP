//
// WRITERES.CPP
//
// January 14, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Resource creation routines.
//
// Routines included:
//		CreateGeneric
// 	CreateCel
// 	CreateAnim
// 	CreatePic
//		CreateFont
//		CreateResFileName
//		OpenResFile
//
// 	Cels, anims, and pics may be created from data passed or with
//		null data.  Data passed is in the form of an array of .
//
//----[]-------------------------------------------------------------

#include <assert.h>
#include <limits.h>
#include <string.h>

#include "apimem.hpp"
#include "comp.hpp"
#include "trle.hpp"
#include "srle.hpp"
//#include "toolmem.hpp"
#include "mem.hpp"
#include "writeres.hpp"


bool	fIsTextMode = TRUE;
bool	WriteResFile(res_t type, uint number, uchar *pResData, uint32 bufSize); 

// Create a generic resource.  'pData' is a pointer to an array of 4 uint16s
// which will be written into the resource.  'size' is the size of the actual
//	data, not including any headers.
//
bool
CreateGeneric(res_t type, uint number, size_t size, void* pData, uint16 aWordData[], bool compress)
{
	uint16		i;
	ResHeader	resHeader;

	compress = compress;		// Gets rid of unreferenced warning

	// Setup the header.
	resHeader.compType = CMP_NONE;
	resHeader.uSize = size;
	resHeader.size = 0;
	resHeader.type = type;
	resHeader.numHeaders = 0;
	resHeader.sizeOfHeaders = 0;
	for (i = 0; i < RH_DATA_MAX; i++)
	{
		resHeader.data[i] = aWordData[i];
	}
	resHeader.genID = GENERATION;

	// Create the final buffer and copy data.
	TRACK_MEM("Create Res");
	uchar* pResData = (uchar*) mem_malloc(size + rhSize);
	memcpy(pResData, &resHeader, rhSize);
	memcpy(pResData + rhSize, pData, size);

	// Write the resource to disk
	if (WriteResFile(type, number, pResData, size))
	{
		mem_free (pResData);
		pResData = NULL;
		return TRUE;
	}

	if (pResData != NULL)
	{
		mem_free (pResData);
	}
	return FALSE;
}


// Create a cel resource.  'celData' contains information about the cel
// and its data which will be written into the resource.
bool
CreateCel(uint number, CelArrayElement* celData, bool compress)
{
	uint16		i;
	ResHeader	resHeader;
	CelHeader	celHeader;
	size_t		celSize;
	size_t		bufSize;

	compress = compress;		// Gets rid of unreferenced warning
	
	// initialize to zero
	memset(&celHeader, 0, sizeof(CelHeader));

	// Calculate the size of the resource.
	celHeader = celData->celHeader;
	celSize = celHeader.width * celHeader.height;
	bufSize = celSize + chSize;

	// Setup the header.
	resHeader.compType = CMP_NONE;
	resHeader.uSize = bufSize;
	resHeader.size = 0;
	resHeader.type = RES_CEL;
	resHeader.numHeaders = 1;
	resHeader.sizeOfHeaders = chSize;

	for (i = 0; i < RH_DATA_MAX; i++)
	{
		resHeader.data[i] = 0;
	}
	resHeader.genID = GENERATION;

	// Setup the cel header.
	celHeader.offset = 0;

	// Create the final buffer and copy data.
	TRACK_MEM("Create Cel");
	uchar* pResData = (uchar*) mem_malloc(bufSize + rhSize);
	memcpy(pResData, &resHeader, rhSize);
	memcpy(pResData + rhSize, &celHeader, chSize);
	memcpy(pResData + rhSize + chSize, celData->celData, celSize);

	// Write the resource to disk
	if (WriteResFile(RES_CEL, number, pResData, bufSize))
	{
		mem_free (pResData);
		return TRUE;
	}

	mem_free (pResData);
	return FALSE;
}


// uint32	CopyTRLE(uchar* pResData, CelArrayElement* pElem);

// Create an anim resource.  'pCelDataArray' is an array of CelData structures
// that contain information about all the cels in the anim.
bool
CreateAnim(uint number, uint numCels, CelArrayElement* pCelDataArray[], uint16 compress)
{
	uint16		i;
	ResHeader 	resHeader;
	CelHeader	celHeader;
	size_t		workBufSize;
	size_t		bufSize = 0;			// total for resource
	size_t		celSize = 0;			// total of all cels
	size_t		singleCelSize = 0;	// size of one cel
	size_t		offset = 0;
	uchar* 		pResData;
	uint16		totalCelHeight;

	// make sure that the BOGUS bit is cleared
	compress &= ~CMP_BOGUS;

	// Calculate the size of the resource.
	for (i = 0; i < numCels; i++)
	{
		pCelDataArray[i]->celHeader.offset = celSize;
		celHeader = pCelDataArray[i]->celHeader;
		singleCelSize = celHeader.width * celHeader.height;
		bufSize += chSize + singleCelSize;
		celSize += singleCelSize;
	}

	// Setup the header.
	resHeader.compType = compress;
	resHeader.uSize = bufSize;
	resHeader.size = 0;
	resHeader.type = RES_ANIM;
	resHeader.numHeaders = (uint16) numCels;
	resHeader.sizeOfHeaders = (uint16) (chSize * resHeader.numHeaders);

	for (i = 0; i < RH_DATA_MAX; i++)
	{
		resHeader.data[i] = 0;
	}
	resHeader.genID = GENERATION;

	// Create the final buffer and copy data.
	if (compress == CMP_TRLE)
	{
		// this may be larger than the bufSize.  
		// allocate a larger buffer to avoid crashing.

		// find the cel height total
		totalCelHeight = 0;
		for (i = 0; i < numCels; i++)
		{
			totalCelHeight += pCelDataArray[i]->celHeader.height - 1;
		}

		TRACK_MEM("Create Anim");
		pResData = (uchar*) mem_malloc((bufSize * 2) + rhSize +
						(sizeof(ScanOffsetTable) * numCels) + 
						(sizeof(uint16) * totalCelHeight));
	}
	else
	{
		// no compression
		TRACK_MEM("Create Anim");
		pResData = (uchar*) mem_malloc(bufSize + rhSize);
	}

	memcpy(pResData, &resHeader, rhSize);
	offset = rhSize;
	for (i = 0; i < numCels; i++)
	{
		memcpy(pResData + offset, &pCelDataArray[i]->celHeader, chSize);
		offset += chSize;
	}

	// Let's see if we want to RLE compress.
	// NOTE: This MUST be done before CMP_NONE because
	// we may find that this compression is bigger than
	// if we didn't compress at all.

	if (compress & CMP_TRLE)
	{
		uint32	dataSize = 0;
		uint32	dstOffset = offset;
		uint32	encodedCelSize;

		// recalc size of buffer to write
		workBufSize = 0;

		for (i = 0; i < numCels; i++)
		{
			// reset offset
			pCelDataArray[i]->celHeader.offset = (dataSize);

			// encode and copy cel data
			encodedCelSize = CopyTRLE(pResData + dstOffset, pCelDataArray[i]);

			dataSize  += encodedCelSize;
			dstOffset += encodedCelSize;
			workBufSize	 += encodedCelSize + chSize;
		}

		if (workBufSize > bufSize)
		{
			// This would be bigger than if we didn't compress!!!
			// Let's not compress this way
			compress &= ~CMP_TRLE;
			resHeader.compType = compress;
			// re-write the update resHeader header
			memcpy(pResData, &resHeader, rhSize);

			// tell the user just to be nice
			printf("\nTRLE produced output larger than the source.");
			printf("\nTRLE won't be used as requested.\n");
		}
		else
		{
			bufSize = workBufSize;
			// re-write headers with updated offsets
			resHeader.uSize = bufSize;
			memcpy(pResData, &resHeader, rhSize);

			for (i = 0; i < numCels; i++)
			{
				memcpy(pResData + rhSize + (chSize*i), &(pCelDataArray[i]->celHeader), chSize);
			}
		}
	}

	if (compress == CMP_NONE)
	{
		for (i = 0; i < numCels; i++)
		{
			celHeader = pCelDataArray[i]->celHeader;
			celSize = celHeader.width * celHeader.height;
			memcpy(pResData + offset, pCelDataArray[i]->celData, celSize);
			offset += celSize;
		}
	}

	// Write the resource to disk
	if (WriteResFile(RES_ANIM, number, pResData, bufSize))
	{
		mem_free (pResData);
		return TRUE;
	}

	mem_free (pResData);
	return FALSE;
}



bool
CreatePic(uint number, uint numCels, CelArrayElement* pCelDataArray[], bool compress)
{
	uint16		i;
	ResHeader 	resHeader;
	CelHeader	celHeader;
	uint32		bufSize = 0;
	uint32		celSize = 0;
	uint32		singleCelSize = 0;
	uint32		offset = 0;

	compress = compress;		// Gets rid of unreferenced warning

	// Calculate the size of the resource.
	for (i = 0; i < numCels; i++)
	{
		pCelDataArray[i]->celHeader.offset = celSize;
		celHeader = pCelDataArray[i]->celHeader;
		singleCelSize = celHeader.width * celHeader.height;
		bufSize += chSize + singleCelSize;
		celSize += singleCelSize;
	}

	// Setup the header.
	resHeader.compType = CMP_NONE;
	resHeader.uSize = bufSize;
	resHeader.size = 0;
	resHeader.type = RES_PIC;
	resHeader.numHeaders = (uint16) numCels;
	resHeader.sizeOfHeaders = (uint16) (chSize * resHeader.numHeaders);

	for (i = 0; i < RH_DATA_MAX; i++)
	{
		resHeader.data[i] = 0;
	}
	resHeader.genID = GENERATION;
	
	// Create the final buffer and copy data.
	TRACK_MEM("Create Pic");
	uchar* pResData = (uchar*) mem_malloc(bufSize + rhSize);
	memcpy(pResData, &resHeader, rhSize);
	offset = rhSize;
	for (i = 0; i < numCels; i++)
	{
		memcpy(pResData + offset, &pCelDataArray[i]->celHeader, chSize);
		offset += chSize;
	}
	for (i = 0; i < numCels; i++)
	{
		celHeader = pCelDataArray[i]->celHeader;
		celSize = celHeader.width * celHeader.height;
		memcpy(pResData + offset, pCelDataArray[i]->celData, celSize);
		offset += celSize;
	}
	
	// Write the resource to disk
	if (WriteResFile(RES_PIC, number, pResData, bufSize))
	{
		mem_free (pResData);
		return TRUE;
	}

	mem_free (pResData);
	return FALSE;
}


// Create a font resource.  'celData' contains information about the font
// and its data which will be written into the resource.
bool
CreateFont(uint number, uint numChars, FontHeader fHdr, FontArrayElement* pFontDataArray[], bool compress)
{
	uint16			i;
	ResHeader 		resHeader;
	FontCharHeader	charHeader;
	uint32			bufSize = 0;			// total resource size (- res header)
	uint32			charSize = 0;			// size of all characters
	uint16			singleCharSize = 0;	// size of current character
	uint32			offset = 0;
	uint16			cWide;

	compress = compress;		// Gets rid of unreferenced warning

	// Calculate the size of the resource.
	for (i = 0; i < numChars; i++)
	{
		pFontDataArray[i]->charHeader.offset = charSize;
		charHeader = pFontDataArray[i]->charHeader;
		cWide = (uint16) (charHeader.charWide ? charHeader.charWide : 1);
		singleCharSize = (uint16) (cWide * fHdr.height);
		bufSize += fchSize + singleCharSize;
		charSize += singleCharSize;
	}
	bufSize += fhSize;

	// Setup the header.
	resHeader.compType = CMP_NONE;
	resHeader.uSize = bufSize;
	resHeader.size = 0;
	resHeader.type = RES_FONT;
	resHeader.numHeaders = (uint16) numChars;
	resHeader.sizeOfHeaders = (uint16) (fhSize + (fchSize * resHeader.numHeaders));

	for (i = 0; i < RH_DATA_MAX; i++)
	{
		resHeader.data[i] = 0;
	}
	resHeader.genID = GENERATION;
	
	// Create the final buffer and copy data.
	TRACK_MEM("Create Font");
	uchar* pResData = (uchar*) mem_malloc(bufSize + rhSize);
	memcpy(pResData, &resHeader, rhSize);
	memcpy(pResData + rhSize, &fHdr, fhSize);
	offset = rhSize + fhSize;
	for (i = 0; i < numChars; i++)
	{
		memcpy(pResData + offset, &pFontDataArray[i]->charHeader, fchSize);
		offset += fchSize;
	}
	for (i = 0; i < numChars; i++)
	{
		charHeader = pFontDataArray[i]->charHeader;
		cWide = (uint16) (charHeader.charWide ? charHeader.charWide : 1);
		charSize = cWide * fHdr.height;
		memcpy(pResData + offset, pFontDataArray[i]->charData, charSize);
		offset += charSize;
	}
	
	// Write the resource to disk
	if (WriteResFile(RES_FONT, number, pResData, bufSize))
	{
		mem_free (pResData);
		return TRUE;
	}

	mem_free (pResData);
	return FALSE;
}



// Create the file name for the resource.
void
CreateResFileName(res_t type, uint number, char* name)
{
	switch (type)
	{
		case RES_CEL:
			sprintf(name, "%d.CEL", number);
			break;

		case RES_ANIM:
			sprintf(name, "%d.ANI", number);
			break;

		case RES_PIC:
			sprintf(name, "%d.PIC", number);
			break;

		case RES_MIDI:
			sprintf(name, "%d.MID", number);
			break;

		case RES_DAC:
			sprintf(name, "%d.DAC", number);
			break;

		case RES_PAL:
			sprintf(name, "%d.PAL", number);
			break;

		case RES_DATA:
			sprintf(name, "%d.DAT", number);
			break;

		case RES_FONT:
			sprintf(name, "%d.FON", number);
			break;

		case RES_CONV:
			sprintf(name, "%d.CNV", number);
			break;

		case RES_SQUIB:
			sprintf(name, "%d.SQB", number);
			break;

		case RES_CHOREO:
			sprintf(name, "%d.cgf", number);
			break;

		case RES_ROUTE:
			sprintf(name, "%d.rot", number);
			break;

		case RES_TILELIB:
			sprintf(name, "%d.tlb", number);
			break;

		case RES_MAP_INFO:
			sprintf(name, "%d.MIF", number);
			break;
	}
}


// Open a resource file.
FILE*
OpenResFile(char* filename)
{
	FILE* pResFile = fopen(filename, "rb");

	if (pResFile)
	{
		fclose(pResFile);

		if (fIsTextMode) 
		{
			// File exists, prompt to overwrite
			printf("File exists.  Overwrite? (Y/N) ");
			int c = 0;
			while (c != 'Y' && c != 'N' && c != 'y' && c != 'n')
			{
				c = getc(stdin);
			}

			if (c == 'N' || c == 'n')
			{
				printf("Write aborted.\n");
				return FALSE;
			}
			printf("\n\n");
		}
	}

	// Open the file for write access
	pResFile = fopen(filename, "wb");
	return pResFile;
}


// Write a resource.
// The bufSize passed should be the buffer size minus the resheader size.

bool
WriteResFile(res_t type, uint number, uchar *pResData, uint32 bufSize) 
{
	FILE*		pResFile;
	char		szFileName[FILENAME_MAX];
	uint32 	compressedSize;
	ResHeader	*resHeader;
	bool		writeResult = FALSE;
	uchar		*pData = pResData + rhSize;

	resHeader = (ResHeader *) pResData;

	while (TRUE)
	{
		// Create the disk file.
		CreateResFileName(type, number, szFileName);
		pResFile = OpenResFile(szFileName);

		if (pResFile)
		{
			// write the resource header
			if (fwrite(pResData, 1, rhSize, pResFile) != rhSize)
			{
				// some sort of write problem
				break;
			}

			// write the sub headers
			if (resHeader->sizeOfHeaders)
			{
				if (fwrite(pData, 1, resHeader->sizeOfHeaders, pResFile) != resHeader->sizeOfHeaders)
				{
					// some sort of write problem
					break;
				}

				// move the data pointer past the sub headers
				pData += resHeader->sizeOfHeaders;
			}

			// write the resource data with possible compression
			compressedSize = AFlatten(pResFile, bufSize - resHeader->sizeOfHeaders, (char *) pData,
							resHeader->compType);

			if (compressedSize > bufSize)
			{
				// The compressed file was greater than the original size.
				// Try again without compression (TRLE has already been done,
				// so keep that bit).
				resHeader->compType &= CMP_TRLE;
				fclose(pResFile);
			}
			else
			{
				// update the res header
				// move to start of file
				fseek(pResFile, 0L, SEEK_SET);
				resHeader->uSize -= resHeader->sizeOfHeaders;
				resHeader->size = compressedSize;

				// rewrite the resource header
				if (fwrite(pResData, 1, rhSize, pResFile) != rhSize)
				{
					// some sort of write problem
					break;
				}
				writeResult = TRUE;
				// we are done here.
				break;
			}
		}
		else
		{
			// file wasn't opened.  exit
			break;
		}
	}

	if (pResFile)
	{
		fclose(pResFile);
	}

	return writeResult;
}

//-------------------------------------------------------------------
//	Tagged-Run-Length Encode celdata


//uint32
//CopyTRLE(uchar* pResData, CelArrayElement* pElem)
//{
//	CelHeader			celHeader;
//	uchar*				pSrc;
//	uchar*				pDst;
//	uchar*				pBase;
//	unsigned				i;
//	ScanOffsetTable*	pTable;
//	unsigned				iPage = 0;	
//
//	pSrc = pElem->celData;
//	uchar* pTest = pSrc + 780;
//
//	celHeader = pElem->celHeader;
//
//	// calc distance past variable-length scan table
//	pTable = (ScanOffsetTable*) pResData;
//	pBase = (uchar*) pTable;
//	pDst = pResData + sizeof(ScanOffsetTable) + (sizeof(uint16) * (celHeader.height-1));
//
//	// init table lastYofPage
//	pTable->lastYofPage[0] = USHRT_MAX;
//	pTable->lastYofPage[1] = USHRT_MAX;
//	pTable->lastYofPage[2] = USHRT_MAX;
//	pTable->lastYofPage[3] = USHRT_MAX;
//
//	// pack each scan line
//	for (i=0; i<celHeader.height; i++)
//	{
//		// if beyond the range of uint16, mark previous y as last of that page
//		if ((pDst - pBase) > USHRT_MAX)
//		{
//			pTable->lastYofPage[iPage++] = (uint16) (i - 1);
//
//			if (iPage > MAX_TRLE_PAGES)
//			{
//				printf ("\nERROR: Run Length Encoded cels can't be greater 327,000 bytes\n");
//				exit(1);
//			}
//			pBase += USHRT_MAX;
//		}
//
//		pTable->offset[i] = (uint16) (pDst - pBase);
//
//		pDst = PackRLE(pDst, pSrc, celHeader.width);
//		pSrc += celHeader.width;
//
//	}
//	
//	return (pDst - pResData);
//}




