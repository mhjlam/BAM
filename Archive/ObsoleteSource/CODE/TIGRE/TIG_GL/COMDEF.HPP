//*
//* COMDEF.HPP
//*
//* Contains:
//*     Definitions for Comm++ constants, enums and error codes
//*
//*
//* The Greenleaf Comm++ Library
//*
//* Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
//*
//* MODIFICATIONS
//* April 23, 1992 - Changed the OVERFLOW enumerator because of a
//*                  name clash with the MATH.H file from Borland.
//* June 30, 1993 - Modified the header file to support TopSpeed 3.1
//*                 for OS/2.
//* August 19, 1995 - Added new macros for the GetWindowsVersion function.
//*

#ifndef COMDEF_H
#define COMDEF_H

#include "cppcomp.h"
#include "asciidef.h"
#include "_defs386.h"

#if defined( GCPP_MICROSOFT )
  #pragma warning( disable:4505 )
#endif

typedef unsigned char GCPP_BYTE;
typedef short FLAG;


#if 0
	#error Don't include this stuff in TIGRE applications!

		#if !defined( TRUE )
			#error	Not again!
  		#define FALSE 0
  		#define TRUE 1
		#endif

		#if !defined( NULL )
			#if defined( GCPP_SMALL ) || defined( GCPP_MEDIUM )
    		#define  NULL    0
  		#else
    		#define  NULL    0L
  		#endif
		#endif
#endif		// TIGRE


#if !defined( GCPP_DELETE )
  #define GCPP_DELETE   0x5300
#endif

#define GCPP_MKFP( seg, offset ) ( void far *) ( ( (unsigned long) \
                                   ( seg ) << 16 ) + ( offset ) )

#define TICKS_PER_SECOND              18
#define MILLISECONDS_PER_TICK         55

//*
//* A defination for the ViewerPort function pointer.  We had to do
//* this to make Watcom happy.
//*
typedef  void (GCPP_DLL_FAR *VIEWER_OUTPUT)( char GCPP_DLL_FAR *data );


#define GCPP_NUMPORTS     34
#define GCPP_NUMIRQS      8
#define GCPP_MAXWINPORTS  9

//*
//* defaults for interrupt buffers
//*
#define GCPP_BUFTYPE_FIXED        0
#define GCPP_BUFTYPE_CHANGE       1
#define GCPP_BUFTYPE_INSTALL      2
#define GCPP_RX                   0
#define GCPP_TX                   1

//*
//* Used for getting buffer info
//*
#define GCPP_RXINFO     1
#define GCPP_TXINFO     2
#define GCPP_FLOWINFO   4

//*
//* used for GFLinkInfo.status
//*
#define GCPP_RXPAUSED     1
#define GCPP_TXPAUSED     2

//*
//* Commnunication Buffer default sizes
//*
const unsigned short GCPP_RXSIZE = 4096;
const unsigned short GCPP_TXSIZE = 2048;
const unsigned short GCPP_RXROOM =  128;

//*
//* Modem Speaker Control Constants
//*
#define SPEAKER_OFF         0
#define SPEAKER_ON          1
#define SPEAKER_ON_ALWAYS   2
#define USR_DEF_1           3
#define USR_DEF_2           4


//*
//* Comm++ Constants
//*
enum rs232
{
  GCPP_NONE, XONXOFF,
  START_outs,
    DTR, RTS, OUT1, OUT2, BREAK, LOOPBACK,
  START_ins,
    IN1, IN2, CTS, DSR, RI, DCD,
  START_deltas,
    DELTA_CTS, DELTA_DSR, DELTA_RI, DELTA_DCD, DELTA_IN1, DELTA_IN2,
  START_errors,
    RX_READY, OVERRUN, PARITY, FRAMING, GCPP_BREAK_DETECTED,
    TX_CLEAR, TX_READY, GCPP_OVERFLOW,
  START_ints,
    IRX, ITX, ILS, IMS
};

//*
//* 16550 TRIGGER Level Settings
//*
typedef enum trigger_level{
    TRIGGER_DISABLE    = 0x00,
    TRIGGER_01         = 0x01,
    TRIGGER_04         = 0x41,
    TRIGGER_08         = 0x81,
    TRIGGER_14         = 0xc1
} TRIGGER_LEVEL;


