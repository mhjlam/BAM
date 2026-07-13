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
//   is used to play samples recorded at slower rates.
//
// - All volumes (midi and digi) range from 0 to 127.  Digi sounds
//   actually have a range of 0 to 0x7ff.  To make things simpler
//   for the applications programmer, all digi sounds are adjusted
//   by the Sound Manager so that we can use the same volume range for
//   all audio devices.
//
//----[]-------------------------------------------------------------

#include <ctype.h>
#include	"api.hpp"
#include "apievt.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "mono.hpp"
#include "resource.hpp"
#include "eventmgr.hpp"
#include "game_config.hpp"
#include "graphmgr.hpp"
#include	"savemgr.hpp"
#include "debug.hpp"

// SDL3_mixer backend (replaces compat/audio.cpp)
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include <vector>
#include <algorithm>

#ifndef NDEBUG
#define NDEBUG
#endif

extern Debugger ResMgrDebug;

extern bool g_verbose;
#define MIDI_LOG(...) do { if (g_verbose) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "midi: " __VA_ARGS__); } while(0)

#include	"soundmgr.hpp"

// borrowed this from smack.h -- used to init hDigiDriverHandle
#define SMACKSOUNDNONE -1

#define	_SOS_DRIVER_RATE		22050
#define  _TIMER_RATE             60

// patch file numbers
#define  _MELODIC_PATCH       1
#define  _DRUM_PATCH          2
#define  _MT32_PATCH          4

//----[]-------------------------------------------------------------
// SDL3_mixer digital audio backend
// (was CODE/compat/audio.cpp -- now inlined into soundmgr.cpp)
//----[]-------------------------------------------------------------

static MIX_Mixer *s_mixer = nullptr;

static const int MAX_SLOTS = 16;

struct SndSlot {
	MIX_Track *track;
	MIX_Audio *audio;
	bool       active;
};

static SndSlot s_slots[MAX_SLOTS];

static int alloc_slot()
{
	for (int i = 0; i < MAX_SLOTS; ++i)
		if (!s_slots[i].active) return i;
	return -1;
}

static bool valid_handle(uint32 h)
{
	return h >= 1 && (int)h <= MAX_SLOTS && s_slots[(int)h - 1].active;
}

static void free_slot(int idx)
{
	MIX_DestroyTrack(s_slots[idx].track);
	MIX_DestroyAudio(s_slots[idx].audio);
	s_slots[idx].track  = nullptr;
	s_slots[idx].audio  = nullptr;
	s_slots[idx].active = false;
}

//----[]-------------------------------------------------------------
// HMP -> SMF conversion helpers
// (HMP "NDMF" format, "013195" variant used by Blood & Magic)
//----[]-------------------------------------------------------------

#define HMP_TRACK_COUNT_OFF  0x30
#define HMP_DIVISION_OFF     0x38
#define HMP_TRACK_START_0    0x308
#define HMP_TRACK_START_1    0x388
#define HMPTRACK_LEN_OFF     4
#define HMPTRACK_DATA_OFF    12

static uint32_t read_hmp_vlq(const uint8_t *&p, const uint8_t *end)
{
	uint32_t time = 0;
	uint8_t  t    = 0;
	int      off  = 0;
	while (!(t & 0x80) && p < end) {
		t = *p++;
		time |= (uint32_t)(t & 0x7F) << off;
		off += 7;
	}
	return time;
}

static void write_midi_vlq(std::vector<uint8_t> &out, uint32_t value)
{
	uint8_t buf[4];
	int n = 0;
	buf[n++] = value & 0x7F;
	while (value >>= 7)
		buf[n++] = (value & 0x7F) | 0x80;
	while (n > 0)
		out.push_back(buf[--n]);
}

static void write_be16(std::vector<uint8_t> &out, uint16_t v)
{
	out.push_back((v >> 8) & 0xFF);
	out.push_back(v & 0xFF);
}

static void write_be32(std::vector<uint8_t> &out, uint32_t v)
{
	out.push_back((v >> 24) & 0xFF);
	out.push_back((v >> 16) & 0xFF);
	out.push_back((v >>  8) & 0xFF);
	out.push_back(v & 0xFF);
}

static inline uint32_t le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
	     | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static const int s_midi_data_len[7] = { 2, 2, 2, 2, 1, 1, 2 };

static void convert_hmp_track(const uint8_t *src, size_t src_len,
                               std::vector<uint8_t> &out)
{
	size_t hdr_pos   = out.size();
	const uint8_t mtrk[] = {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0, 0};
	out.insert(out.end(), mtrk, mtrk + 8);
	size_t data_start = out.size();

	const uint8_t *p   = src;
	const uint8_t *end = src + src_len;
	uint8_t running_status = 0;
	bool    eot_written    = false;

	while (p < end) {
		uint32_t delta = read_hmp_vlq(p, end);
		write_midi_vlq(out, delta);

		if (p >= end) break;

		uint8_t event = *p;

		if (event == 0xFF) {
			p++;
			if (p >= end) break;
			uint8_t meta_type = *p++;
			uint32_t len = read_hmp_vlq(p, end);
			out.push_back(0xFF);
			out.push_back(meta_type);
			write_midi_vlq(out, len);
			size_t copy_len = std::min((size_t)len, (size_t)(end - p));
			out.insert(out.end(), p, p + copy_len);
			p += copy_len;
			if (meta_type == 0x2F) {
				eot_written = true;
				break;
			}
		} else if (event == 0xF0 || event == 0xF7) {
			p++;
			uint32_t len = read_hmp_vlq(p, end);
			out.push_back(event);
			write_midi_vlq(out, len);
			size_t copy_len = std::min((size_t)len, (size_t)(end - p));
			out.insert(out.end(), p, p + copy_len);
			p += copy_len;
		} else if (event == 0xFE) {
			p++;
			if (p >= end) break;
			uint8_t sub = *p++;
			if (sub == 0x13 || sub == 0x15) {
				p = std::min(p + 6, end);
			} else if (sub == 0x12 || sub == 0x14) {
				p = std::min(p + 2, end);
			} else if (sub == 0x10) {
				p = std::min(p + 2, end);
				if (p < end) p = std::min(p + (size_t)*p + 6, end);
			} else {
				break;
			}
			out.push_back(0xFF);
			out.push_back(0x01);
			out.push_back(0x00);
		} else if (event >= 0x80) {
			running_status = event;
			p++;
			out.push_back(event);
			int n = s_midi_data_len[(event & 0x70) >> 4];
			for (int i = 0; i < n && p < end; i++)
				out.push_back(*p++);
		} else {
			if (!running_status) { p++; continue; }
			out.push_back(running_status);
			int n = s_midi_data_len[(running_status & 0x70) >> 4];
			out.push_back(event);
			p++;
			for (int i = 1; i < n && p < end; i++)
				out.push_back(*p++);
		}
	}

	if (!eot_written) {
		out.push_back(0x00);
		out.push_back(0xFF);
		out.push_back(0x2F);
		out.push_back(0x00);
	}

	uint32_t track_len = (uint32_t)(out.size() - data_start);
	out[hdr_pos + 4] = (track_len >> 24) & 0xFF;
	out[hdr_pos + 5] = (track_len >> 16) & 0xFF;
	out[hdr_pos + 6] = (track_len >>  8) & 0xFF;
	out[hdr_pos + 7] =  track_len        & 0xFF;
}

