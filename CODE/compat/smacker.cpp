/*
 * smacker.cpp
 *
 * libsmacker wrapper implementing the RAD Smacker API for Linux.
 *
 * Replaces the no-op stubs in lib_stubs.cpp.  flicsmk.cpp sees Smack*
 * pointers; we return SmkWrap* cast to Smack*.  SmkWrap embeds Smack as
 * its first member so the cast is safe and all field accesses are correct.
 *
 * Pipeline (per frame in flicsmk.cpp::Play()):
 *   SmackDoFrame      → decode frame directly into dst_buf (pDecBuf); update palette
 *   SmackToBufferRect → no-op (full frame already in dst_buf)
 *   AFBlit            → saves pDecBuf pointer; does NOT copy yet  (asm_stubs.cpp)
 *   ChangePalette     → reads smk->Palette → guns[], sets activatePalette
 *   ChangeText        → renders subtitle text into pDecBuf  ← text is now in pDecBuf
 *   SVGASetPalette    → OS_SetPalette(guns, 0, 255) → s_palette  (asm_stubs.cpp)
 *   XModeFlipPage     → memcpy saved pDecBuf → pVGAMem (text included), then
 *                       indexed→RGB24→SDL3 texture→present  (asm_stubs.cpp)
 *   SmackNextFrame    → smk_next()
 *   SmackWait         → frame deadline wait
 */

#include "smack.h"       /* Smack struct, SMACKTRACK1..7                        */
#include <smacker.h>     /* smk, smk_open_file, smk_first, smk_next, etc.      */
#include <SDL3/SDL.h>    /* SDL_GetTicks, SDL_Delay, SDL_AudioStream, etc.      */
#include <string.h>      /* memcpy, memcmp                                       */
#include <stdlib.h>      /* malloc, free                                         */
#include <stdio.h>       /* fprintf, FILE                                        */

/* ci_fopen() from file.cpp — case-insensitive fopen for Linux filesystems. */
extern FILE* ci_fopen(const char* path, const char* mode);

/* ================================================================
 * Internal structs
 * ================================================================ */

struct SmkAudioTrack {
    SDL_AudioStream* stream;   /* opened lazily on first audio chunk */
    unsigned char    channels;
    unsigned char    bitdepth;
    unsigned long    rate;
    int              enabled;
};

struct SmkWrap {
    Smack         pub;           /* MUST be first — game code casts SmkWrap* ↔ Smack* */
    smk           lib;           /* libsmacker opaque handle                            */
    void*         dst_buf;       /* pixel buffer registered by SmackToBuffer (= pDecBuf)*/
    unsigned long dst_pitch;     /* = 320                                               */
    unsigned long dst_height;    /* = 400                                               */
    unsigned char y_scale_mode;  /* SMK_FLAG_Y_NONE / SMK_FLAG_Y_DOUBLE                */
    double        usf;           /* microseconds per frame (from smk_info_all)          */
    Uint64        frame_deadline_ms; /* SDL_GetTicks() deadline for next SmackWait      */
    float         volume;        /* 0.0–1.0, mapped from SmackVolumePan volume arg      */
    int           sound_on;      /* toggled by SmackSoundOnOff                          */
    SmkAudioTrack audio[7];
};

static inline SmkWrap* wrap_of(Smack* s) { return reinterpret_cast<SmkWrap*>(s); }

/* ================================================================
 * Audio helper — lazy-opens one SDL_AudioStream per track
 * ================================================================ */

static void smacker_queue_audio(SmkWrap* w, int t,
                                const unsigned char* data, unsigned long sz)
{
    SmkAudioTrack* tr = &w->audio[t];

    if (!tr->stream) {
        SDL_AudioSpec spec;
        spec.channels = (int)tr->channels;
        spec.freq     = (int)tr->rate;
        spec.format   = (tr->bitdepth == 16) ? SDL_AUDIO_S16 : SDL_AUDIO_U8;

        tr->stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!tr->stream) {
            fprintf(stderr, "smacker: SDL_OpenAudioDeviceStream track %d: %s\n",
                    t, SDL_GetError());
            tr->enabled = 0;
            return;
        }
        SDL_ResumeAudioStreamDevice(tr->stream);
    }

    SDL_SetAudioStreamGain(tr->stream, w->volume);
    SDL_PutAudioStreamData(tr->stream, data, (int)sz);
}

/* ================================================================
 * SmackOpen
 * ================================================================ */

