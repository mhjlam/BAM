/*
Copyright (C) Magna Carta Software, Inc. 1990-1993.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
KERMIT.H -- C Communications Toolkit header file for Kermit file transfer.
*/

#if !defined(KERMIT_H_INCLUDED)
    #define KERMIT_H_INCLUDED
    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if !defined(COMM_H_INCLUDED)
        #include <comm.h>
    #endif

    #if !defined(XFER_H_INCLUDED)
        #include <xfer.h>
    #endif


    /* PACKET FIELDS */
    #define K_MARK      '\x01'
    #define K_MAXL      94
    #define K_TIMEOUT   3
    #define K_NPAD      0
    #define K_PADC      '\x00'
    #define K_EOL       CR
    #define K_QCTL      '#'
    #define K_QBIN      '&'
    #define K_CHKT      '1'
    #define K_REPT      '~'
    #define K_CAPAS     11

    #define KMARK      500
    #define KMAXL      501
    #define KTIMEOUT   502
    #define KNPAD      503
    #define KPADC      504
    #define KEOL       505
    #define KQCTL      506
    #define KQBIN      507
    #define KCHKT      508
    #define KREPT      509
    #define KCAPAS     510

    #define KMMARK     511
    #define KMMAXL     512
    #define KMTIMEOUT  513
    #define KMNPAD     514
    #define KMPADC     515
    #define KMEOL      516
    #define KMQCTL     517
    #define KMQBIN     518
    #define KMCHKT     519
    #define KMREPT     520
    #define KMCAPAS    521


    /* CONSTANTS DENOTING KERMIT PACKET TYPES */
    #define K_ABORT     (0 | 0X1000)
    #define K_BREAK     (1 | 0X1000)
    #define K_DATA      (2 | 0X1000)
    #define K_ERROR     (3 | 0X1000)
    #define K_FHEAD     (4 | 0X1000)
    #define K_SINIT     (5 | 0X1000)
    #define K_EOF       (6 | 0X1000)

    /* FUNCTION PROTOTYPES */
	short   EXPORT_ kermit_deinit(hPORT p);
	long    EXPORT_ kermit_get(hPORT p, short parm);
	short   EXPORT_ kermit_set(hPORT p, short parm, long val);


    #if defined(__cplusplus)
        }
    #endif

#endif
