//*
//* COMPPLIB.HPP
//*
//* Contains:
//*     struct GFLinkInfo
//*     struct GFlinestat
//*     struct ModemInfo
//*     class GFLineStatus
//*     class GFDataFormat
//*     class GFSerial
//*     class GFSmartArnet
//*     class BIOS
//*     class ExtendedBIOS
//*     class SmartDigiBoard
//*     class SmartStarGate
//*     class GFComOS2
//*     class GFComWin3
//*     class GFChecksum
//*     class GFCheck2Byte
//*     class GFCrc16
//*     class GFCrc32
//*     class GFModem
//*
//*
//* The Greenleaf Comm++ Library
//*
//* Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
//*
//* MODIFICATIONS
//*   November 5, 1993 - Modified to support the IBM C/Set++ compiler.
//*   August 19, 1994 - Modified to support Watcom C++ 10.0 compiler.
//*   August 23, 1995 - Made several modifications to help support Win32s
//*                     better.  We reworked the way the Win32 and Win32s
//*                     classes are derived.
//*

#ifndef COMMPPLIB_HPP
#define COMMPPLIB_HPP

#include "cppcomp.h"
#include "comdef.hpp"
#include "compat20.h"
#include "_comlink.hpp"
#include "_defs386.h"

#if !defined( GCPP_WINDOWS ) &&\
    !defined( GCPP_OS2 ) &&\
    !( defined( GCPP_WIN32 ) && !defined( GCPP_X32 ) )
  #include "_dosisr.hpp"
  #if !defined( GCPP_X32 )
    #include "_star.hpp"
    #include "nasi.hpp"
  #endif
#elif defined( GCPP_WINDOWS ) || defined( GCPP_WIN32 )
    #if !defined( STRICT )
        #define STRICT
    #endif
	#include <windows.h>
  #if defined( GCPP_WINDOWS ) && !defined( GCPP_WIN32 )
    #include "nasi.hpp"
  #endif
#endif


//*
//* Struct GFLinkInfo is designed to allow the status of a
//* communications link to be queried. It returns information
//* concerning the current status of the port, the last error
//* to have occured and the number of characters and current
//* sizes of the send and recieve buffers. The member function
//* get_buf_info() of class GFComLink is called to obtain this
//* information.
//*
struct GFLinkInfo
{
  int rxtype;
  unsigned int rxcount;
  unsigned int rxsize;
  int txtype;
  unsigned int txcount;
  unsigned int txsize;
  unsigned short status;
};

//*
//* struct GFlinestat
//*
//* This structure is unsed internally by the GFLinesStatus class
//* Possible values for these fields are: GF_ON, GF_OFF and GF_UNKNOWN
//*
struct GCPP_CLASS_TYPE GFlinestat
{
  unsigned char data_terminal_ready :2;
  unsigned char ready_to_send       :2;
  unsigned char line_break          :2;
  unsigned char data_set_ready      :2;
  unsigned char clear_to_send       :2;
  unsigned char ring_indicator      :2;
  unsigned char carrier_detect      :2;
  unsigned char delta_carrier       :2;
  unsigned char delta_ring          :2;
  unsigned char delta_cts           :2;
  unsigned char delta_dsr           :2;
  unsigned char got_break           :2;
  unsigned char parity              :2;
  unsigned char framing             :2;
  unsigned char overflow            :2;
  unsigned char overrun             :2;
};



//*
//* class GFLineStatus definition
//*
//*
//* This class handle control and monitoring of the RS232 control lines
//* and of hardware and software handshaking.
//*
class GCPP_CLASS_TYPE GFLineStatus
{
  public:
    GCPP_PROTO GFLineStatus( GFSerial GCPP_DLL_FAR *sptr );
    GCPP_PROTO GFLineStatus( GFComLink GCPP_DLL_FAR *cptr );
    GCPP_PROTO GFLineStatus( GFLineStatus GCPP_DLL_FAR &ls );
    GCPP_PROTO ~GFLineStatus( void );

    GFLineStatus& GCPP_PROTO operator=( GFLineStatus GCPP_DLL_FAR &ls );
    void GCPP_PROTO Status( void );
    void GCPP_PROTO SendBreak( unsigned msecs = 300 );
    int GCPP_PROTO Dtr( int st = -1 );
    int GCPP_PROTO Rts( int st = -1 );
    int GCPP_PROTO Cts( void );
    int GCPP_PROTO Dsr( void );
    int GCPP_PROTO Ring( void );
    int GCPP_PROTO Carrier( void );
    int GCPP_PROTO CtsChanged( void );
    int GCPP_PROTO DsrChanged( void );
    int GCPP_PROTO BreakDetected( void );
    int GCPP_PROTO RingDetected( void );
    int GCPP_PROTO CarrierChanged( void );
    int GCPP_PROTO ParityError( void );
    int GCPP_PROTO FramingError( void );
    int GCPP_PROTO BufferOverflow( void );
    int GCPP_PROTO HardwareOverrun( void );
    int GCPP_PROTO LineBreak( int st = -1 );
    int GCPP_PROTO SetHandShake( int inhand, int outhand );
    int GCPP_PROTO GetHandShake( int GCPP_DLL_FAR &inhand,
                                 int GCPP_DLL_FAR &outhand );
    void GCPP_PROTO ReportLineStatus( void (GCPP_DLL_FAR *ViewerOutput)
                                      (char GCPP_DLL_FAR *data) );

  protected:
    int old_inhand;
    int old_outhand;
    int inhand;
    int outhand;
    GFlinestat statinfo;
    GFComLink GCPP_DLL_FAR *devp;
};



