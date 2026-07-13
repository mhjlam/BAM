/*
 * compat/portability.h
 *
 * Force-included in every translation unit (via -include compiler flag).
 */

#ifndef COMPAT_PORTABILITY_H
#define COMPAT_PORTABILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <SDL3/SDL.h>

/* OS_Quit() — shuts down audio + SDL then calls _exit(0).
 * Defined in sdl_backend.cpp.  Call instead of returning from main() or
 * calling exit() to avoid LLVM JIT static-destructor crashes. */
#ifdef __cplusplus
extern "C++" [[noreturn]] void OS_Quit(void);
#else
[[noreturn]] void OS_Quit(void);
#endif

#endif /* COMPAT_PORTABILITY_H */