static bool hmp_to_smf(const uint8_t *hmp, size_t hmp_size,
                        std::vector<uint8_t> &smf_out)
{
	if (hmp_size < 0x100 || memcmp(hmp, "HMIMIDIP", 8) != 0)
		return false;

	size_t track_data_start;
	if (hmp[8] == 0) {
		track_data_start = HMP_TRACK_START_0;
	} else if (memcmp(hmp + 8, "013195", 6) == 0) {
		track_data_start = HMP_TRACK_START_1;
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "midi: unknown HMP version at byte 8");
		return false;
	}

	if (hmp_size < track_data_start + HMPTRACK_DATA_OFF)
		return false;

	uint32_t num_tracks = le32(hmp + HMP_TRACK_COUNT_OFF);
	uint16_t division   = (uint16_t)le32(hmp + HMP_DIVISION_OFF);
	const uint32_t tempo_us = 1000000;

	if (num_tracks == 0 || num_tracks > 256)
		return false;

	smf_out.clear();

	const uint8_t mthd[] = {0x4D, 0x54, 0x68, 0x64};
	smf_out.insert(smf_out.end(), mthd, mthd + 4);
	write_be32(smf_out, 6);
	write_be16(smf_out, 1);
	write_be16(smf_out, (uint16_t)(num_tracks + 1));
	write_be16(smf_out, division);

	{
		size_t hdr_pos = smf_out.size();
		const uint8_t mtrk[] = {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0, 0};
		smf_out.insert(smf_out.end(), mtrk, mtrk + 8);
		size_t data_start = smf_out.size();

		smf_out.push_back(0x00);
		smf_out.push_back(0xFF);
		smf_out.push_back(0x51);
		smf_out.push_back(0x03);
		smf_out.push_back((tempo_us >> 16) & 0xFF);
		smf_out.push_back((tempo_us >>  8) & 0xFF);
		smf_out.push_back( tempo_us        & 0xFF);
		smf_out.push_back(0x00);
		smf_out.push_back(0xFF);
		smf_out.push_back(0x2F);
		smf_out.push_back(0x00);

		uint32_t tlen = (uint32_t)(smf_out.size() - data_start);
		smf_out[hdr_pos + 4] = (tlen >> 24) & 0xFF;
		smf_out[hdr_pos + 5] = (tlen >> 16) & 0xFF;
		smf_out[hdr_pos + 6] = (tlen >>  8) & 0xFF;
		smf_out[hdr_pos + 7] =  tlen        & 0xFF;
	}

	size_t pos = track_data_start;
	for (uint32_t i = 0; i < num_tracks; ++i) {
		if (pos + HMPTRACK_LEN_OFF + 4 > hmp_size) break;

		uint32_t track_total = le32(hmp + pos + HMPTRACK_LEN_OFF);
		if (track_total < (uint32_t)HMPTRACK_DATA_OFF)
			track_total = (uint32_t)HMPTRACK_DATA_OFF;
		track_total = (uint32_t)std::min((size_t)track_total, hmp_size - pos);

		uint32_t midi_len = track_total - (uint32_t)HMPTRACK_DATA_OFF;
		convert_hmp_track(hmp + pos + HMPTRACK_DATA_OFF, midi_len, smf_out);
		pos += track_total;
	}

	return true;
}

//----[]-------------------------------------------------------------
// MIDI slot management
//----[]-------------------------------------------------------------

static const int MAX_MIDI_SLOTS = 4;

struct MidiSlot {
	MIX_Track            *track;
	MIX_Audio            *audio;
	std::vector<uint8_t>  smf_buf;
	bool                  active;
};

static MidiSlot s_midi_slots[MAX_MIDI_SLOTS];
static char     s_soundfont_path[512];

static float    s_midi_master_gain = 1.0f;
static float    s_midi_song_gain[MAX_MIDI_SLOTS] = {1.0f, 1.0f, 1.0f, 1.0f};

static bool find_soundfont()
{
	const char *candidates[] = {
		"/usr/share/sounds/sf2/default-GM.sf2",
		"/usr/share/sounds/sf2/FluidR3_GM.sf2",
		"/usr/share/sounds/sf2/TimGM6mb.sf2",
		"/usr/share/soundfonts/default.sf2",
		"/usr/share/soundfonts/FluidR3_GM.sf2",
		nullptr
	};
	SDL_PathInfo info;
	for (int i = 0; candidates[i]; i++) {
		MIDI_LOG("checking SoundFont: %s\n", candidates[i]);
		if (SDL_GetPathInfo(candidates[i], &info)) {
			snprintf(s_soundfont_path, sizeof(s_soundfont_path), "%s", candidates[i]);
			return true;
		}
	}
	const char *data_dir = SDL_getenv("BAM_DATA");
	if (data_dir) {
		snprintf(s_soundfont_path, sizeof(s_soundfont_path), "%s/default.sf2", data_dir);
		MIDI_LOG("checking SoundFont: %s\n", s_soundfont_path);
		if (SDL_GetPathInfo(s_soundfont_path, &info)) return true;
	}
	s_soundfont_path[0] = '\0';
	return false;
}

