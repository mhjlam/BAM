//
// SOUNDMGR.CPP
//
// March 3, 1994
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Please Note:
//
// - THIS IS VERY IMPORTANT: Currently, because the memory manager
//   is not completely implemented, sound resources and songs are
//   not memory locked.  In the future, the sounds and songs should be
//   memory locked when in use.  Then, they should be memory unlocked
//   when done.
//
// - WARNING!!!! - MT 32 code untested!!!!!
//
//	- WARNING!!!! - Streamed sound are flushed from the Resmgr, if present.
//                 So, don't play a sound (non-streaming) and also play
//                 it using streaming at the same time!
//
// - The patch and bank files are as follows:
//   
//   1.bnk = fm melodic.bnk
//   2.bnk = fm drum.bnk 
//   3.bnk = digi instrument patch file
//   4.bnk = MT32 patch file
//
// - All digi sounds are played back at 22050.  Pitch shifting
//   is used to play samples recorded at slower rates.  In order
//   for the samples to sound correct, they should be recorded
//   at a rate that is evenly divisible into 22050.  
//
// - Interleaved stereo sounds must be sampled at 22050 because
//   no pitch shifting can be done.  Also, you cannot swap or
//   change channels on an interleaved stereo sample.
//
// - All digi sounds used for midi must be sampled at the same rate.
//
// - Looping of streamed samples IS NOT currently supported.
//
// - To loop a sampled sound indefinately, pass 0 as the arguement
//	  Play.
//
// - All volumes (midi and digi) range from 0 to 127.  Digi sounds
//   actually have a range of 0 to 0x7ff.  To make things simpler
//   for the applications programmer, all digi sounds are adjusted
//   by the Sound Manager so that we can use the same volume range for
//   all audio devices.
//  
// - This is how to implement the sound manager
//
//   The "sound.cfg" file must be in your local directory.  This
//   contains information needed by the sound manager.
//   A few drivers must be local.  They are:
//   
//      HMIDET.386	- sound detection
//      HMIDRV.386	- digi drivers
//      HMIMDRV.386	- midi drivers
//
//   You must have an instance of the SoundMgr and you must call Init.
//
//      TRACK_MEM("Snd_Mgr");	new SoundMgr;
//      pSoundMgr->Init();
//
//   In your main game loop, you must call the SoundMgr Cycle
//
//      pSoundMgr->Cycle();
//
//   To play a sound or start a song, just call play for that sound.
//
//      TSound	tSound;
//      tSound.Play(52);
//
//      TMusic	tMusic;
//      tMusic.Play(2);
//
//   There are a few other functions (like Stop) and variations of
//   Play that can be called.  See soundmgr.hpp and .cpp for details.
//
//   If you don't want to play any sound and/or music, this can be done
//   two ways. One way is to set the first parameter of SOUNDFX and/or MUSIC
//   in "sound.cfg" to zero.  The other way is to not construct the
//   SoundMgr.  All TSound and TMusic functions check to see if
//   the sound manager is available.  If it isn't, they don't call
//   it.
//
//   The sound manager can be shut down a variety of ways.  When the
//   sound manager is constructed, it sets up an atexit function.  If
//   nothing else shuts down the sound managers, this will.  You can
//   delete pSoundMgr or call ShutDownSoundMgr() yourself if you want.
//
//   To setup your makefile, you should add two libraries:
//
//      SOSDSXIC.LIB - digital sound and detection 
//      SOSMSXIC.LIB - midi
//
//   The tigre makefile (not application) needs to have a compiler option
//   set.
//
//		  -dZORTECH
//
//----[]-------------------------------------------------------------

#include <ctype.h>
#include <dos.h>
#include	"api.hpp"
#include "apievt.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "mono.hpp"
#include "resource.hpp"
#include	"sconfig.hpp"
#include	"savemgr.hpp"
#include "debug.hpp"

#define NDEBUG

extern "C"
{
#include "sosm.h"
}

extern Debugger ResMgrDebug;

#include	"soundmgr.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

//borrowed this from smack.h -used to init hDigiDriverHandle
#define SMACKSOUNDNONE -1

// If this is defined, sound manager debug messages are printed
//#define	SNDMGR_DEBUG	1

//#define	_SOS_DRIVER_RATE		11025
//// rate for the timer to go off to handle the driver system
//#define  _TIMER_RATE             50

#define	_SOS_DRIVER_RATE		22050
// rate for the timer to go off to handle the driver system
#define  _TIMER_RATE             60
//#define  _TIMER_RATE             50

// patch files
#define  _MELODIC_PATCH       1
#define  _DRUM_PATCH          2
#define  _MT32_PATCH          4

#define	SOUND_DEBUG		_SOS_DEBUG_NORMAL

#define	TIMER_DEBUG		_SOS_DEBUG_NORMAL
//#define	TIMER_DEBUG		_SOS_DEBUG_NO_TIMER

// path to the driver files
#define  _SOS_DRIVER_PATH        _NULL

// size of the dma buffer to use 
#define  _SOS_DMA_BUFFERSIZE     0x1000

// port for the driver to use
#define  _MIDI_DRIVER_PORT    0x388

// This is SOS stuff that MUST BE global (rather, not on the stack).
// If memory gets tight, the memory could be allocated for these,
// and then freed after use. 

_SOS_HARDWARE     sSOSHardwareSettings;
WORD     wError;                 // error code returned from functions
WORD     wPort;                  // port found on detect find hardware

// device capabilities returned from the hardware detection system
_SOS_CAPABILITIES sSOSDeviceCaps;

// structure for the MIDI driver initialization function 
_SOS_MIDI_INIT_DRIVER   sSOSMIDIInitDriver;

// structure for the MIDI driver hardware
_SOS_MIDI_HARDWARE      sSOSMIDIHardware; 

// structure used to initialize the digi driver
_SOS_INIT_DRIVER  sSOSInitDriver;

// structure used to initialize the midi digi driver
_SOS_INIT_DRIVER  sSOSInitDMDriver;

void _far cdecl TSoundStreamCallback( WORD wDriverHandle, WORD wAction,
   WORD wSampleHndl );

// structure to pass sosMIDIInitSong
_SOS_MIDI_INIT_SONG     sSOSInitSong;


// track mapping structure, this is used to map which track goes
// out which device. this can also be mapped by the name of the 
// midi track. to map by the name of the midi track use the define
// _MIDI_MAP_TRACK for each of the tracks 
_SOS_MIDI_TRACK_DEVICE   sSOSTrackMap = { 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK 
                                       };

// One buffer is supposed to last 1 tick.  This is how
// many buffers we allocate.
#define	STREAM_BUF_MULTIPLIER	40

SoundMgr	*pSoundMgr = NULL;


// Function to create a pan value.
// Pass the left and right values.
// Left and Right are a percentage from 0 to 100
// ie. MkPan(100, 0)   -  all playing on the left 
// ie. MkPan(0, 100)   -  all playing on the right 
// ie. MkPan(100, 100) -  playing on both sides

int
MkPan(uint left, uint right)
{
	char 	mess[100];
	uint	highValue;
	int	panPos;

	// make sure that the range is valid 
	if (left > 100 || 
	    right > 100)
	{
		sprintf(mess, "Invalid Pan Value(s) MkPan(%d,%d)", left, right);
		APanic(mess);
	}

	highValue = left > right ? left : right;	

	if (left >= highValue)
	{
		// left is the highest value.
		// we have full playing on the left.
		// we have partial playing on the right

		panPos = DIGI_PAN_LEFT + ((right * DIGI_PAN_CENTER)/highValue);
		
	}
	else
	{
		// right is the highest value.
		// we have full playing on the right.
		// we have partial playing on the left

		panPos = DIGI_PAN_RIGHT - ((left * DIGI_PAN_CENTER)/highValue);
	}

	return panPos;
}


void ShutDownSoundMgrNow(SoundMgr *pSndMgr);

bool
SoundMgrSave(uint16 state, FILE *fp)
{
	return	pSoundMgr->Save(state, fp);
}

SoundMgr::SoundMgr(void)
{
	pInitErr = NULL;
	
	// set the devices to off
	digiDevice = midiDevice = SOUND_OFF;

	// no SOS systems are active
	sosActiveSystems = 0;

	//init the handle to something that smacker will recognize as "no driver"
	hDigiDriverHandle = SMACKSOUNDNONE;

	// init banks to not here
	gDigiInstruments = gInstruments = gDrums = gMT32Patch = 0;

	swapDigiLeftAndRight = FALSE;

	// set the global pointer
	if (!pSoundMgr)
	{
		pSoundMgr = this;

		// initialize some structures
		memset(&sSOSInitDriver, 0, sizeof(_SOS_INIT_DRIVER));
		sSOSInitDriver.wBufferSize = _SOS_DMA_BUFFERSIZE;
		sSOSInitDriver.wAllocateBuffer = TRUE;
		sSOSInitDriver.wTimerID = _SOS_NORMAL_TIMER;
		memcpy(&sSOSInitDMDriver, &sSOSInitDriver, sizeof(_SOS_INIT_DRIVER));

		atexit(ShutDownSoundMgr);
		// setup for save
		AtSave(SoundMgrSave);
	}
}

