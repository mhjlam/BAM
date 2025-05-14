//
// FMT_LBM.CPP
//
// Modular File Format Conversion Tool
// DPaint LBM Format
//
// (C) Copyright 1994, Intellisoft
//
//	3/17/95	Kim Bowdish
// 9/21/95	Kevin Sherrill -modified to work with snap
//
//---[]--------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include	"fmt_lbm.hpp"
#include	"mono.hpp"
#include	"api.hpp"
#include	"apimem.hpp"

FormatLBM::FormatLBM()
{
	base.sourceFmtType = FT_SC_GRAPHIC;
	base.targetFmtType = FT_SC_GRAPHIC;
	base.origin = EX_DONT_CARE;
	base.pal = EX_DONT_CARE;
}


bool
FormatLBM::Write(char* filename)
{
	int 	i;
	int	result;
	Chunk	chunk;
	uchar	cbuf[GENERIC_SIZE];
	int	nBytes = 0;
	int	offset = 0;

	fileSize = 0;
	width    = motr2inti(TINY_COLUMNS);
	height   = motr2inti(TINY_ROWS);

	fLBM = fOpenPrefs(filename, "wb");
	if (!fLBM)
	{
		APanic("Snap: Cannot open file\n");
	}

		// will write the current info into header.  After compressing 
		// sData we will have to rewrite the corrected fileSize
		// over the old (uncompressed) value.
 	WriteHeader();

	strcpy(chunk.id, "CMAP");
	chunk.size = 0x00030000;
	fwrite(&chunk, sizeof(Chunk), 1, fLBM);

	//write entire palette array prepared by Snap
	fwrite(lbmPal, sizeof(lbmPal), 1, fLBM);


		// for the generic DPPS and CRNG (all zeros)
	memset(cbuf, 0, DPPS_SIZE + CRNG_SIZE);

		// write DPPS
	strcpy(chunk.id, "DPPS");
	chunk.size = 0x6e000000;
	fwrite(&chunk, sizeof(Chunk), 1, fLBM);
	fwrite(cbuf, 110, 1, fLBM);

		// write all 16 empty CRNG's
	strcpy(chunk.id, "CRNG");
	chunk.size = 0x08000000;
	for (i=0; i<16; i++)
	{
		fwrite(&chunk, sizeof(Chunk), 1, fLBM);
		fwrite(cbuf, 8, 1, fLBM);
	}


	FillInTinyBuffer();				// every 4th pixel of srcBuff (sData)

		// RLE the sData into curFramebm, then
		// copy the RLE info (curFramebm) back to sData.
	for (i = 0; i < L_UDATA_ROWS; i++)
	{
			// encode a line at a time
		result = DoRLE(sData + offset, L_UDATA_COLUMNS, nBytes);
		offset += L_UDATA_COLUMNS;
		nBytes += result;
	}

	if (ON_ODD_BOUNDRY(nBytes))		// make it an even boundry by added a zero.
	{
		*(curFramebm + nBytes) = 0;
		nBytes++;
	}

	memcpy(sData, curFramebm, nBytes);

		// update to the real (compressed) size
	base.totalSize = nBytes;
	sHdr.size = nBytes;

		// RLE the tiny buffer into curFramebm.
	offset = 0;
	tinyBlockSize = 0;
	for (i = 0; i < TINY_ROWS; i++)
	{
		result = DoRLE(tinyBuffer + offset, TINY_COLUMNS, tinyBlockSize);
		offset += TINY_COLUMNS;
		tinyBlockSize += result;
	}

		// write tiny
	strcpy(chunk.id, "TINY");
	chunk.size = motr2intl(tinyBlockSize + TINY_SIZE_HDR);
	fwrite(&chunk, sizeof(Chunk), 1, fLBM);
	fwrite(&width,  sizeof(short), 1, fLBM);
	fwrite(&height, sizeof(short), 1, fLBM);
		// if the size of the tiny is odd then add one byte ("0") to end 
		//	to make the next chunk (BODY) start on an even boundry.
	if (ON_ODD_BOUNDRY(tinyBlockSize))
	{
		*(curFramebm + tinyBlockSize) = 0;
		tinyBlockSize++;
	}
	fwrite(curFramebm, tinyBlockSize, 1, fLBM);

		// write sData
	strcpy(chunk.id, "BODY");
	if (ON_ODD_BOUNDRY(sHdr.size))
	{
		chunk.size = motr2intl(sHdr.size - 1);
	}
	else
	{
		chunk.size = motr2intl(sHdr.size);
	}
	fwrite(&chunk, sizeof(Chunk), 1, fLBM);
	fwrite(sData, sHdr.size, 1, fLBM);

			// go back to beg. of file and put the new (correct) filesize in.
	fseek(fLBM, 0, SEEK_SET); 				
	strcpy(lbmHeader.id, "FORM");

	fileSize = LBMHEADER_SIZE + CMAP_SIZE + DPPS_SIZE + CRNG_SIZE + 
			     tinyBlockSize + TINY_SIZE_HDR + CHUNK_SIZE + base.totalSize;

	lbmHeader.fileLength = motr2intl(fileSize);
	sHdr.size = base.totalSize;
	fwrite(lbmHeader.id, 4, 1, fLBM);
	fwrite(&lbmHeader.fileLength, 4, 1, fLBM);

	fclose(fLBM);
	return TRUE;
}