static int midi_alloc_slot()
{
	for (int i = 0; i < MAX_MIDI_SLOTS; ++i)
		if (!s_midi_slots[i].active) return i;
	return -1;
}

static bool midi_valid(uint32 handle)
{
	return handle >= 1 && (int)handle <= MAX_MIDI_SLOTS
	    && s_midi_slots[(int)handle - 1].active;
}

static void midi_free_slot(int idx)
{
	if (s_midi_slots[idx].track) {
		MIX_StopTrack(s_midi_slots[idx].track, 0);
		MIX_DestroyTrack(s_midi_slots[idx].track);
		s_midi_slots[idx].track = nullptr;
	}
	if (s_midi_slots[idx].audio) {
		MIX_DestroyAudio(s_midi_slots[idx].audio);
		s_midi_slots[idx].audio = nullptr;
	}
	s_midi_slots[idx].smf_buf.clear();
	s_midi_slots[idx].smf_buf.shrink_to_fit();
	s_midi_slots[idx].active = false;
	s_midi_song_gain[idx] = 1.0f;
}

//----[]-------------------------------------------------------------
// Direct SDL3_mixer helper functions
// (replace the sosDIGI*/sosMIDI*/sosTIMER* SOS API wrapper)
//----[]-------------------------------------------------------------

// Start playing a digital sample; returns 1-based slot handle (0 = failure).
static uint32 bam_digi_start(SampleData *ss)
{
	if (!s_mixer || !ss || !ss->lpSamplePtr || !ss->dwSampleSize)
		return 0;

	int idx = alloc_slot();
	if (idx < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "audio: no free slot for sample");
		return 0;
	}

	int freq = 22050;
	if (ss->wSampleFlags & _PITCH_SHIFT)
		freq = (int)((float)ss->dwSamplePitchAdd / 65536.0f * 22050.0f + 0.5f);
	if (freq < 1) freq = 22050;

	SDL_AudioSpec spec;
	spec.format   = SDL_AUDIO_U8;
	spec.channels = (ss->wChannel == (uint32)_INTERLEAVED) ? 2 : 1;
	spec.freq     = freq;

	MIX_Audio *audio = MIX_LoadRawAudio(s_mixer,
	    ss->lpSamplePtr, (size_t)ss->dwSampleSize, &spec);
	if (!audio) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "audio: MIX_LoadRawAudio failed: %s", SDL_GetError());
		return 0;
	}

	MIX_Track *track = MIX_CreateTrack(s_mixer);
	if (!track) {
		MIX_DestroyAudio(audio);
		return 0;
	}
	MIX_SetTrackAudio(track, audio);

	if (ss->wSampleFlags & _VOLUME) {
		float gain = (float)ss->wVolume / (float)(127 << 8);
		MIX_SetTrackGain(track, gain);
	}

	if ((ss->wSampleFlags & _PANNING) && ss->wChannel != (uint32)_INTERLEAVED) {
		float pan = (float)(uint16_t)ss->wSamplePanLocation / 65535.0f;
		MIX_StereoGains gains = { 1.0f - pan, pan };
		MIX_SetTrackStereo(track, &gains);
	}

	int loops = 0;
	if (ss->wSampleFlags & _LOOPING) {
		int16 lc = (int16)(uint16_t)ss->wLoopCount;
		loops = (lc < 0) ? -1 : (int)lc;
	}

	SDL_PropertiesID props = 0;
	if (loops != 0) {
		props = SDL_CreateProperties();
		SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
	}
	MIX_PlayTrack(track, props);
	if (props) SDL_DestroyProperties(props);

	s_slots[idx].track  = track;
	s_slots[idx].audio  = audio;
	s_slots[idx].active = true;

	return (uint32)(idx + 1);
}

// Stop a digital sample and free its slot.
static void bam_digi_stop(uint32 handle)
{
	if (!valid_handle(handle)) return;
	int idx = (int)handle - 1;
	MIX_StopTrack(s_slots[idx].track, 0);
	free_slot(idx);
}

// Returns true if the sample has finished playing.
static bool bam_digi_done(uint32 handle)
{
	if (!valid_handle(handle)) return true;
	int idx = (int)handle - 1;
	bool active = MIX_TrackPlaying(s_slots[idx].track) ||
	              MIX_TrackPaused(s_slots[idx].track);
	if (!active)
		free_slot(idx);
	return !active;
}

// Set per-sample volume (vol = vol_0_127 << 8).
static void bam_digi_set_volume(uint32 handle, uint32 vol)
{
	if (!valid_handle(handle)) return;
	float gain = (float)vol / (float)(127 << 8);
	MIX_SetTrackGain(s_slots[(int)handle - 1].track, gain);
}

// Set per-sample pan (pan = 0x0000..0xFFFF).
static void bam_digi_set_pan(uint32 handle, uint32 pan)
{
	if (!valid_handle(handle)) return;
	float p = (float)(uint16_t)pan / 65535.0f;
	MIX_StereoGains gains = { 1.0f - p, p };
	MIX_SetTrackStereo(s_slots[(int)handle - 1].track, &gains);
}

