/*
 * COMPAT20.H
 *
 * Contains:
 *     Macro compatibility substitutions
 *
 * The Greenleaf Comm++ Library
 *
 * Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
 *
 * MODIFICATIONS
 */
#ifndef COMPAT20_DOT_H
#define COMPAT20_DOT_H

#define GF_PULSE                      GCPP_PULSE
#define GF_TONE                       GCPP_TONE
#define GF_EVENTMASK                  GCPP_EVENTMASK
#define GF_BYTE                       GCPP_BYTE
#define GF_MKFP                       GCPP_MKFP
#define GF_NUMPORTS                   GCPP_NUMPORTS
#define GF_NUMIRQS                    GCPP_NUMIRQS
#define GF_BUFTYPE_FIXED              GCPP_BUFTYPE_FIXED
#define GF_BUFTYPE_CHANGE             GCPP_BUFTYPE_CHANGE
#define GF_BUFTYPE_INSTALL            GCPP_BUFTYPE_INSTALL
#define GF_RX                         GCPP_RX
#define GF_TX                         GCPP_TX
#define GF_RXPAUSED                   GCPP_RXPAUSED
#define GF_TXPAUSED                   GCPP_TXPAUSED
#define GF_RXSIZE                     GCPP_RXSIZE
#define GF_TXSIZE                     GCPP_TXSIZE
#define GF_RXROOM                     GCPP_RXROOM
#define GF_NONE                       GCPP_NONE
#define GF_BREAK_DETECTED             GCPP_BREAK_DETECTED
#define GF_OVERFLOW                   GCPP_OVERFLOW
#define GF_UNKNOWN                    GCPP_UNKNOWN
#define GF_OFF                        GCPP_OFF
#define GF_ON                         GCPP_ON
#define GF_OUT                        GCPP_OUT
#define GF_IN                         GCPP_IN
#define GF_OK                         GCPP_OK
#define GF_ERROR                      GCPP_ERROR
#define GF_TIMEOUT                    GCPP_TIMEOUT
#define GF_NOT_SUPP                   GCPP_NOT_SUPP
#define GF_EOF                        GCPP_EOF
#define GF_BUFFER_FULL                GCPP_BUFFER_FULL
#define GF_PARITY_ERROR               GCPP_PARITY_ERROR
#define GF_FRAMING_ERROR              GCPP_FRAMING_ERROR
#define GF_RX_OVERFLOW_ERROR          GCPP_RX_OVERFLOW_ERROR
#define GF_OVERRUN_ERROR              GCPP_OVERRUN_ERROR
#define GF_BREAK_RECIEVED             GCPP_BREAK_RECIEVED
#define GF_BREAK_ERROR                GCPP_BREAK_ERROR
#define GF_LINK_ERROR                 GCPP_LINK_ERROR
#define GF_CLOSE_ERROR                GCPP_CLOSE_ERROR
#define GF_BADMODE_ERROR              GCPP_BADMODE_ERROR
#define GF_BADLINE_ERROR              GCPP_BADLINE_ERROR
#define GF_BADARGS_ERROR              GCPP_BADARGS_ERROR
#define GF_SYSFAILED_ERROR            GCPP_SYSFAILED_ERROR
#define GF_WRITE_ERROR                GCPP_WRITE_ERROR
#define GF_READ_ERROR                 GCPP_READ_ERROR
#define GF_ENOMEM_ERROR               GCPP_ENOMEM_ERROR
#define GF_ISOPEN_ERROR               GCPP_ISOPEN_ERROR
#define GF_ILLEGAL_BAUDRATE           GCPP_ILLEGAL_BAUDRATE
#define GF_ILLEGAL_PARITY             GCPP_ILLEGAL_PARITY
#define GF_ILLEGAL_STOPBIT            GCPP_ILLEGAL_STOPBIT
#define GF_ILLEGAL_WORDLENGTH         GCPP_ILLEGAL_WORDLENGTH
#define GF_DRIVER_NOT_INSTALLED       GCPP_DRIVER_NOT_INSTALLED
#define GF_DRIVER_NOT_SUPPORTED       GCPP_DRIVER_NOT_SUPPORTED
#define GF_GENERAL_DRIVER_ERROR       GCPP_GENERAL_DRIVER_ERROR
#define GF_ILLEGAL_BOARD_NUMBER       GCPP_ILLEGAL_BOARD_NUMBER
#define GF_USER_ABORT                 GCPP_USER_ABORT
#define GF_NOHAYESOK                  GCPP_NOHAYESOK
#define GF_BOARD_INUSE                GCPP_BOARD_INUSE
#define GF_NOPORT                     GCPP_NOPORT
#define GF_DOSEXTENDER_ERROR          GCPP_DOSEXTENDER_ERROR
#define GF_IRQINUSE_ERROR             GCPP_IRQINUSE_ERROR
#define GF_TOOMANYHOOKS               GCPP_TOOMANYHOOKS
#define GF_CANTFINDHANDLER            GCPP_CANTFINDHANDLER
#define GF_HANDSHAKE_BLOCK            GCPP_HANDSHAKE_BLOCK
#define GF_GENERAL_ERROR              GCPP_GENERAL_ERROR
#define GF_ERR_U                      GCPP_ERR_U
#define GF_ERR_F                      GCPP_ERR_F
#define GF_ERR_FR                     GCPP_ERR_FR
#define GF_ERR_FW                     GCPP_ERR_FW
#define GF_ERR_FC                     GCPP_ERR_FC
#define GF_ERR_N                      GCPP_ERR_N
#define GF_ERR_R                      GCPP_ERR_R
#define GF_ERR_E                      GCPP_ERR_E
#define GF_ERR_T                      GCPP_ERR_T
#define GF_ERR_EOF                    GCPP_ERR_EOF
#define GF_ERR_X                      GCPP_ERR_X
#define GF_ERR_EOT                    GCPP_ERR_EOT
#define GF_ERR_S                      GCPP_ERR_S
#define GF_ERR_C                      GCPP_ERR_C
#define GF_ERR_IN                     GCPP_ERR_IN
#define GF_ERR_NAK                    GCPP_ERR_NAK
#define GF_ERR_LP                     GCPP_ERR_LP
#define GF_ERR_PI                     GCPP_ERR_PI
#define GF_ERR_FH                     GCPP_ERR_FH
#define GF_ERR_MEM                    GCPP_ERR_MEM
#define GF_ERR_LOGIC                  GCPP_ERR_LOGIC
#define GF_ERR_CSN                    GCPP_ERR_CSN
#define GF_ERR_CSA                    GCPP_ERR_CSA
#define GF_ERR_CPC                    GCPP_ERR_CPC
#define GF_ERR_CPB                    GCPP_ERR_CPB
#define GF_ERR_NOF                    GCPP_ERR_NOF
#define GF_ERR_NFE                    GCPP_ERR_NFE
#define GF_ERR_PRO                    GCPP_ERR_PRO
#define GF_ERR_CGC                    GCPP_ERR_CGC
#define GF_ERR_GEN                    GCPP_ERR_GEN
#define GF_ERR_RNF                    GCPP_ERR_RNF
#define GF_ERR_SZ                     GCPP_ERR_SZ
#define GF_ERR_BPS                    GCPP_ERR_BPS
#define GF_ERR_PA                     GCPP_ERR_PA