bool
FormatLBM::Read(char* filename)
{
	int 	  i;
	Chunk	  bl;

	fLBM = fOpenPrefs(filename, "rb");
	if (!fLBM)
	{
		APanic("Snap: Cannot open file.\n");
	}

 	if (ReadHeader() < 0)
	{
		APanic ("\nCan't read header\n");
	}

		// Read palette
	fread(&bl, sizeof(Chunk), 1, fLBM);
   if (!memcmp(bl.id, "CMAP", 4))
	{
		for (i = 0; i < MAX_COLORS; i++)
		{
			fread(&lbmPal[i], sizeof(lbmPal[i]), 1, fLBM);
		}
	}

	width  = motr2inti(lbmHeader.width);
	height = motr2inti(lbmHeader.length);

	if (!(ReadData(sData)))
	{
		APanic("Snap: Error reading sData file\n");
	}

	sHdr.orgX	 = 0;
	sHdr.orgY	 = 0;
	sHdr.width  = (uint16)width;
	sHdr.height = (uint16)height;
	sHdr.size   = width * height;
	base.totalSize  = sHdr.size;		// uncompressed size
	base.numColors  = MAX_COLORS;
	base.numUnits 	 = 1;

		// now close the src file
	fclose (fLBM);

	return TRUE;
}

long motr2intl(long l)
{
	return(((l & 0xff000000L) >> 24) +
	       ((l & 0x00ff0000L) >> 8) +
	       ((l & 0x0000ff00L) << 8) +
	       ((l & 0x000000ffL) << 24));
}

int motr2inti(int n)
{
	return(((n & 0xff00) >> 8) | ((n & 0x00ff) << 8));
}

bool
FormatLBM::WriteHeader()
{
	fileSize = LBMHEADER_SIZE + CMAP_SIZE + DPPS_SIZE	+ CRNG_SIZE + 
				   tinyBlockSize + CHUNK_SIZE + base.totalSize;

	memset(&lbmHeader, 0, sizeof(LBMHeader));

	strcpy(lbmHeader.id, "FORM");
	lbmHeader.fileLength = motr2intl(fileSize);
	strcpy(lbmHeader.fileType, "PBM BMHD");
	lbmHeader.hdrLength = 0x14000000;
	lbmHeader.width  = 16385;
	lbmHeader.length = 36865;
	lbmHeader.nPlanes = 8;
  	lbmHeader.compression = 1;
	lbmHeader.transparentColor = 0xfe00;
	lbmHeader.xAspect = 5;
	lbmHeader.yAspect = 6;
	lbmHeader.pageW = 16385;
	lbmHeader.pageH = (int16)motr2inti(0xc8);
	fwrite(&lbmHeader, sizeof(LBMHeader), 1, fLBM);

	return TRUE;
}

	// read the LBMHeader (first thing in the file)