// Init a MIDI song from HMP data; returns 1-based slot handle (0 = failure).
static uint32 bam_midi_init(uint8_t *data)
{
	if (!data) return 0;

	int idx = midi_alloc_slot();
	if (idx < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "midi: no free slots");
		return 0;
	}

	const uint8_t *hmp   = data;
	const size_t   maxsz = 4u * 1024u * 1024u;

	char magic9[10] = {};
	memcpy(magic9, hmp, 9);
	MIDI_LOG("InitSong: magic='%s' slot=%d\n", magic9, idx);

	size_t hmp_size = maxsz;
	if (hmp[8] == 0 || memcmp(hmp + 8, "013195", 6) == 0) {
		size_t track_off = (hmp[8] == 0) ? HMP_TRACK_START_0 : HMP_TRACK_START_1;
		uint32_t ntracks = le32(hmp + HMP_TRACK_COUNT_OFF);
		MIDI_LOG("InitSong: ntracks=%u division=%u track_off=0x%zx\n",
		    ntracks, le32(hmp + HMP_DIVISION_OFF), track_off);
		if (ntracks > 0 && ntracks <= 256) {
			size_t pos = track_off;
			bool ok = true;
			for (uint32_t i = 0; i < ntracks && ok; ++i) {
				if (pos + HMPTRACK_LEN_OFF + 4 > maxsz) { ok = false; break; }
				uint32_t tlen = le32(hmp + pos + HMPTRACK_LEN_OFF);
				MIDI_LOG("  track %u: total_len=%u\n", i, tlen);
				if (tlen < (uint32_t)HMPTRACK_DATA_OFF || tlen > 0x100000) {
					ok = false; break;
				}
				pos += tlen;
			}
			if (ok) hmp_size = pos;
		}
	}
	MIDI_LOG("InitSong: computed hmp_size=%zu\n", hmp_size);

	if (!hmp_to_smf(hmp, hmp_size, s_midi_slots[idx].smf_buf)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "midi: HMP->SMF conversion failed");
		return 0;
	}
	MIDI_LOG("InitSong: SMF size=%zu bytes\n", s_midi_slots[idx].smf_buf.size());

	std::vector<uint8_t> &smf = s_midi_slots[idx].smf_buf;
	SDL_IOStream *io = SDL_IOFromConstMem(smf.data(), smf.size());
	if (!io) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "midi: SDL_IOFromConstMem failed: %s", SDL_GetError());
		s_midi_slots[idx].smf_buf.clear();
		return 0;
	}

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetPointerProperty(props,  MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER,        io);
	SDL_SetBooleanProperty(props,  MIX_PROP_AUDIO_LOAD_CLOSEIO_BOOLEAN,         true);
	SDL_SetBooleanProperty(props,  MIX_PROP_AUDIO_LOAD_PREDECODE_BOOLEAN,       false);
	SDL_SetPointerProperty(props,  MIX_PROP_AUDIO_LOAD_PREFERRED_MIXER_POINTER, s_mixer);
	if (s_soundfont_path[0]) {
		MIDI_LOG("InitSong: setting soundfont_path=%s\n", s_soundfont_path);
		SDL_SetStringProperty(props,
		    "SDL_mixer.decoder.fluidsynth.soundfont_path", s_soundfont_path);
	}

	MIX_Audio *audio = MIX_LoadAudioWithProperties(props);
	SDL_DestroyProperties(props);

	if (!audio) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "midi: MIX_LoadAudioWithProperties failed: %s", SDL_GetError());
		s_midi_slots[idx].smf_buf.clear();
		return 0;
	}
	MIDI_LOG("InitSong: audio loaded OK\n");

	MIX_Track *track = MIX_CreateTrack(s_mixer);
	if (!track) {
		MIX_DestroyAudio(audio);
		s_midi_slots[idx].smf_buf.clear();
		return 0;
	}
	MIX_SetTrackAudio(track, audio);

	s_midi_slots[idx].track  = track;
	s_midi_slots[idx].audio  = audio;
	s_midi_slots[idx].active = true;

	MIDI_LOG("InitSong: handle=%u\n", (unsigned)(idx + 1));
	return (uint32)(idx + 1);
}

// Free a MIDI slot.
static void bam_midi_uninit(uint32 handle)
{
	if (!midi_valid(handle)) return;
	midi_free_slot((int)handle - 1);
}

// Start playing a MIDI song.
static void bam_midi_start(uint32 handle)
{
	if (!midi_valid(handle)) { MIDI_LOG("StartSong: invalid handle %u\n", handle); return; }
	MIDI_LOG("StartSong: handle=%u\n", handle);
	int idx = (int)handle - 1;
	MIX_Track *track = s_midi_slots[idx].track;
	MIX_SetTrackGain(track, s_midi_song_gain[idx] * s_midi_master_gain);
	bool ok = MIX_PlayTrack(track, 0);
	MIDI_LOG("StartSong: MIX_PlayTrack=%s\n", ok ? "ok" : SDL_GetError());
}

// Stop a MIDI song.
static void bam_midi_stop(uint32 handle)
{
	if (!midi_valid(handle)) return;
	MIDI_LOG("StopSong: handle=%u\n", handle);
	MIX_StopTrack(s_midi_slots[(int)handle - 1].track, 0);
}

// Pause a MIDI song.
static void bam_midi_pause(uint32 handle)
{
	if (!midi_valid(handle)) return;
	MIDI_LOG("PauseSong: handle=%u\n", handle);
	MIX_PauseTrack(s_midi_slots[(int)handle - 1].track);
}

// Resume a paused MIDI song.
static void bam_midi_resume(uint32 handle)
{
	if (!midi_valid(handle)) return;
	MIDI_LOG("ResumeSong: handle=%u\n", handle);
	MIX_ResumeTrack(s_midi_slots[(int)handle - 1].track);
}

// Returns true if a MIDI song has finished playing.
static bool bam_midi_done(uint32 handle)
{
	if (!midi_valid(handle)) return true;
	MIX_Track *track = s_midi_slots[(int)handle - 1].track;
	bool playing = MIX_TrackPlaying(track) || MIX_TrackPaused(track);
	if (!playing)
		MIDI_LOG("SongDone: handle=%u finished\n", handle);
	return !playing;
}

// Set master MIDI gain (vol 0..127).
static void bam_midi_set_master_vol(uint32 vol)
{
	s_midi_master_gain = (float)(uint8_t)vol / 127.0f;
	for (int i = 0; i < MAX_MIDI_SLOTS; ++i)
		if (s_midi_slots[i].active && s_midi_slots[i].track)
			MIX_SetTrackGain(s_midi_slots[i].track,
			                 s_midi_song_gain[i] * s_midi_master_gain);
}

// Set per-song MIDI volume (vol 0..127).
static void bam_midi_set_song_vol(uint32 handle, uint32 vol)
{
	if (!midi_valid(handle)) return;
	int idx = (int)handle - 1;
	s_midi_song_gain[idx] = (float)(uint16_t)vol / 127.0f;
	MIX_SetTrackGain(s_midi_slots[idx].track,
	                 s_midi_song_gain[idx] * s_midi_master_gain);
}

//----[]-------------------------------------------------------------
// SoundMgr implementation
//----[]-------------------------------------------------------------

// One buffer is supposed to last 1 tick.  This is how
// many buffers we allocate.
#define	STREAM_BUF_MULTIPLIER	40

