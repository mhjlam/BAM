/*
Copyright (C) Magna Carta Software, Inc. 1987-1993.  All Rights Reserved.
Header file for C WINDOWS TOOLKIT & C COMMUNICATIONS TOOLKIT.
PC BIOS Data Area Definitions.
*/


#if !defined(BIOSADDR_H_INCLUDED)
    #define BIOSADDR_H_INCLUDED

    #define BIOS_VIDEO_ADAPTOR      (0X10)
    #define BIOS_KBD_BYTE           (0X17)
    #define BIOS_KBD_BUFFER_HEAD    (0X1A)
    #define BIOS_VIDEO_MODE         (0X49)
    #define BIOS_CRT_COLS           (0X4A)
    #define BIOS_SCREEN_LEN         (0X4C)
    #define BIOS_SCREEN_OFFSET      (0X4E)
    #define BIOS_CURSOR_COL         (0X50)
    #define BIOS_CURSOR_ROW         (0X51)
    #define BIOS_CURSOR_BOT         (0X60)
    #define BIOS_CURSOR_TOP         (0X61)
    #define BIOS_LOW_TIME           (0X6C)
    #define BIOS_HIGH_TIME          (0X6E)
    #define BIOS_CRT_ROWS           (0X84)
    #define BIOS_EGA_CHAR_HEIGHT    (0X85)
    #define BIOS_EGA_INFO_BYTE      (0X87)
    #define BIOS_SAVE_PTR           (0XA8)    /* EGA BIOS ptr. to the table of EGA info. ptrs. */

#endif
