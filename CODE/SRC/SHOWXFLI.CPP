#include <stdlib.h>
#include <conio.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <mem.h>
#include <string.h>

#include "mono.hpp"
#include "flic.hpp"
#include "types.hpp"
#include "modex.hpp"

typedef signed char SChar;
typedef unsigned short ushort;
extern Mono	*pMono;

#pragma SC align 1
FILE *flicfile;
FlicHead flic;
ChunkHead chunk;
FrameHead frame;
#pragma SC align
SChar *pOffsets[400];

#define	SC_INDEX			0x03c4	// sequence controller index register
#define	GC_INDEX			0x03ce	// graphics controller index register
#define	CRTC_INDEX		0x03d4	// CRT controller index register
#define	MAP_MASK			2			// map mask register index in SC
#define	SET_SCREEN_MODE				0x00
#define	GET_SCREEN_MODE				0x0F
#define	VIDEO_BIOS						0x10

void decode_color (SChar* const pData, int chsize, int ncolors);
void set_palette (int start_index, void *pData, int nColors, int doShift);
extern void SetXMode(void);
void decode_byte_run(SChar* const pData, int chsize);
int height = 400, width = 320;
void RepeatByte (int x, int y, char color, int count);
void BlitData(int x, int y, void* pSrc, int count);
void decode_delta_flc(SChar* const pData, int chsize);
void SetPixel(int x, int y, uchar color);
void RepeatWord (int x, int y, short color, int count);

union wpt
{
	short *w;
 	uchar *ub;
 	SChar  *b;
 	short *p2;	// BUGBUG why do we have this data as well?
 	wpt () {}
 	operator char * ()
 	{
 		return ((char *) b);
 	}
 	wpt (void *pData)
 	{
 		b = (SChar*) pData;
 	}
 	inline short GetShort ()
 	{
		short retv = *w;
	 	w++;
 		return (retv);
	}
	inline uchar GetUChar ()
	{
		uchar retv = *ub;
		ub++;
		return (retv);
	}
	inline SChar GetChar ()
	{
		char retv = *b;
		b++;
		return (retv);
	}
};

void main(int argc, char *argv[])
{
	int frameloop, chunkloop, loop1;
	SChar *chunkdata, *pScreen;
	uchar saveMode;
	bool	fStepFrames;
	char	string1[40];

	pMono = new Mono;
	pMono->Out("SHOWXFLI\n");

	pScreen = (SChar *)0xa0000;
	for (loop1 = 0; loop1 < 400; loop1++)
		pOffsets[loop1] = pScreen + loop1 * 80;

	puts("SHOWXFLI - xmode FLI/FLC viewer.\n");
	if(argc < 2)
	{	puts("Format: SHOWXFLI <FLIC.FLC> [/STEP]\n");
		delete pMono;
		return;
	}
	flicfile = fopen(argv[1], "rb");
	if(!flicfile)
	{	puts("Error: unable to open "); puts(argv[1]); puts("\n");
		return;
	}

	for(loop1 = 0; loop1 <= argc; loop1++)
		pMono->Out("%d] %s\n", loop1, argv[loop1]);

	fStepFrames = FALSE;
	if(argc == 3)
	{
		strcpy(string1, argv[2]);
		if(!strcmpi(string1, "/STEP"))
			fStepFrames = TRUE;
	}

	#pragma aux SAVE_CURRENT_MODE =	\
		"mov	ah, 0Fh"	\
		"int	10h"				\
		"mov	saveMode, al";
	SAVE_CURRENT_MODE();
	SetXMode();		// set X mode

	fread(&flic, 1, sizeof(flic), flicfile);
	pMono->Out("sizeof(flic)==%d\n", (int)sizeof(flic));
	pMono->Out("Size==%lu  type==%x  frames==%d\n", flic.size, flic.type, flic.frames);
	pMono->Out("oframe1==%lu  oframe2==%lu\n", flic.oframe1, flic.oframe2);

	fseek(flicfile, flic.oframe1, SEEK_SET);
	
	for(frameloop = 0; frameloop < flic.frames; frameloop++)
	{
		pMono->Out("Frame %d - ", frameloop);
		fread(&frame, 1, sizeof(frame), flicfile);
		pMono->Out("  size==%lu type=%x chunks==%d\n", frame.size, frame.type, frame.chunks);
		if(frame.type != FRAME_TYPE)
			pMono->Out("Invalid frame type\n");
		else for(chunkloop = 0; chunkloop < frame.chunks; chunkloop++)
		{
			pMono->Out("  chunk #%d - ", chunkloop);
			fread(&chunk, 1, sizeof(chunk), flicfile);
			pMono->Out("chunk size==%lu  type==%d  ", (long)chunk.size, (int)chunk.type);
			chunkdata = new SChar[chunk.size - sizeof(chunk)];
			fread((void *)chunkdata, 1, chunk.size - sizeof(chunk), flicfile);
			switch(chunk.type)
			{	case COLOR_256:
					pMono->Out("COLOR_256");
					decode_color(chunkdata, (int)chunk.size - sizeof(chunk), (int)256);
					break;
				case COLOR_64:
					pMono->Out("COLOR_64");
					decode_color(chunkdata, (int)chunk.size - sizeof(chunk), (int)64);
					break;
				case DELTA_FLC:
					pMono->Out("DELTA_FLC");
					decode_delta_flc(chunkdata, (int)chunk.size - sizeof(chunk));
					break;
				case DELTA_FLI:
					pMono->Out("DELTA_FLI");
					break;
				case BLACK:
					pMono->Out("BLACK");
					break;
				case BYTE_RUN:
					pMono->Out("BYTE_RUN");
           		decode_byte_run(chunkdata, (int)chunk.size - sizeof(chunk));
					break;
				case LITERAL:
					pMono->Out("LITERAL");
					break;
				case PSTAMP:
					pMono->Out("PSTAMP");
					break;
			}			
			pMono->Out("\n");
			delete chunkdata;
		}
		if(frameloop == 0 || frameloop == flic.frames - 1 || fStepFrames)
		  getch();
	}
	fclose(flicfile);

	#pragma aux RESTORE_VIDEO_MODE =	\
		"mov	al, saveMode"	\
		"mov	ah, 00h"	\
		"int	10h";
	RESTORE_VIDEO_MODE();
	delete pMono;
}

