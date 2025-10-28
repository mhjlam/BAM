/*
 * _DEFS386.H       3.00A  Febuary 9, 1995
 *
 * The Greenleaf Comm++ Library
 *
 * Copyright (C) 1985-1995 Greenleaf Software Inc.  All Rights Reserved.
 *
 * NOTES
 *
 *  This header file contains prototypes and miscellaneous definitions
 *  for use with all 32 bit DOS Extenders.  The functions and macros
 *  in this file are intended to provide a transparent and uniform
 *  set of functions that can be used with any 386 DOS Extender.
 *
 * Things we need to define in order to support a 386 DOS Extender.
 * These can be macros, typedefs, functions, or whatever.  It
 * all depends on what DOS Extender and compiler you are using.
 *
 * Functions defined in all caps might be defined as Macros.
 *
 *  Defined by the compiler    Defined by the DOS Extender
 *
 *  GCPP_FARPTR32               _GetDosSelector
 *  GCPP_SEL32                  _AllocateDosMemory
 *  GCPP_OFF32                  _LockMemory
 *  GCPP_POKEC32                _SetVector
 *  GCPP_POKEVID32              _GetRealAddress
 *  GCPP_PEEKC32                _GetOldProtHandler
 *  GCPP_FARPTR16               _GetOldRealHandler
 *  GCPP_SEL16                  _SetHandlers
 *  GCPP_OFF16                  _LockRegion
 *  _GetCs()
 *  _GetDs()
 *  _FarMemSet()
 *  _CopyFromFarMem()
 *  _CopyToFarMem()
 *
 *  Always defined: GCPP_X32
 *
 * CONTENTS
 *
 * MODIFICATIONS
 *
 *  Febuary 9, 1995   3.00A :  Initial release.
 */

#if !defined( _DEFS386_H )
#define _DEFS386_H

#include "cppcomp.h"
#if !defined( GCPP_OS2 )
#include <dos.h>
#endif

#if defined( GCPP_X32 )


/*
 * Fortunately, the definitions for real pointers are the same for
 * every compiler.
 */

/*
 * Under 32 bit DOS extenders, we will have to manipulate 16:16
 * real pointers from time to time.  This structure definition and
 * associated macros make that a little easier to do.
 */

#pragma pack( 1 )
typedef struct _tag_realptr {
	unsigned short int offset;
	unsigned short int segment;
} GCPP_FARPTR16;
#pragma pack()

#define GCPP_SEG16( x ) (x).segment
#define GCPP_OFF16( x ) (x).offset

/*
 * Watcom definitions
 */
#if defined( GCPP_WATCOM )

#include <i86.h>

/*
 * Watcom is the one compiler we use right now that actually supports
 * far pointers.  This means that we can use efficient code in some
 * places by simply dereferencing a far pointer instead of performing
 * a far copy.  However, I still create a structure so that I have
 * easy access to the selector and offset parts of the pointer.
 */
#pragma pack( 1 )
typedef union {
	char _far * p;
	struct _tag_farptr {
		unsigned int offset;
		unsigned short selector;
	} s;
} GCPP_FARPTR32;
#pragma pack()

#define GCPP_SEL32( x ) (x).s.selector
#define GCPP_OFF32( x ) (x).s.offset
#define GCPP_POKEC32( x, c ) (*((x).p) = (c))
#define GCPP_POKEVID32( x, c, att ) (*(short int _far *) ((x).p)) = (short)( (c) + ( (att) << 8 ) )
#define GCPP_PEEKC32( c, x ) (c = *((x).p))

#endif

/*
 * Symantec definitions
 */
#if defined( GCPP_SYMANTEC )

/*
 * Symantec actually supports far pointers.  This means that we can use
 * efficient code in some places by simply dereferencing a far pointer
 * instead of performing a far copy.  However, I still create a structure
 * so that I have easy access to the selector and offset parts of the
 *  pointer.
 */

#pragma pack( 1 )
typedef union {
	char _far * p;
	struct _tag_farptr {
		unsigned int offset;
		unsigned short selector;
	} s;
} GCPP_FARPTR32;
#pragma pack()

#define GCPP_SEL32( x ) (x).s.selector
#define GCPP_OFF32( x ) (x).s.offset
#define GCPP_POKEC32( x, c ) (*((x).p) = (c))
#define GCPP_POKEVID32( x, c, att ) *(short int _far *) ((x).p) = ( (c) + ( (att) << 8 ) )
#define GCPP_PEEKC32( c, x ) (c = *((x).p))

#endif

/*
 * Visual C++ definitions
 */

#if defined( GCPP_MICROSOFT )

