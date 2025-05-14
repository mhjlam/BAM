//
// MAKERES.CPP
//
// February 8, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	Create a resource file.
//
//----[]-------------------------------------------------------------


#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.hpp"
#include "tigre.hpp"
#include "writeres.hpp"

//----[Stub Functions]-----------------------------------------------


//----[Function Prototypes]------------------------------------------

uint		PromptForPCXNames(resType theType, uint& first, uint& last);
bool		PromptForMFNName(uint& first);
int16		ReadPCXHeader(char* bmpName);
bool		ReadPCXData(uchar* buffer);
bool		ExpandScanLine(uint32& scanLine, uchar* buffer);

int16		ReadMFNHeader(char* bmpName);


//----[PCX File Structures]------------------------------------------

// #pragma pack(1)

typedef struct
{
	uchar	red;
	uchar	green;
	uchar	blue;
} ColorRegister;

struct PCXHeader
{
	uchar		header;
	uchar		version;
	uchar		Encode;
	uchar		bitPerPix;
	uint16	x1;
	uint16	y1;
	uint16	x2;
	uint16	y2;
	uint16	hres;
	uint16	vres;
};

struct PCXInfo
{
	uchar		vmode;
	uchar		numOfPlanes;
	uint16	bytesPerLine;
	uchar		unused[60];
};


struct PCXFormat
{
	PCXHeader			fileHeader;
	ColorRegister		Palette[16];
	PCXInfo				fileInfo;
};

// #pragma pack()

PCXFormat	PCXFile;
FILE*			bmpFile;


//----[MFN File Structures]------------------------------------------

struct MFNFormat
{
	FontHeader	fileHeader;
	void*			fileData;
};

MFNFormat	MFNFile;


//----[Function Main]------------------------------------------------