SoundMgr	*pSoundMgr = nullptr;


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
		snprintf(mess, sizeof(mess), "Invalid Pan Value(s) MkPan(%d,%d)", left, right);
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
SoundMgrSave(uint16 state, nlohmann::json& root)
{
	return pSoundMgr->Save(state, root);
}

SoundMgr::SoundMgr(void)
{
	pInitErr = nullptr;

	// set the devices to off
	digiDevice = midiDevice = SOUND_OFF;

	// no systems are active
	sosActiveSystems = 0;

	//init the handle to something that smacker will recognize as "no driver"
	hDigiDriverHandle = SMACKSOUNDNONE;

	// init banks to not here
	gDigiInstruments = gInstruments = gDrums = gMT32Patch = grip{};

	swapDigiLeftAndRight = false;

	// set the global pointer
	if (!pSoundMgr)
	{
		pSoundMgr = this;

		atexit(ShutDownSoundMgr);
		// setup for save
		AtSave(SoundMgrSave);
	}
}

SoundMgr::~SoundMgr(void)
{
	ShutDownSoundMgrNow(this);
	pSoundMgr = nullptr;
}

void
SoundMgr::SwapDigiLeftAndRight(bool swapThem)
{
	if (swapThem)
	{
		swapDigiLeftAndRight = true;
	}
	else
	{
		swapDigiLeftAndRight = false;
	}
}


// if the channels are being swapped, flip the bytes of the pan value

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
SoundMgr::Init(const char *configFileName)
{
	// Skip SOUND.CFG hardware detection (DOS SoundBlaster hardware doesn't exist).
	// Set sensible defaults, then restore any volumes saved via the options menu.
	digiDevice    = 1;          // non-zero enables the digital audio path
	midiDevice    = 0xa001;     // _MIDI_MPU401: enable MIDI (HMP->SMF + FluidSynth)
	digiVolume    = MAX_VOLUME;
	digiVolumeMax = MAX_VOLUME;
	midiVolume    = MAX_VOLUME;
	midiVolumeMax = MAX_VOLUME;

	{
		GameConfig cfg = LoadGameConfig();
		digiVolume = digiVolumeMax = (uint16)cfg.digi_volume;
		midiVolume = midiVolumeMax = (uint16)cfg.midi_volume;
		ASetGameSpeed(cfg.game_speed);
		SetHealthBarsEnabled(cfg.health_bars);
	}

	// Initialize SDL3_mixer digital audio
	if (digiDevice != 0)
	{
		MIX_Init();
		s_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
		if (!s_mixer)
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "audio: MIX_CreateMixerDevice failed: %s", SDL_GetError());
		memset(s_slots, 0, sizeof(s_slots));

		sosActiveSystems |= SOS_DIGI_SYS | SOS_DIGI_DRIVER;
		hDigiDriverHandle = 1;
	}

	// Initialize MIDI (FluidSynth via SDL3_mixer)
	if (midiDevice != 0)
	{
		memset(s_midi_slots, 0, sizeof(s_midi_slots));
		s_soundfont_path[0] = '\0';

		if (find_soundfont()) {
			MIDI_LOG("SoundFont found: %s\n", s_soundfont_path);
		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "midi: no SoundFont found -- MIDI will be silent");
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "midi: install fluid-soundfont-gm or timgm6mb-soundfont");
		}

		hMidiDriverHandle = 1;
		sosActiveSystems |= SOS_MIDI_SYS | SOS_MIDI_SYS_DRIVER;

		SetMasterMidiVolume(midiVolume);
	}

	// Set up digital audio volume
	if (digiDevice != 0)
	{
		SetMasterDigiVolume(digiVolume);
	}
}

