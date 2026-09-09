// Optional audio facade.
// The restoration build keeps game timing/API compatibility without HMI SOS.
#ifndef soundmgr_hpp
#define soundmgr_hpp

#include "Manager.hpp"
#include "Tigre.hpp"

#define SOUND_OFF 0
#define MAX_DIGI_SOUNDS 8
#define MAX_MIDI_SONGS 2
#define MAX_MIDI_DIGI_SOUNDS 8
#define MAX_VOLUME 0x7f

#define DIGI_PAN_LEFT 0x0000
#define DIGI_PAN_RIGHT 0xffff
#define DIGI_PAN_CENTER 0x8000
#define DIGI_PAN_RANGE 0xffff

#define DIGI_IRQ "DigiIRQ"
#define DIGI_DMA "DigiDMA"
#define DIGI_PORT "DigiPort"
#define DIGI_ID "DigiID"
#define DIGI_VOLUME "DigiVolume"
#define DIGI_STEREO_SWAP "DigiStereoSwap"
#define MIDI_PORT "MidiPort"
#define MIDI_ID "MidiID"
#define MIDI_VOLUME "MidiVolume"

enum { SNDMGR_FADE_UP = 1, SNDMGR_FADE_DOWN };

class TStreamBase;
class SoundMgr;

int MkPan(uint left, uint right);
void ShutDownSoundMgr();

class SOS_Base : public Object
{
	public:
		SOS_Base();
		virtual void Stop() = 0;
		virtual int IsPlaying() = 0;
		virtual void Pause() = 0;
		virtual void Resume() = 0;
		virtual void SetVolume(int16 volume, int panPos = DIGI_PAN_CENTER,
			bool updateMax = TRUE) = 0;
		virtual bool Fade(uint16 direction, uint32 currentStep,
			uint32 totalSteps) = 0;
		virtual void AutoFade(uint16 direction, uint32 totalSteps,
			uint32 stepDelay, grip callBack = NULL);

		int cueData;

	protected:
		int resourceNum;
		int16 volume;
		int16 volumeMax;
};

class TMusic : public SOS_Base
{
	public:
		TMusic();
		~TMusic();
		virtual void Play(uint16 songNum, int16 volume = MAX_VOLUME,
			grip callBack = NULL);
		virtual void Stop();
		virtual int IsPlaying();
		virtual void Pause();
		virtual void Resume();
		virtual void SetVolume(int16 volume, int panPos = DIGI_PAN_CENTER,
			bool updateMax = TRUE);
		virtual bool Fade(uint16 direction, uint32 currentStep,
			uint32 totalSteps);
};

#define CHAIN_MAX 4

class TSound : public SOS_Base
{
	public:
		TSound();
		~TSound();
		void AddToChain(int resourceNum);
		void PlayNextLink();
		virtual void Play(int soundNum, int16 volume = MAX_VOLUME,
			int panPos = DIGI_PAN_CENTER, grip callBack = NULL,
			int16 loopCount = 1, bool streamIt = FALSE);
		virtual void SetVolume(int16 volume, int panPos = DIGI_PAN_CENTER,
			bool updateMax = TRUE);
		virtual void Stop();
		virtual int IsPlaying();
		virtual void Pause();
		virtual void Resume();
		virtual bool Fade(uint16 direction, uint32 currentStep,
			uint32 totalSteps);
		virtual void SetPanPosition(int panPos);
		virtual TStreamBase *GetStreamer();

		int16 loopCount;

	protected:
		int32 resChain[CHAIN_MAX];
		int chainLength;
		int panPosition;
};

class SoundMgr : public Manager, public Object
{
	public:
		SoundMgr();
		~SoundMgr();
		void Init(char *configFileName = "sound.cfg");
		void Cycle();
		void Pause();
		void Resume();
		void SetMasterDigiVolume(int16 volume);
		int16 GetMasterDigiVolume();
		void SetMasterMidiVolume(int16 volume);
		int16 GetMasterMidiVolume();
		void ShutDown();
		bool Save(uint16 state, FILE *fp = NULL);
		uint16 NumberDigiPlaying();
		uint16 NumberMidiPlaying();
		grip OldestDigiPlaying();
		grip NextOldestDigiPlaying();
		grip OldestMidiPlaying();
		bool Fade(uint16 direction, uint32 currentStep, uint32 totalSteps);
		void AutoFade(uint16 direction, uint32 totalSteps, uint32 stepDelay,
			grip callBack = NULL);
		void SwapDigiLeftAndRight(bool swapThem);
		short GetPanPosition(int panPos);
		bool SystemIsActive(uint16 flags);

		int hDigiDriverHandle;

	private:
		int16 digiVolume;
		int16 midiVolume;
		bool swapDigiLeftAndRight;
};

extern SoundMgr *pSoundMgr;

#endif
