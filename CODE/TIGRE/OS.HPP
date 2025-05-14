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


#ifdef OS_DOS
	#ifdef __WATCOMC__
		#define	EXT_MAX		4
	//	#define	PATH_MAX		64	// already defined

		#ifdef	NDEBUG
			#define	DEBUGGER
		#else
			#pragma aux	DEBUGGER	= "int 0x3";
		#endif
	#else
		// Platform specific defines
		#define	EXT_MAX		4
		#define	PATH_MAX		64

		#ifdef	NDEBUG
			#define	DEBUGGER
		#else
			#define	DEBUGGER		asm	int 0x03
		#endif
	#endif	//watcom

	// useful for portability
	#define	DOS_ONLY(x)		x
	#define	FLUSH

	extern "C" void	OS_ShutDownVideo(int origMode); 

	// The conversion macros below convert "logical" horizontal coordinates to
	// "real" horizontal coordinates. Logical coordinates are the coordinates
	// used in the application programming (and most of the system programming).
	// Real coordinates are the on screen coordinates. 
	// In DOS/Tigre, real and logical coords are congruent, therefore these
	// macros do nothing. They only affect macintosh when playing in the 
	// "double-wide" mode

	#define LOGICAL_2_REAL(x)		(x)			// times 2
	#define REAL_2_LOGICAL(x)		(x)			// divide by 2
	#define L2R(x)	LOGICAL_2_REAL(x)
	#define R2L(x)	REAL_2_LOGICAL(x)

#endif	//OS_DOS


#ifdef OS_MAC

	#define	EXT_MAX		6
	#define	PATH_MAX		64	// already defined

	#ifdef	NDEBUG
		#define	DEBUGGER
	#else
		#define	DEBUGGER	 Debugger
	#endif

	// useful for portability
	#define	DOS_ONLY(x)		x
 	#define	FLUSH

	// The conversion macros below convert "logical" horizontal coordinates to
	// "real" horizontal coordinates. Logical coordinates are the coordinates
	// used in the application programming (and most of the system programming).
	// Real coordinates are the on screen coordinates. 
	//
	// They are only effective when playing in the "double-wide" mode.

	#define DOUBLE_PIX 1

	#ifdef DOUBLE_PIX
		#define LOGICAL_2_REAL(x)		(x<<1)			// times 2
		#define REAL_2_LOGICAL(x)		(x>>1)			// divide by 2
	#else
		#define LOGICAL_2_REAL(x)		(x)			// times 2
		#define REAL_2_LOGICAL(x)		(x)			// divide by 2
	#endif
	#define L2R(x)	LOGICAL_2_REAL(x)
	#define R2L(x)	REAL_2_LOGICAL(x)
#endif	//OS_MAC


// on all platforms
//
extern void	OS_InitMem();

#endif

