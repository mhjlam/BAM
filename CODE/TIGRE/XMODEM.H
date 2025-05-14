/*
Copyright (C) Magna Carta Software, Inc. 1990-1994.  All Rights Reserved.
XMODEM.H -- MagnaComm header file FOR XMODEM file transfers.
*/

#if !defined(XMODEM_H_INCLUDED)
    #define XMODEM_H_INCLUDED
    #if defined(__cplusplus)
        extern "C" {
    #endif


    #if !defined(COMM_H_INCLUDED)
        #include <comm.h>
    #endif


    #define XMODEM_RETRIES          10
    #define XMODEM_WAIT_TIME        10000
    #define XMODEM_IBDELAY          10000
    #define XMODEM_CANCOUNT         5     /* number of CANs that constitute an abort */

    short         parse_ymodem_hdr_(char FAR_ *b, WORD len, char DLL_FAR_ *fname, long DLL_FAR_ *fsize, long DLL_FAR_ *fdate);
    long  EXPORT_ xmodem_get(hPORT p, short parm);
    short EXPORT_ xmodem_set(hPORT p, short parm, long value);
    short         ymodem_hdr_build_(char buf[], char *fspec, struct stat *stat);

    #if defined(__cplusplus)
        }
    #endif
#endif
