//
// SOUNDMGR.HPP
//
// March 3, 1994
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// SoundMgr class definition.
//
//
//----[]-------------------------------------------------------------

#ifndef	soundmgr_hpp
#define	soundmgr_hpp

#include	"list.hpp"
#include "manager.hpp"

extern "C"
{
#include "sos.h"
}


#include	"stream.hpp"
#define BUF_SIZE				20000L
#define RES_NUM				0
#define PLAY_WHOLE_SOUND	((AudioSelectionPtr) 0L)
#define ASYNC_PLAY			TRUE
#define COMPLETION_PROC		(ProcPtr)SoundFinished
//((ProcPtr) 0L)

#undef	WORD
#define	WORD			uint32

#include	"types.hpp"

#define	SOUND_OFF		0

int	MkPan(uint left, uint right);

// defines to be used for looking at entries in the config file
#define	DIGI_IRQ				"DigiIRQ"
#define	DIGI_DMA				"DigiDMA"
#define	DIGI_PORT			"DigiPort"
#define	DIGI_ID				"DigiID"
#define	DIGI_VOLUME			"DigiVolume"
#define	DIGI_STEREO_SWAP	"DigiStereoSwap"

#define	MIDI_PORT	"MidiPort"
#define	MIDI_ID		"MidiID"
#define	MIDI_VOLUME	"MidiVolume"

typedef struct
{
	uint16	formatTag;			// Format category
	uint16	nChannels;			// Number of channels
	uint32	nSamplesPerSec;	// Sampling rate
	uint32	nAvgBytesPerSec;	// For buffering
	uint16	nBlockAlign;		// Block alignments
	uint16	pad;
} WaveFormat;

typedef struct
{
	char			riffStr[4];		// string "RIFF"
	uint32		riffLength;		
	char			waveStr[4];		// string "WAVE"
	char			fmtStr[4];		// string "fmt_"
	uint32		fmtLength;
	WaveFormat	waveFormat;
	char			dataStr[4];		// string "data"
	uint32		dataLength;
} WaveFileHeader;


void	ShutDownSoundMgr();

#define	MAX_DIGI_SOUNDS		8
#define	MAX_MIDI_SONGS			2
#define	MAX_MIDI_DIGI_SOUNDS	8

#define	MAX_VOLUME			0x7f

// sosActiveSystems bits
#define	SOS_DETECT						0x0001
#define	SOS_DIGI_SYS					0x0002
#define	SOS_DIGI_DRIVER				0x0004
#define	SOS_DIGI_TIMER_EVENT			0x0008
#define	SOS_MIDI_SYS					0x0010 
#define	SOS_MIDI_SYS_DRIVER 			0x0020 
#define	SOS_TIMER						0x0200

// sound mgr fade enums
enum
{
	SNDMGR_FADE_UP = 1,
	SNDMGR_FADE_DOWN
};


// left/right panning stuff
#define	DIGI_PAN_LEFT		0x0000
#define	DIGI_PAN_RIGHT		0xffff
#define	DIGI_PAN_CENTER	0x8000
#define	DIGI_PAN_RANGE		0xffff

// forward declaration
class SoundMgr;

// music and sounds are both derived from this class
class SOS_Base : public Object
{
	public:	
		// these friend classes allow more functions to be protected
		friend	class SoundMgr;
		int		cueData;

	protected:
		grip		gToCue;
		int		resourceNum;
		grip		gResource;

		ticks_t	pausedTime;
		uint16	pausedCnt;
		int16		volume;
		int16		volumeMax;

		uint16	fadeDir;
		uint16	fadeTotalSteps;
		uint16	fadeCurrentStep;
		uint32	fadeStepDelay;
		ticks_t	fadeTime;
		grip		fadeCallBack;

	public:	
		SOS_Base();

		virtual	void		Stop(void) = 0;
		virtual	int		IsPlaying(void) = 0;
		virtual	void		Pause(void) = 0;
		virtual	void		Resume(void) = 0;
		virtual	void		SetVolume(int16 sVolume, int panPos = DIGI_PAN_CENTER, bool updateMax = TRUE) = 0;
		virtual	bool		Fade(uint16 direction, uint32 currentStep, uint32 totalSteps) = 0;

		virtual	void		AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay, grip callBack = NULL);
};


class TMusic : public SOS_Base 
{
	public:
		// handle for the initialized MIDI song
		WORD		wSongHandle;         

					TMusic();
					~TMusic();

		virtual	void		Play(uint16	sngNum, int16 mVolume = MAX_VOLUME, grip callBack = NULL);
		virtual	void		Stop(void);
		virtual	int		IsPlaying(void);
		virtual	void		Pause(void);
		virtual	void		Resume(void);
		virtual	void		SetVolume(int16 sVolume, int panPos = DIGI_PAN_CENTER, bool updateMax = TRUE);
		virtual	bool		Fade(uint16 direction, uint32 currentStep, uint32 totalSteps);
};

