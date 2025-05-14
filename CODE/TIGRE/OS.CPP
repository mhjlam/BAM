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
#include "dpmi.hpp"
#include "mono.hpp"
#include "os.hpp"
#include "types.hpp"

#ifndef	OS_DOS
	#error	This is a DOS only module!
#endif

#include <dos.h>
#include	<i86.h>
#include	<malloc.h>


// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

// debugging mesage file
char*	szLastMessage = NULL;




void
_Panic(char* msg, char* fileName, int lineNum)
{
	char	err[256];

	static bool fExitInProcess = FALSE;

	if (!fExitInProcess)
	{
		// set check for double fault
		fExitInProcess = TRUE;

		if(pMono)
		{
			// dump contents of monochrome memory to disk
			pMono->Snapshot();
		}
		AShutDownVideo();

		// inhibit un-needed messages
		APrintUnfreedPtrs(FALSE);

		// just in case this is helpful (and it is sometimes, really (jc))
		if (errno && pMono)
		{
			pMono->Out("\n");
			pMono->Out(strerror(errno));
		}

		sprintf (err, "\n%s\n\tat line %d, file %s\n", msg, lineNum, fileName);
		if (pMono)
		{
			pMono->Out(err);
		}

		#ifndef OS_MAC
			printf(err);
		#endif

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


// Platform-specific initializations
void
AInitializePlatform()
{
	// Just a stub for Mac mostly
}



#include <sys\stat.h>

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
	TRACK_MEM("OS_CreateBitmap");
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
	// WATCOM DOS ONLY!
	//
	// since dpmi and malloc have separate pools of mem, alloc as much
	// as we can so that malloc can have it all.
	// Leave 100K for HMI Sound Operating System
	// 
	size_t	maxAlloc = ALargestAlloc() - (100 * 1024);
	void	*pMaxAlloc = NULL;
	while(!pMaxAlloc)
	{
		pMaxAlloc = malloc(maxAlloc);
		maxAlloc -= 4 * 1024; 
	}
	if (pMaxAlloc)
	{
		free(pMaxAlloc);
	}
}

size_t
ALargestAlloc()
{
	union
	{
		REGS	inRegs;
		REGS	outRegs;
	};
	SREGS	sRegs;
	DPMIMemInfo		infoBuffer;

	memset(&sRegs, 0, sizeof(SREGS));
	memset(&outRegs, 0, sizeof(REGS));

	inRegs.x.eax = 0x0500;
	sRegs.es = FP_SEG((void far*) &infoBuffer);
	inRegs.x.edi = FP_OFF((void far*) &infoBuffer);
	int386x(INT_DPMI, &inRegs, &outRegs, &sRegs);

	size_t	largestBlock = _memmax();
	if (largestBlock < infoBuffer.largestBlock)
	{
		largestBlock = infoBuffer.largestBlock;
	}
	return largestBlock;
}


size_t
AAvailMem()
{
	union
	{
		REGS	inRegs;
		REGS	outRegs;
	};
	SREGS	sRegs;
	DPMIMemInfo		infoBuffer;
	
	memset(&sRegs, 0, sizeof(SREGS));
	memset(&outRegs, 0, sizeof(REGS));

	inRegs.x.eax = 0x0500;
	sRegs.es = FP_SEG((void far*) &infoBuffer);
	inRegs.x.edi = FP_OFF((void far*) &infoBuffer);
	int386x(INT_DPMI, &inRegs, &outRegs, &sRegs);

	size_t	availMem2 = infoBuffer.freePages * 4096;
	size_t	availMem = _memavl();
	if (availMem < availMem2)
	{
		availMem = availMem2;
	}
	return availMem;
}


// Assembly routine prototype (OSGRPH.ASM). This function will need to be
//	implemented differently for different memory models and os's.
//
extern "C"	void*	OS_GetStackPtr();

// Determine if address is on the stack for not. Used by MemMgr optimizations.
bool
AIsOnStack(void* p)
{
#ifdef	ABLE_TO_LINK

	// this version is better, but i can't get it to link! jc.
	// BUGBUG: 	Watcom only
	extern void*	__STACKTOP;
	extern void*	__STACKLOW;

	if ( (p <= __STACKTOP) && (p > __STACKLOW) )
	{
		return TRUE;
	}
	return FALSE;

#else

	// hi and low stack addresses. (hi is bottom of stack, 
	// lo is top of decrementing stack)
	static char*	pStackHi = NULL;
	static char*	pStackLo = NULL;


	if (pStackHi == NULL)
	{
		// init stack limits
		pStackHi = (char*) OS_GetStackPtr();

		pStackLo = pStackHi - stackavail();
	}

	if ( (p <= pStackHi) && (p > pStackLo) )
	{
		return TRUE;
	}

	return FALSE;

#endif
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


// Get pointers to bios variables (only useful in DOS)
//
uchar*
AGetBiosKey()
{
 	DPMI	dpmi;
 	return((uchar*) dpmi.RealToProtected(0x00400017));
}


// Update the event queue (only useful in Mac)
//
void
AQueueUpdate()
{
}


// Get event modifiers
//
uint16
AGetEventMods()
{
//	// NEW KEYBOARD MODS
//
//	uint16	mods = 0;
//
//	if (pEventMgr->KeyIsDown(K_CTRL))
// 	{
//		mods |= MOD_CTRL;
//	}
//
//	if (pEventMgr->KeyIsDown(K_ALT))
// 	{
//		mods |= MOD_ALT;
//	}
//
//	if (pEventMgr->KeyIsDown(K_RIGHT_SHIFT) ||
//		 pEventMgr->KeyIsDown(K_LEFT_SHIFT))
// 	{
//		mods |= MOD_SHIFT;
//	}
//
//	return mods;

	// OLD KEYBOARD MODS
 	return (uint16) *(pEventMgr->real_ptr_bios_key_status);
}


// Set the keyboard interrupt vector (only useful in DOS)
//
void
ASetKeyboardVector(bool setup)
{
	if (setup)
	{
		if (!pEventMgr->prev_int_09)
		{
			pEventMgr->prev_int_09 = _dos_getvect(0x09);
			_dos_setvect(0x09, EventMgr::KeyInterrupt);
		}
	}
	else
	{
		_dos_setvect(0x09, pEventMgr->prev_int_09);
		pEventMgr->prev_int_09 = NULL;
	}
}