extern "C"
Smack* SmackOpen(char* name, uint32_t flags, uint32_t /*extrabuf*/)
{
    /* Use ci_fopen for case-insensitive lookup (game files are e.g. "15.SMK"). */
    FILE* fp = ci_fopen(name, "rb");
    if (!fp) {
        fprintf(stderr, "SmackOpen: ci_fopen(%s) failed\n", name);
        return 0;
    }
    smk lib = smk_open_filepointer(fp, SMK_MODE_DISK);
    if (!lib) {
        fprintf(stderr, "SmackOpen: smk_open_filepointer(%s) failed\n", name);
        fclose(fp);
        return 0;
    }

    SmkWrap* w = (SmkWrap*) calloc(1, sizeof(SmkWrap));
    if (!w) { smk_close(lib); return 0; }

    w->lib      = lib;
    w->sound_on = 1;
    w->volume   = 1.0f;

    /* Video dimensions and frame info */
    unsigned long vid_w = 320, vid_h = 200, frame_count = 0;
    smk_info_video(lib, &vid_w, &vid_h, &w->y_scale_mode);
    smk_info_all(lib, nullptr, &frame_count, &w->usf);

    w->pub.Width    = (uint32_t)vid_w;
    /* Report post-scale height so callers know the output dimensions */
    w->pub.Height   = (uint32_t)((w->y_scale_mode == SMK_FLAG_Y_DOUBLE) ? vid_h * 2 : vid_h);
    w->pub.Frames   = (uint32_t)frame_count;
    w->pub.MSPerFrame = (uint32_t)(w->usf / 1000.0);

    /* Enable video track */
    smk_enable_video(lib, 1);

    /* Enable requested audio tracks (SMACKTRACK1=0x2000, SMACKTRACK2=0x4000, …) */
    unsigned char track_mask = 0;
    unsigned char channels[7] = {};
    unsigned char bitdepth[7] = {};
    unsigned long rate[7]     = {};
    smk_info_audio(lib, &track_mask, channels, bitdepth, rate);

    for (int t = 0; t < 7; t++) {
        unsigned long flag = (unsigned long)(SMACKTRACK1) << t;
        if ((track_mask & (unsigned char)(1u << t)) && (flags & flag)) {
            w->audio[t].channels = channels[t];
            w->audio[t].bitdepth = bitdepth[t];
            w->audio[t].rate     = rate[t];
            w->audio[t].enabled  = 1;
            smk_enable_audio(lib, (unsigned char)t, 1);
        }
    }

    /* Prime the first frame so smk_get_video/palette return valid data.
     * Do NOT copy the palette into pub.Palette here — leave it zeroed.
     * SmackDoFrame will detect the zero→real difference and set NewPalette=1,
     * so the game's frame-1 palette check fires correctly. */
    if (smk_first(lib) == SMK_ERROR) {
        fprintf(stderr, "SmackOpen: smk_first failed\n");
        free(w);
        smk_close(lib);
        return 0;
    }

    w->pub.NewPalette = 1;
    w->pub.FrameNum   = 1;

    w->frame_deadline_ms = SDL_GetTicks() + (Uint64)(w->usf / 1000.0);

    return &w->pub;
}

/* ================================================================
 * SmackToBuffer — register the pixel destination buffer
 * ================================================================ */

extern "C"
void SmackToBuffer(Smack* smk, uint32_t /*left*/, uint32_t /*top*/,
                   uint32_t pitch, uint32_t destheight,
                   void* buf, uint32_t /*Reversed*/)
{
    SmkWrap* w = wrap_of(smk);
    w->dst_buf    = buf;
    w->dst_pitch  = pitch;
    w->dst_height = destheight;
}

/* ================================================================
 * SmackDoFrame — decode current frame into dst_buf; update palette
 * ================================================================ */

