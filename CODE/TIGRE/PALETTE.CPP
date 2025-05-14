//
// PALETTE.CPP
//
// January 22, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Palette class. 
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "palette.hpp"
#include "savebase.hpp"



// Constructor - set properties
TPalette::TPalette()
{
	// fill (most of) palette with grey
	memset(guns, 0x77, sizeof(guns));

	// loop missing ci 253,254
	SetColor(253, 255, 255, 0);
	SetColor(254, 0, 255, 255);

	// set pre-defined constant colors
	SetColor(CI_WHITE,255,255,255);
	SetColor(CI_BLACK,0,0,0);
}


TPalette::~TPalette()
{
}

bool
TPalette::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&tPaletteDataStart, 1, (int)&tPaletteDataEnd -
				(int)&tPaletteDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&tPaletteDataStart, 1, (int)&tPaletteDataEnd -
				(int)&tPaletteDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}


//-------------------------------------------------------------
//	Setup() adds a dynamically created utility to list.

void
TPalette::Setup(grip gCntxt)
{
	// this will set the context and check to see if a context
	// is already set
	SetContext(gCntxt);
	// this will set the context and add the object
	ADerefAs(Context, gCntxt)->AddObject(gSelf, CT_SERV);
}


// Change gun table, but don't update display yet
//
void
TPalette::SetColor(int index, uchar r, uchar g, uchar b)
{
	guns[index].r = r;
	guns[index].g = g;
	guns[index].b = b;
}


// Load the palette from a .PAL file.
//
void
TPalette::Load(uint palNumber)
{
	uchar* pResData = AGetResData(ALoad(RES_PAL, palNumber));
	memcpy(guns, pResData, 256*sizeof(Gun));
}


//================
// Load a Partial Part of a Palette Resource into this Palette.
// The following variables are passed:
//
// loadPalNum		- Palette Resource Number to Load Partial From
// loadPalIndex	- Start loading pens starting at this pen number
//	penCount			- The number of pens to load from the resource. 
// destPalIndex	- Copy from the resource pens to this palette's pen
//						  array starting at this index.  This allows you to
//						  load a partial palette to different indexes than
//						  the loaded resource, if you so choose.
//
void
TPalette::LoadPartial(uint loadPalNum, uint loadPalIndex, uint penCount, uint destPalIndex)
{
	uchar* pResData = AGetResData(ALoad(RES_PAL, loadPalNum));
	memcpy(((uchar*) guns) + (destPalIndex * sizeof(Gun)), pResData + (loadPalIndex * sizeof(Gun)), penCount * sizeof(Gun));
}


void
TPalette::UpdateAll()
{
	UpdateColors(0, 255);
}


// Update one color to display.
void
TPalette::UpdateColor(int index)
{
	UpdateColors(index, index);
}


// Change palette by writing to VGA registers. This is more efficient
// than using the bios, because we can stay in protected mode.
//
void
TPalette::UpdateColors(uint startGun, uint endGun)
{
	UpdateColors(guns, startGun, endGun);
}


// Change palette by writing to VGA registers. This is more efficient
// than using the bios, because we can stay in protected mode.
//
void
TPalette::UpdateColors(Gun newGuns[], uint startGun, uint endGun)
{
	// update the VGA pens for all the colors from startGun to and
	// including endGun

	if (endGun < startGun)
	{
		APanic("endGun less than startGun");
	}

	newGuns += startGun;

	ASetPalette(newGuns, startGun, endGun);
}


// Get the palette by reading the VGA registers. This is more efficient
// than using the bios, because we can stay in protected mode.
//
void
TPalette::GetColors(Gun newGuns[], uint startGun, uint endGun)
{
	// Read the VGA pens for all the colors from startGun to and
	// including endGun

	if (endGun < startGun)
	{
		APanic("endGun less than startGun");
	}

	newGuns += startGun;

	AGetPalette(newGuns, startGun, endGun);
}



/*****************************
void
WaitForVBL ()
{
	#ifdef OS_DOS
	uchar	fVR = FALSE;

	while (!(fVR & 0x08))
	{
		#pragma aux WAIT_FOR_VBL =	\
			"mov	dx,0x3da"			\
			"in	al, dx"				\
			"mov	fVR,al";
	}
	#endif
}


void
delay	(clock_t clocksDelay)
{
	clock_t	clockStart = clock();

	while (clockStart + clocksDelay > clock())
	{
		int	i = 12;
		i *= 37;
		i /= 13;
	}
}
**********************************/



