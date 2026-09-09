//	FLICSMK.CPP
//
//	Copyright 1996, Tachyon, Inc.
// All Rights Reserved
//
// Play a smacker file
//
// 1/15/96


//Data transfer rate of single speed CD-ROM drive
#define SINGLE_SPEED_CD_DRIVE		150000
//Number of colors in 8-bit palette
#define COLORS							256
//Hex form of ASCII space character
#define SPACE							0x20
//Turns sound off in Smacker
#define SOUND_OFF						0
//Turns sound on in Smacker
#define SOUND_ON						1
// rate for the timer to go off to handle the sound driver system
#define  _TIMER_RATE             60

#include "flicsmk.hpp"
#include "apievt.hpp"
#include "apifont.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "bamfunc2.hpp"
#include "fontmgr.hpp"
#include "mem.hpp"
#include "memmgr.hpp"
#include "mono.hpp"
#include "resmgr.hpp"
#include "soundmgr.hpp"

#include "modex.hpp"
#include "i86.h"
#include "dos.h"
#include "string.h"
#include "stdio.h"
#include "fstream.h"
#include "stdlib.h"
#include "conio.h"
#include <ctype.h>


extern void MouseHandler( int draw_mouse );

#ifdef OS_DOS
// In Assembly in DOS/TIGRE (modex.asm)
extern "C"
{
#endif

//Blit routine for Smacker flic files
void		AFBlit(uchar* pData, uint vSeg);

//Synchronize with video blanking period routine
void		SyncWithVBL();

#ifdef OS_DOS
}
#endif

enum
{
	FADE_UP,
	FADE_DOWN
};


RCFUNC void PTR4* RADLINK radmalloc(u32 numbytes)
{
	void PTR4* temp;
	grip	gTmp;

	if (numbytes==0)
  		return(0);

	if(numbytes==0xffffffff)
	{
	 	temp = 0;
	}
	else
	{
  		//temp = mem_malloc(numbytes);

		//force malloc to return any error
 		TRACK_MEM("Smacker Alloc");  	gTmp = pMemMgr->Malloc(numbytes,TRUE);
 		if(gTmp)
 			temp = (void PTR4*) ADeref(gTmp);
 		else
 			temp = 0;
	}

	return(temp);
}

RCFUNC void RADLINK radfree(void PTR4* ptr)
{
	grip	gTmp;

	//mem_free(ptr);
	gTmp = AGetGrip(ptr);
	AFree(gTmp);
}


////Retrieve and store current video mode
//char GetVideoMode()
//{
//	REGS regs;
//
//	memset(&regs, 0, sizeof (regs));
//	regs.x.eax = 0xf00;
//	int386 (0x10, &regs, &regs);
//	return(regs.h.al);
//}
//
//
////Restore original video mode
//void SetVideoMode(char videoMode)
//{
//	REGS regs;
//
//	memset(&regs, 0, sizeof (regs));
//	regs.x.eax = videoMode;
//	int386 (0x10, &regs, &regs);
//}


//======================================================================
TFlicSmacker::TFlicSmacker()
{
	int	i;

	smk = NULL;

	//Sound volume is a range of 0 to 65536
	//Default sound volume
	soundVolume = 40000;

	//Sound pan is a range of 0 to 65536 (0 = all left, 65536 = all right)
	//Default sound pan (middle)
	soundPan = 32768;

	activatePalette = FALSE;
	fFlicDone = FALSE;
	forceCnt = 0;

	for(i=0;i<7;i++)
		trackVol[i] = 100;

	//Smacker decompression buffer
	TRACK_MEM("decomp Buf");	gDecBuf = AMalloc(chSize + 320 * 400);

	TRACK_MEM("Text Script Arr");	changeText = new SmkTextScript[TEXT_SCRIPT_SIZE];

}


TFlicSmacker::~TFlicSmacker()
{
	delete [] changeText;
	AFree(gDecBuf);
}