void
main(int argc, char* argv[])
{
	int		theType;
	uint		theNum;
	size_t	theSize;
	bool		theCompress;
	char		ext[4];
	char		c;

	char*		pData;
	uint16	i, j;
	char		buffer[40];
	uint		firstPCX;
	uint		lastPCX;
	uint		numCels;
	uint		numChars;
	uint16	cWide;

	CelArrayElement*	pCelData[16];
	FontArrayElement*	pFontData[256];
	uint16	aWordData[4];

	Configuration*	pConfig;
	pConfig = new Configuration("res_cfg.hpp", 2, 11, 4);


	// If incorrect number of arguments or unknown resType,
	//	give syntactic help.
	//
	theType = (argc > 1) ? pConfig->HasEntry(argv[1]) : 0;
	if (argc < 3 || argc > 5 || theType == -1)
	{
		printf("\nUsage: MAKERES resType resNum [size [compressed]]\n\n");
		printf("Types: ");
		for (c = 0; c < CFG_ENTRY_MAX; c++)
		{
			pConfig->CopyStr(buffer, c, 2);
			if (!strcmp(buffer, ""))
			{
				break;
			}
			if (c)
				printf("       ");
			printf("%d = %s\n", c, buffer);
		}
		printf("\n");
		printf("Example: MAKERES RES_PIC 15 50\n");
		printf("         Will make an uncompressed 50K pic resource called 15.PIC\n\n");
		printf("         MAKERES RES_CEL 225 0 1\n");
		printf("         Will make a compressed cel resource called 225.CEL\n\n");
		exit(1);
	}


	// Get parameters into temps.
	//
	theNum = atoi(argv[2]);
	theSize = (argc > 3) ? atoi(argv[3]) * 1024 : 0;
	theCompress = ((argc > 4) && atoi(argv[4]) > 0) ? TRUE : FALSE;


	// Create the resource, according to type:
	//
	switch (theType)
	{
		case RES_CEL:
		case RES_ANIM:
		case RES_PIC:
		case RES_PAL:


			// If this is a cel, anim, pic, or palette resource, prompt for .PCX
			//	files from which to load the data.  Pics and anims use multiple
			//	.PCX files, numbered sequentially, and cels and palettes use a
			// single .PCX file.
			//
			numCels = PromptForPCXNames((resType) theType, firstPCX, lastPCX);
			if (numCels)
			{
				if (theType != RES_PAL)
				{
					CelHeader cel;

					// For each .PCX file, setup the cel header and read the cel
					//	data
					for (i = 0; i < numCels; i++)
					{
						sprintf(buffer, "%d.PCX", firstPCX + i);
						if (ReadPCXHeader(buffer) < 0)
						{
							exit(1);
						}

						cel.orgX = 0;
						cel.orgY = 0;
						cel.width = PCXFile.fileHeader.hres;
						cel.height = PCXFile.fileHeader.vres;
						cel.priority = 0;
						cel.offset = 0;
						pCelData[i] = new CelArrayElement;
						pCelData[i]->celHeader = cel;
						pCelData[i]->celData = (uchar*) malloc(cel.width * cel.height + 2);
						if (!ReadPCXData((uchar*) pCelData[i]->celData))
						{
							printf("Error reading .PCX file");
							exit(1);
						}
					}
				}
				else
				{
					// Load the palette data into the data buffer
					sprintf(buffer, "%d.PCX", firstPCX);
					theSize = 256*3;
					pData = (char*) malloc(theSize);
					memset(pData, NULL, 256*3);
					if (ReadPCXHeader(buffer) < 0)
					{
						exit(1);
					}

					fseek(bmpFile, -256*3, SEEK_END);
					fread(pData, theSize, 1, bmpFile);
					fclose(bmpFile);
				}
			}
			else
			{
				return;
			}
			break;

		case RES_FONT:

			// If this is a font resource, prompt for .MFN (Mac Font) file from
			//	which to load the data.  .MFN files may be any number of characters
			// from 1 to 256, each of which is represented by an XxY bitmap. The
			// bitmaps are prefaced by an equal number of Character Headers.
			//
			if (PromptForMFNName(firstPCX))
			{
				#define	CEL_SIZE		MFNFile.fileHeader.bufWidth * MFNFile.fileHeader.height

				sprintf(buffer, "%d.MFN", firstPCX);
				if (ReadMFNHeader(buffer) < 0)
				{
					exit(1);
				}

				uint		first = MFNFile.fileHeader.firstChar;
				uint		last = MFNFile.fileHeader.lastChar;
				uchar*	celBuf = (uchar*) malloc(CEL_SIZE);
				numChars = last - first + 1;

				printf("Source parameters:\nMFNFile.fileHeader.widest=%d, bufWidth=%d, height=%d, ascent=%d\n",
					MFNFile.fileHeader.widest, MFNFile.fileHeader.bufWidth, MFNFile.fileHeader.height, MFNFile.fileHeader.ascent);
				printf("MFNFile.fileHeader.descent=%d, leading=%d, firstChar=%c, lastChar=%c\n",
					MFNFile.fileHeader.descent, MFNFile.fileHeader.leading, (char)MFNFile.fileHeader.firstChar, (char)MFNFile.fileHeader.lastChar);
				printf("numChars = %d\n", numChars);

				// Read the character headers
				for (i = 0; i < numChars; i++)
				{
					pFontData[i] = new FontArrayElement;
					fread(&pFontData[i]->charHeader, sizeof(uint16), 1, bmpFile);
					pFontData[i]->charHeader.charWide >>= 8;
					pFontData[i]->charHeader.charWide += 3;
				}

				// Read the font data into a buffer.
				for (i = 0; i < numChars; i++)
				{
					memset(celBuf, NULL, CEL_SIZE);
					fread(celBuf, CEL_SIZE, 1, bmpFile);

					// Cut out the actual size of the character.
					cWide = pFontData[i]->charHeader.charWide;
					if (!cWide)
					{
						++cWide;
					}
					pFontData[i]->charData = (uchar*) malloc(cWide * MFNFile.fileHeader.height);
					uint16 srcOffset = 0;
					uint16 dstOffset = 0;
					for (j = 0;	j < MFNFile.fileHeader.height; j++)
					{
						memcpy(pFontData[i]->charData + dstOffset,
									celBuf + srcOffset,
									cWide);
						srcOffset += MFNFile.fileHeader.bufWidth;
						dstOffset += cWide;
					}
				}
			}
			else
			{
				return;
			}
			break;

		default:
			// Just create a buffer of the size specified on the command line.
			pData = (char*) malloc(theSize);
			memset(pData, NULL, theSize);
			break;
	}


	// Write the resource file
	switch (theType)
	{
		case RES_CEL:
			strcpy(ext, "CEL");
			CreateCel(theNum, pCelData[0], theCompress);
			break;

		case RES_ANIM:
			strcpy(ext, "ANI");
			CreateAnim(theNum, numCels, pCelData, theCompress);
			break;

		case RES_PIC:
			strcpy(ext, "PIC");
			CreatePic(theNum, numCels, pCelData, theCompress);
			break;

		case RES_MIDI:
			strcpy(ext, "MID");
			for (i = 0; i < 4; i++)
			{
				aWordData[i] = 0;
			}
			CreateGeneric(RES_MIDI, theNum, theSize, pData, aWordData, theCompress);
			break;

		case RES_DAC:
			strcpy(ext, "DAC");
			for (i = 0; i < 4; i++)
			{
				aWordData[i] = 0;
			}
			CreateGeneric(RES_DAC, theNum, theSize, pData, aWordData, theCompress);
			break;

		case RES_PAL:
			strcpy(ext, "PAL");
			for (i = 0; i < 4; i++)
			{
				aWordData[i] = 0;
			}
			CreateGeneric(RES_PAL, theNum, theSize, pData, aWordData, theCompress);
			break;

		case RES_DATA:
			strcpy(ext, "DAT");
			for (i = 0; i < 4; i++)
			{
				aWordData[i] = 0;
			}
			CreateGeneric(RES_DATA, theNum, theSize, pData, aWordData, theCompress);
			break;

		case RES_FONT:
			strcpy(ext, "FON");
			// height is this height of the buffer used for the font source,
			// it is not the actual height.  Now that we are through with
			// the source, lets compute the actual char height
			MFNFile.fileHeader.height = (int16) (MFNFile.fileHeader.ascent + MFNFile.fileHeader.descent 
													+ MFNFile.fileHeader.leading);
			printf("height=%d, ascent=%d, descent=%d, leading=%d\n", MFNFile.fileHeader.height, MFNFile.fileHeader.ascent, MFNFile.fileHeader.descent, MFNFile.fileHeader.leading);
			CreateFont(theNum, numChars, MFNFile.fileHeader, pFontData, theCompress);
			break;
	}

	printf("Created %s %d.%s (%dK)\n",
		theCompress ? "compressed" : "uncompressed",
		theNum,
		ext,
		theSize / 1024);
}



