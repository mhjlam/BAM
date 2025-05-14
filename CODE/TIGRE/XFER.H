/*
Copyright (C) Magna Carta Software, Inc. 1988-1995.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
XFER.H -- Data types used in CCT file transfer
*/

#if !defined(XFER_H_INCLUDED)
    #define XFER_H_INCLUDED
    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if defined(__POWERC) || defined(__TURBOC__)
        #include <dir.h>
    #endif

    #if !defined(CCTIO_H_INCLUDED)
        #include <cctio.h>
    #endif



    /* CONSTANTS DENOTING FILE TRANSFER PARAMETERS USED IN xmodem_set(), etc. */
    #define CCT_XFER_CANCOUNT           203     /* no. of CANs that => cancel */
    #define CCT_XFER_ECC                205
    #define CCT_XFER_INTERBYTE_DELAY    201
    #define CCT_XFER_KEEP_ABORTED       204     /* keep received files after abort */
    #define CCT_XFER_MAX_BLOCKSIZE      206
    #define CCT_XFER_QUOTE              207
    #define CCT_XFER_RETRIES            200
    #define CCT_XFER_ATTEMPTS           200
    #define CCT_XFER_RXFILENAME         202     /* change incoming file name */
    #define CCT_XFER_RXWINDOW           208
    #define CCT_XFER_TXWINDOW           209
    #define CCT_XMODEM_PADCHAR          210

    /* FILE TRANSFER MESSAGE TYPES (USED IN TRANSFER PROGRESS FUNCTION) */
    /* INFORMATION MESSAGES (CANNOT DIRECTLY CAUSE AN ABORT) */
    #define CCT_BAD_PACKET_NUMBER       0X4000  /* packet number out of sequence */
    #define CCT_CRC_ERROR               0X4001  /* CRC error detected by receiver */
    #define CCT_DOWN_RESUME_TRY         0X4002
    #define CCT_EOT                     0X4004
    #define CCT_FILE_DATE               0X4005
    #define CCT_FILE_NAME               0X4006
    #define CCT_FILE_PROTECTED          0X4007  /* cannot create/open/read/write file */
    #define CCT_FILE_SIZE               0X4008
    #define CCT_FILE_SOURCE             0X4009
    #define CCT_FILE_XFER_ENDED         0X400A
    #define CCT_JUNK_RECEIVED           0X400B  /* send next file header or EOT */
    #define CCT_MSG                     0X400C
    #define CCT_QUOTE_OVERHEAD          0X402A
    #define CCT_RENAMED_FILE            0X400D
    #define CCT_RX_ACK                  0X400E
    #define CCT_RX_CAN                  0X400F
    #define CCT_RX_ENQ                  0X4010
    #define CCT_RX_NAK                  0X4011
    #define CCT_RX_SEQ                  0X4012
    #define CCT_SKIP_FILE               0X4013  /* send next file header or EOT */
    #define CCT_SOH                     0X4014
    #define CCT_SYNC_END                0X4003  /* ended synchronization with remote */
    #define CCT_SYNC_START              0X4015
    #define CCT_STX                     0X4016
    #define CCT_TIMEOUT                 0X4017  /* receiver timed-out */
    #define CCT_TX_ACK                  0X4018
    #define CCT_TX_BUFFER_NOT_EMPTY     0X4019
    #define CCT_TX_CAN                  0X401A
    #define CCT_TX_ENQ                  0X401B
    #define CCT_TX_NAK                  0X401C
    #define CCT_TX_SEQ                  0X301D
    #define CCT_USING_CHECKSUM          0X401E
    #define CCT_USING_CRC               0X401F
    #define CCT_USING_XMODEM            0X4020
    #define CCT_USING_XMODEM_1K         0X4021
    #define CCT_USING_XMODEM_CRC        0X4022
    #define CCT_USING_YMODEM            0X4023
    #define CCT_USING_YMODEM_G          0X4024
    #define CCT_XFER_POSITION           0X4025
    #define CCT_XFER_RX_POSITION        0X4026
    #define CCT_XFER_TX_POSITION        0X4027
    #define CCT_WAITING_FOR_HOST        0X4028
    #define CCT_WINDOW_SIZE             0X4029


    /* ERROR MESSAGES (CAN CAUSE AN ABORT) */
    #define CCT_DISK_ERROR              0X4040  /* error in disk I/O */
    #define CCT_DISK_FULL               0X4041  /* insufficient disk space on RX disk */
    #define CCT_MAX_ERRORS_REACHED      0X4042  /* max. error count reached */
    #define CCT_NO_REMOTE_START         0X4043  /* Remote did not start */
    #define CCT_REMOTE_CANCELLED        0X4044  /* transmitter acknowledgement */
    #define CCT_UNSPECIFIED             0X4045  /* unspecified reason for abort */

    /* FILE TRANSFER PARAMETERS FOR x->f_xfer */
    #define CCT_XFER_IN_PROGRESS        1       /* return code when TX intr. in use */
    /* BIT MAP MASKS FOR x->f_xfer  */
    #define CCT_SENDING_FROM_RAM        1


    long  EXPORT_ ascii_get(hPORT p, short parm);
    short EXPORT_ ascii_set(hPORT p, short parm, long value);
    short EXPORT_ cct_xfer_cancel(hPORT p);
    short EXPORT_ fmodname(char DLL_FAR_ *fname);
    short EXPORT_ fqueue(WORD qnum, char DLL_FAR_ *fname);
    short EXPORT_ freceive(hPORT p, short protocol, unsigned bufsize, HPROGRESS progress, short parm);
    short EXPORT_ fsend(hPORT p, WORD qnum, short protocol, unsigned len, HPROGRESS progress, short parm);
    short EXPORT_ funload(WORD qnum, const char DLL_FAR_ *fname);
    short EXPORT_ funqueue(WORD qnum, const char DLL_FAR_ *fspec);

    extern char ftempname[CCT_PATHLEN];  /* temporary name for receive files */

    #if defined(__cplusplus)
            }
    #endif
#endif