void decode_delta_flc(SChar* const pData, int chsize)
{
  int yorg = 0;
  int xorg = 0;
  int width = 640;
  int lastx = xorg + width - 1;
  wpt Data (pData);

  chsize = chsize;	// I hate compiler warnings.

  int lp_count = Data.GetShort ();
//	pMono->Out("lp_count=%d\n", lp_count);

	int y = yorg;
	while (lp_count)
	{
//		getch();
		int opcount = Data.GetShort ();
		if (opcount >= 0)
		{
			int x = xorg;
			while (opcount--)
			{
				x += Data.GetUChar ();
				int psize = Data.GetChar ();
				if ((psize += psize) >= 0)
				{
//					pMono->Out("delta:image @x%d,y%d size%d\n", (int)x, (int)y, (int)psize);
					BlitData(x, y, Data.b, psize);
					x += psize;
					Data.b += psize;
				} else
				{
        			psize = -psize;
//					pMono->Out("delta:repword @x%d,y%d size%d\n", (int)x, (int)y, (int)psize);
        			RepeatWord (x, y, Data.GetShort (), psize);
        			x += psize;
				}
//				if(opcount)
//					getch();
			}
			y++;
			lp_count--;
			continue;
		}
		if (((ushort)opcount) & 0x400)
		{
//			pMono->Out("Opcount & 0x400\n");
			y -= opcount;
			continue;
		}
//		pMono->Out("lastx=%d, y=%d, opcount & 0xFF = %d\n", lastx, y, opcount & 0xff);
		SetPixel (lastx, y, opcount & 0xff);
	}
}

void
SetPixel(int x, int y, uchar color)
{
	uint16 planeNum = x & 3;

	#pragma aux SET_PIXEL_ASM =	\
		"mov	cx, planeNum"	\
		"mov	ah, 1"			\
		"shl	ah,cl"			\
		"mov	al,02h"			\
		"mov	dx,03C4h"		\
		"out	dx,ax";
	SET_PIXEL_ASM();
	*(pOffsets[y] + x / 4) = color;
}

void
RepeatWord (int x, int y, short color, int count)
{
	uint16 loopx;
//		assert (x >= 0 && y >= 0);
//		assert ((x + count * 2) <= width && y < height);
//	CLUDGEW;
	for(loopx = 0; loopx < count; loopx += 2)
	{	SetPixel(x + loopx, y, (char)(color & 0xFF));
		SetPixel(x + loopx + 1, y, (char)(color >> 8));
	}
//	short* pDest = (short *) (pOffsets[y] + x);
//	while (count)
//	{
//		pDest[count - 1] = color;
//		count--;
//	}
}

void
decode_byte_run(SChar* const pData, int chsize)
{
	uint16 x, y = 0, width = 320, height = 400, end = 0 + width;
	uchar *pByteData = (uchar *)pData, color;
	SChar byteCount = 0;

	chsize = chsize;	// quiet compiler warning

	while(y < height)
	{
		pByteData++;		// eat extra byte?
		x = 0;
		byteCount = 0;
//		pMono->Out("Line %2d: ", y);
		while((x += byteCount) < width)
		{
			byteCount = *(pByteData++);		// get length of this byte run
			if(byteCount >= 0)					// if repeat sequence
			{
  				color = *(pByteData++);
//				pMono->Out("RB(x%d,col%d,Cnt%d) ", (int)x, color, (int)byteCount + 1);
				RepeatByte (x, y, color, byteCount);
			} else								// else assume bitmap data
			{
				byteCount = -byteCount;
//				pMono->Out("I(x%d,Cnt%d) ", (int)x, (int)byteCount);
				BlitData(x, y, pByteData, byteCount);
				pByteData += byteCount;
			}
		}
//		pMono->Out("\n");
//		getch();
		y++;
	}
}

