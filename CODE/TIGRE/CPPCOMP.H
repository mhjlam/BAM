/*
 * CPPCOMP.H
 *
 * Contains:
 *     Compiler and environment detection
 *
 * The Greenleaf Comm++ Library
 *
 * Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
 *
 * MODIFICATIONS
 */

#ifndef _CPPCOMP_DOT_H
#define _CPPCOMP_DOT_H

#if defined( __TURBOC__ ) || defined( __BORLANDC__ )
	/*
	 * This section contains macros for all Borland C++ compilers
	 */
	#if !defined( __BORLANDC__ )
    #define GCPP_BORLAND __TURBOC__
  #else
    #define GCPP_BORLAND __BORLANDC__
  #endif
  #define ANSI_PROTOTYPES
  #define GCPP_UNUSED_PARAMETER( a ) (void) a
  #define CLI() disable()
  #define STI() enable()
	#if defined( GCPP_DOSX386 )
		#define GCPP_X32
		#define GCPP_FAR
		#define GCPP_WIN32
	#elif defined( __DPMI16__ )
    #define GCPP_DPMI16
    #define GCPP_X16
    #define GCPP_FAR _far
  #elif defined( __DPMI32__ )
    #define GCPP_DPMI32
    #define GCPP_X32
    #define GCPP_FAR
    #if defined( __WIN32__ )
      #define GCPP_WIN32
    #endif
	#elif defined( _Windows )
    #define GCPP_WINDOWS
    #if defined( __DLL__ )
      #define GCPP_BUILDING_DLL
    #endif
    #if defined( __WIN32__ ) && !defined( __DPMI32__ )
      #define GCPP_WIN32
      #define GCPP_FAR
    #else
      #define GCPP_FAR _far
    #endif
  #elif defined( __OS2__ )
    #define GCPP_OS2
    #define GCPP_FAR _far16
  #else
    #define GCPP_FAR _far
  #endif

	#if defined( __SMALL__ )
    #define GCPP_SMALL
  #endif
	#if defined( __MEDIUM__ )
    #define GCPP_MEDIUM
  #endif
	#if defined( __COMPACT__ )
    #define GCPP_COMPACT
  #endif
	#if defined( __LARGE__ )
    #define GCPP_LARGE
  #endif
	#if defined( __HUGE__ )
    #define GCPP_HUGE
  #endif

#elif defined( __WATCOMC__ )
	/*
	 * This section contains macros for all Watcom C++ compilers
   */
  #define GCPP_WATCOM __WATCOMC__
  #define ANSI_PROTOTYPES
  #define GCPP_UNUSED_PARAMETER( a ) (void) a
  #define CLI() _disable()
  #define STI() _enable()
  #if defined( __WINDOWS__ ) || defined( _WINDOWS )
    #define GCPP_WINDOWS
  #endif
  #if defined( __NT__ )
    #define GCPP_WIN32
    #if !defined( GCPP_FAR )
      #define GCPP_FAR
    #endif
  #endif
  #if !defined( GCPP_FAR )
    #define GCPP_FAR _far
  #endif
  #if defined( __SW_ZU )
    #define GCPP_BUILDING_DLL
  #endif
  #if defined( __OS2__ )
    #define GCPP_OS2
	#endif
	#if defined( GCPP_DOSX386 )
    #define GCPP_X32
    #undef _M_IX86
	#endif
	#if defined( GCPP_DOS4G )
    #define GCPP_DPMI32
		#define GCPP_X32
  #endif
	#if defined( M_I86SM )
    #define GCPP_SMALL
  #endif
	#if defined( M_I86MM )
    #define GCPP_MEDIUM
  #endif
	#if defined( M_I86CM )
    #define GCPP_COMPACT
  #endif
	#if defined( M_I86LM )
    #define GCPP_LARGE
  #endif

#elif defined( __SC__ )
	/*
	 * This section contains macros for all Symantec C++ compilers
   */
  #define GCPP_SYMANTEC __SC__
  #define ANSI_PROTOTYPES
  #define GCPP_UNUSED_PARAMETER( a ) (void) a
  #define GCPP_FAR _far
  #define CLI() int_off()
  #define STI() int_on()
	#if defined( GCPP_DOSX386 )
		#define GCPP_X32
	#endif
	#if defined( _WINDOWS )
    #define GCPP_WINDOWS
  #endif
  #if defined( __NT__ )
    #define GCPP_WIN32
  #endif
	#if defined( M_I86SM )
    #define GCPP_SMALL
  #endif
	#if defined( __MEDIUM__ )
    #define GCPP_MEDIUM
  #endif
	#if defined( __COMPACT__ )
    #define GCPP_COMPACT
  #endif
	#if defined( __LARGE__ )
    #define GCPP_LARGE
  #endif