int16
FormatLBM::ReadHeader()
{
	fread(&lbmHeader, sizeof(LBMHeader), 1, fLBM);

	if (!memcmp(lbmHeader.id, "FORM",4) ||
	    !memcmp(lbmHeader.id, "LIST",4) ||
	    !memcmp(lbmHeader.id, "CAT ",4)) 
	{
		lbmHeader.hdrLength = motr2intl(lbmHeader.hdrLength);
		if (lbmHeader.hdrLength & 1L)
		{
			++lbmHeader.hdrLength;
		}
				// check for bitmap header 
		if (memcmp(lbmHeader.fileType, "PBM BMHD", 8))		// not equal
		{
			APanic("Snap: read header - something wrong...\n");
		}

	} 
	else 
	{
		APanic ("BAD FILE - no FORM,LIST,or CAT\n");
	}

	return TRUE;
}


// read a compressed RLE line 
// and uncompress it into raw bitmap data
int
FormatLBM::UncompressRLE(uchar* pSrc, int bytes)
{
	int c,i,n=0;
	offset = 0;

	do 
	{
		c = *(pSrc + offset) & 0xff;		// get char
		offset++;
		if (c & 0x80)				// run
		{
			i = ((~c) & 0xff) + 2;		// take the high bit off (the rest is the count)
			c = *(pSrc + offset);		// get char
			offset++;
			while(i--) 
			{
				*(sData + n) = (uchar)c;
				n++;
			}
		}
		else							// dump
		{
			i = c + 1;
			while(i--)
			{
				*(sData + n) = *(pSrc + offset);
				n++;
				offset++;
			}
		}
	} while(offset < (bytes - 1));

	return(n);
}

int
FormatLBM::ReadData(uchar* buff)
{
	Chunk	 bl;
	int	 n = 0;
	int	 nBytes = 0;

	buff = buff;

		// read & skip all the chunks until we hit the BODY chunk.
		// when all is done. sData will have the uncompressed bitmap
		// data in it.
	do 
	{
		fread(&bl, sizeof(Chunk), 1, fLBM);
		bl.size  = motr2intl(bl.size);
		if (bl.size & 1L) ++bl.size;

		if (!memcmp(bl.id, "BODY", 4)) 
		{
			int	 result;
			result = fread(curFramebm, bl.size, 1, fLBM);
			if (result != 1)
			{
				APanic("Snap: read sData wrong\n");
			}
			nBytes = UncompressRLE(curFramebm, bl.size);
		}
		else
		{
			fseek(fLBM, bl.size, SEEK_CUR);
		}
	} while (!ferror(fLBM) && memcmp(bl.id, "BODY", 4));

	sHdr.size = nBytes;

	return nBytes;
}

	// when all is done curFramebm will have the finished info.
	// NOTE: we MUST do a line at a time, because D-Paint is expecting
	// the data to be in line length sections.
int
FormatLBM::DoRLE(uchar* pSrc, int size, int destOffset)
{
	uchar*	pDst;			  
	bool		done = FALSE; 
	uint		count;				// # pixels in current block
	int		nBytes = 0;	  		// # bytes of RLE'd data in this block
	uchar*	dumpPtr;				// memcpy'ing part of src to curFramebm
	int		countLeft = 0;
	int		lineCount = 0;		// end of block is when we hit the end of the line
	int		lineLeft = 0;		// how much is left in the line

	offset = 0;

	pCurSrc = pSrc;

		//Increment the destination buffer by line length amount too.
	pDst = curFramebm + destOffset;
 
	while (!done)
	{
		// Grab a block of pixels
		// Mode will get set to MODE_DUMP or MODE_RUN
		// lineLeft ranges from 1 to size (linelength)

		lineLeft = size - lineCount;

		count = CompareBlock(lineLeft);

		lineCount += count;

		switch (mode)
		{
			case MODE_DUMP:

					// dumps are coded: count - 1, the dump data.
					//  (i.e. a dump of 3 bytes -> 02 fe fc fe)
					//  max dump = 0x7f

				dumpPtr = pSrc + offset - count;

				while (count)
				{
					countLeft = (uchar)AFMin(count, 0x80);
					*(pDst++) = (uchar)(countLeft - 1);
					memcpy(pDst, dumpPtr, countLeft);
					dumpPtr += countLeft;
					pDst 	  += countLeft;
					nBytes  += countLeft + 1;
					count -= countLeft;
				}
				break;

			case MODE_RUN:

					// runs are coded: ~(count - 2), run pixel
					//  (i.e. a run of 128 fe's ->  81 fe)
					//  max run = 0x81

				while (count)
				{
					countLeft = (uchar)(AFMin(count, 0x80));

					*(pDst++) = (uchar)~(countLeft - 2);
					*(pDst++) = lastPixel;
					count -= countLeft;
					nBytes += 2;
				}
				break;
		}
		if (lineCount >= size)
		{
			done = TRUE;
			break;
		}
	}

	return nBytes;
}