//*
//* class GFDataFormat definition
//*
//* Here we have class GFDataFormat. This class provides for general
//* control of the GFDataFormat parameters such as baudrate, parity
//* and data length for instance. It is designed to be device independant
//* working through the uart functions to carry out its purpose.
//*
class GCPP_CLASS_TYPE GFDataFormat
{
  public:
    GCPP_PROTO GFDataFormat( GFSerial GCPP_DLL_FAR *sp = (GFSerial *)NULL );
    GCPP_PROTO GFDataFormat( GFDataFormat GCPP_DLL_FAR &lf );
    GCPP_PROTO ~GFDataFormat( void );

    int GCPP_PROTO SetUp( long bd = 9600,
                          char pr = 'N',
                          int dt = 8,
                          int st = 1 );
    GFDataFormat& GCPP_PROTO operator = ( GFDataFormat GCPP_DLL_FAR &lf );
    void GCPP_PROTO Install( GFSerial GCPP_DLL_FAR *sp );
    char GCPP_PROTO Parity( void );
    char GCPP_PROTO Parity( char p );
    int GCPP_PROTO DataLength( void );
    int GCPP_PROTO DataLength( int dt );
    int GCPP_PROTO StopBits( void );
    int GCPP_PROTO StopBits( int st );
    long GCPP_PROTO BaudRate( void );
    long GCPP_PROTO BaudRate( long bd );
    void GCPP_PROTO ReportDataStatus( void (GCPP_DLL_FAR *ViewerOutput)
                                      (char GCPP_DLL_FAR *data) );

  protected:
    int GCPP_PROTO Get( void );
    int GCPP_PROTO Set( void );

    long baud;
    char par;
    int datalen;
    int stops;
    long oldbaud;
    char oldpar;
    int olddatalen;
    int oldstops;
    GFComLink GCPP_DLL_FAR *uart_ptr;
};


//*
//* class GFSerial defnitions
//*
class GCPP_CLASS_TYPE GFSerial
{
  friend class GCPP_CLASS_TYPE GFLineStatus;
  friend class GCPP_CLASS_TYPE GFDataFormat;

  public:
    GCPP_PROTO GFSerial( GFComLink GCPP_DLL_FAR *dp );
    virtual GCPP_PROTO ~GFSerial( void );

    GFLineStatus * GCPP_PROTO GetLineStatus( void );
    GFDataFormat * GCPP_PROTO GetDataFormat( void );
    int GCPP_PROTO SetRXBuffer( unsigned size = GCPP_RXSIZE,
                                unsigned room = GCPP_RXROOM );
    int GCPP_PROTO SetTXBuffer( unsigned size = GCPP_TXSIZE );
    int GCPP_PROTO RXFlush( unsigned count = 0 );
    void GCPP_PROTO TXFlush( void );
    unsigned GCPP_PROTO RXCount( void );
    unsigned GCPP_PROTO TXCount( void );
    void GCPP_PROTO GetBufferInfo( GFLinkInfo GCPP_DLL_FAR &sbc );
    virtual int GCPP_PROTO GetChar( unsigned msecs = 0 );
    long GCPP_PROTO GetString( unsigned char GCPP_DLL_FAR *string,
                               unsigned count,
                               int delim = -1,
                               long msecs = 0 );
    long GCPP_PROTO GetBuffer( unsigned char GCPP_DLL_FAR *buffer,
                               unsigned count,
                               unsigned msecs );
    virtual int GCPP_PROTO PutChar( unsigned char outc );
    unsigned GCPP_PROTO PutString( const unsigned char GCPP_DLL_FAR *string,
                                   int option = -2 );
    int GCPP_PROTO Printf( char GCPP_DLL_FAR *format, ... );
    unsigned GCPP_PROTO PutBuffer( unsigned char GCPP_DLL_FAR *buf,
                                   unsigned count );
    int GCPP_PROTO PutImm( unsigned char c );
    void GCPP_PROTO Delay( unsigned long msecs );
    void GCPP_PROTO Duplex( int d );
    void GCPP_PROTO EchoBack( int d );
    int GCPP_PROTO GetCommError( unsigned short GCPP_DLL_FAR &error );
    virtual int GCPP_PROTO GetKey( void );
    GFSerial& GCPP_PROTO operator << ( int c );
    GFSerial& GCPP_PROTO operator << ( const unsigned char GCPP_DLL_FAR *s );
    GFSerial& GCPP_PROTO operator >> ( int GCPP_DLL_FAR &c );
    int GCPP_PROTO CallSystemIdle( void );
    void GCPP_PROTO ViewPortStatus( void(GCPP_DLL_FAR *ViewerOutput)
                                    (char GCPP_DLL_FAR *data) );
		int GCPP_PROTO BlockingOn( void );
		int GCPP_PROTO BlockingOff( void );
    GFComLink GCPP_DLL_FAR * GCPP_PROTO GetComLinkPointer( void );

		unsigned long wait;

  protected:
    FLAG isopen;
    GFComLink GCPP_DLL_FAR *devp;
    GFLineStatus GCPP_DLL_FAR *line;
    GFDataFormat GCPP_DLL_FAR *format;
};

#if !defined( GCPP_WINDOWS )  &&\
    !defined( GCPP_OS2 )      &&\
    !defined( GCPP_X32 )      &&\
    !defined( GCPP_WIN32 )
//*
//* These classes are only usable on a DOS platform.
//*

//*
//* class GFArnet definition
//*
class GFSmartArnet : public GFComLink
{
  friend class GFSerial;
  friend class GFDataFormat;
  friend class GFLineStatus;

  public:
    GFSmartArnet( int com = COM5 );
    GFSmartArnet( GFSmartArnet &prev );
    ~GFSmartArnet( void );

