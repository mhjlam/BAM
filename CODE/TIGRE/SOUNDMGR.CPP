#include "SoundMgr.hpp"

SoundMgr *pSoundMgr = NULL;

int MkPan(uint left, uint right)
{
	if (left == right)
		return DIGI_PAN_CENTER;
	if (!left && !right)
		return DIGI_PAN_CENTER;
	if (left > right)
		return (int)((right * DIGI_PAN_CENTER) / left);
	return (int)(DIGI_PAN_RIGHT - ((left * DIGI_PAN_CENTER) / right));
}

void ShutDownSoundMgr()
{
	if (pSoundMgr)
		pSoundMgr->ShutDown();
}

SOS_Base::SOS_Base()
{
	cueData = 0;
	resourceNum = 0;
	volume = MAX_VOLUME;
	volumeMax = MAX_VOLUME;
}

void SOS_Base::AutoFade(uint16 direction, uint32, uint32, grip)
{
	Fade(direction, 1, 1);
}

TMusic::TMusic() { }
TMusic::~TMusic() { }
void TMusic::Play(uint16 songNum, int16 newVolume, grip) { resourceNum = songNum; volume = newVolume; }
void TMusic::Stop() { resourceNum = 0; }
int TMusic::IsPlaying() { return FALSE; }
void TMusic::Pause() { }
void TMusic::Resume() { }
void TMusic::SetVolume(int16 newVolume, int, bool updateMax) { volume = newVolume; if (updateMax) volumeMax = newVolume; }
bool TMusic::Fade(uint16, uint32, uint32) { return TRUE; }

TSound::TSound()
{
	loopCount = 0;
	chainLength = 0;
	panPosition = DIGI_PAN_CENTER;
}

TSound::~TSound() { }

void TSound::AddToChain(int number)
{
	if (chainLength < CHAIN_MAX)
		resChain[chainLength++] = number;
}

void TSound::PlayNextLink() { }

void TSound::Play(int soundNum, int16 newVolume, int newPan, grip,
	int16 newLoopCount, bool)
{
	resourceNum = soundNum;
	volume = newVolume;
	volumeMax = newVolume;
	panPosition = newPan;
	loopCount = newLoopCount;
}

void TSound::SetVolume(int16 newVolume, int newPan, bool updateMax)
{
	volume = newVolume;
	panPosition = newPan;
	if (updateMax)
		volumeMax = newVolume;
}

void TSound::Stop() { resourceNum = 0; loopCount = 0; }
int TSound::IsPlaying() { return FALSE; }
void TSound::Pause() { }
void TSound::Resume() { }
bool TSound::Fade(uint16, uint32, uint32) { return TRUE; }
void TSound::SetPanPosition(int newPan) { panPosition = newPan; }
TStreamBase *TSound::GetStreamer() { return NULL; }

SoundMgr::SoundMgr()
{
	digiVolume = MAX_VOLUME;
	midiVolume = MAX_VOLUME;
	swapDigiLeftAndRight = FALSE;
	hDigiDriverHandle = -1;
	if (!pSoundMgr)
		pSoundMgr = this;
}

SoundMgr::~SoundMgr()
{
	if (pSoundMgr == this)
		pSoundMgr = NULL;
}

void SoundMgr::Init(char *) { }
void SoundMgr::Cycle() { }
void SoundMgr::Pause() { }
void SoundMgr::Resume() { }
void SoundMgr::SetMasterDigiVolume(int16 value) { digiVolume = value; }
int16 SoundMgr::GetMasterDigiVolume() { return digiVolume; }
void SoundMgr::SetMasterMidiVolume(int16 value) { midiVolume = value; }
int16 SoundMgr::GetMasterMidiVolume() { return midiVolume; }
void SoundMgr::ShutDown() { }
bool SoundMgr::Save(uint16, FILE *) { return TRUE; }
uint16 SoundMgr::NumberDigiPlaying() { return 0; }
uint16 SoundMgr::NumberMidiPlaying() { return 0; }
grip SoundMgr::OldestDigiPlaying() { return 0; }
grip SoundMgr::NextOldestDigiPlaying() { return 0; }
grip SoundMgr::OldestMidiPlaying() { return 0; }
bool SoundMgr::Fade(uint16, uint32, uint32) { return TRUE; }
void SoundMgr::AutoFade(uint16, uint32, uint32, grip) { }
void SoundMgr::SwapDigiLeftAndRight(bool value) { swapDigiLeftAndRight = value; }
short SoundMgr::GetPanPosition(int panPos) { return (short)panPos; }
bool SoundMgr::SystemIsActive(uint16) { return FALSE; }