SoundMgr::~SoundMgr(void)
{
	ShutDownSoundMgrNow(this);
	pSoundMgr = NULL;
}

void
SoundMgr::SwapDigiLeftAndRight(bool swapThem)
{
	if (swapThem)
	{
		swapDigiLeftAndRight = TRUE;
	}
	else
	{
		swapDigiLeftAndRight = FALSE;
	}
}


// if the channels are begin swapped, flip the bytes of the pan value

short
SoundMgr::GetPanPosition(int panPos)
{
	short	sosPan;
	short	tmpPan;

	tmpPan = (short) (panPos & 0xffff);

	if (swapDigiLeftAndRight)
	{
		sosPan = (tmpPan >> 8) && ((tmpPan & 0xff) << 8);
	}
	else
	{
		sosPan = tmpPan;
	}

	return sosPan;
}


void
SoundMgr::Init(char *configFileName)
{
	SConfig	sConfig;
	char		configToken[MAX_CONFIG_TOKEN_SIZE];
	char		errMess[400];

	// pointer to the instrument file
	LPSTR		lpInstruments;

	// pointer to the drum file
	LPSTR		lpDrums;

	sConfig.LoadConfig(configFileName, FALSE);

	sConfig.GetEntry(DIGI_STEREO_SWAP, configToken);
	if (configToken[0] == '1')
	{
		// 0 means no swap.  1 means swap
		SwapDigiLeftAndRight(TRUE);
	}

	digiVolume = (uint16) sConfig.GetEntryDecimal(DIGI_VOLUME, configToken);
	if (configToken[0] == 0)
	{
		// no config entry was found
		// set the volume to loudest
		digiVolume = MAX_VOLUME;
	}
	else
	{
		// use the volume from the config file

		// make sure that the volume is within a valid range
		if (digiVolume < 0 ||
			 digiVolume > MAX_VOLUME)
		{
			// set the volume to loudest
			digiVolume = MAX_VOLUME;
		}
	}

	digiVolumeMax = digiVolume;

	midiVolume = (uint16) sConfig.GetEntryDecimal(MIDI_VOLUME, configToken);
	if (configToken[0] == 0)
	{
		// no config entry was found
		// set the volume to loudest
		midiVolume = MAX_VOLUME;
	}
	else
	{
		// use the volume from the config file

		// make sure that the volume is within a valid range
		if (midiVolume < 0 ||
			 midiVolume > MAX_VOLUME)
		{
			// set the volume to loudest
			midiVolume = MAX_VOLUME;
		}
	}

	midiVolumeMax = midiVolume;

	// use values from the config file
	digiDevice = (uint16) sConfig.GetEntryDecimal(DIGI_ID, configToken);
	if (*configToken == 0 ||
		 digiDevice == 0xffff)
	{
		// entry was not found
		// or we have no digi device
		digiDevice = 0;
	}
	else
	{
		// we found an entry. 
		// look for the rest of the required things

		digiPort = (uint16) sConfig.GetEntryDecimal(DIGI_PORT, configToken);
		if (*configToken == 0)
		{
			// entry was not found
			digiDevice = 0;
		}

		digiIRQ = (uint16) sConfig.GetEntryDecimal(DIGI_IRQ, configToken);
		if (*configToken == 0)
		{
			// entry was not found
			digiDevice = 0;
		}

		digiDMA = (uint16) sConfig.GetEntryDecimal(DIGI_DMA, configToken);
		if (*configToken == 0)
		{
			// entry was not found
			digiDevice = 0;
		}
	}

	// check for the music config
	midiDevice = (uint16) sConfig.GetEntryDecimal(MIDI_ID, configToken);
	if (*configToken == 0 ||
		 midiDevice == 0xffff)
	{
		// entry was not found
		// or we have no midi device
		midiDevice = 0;
	}
	else
	{
		midiPort = (uint16) sConfig.GetEntryDecimal(MIDI_PORT, configToken);
		if (*configToken == 0)
		{
			// entry was not found
			midiDevice = 0;
		}
	}

	// unload the config file
	sConfig.ResetConfigFile();

	// if a digi device is being used, check some things
	if (digiDevice != 0)
	{
   	// initialize the detection system, the detection system is optional and 
   	// some driver detections use system resources such as DMA and IRQ
   	// channels.
   	wError = sosDIGIDetectInit( ( LPSTR )_SOS_DRIVER_PATH ); 
   	if(wError)
   	{
      	// display the error string and exit
      	sprintf(errMess, "\nError : %s", sosGetErrorString( wError ) );
      	APanic(errMess);
   	}

		// set the bit for shutdown
		sosActiveSystems |= SOS_DETECT;

		// verify that the requested hardware is here.

		// there is no way to verify music hardware

		// digi hardware
		// zero means, no device selected

		if (digiDevice != 0)
		{
   		wError = sosDIGIDetectFindHardware( digiDevice,
      		&sSOSDeviceCaps, &wPort );
   		if(wError)
   		{
				#ifndef NDEBUG
				InitError(wError);
				#else
				digiDevice = 0;
				pInitErr = "Sound card init failed";
				InitError(wError, pInitErr);
				#endif
   		}
			else
			{
				if (wPort != digiPort)
				{
					// we detected this device on another port
					#ifndef NDEBUG
					sprintf(errMess, "SoundFX device really on port %d", wPort);
					InitError(0, errMess);
					#else
					digiDevice = 0;
					pInitErr = "sound card port is incorrect";
					InitError(wError, pInitErr);
					#endif
				}
				else
				{
					sSOSHardwareSettings.wPort = digiPort;
	   			sSOSHardwareSettings.wIRQ = digiIRQ;
   				sSOSHardwareSettings.wDMA = digiDMA;
   				wError = sosDIGIDetectVerifySettings( &sSOSHardwareSettings );
	   			if(wError)
   				{
						#ifndef NDEBUG
						InitError(wError);
						#else
						digiDevice = 0;
						pInitErr = "Sound card init failed";
						InitError(wError, pInitErr);
						#endif
   				}
				}
			}
		}

		// uninitialize the detection
		sosDIGIDetectUnInit();
		// reset the bit for shutdown
		sosActiveSystems &= ~SOS_DETECT;
	}

	// setup things common to both music and digi effects
	//if (digiDevice != 0 ||
	//	 midiDevice != 0)
	//{

	// WE NOW ALWAYS START TIMER AS REQUIRED FOR SMACKER CODE
   // initialize the TIMER system and tell it to call the DOS timer 
   // at a rate of 18.2 times/second. 
   sosTIMERInitSystem( _TIMER_DOS_RATE, TIMER_DEBUG );

	// set the bit for shutdown
	sosActiveSystems |= SOS_TIMER;

	//}


	// if we are going to use digi, init the digi systems
	if (digiDevice != 0)
	{
   	// initialize the driver system
   	sosDIGIInitSystem( ( LPSTR )_SOS_DRIVER_PATH, SOUND_DEBUG );

		// set the bit for shutdown
		sosActiveSystems |= SOS_DIGI_SYS;
	}

	// setup music
	// WARNING!!!!!!! Because of the Gravis Ultrasound, the midi drivers
	// MUST BE inited before the digi drivers.

	// if we are using the midi system, init it
	if (midiDevice != 0)
	{
   	// initialize the MIDI system
   	sosMIDIInitSystem( _SOS_DRIVER_PATH, SOUND_DEBUG );

		// set the bit for shutdown
		sosActiveSystems |= SOS_MIDI_SYS;

   	// set the pointer to the driver memory for the MIDI driver to 
   	// _NULL. this will tell the load driver routine to allocate new
   	// memory for the MIDI driver
   	sSOSMIDIInitDriver.lpDriverMemory  = _NULL;

   	// setup the port for the MIDI driver to use
   	sSOSMIDIHardware.wPort  =  midiPort;

   	// load and initialize the MIDI driver 
   	wError = sosMIDIInitDriver( midiDevice, &sSOSMIDIHardware,
      	&sSOSMIDIInitDriver, &hMidiDriverHandle );
   	if (wError)
   	{
			#ifndef NDEBUG
			InitError(wError);
			#else
			InitError(wError, "Sound card init failed");
			#endif
   	}
		// set the bit for shutdown
		sosActiveSystems |= SOS_MIDI_SYS_DRIVER;

		// Based on the device type, load the required patch banks

		switch(midiDevice)
		{
			case _MIDI_FM:
			case _MIDI_OPL3:
   			// load the instrument file

				gInstruments = ALoad(RES_BNK, _MELODIC_PATCH);
				lpInstruments = (LPSTR) AGetResData(gInstruments);

   			// load the drum file
				gDrums = ALoad(RES_BNK, _DRUM_PATCH);
				lpDrums = (LPSTR) AGetResData(gDrums);

   			// set the instrument file for the MIDI driver to use, since we are using
   			// the FM driver two instrument files are needed, the first is for 
   			// all melodic instruments and the second is for events on channel 10
   			// which is the drum track.

   			// set the melodic instruments
   			wError =  sosMIDISetInsData( hMidiDriverHandle, lpInstruments, 0x01  );
   			if(wError)
   			{
					#ifndef NDEBUG
					InitError(wError);
					#else
					InitError(wError, "Sound card init failed");
					#endif
   			}

   			// set the drum instruments
   			wError =  sosMIDISetInsData( hMidiDriverHandle, lpDrums, 0x01  );
   			if(wError) 
   			{
					#ifndef NDEBUG
					InitError(wError);
					#else
					InitError(wError, "Sound card init failed");
					#endif
   			}

				break;

			case _MIDI_MT_32:
				// load the patch
				gInstruments = ALoad(RES_BNK, _MT32_PATCH);
				lpInstruments = (LPSTR) AGetResData(gInstruments);

				// set the patch
   			wError =  sosMIDISetInsData( hMidiDriverHandle, lpInstruments, 0x01  );
   			if(wError)
   			{
					#ifndef NDEBUG
					InitError(wError);
					#else
					InitError(wError, "Sound card init failed");
					#endif
   			}
				break;
		}

		SetMasterMidiVolume(midiVolume);
	}

	// setup digi
	if (digiDevice != 0)
	{
   	// setup the structure for the driver that we are going to use
   	sSOSInitDriver.wSampleRate     =  _SOS_DRIVER_RATE;
   	sSOSHardwareSettings.wPort = digiPort;
   	sSOSHardwareSettings.wIRQ = digiIRQ;
   	sSOSHardwareSettings.wDMA = digiDMA;
		wError = sosDIGIInitDriver(  digiDevice,
      	&sSOSHardwareSettings, &sSOSInitDriver, &hDigiDriverHandle );
   	if(wError)
   	{
			#ifndef NDEBUG
			InitError(wError);
			#else
			InitError(wError, "Sound card init failed");
			#endif
   	}

		// set the bit for shutdown
		sosActiveSystems |= SOS_DIGI_DRIVER;

   	// register the fill handler as a timer event
   	wError = sosTIMERRegisterEvent( _TIMER_RATE,
      	sSOSInitDriver.lpFillHandler, &hTimerEventHandle );
   	if(wError)
   	{
			#ifndef NDEBUG
			InitError(wError);
			#else
			InitError(wError, "Sound card init failed");
			#endif
   	}
		// set the bit for shutdown
		sosActiveSystems |= SOS_DIGI_TIMER_EVENT;

		SetMasterDigiVolume(digiVolume);
	}

#ifdef SNDMGR_DEBUG
   printf( "\n%x", sSOSHardwareSettings.wPort );
   printf( "\n%x", sSOSHardwareSettings.wDMA );
   printf( "\n%x\n", sSOSHardwareSettings.wIRQ );
#endif

}