#elif defined( M_I86 ) && defined( MSDOS )
	/*
	 * This section contains macros for Microsoft DOS/Win16 C++ compilers
   */
  #define GCPP_MICROSOFT _MSC_VER
  #define ANSI_PROTOTYPES
  #define GCPP_FAR _far
  #define GCPP_UNUSED_PARAMETER( a )   (a = a)
  #define CLI() _disable()
  #define STI() _enable()
  #if defined( _WINDOWS )
    #define GCPP_WINDOWS
    #if defined( _WINDLL )
      #define GCPP_BUILDING_DLL
    #endif
  #endif
  #if ( _MSC_VER >= 800 )
	#pragma warning( disable : 4705 )
    #pragma warning( disable : 4699 )
  #else
    #if defined( M_I86HM )
      #define GCPP_HUGE_MODEL
    #endif
  #endif
	#if defined( M_I86SM )
    #define GCPP_SMALL
  #endif
	#if defined( M_I86MM )
    #define GCPP_MEDIUM
  #endif
	#if defined( M_I86CM )
    #define GCPP_COMPACT
  #endif
	#if defined( M_I86LM )
    #define GCPP_LARGE
  #endif
	#if defined( M_I86HM )
    #define GCPP_HUGE
  #endif

#elif defined( _M_IX86 ) || defined( _68K_ )
	/*
	 * This section contains macros for Microsoft Win32 C++ compilers
   */
  #define GCPP_MICROSOFT _MSC_VER
  #define ANSI_PROTOTYPES
  #define GCPP_FAR
  #define GCPP_UNUSED_PARAMETER( a )   (a = a)
  #define CLI() _disable()
  #define STI() _enable()
  #pragma warning( disable : 4699 )
  #if defined( GCPP_DOSX386 )
    #define GCPP_X32
  #endif
  #if defined( _WINDOWS )
    #define GCPP_WINDOWS
  #endif
  #if defined( _WIN32 )
    #define GCPP_WIN32
    #if !defined( WIN32 )
      #define WIN32
    #endif
  #endif
  #if defined( _WINDLL )
    #define GCPP_BUILDING_DLL
  #endif

#elif defined(__IBMC__) || defined(__IBMCPP__)
	/*
	 * This section contains macros for IBM OS/2 C++ compilers
   */
  #define GCPP_IBM
  #define ANSI_PROTOTYPES
  #define GCPP_FAR
  #define GCPP_UNUSED_PARAMETER( a )   (a = a)
  #define GCPP_OS2

#elif
	#error This is an unknown compiler!
#endif

#if defined( GCPP_BUILDING_DLL ) && ( defined( GCPP_WINDOWS ) || defined( GCPP_WIN32 ) )

  #if defined( GCPP_WIN32 )
    #if defined( GCPP_MICROSOFT )
      #define PROTOS              __declspec(dllexport)
      #define GCPP_TYPE           __declspec(dllexport)
      #define GCPP_CLASS_TYPE     __declspec(dllexport)
      #define GCPP_PROTO
      #define GCPP_DLL_FAR
      #define GCPP_FUN            __stdcall
      #define GCPP_CFUN           _cdecl
    #else
      #define PROTOS              far pascal _export
      #define GCPP_TYPE
      #if defined( GCPP_WATCOM ) && ( GCPP_WATCOM < 1050 )
        #define GCPP_CLASS_TYPE
      #else
        #define GCPP_CLASS_TYPE   _export
      #endif
      #define GCPP_PROTO          _export
      #define GCPP_DLL_FAR
      #define GCPP_FUN            _export pascal
      #define GCPP_CFUN           _export cdecl
    #endif
  #else
    #define PROTOS
    #define GCPP_TYPE
		#if defined( GCPP_WATCOM )
			#define GCPP_CLASS_TYPE
		#else
      #define GCPP_CLASS_TYPE     _export
		#endif
    #define GCPP_PROTO            _far
    #define GCPP_DLL_FAR          _far
    #define GCPP_FUN              _export _far pascal
    #define GCPP_CFUN             _export _far cdecl
  #endif
