#include <ctype.h>
#include <string.h>
#include <sys\stat.h>
#include "api.hpp"
#include "apimem.hpp"
#include "dialog.hpp"
#include "fontmgr.hpp"
#include "mono.hpp"
#include "palette.hpp"
#include "vgabuf.hpp"

#include "alldefs.hpp"
#include "tilelib2.hpp"
#include "tilelib.hpp"

#define	LBM_WIDE				320
#define	LBM_HIGH				400

// LBM BitMap header - for TileLibrary to read tile images from source LBMs.  Used by TileLibrary::RefreshPics()
typedef struct {
	unsigned short w,h;
	short x,y;
	char nPlanes;
	char masking;
	char compression;
	char pad1;
	unsigned short transparentColor;
	char xAspect,yAspect;
	short pageW,pageH;
} LBMBMHeader;

LBMBMHeader *pLBMBMHeader;

//---------------------------
//	TileGroup functions

TileGroup::TileGroup(void)
{
	memset(&h, NULL, sizeof(h));
	strcpy(h.descr, "<empty>");
	h.miniColor = CI_WHITE;
}

TileGroup::~TileGroup()
{
}

bool		// write header of this group to specified file
TileGroup::Write(FILE *output)
{
	fwrite(&h, 1, sizeof(TileGroupHeader), output);
	return(TRUE);
}

//----------------
//	Tile functions

Tile::Tile(void)
{
	memset(&h, 0, sizeof(TileHeader));
}

Tile::~Tile()
{
}

bool		// read header of this tile from specified file
Tile::Read(FILE *input)
{
	fread(&h, 1, sizeof(TileHeader), input);
	if(h.GroupNum >= MAX_TILE_GROUPS || h.GroupNum < 0)
		h.GroupNum = 0;
	if(h.swapTile >= MAX_TILES || h.swapTile < 0)
		h.swapTile = 0;
	if(h.animTile >= MAX_TILES)
		h.animTile = 0;
	if(h.animDelay >= 1000)
		h.animDelay = 0;
	//	DrawTile(ADerefAs(Tile, gSelf), 0, 0);
	return(TRUE);
}

bool		// write header of this tile to specified file
Tile::Write(FILE *output)
{
//	int loopX, loopY;
//	char	*Dest = (char *)_x32_real_to_protected(0xa0000000), *Src = (char *)h.BitMap;

	fwrite(&h, 1, sizeof(TileHeader), output);
	return(TRUE);
}

// copy data from one buffer to another - useful for extracting tiles from
// source bitmaps
bool
BlitCopy(uchar *Dest, uint16 DestWidth, uchar *Src, uint16 SrcWidth, uint16 CopyWidth, uint16 CopyHeight, uint16 SrcXOff, uint16 SrcYOff, blit_type xDir, blit_type yDir)
{
	uint16	LoopY, LoopX;
//	uint32	SrcStart, SrcEnd, DestStart, DestEnd;
	uchar		*TempDest, *TempSrc;

	Src += SrcYOff * SrcWidth + SrcXOff;

	switch(yDir)
	{
		case BLIT_TB:		// copy Top-to-Bottom (normal vertical)
			for(LoopY = 0; LoopY < CopyHeight; LoopY++)
			{
				switch(xDir)
				{
					case BLIT_LR:		// copy Left-to-Right, horizontal normal
						memcpy(Dest, Src, CopyWidth);
				//		for(LoopX = 0; LoopX < CopyWidth; LoopX++)
				//			WritePixel(LoopX, LoopY, *(Dest + LoopX));
						break;
					 case BLIT_RL:		// copy Right-to-Left, horiz reverse
						TempDest = Dest + (CopyWidth - 1);
						TempSrc = Src + (CopyWidth - 1);
					 	for(LoopX = (uint16) (CopyWidth - 1); LoopX >= 0; LoopX--)
						{
							*(TempDest--) = *(TempSrc--);
						}
				}
				Src += SrcWidth;
				Dest += DestWidth;
			}
			break;
		case BLIT_BT:	// copy Bottom-to-Top (reverse vertical)
			Dest += DestWidth * (CopyHeight - 1);
			Src += SrcWidth * (CopyHeight - 1);
			for(LoopY = (uint16) (CopyHeight - 1); LoopY >= 0; LoopY--)
			{
				switch(xDir)
				{
					case BLIT_LR:		// copy Left-to-Right, horizontal normal
						memcpy(Dest, Src, CopyWidth);
						break;
					 case BLIT_RL:		// copy Right-to-Left, horiz reverse
						TempDest = Dest + (CopyWidth - 1);
						TempSrc = Src + (CopyWidth - 1);
					 	for(LoopX = (uint16) (CopyWidth - 1); LoopX >= 0; LoopX--)
						{
							*(TempDest--) = *(TempSrc--);
						}
				}
				Src -= SrcWidth;
				Dest -= DestWidth;

			}
			break;
	}
	return(TRUE);
}

