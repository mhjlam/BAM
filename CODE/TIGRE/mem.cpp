//
// MEM.CPP
//
// May 26, 1989
// WATCOM: September 23, 1994  BKH
// (C) 1986-1991 by Walter Bright  All Rights Reserved.
// Modified December 7, 1993 by Tachyon, Inc.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apimem.hpp"
#include "mem.hpp"
#include "memmgr.hpp"

// set to false if crashing
bool fPrintUnfreedPtrs = true;


// call this new for classes that need to be in the systems grip area
void * newSys(size_t size)
{
	grip g = pMemMgr->SysMalloc(size);
	return pMemMgr->Deref(g);
}


// Global delete overrides: route grip-tracked pointers through the grip system,
// fall back to free() for non-grip pointers.
// ::operator new is intentionally NOT overridden here — TMovable::operator new
// handles game objects, and other allocations go directly to system malloc.
// Class-level TMovable::operator delete takes precedence over these globals for
// TMovable subclasses, so there is no double-free.

void operator delete(void* p) noexcept
{
	if (!p) return;
	grip g = pMemMgr->GetGrip(p);
	if (g != INVALID_GRIP)
		pMemMgr->Free(g);
	else
		free(p);
}

void operator delete(void* p, size_t) noexcept
{
	::operator delete(p);
}

void operator delete[](void* p) noexcept
{
	if (!p) return;
	grip g = pMemMgr->GetGrip(p);
	if (g != INVALID_GRIP)
		pMemMgr->Free(g);
	else
		free(p);
}

void operator delete[](void* p, size_t) noexcept
{
	::operator delete[](p);
}
