//
// OS.hpp
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Useful os-dependent declarations
//
//----[]-------------------------------------------------------------


#ifndef	os_hpp
#define	os_hpp


#include "types.hpp"


#define	EXT_MAX		4		// max file extension length (e.g. "ANI" + null)

extern "C" void	OS_ShutDownVideo(int origMode);


// on all platforms
//
extern void	OS_InitMem();

#endif