bool
Tile::LoadImage(uchar *SrcBitMap, uint16 SrcBitMapWidth, uint16 cel_x, uint16 cel_y)
{
//	uint16	loop1, loop2;
	uint16	SrcXOffSet, SrcYOffSet;

	SrcXOffSet = (uint16) (cel_x * (TILE_WIDTH + 1) + 1);
	SrcYOffSet = (uint16) (cel_y * (TILE_HEIGHT + 1) + 1);

//	pMono->Out("Tile::Loading cel %d/%d from %d wide bitmap\n", cel_x, cel_y, SrcBitMapWidth);
//	for(loop1 = 0; loop1 < TILE_HEIGHT; loop1++)
//		for(loop2 = 0; loop2 < TILE_WIDTH; loop2++)
//		  WritePixel(SrcXOffSet + loop2, SrcYOffSet + loop1, *(SrcBitMap + (SrcYOffSet + loop1) * SrcBitMapWidth + SrcXOffSet + loop2));
	BlitCopy(h.BitMap, TILE_WIDTH, SrcBitMap, SrcBitMapWidth, TILE_WIDTH,
			TILE_HEIGHT, (uint16) (cel_x * (TILE_WIDTH + 1) + 1), (uint16) (cel_y * (TILE_HEIGHT + 1) + 1));
	return(TRUE);
}

//-----------------
// TileLibrary code

TileLibrary::TileLibrary(void)		// zero everything out, leave library empty and ready
{
	TPalette *pPal;

	fogTile = 475;		// default
	memset(&h, 0, sizeof(TileLibraryHeader));
	memset(&gTile[0], 0, sizeof(grip) * MAX_TILES);
	memset(&gTileGroup[0], 0, sizeof(grip) * MAX_TILE_GROUPS);
	strcpy(h.LibName, "untitled");
	TRACK_MEM("Palette"); pPal = new TPalette;
	gPal = pPal->gSelf;
}

TileLibrary::~TileLibrary()
{
	int			loop1;
	TileGroup	*pTileGroup;

	// delete all tile group objects
	for(loop1 = 0; loop1 < MAX_TILE_GROUPS; loop1++)
		if(gTileGroup[loop1])
		{	pTileGroup = ADerefAs(TileGroup, gTileGroup[loop1]);
			ADelete(gTileGroup[loop1]);
		}

	// delete all tile objects
	for(loop1 = 0; loop1 < MAX_TILES; loop1++)
		if(gTile[loop1])
			ADelete(gTile[loop1]);

	if(gPal)
		ADelete(gPal);
}

bool ReadLBMLine(FILE *infile, uchar *bitmap, uint32& scanLine);