//*
//* Port Constants
//*
const unsigned short USEDEFAULT = 0;
const unsigned short COM1 = 0;
const unsigned short COM2 = 1;
const unsigned short COM3 = 2;
const unsigned short COM4 = 3;
const unsigned short COM5 = 4;
const unsigned short COM6 = 5;
const unsigned short COM7 = 6;
const unsigned short COM8 = 7;
const unsigned short COM9 = 8;
const unsigned short COM10 = 9;
const unsigned short COM11 = 10;
const unsigned short COM12 = 11;
const unsigned short COM13 = 12;
const unsigned short COM14 = 13;
const unsigned short COM15 = 14;
const unsigned short COM16 = 15;
const unsigned short COM17 = 16;
const unsigned short COM18 = 17;
const unsigned short COM19 = 18;
const unsigned short COM20 = 19;
const unsigned short COM21 = 20;
const unsigned short COM22 = 21;
const unsigned short COM23 = 22;
const unsigned short COM24 = 23;
const unsigned short COM25 = 24;
const unsigned short COM26 = 25;
const unsigned short COM27 = 26;
const unsigned short COM28 = 27;
const unsigned short COM29 = 28;
const unsigned short COM30 = 29;
const unsigned short COM31 = 30;
const unsigned short COM32 = 31;
const unsigned short COM33 = 32;
const unsigned short COM34 = 33;
const unsigned short COM35 = 34;

//*
//* IRQ constants
//*
const int IRQ1 = 1;
const int IRQ2 = 2;
const int IRQ3 = 3;
const int IRQ4 = 4;
const int IRQ5 = 5;
const int IRQ6 = 6;
const int IRQ7 = 7;
const int IRQ8 = 8;

//*
//* Constants for set - functions
//*
#define GCPP_UNKNOWN 2
#define GCPP_OFF 0
#define GCPP_ON  1
#define GCPP_OUT 0
#define GCPP_IN  1

//*
//* Windows Platform Versions
//*
#define GCPP_SYS_UNKNOWN_WIN  1
#define GCPP_SYS_WIN3X        2
#define GCPP_SYS_WFW          3
#define GCPP_SYS_WIN32S       4
#define GCPP_SYS_WIN32C       5
#define GCPP_SYS_WINNT        6
#define GCPP_SYS_WINNTAS      7


//*
//* Error definitions
//*
#define GCPP_OK            0            //* All OK folks!!
#define GCPP_ERROR        -801          //* General error
#define GCPP_TIMEOUT      -802          //* Timed out
#define GCPP_NOT_SUPP     -803          //* Not supported
#define GCPP_EOF          -804          //* End of file transfer
#define GCPP_BUFFER_FULL  -805          //* Buffer was full

//*
//* Definitions for error in GFSerial::getcommerror(unsigned short &error)
//*
#define GCPP_PARITY_ERROR       0x0001
#define GCPP_FRAMING_ERROR      0x0002
#define GCPP_RX_OVERFLOW_ERROR  0x0004
#define GCPP_OVERRUN_ERROR      0x0008
#define GCPP_BREAK_RECIEVED     0x0010

//*
//* Definition for System errors returned by GFSerial::getcommerror()
//*
#define GCPP_BREAK_ERROR            -827  //* Error setting up Break condition
#define GCPP_LINK_ERROR             -828  //* Comms Link couldn't be opened
#define GCPP_CLOSE_ERROR            -829  //* Comms Link couldn't be closed
#define GCPP_BADMODE_ERROR          -830  //* Bad Handshaking Mode requested
#define GCPP_BADLINE_ERROR          -831  //* Attempt to change read only line
#define GCPP_BADARGS_ERROR          -832  //* Argument to function is bad
#define GCPP_SYSFAILED_ERROR        -833  //* System call failed
#define GCPP_WRITE_ERROR            -834  //* Error during write operation
#define GCPP_READ_ERROR             -835  //* Error during read operation
#define GCPP_ENOMEM_ERROR           -836  //* A memory allocation failed
#define GCPP_ISOPEN_ERROR           -838  //* Link is already open
#define GCPP_ILLEGAL_BAUDRATE       -839  //* Used an unsupported baudrate setting
#define GCPP_ILLEGAL_PARITY         -840  //* Used an unsupported parity setting
#define GCPP_ILLEGAL_STOPBIT        -841  //* Used an unsupported stopbit setting
#define GCPP_ILLEGAL_WORDLENGTH     -842  //* Used an unsupported wordlength setting
#define GCPP_DRIVER_NOT_INSTALLED   -843  //* Device driver not installed
#define GCPP_DRIVER_NOT_SUPPORTED   -844  //* This device driver is not supported
#define GCPP_GENERAL_DRIVER_ERROR   -845  //* The driver had a general failure
#define GCPP_ILLEGAL_BOARD_NUMBER   -846  //* This board number is not installed
#define GCPP_USER_ABORT             -847  //* The user has selected to abort
#define GCPP_NOHAYESOK              -848  //* There was no OK from modem in specifed time
#define GCPP_BOARD_INUSE            -849  //* This board has already been configured
#define GCPP_NOPORT                 -850  //* No GFSerial object is attached to class in use
#define GCPP_DOSEXTENDER_ERROR      -851  //* An error occured using DOS extender API
#define GCPP_IRQINUSE_ERROR         -852  //* This IRQ is already used by another comm port
#define GCPP_TOOMANYHOOKS           -853  //* Tried to hook more than the allowed IRQs
#define GCPP_GENERAL_ERROR          -854  //* General Error, non-specfic.
#define GCPP_NASI_ERROR             -855  //* A error occured using the NASI interface.
#define GCPP_NOT_SETUP_ERROR        -856  //* NASI.EXE or NASI.DLL was not setup properly.
#define GCPP_CONNECT_FAILURE        -857  //* Unable to connect to a modem on a NASI server.
#define GCPP_THUNK_SETUP_FAILED     -858  //* Win 32s thunking layer failed to initialize.

