#ifndef _VIDEO_DOT_H
#define _VIDEO_DOT_H

/*
 * VIDEO.H
 *
 * This is the header file needed to use the video routines supplied
 * with Comm++ 3.00 and up.  These routines are used in demo programs
 * only, and are not a supported part of Comm++.  For the curious, see
 * VIDEO.CPP for more information.
 *
 *  The Greenleaf Comm++ Library
 *
 *  Copyright (C) 1991-95 Greenleaf Software Inc.  All Rights Reserved.
 *
 * MODIFICATIONS
 *
 *  Febuary 8, 1995      3.00 : Released.
 *
 */

#include "cppcomp.h"
#include "_defs386.h"

#define WRAP_ON  1
#define WRAP_OFF 0

typedef enum vidtype {
    VID_UNKNOWN = -1,
    VID_MDA,
    VID_CGA,
    VID_EGA,
    VID_MCGA,
    VID_VGA
} VIDTYPE;

typedef enum vidattribute { VID_NORMAL=0, VID_REVERSE } VIDATTR;

typedef struct {
#if defined( GCPP_X32 )
    GCPP_FARPTR32 address;
    void *restore_buffer;
    unsigned char attribute;
#elif defined( GCPP_WIN32 ) && !defined( GCPP_DOSX386 ) && !defined( GCPP_X32 )
    HANDLE hInput;
    HANDLE hOutput;
    CHAR_INFO *restore_buffer;
    WORD attribute;
#elif defined( GCPP_OS2 )
    #if defined( GCPP_BORLAND )
        char __far16 *address;
    #else
        char *address;
    #endif
    void *restore_buffer;
    unsigned char attribute;
#else
    unsigned int GCPP_FAR *address;
    void *restore_buffer;
    unsigned char attribute;
#endif
    int border;
    unsigned char page;
#if defined( GCPP_OS2 )
    int row;
    int col;
    int  rows;
    int  cols;
    int  first_row;
    int  first_col;
#else
    unsigned char row;
    unsigned char col;
    unsigned char rows;
    unsigned char cols;
    unsigned char first_row;
    unsigned char first_col;
#endif
    int wrap_flag;
} VID_WINDOW;


extern VID_WINDOW * const PhysicalWindow;
extern unsigned char         VidPage;
extern VID_WINDOW *   VidCursorOwner;


GCPP_TYPE VIDTYPE GCPP_FUN VidInitialize( VIDATTR attribute,
                                          int save_screen );
GCPP_TYPE VID_WINDOW * GCPP_FUN VidDefineWindow( unsigned char ul_row,
                                                 unsigned char ul_col,
                                                 unsigned char lr_row,
                                                 unsigned char lr_col,
                                                 int border );
GCPP_TYPE void GCPP_FUN VidTerminate( int restore_screen );
GCPP_TYPE void GCPP_FUN VidGoto( VID_WINDOW *window, int row, int col );
GCPP_TYPE void GCPP_FUN VidPhysicalGoto( unsigned char row,
                                         unsigned char col );
GCPP_TYPE void GCPP_FUN VidReadPhysicalPosition( unsigned char *row,
                                                 unsigned char *col );
GCPP_TYPE void GCPP_CFUN VidPrintf( VID_WINDOW *window, char *fmt, ... );
GCPP_TYPE void GCPP_FUN VidBeep( void );
GCPP_TYPE char * GCPP_FUN VidGets( VID_WINDOW *window, char *buffer, int n );
GCPP_TYPE void GCPP_CFUN VidPokef( VID_WINDOW *window,
                                   int row,
                                   int col,
                                   char *fmt,
                                   ... );
GCPP_TYPE void GCPP_FUN VidSetAttribute( VID_WINDOW *window, int attribute );
GCPP_TYPE int GCPP_FUN VidMenu( unsigned char row,
                                unsigned char col,
                                char *menu[] );
