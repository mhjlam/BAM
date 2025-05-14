//
// DPMI.HPP
//
// October 12, 1994
// WATCOM: October 14, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// DPMI Class
//
//----[]-------------------------------------------------------------


#ifndef	dpmi_hpp
#define	dpmi_hpp

#include <i86.h>

#include "types.hpp"


struct RealModeCall
{
	uint32 edi;
	uint32 esi;
	uint32 ebp;
	uint32 reserved;
	uint32 ebx;
	uint32 edx;
	uint32 ecx;
	uint32 eax;
	uint16 flags;
	uint16 es;
	uint16 ds;
	uint16 fs;
	uint16 gs;
	uint16 ip;
	uint16 cs;
	uint16 sp;
	uint16 ss;
	RealModeCall ()
	{
		memset (this, 0, sizeof (*this));
	}
};

const uint CARRY_FLAG = 0x1;
const uint DOS_INT	 = 0x21;
class DPMI
{
	enum MISC { DPMI_INT = 0x31 };

	public:
		DPMI ();
		virtual ~DPMI ();
		void 			Interrupt (REGS* r, SREGS* s = NULL);
		void far* 	AllocateDosMemoryBlock (size_t size, uint16* seg);
		void 			FreeDosMemoryBlock (uint16 selector);
		void 			FreeDosMemoryBlock (void far* p);
		bool 			RealModeInterrupt (uint iNum, RealModeCall *pRMC);
		void*			RealToProtected (uint realPtr);
		void*			RealToProtected (uint realSeg, uint realOffset);
};


typedef DPMI* pDPMI;


#endif