// This should be called for any error during the Init
// If this is an SOS error, send the error number and NULL.
// If this is a message, send zero and the message pointer. 
void 
SoundMgr::InitError(WORD wError, char *message)
{
	char	errMess[400];
	char	*pMess;
	
	if (message == NULL)
	{
		// an SOS message
   	// display the error types
   	sprintf( errMess, "\nError : %s", sosGetErrorString( wError ) );
		pMess = errMess;
	}
	else 
	{
		// just a message to display
		pMess = message;
	}

	printf(message);

	ShutDownSoundMgr();
	APrintUnfreedPtrs(FALSE);

	exit(1);
}

void
SoundMgr::Cycle(void)
{
	bool		donePlaying;

	grip		gDigiSound;
	TSound	*pDigiSound;

	grip		gMidiSong;
	TMusic	*pMidiSong;

	// only check things if the sound manager is active
	if (pSoundMgr) {
		for (gDigiSound = (grip) digiList.FirstValue();
			gDigiSound != NULL;
			gDigiSound = (grip) digiList.NextValue())
		{
			pDigiSound = ADerefAs(TSound, gDigiSound);

			// only process sounds that aren't paused
			if (!pDigiSound->pausedCnt)
			{
				donePlaying = FALSE;
 
				if (pDigiSound->waitTimer)
				{
					// no driver is loaded so we are using a timer to
					// determine when the sound should be done.

					if (pDigiSound->streaming)
 					{
						// we are streaming.  calculate the number of bytes played
						pDigiSound->bytesPlayed = (ATicks() - pDigiSound->startTicks) * pDigiSound->bytesPerTick;
					}

					if (pDigiSound->waitTimer <= ATicks())
					{
						donePlaying = TRUE;
					}
				}
				else
				{
					// we are using a driver
					if (pDigiSound->streaming)
					{
						// update the stream
						pDigiSound->GetStreamer()->Stream(); 

						if (pDigiSound->GetStreamer()->StreamDone() &&
						 	sosDIGISampleDone(hDigiDriverHandle, pDigiSound->hSOSSampleHandle))
						{
							// we are done streaming and the last sample has played
							donePlaying = TRUE;
						}
					}
					else 
					{
						// we are not streaming.
						if (sosDIGISampleDone(hDigiDriverHandle, pDigiSound->hSOSSampleHandle)) 
						{
							donePlaying = TRUE;
						}
					}
				}

				if (donePlaying)
				{
					// this sound is done	
					DeleteDigi(pDigiSound);
					if(pDigiSound->chainLength)
					{
						// next!
						donePlaying = FALSE;
						pDigiSound->PlayNextLink();
					}

	#ifdef SNDMGR_DEBUG
					printf("deleting %d\n", pDigiSound->gSelf);
	#endif
				}
				else
				{
					if (pDigiSound->fadeTotalSteps && (pDigiSound->fadeTime < ATicks()))
					{
						// we are autofading
						pDigiSound->fadeCurrentStep++;
						pDigiSound->fadeTime = ATicks() + pDigiSound->fadeStepDelay;

						if (pDigiSound->Fade(pDigiSound->fadeDir, pDigiSound->fadeCurrentStep, pDigiSound->fadeTotalSteps))
						{
							// that was the last step
							if (pDigiSound->fadeCallBack)
							{
								APostNotice(N_CUE, pDigiSound->fadeCallBack);
							}
							pDigiSound->fadeCallBack = NULL;
							pDigiSound->fadeTotalSteps = 0;
						}
					}
				}
			}
		}

		for (gMidiSong = (grip) midiList.FirstValue();
			gMidiSong != NULL;
			gMidiSong = (grip) midiList.NextValue())
		{
			pMidiSong = ADerefAs(TMusic, gMidiSong);

			// only process songs that aren't paused
			if (!pMidiSong->pausedCnt)
			{
				if (sosMIDISongDone(pMidiSong->wSongHandle))
				{
					// this song is done.
					DeleteMidi(pMidiSong);

	#ifdef SNDMGR_DEBUG
					printf("deleting %d\n", pMidiSong->gSelf);
	#endif
				}
				else
				{
					if (pMidiSong->fadeTotalSteps && (pMidiSong->fadeTime < ATicks()))
					{
						// we are autofading
						pMidiSong->fadeCurrentStep++;
						pMidiSong->fadeTime = ATicks() + pMidiSong->fadeStepDelay;

						if (pMidiSong->Fade(pMidiSong->fadeDir, pMidiSong->fadeCurrentStep, pMidiSong->fadeTotalSteps))
						{
							// that was the last step
							if (pMidiSong->fadeCallBack)
							{
								APostNotice(N_CUE, pMidiSong->fadeCallBack);
							}
							pMidiSong->fadeCallBack = NULL;
							pMidiSong->fadeTotalSteps = 0;
						}
					}
				}
			}
		}
	}
}

void
SoundMgr::Pause(void)
{
	grip		gDigiSound;
	grip		gMidiSong;

	for (gDigiSound = (grip) digiList.FirstValue();
		gDigiSound != NULL;
		gDigiSound = (grip) digiList.NextValue())
	{
		ADerefAs(TSound, gDigiSound)->Pause();
	}

	for (gMidiSong = (grip) midiList.FirstValue();
		gMidiSong != NULL;
		gMidiSong = (grip) midiList.NextValue())
	{
		ADerefAs(TMusic, gMidiSong)->Pause();
	}
}

void
SoundMgr::Resume(void)
{
	grip		gDigiSound;
	grip		gMidiSong;

	for (gDigiSound = (grip) digiList.FirstValue();
		gDigiSound != NULL;
		gDigiSound = (grip) digiList.NextValue())
	{
		ADerefAs(TSound, gDigiSound)->Resume();
	}

	for (gMidiSong = (grip) midiList.FirstValue();
		gMidiSong != NULL;
		gMidiSong = (grip) midiList.NextValue())
	{
		ADerefAs(TMusic, gMidiSong)->Resume();
	}
}

// Set the volume for all music and sounds to be the 
// fraction of total volume as established by the current step.
// the first current step should be 1 (not 0).  the last step
// should equal totalSteps.
// If the last step, TRUE is returned.

bool
SoundMgr::Fade(uint16 direction, uint32 currentStep, uint32 totalSteps)
{
	bool	done = FALSE;
	grip	gDigiSound;
	grip	gMidiSong;

	for (gDigiSound = (grip) digiList.FirstValue();
		gDigiSound != NULL;
		gDigiSound = (grip) digiList.NextValue())
	{
		done = ADerefAs(TSound, gDigiSound)->Fade(direction, currentStep, totalSteps);
	}

	for (gMidiSong = (grip) midiList.FirstValue();
		gMidiSong != NULL;
		gMidiSong = (grip) midiList.NextValue())
	{
		done = ADerefAs(TMusic, gMidiSong)->Fade(direction, currentStep, totalSteps);
	}

	return done;
}