//////////////////////////////////////////////////////////////////////////////
//																									 //
//Change sound volume as indicated by the action code								 //
//		Action Code					Description												 //
//			  0				turn sound off				 									 //
//			  1				turn sound on					 								 //
//			  v				adjust individual track's volume							 //
//																									 //
//////////////////////////////////////////////////////////////////////////////

void
TFlicSmacker::ChangeVolume(SmkScript* pChgVol, Smack* smk, u32 frameNo)
{

	u16	thisStep, faderSteps, fadeVolume;
	int	i;
	long	track;


	if(pChgVol)
	{
		//Check for "stop sound" key 
		if(pChgVol->action == '0')
		{
			//Play sound with no volume
			SmackVolumePan(smk, SMACKTRACKS, 0, soundPan);
		}
		else
		//Check for "start sound" key 
		if(pChgVol->action == '1')
		{
			//Turn sound on
			for(i=0,track=SMACKTRACK1; i < 7; i++,track<<1)
				SmackVolumePan(smk, track, (soundVolume * trackVol[i])/100, soundPan);
	
		}
		else
		if(pChgVol->action == 'v')
		{
			if(pChgVol->aux1)
			{
				i = pChgVol->aux1-1;
				trackVol[i] = pChgVol->aux2;
				SmackVolumePan(smk, (SMACKTRACK1)<<(i), (soundVolume * trackVol[i])/100, soundPan);
			}
		}
		else
		{
	
			//Compute number of steps in the volume fade progression
			faderSteps = (u16) (pChgVol->lastFrame - pChgVol->firstFrame + 1);

			//Compute which step this is in the volume fade progression
			thisStep = (pChgVol->action == '-')? (u16) (pChgVol->lastFrame - frameNo) : (u16) (frameNo - pChgVol->firstFrame + 1);

			//Fade the volume level up or down
			fadeVolume = (u16) ((soundVolume * thisStep) / faderSteps);

			//Set new volume level
			for(i=0,track=SMACKTRACK1; i < 7; i++,track=track<<1)
				SmackVolumePan(smk, track, (fadeVolume * trackVol[i])/100, soundPan);
		}
	}
	//lets set default volume for starting 
	else
	{
	  	SmackVolumePan(smk, SMACKTRACKS, soundVolume, soundPan);
	}
}


//////////////////////////////////////////////////////////////////////////////
//																									 //
//Change palette as indicated by the action code								 	 //
//		Action Code					Description												 //
//			  d				fade palette down to minimum intensity					 //
//			  i				ignore palette change (default)							 //
//			  r				remap palette without changing intensity				 //
//			  u				fade palette up to maximum intensity					 //
//																									 //
//////////////////////////////////////////////////////////////////////////////

void
TFlicSmacker::ChangePalette(SmkScript* pChgPal, Smack* smk, u32 frameNo)
{
	u32			i,	thisStep, faderSteps;

	//Check for a palette change request with an action code other than 'i' 
	if(pChgPal)
	{
		if(pChgPal->action != 'i')
		{
			//Copy new palette values to the palette work area
			memcpy(guns, smk->Palette, COLORS * sizeof(Gun));

			//Check for a palette remap request
			if(pChgPal->action == 'r')
			{
				//Check for a palette change in this frame
				if(smk->NewPalette)
				{
					//Force the palette change to occur when the current frame is displayed
					activatePalette = TRUE;
				}
			}
			else
			{
				//Compute number of steps in the palette fade progression
				faderSteps = pChgPal->lastFrame - pChgPal->firstFrame + 1;

				//Compute which step this is in the palette fade progression
				thisStep = (pChgPal->action == 'd')? pChgPal->lastFrame - frameNo : frameNo - pChgPal->firstFrame + 1;

				//Change the intensity (fade) each of the 256 colors in the palette
				for (i = 0; i < COLORS; i++)
				{
					//Change the intensity of each gun in this color
					guns[i].r = (uchar) ((guns[i].r * thisStep) / faderSteps);
					guns[i].g = (uchar) ((guns[i].g * thisStep) / faderSteps);
					guns[i].b = (uchar) ((guns[i].b * thisStep) / faderSteps);
				}

				//Force the palette change to occur when the current frame is displayed
				activatePalette = TRUE;
			}
		}
	}
	//no scripted change requested, but we need to push out a starting palette
	else
	{
		//Copy new palette values to the palette work area
		memcpy(guns, smk->Palette, COLORS * sizeof(Gun));

		//Force the palette change to occur when the current frame is displayed
		activatePalette = TRUE;
	}
}