  protected:
    void Flush( void );
    void Initialize( int com );
    int Open( void );
    int Close( void );
    void GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int NewLinkSize( unsigned insize,
                     unsigned inroom,
                     unsigned outsize = 0 );
    int SetHandShake( int inhand, int outhand );
    int GetHandShake( int &inhand, int &outhand );
    int LineBreak( int state );
    int LineControl( int line, int state = -1 );
    int SetDataFormat( long bd, char pr, int dl, int st );
    void GetDataFormat( long &bd, char &pr, int &dl, int &st );
    void UpdateLineStatus( GFlinestat &line );
    int PutChar( unsigned char b );
    unsigned PutString( const unsigned char *string );
    unsigned PutBuffer( const unsigned char *buffer, unsigned n );
    int GetChar( void );
    unsigned GetBuffer( unsigned char *buffer, unsigned count );
    int ClearLink( int dir, unsigned numbytes = 0 );
    void ReportLinkStatus( void(*ViewerOutput)(char *data) );
    virtual int SystemIdle( void );

    unsigned short AIB_LineStatus;
    unsigned short AIB_ModemStatus;
    int inhand;
    int outhand;
    unsigned AIB_DTR;
    unsigned AIB_RTS;
};


//*
//* class GFBIOS definition
//*
class GFBIOS : public GFComLink
{
  friend class GFSerial;
  friend class GFDataFormat;
  friend class GFLineStatus;

  public:
    GFBIOS( int com = COM1 );
    GFBIOS( GFBIOS &prev );
    ~GFBIOS( void );

  protected:
    void Initialize( int com );
    int Open( void );
    virtual int Close( void );
    void GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int NewLinkSize( unsigned insize,
                     unsigned inroom,
                     unsigned outsize);
    int SetHandShake( int inmode, int outmode );
    int GetHandShake( int &inmode, int &outmode );
    virtual int LineBreak( int state );
    virtual int LineControl( int line, int state = -1 );
    int GetChar( void );
    unsigned GetBuffer( unsigned char *buffer, unsigned count );
    virtual int SetDataFormat( long bd,
                               char pr,
                               int dl,
                               int st );
    void GetDataFormat( long &bd,
                        char &pr,
                        int &dl,
                        int &st );
    void UpdateLineStatus( GFlinestat &line );
    int PutChar( unsigned char b );
    unsigned PutString( const unsigned char *strng );
    unsigned PutBuffer( const unsigned char *buffer, unsigned count );
    void Flush( void );
    int ClearLink( int dir, unsigned numbytes );
    virtual void ReportLinkStatus( void(*ViewerOutput)(char *data) );
    virtual int SystemIdle( void );

    unsigned short B_LineStatus;
    unsigned short B_ModemStatus;
};


//*
//* class GFExtendedBIOS definition
//*
class GFExtendedBIOS : public GFBIOS
{
  public:
    GFExtendedBIOS( int com = COM1 );
    GFExtendedBIOS( GFExtendedBIOS &prev );

  protected:
    int Close( void );
    int LineBreak( int state );
    int LineControl( int line, int state );
    int SetDataFormat( long bd, char pr, int dl, int st );
    void ReportLinkStatus( void(*ViewerOutput)(char *data) );

    int BX_Reg;
    int CX_Reg;
    int break_state;
};

   

//*
//* class GFSmartDigiBoard defintion
//*
class GFSmartDigiBoard : public GFComLink
{
  friend class GFSerial;
  friend class GFDataFormat;
  friend class GFLineStatus;

  public:
    GFSmartDigiBoard( int com = COM5 );
    GFSmartDigiBoard( GFSmartDigiBoard &prev );
    ~GFSmartDigiBoard( void );

  protected:
    void Initialize( int com );
    int Open( void );
    void Flush( void );
    int Close( void );
    int GetError( void );
    void GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int NewLinkSize( unsigned insize,
                     unsigned inroom,
                     unsigned outsize = 0 );
    int SetHandShake( int inhand, int outhand );
    int GetHandShake( int& inhand, int& outhand );
    int LineBreak( int state );
    int LineControl( int line, int state = -1 );
    int SetDataFormat( long bd, char pr, int dl, int st );
    void GetDataFormat( long &bd, char &pr, int &dl, int &st );
    void UpdateLineStatus( GFlinestat &line );
    int PutChar( unsigned char b );
    unsigned PutString( const unsigned char *strng );
    unsigned PutBuffer( const unsigned char *buffer, unsigned count );
    int PutImm( unsigned char b );
    int GetChar( void );
    unsigned GetBuffer( unsigned char *buffer, unsigned count );
    int ClearLink( int dir, unsigned numbytes = 0 );
    int SetDigiboardDriverType( void );
    void ReportLinkStatus( void (*ViewerOutput)(char *data) );
    virtual int SystemIdle( void );

    unsigned short SDB_ModemStatus;
    unsigned short SDB_LineStatus;

    int inhand;
    int outhand;
    int break_state;
    char driver_name[ 9 ];
    char port_name[ 9 ];
};


class GFSmartStarGate : public GFComLink
{
  friend class GFSerial;
  friend class GFDataFormat;
  friend class GFLineStatus;

  public:
    GFSmartStarGate( int com = COM5 );
    GFSmartStarGate( GFSmartStarGate &prev );
    ~GFSmartStarGate( void );
    static int ConfigureSmartStarGateBoard( int board = 0,
                                            unsigned int board_segment = 0xd000,
                                            unsigned int port = 0x200,
                                            int first_port_number = COM5,
                                            int number_of_ports = 16 );

