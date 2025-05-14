//
// DPMI.CPP
//
// October 12, 1994
// WATCOM: October 14, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// DPMI Class
//
//----[]-------------------------------------------------------------


#include <dos.h>
#include <i86.h>

#include "dpmi.hpp"


DPMI::DPMI ()
{
}

DPMI::~DPMI ()
{
}


void
DPMI::Interrupt (REGS* r, SREGS* s)
{
  SREGS blank;

	if (!s)
	{
		memset (&blank, 0, sizeof (blank));
		s = &blank;
	}
	int386x (DPMI_INT, r, r, s);
}


void far*
DPMI::AllocateDosMemoryBlock (size_t size, uint16* seg)
{
  REGS r;

	r.x.eax = 0x100;
	r.x.ebx = size / 16 + (size & 0xf ? 1 : 0);
	Interrupt (&r);
	if ((r.x.cflag & CARRY_FLAG) == 0)	// Carry clear on success
	{
		*seg = r.w.ax;
		return (MK_FP (r.w.dx, 0));
	}
	return NULL;
}


void
DPMI::FreeDosMemoryBlock (uint16 selector)
{
  REGS r;

	r.x.eax = 0x101;
	r.x.edx = selector;
	Interrupt (&r);
}

void
DPMI::FreeDosMemoryBlock (void far* p)
{
	FreeDosMemoryBlock (FP_SEG (p));
}


bool
DPMI::RealModeInterrupt (uint iNum, RealModeCall *pRMC)
{
	REGS 	r;
	SREGS s;
	segread (&s);
	r.x.eax = 0x300;
	r.x.ebx = iNum;
	r.x.ecx = 0;
	r.x.edi = (uint) pRMC;
	Interrupt (&r, &s);
	if ((r.x.cflag & CARRY_FLAG) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// Create a protected mode pointer to real memory,
// given an encoded integer in the format PARAGRAPHS:OFFSET.
// Example:  0xA0000000 would be returned as 0xA0000
//
void*
DPMI::RealToProtected (uint realPtr)
{
	uint	para, off;

	para = (realPtr & 0xFFFF0000) >> 16;
	off = realPtr & 0x0000FFFF;
	return (void*) (para * 16 + off);
}


// Create a protected mode pointer to real memory, given 
// a segment and offset.
//
void*
DPMI::RealToProtected (uint realSeg, uint realOffset)
{
	return (void*) ((realSeg << 16) + realOffset);
}

