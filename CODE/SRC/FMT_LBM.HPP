//
// FMT_LBM.HPP
//
// Modular File Format Conversion Tool
// DPaint LBM Format
//
// (C) Copyright 1994, Intellisoft
//
//	3/17/95	Kim Bowdish
// 9/22/95  Kevin Sherrill -modified to work with snap
//
//---[]--------------------------------------------------------------

#ifndef	fmt_lbm_hpp
#define	fmt_lbm_hpp

#include "tigre.hpp"

//		LBM FILE STRUCTURE
//
//LBM  Header 		40 Bytes total
//CMAP Header			8 Bytes (4 char id "CMAP", 4 long size)
//CMAP(chunk)			768 Bytes total (256 * 3) palette
//DPPS Header			8 Bytes (4 char id "DPPS", 4 long size)
//DPPS(chunk)			110 Bytes total (fixed size)
//
//		(there are 16 CRNG's)
//CRNG Header			8 Bytes (4 char id "CRNG", 4 long size)
//CRNG(chunk)			8 Bytes total
//TINY Header			8 Bytes (4 char id "TINY", 4 long size)
//TINY(chunk)			RLE'd data of size from TINY Header
//						NOTE: Be sure that the data ends on an even boundry.
//
//BODY Header			8 Bytes (4 char id "BODY", 4 long size)
//BODY(chunk)			RLE'd data of size from BODY Header
//						NOTE: Usually the last chunk in the file.
//
//
//NOTE: The DPPS, 16 CRNG's, and TINY chunks are needed in the file.  DPPS
//	and 16 CRNG's are set to all zeros.  The TINY chunk is created by
//	catching every 4th pixel from the BODY data.  Both TINY and BODY chunks
//	are RLE compressed.
//ALSO:
//	All the long values are in MACINTOSH (Flipped) Format.  We must use
//	the two functions "motr2intl" and "motr2inti" to flip them from PC
//	format to the MACINTOSH format.
//
//

//	 RLE 
//
//				 DUMP			  		RUN
//decimal		1 - 128			  128 - 2	decimal
//code
//Range ->	  00 - 7F 	 80  	  81 - FF	hex
//						  	 ^^
//						   ERROR
//
//COUNT ->  = CODE + 1			= ~CODE + 2
//
//CODE  ->  = COUNT - 1		= ~(COUNT - 2)
//
//

typedef	unsigned long	ulong;
typedef	unsigned char	color;

//typedef	unsigned char	uchar;
//typedef	unsigned int	uint;
//typedef	unsigned short	uint16;
//typedef	short				int16;
//typedef	uint				uint32;
//typedef	uint				coord;

//enum bool
//{
//	FALSE,
//	TRUE
//};

enum
{
	FT_INVALID,
	FT_SC_GRAPHIC,		// Single cel graphic
	FT_MC_GRAPHIC,		// Multi-cel graphic
	FT_SOUND				// Sound
};

enum
{
	EX_DONT_CARE,		// No extra conversion required
	EX_HAVE_SPECIAL,	// Source has special (palette or origin cel)
	EX_NEED_SPECIAL	// Special (palette or origin cel) must be created for Target
};

//#define	MAX_FORMATS		12		// Maximum number of formats available
//#define  MAX_NUM_TYPES	3

#define	UDATA_COLUMNS			320
#define  UDATA_ROWS				400

#define  GENERATION				0xfefe

#define	ORIGIN_CLR			 	0xFB	// Color of origin point markers
#define	BORDER_CLR			 	0xFC	// Color of "red boxes"
#define 	SKIP_COLOR			 	0xFE
#define 	MAX_COLORS			 	256
#define 	TIGRE_PALETTE_SIZE  	768

	// for writing the red boxes around the artwork.
#define	COLUMN_SPACING			4
#define  ROW_SPACING				6

#define  AFMin(a,b)			  	(uint16)((a)<(b)?(a):(b))
#define  AFMax(a,b)				(uint16)((a)>(b)?(a):(b))