  protected:
    void Initialize( int com );
    int Open( void );
    void Flush( void );
    int Close( void );
    int GetError( void );
    void GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int NewLinkSize( unsigned insize,
                     unsigned inroom,
                     unsigned outsize = 0 );
    int SetHandShake( int inhand, int outhand );
    int GetHandShake( int &inhand, int &outhand );
    int LineBreak( int state );
    int LineControl( int line, int state = -1 );
    int SetDataFormat( long bd, char pr, int dl, int st );
    void GetDataFormat( long &bd, char &pr, int &dl, int &st );
    void UpdateLineStatus( GFlinestat &line );
    int PutChar( unsigned char b );
    unsigned PutString( const unsigned char *strng );
    unsigned PutBuffer( const unsigned char *buffer, unsigned count );
    int PutImm( unsigned char b );
    int GetChar( void );
    unsigned GetBuffer( unsigned char *buffer, unsigned count );
    int ClearLink( int dir, unsigned numbytes = 0 );
    void ReportLinkStatus( void (*ViewerOutput)(char *data) );
    virtual int SystemIdle( void );
    int SendCCBCommand( int cmd );
    void EnableRam( void );
    void DisableRam( void );

    static STARGATE_BOARD SSG_Boards[ MAX_BOARDS ];
    unsigned short SSG_ModemStatus;
    unsigned short SSG_LineStatus;
    unsigned char volatile far *SSG_Buffer;
    CHANNEL_CONTROL_BLOCK volatile far *ccb;
    int ram_latch;
    int ram_enable;
    int ram_disable;
    int board_number;
};


#endif

#if defined( GCPP_OS2 )
//*
//* class GFComOS2 definition
//*
class GFComOS2 : public GFComLink
{
  friend class GFSerial;
  friend class GFDataFormat;
  friend class GFLineStatus;

  public:
    GFComOS2( int com = COM1 );
    GFComOS2( GFComOS2 &prev );
    ~GFComOS2( void );

  protected:
    void Flush( void );
    int DoError( unsigned short err )
      { return(LSerror = err); }
    void Initialize( int com );
    int Open( void );
    int Close( void );
    int GetError( void );
    void GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int NewLinkSize( unsigned insize,
                     unsigned inroom,
                     unsigned outsize = 0 );
    int SetHandShake( int inhand, int outhand );
    int GetHandShake( int &inhand, int &outhand );
    int LineBreak( int state );
    int LineControl( int line, int state = -1 );
    int SetDataFormat( long bd, char pr, int dl, int st );
    void GetDataFormat( long &bd, char &pr, int &dl, int &st );
    void UpdateLineStatus( GFlinestat &line );
    int PutChar( unsigned char b );
    unsigned PutString( const unsigned char *string );
    unsigned PutBuffer( const unsigned char *buffer, unsigned count );
    int PutImm( unsigned char b );
    unsigned PutChars( const unsigned char* buf, unsigned n );
    int GetChar( void );
    unsigned GetBuffer( unsigned char *buffer, unsigned count );
    int ClearLink( int dir, unsigned numbytes = 0 );
    void ReportLinkStatus( void (*ViewerOutput)(char *data) );
    virtual int SystemIdle( void );

    unsigned char *iBuf;
    unsigned long iBufSize;
    unsigned long iBuffered;
    unsigned long iUsed;
    unsigned long handle;
    int inhand;
    int outhand;
    unsigned short LSsignals[2];
    unsigned short LSerror;
    unsigned long ParmLengthInOut;
    unsigned long DataLengthInOut;
};

#define GFCOMLINK GFComOS2

#endif

#if defined( GCPP_WINDOWS ) || defined( GCPP_WIN32 ) && !defined( GCPP_X32 )
//*
//* Enumerated type for the event mask used for enabling WM_COMMNOTIFY
//* messages in GFComWin3 or waiting for a comm event in GFComWin32.
//*
enum WinEventFlags {
    ON_EVENTS_DISABLED          = 0,
    ON_RECEIVE_CHARACTER        = EV_RXCHAR,
    ON_RECEIVE_EVENT_CHARACTER  = EV_RXFLAG,
    ON_TRANSMIT_EMPTY           = EV_TXEMPTY,
    ON_CTS_CHANGED              = EV_CTS,
    ON_DSR_CHANGED              = EV_DSR,
    ON_CD_CHANGED               = EV_RLSD,
    ON_BREAK_DETECTED           = EV_BREAK,
    ON_LINE_STATUS_ERROR        = EV_ERR,
    ON_RING_DETECTED            = EV_RING
};
#endif


#if defined( GCPP_WIN32 ) && !defined( GCPP_X32 )
//*
//* class GFComWin32 definition
//*

#if !defined( EV_RINGTE )
#define EV_RINGTE 0x2000
#endif

#define GCPP_EVENTMASK EV_BREAK|EV_CTS|EV_DSR|EV_ERR|EV_RING|EV_RLSD|EV_RXCHAR|EV_RXFLAG|EV_TXEMPTY|EV_RINGTE

enum Lines { EDTR=0x01, ERTS, ECTS=0x10, EDSR=0x20, ERI=0x40, EDCD=0x80 };
#define GFCOMLINK GFComWin32

class GCPP_CLASS_TYPE GFComWin32 : public GFComLink
{
  friend class GCPP_CLASS_TYPE GFSerial;
  friend class GCPP_CLASS_TYPE GFDataFormat;
  friend class GCPP_CLASS_TYPE GFLineStatus;

  public:
    GCPP_PROTO GFComWin32( int com = COM1,
                          unsigned rxsize = GCPP_RXSIZE,
                          unsigned txsize = GCPP_TXSIZE,
                          int bypass = 0 );
    GCPP_PROTO GFComWin32( GFComWin32 GCPP_DLL_FAR &prev );
    virtual GCPP_PROTO ~GFComWin32( void );
    virtual void GCPP_PROTO SetEventCharacter( char event_character );
    virtual int GCPP_PROTO GetEventMask( unsigned long &event_signal_mask );
    virtual int GCPP_PROTO SetEventMask( unsigned long event_signal_mask );
    virtual int GCPP_PROTO WaitForEvent( unsigned long &event_signal_mask );