//*
//* file transfer errors
//*
#define GCPP_ERR_U              -901    //* User cancelled
#define GCPP_ERR_F              -902    //* File open error
#define GCPP_ERR_FR             -903    //* File read error
#define GCPP_ERR_FW             -904    //* File write error
#define GCPP_ERR_FC             -905    //* File close error
#define GCPP_ERR_N              -906    //* Transfer error
#define GCPP_ERR_R              -907    //* To many retries
#define GCPP_ERR_E              -908    //* To many errors
#define GCPP_ERR_T              -909    //* Timeout error
#define GCPP_ERR_EOF            -910    //* RX EOF error
#define GCPP_ERR_X              -911    //* TX EOF error
#define GCPP_ERR_EOT            -912    //* End of transfer error
#define GCPP_ERR_S              -913    //* Sync. error
#define GCPP_ERR_C              -914    //* Remote cancelled
#define GCPP_ERR_IN             -915    //* Init not acknowledged
#define GCPP_ERR_NAK            -916    //* Packet NAK'ed
#define GCPP_ERR_LP             -917    //* Previous packet received
#define GCPP_ERR_PI             -918    //* Packet integrity error
#define GCPP_ERR_FH             -919    //* File header error
#define GCPP_ERR_MEM            -920    //* Insufficient memory for transfer
#define GCPP_ERR_LOGIC          -921    //* Internal logic error
#define GCPP_ERR_CSN            -922    //* Can't send NACK
#define GCPP_ERR_CSA            -923    //* Can't send ACK
#define GCPP_ERR_CPC            -924    //* Can't put char
#define GCPP_ERR_CPB            -925    //* Can't put buffer
#define GCPP_ERR_NOF            -926    //* Can't find a filename
#define GCPP_ERR_NFE            -927    //* File Engine not created
#define GCPP_ERR_PRO            -928    //* Protocol error
#define GCPP_ERR_CD             -929    //* Lost carrier signal
#define GCPP_ERR_CGC            -930    //* Can't get char
#define GCPP_ERR_GEN            -931    //* General Transfer error
#define GCPP_ERR_RNF            -932    //* Sent no files
#define GCPP_ERR_SZ             -933    //* Receiver should send ZSKIP
#define GCPP_ERR_BPS            -934    //* Bad packet sequence
#define GCPP_ERR_PA             -935    //* Packet arrived
#define GCPP_ERR_FE_PTRUSED     -950    //* File engine pointer not NULL
#define GCPP_ERR_FE_OPEN        -951    //* File engine open failed
#define GCPP_ERR_FE_CLOSE       -952    //* File engine close failed
#define GCPP_ERR_FE_INVPTR      -953    //* File engine invalid pointer
#define GCPP_ERR_FE_SETPOS      -954    //* File engine unable to set position
#define GCPP_ERR_FE_GETPOS      -955    //* File engine unable to get position
#define GCPP_ERR_FE_MEM         -956    //* File engine insufficent memory
#define GCPP_ERR_FE_TD          -957    //* File engine time/date error

#endif