#define TINY_BUF_SIZE				(80 * 100)

#define TINY_COLUMNS				80
#define TINY_ROWS					100

#define CHUNK_SIZE				(sizeof(Chunk))
#define LBMHEADER_SIZE			(sizeof(LBMHeader))
#define CMAP_SIZE					776		// includes header
#define DPPS_SIZE					118		// includes header
#define CRNG_SIZE					256		// includes headers
#define GENERIC_SIZE				DPPS_SIZE + CRNG_SIZE
#define TINY_SIZE_HDR			4

#define L_UDATA_ROWS				400
#define L_UDATA_COLUMNS			320

#define N_BYTES_IN_LBM_BITMAP (320 * 400)

#define ON_ODD_BOUNDRY(x)		x % 2			// TRUE(1)means odd, FALSE(0)even.

long motr2intl(long l);
int motr2inti(int n);

struct LBMHeader
{
	char		id[4];	 			// "FORM"
	ulong		fileLength;
	char		fileType[8];		// "PBM BMHD"
	ulong		hdrLength;			// 0x14 in PC format
	uint16 	width;				// 320
	uint16	length;				// 400
	uint16 	xOffset;				// 0 
	uint16	yOffset;				// 0
	uchar 		nPlanes;				// 8
	uchar 		masking;				// 0
	uchar 		compression;		// 1
	uchar 		pad1;	 				// not used.
	uint16 	transparentColor;	// 00 fe
	uchar 		xAspect;				// 5
	uchar		yAspect;				// 6
	int16 	pageW;				
	int16		pageH;				
};										// size 40 BYTES

	// "DPPS", "CMAP", "CRNG", "TINY"
struct Chunk
{
	char	id[4];
	ulong	size;
};										// size 8 BYTES

struct LBMPalette
{
	uchar r;
	uchar g;
	uchar b;
};

	// modes for RLE finding a dump or run block
enum mode_t
{
	MODE_DONE,	// signals end of block
	MODE_DUMP,	// Block will be a short dump or long dump
	MODE_RUN		// Block will be a short run or long run
};

struct Base
{
	int		numColors;		// 4, 16, 256, et al
	uint16	numUnits;		// Number of sub-units (cels, characters, chunks)
	bool		trueColor;		// Is this file 24-bit color?
	uint		totalSize;		// Total size of all the data
	uint		sourceFmtType;	// Source format type
	uint		targetFmtType;	// Target format type
	uint		pal;				// Palette conversion flag
	uint		origin;			// Origin conversion flag
};

struct UnitHeader
{
	uint		orgX;		// Origin point of cel (relative to upper-left)
	uint		orgY;		//
	uint16	width;
	uint16	height;
	uint		size;
};

class FormatLBM
{
	public:
		FormatLBM();

		LBMHeader	lbmHeader;
		LBMPalette	lbmPal[256];
		FILE*			fLBM;

		Base			base;					// Base (raw) format
		UnitHeader	sHdr;
		uchar			sData[N_BYTES_IN_LBM_BITMAP];
		uchar			curFramebm[N_BYTES_IN_LBM_BITMAP];
		uchar			tinyBuffer[TINY_BUF_SIZE];

		uchar*		pCurSrc;
												// for the CompareBlock stuff
		int			blkCount;
		uchar			curPixel;
		uchar			lastPixel;
		mode_t		mode;
		mode_t		lastMode;
		int			offset;				// into source buffer

		int			fileSize;			// of entire file
		int			width;				// of lbmHeader
		int			height;				// of lbmHeader
		int			tinyBlockSize;
		uchar*		redBuffer;	  	  	// for putting in the red boxes(tigre format)

		bool	Write(char* filename);
		bool	Read(char* filename);
		bool	WriteHeader();
		int16 ReadHeader();
		int	ReadData(uchar* buff);
		int	DoRLE(uchar* buff, int size, int offset);
		int	UncompressRLE(uchar* buff, int size);
		int	CompareBlock(int size);
		bool	ComparePixel();
		void  FillInTinyBuffer();
};

#endif