#elif defined( GCPP_USING_DLL ) && ( defined( GCPP_WINDOWS ) || defined( GCPP_WIN32 ) )
  #if defined( GCPP_WIN32 ) && defined( GCPP_BORLAND )
    #define PROTOS                GCPP_FAR pascal
    #define GCPP_TYPE
    #define GCPP_CLASS_TYPE       _import
    #define GCPP_PROTO            _import
    #define GCPP_DLL_FAR
    #define GCPP_FUN              _import pascal
    #define GCPP_CFUN             _import cdecl
  #elif defined( GCPP_WIN32 ) && defined( GCPP_SYMANTEC )
    #define PROTOS                GCPP_FAR pascal
    #define GCPP_TYPE
    #define GCPP_CLASS_TYPE       _export
    #define GCPP_PROTO            _export
    #define GCPP_DLL_FAR
    #define GCPP_FUN              _export pascal
    #define GCPP_CFUN             _export cdecl
  #elif defined( GCPP_WIN32 ) && defined( GCPP_MICROSOFT )
    #define PROTOS                __declspec(dllimport)
    #define GCPP_TYPE             __declspec(dllimport)
    #define GCPP_CLASS_TYPE       __declspec(dllimport)
    #define GCPP_PROTO
    #define GCPP_DLL_FAR
    #define GCPP_FUN              __stdcall
    #define GCPP_CFUN             _cdecl
  #elif defined( GCPP_WIN32 ) && defined( GCPP_WATCOM )
    #define PROTOS                GCPP_FAR _pascal
    #define GCPP_TYPE
    #define GCPP_CLASS_TYPE
    #define GCPP_PROTO
    #define GCPP_DLL_FAR
    #define GCPP_FUN              pascal
    #define GCPP_CFUN             cdecl
  #elif defined( GCPP_WATCOM )
    #define PROTOS                GCPP_FAR _pascal
    #define GCPP_TYPE
    #define GCPP_CLASS_TYPE
    #define GCPP_PROTO
    #define GCPP_DLL_FAR
    #define GCPP_FUN              _far _pascal
    #define GCPP_CFUN             _far cdecl
  #else
    #define PROTOS                GCPP_FAR pascal
    #define GCPP_TYPE
    #define GCPP_PROTO            _far
    #define GCPP_DLL_FAR          _far
    #ifdef GCPP_BORLAND
      #if( GCPP_BORLAND >= 0x450 )
        #define GCPP_CLASS_TYPE   _import
      #else
        #define GCPP_CLASS_TYPE   _huge
      #endif
    #else
      #define GCPP_CLASS_TYPE     _export
    #endif
    #define GCPP_FUN              _export _far _pascal
    #define GCPP_CFUN             _export _far cdecl
  #endif
#else
  #define PROTOS
  #define GCPP_TYPE
  #define GCPP_DLL_FAR
  #define GCPP_PROTO
  #define GCPP_CLASS_TYPE
  #if defined( GCPP_IBM )
    #define GCPP_CFUN             _Optlink
  #elif defined( GCPP_WATCOM )
    #define GCPP_CFUN             cdecl
  #else
    #define GCPP_CFUN                                                                                       _cdecl
  #endif
  #if defined( GCPP_MICROSOFT )
    #if defined( GCPP_WIN32 )
        #define GCPP_FUN          __stdcall
    #elif defined( GCPP_WINDOWS )
      #define GCPP_FUN            _far pascal
    #else
      #define GCPP_FUN            _fastcall
    #endif
  #else
    #define GCPP_FUN
  #endif
#endif


#ifdef GCPP_WINDOWS
	#define GCPP_SPRINTF										wsprintf
	#define GCPP_VSPRINTF 									wvsprintf
	#define GCPP_STRING 										char GCPP_FAR *
	#define GCPP_USTRING										unsigned char GCPP_FAR *
  #if defined( GCPP_BUILDING_DLL ) || defined( GCPP_USING_DLL )
    #define GCPP_STATIC
  #else
		#define GCPP_STATIC 									static
  #endif
#else
	#define GCPP_SPRINTF										sprintf
	#define GCPP_VSPRINTF 									vsprintf
	#define GCPP_STRING 										char *
	#define GCPP_USTRING										unsigned char *
	#define GCPP_STATIC 										static
#endif

#endif