//----[Support Routines]---------------------------------------------

// Prompt the user for the .PCX file names to use and return the total
// number of files (1 for cels, multiple for pics and anims).  If the
// user aborts this process, returns 0.
//
uint
PromptForPCXNames(resType theType, uint& first, uint& last)
{
	int	c;
	char	buffer[50];

	printf("Import bitmap (.PCX)? (Y/n): ");
	flushall();
	c = 0;
	while (!c)
	{
		c = getche();
		switch (c)
		{
			case 13:
			case 'Y':
			case 'y':
				c = 2;
				break;

			case 'N':
			case 'n':
				c = 1;
				break;

			default:
				c = 0;
				printf("\b");
				break;
		}
	}
	
	if (c - 1)
	{
		// Ask for the single .PCX for cels and palettes
		if (theType == RES_CEL || theType == RES_PAL || theType == RES_FONT)
		{
			printf("\nEnter .PCX file number: ");
			gets(buffer);
			first = atoi(buffer);
			last = first;
			return 1;
		}
		// Ask for the multiple .PCXs for anims and pics
		else
		{
			printf("\nEnter first .PCX file number: ");
			gets(buffer);
			first = atoi(buffer);
			printf("Enter last .PCX file number: ");
			gets(buffer);
			last = atoi(buffer);
			return last - first + 1;
		}
	}
	else
	{
		return NULL;
	}
}


