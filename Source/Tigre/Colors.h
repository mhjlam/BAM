/* Copyright (C) Magna Carta Software, Inc. 1988-1992.  All Rights Reserved.
COLORS.H -- C Windows Toolkit and C Communications Toolkit header file for
screen attributes.
*/


#if !defined(COLORS_H_INCLUDED)
    #define COLORS_H_INCLUDED

    #define MC_BLACK           0
    #define MC_BLUE            1
    #define MC_GREEN           2
    #define MC_CYAN            3
    #define MC_RED             4
    #define MC_MAGENTA         5
    #define MC_BROWN           6
    #define MC_LIGHTGRAY       7
    #define MC_DARKGRAY        8
    #define MC_LIGHTBLUE       9
    #define MC_LIGHTGREEN      10
    #define MC_LIGHTCYAN       11
    #define MC_LIGHTRED        12
    #define MC_LIGHTMAGENTA    13
    #define MC_YELLOW          14
    #define MC_WHITE           15
    #define MC_TRANSPARENT_    (-2)
    #define MC_LIGHTGREY       7
    #define MC_DARKGREY        8
    #define MC_BRIGHT          8
    #define MC_LIGHT           8
    #define MC_LIGHTWHITE      15
    #define MC_BRIGHTWHITE     15
    #define MC_BLINK           128
    #define MC_UNDERLINE       1   /* MONO ONLY -- USE AS FOREGROUND WITH BLACK BGD. */
    #define MC_TRANSPARENT     (-2)
#if 0
    /* OBSOLESCENT -- AVOID USING THE MANIFEST CONSTANTS BELOW */
    #if !defined(__COLORS)
        #define BLACK           0
        #define BLUE            1
        #define GREEN           2
        #define CYAN            3
        #define RED             4
        #define MAGENTA         5
        #define BROWN           6
        #define LIGHTGRAY       7
        #define DARKGRAY        8
        #define LIGHTBLUE       9
        #define LIGHTGREEN      10
        #define LIGHTCYAN       11
        #define LIGHTRED        12
        #define LIGHTMAGENTA    13
        #define YELLOW          14
        #define WHITE           15
        #define __COLORS
    #endif
    #define TRANSPARENT         (-2)
    #define LIGHTGREY           7
    #define DARKGREY            8
    #define BRIGHT              8
    #define LIGHT               8
    #define LIGHTWHITE          15
    #define BRIGHTWHITE         15
    #define BLINK               128

    #define UNDERLINE   1   /* MONO ONLY -- USE AS FOREGROUND WITH BLACK BGD. */
    #define underline   1   /* MONO ONLY -- USE AS FOREGROUND WITH BLACK BGD. */
#endif
#endif
