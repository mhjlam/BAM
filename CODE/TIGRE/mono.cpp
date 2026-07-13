//
// MONO.CPP
//
// Simplified for Linux port: the DOS monochrome display buffer has been
// removed.  Out() routes through SDL_Log when g_verbose is true.
//

#include <stdarg.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "mono.hpp"

Mono* pMono    = nullptr;
bool  g_verbose = false;

void
Mono::Out(const char* fmt, ...)
{
	if (!g_verbose)
		return;
	va_list args;
	char msg[512];
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	// SDL_Log appends its own newline; strip trailing one to avoid blank lines
	size_t len = strlen(msg);
	if (len > 0 && msg[len - 1] == '\n')
		msg[len - 1] = '\0';
	SDL_Log("%s", msg);
}
