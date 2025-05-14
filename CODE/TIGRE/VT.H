/*
Copyright (C) Magna Carta Software, Inc. 1990-1993. All Rights Reserved.
C COMMUNICATIONS TOOLKIT
VT.H -- Include file for VTxxx terminal emulation.
*/

#if !defined(VT_H_INCLUDED)
    #define VT_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if !defined(TERMINAL_H_INCLUDED)
        #include <terminal.h>
    #endif

    void    EXPORT_ vt52_init(hTERMINAL t);
    void    EXPORT_ vt100_init(hTERMINAL t);
    void    EXPORT_ vt220_init(hTERMINAL t);
    short   EXPORT_ vt_reset_graphics_chars(hTERMINAL t, WORD charset);
    short   EXPORT_ vt_set_graphics_chars(hTERMINAL t, WORD charset);

    #if defined(__cplusplus)
            }
    #endif
#endif