bool
ReadLBMLine(FILE *infile, uchar *bitmap, uint32& scanLine)
{
	uchar		charRead;	//register short charRead;
	uint16	repCount;
	uint16	bytesToRead = LBM_WIDE;
	uint16	scanChar = 0;

	do
	{	// Read a byte from the file - error if EOF
		charRead = (uchar) fgetc(infile);
		if (charRead == EOF)
		{
			return FALSE;
		}

		// See if we have a repeat tag
		if ((charRead & 0x80) && pLBMBMHeader->compression)
		{
			repCount = (uint16) (((~charRead) & 0xff) + 2);
//			repCount = charRead - 0x80;
			charRead = (uchar) fgetc(infile);
//			if (charRead == EOF)
//			{
//				return FALSE;
//			}

			while (repCount--)
			{
				*(bitmap + scanLine + scanChar) = charRead;
				++scanChar;
			}
		}
		else
		{	repCount = (uint16) ((charRead & 0xFF) + 1);

			while(repCount--)
			{
				charRead = (uchar) fgetc(infile);
				*(bitmap + scanLine + scanChar) = charRead;
				++scanChar;
			}
		}
	} while (scanChar < bytesToRead);

	scanLine += pLBMBMHeader->w;
//	pMono->Out("pLBMBMHeader->h = %d\n", pLBMBMHeader->h);
	return TRUE;
}

long motr2intl(long l);

long
motr2intl(long l)
{
	return(((l & 0xff000000L) >> 24) +
	       ((l & 0x00ff0000L) >> 8) +
	       ((l & 0x0000ff00L) << 8) +
	       ((l & 0x000000ffL) << 24));
}

int motr2inti(int n);

int
motr2inti(int n)
{
	return(((n & 0xff00) >> 8) | ((n & 0x00ff) << 8));
}