  protected:
    virtual void GCPP_PROTO Flush( void );
    virtual int  GCPP_PROTO GetLine( enum Lines line );
    virtual void GCPP_PROTO Initialize( int com );
    virtual int GCPP_PROTO Open( void );
    virtual int GCPP_PROTO Close( void );
    virtual void GCPP_PROTO GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    virtual int GCPP_PROTO NewLinkSize( unsigned insize,
                                unsigned inroom,
                                unsigned outsize = 0 );
    virtual int GCPP_PROTO SetHandShake( int inhand, int outhand );
    virtual int GCPP_PROTO GetHandShake( int GCPP_DLL_FAR &inhand,
                                 int GCPP_DLL_FAR &outhand );
    virtual int GCPP_PROTO LineBreak( int state );
    virtual int GCPP_PROTO LineControl( int line, int state = -1 );
    virtual int GCPP_PROTO SetDataFormat( long bd, char pr, int dl, int st );
    virtual void GCPP_PROTO GetDataFormat( long GCPP_DLL_FAR &bd,
                                   char GCPP_DLL_FAR &pr,
                                   int GCPP_DLL_FAR &dl,
                                   int GCPP_DLL_FAR &st );
    virtual void GCPP_PROTO UpdateLineStatus( GFlinestat GCPP_DLL_FAR &line );
    virtual int GCPP_PROTO GetChar( void );
    virtual unsigned GCPP_PROTO GetBuffer( unsigned char GCPP_DLL_FAR *buffer,
                                   unsigned count );
    virtual int GCPP_PROTO PutChar( unsigned char b );
    virtual unsigned GCPP_PROTO PutString( const unsigned char GCPP_DLL_FAR *string );
    virtual unsigned GCPP_PROTO PutBuffer( const unsigned char GCPP_DLL_FAR *buffer,
                                   unsigned count );
    virtual int GCPP_PROTO PutImm( unsigned char b );
    virtual int GCPP_PROTO ClearLink( int dir, unsigned numbytes = 0 );
    virtual int GCPP_PROTO TranslateWin32ErrorCode( void );
    virtual void GCPP_PROTO ReportLinkStatus( VIEWER_OUTPUT ViewerOutput );
    virtual int GCPP_PROTO SystemIdle( void );

    int GCPP_PROTO GetInUseFlag( int pid );
    void GCPP_PROTO IncrementInUseFlag( int pid );
    void GCPP_PROTO DecrementInUseFlag( int pid );

    DCB GCPP_DLL_FAR *CommDCB;
    OVERLAPPED *com_ov;
    COMMTIMEOUTS *comm_timeouts;
    HANDLE handle;
    int inhand;
    int outhand;
    int WinRts;
    int WinDtr;
    unsigned short base;
    unsigned short breakon;
    unsigned LSerror;
    unsigned LSsignals;
    DWORD event_mask;
    unsigned txqueue;
    unsigned rxqueue;

};

GCPP_TYPE GFComWin32 * GCPP_FUN CreateWin32Port( int port,
                                      unsigned rxsize = GCPP_RXSIZE,
                                      unsigned txsize = GCPP_TXSIZE );

#if defined( GCPP_WIN32S )
class GCPP_CLASS_TYPE GFComWin32s : public GFComWin32
{
  friend class GCPP_CLASS_TYPE GFSerial;
  friend class GCPP_CLASS_TYPE GFDataFormat;
  friend class GCPP_CLASS_TYPE GFLineStatus;

  public:
    GCPP_PROTO GFComWin32s( int com = COM1,
                          unsigned rxsize = GCPP_RXSIZE,
                          unsigned txsize = GCPP_TXSIZE );
    GCPP_PROTO GFComWin32s( GFComWin32s GCPP_DLL_FAR &prev );
    GCPP_PROTO ~GFComWin32s( void );
    void GCPP_PROTO SetEventCharacter( char event_character );
    int GCPP_PROTO GetEventMask( unsigned long &event_signal_mask );
    int GCPP_PROTO SetEventMask( unsigned long event_signal_mask );
    int GCPP_PROTO WaitForEvent( unsigned long &event_signal_mask );

  protected:
    void GCPP_PROTO Flush( void );
    int  GCPP_PROTO GetLine( enum Lines line );
    void GCPP_PROTO Initialize( int com );
    int GCPP_PROTO Open( void );
    int GCPP_PROTO Close( void );
    void GCPP_PROTO GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int GCPP_PROTO NewLinkSize( unsigned insize,
                                unsigned inroom,
                                unsigned outsize = 0 );
    int GCPP_PROTO SetHandShake( int inhand, int outhand );
    int GCPP_PROTO GetHandShake( int GCPP_DLL_FAR &inhand,
                                 int GCPP_DLL_FAR &outhand );
    int GCPP_PROTO LineBreak( int state );
    int GCPP_PROTO LineControl( int line, int state = -1 );
    int GCPP_PROTO SetDataFormat( long bd, char pr, int dl, int st );
    void GCPP_PROTO GetDataFormat( long GCPP_DLL_FAR &bd,
                                   char GCPP_DLL_FAR &pr,
                                   int GCPP_DLL_FAR &dl,
                                   int GCPP_DLL_FAR &st );
    void GCPP_PROTO UpdateLineStatus( GFlinestat GCPP_DLL_FAR &line );
    int GCPP_PROTO GetChar( void );
    unsigned GCPP_PROTO GetBuffer( unsigned char GCPP_DLL_FAR *buffer,
                                   unsigned count );
    int GCPP_PROTO PutChar( unsigned char b );
    unsigned GCPP_PROTO PutString( const unsigned char GCPP_DLL_FAR *string );
    unsigned GCPP_PROTO PutBuffer( const unsigned char GCPP_DLL_FAR *buffer,
                                   unsigned count );
    int GCPP_PROTO PutImm( unsigned char b );
    int GCPP_PROTO ClearLink( int dir, unsigned numbytes = 0 );
    int GCPP_PROTO TranslateWin32ErrorCode( void );
    void GCPP_PROTO ReportLinkStatus( VIEWER_OUTPUT ViewerOutput );
    virtual int GCPP_PROTO SystemIdle( void );