// Setup a fade that the SoundMgr will take care of.
// stepDelay is in ticks.  
// totalSteps must be greater than 0
void
SoundMgr::AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay, grip callBack)
{
	grip		gDigiSound;
	grip		gMidiSong;

	// we only set callBack on the first sound/music that we set,
	// so that the cue only happens once.

	for (gDigiSound = (grip) digiList.FirstValue();
		gDigiSound != NULL;
		gDigiSound = (grip) digiList.NextValue())
	{
		ADerefAs(TSound, gDigiSound)->AutoFade(direction, totalSteps, stepDelay, callBack);
		// only set the callback for the first one
		callBack = NULL;
	}

	for (gMidiSong = (grip) midiList.FirstValue();
		gMidiSong != NULL;
		gMidiSong = (grip) midiList.NextValue())
	{
		ADerefAs(TMusic, gMidiSong)->AutoFade(direction, totalSteps, stepDelay, callBack);
		// only set the callback for the first one
		callBack = NULL;
	}
}

void
SoundMgr::AddDigi(TSound *pSound, int sndNum, grip callBack)
{
	uint16		i;
	bool			dummyDriver = FALSE;
	uchar 		*pWavFile;
	uint32		sampleRate;
	WaveFileHeader	waveFileHeader;
	grip			gSound;

	if (!(sosActiveSystems & SOS_DIGI_DRIVER))
	{
		// the digi driver hasn't been inited.  	
		if (callBack || pSound->streaming)
		{
			// we're supposed to cue, so setup a timer
			dummyDriver = TRUE;

			if (pSound->streaming)
			{
				// adjust stream play size to conserve memory 
				// we really only need enough to read the wave file header
				pSound->streamPlaySize = sizeof(WaveFileHeader);
			}
		}
		else
		{
			// since we aren't going to cue, no need to even start
			return;
		}
	}

	ResMgrDebug.Out("SndMgr:AddDigi(%x %d g%d)", (void *)pSound, sndNum, callBack);
	if (digiList.Find((void*) pSound->gSelf))
	{
		// this sound is in the play list right now.
		// shut it down
		ResMgrDebug.Out(" kill g%d", pSound->gSelf);
#ifdef SNDMGR_DEBUG
		printf("killing to start %d\n", pSound->gSelf);
#endif
		DeleteDigi(pSound);
	}

	// now we set a few things in the TSound variables
	// (these couldn't be set before because of the possible
	//  call to DeleteDigi)
	pSound->resourceNum = sndNum;
	pSound->gToCue = callBack;
	pSound->cueData = NULL;

	while(digiList.count >= MAX_DIGI_SOUNDS)
	{
		// delete the oldest sound.
		// since sounds are always added to the end of the list,
		// the first one in the list is the oldest
#ifdef SNDMGR_DEBUG
		printf("killing %d\n", ADerefAs(TSound, (grip) digiList.FirstValue())->gSelf);
#endif
		gSound = (grip)digiList.FirstValue();
		ResMgrDebug.Out(" kill2 g%d", pSound->gSelf);
		DeleteDigi(ADerefAs(TSound, gSound));
	}

	// load the sample
	// our sample is in WAV format.

	// clear the sample data structure
	memset(&pSound->sSOSSampleData, 0, sizeof(_SOS_START_SAMPLE));

	if (pSound->streaming)
	{
		// we are streaming.  start the stream and get the header

		// adjust stream play size to conserve memory 
		// we really only need enough to read the wave file header
		// (we will adjust this be optimal later)
		pSound->streamPlaySize = sizeof(WaveFileHeader);

		ResMgrDebug.Out(" stream ");
		pSound->GetStreamer()->Open(RES_DAC, pSound->resourceNum, sizeof(WaveFileHeader), sizeof(WaveFileHeader));
		pSound->GetStreamer()->GetBytes((char *) &waveFileHeader, sizeof(WaveFileHeader));
	}
	else
	{
		ResMgrDebug.Out(" load ");

		// load the whole resource
		pSound->gResource = ALoad(RES_DAC, pSound->resourceNum);
		ADerefAs(Resource, pSound->gResource)->Lock();
		pWavFile = AGetResData(pSound->gResource);
		memcpy(&waveFileHeader, pWavFile, sizeof(WaveFileHeader));

		// point pWavFile past the header
		pWavFile += sizeof(WaveFileHeader);
	}

	char 	riffName[4] = {'R','I','F','F'};
	char	mess[200];

	// make sure that this is a WAV file
	for(i = 0; i < 4; i++)
	{
		if (waveFileHeader.riffStr[i] != riffName[i])
		{
			// not a match
			sprintf(mess, "Not a WAV file: %d", pSound->resourceNum);
			APanic(mess);
		}
	}

	// get the sample rate
	sampleRate = waveFileHeader.waveFormat.nSamplesPerSec;

	// save the # of bytes per samples
	// (2 bytes for 16 bit data) (double for stereo)
	pSound->bytesPerSample = waveFileHeader.waveFormat.nBlockAlign;

	// kludge for SOS version 3. BUG BUG   
	// pitch shifted samples always play 1 byte too many.
	// DON'T MESS WITH STEREO SOUNDS
	if (sampleRate != _SOS_DRIVER_RATE &&
		 pSound->bytesPerSample == 1)
	{
		waveFileHeader.dataLength--;
	}

	// setup the sample data structure for sos
	// (and maybe set playSize)
	pSound->bytesPlayed = 0L; 
	if (pSound->streaming)
	{
		ResMgrDebug.Out(" stream2");
		if (dummyDriver)
		{
			pSound->sSOSSampleData.dwSampleSize = waveFileHeader.dataLength;
		}
		else
		{
			// set the playsize.

			// since we are actually going to play the sound, adjust
			// the playSize to be optimal given the wav header info.
			// set the playsize.  we want to have approx 60 samples per sec.

			pSound->streamPlaySize = ((float) sampleRate / 60) + 0.5;

			// adjust the playSize for stereo and number of bits per Second
			pSound->streamPlaySize *= pSound->bytesPerSample;

			// shut down the old streamer
			pSound->GetStreamer()->Close();
			
			// restart with the new play size value
			pSound->GetStreamer()->Open(RES_DAC, pSound->resourceNum, waveFileHeader.dataLength, 
					pSound->streamPlaySize * STREAM_BUF_MULTIPLIER, 0, sizeof(WaveFileHeader), pSound->streamPlaySize);

			// make sure that the current buffer offset is zero (for speed)
			pSound->GetStreamer()->ZeroOffsetBufs();

			// The streamer has it's own buffers.  Since the
			// chunk that we need may span the streamer's buffers,
			// we will allocate our own buffer.
			TRACK_MEM("Snd_StreamBuf");
			pSound->gStreamBuf = AMalloc(pSound->streamPlaySize); 
			pSound->sSOSSampleData.lpSamplePtr = (LPSTR) ADerefAs(char, pSound->gStreamBuf);

			// load the streaming buffer for the first time
			pSound->bytesPlayed = 
				pSound->GetStreamer()->GetBytes(ADerefAs(char, pSound->gStreamBuf), pSound->streamPlaySize);

			pSound->sSOSSampleData.dwSampleSize = pSound->bytesPlayed;

			// setup the pointer to the routine that will continue the streaming
   		pSound->sSOSSampleData.lpCallback = TSoundStreamCallback;
		}
	}
	else 
	{
		pSound->sSOSSampleData.lpSamplePtr = (LPSTR) pWavFile;
		pSound->sampleSize = pSound->sSOSSampleData.dwSampleSize = waveFileHeader.dataLength;
	}

	if (pSound->sSOSSampleData.dwSampleSize < 1)
	{
		// something is wrong.
		// let's get out of here
		ResMgrDebug.Out(" kill3 g%d", pSound->gSelf);
		DeleteDigi(pSound, FALSE);
		return;
	}

	if (sampleRate > 22050)
	{
		sprintf(mess, "Sample rate too high: %d", sampleRate);
		APanic(mess);
	}

	// any sample rate besides 22050 is handled through pitch shifting
	if (sampleRate != 22050)
	{
		pSound->sSOSSampleData.wSampleFlags |= _PITCH_SHIFT;
		// find the fraction of the whole and put that in samplePitchAdd
		pSound->sSOSSampleData.dwSamplePitchAdd = ((float) sampleRate / _SOS_DRIVER_RATE) * 0x10000;	
	}

	// check to see if we are looping.  don't allow looping if we are streaming
	if (pSound->loopCount != 1 && !pSound->streaming)
	{
		// looping more than once.  setup
		pSound->sSOSSampleData.wSampleFlags |= _LOOPING;
		pSound->sSOSSampleData.wLoopCount = (int16) (pSound->loopCount - 1);
	}

	// check to see if this is stereo or not
	if (waveFileHeader.waveFormat.nChannels == 2)
	{
		// stereo 
		if (sampleRate != 22050)
		{
			sprintf(mess, "Stereo samples must be at 22050.  File %d", pSound->resourceNum);
			APanic(mess);
		}
		else
		{
			pSound->sSOSSampleData.wChannel = _INTERLEAVED;
		}
	}
	else
	{
		// mono	
		pSound->sSOSSampleData.wChannel = _CENTER_CHANNEL;
		pSound->sSOSSampleData.wSampleFlags |= _PANNING;
		pSound->sSOSSampleData.wSamplePanLocation = GetPanPosition(pSound->panPosition);
	}

	ResMgrDebug.Out(" AD1");
	// save a few things
	if (pSound->pausedCnt)
	{
		// paused.  have the sound start at the paused time
		pSound->startTicks = pSound->pausedTime;
	}
	else
	{
		pSound->startTicks = ATicks();
	}
	pSound->bytesPerTick = (((float) sampleRate / TICKS_PER_SEC) + 0.5) * pSound->bytesPerSample; 

	// This flag MUST always be set to allow MasterDigiVolume to work.
	pSound->sSOSSampleData.wSampleFlags |= _VOLUME;

	pSound->sSOSSampleData.wVolume = (uint16) (pSound->volume << 8);

	ResMgrDebug.Out(" AD2");
	// Since each sample needs a unique id, let's use the grip number.
	// If we used the sound number, we wouldn't be able to play a sound
	// multiple times and the same time (gun shots, etc).
	// Also, streaming MUST have this so that it can know which
	// sound to use (see TSoundStreamCallback).
	pSound->sSOSSampleData.wSampleID = pSound->gSelf;
	
	// find out what the time will be when the sample is done
	// (This is always done, because a restore game may need to
	//  switch between having a sound driver and not having a sound driver)

	pSound->playTimer = pSound->startTicks +
 		((((float) waveFileHeader.dataLength / pSound->bytesPerSample) / sampleRate) * TICKS_PER_SEC);

   // start the sample playing
	if (dummyDriver)
	{
		// a driver isn't loaded.  setup the timer based
		// on the play time of the sample
		pSound->waitTimer = pSound->playTimer;

		if (pSound->streaming)
		{
			// shut down the streamer now
			pSound->GetStreamer()->CloseSoundFile();
		}
	}
	else 
	{
		// really play the sound
		pSound->waitTimer = 0L;
		if (!pSound->pausedCnt)
		{
			// if not paused, start the sound
	   	pSound->hSOSSampleHandle = sosDIGIStartSample( hDigiDriverHandle, &pSound->sSOSSampleData );
		}
	}

#ifdef SNDMGR_DEBUG
	printf("adding %d\n", pSound->gSelf);
#endif

	// add to our list
	digiList.Add((void*) (pSound->gSelf));
	ResMgrDebug.Out(" done\n");
}