//	immediate, synchronous fade to black
//
void
TPalette::FadeToBlack(int faderSteps)
{
	int		 	i;
	int		 	iStep;
	Gun			origGuns[256];

	// get original palette
	GetColors(origGuns, 0, 255);
	memcpy(guns, origGuns, sizeof(guns));

	if(faderSteps > 1)
	{
		for (iStep = faderSteps; iStep >= 0; iStep--)
		{
			for (i = 0; i < 256; i++)
			{
				guns[i].r = (uchar) ((origGuns[i].r * iStep) / faderSteps);
				guns[i].g = (uchar) ((origGuns[i].g * iStep) / faderSteps);
				guns[i].b = (uchar) ((origGuns[i].b * iStep) / faderSteps);
			}
		
			//WaitForVBL();
			UpdateColors(0, 255);
		}
	}
	else
	{
		memset(guns,CI_BLACK, sizeof(guns));
		UpdateColors(0, 255);
	}
}


//	immediate, synchronous fade from black
//
void
TPalette::FadeUp(int faderSteps)
{
	int		 	i;
	int		 	iStep;
	Gun			destGuns[256];

	if(faderSteps > 1)
	{
		// dest palette is in TPalette::guns already
		memcpy(destGuns, guns, sizeof(guns));

		// 16 fade steps
		for (iStep=1; iStep<=faderSteps; iStep++)
		{
			for (i = 0; i < 256; i++)
			{
				guns[i].r = (uchar) ((destGuns[i].r * iStep) / faderSteps);
				guns[i].g = (uchar) ((destGuns[i].g * iStep) / faderSteps);
				guns[i].b = (uchar) ((destGuns[i].b * iStep) / faderSteps);
			}
		
			//WaitForVBL();
			UpdateColors(0, 255);
		}
	}
	else
	{
		// dest palette is in TPalette::guns already
		UpdateColors(0, 255);
	}
}



//--------------------------------------------------------------------------
//
// Class PalUtility
//
//--------------------------------------------------------------------------

PalUtility::PalUtility(void)
{
	gToCue = NULL;
}


// Make sure that a valid range is going to be used
void
PalUtility::CheckRange(uchar start, uchar end)
{
	char mess[100];

	if (start >= end)
	{
		sprintf(mess, "Invalid cycle range: %d %d", start, end);
		APanic(mess);
	}
}


//--------------------------------------------------------------------------
//
// Class PalCycler
//
//--------------------------------------------------------------------------

PalCycler::PalCycler(uchar cycType, uchar gunStart, uchar gunEnd,
		grip gSourcePal,
		uint32 cycRate, uint16 cycTimes,
		grip gCueWhenDone)
{
	uint16	i;
	TPalette	*palette;

	char mess[100];

	if (cycType <= PCYC_NONE ||
		cycType >= PCYC_MAX)
	{
		sprintf(mess, "Invalid cycle type: %d", cycType);
		APanic(mess);		
	}

	CheckRange(gunStart, gunEnd);

	cycleType = cycType;
	switch(cycleType)
	{
		case PCYC_FORWARD:
		case PCYC_FWD_OSC:
			curDirection = PDIR_FORWARD;
			break;

		case PCYC_REVERSE:
		case PCYC_REV_OSC:
			curDirection = PDIR_REVERSE;
			break;
		
	}

	SetInterval(cycRate);

	// number of times to cycle (0 means forever)
	cycleTimes = cycTimes;

	// Oscillators must go both directions to complete a cycle.
	// In order to save code space and execution time, we will
	// just multiply the cycleTimes by 2.  This also means that
	// oscillators have a cycleTimes limit half the size of
	// the other cyclers.

	if (cycleType == PCYC_FWD_OSC ||
	 	 cycleType == PCYC_REV_OSC)
	{
		cycleTimes *= 2;		
	}

	startGun = gunStart;
	endGun = gunEnd;
	gunRange = endGun - startGun;

	gOwner = gSourcePal;

	gToCue = gCueWhenDone;

	// init the count variables

	curCount = totalCycles = 0;

	// copy the range that we are going to cycle
	
	palette = ADerefAs(TPalette, gOwner);
	for (i = startGun; i <= endGun; i++)
 	{
		guns[i] = palette->guns[i];
	}
}


PalCycler::~PalCycler()
{
}


void
PalCycler::Service()
{
	char		cycleDirection;
	TPalette	*palette;

	if (cycleType != PCYC_NONE) {
		// regardless of what happens, we will cycle in our current
		// direction for this pass
		cycleDirection = curDirection;

		// inc the count so that we can check to see if
		// we have gone through the entire range
		curCount++;

		if (curCount >= gunRange)
		{
			// we have reached the end of the range
			curCount = 0;

			totalCycles++; 

			if (cycleType == PCYC_FWD_OSC ||
	 	 		 cycleType == PCYC_REV_OSC)
			{
				// toggle back and forth with an XOR
				curDirection ^= 1;
			}

		}

		RotatePalette(cycleDirection);
		palette = ADerefAs(TPalette, gOwner);
		palette->UpdateColors(guns, startGun, endGun);

		if (cycleTimes)
		{
			if (totalCycles >= cycleTimes)
			{
				// we are done

				if (gToCue)
				{
					APostNotice(N_CUE, gToCue);
				}

				// get ourself out of the list and free the memory
				palette->CleanupUtil(gSelf);
			}
		}
	}
}