    long CommHandle;
};

extern "C"
#if defined( GCPP_BORLAND )
GCPP_TYPE long GCPP_FUN AllocateWin32sPort( int,
                unsigned rxsize,
                unsigned txsize );
#else
GCPP_TYPE long AllocateWin32sPort( int port,
                unsigned rxsize,
                unsigned txsize );
#endif
//
//extern "C" _declspec( dllexport ) void * _stdcall AllocateWin32sPort( int port,
//                                                      unsigned rxsize,
//                                                      unsigned txsize );
#endif


#define GFCOMLINK GFComWin32

#endif


#if defined( GCPP_WINDOWS ) && !defined( GCPP_WIN32 )
//*
//* class GFComWin3 definition
//*

#if !defined( EV_RINGTE )
#define EV_RINGTE 0x2000
#endif

#define GCPP_EVENTMASK EV_BREAK|EV_CTS|EV_DSR|EV_ERR|EV_RING|EV_RLSD|EV_RXCHAR|EV_RXFLAG|EV_TXEMPTY|EV_RINGTE

enum Lines { EDTR=0x01, ERTS, ECTS=0x10, EDSR=0x20, ERI=0x40, EDCD=0x80 };

class GCPP_CLASS_TYPE GFComWin3 : public GFComLink
{
  friend class GCPP_CLASS_TYPE GFSerial;
  friend class GCPP_CLASS_TYPE GFDataFormat;
  friend class GCPP_CLASS_TYPE GFLineStatus;

  public:
    GCPP_PROTO GFComWin3( int com = COM1,
                          unsigned rxsize = GCPP_RXSIZE,
                          unsigned txsize = GCPP_TXSIZE );
    GCPP_PROTO GFComWin3( GFComWin3 GCPP_DLL_FAR &prev );
    GCPP_PROTO ~GFComWin3( void );
    unsigned int GCPP_PROTO GetMaskedEvents( void );
    int GCPP_PROTO GetCommHandle( void );
    unsigned int GCPP_PROTO GetEventMask( void );
    void GCPP_PROTO SetEventCharacter( char event_character );
    int GCPP_PROTO EnableNotification( GFSerial *port,
                                       HWND win_handle,
                                       int in_notify,
                                       int out_notify,
                                       unsigned event_mask );
    int GCPP_PROTO IsNotification( void );
    int GCPP_PROTO IsNotificationSuspended( void );
    void GCPP_PROTO SuspendNotification( void );
    void GCPP_PROTO ResumeNotification( void );

  protected:
    void GCPP_PROTO Flush( void );
    int  GCPP_PROTO GetLine( enum Lines line );
    void GCPP_PROTO Initialize( int com );
    int GCPP_PROTO Open( void );
    int GCPP_PROTO Close( void );
    void GCPP_PROTO GetBufferInfo( GFLinkInfo GCPP_DLL_FAR &sbc,
            int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int GCPP_PROTO NewLinkSize( unsigned insize,
                                unsigned inroom,
                                unsigned outsize = 0 );
    int GCPP_PROTO SetHandShake( int inhand, int outhand );
    int GCPP_PROTO GetHandShake( int GCPP_DLL_FAR &inhand,
                                 int GCPP_DLL_FAR &outhand );
    int GCPP_PROTO LineBreak( int state );
    int GCPP_PROTO LineControl( int line, int state = -1 );
    int GCPP_PROTO SetDataFormat( long bd, char pr, int dl, int st );
    void GCPP_PROTO GetDataFormat( long GCPP_DLL_FAR &bd,
                                   char GCPP_DLL_FAR &pr,
                                   int GCPP_DLL_FAR &dl,
                                   int GCPP_DLL_FAR &st );
    void GCPP_PROTO UpdateLineStatus( GFlinestat GCPP_DLL_FAR &line );
    int GCPP_PROTO GetChar( void );
		unsigned GCPP_PROTO GetBuffer( unsigned char GCPP_DLL_FAR *buffer,
                                   unsigned count );
    int GCPP_PROTO PutChar( unsigned char b );
    unsigned GCPP_PROTO PutString( const unsigned char GCPP_DLL_FAR *string );
    unsigned GCPP_PROTO PutBuffer( const unsigned char GCPP_DLL_FAR *buffer,
                                   unsigned count );
    int GCPP_PROTO PutImm( unsigned char b );
    int GCPP_PROTO ClearLink( int dir, unsigned numbytes = 0 );
    int GCPP_PROTO TranslateWindowsErrorCode( int result );
    void GCPP_PROTO ReportLinkStatus( VIEWER_OUTPUT ViewerOutput );
    virtual int GCPP_PROTO SystemIdle( void );


    DCB GCPP_DLL_FAR *CommDCB;
    int handle;
    int inhand;
    int outhand;
    int WinRts;
    int WinDtr;
    unsigned short base;
    unsigned short txqueue;
    unsigned short rxqueue;
    unsigned short breakon;
    unsigned LSerror;
    unsigned LSsignals;
    char far *event_mask;
		int notification;
    int notify_disabled;
    int rx_notify;
    int tx_notify;
    unsigned int notify_events;
    HWND hwnd;
};

#define GFCOMLINK GFComWin3

#endif



//*
//* Constants for pulse or tone dialling
//*
const int GCPP_PULSE = 9;
const int GCPP_TONE = 10;

#define LEVEL0    0
#define LEVEL1    1
#define LEVEL2    2
#define LEVEL3    3
#define LEVEL4    4
#define LEVEL5    5
#define LEVEL6    6
#define LEVEL7    7
#define LEVEL8    8
#define LEVEL9    9


//*
//* Here is the struct GFModemInfo.
//* It is used to hold all of the modem control
//* strings. A default structure is initialized by the
//* install function of class GFModem which is suitable for
//* Hayes compatible modems, but an alternative can be
//* provided to the install function if required.
//*
struct GFModemInfo
{
    char type[40];                  //* description of modem
    char prefix[4];                 //* normal command prefix
    char terminator[4];             //* normal command terminator
		char init_string[250];					//* modem initialization string
    char tone[20];                  //* tone dial code
    char pulse[20];                 //* pulse dial code
    char hangup[20];                //* hangup code
    char reset[20];                 //* reset code
    char setreg[20];                //* code to set a register
    char getreg[20];                //* code to get a register
    char auto_on[20];               //* auto answer on "" = not supported
    char auto_off[20];              //* auto answer off "" = as above
    char answer[5];                 //* answer code
    char echo_on[5];                //* echo on
    char echo_off[5];               //* echo off
    char online[5];                 //* force modem to go online
    char on_hook[5];                //* force modem to go on hook
    char off_hook[5];               //* force modem to go off hook
    char numeric_response[5];       //* put modem in numeric response mode
    char verbal_response[5];        //* put modem in verbal response mode
    char *speaker_level[5];         //* different speaker settings
    char *result_codes[10];         //* different result code settings
};

//*
//* Here is the definition for class GFModem. It is
//* intended to cover both Hayes and non-Hayes modems,
//* and should be flexible enough to cope with most types.
//* There are six private data items in this class, echo is
//* simply a flag to tell the member functions whether to
//* allow for the fact that the modem is echoing all
//* received characters back when they are examining modem
//* responses to commands. The character array cmdbuf is
//* where the modem command string is built prior to
//* sending it to the modem. The size of the array may be
//* changed to support a modem which has longer command
//* strings than a Hayes compatible.The character array
//* lastnum holds the last dial instruction. This gives
//* a redial facility which is independant of the modem.
//* The data item port is a pointer to an object of
//* class GFSerial. All input and output is directed
//* through this object. The two remaining private data
//* items are commands which is a pointer to an object of
//* struct GFModemInfo which hold the modem commands for this
//* modem, and method which simply indicates whether to
//* use pulse or tone dialling. All the class methods, bar
//* two, are public.
//*
class GCPP_CLASS_TYPE GFModem {