void
TileLibrary::RefreshPics(void)	// load/reload tile pics from LBM
{
	uint32		ChunkSize, ScanLine;
	uint16		loop1, loop2, PageNum, TileNum;
	char			BitMapFile[80], buffer[10];
	uchar			*pBitMap;
	FILE			*binfile;
	Tile			*pTile;
	bool			reading, fPalFound;
	TPalette		*pPal;

	TRACK_MEM("LBMHeader"); pLBMBMHeader = new LBMBMHeader;
	TRACK_MEM("BitMap"); pBitMap = new uchar[LBM_WIDE * LBM_HIGH];

	reading = TRUE;
	PageNum = 0;
	TileNum = 0;
	pMono->Out("Reading fresh tile images..\n");
	while(reading)
	{
		sprintf(BitMapFile, "%d.LBM", (atoi(h.LibName) / 20) * 20 + PageNum + 1);
		binfile = fopen(BitMapFile, "rb");
		if(!binfile)
		{	pMono->Out("Warning: Unable to open tile bitmap %s\n", BitMapFile);
			reading = FALSE;
			continue;
		}
		fPalFound = FALSE;
		pMono->Out("Processing %s..", BitMapFile);
		// read & discard LBM header
		fread(buffer, 1, 4, binfile);
		if(memcmp(buffer, "FORM", 4) && memcmp(buffer, "LIST", 4) && memcmp(buffer, "CAT ", 4))
			pMono->Out("Error: %s is not valid.\n", BitMapFile);
		else
		{
			fread(&ChunkSize, 1, 4, binfile);		// file size - ignore
			fread(buffer, 1, 4, binfile);		// sub-type

			// search LBM for BODY chunk
			do
			{
				fread(buffer, 1, 4, binfile);
				fread(&ChunkSize, 1, 4, binfile);
				ChunkSize = motr2intl(ChunkSize);
				if(ChunkSize & 1L)
					++ChunkSize;
				if(!memcmp(buffer, "BMHD", 4))
				{
					pMono->Out("Reading LBM bitmap header..\n");
					fread(pLBMBMHeader, 1, sizeof(LBMBMHeader), binfile);
					pLBMBMHeader->h = (unsigned short) motr2inti(pLBMBMHeader->h);
					pLBMBMHeader->w = (unsigned short) motr2inti(pLBMBMHeader->w);
					pMono->Out("  w=%d, h=%d, x=%d, y=%d, nPlanes=%d, compression=%d\n",
					 pLBMBMHeader->w, pLBMBMHeader->h, pLBMBMHeader->x, pLBMBMHeader->y, (int)(pLBMBMHeader->nPlanes), (int)(pLBMBMHeader->compression));
				}
				else if(!memcmp(buffer, "CMAP", 4))
				{
					pMono->Out("Reading LBM palette\n");
					fPalFound = TRUE;
//					pMono->Out("Reading palette.\n");		
					fread(h.palette, 1, 256*3, binfile);
					pPal = ADerefAs(TPalette, gPal);
					memcpy(pPal->guns, h.palette, 256*3);
					pPal->UpdateAll();
					//					memset(pPal->guns, 255, 256*3);
/*					FILE *pPalFile = fopen("palette.dmp", "at");
					fprintf(pPalFile, "-------------------\n");
					for(loop1 = 0; loop1 < 256; loop1++)
						fprintf(pPalFile, "%3d %3d %3d\n", pPal->guns[loop1].r, pPal->guns[loop1].g, pPal->guns[loop1].b);
					pPal->GetColors((Gun *)h.palette, 0, 255);
					fprintf(pPalFile, "\nnew palette\n");
					for(loop1 = 0; loop1 < 256; loop1++)
						fprintf(pPalFile, "%3d %3d %3d\n", h.palette[loop1 * 3], h.palette[loop1 * 3 + 1], h.palette[loop1 * 3 + 2]);						
					fclose(pPalFile);*/
				}
				else if(!memcmp(buffer, "BODY", 4))
				{
//					pMono->Out("Reading 400 raster lines\n");
					ScanLine = 0;
					for(loop1 = 0; loop1 < 400; loop1++)
					{
						ReadLBMLine(binfile, pBitMap, ScanLine);
					}
				} else fseek(binfile, ChunkSize, SEEK_CUR);
			} while(memcmp(buffer, "BODY", 4) && !ferror(binfile));
		}
		if(!fPalFound)
			pMono->Out("Warning: no CMAP chunk found/\n");
//		pMemMgr->HeapCheck();

//		pMono->Out("Loading images for existing tiles..\n");
		// load in tiles and their corresponding images
		for(loop1 = 0; TileNum < h.totalTiles && loop1 < 150; loop1++, TileNum++)
		{
			pTile = ADerefAs(Tile, gTile[TileNum]);
			// extract new image from bitmap
			pTile->LoadImage(pBitMap, LBM_WIDE, (uint16) (loop1 % 15), (uint16) (loop1 / 15));
		}
		pMono->Out("%d images loaded.\n", loop1);
		
		// if entire page not used
//		pMemMgr->HeapCheck();
		if(loop1 < 150)
		{
			// create new Tile for each remaining image in source bitmap.
			// EOF is when an image using color 254 in top-left pixel is found.
			pMono->Out("Creating new tiles for remaining images..\n");
			loop2 = loop1;
			do
			{
				TRACK_MEM("Tile"); pTile = new Tile;
				gTile[TileNum++] = pTile->gSelf;
				pTile->LoadImage(pBitMap, LBM_WIDE, (uint16) (loop1 % 15), (uint16) (loop1 / 15));
				loop1++;
			}
			while(pTile->h.BitMap[0] != 254 && TileNum < MAX_TILES && loop1 < 150);
			
			// if last image read was a blank image
			if(pTile->h.BitMap[0] == 254)
			{
				loop1--;
				TileNum--;
				ADelete(gTile[TileNum]);		// delete last one - image was blank
				gTile[TileNum] = NULL;
			}
  //			pMono->Out("%d new images created\n", loop1 - loop2);	
		}
		fclose(binfile);
		PageNum++;
	}
	pMono->Out("%d total images loaded\n", TileNum);
	h.totalTiles = TileNum;
//	pMemMgr->HeapCheck();
	delete pLBMBMHeader;
	delete pBitMap;
	ADerefAs(TPalette, gPal)->UpdateAll();		// update screen palette

	ParseNewTileInfo();
}