void
SoundMgr::DeleteDigi(TSound *pSound, bool cue)
{
	if (cue && pSound->gToCue)
	{
		APostNotice(N_CUE, pSound->gToCue, (void *)(pSound->cueData));
	}

	if (sosActiveSystems & SOS_DIGI_DRIVER)
	{
		// digi driver is active
   	sosDIGIStopSample( hDigiDriverHandle, pSound->hSOSSampleHandle );
	}

	// free the sample resource
	if (pSound->gResource)
	{
		(ADerefAs(Resource, pSound->gResource))->Unlock();
		pSound->gResource = 0;
	}

	// Close down the streamer stuff.  It's fine to call this
	// when not streaming.
	pSound->GetStreamer()->CloseSoundFile();

	if (pSound->gStreamBuf)
	{
		// we have a streaming buffer.  free it.
		AFree(pSound->gStreamBuf);
	}

	digiList.Delete((void*) pSound->gSelf);
}

void
SoundMgr::AddMidi(TMusic *pMusic, uint16 sndNum, grip callBack)
{
	if (!(sosActiveSystems & SOS_MIDI_SYS_DRIVER))
	{
		// the midi driver hasn't been inited.  	
		// we can't play, so cue if we're supposed to
		if (callBack)
		{
			APostNotice(N_CUE, callBack);
		}

		// let's get outta here before something bad happens
		return;
	}

	if (midiList.Find((void*) pMusic->gSelf))
	{
		// this song is in the play list right now.
		// shut it down
		#ifdef SNDMGR_DEBUG
			printf("killing to start %d\n", pMusic->gSelf);
		#endif
		DeleteMidi(pMusic);
	}

	// now we set a few things in the TMusic variables
	// (these couldn't be set before because of the possible
	//  call to DeleteMidi)
	pMusic->resourceNum = sndNum;
	pMusic->gToCue = callBack;

	while(midiList.count >= MAX_MIDI_SONGS)
	{
		// delete the oldest sound.
		// since sounds are always added to the end of the list,
		// the first one in the list is the oldest
		#ifdef SNDMGR_DEBUG
			printf("killing %d\n", ADerefAs(TMusic, (grip) midiList.FirstValue())->gSelf);
		#endif
		DeleteMidi(ADerefAs(TMusic, (grip) midiList.FirstValue()));
	}

	// load the song

	pMusic->gResource = ALoad(RES_MIDI, pMusic->resourceNum);
	(ADerefAs(Resource, pMusic->gResource))->Lock();

   // setup the song initialization structure
   sSOSInitSong.lpSongData = (LPSTR) AGetResData(pMusic->gResource);
   sSOSInitSong.lpSongCallback = _NULL;

   // initialize the song
   wError = sosMIDIInitSong( &sSOSInitSong, &sSOSTrackMap, &pMusic->wSongHandle );
   if(wError)
   {
		#ifndef NDEBUG
		InitError(wError);
		#else
		InitError(wError, "sound card digi failure");
		#endif
   }
 
   // start the song playing
	if (!pMusic->pausedCnt)
	{
		// if not paused, start the song
   	wError = sosMIDIStartSong( pMusic->wSongHandle );
   	if(wError)
   	{
			#ifndef NDEBUG
			InitError(wError);
			#else
			InitError(wError, "sound card digi failure");
			#endif
   	}

		_disable();
		sosMIDISetSongVolume((WORD) pMusic->wSongHandle, (BYTE) pMusic->volume);
		_enable();
	}

	// add to our list
	midiList.Add((void*) (pMusic->gSelf));
	#ifdef SNDMGR_DEBUG
		printf("adding %d\n", pMusic->gSelf);
	#endif
}

void
SoundMgr::DeleteMidi(TMusic *pMusic, bool cue)
{
	if (cue && pMusic->gToCue)
	{
		APostNotice(N_CUE, pMusic->gToCue);
	}

	if (sosActiveSystems & SOS_MIDI_SYS_DRIVER)
	{
		// midi driver was is active

		// stop the MIDI song from playing
   	sosMIDIStopSong(pMusic->wSongHandle);

		// uninitialize the MIDI song
		sosMIDIUnInitSong(pMusic->wSongHandle);
	}

	// free the sample resource
	if (pMusic->gResource)
	{
		(ADerefAs(Resource, pMusic->gResource))->Unlock();
		pMusic->gResource = 0;
	}

	midiList.Delete((void*) pMusic->gSelf);
}

void
SoundMgr::SetMasterDigiVolume(int16 volume)
{
	if (!(sosActiveSystems & SOS_DIGI_DRIVER))
	{
		// midi driver was never inited
		return;
	}

	// make sure that this is a valid volume
	if (volume < 0)
	{
		volume = 0;
	}
	else
	{
		if (volume > MAX_VOLUME)
		{
			volume = MAX_VOLUME;
		}
	}

	digiVolumeMax = digiVolume = volume;
	wError = sosDIGISetMasterVolume(hDigiDriverHandle, (digiVolume << 8));	
   if (wError)
   {
      // display the error string and exit
		char	errMess[400];
      sprintf(errMess, "\nError : %s", sosGetErrorString( wError ) );
      APanic(errMess);
   }
}

int16
SoundMgr::GetMasterDigiVolume(void)
{
	return digiVolume;
}

void
SoundMgr::SetMasterMidiVolume(int16 volume)
{
	if (!(sosActiveSystems & SOS_MIDI_SYS_DRIVER))
	{
		// midi driver was never inited
		return;
	}

	// make sure that this is a valid volume
	if (volume < 0)
	{
		volume = 0;
	}
	else
	{
		if (volume > MAX_VOLUME)
		{
			volume = MAX_VOLUME;
		}
	}

	midiVolumeMax = midiVolume = volume;
	_disable();
	wError = (WORD) sosMIDISetMasterVolume((BYTE) midiVolume);	
	_enable();
   if (wError)
   {
      // display the error string and exit
		char	errMess[400];
      sprintf(errMess, "\nError : %s", sosGetErrorString( wError ) );
      APanic(errMess);
   }
}

int16
SoundMgr::GetMasterMidiVolume(void)
{
	return midiVolume;
}

bool
SoundMgr::SystemIsActive(uint16 flags)
{
	return (sosActiveSystems & flags) ? TRUE : FALSE;
}