  public:
    GCPP_PROTO GFModem( void );
    GCPP_PROTO GFModem( GFModem GCPP_DLL_FAR &mod );
    GFModem& GCPP_PROTO operator=( GFModem GCPP_DLL_FAR &mod );
    virtual int GCPP_PROTO Install( GFSerial GCPP_DLL_FAR *sptr,
                                    GFModemInfo GCPP_DLL_FAR *cmptr = (GFModemInfo *)NULL );
    virtual int GCPP_PROTO DialMode( int mode );
    virtual int GCPP_PROTO Dial( char GCPP_DLL_FAR *number );
    virtual int GCPP_PROTO Redial( void );
    virtual int GCPP_PROTO HangUp( void );
    virtual int GCPP_PROTO Reset( void );
    virtual int GCPP_PROTO AutoOn( void );
    virtual int GCPP_PROTO AutoOff( void );
    virtual int GCPP_PROTO Write( char GCPP_DLL_FAR *string,
                                  int use_at = 1 );
    virtual int GCPP_PROTO WriteNoWait( char GCPP_DLL_FAR *string,
                                        int use_at = 1 );
    virtual int GCPP_PROTO SetRegister( int reg, int val );
    virtual char GCPP_DLL_FAR * GCPP_PROTO GetRegister( int reg );
    virtual int GCPP_PROTO Initialize( void );
    virtual int GCPP_PROTO EchoOn( void );
    virtual int GCPP_PROTO EchoOff( void );
    virtual int GCPP_PROTO AnswerPhone( void );
    virtual int GCPP_PROTO ModemOnline( void );
    virtual int GCPP_PROTO OnHook( void );
    virtual int GCPP_PROTO OffHook( void );
    virtual int GCPP_PROTO SetNumericResponse( void );
    virtual int GCPP_PROTO SetVerbalResponse( void );
    virtual int GCPP_PROTO SetSpeakerLevel( int level );
    virtual int GCPP_PROTO SetResultCodeLevel( int level );
    virtual long GCPP_PROTO GetModemInput( char GCPP_DLL_FAR *buffer,
                                           int length,
                                           long msecs );
    void GCPP_PROTO DelayChar( char dc );
    void GCPP_PROTO FixedDelay( long msecs );
    void GCPP_PROTO SetAbortKey( unsigned int key );
    void GCPP_PROTO WaitForOk( long msecs,
                               char GCPP_DLL_FAR *match = "OK" );
    virtual void GCPP_PROTO ModemPrinter( char c );
    virtual int GCPP_PROTO AbortModemFunction( void );

  protected:
    virtual int GCPP_PROTO Build( char GCPP_DLL_FAR *filling,
                                  int use_at );
    int GCPP_PROTO Printf( char GCPP_DLL_FAR *format, ... );

