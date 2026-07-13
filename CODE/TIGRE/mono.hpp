//
// MONO.HPP
//
// Simplified for Linux port: the DOS monochrome display buffer has been
// removed.  Out() writes to stderr when g_verbose is true.  All other
// methods are no-ops retained so that call sites compile without changes.
//

#ifndef	mono_hpp
#define	mono_hpp

#include <stdarg.h>

class Mono
{
public:
	Mono() {}
	virtual ~Mono() {}

	void Out(const char* fmt, ...);

	// The following were originally used to manage a DOS monochrome
	// monitor display.  They are no-ops; call sites are left unchanged.
	void            Put(const char*) {}
	virtual void    Clear() {}
	virtual void    Goto(unsigned short, unsigned short) {}
	virtual void    SaveWindow() {}
	virtual void    RestoreWindow() {}
	virtual void    SetWindow(int, int, int, int) {}
	virtual void    SetScrollCount(int) {}
	virtual void    Suspend() {}
	virtual void    Resume() {}
	virtual void    Snapshot() {}
};

extern Mono* pMono;
extern bool  g_verbose;

#endif