void
PalCycler::RotatePalette(char cycleDirection)
{
	Gun		tempGun;
	uint16	i;

	if (cycleDirection == PDIR_FORWARD)
	{
		// forward
		tempGun = guns[startGun];

		for (i = startGun; i < endGun; i++)
		{
			guns[i] = guns[i + 1];
		}

		guns[i] = tempGun;
	}
	else
	{
		// backward
		tempGun = guns[endGun];

		for (i = endGun; i > startGun; i--)
		{
			guns[i] = guns[i - 1];
		}

		guns[i] = tempGun;
	}
}


//	the Owning object calls Utility::Setup() to start it up
void
PalCycler::Setup(void* pOwner)
{
	// this is just here to keep Utility::Setup from calling APanic

	pOwner = pOwner;	// Gets rid of unreferenced warning
}


//--------------------------------------------------------------------------
//
// Class PalFader
//
//--------------------------------------------------------------------------


PalFader::PalFader(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					uchar red, uchar green, uchar blue,
					grip gCueWhenDone)
{
		Setup(gunStart, gunEnd, gCaller,	fadeRate, fadeSteps,
				red, green, blue,gCueWhenDone);
}


PalFader::PalFader(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					grip gCueWhenDone)
{
		Setup(gunStart, gunEnd, gCaller,	fadeRate, fadeSteps,	gCueWhenDone);
}


void
PalFader::Setup(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					uchar red, uchar green, uchar blue,
					grip gCueWhenDone)
{
		Setup(gunStart, gunEnd, gCaller,	fadeRate, fadeSteps, gCueWhenDone);
		targetGun.r = red;	
		targetGun.g = green;	
		targetGun.b = blue; 

		// we will be fading to a gun, not a palette
		useTargetGun = TRUE;
}


void
PalFader::Setup(uchar gunStart, uchar gunEnd,
					grip gCaller,
					uint32 fadeRate, uint16 fadeSteps,
					grip gCueWhenDone)
{
	TPalette	palette;
	char mess[100];

	CheckRange(gunStart, gunEnd);

	// we will be using a palette, not just a gun
	useTargetGun = FALSE;

	SetInterval(fadeRate);

	// number of times it will take to fade to the new palette
	if (fadeSteps < 1)
	{
		sprintf(mess, "Invalid fadeSteps: %d", fadeSteps);
		APanic(mess);
	}

	faderSteps = fadeSteps;

	startGun = gunStart;
	endGun = gunEnd;

	gOwner = gCaller;

	gToCue = gCueWhenDone;

	// init the count variables

	curStep = 0;

	// get the current palette from the VGA card
	palette.GetColors(srcGuns, startGun, endGun);
}


void
PalFader::Service()
{
	TPalette	*palette;
	uint		i;
	uint		percent;
	Gun		workGun;

	// find the current percent that we are going to do

	curStep++;
	if (curStep >= faderSteps)
	{
		// last step
		percent = 100;
	}
	else 
	{
		percent = (100 / faderSteps) * curStep;
	}

	if (useTargetGun)
	{
		// we will be fading to a gun, not a palette
		workGun = targetGun;	
	}

	palette = ADerefAs(TPalette, gOwner);
	for (i = startGun; i <= endGun; i++)
 	{
		if (!useTargetGun)
		{
			// if not using a target gun, use the palette
			workGun = palette->guns[i];
		}
	
		// NOTE: No floating point is used in the below math.
		//       The division by 100 is delayed until the end.
		guns[i].r = (uchar) (srcGuns[i].r + (((workGun.r - srcGuns[i].r) * percent) / 100));
		guns[i].g = (uchar) (srcGuns[i].g + (((workGun.g - srcGuns[i].g) * percent) / 100));
		guns[i].b = (uchar) (srcGuns[i].b + (((workGun.b - srcGuns[i].b) * percent) / 100));
	}

	palette->UpdateColors(guns, startGun, endGun);

	if (curStep >= faderSteps)
	{
		// last step

		if (gToCue)
		{
			APostNotice(N_CUE, gToCue);
		}

		// get ourself out of the list and free the memory
		palette->CleanupUtil(gSelf);
	}
}


//	the Owning object calls Utility::Setup() to start it up
void
PalFader::Setup(void* pOwner)
{
	// this is just here to keep Utility::Setup from calling APanic

	pOwner = pOwner;	// Gets rid of unreferenced warning
}




