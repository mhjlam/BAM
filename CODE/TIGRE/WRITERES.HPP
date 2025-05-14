//
// WRITERES.HPP
//
// February 8, 1994
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Header for resource creation.
//
//----[]-------------------------------------------------------------


#ifndef	writeres_hpp
#define	writeres_hpp

#include "tigre.hpp"


// Compression types
#define	CMP_NONE			0x00	// No compression
#define	CMP_BOGUS		0x01	// Some old resources had this when they
										// shouldn't have.  Invalid now to avoid 
										// problems. (used to be TRLE)
#define	CMP_T12			0x02	// LZT 12-bit (whole file - header)
#define	CMP_TRLE			0x04	// Tagged RLE (internal only)
#define	CMP_SRLE			0x08	// Skip RLE (internal only)
#define	CMP_2_WIDE		0x10	// Used on Mac/BAM only, if set, bitmap is
										// double width of celheader coords

// Resource attributes
#define	RA_LOCKED		0x01	// Resource is locked - can't be purged
#define	RA_MOVEABLE		0x02	// Resource can't move in memory
#define	RA_SAVEABLE		0x04	// Resource will be saved in save game
#define	RA_COMPRESSED	0x08	// Resource is compressed in memory
#define	RA_DYNAMIC		0x10	// Resource was created dynamically, not loaded
#define	RA_HEADERLESS	0x20	// Resource has no header
#define	RA_SYS_GRIP		0x40	// Resource uses system grips

// Resource generation
#define	GENERATION		0xfefe

#define	rhSize	sizeof(ResHeader)
#define	chSize	sizeof(CelHeader)
#define	fhSize	sizeof(FontHeader)
#define	fchSize	sizeof(FontCharHeader)


#ifdef 	OS_MAC
	#pragma pack(4)
#endif

#define	RH_DATA_MAX	3

// Use this to create the file header.  'data' is 4 words that are
// resource-specific.
struct ResHeader
{
	uint16 	compType;				// Compression type
	uint32	uSize;					// Uncompressed resource size (data + cel headers only)
	uint32	size;						// Compressed resource size
	uint16	type;						// Resource type
	uint16	numHeaders;				// Number of sub-headers
	uint16	sizeOfHeaders;			// Size of all sub-headers in bytes
	uint16	data[RH_DATA_MAX];	// Resource type-specific data
	uint16	genID;					// Generation ID of resource
};


// This structure defines a single cel piece of a cel, anim, or pic
// resource.  One (cel) or more (anim, pic) will follow the header
// and precede the data in the file.
struct CelHeader
{
//	coord		x;				// location of cel on screen
//	coord		y;				//
	coord		orgX;			// origin of cel (relative to cel's top-left)
	coord		orgY;			//
	uint16	width;		// width of cel in pixels
	uint16	height;		// height of cel in pixels
	uint16	priority;	// priority of cel
	uint32	offset;		// offset into resource of cel data
};


// Pass an array of this structure to the Create functions.
struct CelArrayElement
{
	CelHeader	celHeader;
	uchar*		celData;
};


// This structure defines a .MFN (Mac Font) file.
struct FontHeader
{
	uint16	widest;		// widest character width in font
	uint16	height;		// height of character (point)
	uint16	ascent;		// pixels above baseline
	uint16	descent;		// pixels below baseline
	uint16	leading;		// pixels below baseline + descent
	uint16	firstChar;	// ASCII of first character
	uint16	lastChar;	// ASCII of last character
	uint16	bufWidth;	// width of character
};


// This structure defines a single character in a .MFN file.
struct FontCharHeader
{
	uint16	charWide;	// width in pixels of this character
	uint32	offset;		// offset into resource of char data
};


// Pass an array of this structure to the CreateFont function.
struct FontArrayElement
{
	FontCharHeader	charHeader;
	uchar*			charData;
};


#ifdef 	OS_MAC
	#pragma pack()
#endif


//----[ Function Prototypes ]----------------------------------------

bool	CreateGeneric(res_t type, uint number, size_t size, void* pData, uint16 pWordData[], bool compress);
bool	CreateCel(uint number, CelArrayElement* pCelData, bool compress);
bool	CreateAnim(uint number, uint numCels, CelArrayElement* pCelDataArray[], uint16 compress);
bool	CreatePic(uint number, uint numCels, CelArrayElement* pCelDataArray[], bool compress);
bool	CreateFont(uint number, uint numChars, FontHeader fHdr, FontArrayElement* pFontDataArray[], bool compress);
void	CreateResFileName(res_t type, uint number, char* name);
FILE*	OpenResFile(char* filename);



//----[ global variables ]----------------------------------------

extern bool	fIsTextMode;


#endif


/*

----[ Mac Fonts ]----------------------------------------------------


Mac fonts contain the following character structure:

		ÚÄÄÄÄÄÄ¿¿			 ¿
		³ þþþ  ³Ã ascent	 ³
		³ þ  þ ³³			 ³
		³ þ  þ ³Ù			 ³
		³ þþþ  ³Ä baseline Ã height
		³ þ    ³¿			 ³
		³ þ    ³Á descent	 ³
		³      ³Ä leading  ³
		ÀÄÄÄÄÄÄÙ				 Ù

*/