#define GF_CDECL                      GCPP_CFUN
#define GF_CONV                       GCPP_FUN
#define GF_UNUSED_PARAMETER           GCPP_UNUSED_PARAMETER
#define GF_FAR                        GCPP_FAR
#define GF_DLL_FAR                    GCPP_DLL_FAR
#define GF_CPP_TYPE                   GCPP_CLASS_TYPE
#define GF_PROTO                      GCPP_PROTO
#define GF_SPRINTF                    GCPP_SPRINTF
#define GF_VSPRINTF                   GCPP_VSPRINTF
#define GF_STRING                     GCPP_STRING
#define GF_USTRING                    GCPP_USTRING
#define GF_STATIC                     GCPP_STATIC

#if defined( GCPP_BORLAND )
  #define GF_TURBO_CPP                GCPP_BORLAND
  #define GF_BORLAND_CPP              GCPP_BORLAND
#endif
#if defined( GCPP_WATCOM )
  #define GF_WATCOM_C                 GCPP_WATCOM
#endif
#if defined( GCPP_SYMANTEC )
  #define GF_SYMANTEC_CPP             GCPP_SYMANTEC
#endif
#if defined( GCPP_MICROSOFT )
  #define GF_MICROSOFT_C              GCPP_MICROSOFT
#endif
#if defined( GCPP_IBM )
  #define GF_IBM_CPP                  GCPP_IBM
#endif
#if defined( GCPP_WINDOWS )
  #define GF_WINDOWS                  GCPP_WINDOWS
  #if defined( GCPP_USING_DLL )
    #define CPP_USING_DLL             GCPP_USING_DLL
  #endif
  #if defined( GCPP_BUILDING_DLL )
    #define CPP_BUILDING_DLL          GCPP_BUILDING_DLL
  #endif
#endif
#if defined( GCPP_OS2 )
  #define GF_OS2                      GCPP_OS2
#endif

#endif