void
TFlicSmacker::Fader(int	upDown)
{
	u32  	i,	iSteps, faderSteps = 96;
	u32	startStep, endStep;
	int	dir;

	if(upDown == FADE_UP)
	{
		startStep = 1;
		endStep = faderSteps;
		dir = 1;
	}
	else
	{
		startStep = faderSteps;
		endStep = 1;
		dir = -1;
	}

	//copy current value of guns which has already been set for first frame
	//and then fade up to whatever that is
	memcpy(destGuns, guns, COLORS * sizeof(Gun));

	//gradually fade up
	for (iSteps=startStep; iSteps!=endStep; iSteps += dir)
	{
		for (i = 0; i < 256; i++)
		{
			guns[i].r = (uchar) ((destGuns[i].r * iSteps) / faderSteps);
			guns[i].g = (uchar) ((destGuns[i].g * iSteps) / faderSteps);
			guns[i].b = (uchar) ((destGuns[i].b * iSteps) / faderSteps);
		}
	
	  	SVGASetPalette(guns);
	}
}


//////////////////////////////////////////////////////////////////////////////
//																									 //
//Process text change																		 //
//		Action Code				Description													 //
//			  t				text entry -changed to 0 after processing				 //
//																									 //
//////////////////////////////////////////////////////////////////////////////

void
TFlicSmacker::ChangeText(SmkTextScript *pChgText, int frameNum)
{
	char	*pTxt;
	justify	just;

	//if first frame lets write text into buffer
	if(frameNum == pChgText->firstFrame)
	{
		if(toupper(pChgText->just) == 'C')
			just = DG_JUST_CENTER;
		else
		if(toupper(pChgText->just) == 'R')
			just = DG_JUST_RIGHT;
		else
			just = DG_JUST_LEFT;

		pFontMgr->SetRes(pChgText->font);
		SetFontColors(pChgText->c[0],pChgText->c[1],pChgText->c[2],pChgText->c[3],pChgText->c[4],pChgText->c[5],pChgText->c[6]);
		pTxt = sqbCine.Load(pChgText->sqbRes,pChgText->sqbNum);
		ASetString(pChgText->textX, pChgText->textY,pTxt, (uchar*)pDecBufCH, pChgText->wrapWidth, NULL, just);
	}
	//last frame -lets cover over the text
	else
	{
		pChgText->action = 0;
		CopyCel(pDecBufCH,pChgText->coverX,pChgText->coverY,RES_ANIM,
				  pChgText->coverAni, pChgText->coverCel, FALSE);
	}
}


//////////////////////////////////////////////////////////////////////////////
//																									 //
//Main Smacker player routine																 //
//	Usage:																						 //
// 		file							  smacker file path and name					 //
// 		[extra (% 1024)]			  extra buffer size in k bytes				 //
// 		[CD sim speed (% 150k)]	  CD-ROM drive simulation speed x 150kb	 //
//			[start frame]				  frame# to advance to and begin playback  //
// 		[skip (y/n)]				  drop frames to maintain audio streaming? //																				 //
//																									 //
//////////////////////////////////////////////////////////////////////////////