// This should be called for any error during the Init
// If this is an SOS error, send the error number and nullptr.
// If this is a message, send zero and the message pointer.
void
SoundMgr::InitError(uint32 wError, const char *message)
{
	char	errMess[400];
	const char	*pMess;

	if (message == nullptr)
	{
		snprintf(errMess, sizeof(errMess), "\nSound error: %u", wError);
		pMess = errMess;
	}
	else
	{
		pMess = message;
	}

	SDL_Log("%s", pMess);

	ShutDownSoundMgr();
	APrintUnfreedPtrs(false);

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
		for (gDigiSound = ptr_to_grip(digiList.FirstValue());
			gDigiSound;
			gDigiSound = ptr_to_grip(digiList.NextValue()))
		{
			pDigiSound = ADerefAs(TSound, gDigiSound);

			// only process sounds that aren't paused
			if (!pDigiSound->pausedCnt)
			{
				donePlaying = false;

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
						donePlaying = true;
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
						 	bam_digi_done(pDigiSound->hSOSSampleHandle))
						{
							// we are done streaming and the last sample has played
							donePlaying = true;
						}
					}
					else
					{
						// we are not streaming.
						if (bam_digi_done(pDigiSound->hSOSSampleHandle))
						{
							donePlaying = true;
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
						donePlaying = false;
						pDigiSound->PlayNextLink();
					}

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
							pDigiSound->fadeCallBack = grip{};
							pDigiSound->fadeTotalSteps = 0;
						}
					}
				}
			}
		}

		for (gMidiSong = ptr_to_grip(midiList.FirstValue());
			gMidiSong;
			gMidiSong = ptr_to_grip(midiList.NextValue()))
		{
			pMidiSong = ADerefAs(TMusic, gMidiSong);

			// only process songs that aren't paused
			if (!pMidiSong->pausedCnt)
			{
				if (bam_midi_done(pMidiSong->wSongHandle))
				{
					// this song is done.
					DeleteMidi(pMidiSong);

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
							pMidiSong->fadeCallBack = grip{};
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

	for (gDigiSound = ptr_to_grip(digiList.FirstValue());
		gDigiSound;
		gDigiSound = ptr_to_grip(digiList.NextValue()))
	{
		ADerefAs(TSound, gDigiSound)->Pause();
	}

	for (gMidiSong = ptr_to_grip(midiList.FirstValue());
		gMidiSong;
		gMidiSong = ptr_to_grip(midiList.NextValue()))
	{
		ADerefAs(TMusic, gMidiSong)->Pause();
	}
}

void
SoundMgr::Resume(void)
{
	grip		gDigiSound;
	grip		gMidiSong;

	for (gDigiSound = ptr_to_grip(digiList.FirstValue());
		gDigiSound;
		gDigiSound = ptr_to_grip(digiList.NextValue()))
	{
		ADerefAs(TSound, gDigiSound)->Resume();
	}

	for (gMidiSong = ptr_to_grip(midiList.FirstValue());
		gMidiSong;
		gMidiSong = ptr_to_grip(midiList.NextValue()))
	{
		ADerefAs(TMusic, gMidiSong)->Resume();
	}
}

// Set the volume for all music and sounds to be the
// fraction of total volume as established by the current step.
// the first current step should be 1 (not 0).  the last step
// should equal totalSteps.
// If the last step, true is returned.

bool
SoundMgr::Fade(uint16 direction, uint32 currentStep, uint32 totalSteps)
{
	bool	done = false;
	grip	gDigiSound;
	grip	gMidiSong;

	for (gDigiSound = ptr_to_grip(digiList.FirstValue());
		gDigiSound;
		gDigiSound = ptr_to_grip(digiList.NextValue()))
	{
		done = ADerefAs(TSound, gDigiSound)->Fade(direction, currentStep, totalSteps);
	}

	for (gMidiSong = ptr_to_grip(midiList.FirstValue());
		gMidiSong;
		gMidiSong = ptr_to_grip(midiList.NextValue()))
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

	for (gDigiSound = ptr_to_grip(digiList.FirstValue());
		gDigiSound;
		gDigiSound = ptr_to_grip(digiList.NextValue()))
	{
		ADerefAs(TSound, gDigiSound)->AutoFade(direction, totalSteps, stepDelay, callBack);
		// only set the callback for the first one
		callBack = grip{};
	}

	for (gMidiSong = ptr_to_grip(midiList.FirstValue());
		gMidiSong;
		gMidiSong = ptr_to_grip(midiList.NextValue()))
	{
		ADerefAs(TMusic, gMidiSong)->AutoFade(direction, totalSteps, stepDelay, callBack);
		// only set the callback for the first one
		callBack = grip{};
	}
}

void
SoundMgr::AddDigi(TSound *pSound, int sndNum, grip callBack)
{
	uint16		i;
	bool			dummyDriver = false;
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
			dummyDriver = true;

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
	if (digiList.Find(grip_to_ptr(pSound->gSelf)))
	{
		// this sound is in the play list right now.
		// shut it down
		ResMgrDebug.Out(" kill g%d", pSound->gSelf);
		DeleteDigi(pSound);
	}

	// now we set a few things in the TSound variables
	// (these couldn't be set before because of the possible
	//  call to DeleteDigi)
	pSound->resourceNum = sndNum;
	pSound->gToCue = callBack;
	pSound->cueData = 0;

	while(digiList.count >= MAX_DIGI_SOUNDS)
	{
		// delete the oldest sound.
		// since sounds are always added to the end of the list,
		// the first one in the list is the oldest
		gSound = ptr_to_grip(digiList.FirstValue());
		ResMgrDebug.Out(" kill2 g%d", pSound->gSelf);
		DeleteDigi(ADerefAs(TSound, gSound));
	}

	// load the sample
	// our sample is in WAV format.

	// clear the sample data structure
	memset(&pSound->sSOSSampleData, 0, sizeof(SampleData));

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
			snprintf(mess, sizeof(mess), "Not a WAV file: %d", pSound->resourceNum);
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

	// setup the sample data structure
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
			pSound->gStreamBuf = AMalloc(pSound->streamPlaySize);
			pSound->sSOSSampleData.lpSamplePtr = (char *) ADerefAs(char, pSound->gStreamBuf);

			// load the streaming buffer for the first time
			pSound->bytesPlayed =
				pSound->GetStreamer()->GetBytes(ADerefAs(char, pSound->gStreamBuf), pSound->streamPlaySize);

			pSound->sSOSSampleData.dwSampleSize = pSound->bytesPlayed;

			// streaming callback -- not wired into SDL3_mixer backend;
			// streaming sounds play only their first buffer
			pSound->sSOSSampleData.lpCallback = nullptr;
		}
	}
	else
	{
		pSound->sSOSSampleData.lpSamplePtr = (char *) pWavFile;
		pSound->sampleSize = pSound->sSOSSampleData.dwSampleSize = waveFileHeader.dataLength;
	}

	if (pSound->sSOSSampleData.dwSampleSize < 1)
	{
		// something is wrong.
		// let's get out of here
		ResMgrDebug.Out(" kill3 g%d", pSound->gSelf);
		DeleteDigi(pSound, false);
		return;
	}

	if (sampleRate > 22050)
	{
		snprintf(mess, sizeof(mess), "Sample rate too high: %d", sampleRate);
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
			snprintf(mess, sizeof(mess), "Stereo samples must be at 22050.  File %d", pSound->resourceNum);
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
	pSound->sSOSSampleData.wSampleID = pSound->gSelf.index;

	// find out what the time will be when the sample is done
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
		   	pSound->hSOSSampleHandle = bam_digi_start(&pSound->sSOSSampleData);
		}
	}


	// add to our list
	digiList.Add(grip_to_ptr(pSound->gSelf));
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
		// digi driver is active -- stop the sample
		bam_digi_stop(pSound->hSOSSampleHandle);
	}

	// free the sample resource
	if (pSound->gResource)
	{
		(ADerefAs(Resource, pSound->gResource))->Unlock();
		pSound->gResource = grip{};
	}

	// Close down the streamer stuff.  It's fine to call this
	// when not streaming.
	pSound->GetStreamer()->CloseSoundFile();

	if (pSound->gStreamBuf)
	{
		// we have a streaming buffer.  free it.
		AFree(pSound->gStreamBuf);
	}

	digiList.Delete(grip_to_ptr(pSound->gSelf));
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

	if (midiList.Find(grip_to_ptr(pMusic->gSelf)))
	{
		// this song is in the play list right now.
		// shut it down
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
		DeleteMidi(ADerefAs(TMusic, ptr_to_grip(midiList.FirstValue())));
	}

	// load the song
	pMusic->gResource = ALoad(RES_MIDI, pMusic->resourceNum);
	(ADerefAs(Resource, pMusic->gResource))->Lock();

	// initialize the song (HMP -> SMF -> SDL3_mixer)
	pMusic->wSongHandle = bam_midi_init((uint8_t*)AGetResData(pMusic->gResource));
	if (!pMusic->wSongHandle)
	{
		InitError(1, "sound card digi failure");
	}

	// start the song playing
	if (!pMusic->pausedCnt)
	{
		// if not paused, start the song
		bam_midi_start(pMusic->wSongHandle);
		bam_midi_set_song_vol(pMusic->wSongHandle, (uint32)pMusic->volume);
	}

	// add to our list
	midiList.Add(grip_to_ptr(pMusic->gSelf));
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
		// midi driver is active -- stop and uninit the song
		bam_midi_stop(pMusic->wSongHandle);
		bam_midi_uninit(pMusic->wSongHandle);
	}

	// free the sample resource
	if (pMusic->gResource)
	{
		(ADerefAs(Resource, pMusic->gResource))->Unlock();
		pMusic->gResource = grip{};
	}

	midiList.Delete(grip_to_ptr(pMusic->gSelf));
}