// return TRUE if the digi sound is in the play list
bool
SoundMgr::DigiIsPlaying(TSound *pSound)
{
	return (bool) (digiList.Find((void*) pSound->gSelf));
}

// return TRUE if the midi song is in the play list
bool
SoundMgr::MidiIsPlaying(TMusic *pMusic)
{
	return (bool) (midiList.Find((void*) pMusic->gSelf));
}

uint16
SoundMgr::NumberDigiPlaying(void)
{
	return digiList.count;
}

uint16
SoundMgr::NumberMidiPlaying(void)
{
	return midiList.count;
}

grip
SoundMgr::NextOldestDigiPlaying(void)
{
	return ADerefAs(TSound, (grip) digiList.NextValue())->gSelf;
}

grip
SoundMgr::OldestDigiPlaying(void)
{
	return ADerefAs(TSound, (grip) digiList.FirstValue())->gSelf;
}

grip
SoundMgr::OldestMidiPlaying(void)
{
	return ADerefAs(TMusic, (grip) midiList.FirstValue())->gSelf;
}

bool
SoundMgr::Save(uint16 state, FILE *fp)
{
	bool		error = FALSE;
	grip		gMgr = 0;
	SoundMgr	*pTempMgr;
	grip		gOldListData;
	grip		gListData;
	grip		gDigiSound;
	TSound	*pDigiSound;

	grip		gMidiSong;
	TMusic	*pMidiSong;

	switch(state)
	{
		case BEFORE_SAVE:
			// pause all sounds
			Pause();
			break;

		case DURING_SAVE:
			// write out sound mgr
			if (fwrite(this, 1, sizeof(SoundMgr), fp) != sizeof(SoundMgr))
			{
				// some sort of error
				error = TRUE;
				break;
			}

			// write out the sound list
			if (ASaveGripNum(digiList.gData, fp))
			{
				// some sort of error
				error = TRUE;
				break;
			}

			// write out the midi list
			if (ASaveGripNum(midiList.gData, fp))
			{
				// some sort of error
				error = TRUE;
				break;
			}
			break;

		case AFTER_SAVE:
			// resume all sounds
			Resume();
			break;

		case BEFORE_RESTORE:
			// stop all sounds
			for (gDigiSound = (grip) digiList.FirstValue();
				gDigiSound != NULL;
				gDigiSound = (grip) digiList.NextValue())
			{
				pDigiSound = ADerefAs(TSound, gDigiSound);

				// delete this sound with no cue
				DeleteDigi(pDigiSound, FALSE);
			}

			// stop all songs
			for (gMidiSong = (grip) midiList.FirstValue();
				gMidiSong != NULL;
				gMidiSong = (grip) midiList.NextValue())
			{
				pMidiSong = ADerefAs(TMusic, gMidiSong);

				// delete this sond with no cue
				DeleteMidi(pMidiSong, FALSE);
			}
			break;

		case DURING_RESTORE:
			// read old soundmgr into temp
			TRACK_MEM("TmpSndMgr");	gMgr = AMalloc(sizeof(SoundMgr));
			pTempMgr = ADerefAs(SoundMgr, gMgr);

			// run in a while loop so that we can break on an error
			// and clean up properly
			while(TRUE)
			{
				// read the graph manager
				if (fread(pTempMgr, 1, sizeof(SoundMgr), fp) != sizeof(SoundMgr))
				{
					// some sort of error
					error = TRUE;
					break;
				}

				// restore the digi and midi List classes
				memcpy(&pSoundMgr->digiList, &pTempMgr->digiList, sizeof(SysList) - sizeof(void **));
				memcpy(&pSoundMgr->midiList, &pTempMgr->midiList, sizeof(SysList) - sizeof(void **));

				// read the list data 
				gOldListData = pSoundMgr->digiList.gData;
				AFree(gOldListData);
				ARestoreGripNum(&gListData, fp);
				pSoundMgr->digiList.gData = gListData;

				gOldListData = pSoundMgr->midiList.gData;
				AFree(gOldListData);
				ARestoreGripNum(&gListData, fp);
				pSoundMgr->midiList.gData = gListData;

				break;
			}

			// if we allocated memory, free it
			if (gMgr)
			{
				AFree(gMgr);
			}

			break;

		case AFTER_RESTORE:
			uint32	bytesRetrieved;

			// setup all sounds and music to be able to restart when Resume is
			// called.
			// if a digi driver was present during save, but not
			// present during restore, convert sounds to be timer based.
			//
			// if a digi driver was not present during save, but is
			// present during restore, leave on a timer.  I suppose
			// this could be changed to start the sound in progress,
			// but I don't have the time right now.

			// setup sounds
			for (gDigiSound = (grip) digiList.FirstValue();
				gDigiSound != NULL;
				gDigiSound = (grip) digiList.NextValue())
			{
				pDigiSound = ADerefAs(TSound, gDigiSound);

				if (!(sosActiveSystems & SOS_DIGI_DRIVER))
 				{
					// no sound driver present
					if (pDigiSound->waitTimer == 0)
					{
						// this must be a sound that was launched when
						// a sound driver was active.  see if we need a timer.

						if (pDigiSound->gToCue)
						{
							// we need to run a timer
							pDigiSound->waitTimer = pDigiSound->playTimer;

							// we could also flush the resource here, but since
							// the way that we do that may change very soon,
							// we'll just let DeleteDigi handle that.
						}
						else
						{
							if (pDigiSound->streaming)
							{
								// we need to run a timer

								// shut down the old streamer stuff
								// the file pointer from the save will be
								// invalid.  Null it.
								pDigiSound->GetStreamer()->ClearFilePtr();
								pDigiSound->GetStreamer()->Close();

								pDigiSound->waitTimer = pDigiSound->playTimer;
							}
							else
							{
								// no need to keep this around
								digiList.Delete((void*) pDigiSound->gSelf);
							}
						}
					}
				}
				else
				{
					// sound driver present
					
					if (pDigiSound->waitTimer == 0)
					{
						if (pDigiSound->streaming)
						{
							// we need to setup the streamer again
							// the file pointer from the save will be
							// invalid.  Null it.
							pDigiSound->GetStreamer()->ClearFilePtr();
							// init the streamer and move the file pointer to where we left
							// off at.
							pDigiSound->GetStreamer()->ReInit(pDigiSound->bytesPlayed + sizeof(WaveFileHeader));

							pDigiSound->sSOSSampleData.lpSamplePtr = (LPSTR) ADerefAs(char, pDigiSound->gStreamBuf);

      					// load the play buffer
      					bytesRetrieved =
								pDigiSound->GetStreamer()->GetBytes(ADerefAs(char, pDigiSound->gStreamBuf),
									pDigiSound->streamPlaySize);

							pDigiSound->bytesPlayed += bytesRetrieved;
							pDigiSound->sSOSSampleData.dwSampleSize = bytesRetrieved; 

							// setup the pointer to the routine that will continue the streaming
   						pDigiSound->sSOSSampleData.lpCallback = TSoundStreamCallback;
						}
					}
				}
			}

			// setup songs
			for (gMidiSong = (grip) midiList.FirstValue();
				gMidiSong != NULL;
				gMidiSong = (grip) midiList.NextValue())
			{
				pMidiSong = ADerefAs(TMusic, gMidiSong);

				pMidiSong->gResource = ALoad(RES_MIDI, pMidiSong->resourceNum);

   			// setup the song initialization structure
   			sSOSInitSong.lpSongData = (LPSTR) AGetResData(pMidiSong->gResource);
   			sSOSInitSong.lpSongCallback = _NULL;

   			// initialize the song
   			wError = sosMIDIInitSong( &sSOSInitSong, &sSOSTrackMap, &pMidiSong->wSongHandle );
   			if(wError)
   			{
					InitError(wError);
   			}

 				// start the song
   			wError = sosMIDIStartSong( pMidiSong->wSongHandle );
   			if(wError)
   			{
					InitError(wError);
   			}

				_disable();
				sosMIDISetSongVolume((WORD) pMidiSong->wSongHandle, (BYTE) pMidiSong->volume);

				// pause the song
				sosMIDIPauseSong(((TMusic *) ADeref(gMidiSong))->wSongHandle, TRUE);
				_enable();
			}

			Resume();

			break;
	}
	return error;
}

// do everything that is necessary to shut down the sound manager
void
ShutDownSoundMgr(void)
{
	ShutDownSoundMgrNow(pSoundMgr);	
}

// do everything that is necessary to shut down the sound manager
void
ShutDownSoundMgrNow(SoundMgr *pSndMgr)
{
	if (pSndMgr)
	{
		pSndMgr->ShutDown();
	}
}