// Prompt the user for the .MFN file name to use.  If the
// user aborts this process, returns FALSE.
//
bool
PromptForMFNName(uint& first)
{
	int	c;
	char	buffer[50];

	printf("Import Mac font (.MFN)? (Y/n): ");
	flushall();
	c = 0;
	while (!c)
	{
		c = getche();
		switch (c)
		{
			case 13:
			case 'Y':
			case 'y':
				c = 2;
				break;

			case 'N':
			case 'n':
				c = 1;
				break;

			default:
				c = 0;
				printf("\b");
				break;
		}
	}
	
	if (c - 1)
	{
		// Ask for the single .MFN for fonts
		printf("\nEnter .MFN file number: ");
		gets(buffer);
		first = atoi(buffer);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//----[PCX Routines]-------------------------------------------------

int16
ReadPCXHeader(char* bmpName)
{
	bmpFile = fopen(bmpName, "rb");
	if (!bmpFile)
	{
		printf("Cannot open %s\n", bmpName);
		return -2;
	}

	if (fread(&PCXFile, sizeof(PCXFormat), 1, bmpFile) < 1)
	{
		printf("Error reading .PCX file header\n");
		return -4;
	}

	PCXFile.fileHeader.hres = (uint16) (PCXFile.fileHeader.x2 + 1);
	PCXFile.fileHeader.vres = (uint16) (PCXFile.fileHeader.y2 + 1);

	return FALSE;
}

bool
ReadPCXData(uchar* buffer)
{
	uint32	scanLine = 0;
	uint16	scanNum;

	for (scanNum = 0; scanNum < PCXFile.fileHeader.vres; scanNum++)
	{
		if (!ExpandScanLine(scanLine, buffer))
		{
			free(buffer);
			return FALSE;
		}
	}
	return TRUE;
}

bool
ExpandScanLine(uint32& scanLine, uchar* buffer)
{
	uchar		charRead;	//register short charRead;
	uint		repCount;

	uint		bytesToRead = PCXFile.fileInfo.numOfPlanes * PCXFile.fileInfo.bytesPerLine;
	uint16	scanChar = 0;

	do
	{
		// Read a byte from the file - error if EOF
		charRead = (uchar) fgetc(bmpFile);
		if (charRead == EOF)
		{
			return FALSE;
		}

		// See if we have a repeat tag
		if ((charRead & 0xC0) == 0xC0)
		{
			repCount = charRead - 0xC0;
			charRead = (uchar) fgetc(bmpFile);
//			if (charRead == EOF)
//			{
//				return FALSE;
//			}

			while (repCount--)
			{
				*(buffer + scanLine + scanChar) = charRead;
				++scanChar;
			}
		}
		else
		{
			*(buffer + scanLine + scanChar) = charRead;
			++scanChar;
		}
	} while (scanChar < bytesToRead);

	scanLine += PCXFile.fileHeader.hres;
	return TRUE;
}


//----[MNP Routines]-------------------------------------------------

int16
ReadMFNHeader(char* bmpName)
{
	bmpFile = fopen(bmpName, "rb");
	if (!bmpFile)
	{
		printf("Cannot open %s\n", bmpName);
		return -2;
	}

	if (fread(&MFNFile.fileHeader, sizeof(FontHeader), 1, bmpFile) < 1)
	{
		printf("Error reading .MFN file header\n");
		return -4;
	}

	// Byte-swap to fix the lame Mac's output
	MFNFile.fileHeader.widest >>= 8;
	MFNFile.fileHeader.bufWidth >>= 8;
	MFNFile.fileHeader.height >>= 8;
	MFNFile.fileHeader.ascent >>= 8;
	MFNFile.fileHeader.descent >>= 8;
	MFNFile.fileHeader.leading >>= 8;
	MFNFile.fileHeader.firstChar >>= 8;
	MFNFile.fileHeader.lastChar >>= 8;
//	MFNFile.fileHeader.pad3 >>= 8;
	return FALSE;
}