void
SoundMgr::SetMasterDigiVolume(int16 volume)
{
	if (!(sosActiveSystems & SOS_DIGI_DRIVER))
	{
		// digi driver was never inited
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
	if (s_mixer)
		MIX_SetMixerGain(s_mixer, (float)(digiVolume << 8) / (float)(127 << 8));
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
	bam_midi_set_master_vol((uint32)midiVolume);
}

int16
SoundMgr::GetMasterMidiVolume(void)
{
	return midiVolume;
}

bool
SoundMgr::SystemIsActive(uint16 flags)
{
	return (sosActiveSystems & flags) ? true : false;
}

// return true if the digi sound is in the play list
bool
SoundMgr::DigiIsPlaying(TSound *pSound)
{
	return (bool) (digiList.Find(grip_to_ptr(pSound->gSelf)));
}

// return true if the midi song is in the play list
bool
SoundMgr::MidiIsPlaying(TMusic *pMusic)
{
	return (bool) (midiList.Find(grip_to_ptr(pMusic->gSelf)));
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
	return ADerefAs(TSound, ptr_to_grip(digiList.NextValue()))->gSelf;
}

grip
SoundMgr::OldestDigiPlaying(void)
{
	return ADerefAs(TSound, ptr_to_grip(digiList.FirstValue()))->gSelf;
}

grip
SoundMgr::OldestMidiPlaying(void)
{
	return ADerefAs(TMusic, ptr_to_grip(midiList.FirstValue()))->gSelf;
}

bool
SoundMgr::Save(uint16 state, nlohmann::json& root)
{
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

		case AFTER_SAVE:
			// resume all sounds
			Resume();
			break;

		case BEFORE_RESTORE:
			// stop all sounds
			for (gDigiSound = ptr_to_grip(digiList.FirstValue());
				gDigiSound;
				gDigiSound = ptr_to_grip(digiList.NextValue()))
			{
				pDigiSound = ADerefAs(TSound, gDigiSound);
				DeleteDigi(pDigiSound, false);
			}

			// stop all songs
			for (gMidiSong = ptr_to_grip(midiList.FirstValue());
				gMidiSong;
				gMidiSong = ptr_to_grip(midiList.NextValue()))
			{
				pMidiSong = ADerefAs(TMusic, gMidiSong);
				DeleteMidi(pMidiSong, false);
			}
			break;

		case AFTER_RESTORE:
			uint32	bytesRetrieved;

			// setup all sounds and music to be able to restart when Resume is
			// called.
			// if a digi driver was present during save, but not
			// present during restore, convert sounds to be timer based.

			// setup sounds
			for (gDigiSound = ptr_to_grip(digiList.FirstValue());
				gDigiSound;
				gDigiSound = ptr_to_grip(digiList.NextValue()))
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
								digiList.Delete(grip_to_ptr(pDigiSound->gSelf));
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

							pDigiSound->sSOSSampleData.lpSamplePtr = (char *) ADerefAs(char, pDigiSound->gStreamBuf);

      					// load the play buffer
      					bytesRetrieved =
								pDigiSound->GetStreamer()->GetBytes(ADerefAs(char, pDigiSound->gStreamBuf),
									pDigiSound->streamPlaySize);

							pDigiSound->bytesPlayed += bytesRetrieved;
							pDigiSound->sSOSSampleData.dwSampleSize = bytesRetrieved;

							// streaming callback not wired in SDL3_mixer backend
							pDigiSound->sSOSSampleData.lpCallback = nullptr;
						}
					}
				}
			}

			// setup songs
			for (gMidiSong = ptr_to_grip(midiList.FirstValue());
				gMidiSong;
				gMidiSong = ptr_to_grip(midiList.NextValue()))
			{
				pMidiSong = ADerefAs(TMusic, gMidiSong);

				pMidiSong->gResource = ALoad(RES_MIDI, pMidiSong->resourceNum);

				// initialize the song (HMP -> SMF -> SDL3_mixer)
				pMidiSong->wSongHandle = bam_midi_init(
				    (uint8_t*)AGetResData(pMidiSong->gResource));
				if (pMidiSong->wSongHandle)
				{
					// start the song
					bam_midi_start(pMidiSong->wSongHandle);
					bam_midi_set_song_vol(pMidiSong->wSongHandle, (uint32)pMidiSong->volume);

					// pause the song (Resume() will unpause)
					bam_midi_pause(pMidiSong->wSongHandle);
				}
			}

			Resume();

			break;
	}
	return false;
}

// do everything that is necessary to shut down the sound manager
void
ShutDownSoundMgr(void)
{
	ShutDownSoundMgrNow(pSoundMgr);
}

