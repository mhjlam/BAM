#ifndef _TEXTWIN_DOT_H
#define _TEXTWIN_DOT_H

/*
 * TW.H              3.00A  Febuary 8, 1995
 *
 * The Greenleaf Comm++ Library
 *
 * Copyright (C) 1984-95 Greenleaf Software Inc.  All Rights Reserved.
 *
 * NOTES
 *
 *  This file contains all the function prototypes, structures, and other
 *  definitions needed to use the TW stuff under Windows.
 *
 * MODIFICATIONS
 *  November 5, 1993 - Modified to support the Symantec C++ 6.0 compiler.
 *  June 9, 1994 - Added the OldFont member to the TW struct.  This was done
 *                 to save the font so that we could map it back in when
 *                 closing down.
 */
#include "cppcomp.h"

#if defined( GCPP_WINDOWS )
#ifndef STRICT
 #define STRICT
#endif
#include <windows.h>

#if defined( GCPP_SYMANTEC )
#include <dos.h>
#define _fcalloc farcalloc
#define _ffree farfree
#endif

typedef enum tw_attribute { TW_NORMAL=0, TW_REVERSE } TW_ATTR;

/*
 * VTW stands for Virtual Text Window.  A Text Window is nominally a virtual
 * 132x25 window, with scroll back capability of 75 lines.
 */

#define VTW_MAX_ROWS         25
#define VTW_MAX_COLS         132
#define VTW_BUFFER_ROWS      100
#define VTW_BUFFER_COLS      132
#define VTW_CARET_ROW_OFFSET ( VTW_BUFFER_ROWS - VTW_MAX_ROWS )

#define WRAP_ON  1
#define WRAP_OFF 0

struct TWINSTANCEDATAtag {
    HINSTANCE ThisInstance;
    HINSTANCE WhatInstance;
    int ThisShow;
    HWND ClientHandle;
    int TWAppClosed;
    HMENU TWMainMenu;
    HMENU TWFileMenu;
    HMENU TWWindowMenu;
    HWND TWFrameHandle;
#if defined( GCPP_WINDOWS ) && !defined( GCPP_WIN32 )
    HTASK hTask;
#elif defined( GCPP_WIN32 )
    HANDLE hTask;
#else
    int hTask;
#endif
    int nRefCount;
};

typedef struct {
    HWND hwnd;
    LPSTR Title;
    int VTWRows;                     /* Rows in the Virtual Text Window    */
    int VTWCols;                     /* Cols in the Virtual Text Window    */
    int VTWCaretRow;                 /* Current Caret row in the VTW       */
    int VTWCaretCol;                 /* Current Caret col in the VTW       */
    int CharWidth;
    int CharHeight;
    int VisibleRows;
    int VisibleCols;
    int VisibleXPixels;
    int VisibleYPixels;
    int FirstVisibleCol;
    int FirstVisibleRow;
    int FirstVisibleRowOffset;
    int CaretOn;
    int HasFocus;
    int Scrollable;
    int CurrentAttribute;
    int DefaultAttribute;
    unsigned int KeyboardHead;
    unsigned int KeyboardTail;
    unsigned int KeyboardBuffer[ 128 ];
    char ScreenBuffer[ VTW_BUFFER_ROWS ][ VTW_BUFFER_COLS ];
    char AttributeBuffer[ VTW_BUFFER_ROWS ][ VTW_BUFFER_COLS ];
    HFONT Font;
    HFONT OldFont;
    int LastCommand;
    int wrap_flag;
    struct TWINSTANCEDATAtag far *lpThis;
} far TW_WINDOW;

#if defined( GCPP_WATCOM )
extern int TWAppClosed;
extern HMENU TWMainMenu;
extern HMENU TWFileMenu;
extern HMENU TWWindowMenu;
extern HWND TWFrameHandle;
#else
extern GCPP_TYPE int GCPP_PROTO TWAppClosed;
extern GCPP_TYPE HMENU GCPP_PROTO TWMainMenu;
extern GCPP_TYPE HMENU GCPP_PROTO TWFileMenu;
extern GCPP_TYPE HMENU GCPP_PROTO TWWindowMenu;
extern GCPP_TYPE HWND GCPP_PROTO TWFrameHandle;
#endif


GCPP_TYPE int GCPP_FUN TWInitialize( HINSTANCE hInstance,
                                     HINSTANCE PreviousInstance,
                                     int nCmdShow,
									 char GCPP_DLL_FAR *title );
GCPP_TYPE TW_WINDOW * GCPP_FUN TWDefineWindow( int ul_row,
                                               int ul_col,
                                               int width,
                                               int height,
                                               int scrollable,
											   char GCPP_DLL_FAR *name,
											   char GCPP_DLL_FAR *font,
                                               int font_size );
GCPP_TYPE void GCPP_FUN TWPutc( TW_WINDOW GCPP_DLL_FAR *window, int c );
GCPP_TYPE void GCPP_FUN TWPuts( TW_WINDOW GCPP_DLL_FAR *window,
								char GCPP_DLL_FAR *string );