bool
TileLibrary::LoadLibrary(char LibName[])		// load an existing library
{
	int32		loop1, loop2, libNum;
	char			buffer[80];
	FILE			*libfile;
	Tile 			*pTile;
	TileGroup	*pTileGroup;
//	TPalette		*pPal;

	libNum = atoi(LibName);
	libNum = (libNum / 20) * 20;	// get base number (.TIL)

	sprintf(buffer, "%d.TIL", libNum);
 	pMono->Out("Loading tile library %s\n", buffer);

	libfile = fopen(buffer, "rb");
	if(!libfile)
	{
		pMono->Out("Error: Unable to open tile library %s\n", buffer);
		strcpy(h.LibName, LibName);
		return(FALSE);
	}

	fread(&h, 1, sizeof(TileLibraryHeader), libfile);
	strcpy(h.LibName, LibName);

//	pPal = ADerefAs(TPalette, gPal);
//	memcpy(pPal->guns, h.palette, 256*3);
// pPal->UpdateAll();

	if(h.totalTiles > MAX_TILES)
	{
		pMono->Out("Warning: # of tiles (%d) in header exceeds MAX_TILES\n", h.totalTiles);
		h.totalTiles = MAX_TILES;
	}
	if(h.totalTileGroups > MAX_TILE_GROUPS)
	{
		pMono->Out("Warning: # of tile groups (%d) in header exceeds MAX_TILE_GROUPS\n", h.totalTileGroups);
		h.totalTileGroups = MAX_TILE_GROUPS;
	}

	pMono->Out("Reading %d tile group headers..\n", h.totalTileGroups);
		
	if(h.totalTileGroups > MAX_TILE_GROUPS)
	{
		pMono->Out("Error: # of tile groups in header exceeds MAX_TILE_GROUPS\n", h.totalTileGroups);
		h.totalTileGroups = MAX_TILE_GROUPS;
	}

	for(loop1 = 0; loop1 < h.totalTileGroups; loop1++)
	{
		TRACK_MEM("TileGroup"); pTileGroup = new TileGroup;
		gTileGroup[loop1] = pTileGroup->gSelf;
		fread(&(pTileGroup->h), 1, sizeof(TileGroupHeader), libfile);
		strupr(pTileGroup->h.descr);

		// chop off end spaces
		for(loop2 = strlen(pTileGroup->h.descr) - 1; loop2 > 0 && pTileGroup->h.descr[loop2] == ' '; loop2--)
			pTileGroup->h.descr[loop2] = NULL;	// chop off trailing spaces
	}
	
	if(h.totalTileGroups == 0)
	{
		AddGroup();	// add a blank group
	}

	for(loop1 = 0; loop1 < h.totalTiles; loop1++)
	{
		// read tile info from tile lib
		TRACK_MEM("Tile");	pTile = new Tile;
		gTile[loop1] = pTile->gSelf;
		pTile->Read(libfile);
		if(pTile->h.GroupNum == FOG_TERR)
			fogTile = loop1;	// remember this one - ViewPort::Draw() will need it
		//		DrawTile(ADerefAs(Tile, gTile[loop1]), 0, (loop1 * (TILE_HEIGHT + 1)) % 360);
	}

	fclose(libfile);
	pMono->Out("%d total tiles loaded\n", loop1);
//	ADerefAs(TPalette, gPal)->UpdateAll();
//	pMemMgr->HeapCheck();
	pMono->Out("LoadLibrary();\n");
	return(TRUE);
}

