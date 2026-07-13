#ifndef __RAD__
#define __RAD__

// Simplified Linux-only rad.h.
// Original was a 209-line 1994 multi-platform header (DOS/Win16/Win32/Mac/PowerMac).
// All dead-platform code removed: far-pointer macros (PTR4), ASM calling-convention
// shims (RADLINK/RCSTART/etc.), and the u8/u16/u32 typedefs are all gone.
// Use <stdint.h> types (uint8_t, uint16_t, uint32_t) directly.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void* radmalloc(uint32_t numbytes);
void  radfree(void* ptr);
#ifdef __cplusplus
}
#endif

#endif