void
SoundMgr::ShutDown(void)
{
	grip		gDigiSound;
	TSound	*pDigiSound;

	grip		gMidiSong;
	TMusic	*pMidiSong;

	if (sosActiveSystems)
	{
		// the sound manager is here and some systems are active.
		// close things down.

		// stop all sounds
		for (gDigiSound = (grip) digiList.FirstValue();
			gDigiSound != NULL;
			gDigiSound = (grip) digiList.NextValue())
		{
			pDigiSound = ADerefAs(TSound, gDigiSound);

			// delete this sound with no cue
			DeleteDigi(pDigiSound, FALSE);
		}

		// stop all songs
		for (gMidiSong = (grip) midiList.FirstValue();
			gMidiSong != NULL;
			gMidiSong = (grip) midiList.NextValue())
		{
			if(pMemMgr->CheckGrip(gMidiSong) != GRIP_VALID)
			{
				pMono->Out("SoundMgr::ShutDown() WARNING! gMidiSong %d invalid!\n",
					gMidiSong);
				continue;
			}

			pMidiSong = ADerefAs(TMusic, gMidiSong);

			// delete this sond with no cue
			DeleteMidi(pMidiSong, FALSE);
		}

		if (sosActiveSystems & SOS_DETECT)
		{
			sosDIGIDetectUnInit();	
		}

		if (sosActiveSystems & SOS_DIGI_TIMER_EVENT)
		{
   		// remove the fill handler event from the timer
   		sosTIMERRemoveEvent(hTimerEventHandle );
		}

		if (sosActiveSystems & SOS_DIGI_DRIVER)
		{
			sosDIGIUnInitDriver( hDigiDriverHandle, _FALSE, _TRUE );
			hDigiDriverHandle = SMACKSOUNDNONE;
			sSOSInitDriver.wAllocateBuffer = _FALSE;
			sSOSInitDriver.lpDriverMemory = (LPSTR) _NULL;
			sSOSInitDriver.lpTimerMemory = (LPSTR) _NULL;
			sSOSInitDriver.lpDriverMemoryCS = (LPSTR) _NULL;
			sSOSInitDriver.lpTimerMemoryCS = (LPSTR) _NULL;
			sSOSInitDriver.lpFillHandler = _NULL;
		}

		if (sosActiveSystems & SOS_DIGI_SYS)
		{
			sosDIGIUnInitSystem();	
		}

		if (sosActiveSystems & SOS_MIDI_SYS_DRIVER)
		{
			// reset the midi driver
			sosMIDIResetDriver(hMidiDriverHandle);

			// uninitialize the midi driver and tell it
			// not to free the memory allocated (in case we reinit)
//      	sosMIDIUnInitDriver(hMidiDriverHandle, _FALSE );
      	sosMIDIUnInitDriver(hMidiDriverHandle, _TRUE );
		}

		if (sosActiveSystems & SOS_MIDI_SYS)
		{
			sosMIDIUnInitSystem();	
		}

		if (sosActiveSystems & SOS_TIMER)
		{
			sosTIMERUnInitSystem(0);	
		}

		// clean up all the loaded resources
		if (gInstruments)
		{
			AFlush(gInstruments);
			gInstruments = 0;
		}
		if (gDrums)
		{
			AFlush(gDrums);
			gDrums = 0;
		}

		if (gDigiInstruments)
		{
			AFlush(gDigiInstruments);
			gDigiInstruments = 0;
		}

		if (gMT32Patch)
		{
			AFlush(gMT32Patch);
			gMT32Patch = 0;
		}

		sosActiveSystems = 0;

		// set the devices to off
		digiDevice = midiDevice = SOUND_OFF;
	}
}

//-----------------------------------------------------------------------
//
// SOS_Base
//
//-----------------------------------------------------------------------

SOS_Base::SOS_Base(void)
{
	gResource = 0;	
	pausedCnt = 0;
	fadeTotalSteps = 0;
	fadeCallBack = NULL;
}

// Setup a fade that the SoundMgr will take care of.
// stepDelay is in ticks.  
// totalSteps must be greater than 0
void
SOS_Base::AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay, grip callBack)
{
	fadeDir = direction;

	if (totalSteps < 1)
	{
		totalSteps = 1;
	}
	fadeTotalSteps = (uint16) totalSteps;

	fadeCurrentStep = 1;

//	if (stepDelay < 0)
//	{
//		stepDelay = 0;
//	}
	fadeStepDelay = stepDelay;

	fadeCallBack = callBack;
	fadeTime = ATicks() + fadeStepDelay;

	if (Fade(fadeDir, fadeCurrentStep, fadeTotalSteps))
	{
		// that was the last step
		if (fadeCallBack)
		{
			APostNotice(N_CUE, fadeCallBack);
		}
		fadeCallBack = NULL;
		fadeTotalSteps = 0;
	}
}

//-----------------------------------------------------------------------
//
// TSound
//
//-----------------------------------------------------------------------


TSound::TSound(void)
{
	waitTimer = 0L;
	streaming = FALSE;
	gStreamBuf = 0;
	panPosition = DIGI_PAN_CENTER;

	// This is how many bytes will be played before the stream
	// is continued.  The default number is what the lip-syncing
	// currently requires.  If you want to change this, change it
	// before you call Play().
	streamPlaySize = 184 * 2;
	
	bytesPerSample = 1;
	bytesPlayed = 0;

	// init the sample data structure to all zeros
	memset(&sSOSSampleData, 0, sizeof(_SOS_START_SAMPLE));
}


TSound::~TSound(void)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		if (pSoundMgr->DigiIsPlaying(this))
		{
			// the sound is still in the play list.
			// shut it down.  don't cue
			pSoundMgr->DeleteDigi(this, FALSE);
		}
	}
}

void
TSound::AddToChain(int resNum)
{
	if(chainLength >= CHAIN_MAX)
		return;
	resChain[chainLength++] = resNum;
}

void
TSound::PlayNextLink(void)
{
	int resNum, tempChainLength;

	if(!chainLength)
		return;
	resNum = resChain[0];
	memcpy(&resChain[0], &resChain[1], sizeof(resChain) - sizeof(resChain[0]));
	tempChainLength = --chainLength;
	if(resNum)
		Play(resNum, volume, panPosition, gToCue, loopCount, streaming);
	chainLength = tempChainLength;
}

void
TSound::Play(int sndNum, int16 sVolume, int panPos, grip callBack, 
				int16 loopCnt, bool streamIt)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		if (sVolume < 0 ||
			 sVolume > MAX_VOLUME)
		{
			// set the volume to loudest
			sVolume = MAX_VOLUME;
		}
		volumeMax = volume = sVolume;

		loopCount = loopCnt;
		streaming = streamIt;
		panPosition = panPos;

		pSoundMgr->AddDigi(this, sndNum, callBack);
		cueData = NULL;
		chainLength = 0;
	}
}

void
TSound::SetVolume(int16 sVolume, int panPos, bool updateMax)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		SetPanPosition(panPos);

		if (sVolume < 0 ||
			 sVolume > MAX_VOLUME)
		{
			// set the volume to loudest
			sVolume = MAX_VOLUME;
		}
		volume = sVolume;
		if (updateMax)
		{
			volumeMax = volume;
		}

		sSOSSampleData.wVolume = (uint16) (volume << 8);
		sSOSSampleData.wSampleFlags |= _VOLUME;

		// only call sos stuff if digi driver has been inited
		if (pSoundMgr->SystemIsActive(SOS_DIGI_DRIVER))
		{
			sosDIGISetSampleVolume(pSoundMgr->hDigiDriverHandle, hSOSSampleHandle, (volume << 8));
		}
	}
}

void
TSound::Stop(void)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		if (pSoundMgr->DigiIsPlaying(this))
		{
			// if we are saying to stop, don't cue
			pSoundMgr->DeleteDigi(this, FALSE);
		}
		chainLength = 0;
	}
}


int
TSound::IsPlaying(void)
{
	int	isPlaying = 0; 

	if (pSoundMgr)
	{
		if (pSoundMgr->DigiIsPlaying(this))
		{
			isPlaying = resourceNum;	
		}
		else
		{
			isPlaying = 0;	
		}
	}

	return isPlaying;
}

void
TSound::Pause(void)
{
	pausedCnt++;	

	// only do the follow stuff if this is the first time that
	// we are going to pause
	if (pausedCnt == 1)
	{
		// save the paused ticks so so we can fix time based things in resume
		pausedTime = ATicks();

		// if the digi driver is active, pause digi
		if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_DIGI_DRIVER)))
		{
			// if we are using a timer, we aren't really playing a sample.
			// only shut down samples that are really playing
			if (waitTimer == 0)
			{
				// streaming samples keep track of bytesPlayed, so don't
				// set bytesPlayed for them.
				if (streaming == FALSE)
				{
					if (sSOSSampleData.wSampleFlags & _LOOPING)
					{
						// looping sound.  restart at the beginning when Resume
						// is called
						bytesPlayed = 0;
					}
					else
					{
						// single play sound
						bytesPlayed += sosDIGIGetBytesProcessed( pSoundMgr->hDigiDriverHandle, hSOSSampleHandle );
					}
				}

	   		sosDIGIStopSample( pSoundMgr->hDigiDriverHandle, hSOSSampleHandle );
			}
		}
	}
}