extern "C"
uint32_t SmackDoFrame(Smack* smk)
{
    SmkWrap* w = wrap_of(smk);

    /* Palette: detect changes and update pub.Palette / pub.NewPalette */
    const unsigned char* pal = smk_get_palette(w->lib);
    if (pal) {
        if (memcmp(w->pub.Palette, pal, 768) != 0) {
            memcpy(w->pub.Palette, pal, 768);
            w->pub.NewPalette = 1;
        } else {
            w->pub.NewPalette = 0;
        }
    }

    /* Decode video frame directly into dst_buf (pDecBuf).
     * AFBlit defers the copy to pVGAMem until XModeFlipPage, so ChangeText
     * (which runs after AFBlit) can write subtitles into dst_buf first. */
    if (w->dst_buf) {
        const unsigned char* frame = smk_get_video(w->lib);
        if (frame) {
            unsigned long src_w = w->pub.Width;
            unsigned long src_h = (w->y_scale_mode == SMK_FLAG_Y_DOUBLE)
                                  ? w->pub.Height / 2
                                  : w->pub.Height;
            unsigned long dp    = w->dst_pitch;
            unsigned long dh    = w->dst_height;
            unsigned char* dst  = (unsigned char*)w->dst_buf;

            if (w->y_scale_mode == SMK_FLAG_Y_DOUBLE) {
                for (unsigned long row = 0; row < src_h && (row*2+1) < dh; row++) {
                    const unsigned char* src_row = frame + row * src_w;
                    memcpy(dst + (row*2)   * dp, src_row, src_w);
                    memcpy(dst + (row*2+1) * dp, src_row, src_w);
                }
            } else {
                for (unsigned long row = 0; row < src_h && row < dh; row++) {
                    memcpy(dst + row * dp, frame + row * src_w, src_w);
                }
            }
        }
    }

    /* Audio: queue PCM for all enabled tracks */
    if (w->sound_on) {
        for (int t = 0; t < 7; t++) {
            if (!w->audio[t].enabled) continue;
            const unsigned char* adata = smk_get_audio(w->lib, (unsigned char)t);
            unsigned long asz = smk_get_audio_size(w->lib, (unsigned char)t);
            if (adata && asz > 0)
                smacker_queue_audio(w, t, adata, asz);
        }
    }

    return 0;
}

/* ================================================================
 * SmackToBufferRect — no-op; full frame already in dst_buf from SmackDoFrame.
 * Returns 0 to end the game's while(SmackToBufferRect...) loop.
 * ================================================================ */

extern "C"
uint32_t SmackToBufferRect(Smack* /*smk*/, uint32_t /*SmackSurface*/)
{
    return 0;
}

/* ================================================================
 * SmackNextFrame — advance to the next frame
 * ================================================================ */

extern "C"
void SmackNextFrame(Smack* smk)
{
    SmkWrap* w = wrap_of(smk);
    smk_next(w->lib);
    w->pub.FrameNum++;
    /* Advance the per-frame deadline by one frame interval */
    w->frame_deadline_ms += (Uint64)(w->usf / 1000.0);
}

/* ================================================================
 * SmackWait — block until it's time to display the next frame
 *
 * Returns non-zero while still waiting; 0 when the deadline has passed.
 * The game loop calls APublishNext() / MouseHandler() between calls.
 * ================================================================ */

extern "C"
uint32_t SmackWait(Smack* smk)
{
    SmkWrap* w = wrap_of(smk);
    Uint64 now = SDL_GetTicks();
    if (now >= w->frame_deadline_ms)
        return 0;
    SDL_Delay(1);   /* 1 ms yield — avoids spinwait, game loop pumps events */
    return 1;
}

/* ================================================================
 * SmackClose
 * ================================================================ */

extern "C"
void SmackClose(Smack* smk)
{
    SmkWrap* w = wrap_of(smk);
    for (int t = 0; t < 7; t++) {
        if (w->audio[t].stream) {
            SDL_DestroyAudioStream(w->audio[t].stream);
            w->audio[t].stream = nullptr;
        }
    }
    smk_close(w->lib);
    free(w);
}

/* ================================================================
 * SmackVolumePan — volume and pan control
 * ================================================================ */

extern "C"
void SmackVolumePan(Smack* smk, uint32_t /*trackflag*/,
                    uint32_t volume, uint32_t /*pan*/)
{
    SmkWrap* w = wrap_of(smk);
    /* RAD volume range: 0–65536 */
    w->volume = (float)volume / 65536.0f;
}

/* ================================================================
 * SmackSoundOnOff — global audio mute
 * ================================================================ */

extern "C"
uint32_t SmackSoundOnOff(Smack* smk, uint32_t on)
{
    SmkWrap* w = wrap_of(smk);
    w->sound_on = (int)on;
    return on;
}

/* ================================================================
 * SmackSimulate — CD-ROM speed simulation (no-op on Linux)
 * ================================================================ */

extern "C"
void SmackSimulate(uint32_t /*sim*/)
{
}
