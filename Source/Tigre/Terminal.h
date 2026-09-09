/*
Copyright (C) Magna Carta Software, Inc. 1990-1994.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
TERMINAL.H -- Common terminal include file.
*/

#if !defined(TERMINAL_H_INCLUDED)
    #define TERMINAL_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if !defined(CCTDEF_H_INCLUDED)
        #include <cctdef.h>                 /* constant definitions */
    #endif
#if 0
    #if !defined(CCTTYPE_H_INCLUDED)
      	#include <ccttype.h>                /* type definitions */
    #endif
#endif

    /* TERMINAL TYPES (SUPPORTED OR IN TEST) */
    #define TTY             0
    #define ALTOSIII        7
    #define ANSI_SYS        1
    #define ANSI_X364       2
    #define DEC_VT52        3
    #define DEC_VT100       4
    #define DEC_VT220       5
    #define DEC_VT320       6
    #define MC_BINARY       11
    #define TV_910          8
    #define TV_912          9
    #define TV_925          10


    /* TERMINAL CONFIGURATION PARAMETERS */
    #define TERM_COLORS         0
    #define TERM_COLUMNS        1
    #define TERM_CURSOR         8
    #define TERM_DEBUG          9
    #define TERM_KEYCLICK       2
    #define TERM_ROWS           3
    #define TERM_MARGIN_BOTTOM  4
    #define TERM_MARGIN_LEFT    5
    #define TERM_MARGIN_RIGHT   6
    #define TERM_MARGIN_TOP     7
    #define TERM_FONT           10


    /* UPDATE FLAGS */
    #define UPDATE_LINE         1
    #define UPDATE_DISPLAY      2

    typedef short (DLL_FAR_ *HTERMCALLFUNC)(hTERMINAL, short);

    /* FUNCTION PROTOTYPES */
    void        EXPORT_ binary_init(hTERMINAL t);
    short       EXPORT_ term_assign_cmd(hTERMINAL t, short cmd, const char DLL_FAR_ *sh2t, short (DLL_FAR_ *func)(hTERMINAL, short), const char DLL_FAR_ *st2h);
    short       EXPORT_ term_assign_key(hTERMINAL t, WORD key, const char DLL_FAR_ *cmd);
    short       EXPORT_ term_cursor_blink(hTERMINAL t);
    short       EXPORT_ term_cursor_write(hTERMINAL t);
    short       EXPORT_ term_deassign_key(hTERMINAL t, WORD key);
    short       EXPORT_ term_deinit(hPORT p);
    long        EXPORT_ term_get(hTERMINAL t, short parm);
    short       EXPORT_ term_hts(hTERMINAL t, WORD col);
    hTERMINAL   EXPORT_ term_init(hPORT p, void (WPASCAL_ DLL_FAR_ *v)(hTERMINAL t), void (WPASCAL_ DLL_FAR_ *video)(TERMINAL DLL_FAR_ *t));
    short       EXPORT_ term_set(hTERMINAL t, short parm, long val);
    short       EXPORT_ term_update_display(hTERMINAL t);
    hTERMINAL   EXPORT_ terminal_init(hPORT p, WORD termtype, DWORD lp);
    void        EXPORT_ tty_init(hTERMINAL t);
    #if defined(__cplusplus)
            }
    #endif

#endif
