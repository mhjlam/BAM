/*
Copyright (C) Magna Carta Software, Inc. 1990-1993.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
ANSI.H -- ANSI X3.64 terminal emulation header.
*/


#if !defined(ANSI_H_INCLUDED)
    #define ANSI_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if !defined(COMM_H_INCLUDED)
        #include <comm.h>
    #endif

    /*
    ANSI COMMANDS (refer to X3.64-1979 to see how these manifest constants
    are defined and the origin of the mnemonics)
    */
    /* CONTROL SEQUENCES */
    #define ANSI_ICH            0X40    /* insert character */
    #define ANSI_CUU            0X41    /* cursor up */
    #define ANSI_CUD            0X42    /* cursor down */
    #define ANSI_CUF            0X43    /* cursor forward */
    #define ANSI_CUB            0X44    /* cursor back */
    #define ANSI_CNL            0X45    /* cursor next line */
    #define ANSI_CPL            0X46    /* cursor preceding line */
    #define ANSI_CHA            0X47    /* cursor horizontal absolute */
    #define ANSI_CUP            0X48    /* cursor position */
    #define ANSI_CHT            0X49    /* cursor horizontal tabulation */
    #define ANSI_ED             0X4A    /* erase in display */
    #define ANSI_EL             0X4B    /* erase in line */
    #define ANSI_IL             0X4C    /* insert line */
    #define ANSI_DL             0X4D    /* delete line */

    #define ANSI_DCH            0X50    /* del character */
    #define ANSI_CPR            0X52    /* cursor position report */
    #define ANSI_ECH            0X58    /* erase character */

    #define ANSI_DA             0X63    /* device attributes */
    #define ANSI_HVP            0X66    /* cursor horizontal and vertical pos. */
    #define ANSI_TBC            0X67    /* tabulation clear */
    #define ANSI_SM             0X68    /* set mode */
    #define ANSI_MC             0X69    /* media copy */
    #define ANSI_RM             0X6C    /* reset mode */
    #define ANSI_SGR            0X6D    /* set graphics rendition */
    #define ANSI_DSR            0X6E    /* device status report (DSR already assigned) */


    /* INDEPENDENT CONTROL FUNCTIONS BY MNEMONIC */
    #define ANSI_IND            0X84    /* index */
    #define ANSI_NEL            0X85    /* next line */
    #define ANSI_SSA            0X86    /* start of selected area */
    #define ANSI_ESA            0X87    /* end of selected area */
    #define ANSI_HTS            0X88    /* horizontal tabulation set */
    #define ANSI_HTJ            0X89    /* horizontal tabulation with just. */
    #define ANSI_VTS            0X8A    /* vertical tabulation set */
    #define ANSI_PLD            0X8B    /* partial line down */
    #define ANSI_RI             0X8C    /* reverse index */

    /* SET/RESET MODE OPTIONS */
    #define ANSI_IRM            0X04    /* insert/replace mode */
    #define ANSI_LNM            20      /* line feed/new line mode */

    #define ANSI_RIS            0XA0    /* reset to initial state */


    /* PRIVATE SEQUENCES -- IBM ANSI.SYS */
    #define IBMANSI_SC          0X73    /* 's' save cursor position */
    #define IBMANSI_RC          0X75    /* 'u' restore cursor position */

    void    EXPORT_ ibmansi_init(hTERMINAL t);

    #if defined(__cplusplus)
        }
    #endif
#endif
