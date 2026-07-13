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
#include "json.hpp"

// SDL3_mixer audio backend -- types formerly from the HMI SOS SDK headers.
// The SOS API wrapper (sosDIGI*, sosMIDI*, sosTIMER*) has been removed;
// soundmgr.cpp now calls SDL3_mixer directly.

// Sample control flag bits (values match original sos.h definitions)
#define _VOLUME      0x0100
#define _LOOPING     0x4000
#define _PITCH_SHIFT 0x0400
#define _PANNING     0x0200

// Audio channel layout constants (values match original sos.h enum)
enum { _LEFT_CHANNEL = 0, _RIGHT_CHANNEL, _CENTER_CHANNEL, _INTERLEAVED };

// Parameter bag for playing a digital audio sample.
// Replaces _SOS_START_SAMPLE from sos.h.  Field names are unchanged so all
// existing soundmgr.cpp code compiles without modification.
struct SampleData {
	char   *lpSamplePtr;           // pointer to raw PCM data
	uint32  dwSampleSize;          // byte count of PCM data
	int16   wLoopCount;            // loop count (< 0 = infinite)
	uint32  wChannel;              // _CENTER_CHANNEL or _INTERLEAVED
	uint32  wVolume;               // volume: vol_0_127 << 8
	uint32  wSampleID;             // unique sample ID (grip handle)
	void  (*lpCallback)(uint32, uint32, uint32); // streaming callback (unused)
	uint32  wSamplePort;
	uint32  wSampleFlags;          // _VOLUME | _LOOPING | _PITCH_SHIFT | _PANNING
	uint32  dwSampleByteLength;
	uint32  dwSampleLoopPoint;
	uint32  dwSampleLoopLength;
	uint32  dwSamplePitchAdd;      // pitch ratio * 65536 when _PITCH_SHIFT set
	uint32  wSamplePitchFraction;
	uint32  wSamplePanLocation;    // 0x0000=left, 0x8000=centre, 0xFFFF=right
};

#include	"stream.hpp"
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
		virtual	void		SetVolume(int16 sVolume, int panPos = DIGI_PAN_CENTER, bool updateMax = true) = 0;
		virtual	bool		Fade(uint16 direction, uint32 currentStep, uint32 totalSteps) = 0;

		virtual	void		AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay, grip callBack = grip{});
};


class TMusic : public SOS_Base
{
	public:
		// handle for the initialized MIDI song (1-based index into midi slot table)
		uint32		wSongHandle;

					TMusic();
					~TMusic();

		virtual	void		Play(uint16	sngNum, int16 mVolume = MAX_VOLUME, grip callBack = grip{});
		virtual	void		Stop(void);
		virtual	int		IsPlaying(void);
		virtual	void		Pause(void);
		virtual	void		Resume(void);
		virtual	void		SetVolume(int16 sVolume, int panPos = DIGI_PAN_CENTER, bool updateMax = true);
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
		SampleData 	sSOSSampleData;
		uint16	streamPlaySize;
		grip		gStreamBuf;
		uint32	bytesPlayed;
		// handle for the playing sample (1-based index into digi slot table)
		uint32		hSOSSampleHandle;

					TSound();
					~TSound();

		virtual	void		Play(int sndNum, int16 sVolume = MAX_VOLUME, int panPos = DIGI_PAN_CENTER,
									grip callBack = grip{},
									int16 loopCnt = 1, bool streamIt = false);
		virtual	void		SetVolume(int16 sVolume, int panPos = DIGI_PAN_CENTER, bool updateMax = true);
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
		using Manager::operator new;
		SoundMgr();
		~SoundMgr();

		void		Init(const char *configFileName = "sound.cfg");
		void		Cycle();

		void		Pause();
		void		Resume();

		void		SetMasterDigiVolume(int16 volume);
		int16		GetMasterDigiVolume();
		void		SetMasterMidiVolume(int16 volume);
		int16		GetMasterMidiVolume();

		void		ShutDown();

		bool		Save(uint16 state, nlohmann::json& root);

		uint16	NumberDigiPlaying();
		uint16	NumberMidiPlaying();

		grip		OldestDigiPlaying(), NextOldestDigiPlaying(void);
		grip		OldestMidiPlaying();

		bool		Fade(uint16 direction, uint32 currentStep, uint32 totalSteps);
		void		AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay, grip callBack = grip{});

		void		SwapDigiLeftAndRight(bool swapThem);
		short		GetPanPosition(int panPos);

		// handle for the digi driver (always 1 after Init; kept for compatibility)
		uint32     hDigiDriverHandle;

		// these friend classes allow more functions to be protected
		friend	class TMusic;
		friend	class TSound;

		// this is no longer protected so that it can be used for smacker stuff
		bool		SystemIsActive(uint16 flags);

	protected:

		void		AddDigi(TSound *pSound, int sndNum, grip callBack);
		void		DeleteDigi(TSound *pSound, bool cue = true);

		void		AddMidi(TMusic *pMusic, uint16 sndNum, grip callBack);
		void		DeleteMidi(TMusic *pMusic, bool cue = true);

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

		// handle for the MIDI driver (always 1 after Init; kept for compatibility)
		uint32     hMidiDriverHandle;

		void	InitError(uint32 wError, const char *message = nullptr);
};

extern SoundMgr* pSoundMgr;

#endif