void
TileLibrary::ParseNewTileInfo(void)
{
	FILE		*report;
	int32		loop1, groupNum, tileNum, loop2;
	char		buffer[256];
	TileGroup	*pTileGroup;
	Tile			*pTile;
	struct stat fileStat;
	time_t		tilTime, txtTime;

	sprintf(buffer, "%d.TIL", (atoi(h.LibName) / 20) * 20);
	stat(buffer, &fileStat);
	tilTime = fileStat.st_atime;

	sprintf(buffer, "%d.TXT", (atoi(h.LibName) / 20) * 20);
	stat(buffer, &fileStat);
	txtTime = fileStat.st_atime;

	pMono->Out("Parsing new tile info from %s\n", buffer);

	if(txtTime <= tilTime)
	{
		pMono->Out("Tile info file %s is not newer than .TIL - ignoring\n", buffer);
		return;
	}

	report = fopen(buffer, "r");
	if(!report)
		return;

	fgets(buffer, 255, report);
	fgets(buffer, 255, report);
	fgets(buffer, 255, report);

	fgets(buffer, 255, report);
	loop1 = 0;
	while(buffer[0] == '#')
	{
		groupNum = atoi(&buffer[1]);
		if(!gTileGroup[groupNum])		// if no existing tile group in this slot
		{
			TRACK_MEM("TileGroup"); pTileGroup = new TileGroup;	// make a new one
			gTileGroup[groupNum] = pTileGroup->gSelf;
			h.totalTileGroups = (uint16)AMax(h.totalTileGroups, groupNum + 1);
		}
		pTileGroup = ADerefAs(TileGroup, gTileGroup[groupNum]);
		strncpy(pTileGroup->h.descr, &buffer[5], 15);

		// chop off end spaces
		for(loop2 = strlen(pTileGroup->h.descr) - 1; loop2 > 0 && pTileGroup->h.descr[loop2] == ' '; loop2--)
			pTileGroup->h.descr[loop2] = NULL;	// chop off trailing spaces

		pTileGroup->h.moveRate = (uint16)atoi(&buffer[24]);
		pTileGroup->h.attackMod = (int16)atoi(&buffer[30]);
		pTileGroup->h.attackRangeMod = (int16)atoi(&buffer[36]);
		pTileGroup->h.defenseMod = (int16)atoi(&buffer[42]);
		pTileGroup->h.damageVal = (int16)atoi(&buffer[48]);
		pTileGroup->h.burnable = (toupper(buffer[56]) == 'Y')? TRUE: FALSE;
		pTileGroup->h.miniColor = atoi(&buffer[61]);
		pTileGroup->h.portrait = atoi(&buffer[69]);

		fgets(buffer, 255, report);
		loop1++;
	}

	fgets(buffer, 255, report);
	fgets(buffer, 255, report);

	fgets(buffer, 255, report);
	loop1 = 0;
	while(buffer[0] == '#')
	{
		tileNum = atoi(&buffer[1]);
		if(!gTile[tileNum])		// if no existing tile in this slot
		{
			TRACK_MEM("Tile"); pTile = new Tile;	// make a new one
			gTile[tileNum] = pTile->gSelf;
			h.totalTiles = (uint16)AMax(h.totalTiles, tileNum + 1);
		}
		pTile = ADerefAs(Tile, gTile[tileNum]);
		pTile->h.GroupNum = (uint16)atoi(&buffer[9]);
		pTile->h.swapTile = (uint16)atoi(&buffer[38]);
		pTile->h.animTile = (uint32)atoi(&buffer[50]);
		pTile->h.animDelay = atoi(&buffer[63]);

		fgets(buffer, 255, report);
		loop1++;
	}

	fclose(report);
}