int
FormatLBM::CompareBlock(int size)
{
	int countRun = 0;
	int countDump = 0;

	blkCount = 0;
	if (ComparePixel())
	{
			// curPixel and lastPixel are same
		mode = MODE_RUN;
	} 
	else
	{
			// curPixel and lastPixel are different
		mode = MODE_DUMP;
	}

		// Save the current mode for comparison later
	lastMode = mode;

		// Compare until the mode changes or we reach the end of the line
	while (mode == lastMode) 
	{
		if (ComparePixel())
		{
			// Pixels are the same

			countDump = 0;

			switch (mode)
			{
				case MODE_RUN:

					// We are in a run block.
					break;

				case MODE_DUMP:

					countRun++;

						// change mode to a run cause the first check is bogus.
						// trying to check lastpixel on first one is who knows.
					if (blkCount == 2)
					{
						mode = MODE_RUN;
						lastMode = MODE_RUN;
					}
					else
					{
							// really the second one that is the same...
							// so take it off to get correct dump and count
						if (countRun > 1)
						{
							offset -= countRun;
							blkCount -= countRun;
							mode = MODE_DONE;
						}
					}
					break;
			}
		}
		else
		{
			// Pixels are different

			switch (mode)
			{
				case MODE_RUN:

					countDump++;

					if (blkCount < 3)
					{
						// change mode to a dump cause the first pixel check is bogus.
						// trying to check lastpixel on first one is who knows.
						countRun = 0;
						mode = MODE_DUMP;
						lastMode = MODE_DUMP;
					}
					else
					{
				  		mode = MODE_DONE;
					}
					break;

				case MODE_DUMP:

					// We are in a dump block.
					countRun = 0;
					break;
			}
		} 
		if (blkCount > size)	// we are only doing a line at a time
			mode = MODE_DONE;
	} 

	// Back up the counters to reflect the last pixel compared
	--offset;
	--blkCount;

	// Since the mode is MODE_DONE, set it back to what it was
	// at the beginning of the compare.
	mode = lastMode;
	
	return blkCount;
}


// Compare two pixels, (curPixel & lastPixel).
// Return TRUE if they are the same
// Return FALSE if they are different
//
bool
FormatLBM::ComparePixel()
{
	bool		ret = FALSE;

	curPixel = *(pCurSrc + offset);
	lastPixel = *(pCurSrc + offset - 1);

	if (lastPixel == curPixel)
	{
		ret = TRUE;
	}

	// Increment the counters to the next pixel in the buffer
	++offset;
	++blkCount;
	return ret;
}
	
	// tiny picture is scaled to 1/4 of the main picture.
	// tiny picture is generated by every 4th pixel of the main picture.
void
FormatLBM::FillInTinyBuffer()
{
	int	   lineNo, colNo;
	uchar*	pDst;
									
	pDst = tinyBuffer;

	for (lineNo = 0; lineNo < L_UDATA_ROWS; lineNo += 4)
	{
		for (colNo = 0; colNo < L_UDATA_COLUMNS; colNo += 4)
		{
			*(pDst++) = *(sData + ((lineNo * L_UDATA_COLUMNS) + colNo));
		}
	}
}

																 