#define CLUDGE						\
{										\
	if (y < 0)						\
		return;						\
	if (y >= height)				\
		return;						\
	if (x >= width)				\
		return;						\
	if (x + count >= width)		\
	{									\
		count = width - x - 1;	\
	}									\
	if (count <= 0)				\
		return;						\
	}

#define CLUDGEW	\
{						\
	count <<= 1;	\
	CLUDGE;			\
	count >>= 1;	\
}

void
RepeatByte (int x, int y, char color, int count)
{
	uint16 planeNum, vidCol;

//		assert (x >= 0 && y >= 0);
//		assert (x + count <= width && y < height);
//	CLUDGE;
	if(x >= 320)
	  return;
	if(y >= 400)
	  return;
	if(x + count >= 320)
	  count = 320 - x;

	for(planeNum = x & 3, vidCol = x / 4; count > 0; count--, planeNum = (planeNum + 1) % 4)
	{	// set approp video bank, for X mode

		#pragma aux	SET_PLANE_NUM =	\
			"mov	cx, planeNum"	\
			"mov	ah, 1"			\
			"shl	ah,cl"			\
			"mov	al,02h"			\
			"mov	dx,03C4h"		\
			"out	dx,ax";
		SET_PLANE_NUM();
		*(pOffsets[y] + vidCol) = color;

		if(planeNum == 3)
	  		vidCol++;
	}

//	memset (pOffsets[y] + x, color, count);
}

void
BlitData(int x, int y, void* pSrc, int count)
{
	uint16 planeNum, vidCol;
	uchar	*pImage = (uchar *)pSrc;

	//		assert (x >= 0 && y >= 0);
//		assert (x + count <= width && y < height);
//		CLUDGE;
	if(x >= 320)
	  return;
	if(y >= 400)
	  return;
	if(x + count >= 320)
	  count = 320 - x;

	for(planeNum = x & 3, vidCol = x / 4; count > 0; count--, planeNum = (planeNum + 1) % 4, pImage++)
	{	// set approp video bank, for X mode

		#pragma aux BLIT_SET_PLANE =	\
			"mov	cx, planeNum"	\
			"mov	ah, 1"			\
			"shl	ah,cl"			\
			"mov	al,02h"			\
			"mov	dx,03C4h"		\
			"out	dx,ax";
		BLIT_SET_PLANE();

		*(pOffsets[y] + vidCol) = *pImage;

		if(planeNum == 3)
	  		vidCol++;
	}
//		memcpy (pOffsets[y] + x, pSrc, count);
}

void 
decode_color (SChar* const pData, int chsize, int ncolors)
{
	wpt pDat;
	int index = 0;
	int shift = ncolors == 256 ? 2 : 0;

//	pMono->Out("\ndecode_color(x, %d, %d), ", chsize, ncolors);
//	for(loop1 = 0; loop1 < 16; loop1++)
//	{ pMono->Out("%02x ", *(pData + loop1));
//	}
//	pMono->Out("\n");
	pDat.b = pData;
	int packets = pDat.GetShort();
	pMono->Out("  Packets==%d\n", packets);
	for(int i = 0; i < packets; i++)
	{
		index += pDat.GetUChar();
		int count = pDat.GetUChar();
		if (!count)
		{
			count = 256;
		}
//		pMono->Out("pData=%x, chsize=%d, pDat.b=%x, count=%d, index=%d\n", pData, chsize, pDat.b, count, index);
		assert (pData + chsize >= pDat.b + 3 * count);
		set_palette (index, pDat.b, count, shift);
		pDat.b += 3 * count;
	}
}


// EAX index of first palette entry
// EBX number of colors to change
// EDX pointer to palette data
// ECX is whether to shift or not
void
set_palette (int start_index, void *pData, int nColors, int doShift)
{
	int local_start_index = start_index;

	#pragma aux SET_PALETTE_ASM =	\
		"mov	eax, local_start_index"	\
		"mov	ebx, nColors"	\
		"mov	edx, pData"		\
		"mov	ecx, doShift"	\
		"mov 	esi, edx"		\
		"xchg	ebx, ecx"		\
		"cli"						\
		"lea	ecx, [ecx*2+ecx]"	\
		"mov	dx, 3C8H"		\
		"out	dx, al"			\
		"inc	dx"				\
	"sp010:"						\
		"lodsb"					\
		"and	ebx, ebx"		\
		"je		no_shift"	\
		"shr	eax, 2"			\
	"no_shift:"					\
		"out	dx, al"			\
		"loop	sp010"			\
		"sti";
	SET_PALETTE_ASM();
}