GCPP_TYPE void GCPP_FUN VidDrawBorder( VID_WINDOW *window );
GCPP_TYPE void GCPP_FUN VidClearToEndOfWindow( VID_WINDOW *window );
GCPP_TYPE void GCPP_FUN VidClearToEndOfLine( VID_WINDOW *window );
GCPP_TYPE int GCPP_FUN VidSet40x25( void );
GCPP_TYPE int GCPP_FUN VidSet80x25( void );
GCPP_TYPE int GCPP_FUN VidSet80x28( void );
GCPP_TYPE int GCPP_FUN VidSet80x43( void );
GCPP_TYPE int GCPP_FUN VidSet80x50( void );
GCPP_TYPE int GCPP_FUN VidSet640x350( void );
GCPP_TYPE void GCPP_FUN VidSelectPage( unsigned char c );
GCPP_TYPE void GCPP_FUN VidSelectFastDrivers( void );
GCPP_TYPE void GCPP_FUN VidSelectBIOSDrivers( void );
GCPP_TYPE int GCPP_FUN VidSetWindowPage( VID_WINDOW *window, int page );
GCPP_TYPE void GCPP_FUN OldVidGetLine( char *prompt, char *field, int length );
GCPP_TYPE int GCPP_FUN VidGetLine( char *prompt, char *field, int length );
GCPP_TYPE void GCPP_FUN VidInsertChar( VID_WINDOW *window, int row, int col, int c );
GCPP_TYPE void GCPP_FUN VidDeleteChar( VID_WINDOW *window, int row, int col );
GCPP_TYPE void GCPP_FUN VidFreeVid( VID_WINDOW *window );
GCPP_TYPE void GCPP_FUN VidHideCursor( void );
GCPP_TYPE void GCPP_FUN VidShowCursor( void );
GCPP_TYPE void GCPP_FUN VidMakeAttribute( VID_WINDOW *window,
                                          unsigned char fore,
                                          unsigned char back ); /* NEW function */

#if defined( GCPP_WIN32 ) && !defined( GCPP_DOSX386 ) && !defined( GCPP_X32 )

GCPP_TYPE VID_WINDOW * GCPP_FUN VidGetPhysicalWindow( void );
GCPP_TYPE void GCPP_FUN VidSaveWindow( VID_WINDOW *window );
GCPP_TYPE void GCPP_FUN VidRestoreWindow( VID_WINDOW *window, int delete_flag );
GCPP_TYPE void GCPP_FUN VidPuts( VID_WINDOW *window, char *string );
GCPP_TYPE void GCPP_FUN VidScrollUp( VID_WINDOW *window, int line_count );
GCPP_TYPE void GCPP_FUN VidPokec( VID_WINDOW *window, int row, int col, int c );
GCPP_TYPE void GCPP_FUN VidPokes( VID_WINDOW *window, int row,
                                  int col, char *string );
GCPP_TYPE void GCPP_FUN VidClearWindow( VID_WINDOW *window );
GCPP_TYPE void GCPP_FUN VidPutc( VID_WINDOW *window, int c );
GCPP_TYPE int GCPP_FUN VidPeekWord( VID_WINDOW *window, int row, int col );
GCPP_TYPE void GCPP_FUN VidClearLine( VID_WINDOW *window, int row );
GCPP_TYPE void GCPP_FUN VidScrollDown( VID_WINDOW *window, int line_count );
GCPP_TYPE void GCPP_FUN VidCls( void );

#else

extern void ( GCPP_FUN * VidSaveWindow )( VID_WINDOW *window );
extern void ( GCPP_FUN * VidRestoreWindow )( VID_WINDOW *window,
                                            int delete_flag );
extern void ( GCPP_FUN * VidPuts)( VID_WINDOW *window, char *string );
extern void ( GCPP_FUN * VidScrollUp )( VID_WINDOW *window, int line_count );
extern void ( GCPP_FUN * VidPokec )( VID_WINDOW *window, int row,
                                    int col, int c );
extern void ( GCPP_FUN * VidPokes )( VID_WINDOW *window, int row,
                                    int col, char *string );
extern void ( GCPP_FUN * VidClearWindow)( VID_WINDOW *window );
extern void ( GCPP_FUN * VidPutc)( VID_WINDOW *window, int c );
extern int ( GCPP_FUN * VidPeekWord )( VID_WINDOW *window, int row, int col );
extern void ( GCPP_FUN * VidClearLine)( VID_WINDOW *window, int row );
extern void ( GCPP_FUN * VidScrollDown )( VID_WINDOW *window, int line_count );
extern void ( GCPP_FUN * VidCls)( void );

#endif


#define UL_CORNER       218
#define UR_CORNER       191
#define LL_CORNER       192
#define LR_CORNER       217
#define HORIZONTAL_LINE 196
#define VERTICAL_LINE   179
#define TOP_TEE         194
#define RIGHT_TEE       180
#define BOTTOM_TEE      193
#define LEFT_TEE        195
#define CENTER_TEE      197

#if !defined( GCPP_WIN32 )
#define VID_MKFP( seg, offset ) ( void far *) (((unsigned long) (seg) << 16 ) + (offset) )  /* Tag: Misc private */
#endif

#endif /* _VIDEO_DOT_H */