void
TSound::Resume(void)
{
	ticks_t	ticksDiff;
	if (pausedCnt)
	{
		pausedCnt--;

		// only unpause things when we are completely unpaused
		if (pausedCnt < 1)
		{
			pausedCnt = 0;

			ticksDiff = ATicks() - pausedTime;

			// if the digi driver is active, pause digi
			if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_DIGI_DRIVER)))
			{
				if (waitTimer == 0)
				{
					// if we are streaming, everything is already set to go
					if (streaming == FALSE)
					{
						// must be a non-streaming sample
						// fast forward the pointer and adjust a few things
						sSOSSampleData.lpSamplePtr = (LPSTR) (AGetResData(gResource) + sizeof(WaveFileHeader) + bytesPlayed);
						sSOSSampleData.dwSampleSize = sampleSize - bytesPlayed;
					}

   				hSOSSampleHandle = sosDIGIStartSample( pSoundMgr->hDigiDriverHandle, &sSOSSampleData );
				}
				else
				{
					// if we are using a timer, we aren't really playing a sample.
					// fix up the timer.

					if (streaming)
 					{
						// we are streaming.  calculate the number of bytes played
						startTicks += ticksDiff;
					}
					else
					{
						waitTimer += ticksDiff;
					}
				}
			}
		}
	}
}


// Set the volume for a sound to be the 
// fraction of total volume as established by the current step.
// the first current step should be 1 (not 0).  the last step
// should equal totalSteps.

bool
TSound::Fade(uint16 direction, uint32 currentStep, uint32 totalSteps)
{
	if (direction == SNDMGR_FADE_UP)
	{
		if (currentStep == 1)
		{
			// first step of a fade up.  
			Resume();
		}
	}
	else
	{
		if (currentStep == totalSteps)
		{
			// last step of a fade down
			Pause();
			// we are done
			return TRUE;
		}
	}
		
	// adjust the current step so that we can use the same code
	// for either direction
	if (direction == SNDMGR_FADE_DOWN)
	{
		currentStep = totalSteps - currentStep;		
	}

	SetVolume((int16) ((volumeMax * currentStep)/totalSteps), panPosition, FALSE);

	if (direction == SNDMGR_FADE_UP && currentStep == totalSteps)
	{
		// last step
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void
TSound::SetPanPosition(int panPos)
{
	if (panPosition != panPos &&
		 sSOSSampleData.wChannel != _INTERLEAVED &&
		 pSoundMgr)
	{
		panPosition = panPos;

		sosDIGISetPanLocation(pSoundMgr->hDigiDriverHandle, hSOSSampleHandle,
			pSoundMgr->GetPanPosition(panPosition));
	}
}


TStreamBase*	
TSound::GetStreamer()
{
	return	&streamer;
}

#pragma pack(4)

// function to handle the sample done callbacks, the next sample
// will be started in here.
void _far cdecl 
TSoundStreamCallback(WORD wDriverHandle, WORD wAction, WORD wSampleHndl)
{
	TSound	*pSound;
	uint32	bytesRetrieved;
	uint32	bytesToPlay;

	wDriverHandle = wDriverHandle;	// Gets rid of unreferenced warning

   // check to see if the sample is finished processing
   if( wAction == _SAMPLE_PROCESSED )
   {
		// continue the sample
		pSound = ADerefAs(TSound, (grip) (sosDIGIGetSampleID(pSoundMgr->hDigiDriverHandle, wSampleHndl)));

      // start the sample playing if there is any data to play
      bytesRetrieved =
			pSound->GetStreamer()->GetBytes(ADerefAs(char, pSound->gStreamBuf),
				pSound->streamPlaySize);

		if (bytesRetrieved < pSound->streamPlaySize &&
			pSound->GetStreamer()->HasReachedEOF() == FALSE)
		{
			// we were unable to read more bytes because we can't read
			// during an interrupt.  pad the sound with silence and play the
			// standard size

			// WARNING - Silence is 0x80 for 8bit, and 0x00 for 16 bit
			memset(ADerefAs(char, pSound->gStreamBuf) + bytesRetrieved, 0x80, pSound->streamPlaySize - bytesRetrieved);
			bytesToPlay = pSound->streamPlaySize;
		}
		else
		{
			bytesToPlay = bytesRetrieved;
		}

      if(bytesToPlay)
		{
         if (pSoundMgr)
			{
				pSound->bytesPlayed += bytesRetrieved;
				pSound->sSOSSampleData.dwSampleSize = bytesToPlay; 
         	sosDIGIContinueSample( pSoundMgr->hDigiDriverHandle, pSound->hSOSSampleHandle, &pSound->sSOSSampleData);
			}
		}
   }
} 

#pragma pack()

//-----------------------------------------------------------------------
//
// TMusic
//
//-----------------------------------------------------------------------


TMusic::TMusic(void)
{
}


TMusic::~TMusic(void)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		if (pSoundMgr->MidiIsPlaying(this))
		{
			// the sound is still in the play list.
			// shut it down.  don't cue
			pSoundMgr->DeleteMidi(this, FALSE);
		}
	}
}

void
TMusic::Play(uint16 sngNum, int16 mVolume, grip callBack)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		if (mVolume < 0 ||
			 mVolume > MAX_VOLUME)
		{
			// set the volume to loudest
			mVolume = MAX_VOLUME;
		}
		volumeMax = volume = mVolume;

		pSoundMgr->AddMidi(this, sngNum, callBack);
	}
}

void
TMusic::Stop(void)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		if (pSoundMgr->MidiIsPlaying(this))
		{
			// if we are saying to stop, don't cue
			pSoundMgr->DeleteMidi(this, FALSE);
		}
	}
}

int
TMusic::IsPlaying(void)
{
	int	isPlaying;

	if (pSoundMgr)
	{
		if (pSoundMgr->MidiIsPlaying(this))
		{
			isPlaying = resourceNum;	
		}
		else
		{
			isPlaying = 0;	
		}
	}

	return isPlaying;
}

void
TMusic::Pause(void)
{
//	int16	oldVolume;

	pausedCnt++;	

	// only do the follow stuff if this is the first time that
	// we are going to pause
	if (pausedCnt == 1)
	{
		// NOTE: timer stuff is here for later use.  we currently can't use
		// these timer values

		// save the paused ticks so so we can fix time based things in resume
		pausedTime = ATicks();

		// if the music driver is active, pause all songs
		if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_MIDI_SYS_DRIVER)))
		{
			// because of a bug in SOS, we have to turn the volume down
			// before we pause.
//			oldVolume = pSoundMgr->GetMasterMidiVolume();
//			wError = sosMIDISetMasterVolume(0);	
//   		if(wError)
//   		{
//      		// display the error string and exit
//				char	errMess[400];
//      		sprintf(errMess, "\nError : %s", sosGetErrorString( wError ) );
//      		APanic(errMess);
//   		}

			_disable();
			sosMIDIPauseSong(wSongHandle, TRUE);
			_enable();

//			wError = sosMIDISetMasterVolume(127);	
//			wError = sosMIDISetMasterVolume(oldVolume);	
//   		if(wError)
//   		{
//      		// display the error string and exit
//				char	errMess[400];
//      		sprintf(errMess, "\nError : %s", sosGetErrorString( wError ) );
//      		APanic(errMess);
//   		}
		}
	}
}

void
TMusic::Resume(void)
{
	ticks_t	ticksDiff;

	if (pausedCnt)
	{
		pausedCnt--;

		// only unpause things when we are completely unpaused
		if (pausedCnt < 1)
		{
			pausedCnt = 0;

			// NOTE: timer stuff is here for later use.  we currently can't use
			// these timer values

			ticksDiff = ATicks() - pausedTime;

			// if the music driver is active, pause all songs
			if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_MIDI_SYS_DRIVER)))
			{
				_disable();
				sosMIDIResumeSong(wSongHandle);
				_enable();
			}
		}
	}
}

void
TMusic::SetVolume(int16 sVolume, int panPos, bool updateMax)
{
	// make sure that sound manager is inited
	if (pSoundMgr)
	{
		// take care of unreferenced warning
		panPos = panPos;

		if (sVolume < 0 ||
			 sVolume > MAX_VOLUME)
		{
			// set the volume to loudest
			sVolume = MAX_VOLUME;
		}
		volume = sVolume;
		if (updateMax)
		{
			volumeMax = volume;
		}

		// only call sos stuff if digi driver has been inited
		if (pSoundMgr->SystemIsActive(SOS_MIDI_SYS_DRIVER))
		{
			_disable();
			sosMIDISetSongVolume((WORD) wSongHandle, (BYTE) volume);
			_enable();
		}
	}
}

bool
TMusic::Fade(uint16 direction, uint32 currentStep, uint32 totalSteps)
{
	if (direction == SNDMGR_FADE_UP)
	{
		if (currentStep == 1)
		{
			// first step of a fade up.  
			Resume();
		}
	}
	else
	{
		if (currentStep == totalSteps)
		{
			// last step of a fade down
			Pause();
			// we are done
			return TRUE;
		}
	}
		
	// adjust the current step so that we can use the same code
	// for either direction
	if (direction == SNDMGR_FADE_DOWN)
	{
		currentStep = totalSteps - currentStep;		
	}

	SetVolume((int16) ((volumeMax * currentStep)/totalSteps), DIGI_PAN_CENTER, FALSE);

	if (direction == SNDMGR_FADE_UP && currentStep == totalSteps)
	{
		// last step
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}