void
TileLibrary::Report(void)
{
	FILE		*report;
	uint32	loop1;
	char		buffer[256], *str;
	TileGroup	*pTileGroup;
	Tile			*pTile;

	sprintf(buffer, "%d.TXT", (atoi(h.LibName) / 20) * 20);
	pMono->Out("Writing tile lib report %s\n", buffer);

	report = fopen(buffer, "w");
	fprintf(report, "// %s tile library content report\n", h.LibName);
	fprintf(report, "// -----------------\n// Terrain Types\n");

	for(loop1 = 0; loop1 < h.totalTileGroups; loop1++)
	{
		pTileGroup = ADerefAs(TileGroup, gTileGroup[loop1]);
		
		// convert descr to uppercase
		str = pTileGroup->h.descr;
		while(*str)
		{
			*str = (char)(toupper(*str));
			str++;
		}

		fprintf(report, "#%3d %-15s Mov%02d Att%02d Rng%02d Def%02d Dmg%02d Burn:%c Clr%03d Port%04d\n", loop1, pTileGroup->h.descr,
		 pTileGroup->h.moveRate, pTileGroup->h.attackMod, pTileGroup->h.attackRangeMod, pTileGroup->h.defenseMod, 
		 pTileGroup->h.damageVal, pTileGroup->h.burnable? 'Y': 'n', pTileGroup->h.miniColor, pTileGroup->h.portrait);
	}
	fprintf(report, "\n-----------------\nTiles\n");

	for(loop1 = 0; loop1 < h.totalTiles; loop1++)
	{
		pTile = ADerefAs(Tile, gTile[loop1]);

		// sanity checking
		if(pTile->h.GroupNum > h.totalTileGroups || pTile->h.GroupNum < 0)
		{
			pMono->Out("Error! tile %02d probably corrupt.\n", loop1);
			pTileGroup = ADerefAs(TileGroup, gTileGroup[0]);
		}
		else
			pTileGroup = ADerefAs(TileGroup, gTileGroup[pTile->h.GroupNum]);

		sprintf(buffer, "#%3d type%03d(%-15s) swapTile%03d animTile%03d animDelay%02d\n", loop1, pTile->h.GroupNum, pTileGroup->h.descr, pTile->h.swapTile,
			pTile->h.animTile, pTile->h.animDelay);
		fprintf(report, buffer);
	}
	fprintf(report, "-----\nEnd of file\n");
	fclose(report);
}

bool TileLibrary::SaveLibrary(char fileName[])
{	
	sprintf(h.LibName, "%d", (atoi(fileName) / 20) * 20);
	return(SaveLibrary());
}

uint16
TileLibrary::AddGroup(void)
{
	TileGroup	*pGroup;
	uint16		GroupNum;

	GroupNum = h.totalTileGroups++;
	TRACK_MEM("TileGroup");	pGroup = new TileGroup;
	gTileGroup[GroupNum] = pGroup->gSelf;
	return(GroupNum);
}

bool TileLibrary::SaveLibrary(void)		// save tile library, overwriting old
{
	int			loop1;
	TileGroup	*pTileGroup;
	Tile			*pTile;
	FILE			*libfile;
	char			string1[40];

	sprintf(string1, "%d.TIL", (atoi(h.LibName) / 20) * 20);
	pMono->Out("Saving tile library %s\n", string1);

	Report();

	libfile = fopen(string1, "wb");
	if(!libfile)
	{ pMono->Out("Error: Unable to save tile library to %s\n", string1);
	  return(FALSE);
	}

	fwrite(&h, 1, sizeof(TileLibraryHeader), libfile);
	pMono->Out("Writing %d tile group headers..\n", h.totalTileGroups);

	for(loop1 = 0; loop1 < h.totalTileGroups; loop1++)
	{
		pTileGroup = ADerefAs(TileGroup, gTileGroup[loop1]);
		pTileGroup->Write(libfile);
	}

	pMono->Out("Writing %d tiles..\n", h.totalTiles);
	for(loop1 = 0; loop1 < h.totalTiles; loop1++)
	{
		pTile = ADerefAs(Tile, gTile[loop1]);
		pTile->Write(libfile);
	}

	fclose(libfile);
	return(TRUE);
}

// note: Format of tile library (*.TIL) file as follows:
//		library header
//		tile group headers
//		tiles headers (includes bitmap)