GCPP_TYPE void GCPP_CFUN TWPrintf( TW_WINDOW GCPP_DLL_FAR *window,
								   char GCPP_DLL_FAR *fmt,
                                   ... );
GCPP_TYPE int GCPP_FUN TWKbhit( TW_WINDOW GCPP_DLL_FAR *vw );
GCPP_TYPE unsigned int GCPP_FUN TWGetkey( TW_WINDOW GCPP_DLL_FAR *vw );
GCPP_TYPE void GCPP_FUN TWPokec( TW_WINDOW GCPP_DLL_FAR *window,
                                 int row,
                                 int col,
                                 int c );
GCPP_TYPE void GCPP_FUN TWScrollUp( TW_WINDOW GCPP_DLL_FAR *window,
                                    int line_count );
GCPP_TYPE void GCPP_FUN TWScrollDown( TW_WINDOW GCPP_DLL_FAR *window,
                                      int line_count );
GCPP_TYPE void GCPP_FUN TWYield( void );
GCPP_TYPE void GCPP_FUN TWSetAttribute( TW_WINDOW GCPP_DLL_FAR *window,
                                        int attribute );
GCPP_TYPE void GCPP_FUN TWSetDefaultAttribute( TW_WINDOW GCPP_DLL_FAR *window,
                                               int attribute );
GCPP_TYPE void GCPP_FUN TWPokes( TW_WINDOW GCPP_DLL_FAR *window,
                                 int row,
                                 int col,
								 char GCPP_DLL_FAR *string );
GCPP_TYPE void GCPP_FUN TWGoto( TW_WINDOW GCPP_DLL_FAR *window,
                                int row,
                                int col );
GCPP_TYPE void GCPP_CFUN TWPokef( TW_WINDOW GCPP_DLL_FAR *window,
                                  int row,
                                  int col,
								  char GCPP_DLL_FAR *fmt,
                                  ... );
GCPP_TYPE void GCPP_FUN TWClearWindow( TW_WINDOW GCPP_DLL_FAR *window );
GCPP_TYPE void GCPP_FUN TWInsertChar( TW_WINDOW GCPP_DLL_FAR *window,
                                      int row,
                                      int col,
                                      int c );
GCPP_TYPE void GCPP_FUN TWDeleteChar( TW_WINDOW GCPP_DLL_FAR *window,
                                      int row,
                                      int col );
GCPP_TYPE void GCPP_FUN TWClearToEndOfLine( TW_WINDOW GCPP_DLL_FAR *window,
                                            int row,
                                            int col );
GCPP_TYPE void GCPP_FUN TWClearToEndOfWindow( TW_WINDOW GCPP_DLL_FAR *window,
                                              int row,
                                              int col );
GCPP_TYPE int GCPP_FUN TWGetLine( char GCPP_DLL_FAR *caption,
								  char GCPP_DLL_FAR *prompt,
								  char GCPP_DLL_FAR *buffer,
                                  int n );
GCPP_TYPE int GCPP_FUN TWMenu( TW_WINDOW GCPP_DLL_FAR *tw,
                               unsigned char row,
                               unsigned char col,
							   char GCPP_DLL_FAR *menu[] );
GCPP_TYPE void GCPP_FUN TWClearVirtualTerminal( TW_WINDOW GCPP_DLL_FAR *window );
GCPP_TYPE void GCPP_FUN TWSetClientScrollBars( int toggle );
GCPP_TYPE void GCPP_FUN TWSetBackGroundBrush( int brush );
GCPP_TYPE char * GCPP_FUN TWGets( TW_WINDOW GCPP_DLL_FAR *window,
								  char GCPP_DLL_FAR *string,
                                  int length );
GCPP_TYPE struct TWINSTANCEDATAtag far * GCPP_FUN GetTWInstanceDataPtr( void );
GCPP_TYPE int GCPP_FUN GetTWAppClosed( void );
GCPP_TYPE HMENU GCPP_FUN GetTWMainMenu( void );
GCPP_TYPE HMENU GCPP_FUN GetTWFileMenu( void );
GCPP_TYPE HMENU GCPP_FUN GetTWWindowMenu( void );
GCPP_TYPE HWND GCPP_FUN GetTWFrameHandle( void );
GCPP_TYPE void GCPP_FUN TWFreeInstanceData( void );
GCPP_TYPE void GCPP_FUN TWFreeWindow( TW_WINDOW *tw );

#if !defined( GCPP_WIN32 )

typedef int (GCPP_FUN *NOTIFY_HANDLER)(TW_WINDOW *tw, HWND hwnd, WPARAM wParam, LPARAM lParam);
void GCPP_FUN SetTWCommNotifyHandler( NOTIFY_HANDLER f );
extern NOTIFY_HANDLER TWCommNotifyHandler;

#endif

#endif /* ifdef GF_WINDOWS */

#endif /* #ifndef _TEXTWIN_DOT_H */

