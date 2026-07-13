// savestream.hpp
//
// Bidirectional save/restore stream for explicit field-by-field serialisation.
// Direction (save vs restore) is set at construction; the same sync() calls
// work for both directions, removing the need for paired fwrite/fread blocks.
//
// Usage:
//   SaveStream ss{fp, true};    // saving
//   SaveStream ss{fp, false};   // restoring
//   ss.sync(myInt32);
//   ss.syncGrip(myGrip);        // grips stored as uint16 on disk
//   ss.sentinel();              // integrity check between subsections

#ifndef savestream_hpp
#define savestream_hpp

#include <stdio.h>
#include <stdint.h>
#include "types.hpp"    // int8, uint8, int16, uint16, int32, uint32, ticks_t, grip

// Note on types: int32 = int, uint32 = unsigned int, grip = unsigned int.
// They are the same underlying types, so we only overload on distinct types.

struct SaveStream {
    FILE *fp;
    bool  saving;

    // ---- 1-byte ----
    void sync(bool   &v) { saving ? (void)fwrite(&v,1,1,fp) : (void)fread(&v,1,1,fp); }
    void sync(int8   &v) { saving ? (void)fwrite(&v,1,1,fp) : (void)fread(&v,1,1,fp); }
    void sync(uint8  &v) { saving ? (void)fwrite(&v,1,1,fp) : (void)fread(&v,1,1,fp); }

    // ---- 2-byte ----
    void sync(int16  &v) { saving ? (void)fwrite(&v,2,1,fp) : (void)fread(&v,2,1,fp); }
    void sync(uint16 &v) { saving ? (void)fwrite(&v,2,1,fp) : (void)fread(&v,2,1,fp); }

    // ---- 4-byte (int, unsigned int — covers int32, uint32, ticks_t, uint, grip) ----
    void sync(int    &v) { saving ? (void)fwrite(&v,4,1,fp) : (void)fread(&v,4,1,fp); }
    void sync(unsigned int &v) { saving ? (void)fwrite(&v,4,1,fp) : (void)fread(&v,4,1,fp); }

    // ---- 8-byte (Uint64, uint64_t, int64_t) ----
    void sync(int64_t  &v) { saving ? (void)fwrite(&v,8,1,fp) : (void)fread(&v,8,1,fp); }
    void sync(uint64_t &v) { saving ? (void)fwrite(&v,8,1,fp) : (void)fread(&v,8,1,fp); }

    // ---- grip: only slot index is persisted; generation is runtime-only ----
    void syncGrip(grip &g) {
        uint16 w = g.index;
        saving ? (void)fwrite(&w,2,1,fp) : (void)fread(&w,2,1,fp);
        if (!saving) g = grip{w, 0};
    }

    // ---- Raw bytes — for flat POD blocks known to contain no pointers ----
    void syncBytes(void *p, size_t n) {
        saving ? (void)fwrite(p,1,n,fp) : (void)fread(p,1,n,fp);
    }

    // ---- Fixed-length char array ----
    void syncStr(char *s, size_t maxLen) {
        syncBytes(s, maxLen);
    }

    // ---- Sentinel: written after each subsystem block to detect misalignment ----
    void sentinel(const char* tag = "") {
        uint16 val = 0xBABE;
        if (saving) {
            fwrite(&val, 2, 1, fp);
        } else {
            uint16 got = 0;
            fread(&got, 2, 1, fp);
            if (got != 0xBABE) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                    "BAM save: sentinel mismatch [%s] (got 0x%04x, expected 0xBABE)"
                    " at file offset %ld", tag, (unsigned)got, ftell(fp));
                abort();
            }
        }
    }
};

#endif // savestream_hpp
