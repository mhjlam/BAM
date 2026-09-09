/*
Copyright (C) Magna Carta Software, Inc. 1990-1993. All Rights Reserved.
C COMMUNICATIONS TOOLKIT
BIOSVID.H -- Header for BIOS video driver.
*/

#if !defined(BIOSVID_H_INCLUDED)
    #define BIOSVID_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif


    WORD    EXPORT_ bios_cur_get_col(TERMINAL DLL_FAR_ *t);
    WORD    EXPORT_ bios_cur_get_row(TERMINAL DLL_FAR_ *t);
    short   EXPORT_ bios_cur_get_size(TERMINAL DLL_FAR_ *t);
    void    EXPORT_ bios_cur_off(TERMINAL DLL_FAR_ *t);
    void    EXPORT_ bios_cur_on(TERMINAL DLL_FAR_ *t);
    void    EXPORT_ bios_cur_pos(TERMINAL DLL_FAR_ *t, short col, short row);
    void    EXPORT_ bios_cur_size(TERMINAL DLL_FAR_ *t, WORD size);
    void    EXPORT_ bios_cur_up(TERMINAL DLL_FAR_ *t);
    void    EXPORT_ bios_ebd(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    void    EXPORT_ bios_ebl(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    void    EXPORT_ bios_ech(TERMINAL DLL_FAR_ *t, WORD ch, WORD att, WORD num);
    void    EXPORT_ bios_ed(TERMINAL DLL_FAR_ *t,  WORD ch, WORD att);
    void    EXPORT_ bios_eed(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    void    EXPORT_ bios_eel(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    void    EXPORT_ bios_eil(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    short   EXPORT_ bios_get_crt_cols(TERMINAL DLL_FAR_ *t);
    short   EXPORT_ bios_get_crt_rows(TERMINAL DLL_FAR_ *t);
    short   EXPORT_ bios_getc(TERMINAL DLL_FAR_ *t);
    short   EXPORT_ bios_put(TERMINAL DLL_FAR_ *t, short col, short row, WORD ch, WORD att);
    short   EXPORT_ bios_putc(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    void    EXPORT_ bios_scroll(TERMINAL DLL_FAR_ *t, WORD tlc, WORD tlr, WORD brc, WORD brr, WORD att, short count);
    void    EXPORT_ bios_video(TERMINAL DLL_FAR_ *t);

    #if defined(__cplusplus)
        }
    #endif

#endif