    long modem_delay_value;               //* Delay value after sending string
    char GCPP_DLL_FAR *modem_match_string;//* Match against this string
    unsigned int modem_abort_key;         //* The user assigned abort key
    int echo;                             //* Is the modem set for echoing
    int dchar;                            //* Char to indicate delay
    int method;                           //* Whether pulse or tone dial
		char cmdbuf[ 256 ]; 									//* Buffer where commands built
    char lastnum[ 40 ];                   //* Last no. dialed, for redial
    GFSerial GCPP_DLL_FAR *port;          //* Serial port modem is on.
    GFModemInfo GCPP_DLL_FAR *commands;   //* Modem command strings
};



//*
//* Class GFChecksum is the base class used for deriving the
//* classes which carry out integrity checking on data
//* buffers. The most significant feature is the union
//* which holds the check value. It is capable of holding
//* either an 8, 16 or 32 bit check value.
//*
class GCPP_CLASS_TYPE GFChecksum
{
  public:
    GCPP_PROTO GFChecksum( void );
    virtual GCPP_PROTO ~GFChecksum( void );
    virtual unsigned long GCPP_PROTO CalculateBlock( unsigned char GCPP_DLL_FAR *data,
                                                     unsigned n,
                                                     unsigned long seed = 0L );

  protected:
    union {
      unsigned char check8;
      unsigned short check16;
      unsigned long check32;
    };
    int size;
};

//*
//* This simple class builds on GFChecksum to provide a 2 byte
//* checksum facility.
//*
class GCPP_CLASS_TYPE GFCheck2Byte : public GFChecksum
{
  public:
    GCPP_PROTO GFCheck2Byte( void );
    GCPP_PROTO ~GFCheck2Byte( void );
    unsigned long GCPP_PROTO CalculateBlock( unsigned char GCPP_DLL_FAR *data,
                                             unsigned n,
                                             unsigned long seed = 0L );
};

//*
//* Class Crc16 is an example of a more sophisticted integrity
//* checking class. It has a new private data item table_16, and
//* a second private function build_table_16(). It also has its own
//* implementation of CalculatBlock(). It implements a 16 bit version
//* of a table driven cyclic redundancy check calculation.  And a new
//* function CalculateCharacter().
//*
#define CRC16_POLYNOMIAL 0x1021

class GCPP_CLASS_TYPE GFCrc16 : public GFChecksum
{
  public:
    GCPP_PROTO GFCrc16( void );
    GCPP_PROTO ~GFCrc16( void );
    unsigned long GCPP_PROTO CalculateBlock( unsigned char GCPP_DLL_FAR *data,
                                             unsigned count,
                                             unsigned long crc = 0 );
    unsigned long GCPP_PROTO CalculateCharacter( unsigned char c,
                                                 unsigned long crc = 0 );
	unsigned short int GCPP_PROTO CalculateCharacterModified( unsigned char c,
												 unsigned short int crc = 0 );
};

//*
//* Class GFCCITTCrc16 is a used to perform a CCITT-16 integrity
//* check.	This class has its own implemenation of CalculateBlock(),
//* along with a CalculateCharacter() function.  This class provides
//* a 16-bit table lookup method for the cyclic redundancy calculation.
//*

class GCPP_CLASS_TYPE GFCCITTCrc16 : public GFChecksum
{
  public:
	GCPP_PROTO GFCCITTCrc16( void );
	GCPP_PROTO ~GFCCITTCrc16( void );
    unsigned long GCPP_PROTO CalculateBlock( unsigned char GCPP_DLL_FAR *data,
                                             unsigned count,
                                             unsigned long crc = 0 );
	unsigned short int GCPP_PROTO CalculateCharacter( unsigned char c,
												 unsigned short int crc = 0 );
};


//*
//* Class Crc32 is an example of a more sophisticted integrity
//* checking class. It has a new private data item table_32, and
//* a second private function build_table_32(). It also has its own
//* implementation of CalculatBlock(). It implements a 32 bit version
//* of a table driven cyclic redundancy check calculation.  And a new
//* function CalculateCharacter().
//*
#define CRC32_POLYNOMIAL     0xEDB88320L

class GCPP_CLASS_TYPE GFCrc32 : public GFChecksum
{
  public:
    GCPP_PROTO GFCrc32( void );
    GCPP_PROTO ~GFCrc32( void );
    unsigned long GCPP_PROTO CalculateBlock( unsigned char GCPP_DLL_FAR *data,
                                             unsigned count,
                                             unsigned long crc = 0 );
    unsigned long GCPP_PROTO CalculateCharacter( unsigned char c,
                                                 unsigned long crc = 0 );
};

GCPP_TYPE int   GCPP_FUN UseSysTime( GFSerial GCPP_DLL_FAR *cp, long msecs );
GCPP_TYPE long  GCPP_FUN ElapsedSysTime( void );
GCPP_TYPE int   GCPP_FUN Modify8259Priority( int irq );
GCPP_TYPE int   GCPP_FUN IsMicroChannelBus( void );
GCPP_TYPE unsigned int GCPP_FUN MachineType( void );
GCPP_TYPE int   GCPP_FUN SubmodelType( void );
GCPP_TYPE unsigned int GCPP_FUN GetBiosSegment( void );
GCPP_TYPE char  GCPP_DLL_FAR *GCPP_FUN GetCommErrorName( int error );
GCPP_TYPE int   GCPP_CFUN pvtrap( char GCPP_DLL_FAR *file, int line, ... );
GCPP_TYPE int   GCPP_FUN IsWindowsEnhancedMode( void );
GCPP_TYPE int   GCPP_FUN IsDESQViewRunning( void );
GCPP_TYPE void  GCPP_FUN SysYieldWindowsTimeSlice( void );
GCPP_TYPE void  GCPP_FUN SysYieldDESQViewTimeSlice( void );
GCPP_TYPE int   GCPP_FUN GetWindowsVersion( void );

extern "C" {
int GCPP_CFUN _asinb( unsigned ioaddress );
int GCPP_CFUN _asoutb( unsigned ioaddress, unsigned value );
}

#endif