// C-linkage entry point called from OS_Quit() in asm_stubs.cpp to ensure
// audio is cleaned up before SDL_Quit() tears down the audio device.
extern "C" void bam_audio_quit(void) { ShutDownSoundMgr(); }

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
		for (gDigiSound = ptr_to_grip(digiList.FirstValue());
			gDigiSound;
			gDigiSound = ptr_to_grip(digiList.NextValue()))
		{
			pDigiSound = ADerefAs(TSound, gDigiSound);

			// delete this sound with no cue
			DeleteDigi(pDigiSound, false);
		}

		// stop all songs
		for (gMidiSong = ptr_to_grip(midiList.FirstValue());
			gMidiSong;
			gMidiSong = ptr_to_grip(midiList.NextValue()))
		{
			if(pMemMgr->CheckGrip(gMidiSong) != GRIP_VALID)
			{
				pMono->Out("SoundMgr::ShutDown() WARNING! gMidiSong %d invalid!\n",
					gMidiSong);
				continue;
			}

			pMidiSong = ADerefAs(TMusic, gMidiSong);

			// delete this song with no cue
			DeleteMidi(pMidiSong, false);
		}

		// shut down all MIDI slots
		if (sosActiveSystems & SOS_MIDI_SYS_DRIVER)
		{
			for (int i = 0; i < MAX_MIDI_SLOTS; ++i)
				if (s_midi_slots[i].active) midi_free_slot(i);
		}

		// shut down digital audio
		if (sosActiveSystems & SOS_DIGI_DRIVER)
		{
			for (int i = 0; i < MAX_SLOTS; ++i)
				if (s_slots[i].active) free_slot(i);
		}
		if (sosActiveSystems & SOS_DIGI_SYS)
		{
			if (s_mixer) {
				MIX_DestroyMixer(s_mixer);
				s_mixer = nullptr;
			}
			MIX_Quit();
		}

		sosActiveSystems = 0;

		// set the devices to off
		digiDevice = midiDevice = SOUND_OFF;

		// clean up all the loaded resources
		if (gInstruments)
		{
			AFlush(gInstruments);
			gInstruments = grip{};
		}
		if (gDrums)
		{
			AFlush(gDrums);
			gDrums = grip{};
		}

		if (gDigiInstruments)
		{
			AFlush(gDigiInstruments);
			gDigiInstruments = grip{};
		}

		if (gMT32Patch)
		{
			AFlush(gMT32Patch);
			gMT32Patch = grip{};
		}
	}
}

//-----------------------------------------------------------------------
//
// SOS_Base
//
//-----------------------------------------------------------------------

SOS_Base::SOS_Base(void)
{
	gResource = grip{};
	pausedCnt = 0;
	fadeTotalSteps = 0;
	fadeCallBack = grip{};
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
		fadeCallBack = grip{};
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
	streaming = false;
	gStreamBuf = grip{};
	panPosition = DIGI_PAN_CENTER;

	// This is how many bytes will be played before the stream
	// is continued.  The default number is what the lip-syncing
	// currently requires.  If you want to change this, change it
	// before you call Play().
	streamPlaySize = 184 * 2;

	bytesPerSample = 1;
	bytesPlayed = 0;

	// init the sample data structure to all zeros
	memset(&sSOSSampleData, 0, sizeof(SampleData));
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
			pSoundMgr->DeleteDigi(this, false);
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
	memmove(&resChain[0], &resChain[1], sizeof(resChain) - sizeof(resChain[0]));
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
		cueData = 0;
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

		// only call audio stuff if digi driver has been inited
		if (pSoundMgr->SystemIsActive(SOS_DIGI_DRIVER))
		{
			bam_digi_set_volume(hSOSSampleHandle, (volume << 8));
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
			pSoundMgr->DeleteDigi(this, false);
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
				if (streaming == false)
				{
					if (sSOSSampleData.wSampleFlags & _LOOPING)
					{
						// looping sound.  restart at the beginning when Resume
						// is called
						bytesPlayed = 0;
					}
					else
					{
						// single play sound -- SDL3_mixer doesn't expose
						// bytes-processed, so we use the timer difference instead
						bytesPlayed += (uint32)((ATicks() - startTicks) * bytesPerTick);
					}
				}

	   		bam_digi_stop(hSOSSampleHandle);
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

			// if the digi driver is active, resume digi
			if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_DIGI_DRIVER)))
			{
				if (waitTimer == 0)
				{
					// if we are streaming, everything is already set to go
					if (streaming == false)
					{
						// must be a non-streaming sample
						// fast forward the pointer and adjust a few things
						sSOSSampleData.lpSamplePtr = (char *) (AGetResData(gResource) + sizeof(WaveFileHeader) + bytesPlayed);
						sSOSSampleData.dwSampleSize = sampleSize - bytesPlayed;
					}

   				hSOSSampleHandle = bam_digi_start(&sSOSSampleData);
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
			return true;
		}
	}

	// adjust the current step so that we can use the same code
	// for either direction
	if (direction == SNDMGR_FADE_DOWN)
	{
		currentStep = totalSteps - currentStep;
	}

	SetVolume((int16) ((volumeMax * currentStep)/totalSteps), panPosition, false);

	if (direction == SNDMGR_FADE_UP && currentStep == totalSteps)
	{
		// last step
		return true;
	}
	else
	{
		return false;
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

		bam_digi_set_pan(hSOSSampleHandle,
		    pSoundMgr->GetPanPosition(panPosition));
	}
}


TStreamBase*
TSound::GetStreamer()
{
	return	&streamer;
}

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
			pSoundMgr->DeleteMidi(this, false);
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
			pSoundMgr->DeleteMidi(this, false);
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
	pausedCnt++;

	// only do the follow stuff if this is the first time that
	// we are going to pause
	if (pausedCnt == 1)
	{
		// save the paused ticks so we can fix time based things in resume
		pausedTime = ATicks();

		// if the music driver is active, pause all songs
		if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_MIDI_SYS_DRIVER)))
		{
			bam_midi_pause(wSongHandle);
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

			ticksDiff = ATicks() - pausedTime;
			(void)ticksDiff;

			// if the music driver is active, resume all songs
			if (pSoundMgr && (pSoundMgr->SystemIsActive(SOS_MIDI_SYS_DRIVER)))
			{
				bam_midi_resume(wSongHandle);
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

		// only call audio stuff if midi driver has been inited
		if (pSoundMgr->SystemIsActive(SOS_MIDI_SYS_DRIVER))
		{
			bam_midi_set_song_vol((uint32)wSongHandle, (uint32)volume);
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
			return true;
		}
	}

	// adjust the current step so that we can use the same code
	// for either direction
	if (direction == SNDMGR_FADE_DOWN)
	{
		currentStep = totalSteps - currentStep;
	}

	SetVolume((int16) ((volumeMax * currentStep)/totalSteps), DIGI_PAN_CENTER, false);

	if (direction == SNDMGR_FADE_UP && currentStep == totalSteps)
	{
		// last step
		return true;
	}
	else
	{
		return false;
	}
}