#define CHAIN_MAX	4
class TSound : public SOS_Base
{
	public:
		// these friend classes allow more functions to be protected
		friend	class SoundMgr;
		void		AddToChain(int resNum), PlayNextLink(void);
		int16		loopCount;	// made public so permanence can be read by app

	protected:
		int32		resChain[CHAIN_MAX], chainLength;
		ticks_t	waitTimer;

		// this variable is used just in case we switch from having
		// a sound driver to not having one and need to run on
		// a timer.
		ticks_t	playTimer;

		uint32	sampleSize;

		bool		streaming;

		uint16	bytesPerSample;

		uint32	startTicks;
		uint16	bytesPerTick;

		int		panPosition;

	public:
		
		// must be accessible to call back function
		_SOS_START_SAMPLE 	sSOSSampleData;
		uint16	streamPlaySize;
		grip		gStreamBuf;
		uint32	bytesPlayed;
		// used for streaming.  pass to sosDIGIContinueSample
		WORD		hSOSSampleHandle;

					TSound();
					~TSound();

		virtual	void		Play(int sndNum, int16 sVolume = MAX_VOLUME, int panPos = DIGI_PAN_CENTER, 
									grip callBack = NULL, 
									int16 loopCnt = 1, bool streamIt = FALSE);
		virtual	void		SetVolume(int16 sVolume, int panPos = DIGI_PAN_CENTER, bool updateMax = TRUE);
		virtual	void		Stop(void);
		virtual	int		IsPlaying(void);
		virtual	void		Pause(void);
		virtual	void		Resume(void);
		virtual	bool		Fade(uint16 direction, uint32 currentStep, uint32 totalSteps);
		virtual	void		SetPanPosition(int panPos);

		virtual	TStreamBase*	GetStreamer();

	protected:
		TStream	streamer;
};

class SoundMgr : public Manager, public Object
{
	public:
		SoundMgr();
		~SoundMgr();

		void		Init(char *configFileName = "sound.cfg");
		void		Cycle();

		void		Pause();
		void		Resume();

		void		SetMasterDigiVolume(int16 volume);
		int16		GetMasterDigiVolume();
		void		SetMasterMidiVolume(int16 volume);
		int16		GetMasterMidiVolume();

		void		ShutDown();

		bool		Save(uint16 state, FILE *fp = NULL);

		uint16	NumberDigiPlaying();
		uint16	NumberMidiPlaying();

		grip		OldestDigiPlaying(), NextOldestDigiPlaying(void);
		grip		OldestMidiPlaying();

		bool		Fade(uint16 direction, uint32 currentStep, uint32 totalSteps);
		void		AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay, grip callBack = NULL);

		void		SwapDigiLeftAndRight(bool swapThem);
		short		GetPanPosition(int panPos);

		// handle for the SOS driver being used. 
		// this is public because so many people need it and
		// I didn't want to have a function call or inline crap.
		WORD     hDigiDriverHandle;

		// these friend classes allow more functions to be protected
		friend	class TMusic;
		friend	class TSound;

		// this is no longer protected so that it can be used for smacker stuff
		bool		SystemIsActive(uint16 flags);

	protected:

		void		AddDigi(TSound *pSound, int sndNum, grip callBack);
		void		DeleteDigi(TSound *pSound, bool cue = TRUE);

		void		AddMidi(TMusic *pMusic, uint16 sndNum, grip callBack);
		void		DeleteMidi(TMusic *pMusic, bool cue = TRUE);

		bool		DigiIsPlaying(TSound *pSound);
		bool		MidiIsPlaying(TMusic *pMusic);

		char		*pInitErr;		
		uint16	digiDevice;
		uint16	digiPort;
		uint16	digiIRQ;
		uint16	digiDMA;
		uint16	digiVolume;
		uint16	digiVolumeMax;

		uint16 	midiDevice;
		uint16	midiPort;
		uint16	midiVolume;
		uint16	midiVolumeMax;

		uint16	sosActiveSystems;

		SysList	digiList;
		SysList	midiList;

		grip		gInstruments;
		grip		gDrums;
		grip		gDigiInstruments;
		grip		gMT32Patch;

		bool		swapDigiLeftAndRight;

		// handle for the SOS driver being used 
		WORD     hMidiDigiDriverHandle;

		// handle for the registered timer event
	   WORD     hTimerEventHandle;
	   WORD     hMidiDigiTimerEventHandle;

		// handle for the loaded MIDI driver
		WORD     hMidiDriverHandle;          
		WORD     hMidiDigiSysDriverHandle;          

		void	InitError(WORD wError, char *message = NULL);
};

extern SoundMgr* pSoundMgr;

#endif