int
TFlicSmacker::Play(int cineNum_P, int extraBuf, int simSpeed,
						 int startFrame, bool skipFrames)
{
	u32 				sFirstFrame;
	u32 				sLastFrame;
	u32 				palChgs = 0;
	u32 				volChgs = 0;
	u32 				textChgs = 0;
	u32				jumpToFrame = 0;
	u32 				i=0, j=0;
	u32 				startFlags = 0;
	u32 				xtraBufSize = SMACKAUTOEXTRA;
	u32				purgeSize=0;
	FILE* 			smkFile = NULL;
	FILE* 			scriptFile = NULL;
	char				smkLine[256];
	char				scriptLine[256];
	char 				sAction;
	char*				pScriptLine;
	SmkScript*		pChgPal = NULL;
	SmkScript*		pChgVol = NULL;
	SmkTextScript* pChgText = NULL;
	int				ret = 0;
	void PTR4* 		tmpMemPtr=NULL;

	//set default font colors -5 color light blue
	int	defColor[7] = {CI_SKIP,93,76,74,74,48,0};

	cineNum = cineNum_P;

	//Save VGA adapter's current mode
	//prevVideoMode = GetVideoMode();

	//Set VGA adapter to X mode
	SetXMode();
	//Clear screen page 0 to black (@A0000)
	FillScreen(0);
	//Switch to screen page 1
	XModeFlipPage();
	//Clear screen page 1 to black (@A8000)
	FillScreen(0);
	//Switch back to screen page 0
	XModeFlipPage();

	//Initialize pointer to fake celheader of decompression buffer
	//fake celheader for use by CopyCel
	pDecBufCH = ADerefAs(CelHeader,gDecBuf);
	memset(pDecBufCH,NULL,chSize);
	pDecBufCH->width = 320;
	pDecBufCH->height = 400;

	//Initialize pointer to decompression buffer
	pDecBuf = (uchar*)pDecBufCH + chSize;

	//Set extra buffer size
	if(extraBuf)
	{
		xtraBufSize = extraBuf * 1024;

		//Set flag to preload the extra buffer
		startFlags |= SMACKLOADEXTRA;
	}

	if(simSpeed)
	{
		SmackSimulate(simSpeed * SINGLE_SPEED_CD_DRIVE);
		startFlags |= SMACKSIMULATE;
	}

	//Set "jump to" frame
	if(startFrame > 1)
		jumpToFrame = startFrame;

	//Check for "don't skip frames" parameter
	//Smacker will only skip frames if there's audio and the video gets behind
	if(!skipFrames)
	{
		//Set "don't skip frames" flag
		//could possibly lead to audio starvation and cause audio "dead-time"
		startFlags |= SMACKNOSKIP;
	}

	//smacker volume is 0 to 65536 -not 128
	soundVolume = (u16)(pSoundMgr->GetMasterMidiVolume() * 512);

	//Set flag to allow us to set volume on-the-fly
	startFlags |= SMACKNEEDVOLUME;

	if(pSoundMgr->hDigiDriverHandle != -1)
	{
		//Set audio track flags
		startFlags |= SMACKTRACKS;
	}

	//we can't be here if the file didn't exist
	//startFlags |= SMACKFILEHANDLE;
	//smkFile = pResMgr->GetCuedResFile(RES_SMACKER, cineNum);

	//may have to use this if can't find smacker files in stuff files
	while (pResMgr->CopyResName(smkLine, RES_SMACKER, cineNum, i))
	{
		// Open the resource file
		smkFile = fopen(smkLine, "rb");
		if (smkFile)
		{
			fclose(smkFile);
			break;
		}
		++i;
	}

	if(AExists(RES_SMK_SCRIPT,cineNum))
	{
		scriptFile = pResMgr->GetCuedResFile(RES_SMK_SCRIPT, cineNum);
	}
	else
	//search for default script
	if(AExists(RES_SMK_SCRIPT,1))
	{
		scriptFile = pResMgr->GetCuedResFile(RES_SMK_SCRIPT, 1);
	}


	//Check for successful opening of script file
	if(scriptFile)
	{
		//Initialize palette changes pointer
		pChgPal = changePal;

		//Initialize volume changes pointer
		pChgVol = changeVol;

		//Initalize text overlay changes pointer
		pChgText = changeText;

		//Process script file
		do
		{
			//Read in a line from the script file
			fgets(scriptLine, sizeof(scriptLine), scriptFile);

			//Check that line is not a comment and EOF hasn't been reached
			if(scriptLine[0] != ';' && scriptLine[0] != '\r' && !feof(scriptFile))
			{
				//Reset pointer to beginning of script line
				pScriptLine = scriptLine;

				//Ignore any leading spaces
				while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++))); 

				//Get first frame number of this script entry
				sFirstFrame = (u32) atol(--pScriptLine);

				//Scan past the "first frame" parameter
				while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

				//Ignore spaces leading to the "last frame" parameter
				while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

				//Get last frame number of this script entry
				sLastFrame = (u32) atol(--pScriptLine);

				//Scan past the "last frame" parameter
				while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

				//Ignore spaces leading to the "action code" parameter
				while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

				//Get the "action code"
				sAction = *(--pScriptLine);

				//Check for a font color change
				if(sAction == 'f')
				{
					for(j=0;j<7;j++)
					{
						//---------------
						//Scan past this parameter
						while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 
						//Ignore any leading spaces
						while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++))); 

						//Get font colors
						defColor[j] = (u32) atol(--pScriptLine);
					}
				}
				else
				//Check for a text overlay action
				if(sAction == 't')
				{
					//Get first frame number of this script entry
					pChgText->firstFrame = sFirstFrame;

					//Get last frame number of this palette change
					pChgText->lastFrame = sLastFrame;

					//Get the change volume "action code"
					pChgText->action = sAction;

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 
					//Ignore any leading spaces
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++))); 

					//Get font number of this script entry
					pChgText->font = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get sqb Res of this script entry
					pChgText->sqbRes = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get sqb number of this script entry
					pChgText->sqbNum = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the text X coord
					pChgText->textX = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the text Y coord
					pChgText->textY = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the text wrap width
					pChgText->wrapWidth = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the text justification 'L' -left, 'R' -right, 'C' -centered
					pChgText->just = *(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the coverAni
					pChgText->coverAni = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the coverCel
					pChgText->coverCel = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the cover X coord
					pChgText->coverX = (u32) atol(--pScriptLine);

					//---------------
					//Scan past this parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 

					//Ignore spaces leading to the next parameter
					while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++)));

					//Get the cover Y coord
					pChgText->coverY = (u32) atol(--pScriptLine);

					//---------------
					//set the current font colors as those to be used for this string
					pChgText->c[0] = defColor[0];
					pChgText->c[1] = defColor[1];
					pChgText->c[2] = defColor[2];
					pChgText->c[3] = defColor[3];
					pChgText->c[4] = defColor[4];
					pChgText->c[5] = defColor[5];
					pChgText->c[6] = defColor[6];

					pChgText++;

					//Update volume changes tally
					textChgs++;
				}
				else
				//Check for a volume change action
				if(sAction == '-' || sAction == '+' || sAction == '0' ||
					sAction == '1' || sAction == 'v')
				{
					//Get first frame number of this script entry
					pChgVol->firstFrame = sFirstFrame;

					//Get last frame number of this palette change
					pChgVol->lastFrame = sLastFrame;

					//Get the change volume "action code"
					pChgVol->action = sAction;

					if(sAction == 'v')
					{
						//Scan past this parameter
						while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 
						//Ignore any leading spaces
						while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++))); 

						//Get track number
						pChgVol->aux1 = (uchar) atol(--pScriptLine);

						//bounds checking
						if(pChgVol->aux1 < 1 || pChgVol->aux1 > 7)
							pChgVol->aux1 = 0;

						//Scan past this parameter
						while(pScriptLine < (scriptLine + sizeof(scriptLine)) && !isspace(*(++pScriptLine))); 
						//Ignore any leading spaces
						while(pScriptLine < (scriptLine + sizeof(scriptLine)) && isspace(*(pScriptLine++))); 

						//Get volume percent change
						pChgVol->aux2 = (uchar) atol(--pScriptLine);

					}

					pChgVol++;

					//Update volume changes tally
					volChgs++;
				}
				//Assume action is a palette change (d, i, r, or u)
				else
				{
					//Get first frame number of this script entry
					pChgPal->firstFrame = sFirstFrame;

					//Get last frame number of this palette change
					pChgPal->lastFrame = sLastFrame;

					//Get the change palette "action code"
					pChgPal++->action = sAction;

					//Update palette changes tally
					palChgs++;
				}
			}
			MouseHandler(FALSE);
		//Loop until all lines have been processed
		}while(!feof(scriptFile));

		//Close script file
		fclose(scriptFile);

		//Reset volume changes pointer to first palette change item
		pChgVol = changeVol;

		//Reset palette changes pointer to first palette change item
		pChgPal = changePal;
	}

	if(extraBuf)
	{
	//because we may need a large contigious prealloc buffer lets try
	//to acquire it now and then free it just before smackOpen
	TRACK_MEM("Flicsmk prealloc");		tmpMemPtr = mem_malloc(xtraBufSize+5000);
	}

	if(tmpMemPtr)
	{
		//we got the memory now free it for SmackOpen
		mem_free(tmpMemPtr);
	}
	else
	{
		//lets not try purging (SMACKAUTOEXTRA == 0xFFFFFFFF)
		if(extraBuf)
		{
			//dummy statement
			switch(extraBuf)
			{
				default:
					i=0; //dummy statement
					purgeSize = pResMgr->Purge((uint16)extraBuf);

					//now lets see if we can get it
					tmpMemPtr = mem_malloc(xtraBufSize);

					if(tmpMemPtr)
					{
						mem_free(tmpMemPtr);
						break;
					}

					//lets back-off in 1/2 meg incr.
					while(xtraBufSize > purgeSize && xtraBufSize >= (500 * 1024) )
			  			xtraBufSize -= (500 * 1024);

					//if still to big or to small to be useful
					if(xtraBufSize > purgeSize || xtraBufSize < 50000)
					{
						//lets turn the flag back off
						startFlags &= !SMACKLOADEXTRA;
						xtraBufSize = SMACKAUTOEXTRA;
						break;
					}
					else
					{
						//now lets see if we can get it
						tmpMemPtr = mem_malloc(xtraBufSize);

						if(tmpMemPtr)
						{
							mem_free(tmpMemPtr);
							break;
						}

						// we still didn't get it -so...
						//lets turn the flag back off
						startFlags &= !SMACKLOADEXTRA;
						xtraBufSize = SMACKAUTOEXTRA;
					}
			}
			pMono->Out("=============================================================\n");
			pMono->Out("Flic could only acquire %d of requested %d for preload.\n",xtraBufSize,(extraBuf*1024));
		}
	}


	//Open smacker file
	//smk=SmackOpen((char PTR4*)smkFile, startFlags, xtraBufSize);
	smk=SmackOpen(smkLine, startFlags, xtraBufSize);

	//Check for successful open of smacker file
	if(smk)
	{
		//Check for jump ahead to frame option
		if(jumpToFrame)
		{
			//Turn sound off while jumping ahead
			SmackSoundOnOff(smk, SOUND_OFF);
		}

		//Tell Smacker where to decompress the flic
      SmackToBuffer(smk, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, pDecBuf, 0);

      //Decompress and display each flic frame
   	for(i = 1; i <= smk->Frames; i++)
		{
			MouseHandler(FALSE);

			//Decompress smacker frame data
			SmackDoFrame(smk);

			//if first frame - lets set default volume
			//-may be changed later by script
			if(i == 1)
			{
				ChangeVolume(NULL, smk, i);
			}

			//Check for volume changes
			if(volChgs)
			{
				//search all text entries for change this frame
				for(j=0;j<volChgs;j++)
				{
					pChgVol = &changeVol[j];

					//lets cut down on the size of the list processed at once
					//if current frame less than this entry's firstFrame
					//then lets stop processing the rest of the list
					//(this assumes the list is in order, but we can still
					// process multiple entries from the same frame)
					if(i < pChgVol->firstFrame)
						break;

					//are we finished processing this entry? t=no 0=yes
 					if(pChgVol->action)
					{
						//Check for volume change in this frame
 						if(i >= pChgVol->firstFrame && i <= pChgVol->lastFrame)
						{
							//Process volume change
							ChangeVolume(pChgVol, smk, i);

							//Check for last frame in this change group
							if(i == pChgVol->lastFrame)
							{
								//Update volume change variables
								pChgVol->action = 0;
							}
						}
					}
				}
			}

			//Check for jump ahead frame being reached
			if(i >= jumpToFrame)
			{
				//Check for this frame being the jump ahead frame
				if(i == jumpToFrame)	
				{
					//Turn sound on for remainder of playback
					SmackSoundOnOff(smk, SOUND_ON);
				}				

				//Blit each frame's rectangles to the decompression buffer
				while(SmackToBufferRect(smk, 0))
				{
					MouseHandler(FALSE);
				};

				//Copy current frame to hidden screen page
				AFBlit((uchar*) pDecBuf, ((uint) pVGAMem)^X_MODE_PAGE_MASK);
				MouseHandler(FALSE);
			}

			//if first frame - lets force the palette out
			//-can be changed later by script
			if(i == 1 && smk->NewPalette)
			{
				//Change screen colors to those of the new palette
				ChangePalette(NULL, smk, 0);
			}

			//Check for palette changes
			if(palChgs)
			{
				//Check for palette change in this frame
 				if(i >= pChgPal->firstFrame && i <= pChgPal->lastFrame)
				{
					//Process palette change
					ChangePalette(pChgPal, smk, i);

					//Check for last frame in this change group
					if(i == pChgPal->lastFrame)
					{
						//Update palette change variables
						palChgs--;
						pChgPal++;
					}
				}
			}
			//no scripted palette events left -so ignore any new palettes
			//else
			//{
			//	//Check for new palette
  		  	//	if(smk->NewPalette)
			//	{
			//		//Change screen colors to those of the new palette and fade to black
			//		ChangePalette(NULL, smk, 0);
			//	}
			//}


			//Check for text changes
			if(textChgs)
			{
				//search all text entries for change this frame
				for(j=0;j<textChgs;j++)
				{
					pChgText = &changeText[j];

					if(i < pChgText->firstFrame)
						break;

					//are we finished processing this entry? t=no 0=yes
 					if(pChgText->action)
					{
						if(i == pChgText->firstFrame || i == pChgText->lastFrame)
						{
							//Process text change
							ChangeText(pChgText, i);
						}

					}
				}
			}

			//Check for jump ahead frame being reached
			if(i >= jumpToFrame)
			{
				//Wait until vertical blanking period before updating video screen
				SyncWithVBL();

				//Check for need to do a palette change
				if(i != 1 && activatePalette)
				{
					//Change screen colors to those of the new palette
	  				SVGASetPalette(guns);
					activatePalette = FALSE;
				}

				//Toggle screen address	to next page and display hidden screen page
				XModeFlipPage();

				//after displaying first page, fade the palette up
				if(i == 1)
					Fader(FADE_UP);
			}

			//Check for another frame to process after the current frame
			if(i < smk->Frames)
			{
				//Decompress the next frame
				MouseHandler(FALSE);
        		SmackNextFrame(smk);
			}
			else
			{
				fFlicDone = TRUE;
			}

			//Only do key press check after playback starts
			if(i > 1 && i >= jumpToFrame)
			{
				forceCnt++;
				//Wait until time to display the next frame
				while(forceCnt >= 10 || (!fFlicDone && SmackWait(smk)))
				{
					APublishNext();
					MouseHandler(FALSE);
					forceCnt = 0;
				} 
			}

			if(fFlicDone)
			{
				Fader(FADE_DOWN);

				break;
			}
		}
	}
	else
	{
		//couldn't open smk file
		ret = 1;
	}

	//we have to close the file ourselves
	//if(smkFile)
	//	fclose(smkFile);

	//Close smacker file  
	if(smk)
		SmackClose(smk);

	//lets do this to make sure we've cleaned up and on page 0
	//Set VGA adapter to X mode
	SetXMode();

	return(ret);
}
