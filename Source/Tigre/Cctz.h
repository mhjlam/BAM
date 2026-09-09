/*
Copyright (C) Magna Carta Software, Inc. 1992-1994.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
CCTZ.H -- Manifest constants for ZMODEM file transfer protocol;
*/

#if !defined(CCTZ_H_INCLUDED)
    #define CCTZ_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    
    /* CONSTANTS DENOTING ZMODEM OPTIONS */
    #define CCT_Z_BLKLEN        0X1000
    #define CCT_Z_CAPABILITY    0X1001
    #define CCT_Z_CONVERT       0X1002
    #define CCT_Z_MANAGE        0X1003
    #define CCT_Z_RETRIES       0X1004
    #define CCT_Z_RXTIMEOUT     0X1005
/*  #define CCT_Z_RXWINDOW      0X1006 */


    /* BIT MASKS FOR ZMODEM RECEIVER CAPABILITIES ZRINIT FLAGS BYTE ZF0 */
    #define CANFDX      0X01        /* Rx can send and receive true FDX */
    #define CANOVIO     0X02        /* Rx can receive data during disk I/O */
    #define CANBRK      0X04        /* Rx can send a break signal */
    #define CANCRY      0X08        /* Receiver can decrypt */
    #define CANLZW      0X10        /* Receiver can uncompress */
    #define CANFC32     0X20        /* Receiver can use 32 bit Frame Check */
    #define ESCCTL      0X40        /* Receiver expects ctl chars to be escaped */
    #define ESC8        0X80        /* Receiver expects 8th bit to be escaped */

    /* PARAMETERS FOR ZFILE FRAME */
    /* CONVERSION OPTIONS (ONE OF THESE IN ZF0) */
    #define ZCBIN       1           /* Binary transfer - inhibit conversion */
    #define ZCNL        2           /* Convert NL to local end of line convention */
    #define ZCRESUM     3           /* Resume interrupted file transfer */

    /* MANAGEMENT INCLUDE OPTIONS, ONE OF THESE ORED IN ZF1 */
    #define ZMSKNOLOC   0X80        /* Skip file if not present at rx */

    /* MANAGEMENT OPTIONS (MUTUALLY EXCLUSIVE IN ZF1) */
    #define ZMMASK      0X1F        /* Mask for the choices below */
    #define ZMNEWL      1           /* Transfer if source newer or longer */
	#define ZMCRC       2           /* Transfer if different file CRC or length */
    #define ZMAPND      3           /* Append contents to existing file (if any) */
    #define ZMCLOB      4           /* Replace existing file */
    #define ZMNEW       5           /* Transfer if source newer */
    #define ZMDIFF      6           /* Transfer if dates or lengths different */
    #define ZMPROT      7           /* Protect destination file */



    /* CONSTANTS DENOTING ZMODEM FRAME TYPES (FRAME TYPE NUMBER ORED WITH 0X2000 */
    #define Z_RQINIT     (0  | 0X2000)   /* Request receive init */
    #define Z_RINIT      (1  | 0X2000)   /* Receive init */
    #define Z_SINIT      (2  | 0X2000)   /* Send init sequence (optional) */
    #define Z_ACK        (3  | 0X2000)   /* ACK to above */
    #define Z_FILE       (4  | 0X2000)   /* File name from sender */
    #define Z_SKIP       (5  | 0X2000)   /* To sender: skip this file */
    #define Z_NAK        (6  | 0X2000)   /* Last packet was garbled */
    #define Z_ABORT      (7  | 0X2000)   /* Abort batch transfers */
    #define Z_FIN        (8  | 0X2000)   /* Finish session */
    #define Z_RPOS       (9  | 0X2000)   /* Resume data trans at this position */
    #define Z_DATA       (10 | 0X2000)   /* Data packet(s) follow */
    #define Z_EOF        (11 | 0X2000)   /* End of file */
    #define Z_FERR       (12 | 0X2000)   /* Fatal Read or Write error Detected */
    #define Z_CRC        (13 | 0X2000)   /* Request for file CRC and response */
    #define Z_CHALLENGE  (14 | 0X2000)   /* Receiver's Challenge */
    #define Z_COMPL      (15 | 0X2000)   /* Request is complete */
    #define Z_CAN        (16 | 0X2000)   /* Other end canned session with CAN*5 */
    #define Z_FREECNT    (17 | 0X2000)   /* Request for free bytes on filesystem */
    #define Z_COMMAND    (18 | 0X2000)   /* Command from sending program */
	#define Z_STDERR     (19 | 0X2000)   /* Output to standard error, data follows */

	#define Z_NEG_BS	 (20 | 0X2000)   /* ZMODEM negotiated packet size */

	#define ZMODEM_WAIT_TIME	60*1000	 /* long wait during sends */

    /* FUNCTION PROTOTYPES */
    short       FCALL_ EXPORT_  zmodem_deinit(hPORT p);
    long        FCALL_ EXPORT_  zmodem_get(hPORT p, short parm);
    short       FCALL_ EXPORT_  zmodem_set(hPORT p, short parm, long value);

    #if defined(__cplusplus)
        }
    #endif
#endif
