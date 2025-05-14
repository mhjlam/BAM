//
// PALETTE.HPP
//
// November, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Palette class definition.
//
//----[]-------------------------------------------------------------


#ifndef	Palette_hpp
#define	Palette_hpp


#include "types.hpp"
#include "scrimage.hpp"


// pre-defined color index
#ifdef OS_MAC
	#define	CI_WHITE		0
	#define	CI_BLACK		255
#else
	#define	CI_WHITE		255
	#define	CI_BLACK		0
#endif 

#define	CI_SKIP		254

// #pragma pack(1)

struct Gun
{
	uchar	r;		// red	
	uchar	g;		// green	
	uchar	b;		// blue
};

// #pragma pack()


//---------[ TPalette ]--------------------------------------------

class TPalette : public Utilizer
{
	public:
							TPalette();
		virtual			~TPalette();

		void				Setup(grip gCntxt);

		void				SetColor(int index, uchar r, uchar g, uchar b);
		void				Load(uint palNum);
		void				LoadPartial(uint loadPalNum, uint loadPalIndex, uint penCount, uint destPalIndex);

		void				UpdateAll();
		void				UpdateColor(int index);
		void				UpdateColors(uint startGun, uint endGun);
		void				UpdateColors(Gun newGuns[], uint startGun, uint endGun);

		void				GetColors(Gun newGuns[], uint startGun, uint endGun);

		void				FadeToBlack(int faderSteps = 64);
		void				FadeUp(int faderSteps = 64);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char	tPaletteDataStart;
		Gun	guns[256];
		char	tPaletteDataEnd;
};


//---------[ PalUtility ]--------------------------------------------

// Palette Utilities should all be derived from this class

class PalUtility : public Utility
{
	public:
		grip		gToCue;

		Gun		guns[256];

		PalUtility();

		void		CheckRange(uchar start, uchar end);
};


//---------[ PalCycler ]--------------------------------------------

// Palette Cycler Types
enum
{
	PCYC_NONE = 0,
	PCYC_FORWARD,
	PCYC_REVERSE,
	PCYC_FWD_OSC,
	PCYC_REV_OSC,
	PCYC_MAX					// this MUST be the last item in this enum
};

// Palette Cycler Directions
// NOTE: These are setup so that they can be toggled with an XOR
#define	PDIR_FORWARD	1
#define	PDIR_REVERSE	0

class PalCycler : public PalUtility
{
	public:
		
		uchar		cycleType;
		uint16	cycleTimes;

		uint16	curCount;
		uint16	totalCycles;
		char		curDirection;

		uchar		startGun;
		uchar		endGun; 
		uint		gunRange;

		PalCycler(uchar cycType, uchar gunStart, uchar gunEnd,
					grip gSourcePal,
					uint32 cycRate, uint16 cycTimes,
					grip gCueWhenDone = NULL);
		~PalCycler();

		void	RotatePalette(char cycleDirection);

		virtual void	Setup(void* pOwner);
		virtual void	Service();
};


//---------[ PalFader ]--------------------------------------------

class PalFader : public PalUtility
{
	public:

		Gun		srcGuns[256];

		Gun		targetGun;
		bool		useTargetGun;

		uint16	faderSteps;
		uint16	curStep;

		uchar		startGun;
		uchar		endGun; 

		PalFader(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					grip gCueWhenDone = NULL);

		PalFader(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					uchar red, uchar green, uchar blue,
					grip gCueWhenDone = NULL);

		virtual void	Setup(void* pOwner);
		virtual void	Service();


	protected:

		void	Setup(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					grip gCueWhenDone = NULL);

		void	Setup(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					uchar red, uchar green, uchar blue,
					grip gCueWhenDone = NULL);
};



#endif

