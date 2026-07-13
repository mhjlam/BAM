//
// OS.CPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Non-portable global API calls
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "mono.hpp"
#include "os.hpp"
#include "types.hpp"

#include <SDL3/SDL.h>
#include <errno.h>


// because we have things in here that may be called during an ,
// turn stack checking off

// debugging mesage file
const char*	szLastMessage = nullptr;




void
_Panic(const char* msg, const char* fileName, int lineNum)
{
	char	err[256];

	static bool fExitInProcess = false;

	if (!fExitInProcess)
	{
		// set check for double fault
		fExitInProcess = true;

		if(pMono)
		{
			// dump contents of monochrome memory to disk
			pMono->Snapshot();
		}
		AShutDownVideo();

		// inhibit un-needed messages
		APrintUnfreedPtrs(false);

		// just in case this is helpful (and it is sometimes, really (jc))
		if (errno && pMono)
		{
			pMono->Out("\n");
			pMono->Out(strerror(errno));
		}

		snprintf(err, sizeof(err), "\n%s\n\tat line %d, file %s\n", msg, lineNum, fileName);
		if (pMono)
		{
			pMono->Out(err);
		}

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err);

		if (pMono)
		{
			pMono->Out("\n");
		}

		//PanicShutDown();
		exit(1);
	}
	else 
	{
		// error during shutdown,
		// exit without attempting shutdown
		if (pMono)
		{
			pMono->Out("DOUBLE FAULT, we advise rebooting\n");
		}
		abort();
	}
}




#include <sys/stat.h>

uint32
AFileLastAccessTime (char* fn)
{		
	struct stat fileStat;

	stat(fn, &fileStat);
	return (uint32) fileStat.st_atime;
}

uint32
AFileLastModTime (char* fn)
{
	struct stat fileStat;

	stat(fn, &fileStat);
	return (uint32) fileStat.st_mtime;
}


//--------------------------------------------------------------
// Declared in ApiGraph.hpp
//--------------------------------------------------------------

// Create a bitmap
bitmap_t
ACreateBitMap(coord w, coord h)
{
	return AMalloc(w * h);
}


// Destroy a bitmap
void
ADestroyBitMap(bitmap_t bm)
{
	if (bm)
	{
		AFree(bm);
	}
}


// Return a pointer to a bitmap
uchar*
ADerefBitMap(bitmap_t bm)
{
	return ADerefAs(uchar, bm);
}


//--------------------------------------------------------------
// Declared in ApiMem.hpp
//--------------------------------------------------------------


// do platform specific memory initialization
//
void
OS_InitMem()
{
	// No-op on Linux — virtual memory is managed by the kernel.
}

size_t
ALargestAlloc()
{
	return SIZE_MAX;
}


size_t
AAvailMem()
{
	return SIZE_MAX;
}


// Determine if address is on the stack or not. Used by MemMgr optimizations.
bool
AIsOnStack(void* /*p*/)
{
	// Not implemented on Linux — MemMgr optimizations that rely on this
	// simply take the conservative path.
	return false;
}




//--------------------------------------------------------------
// Declared in ApiEvt.hpp
//--------------------------------------------------------------

// Convert a scan code to an ASCII character
//
char
AScanToASCII(Event e)
{
	return pEventMgr->ScanToASCII(e);
}


// Get event modifiers
//
uint16
AGetEventMods()
{
	/* Read modifier state from the scanKeys[] array populated by the SDL event loop. */
	if (!pEventMgr) return 0;
	uint16 mods = 0;
	if (pEventMgr->KeyIsDown(K_CTRL))
		mods |= MOD_CTRL;
	if (pEventMgr->KeyIsDown(K_ALT))
		mods |= MOD_ALT;
	if (pEventMgr->KeyIsDown(K_RIGHT_SHIFT) || pEventMgr->KeyIsDown(K_LEFT_SHIFT))
		mods |= MOD_SHIFT;
	return mods;
}