#pragma pack( 1 )
typedef struct _tag_farptr {
    unsigned int offset;
    unsigned short selector;
} GCPP_FARPTR32;
#pragma pack()

#define GCPP_SEL32( x ) (x).selector
#define GCPP_OFF32( x ) (x).offset
#define GCPP_POKEC32( x, c ) _gf_pokec32( (x), (c) )
#define GCPP_POKEVID32( x, c, att ) _gf_pokevid32( (x), (c), (att) )
#define GCPP_PEEKC32( c, x ) c = _gf_peekc32( (x) )

void GCPP_FUN _gf_pokec32( GCPP_FARPTR32 x, unsigned char c );
void GCPP_FUN _gf_pokevid32( GCPP_FARPTR32 x, unsigned char c, unsigned char att );
unsigned char GCPP_FUN _gf_peekc32( GCPP_FARPTR32 x );

#endif /* #if defined( GCPP_MICROSOFT ) */

/*
 * Borland definitions.
 */

#if defined( GCPP_BORLAND )

#pragma pack( 1 )
typedef struct _tag_farptr {
    unsigned int offset;
    unsigned short selector;
} GCPP_FARPTR32;
#pragma pack()

#define GCPP_SEL32( x ) (x).selector
#define GCPP_OFF32( x ) (x).offset
#define GCPP_POKEC32( x, c ) _gf_pokec32( (x), (c) )
#define GCPP_POKEVID32( x, c, att ) _gf_pokevid32( (x), (c), (att) )
#define GCPP_PEEKC32( c, x ) c = _gf_peekc32( (x) )

void GCPP_FUN _gf_pokec32( GCPP_FARPTR32 x, unsigned char c );
void GCPP_FUN _gf_pokevid32( GCPP_FARPTR32 x, unsigned char c, unsigned char att );
unsigned char GCPP_FUN _gf_peekc32( GCPP_FARPTR32 x );

#endif

/*
 * The following functions are prototyped for every DOS Extender and
 * compiler.  The prototypes are identical regardeless of the tool,
 * so you won't see any ifdefs here.
 */

#ifdef __cplusplus
extern "C" {
#endif

unsigned short int GCPP_FUN _GetDosSelector( void );
GCPP_FARPTR32 GCPP_FUN _AllocateDosMemory( int size );
int GCPP_FUN _FreeDosMemory( GCPP_FARPTR32 p );
void GCPP_FUN _FarMemSet( GCPP_FARPTR32 add, unsigned char val, int length );
void GCPP_FUN _CopyToFarMem( GCPP_FARPTR32 destination, void *source, int length );
void GCPP_FUN _CopyFromFarMem( void *destination, GCPP_FARPTR32 source, int length );
GCPP_FARPTR16 GCPP_FUN _GetRealAddress( GCPP_FARPTR32 protected_address );
GCPP_FARPTR32 GCPP_FUN _GetOldProtHandler( int interrupt_number );
GCPP_FARPTR16 GCPP_FUN _GetOldRealHandler( int interrupt_number );
int GCPP_FUN _SetHandlers( int interrupt_number, GCPP_FARPTR32 port_handler, GCPP_FARPTR16 real_handler );
int GCPP_FUN _LockRegion( GCPP_FARPTR32 base, int size );
unsigned short int GCPP_FUN _GetCs( void );
unsigned short int GCPP_FUN _GetDs( void );

#ifdef __cplusplus
}
#endif

/*
 * If you don't have REGS or int386 defined, we can make do with the
 * Phar Lap TNT versions.  However, there are some naming problems,
 * so we have to create a new structure.  This means that this code
 * can be broken by a new release of TNT.
 */
#if defined( GCPP_DOSX386 )


#if defined( GCPP_BORLAND ) || defined( GCPP_MICROSOFT ) || defined( GCPP_SYMANTEC )

#include <pldos32.h>

#pragma pack( 1 )
#undef REGS
union REGS {
	struct _DWORDREGS x;
	struct _WORDREGS w;
	struct _BYTEREGS h;
};
#pragma pack()

#define SREGS _SREGS
#define int386( i, r1, r2 ) 	 _int86( (i), 							 \
                               (union _REGS *)( r1 ),      \
                               (union _REGS *)( r2 ) )

#define int386x( i, r1, r2, s ) 	_int86x( (i), 							\
                                  (union _REGS *)( r1 ),      \
                                  (union _REGS *)( r2 ),      \
                                         (s) )
#endif /* #if defined( GCPP_BORLAND ) || defined( GCPP_MICROSOFT ) */
#endif /* #if defined( GCPP_DOSX386 ) */

#endif /* #if defined( GCPP_X32 )       */

#endif /* #if !defined( _DEFS386_H ) */
