//*
//* _DOSISR.HPP
//*
//* DESCRIPTION:
//*         class GFDevice
//*         class GFDeviceItem
//*         class GFMutlibitList
//*         class GFUart
//*         class GFI8250
//*         class GFDigiPC
//*
//* The Greenleaf Comm++ Library
//*
//* Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
//*
//* MODIFICATIONS
//*   August 19, 1994 - Modified to support Watcom C++ 10.0.
//*

#ifndef _DOSISR_HPP
#define _DOSISR_HPP

#include <dos.h>
#include "cppcomp.h"
#include "_defs386.h"
#include "compplib.hpp"
#include "_8250.hpp"

struct GFLinkInfo;
struct GFlinestat;
class GFDataFormat;
class GFSerint;

//*
//* class GFDevice and related definitions
//*

//*
//* This is the class definition of class GFDevice.
//* Although this is an abstract class designed to
//* implement communications between the cpu and and
//* almost any device which is controlled through
//* internal byte registers, it actually contains a
//* large number of implemented functions. This is due
//* to the fact that the non-portable part of the
//* class definition has been restricted to two
//* virtual functions, read and write. These functions
//* are intended to be redefined in derived classes.
//* Notice that the constructor for class GFDevice
//* is protected. This is intended to prevent the
//* creation of an instance of this class.
//*
class GFDevice : public GFComLink
{
  friend class GFDeviceList;
  friend class GFMultibitList;

  protected:
    int numregs;
    unsigned int base;
    unsigned char *initial_reg;
    unsigned char *virtual_reg;

    GFDevice( int no_reg, unsigned short ubase );
    ~GFDevice( void );

    virtual unsigned char Set( unsigned char reg, unsigned char bit );
    virtual int Test( unsigned char reg, unsigned char bit );
    virtual unsigned char Reset( unsigned char reg, unsigned char bit );
    int Open( void );
    int Close( void );
    virtual unsigned char Read( int reg_no );
    virtual void Write( int reg_no, unsigned char value );
};

//*
//* Class GFDeviceItem is used to store settings for
//* device registers, for example parity settings in
//* a UART, together with the register bit pattern
//* that will set a parameter in the device register.
//* It is intended that GFDeviceItem be used as a
//* way of holding implementation dependant
//* information so that it is kept together in one
//* place.
//* Note that that all members of class GFDeviceItem
//* are public and there are no constructors. This
//* is so that we can use an initializer list with
//* arrays of GFDeviceItems. This is essential for the
//* efficient initialization of GFDeviceLists.
//*
class GFDeviceItem
{
  public:
    int setting;
    unsigned char bitmask;

    GFDeviceItem& operator=( GFDeviceItem &d );
    GFDeviceItem& operator()( int set, unsigned char bm );
};

//*
//* The class GFDeviceList is used to manipulate lists
//* of possible bit masks to allow the setting of data
//* formats and data lines etcetera. This is one
//* method of harnessing the features of C++ in order
//* to improve portability.
//*
class GFDeviceList
{
  public:
    GFDeviceList( GFDeviceItem *dp, int s, int offset, int wr );
    int Mask( int n, ... );
    GFDeviceItem *Search( unsigned set );
    GFDeviceItem *Search( unsigned char bit );
    virtual int Set( GFDevice *dp, unsigned p );
    virtual int Reset( GFDevice *dp, unsigned p );
    virtual int Test( GFDevice *dp, unsigned p = 0 );
    virtual int Get( GFDevice *dp, unsigned p = 0 );

  protected:
    int max;
    GFDeviceItem *data;
    int port_offset;
    int writeable;
    unsigned char allbits;
};

//*
//* The class GFMultibitList extends GFDeviceList to
//* enable its use for items such as parity settings
//* which require more than one bit to be set or
//* reset.
//*
class GFMultibitList: public GFDeviceList
{
  public:
    GFMultibitList( GFDeviceItem *dp, int s, int offset, int wr );
    int Set( GFDevice *dp, unsigned p );
    int Get( GFDevice *dp, unsigned p );
};



//*
//* class GFUart definition
//*

//*
//* The UART is represented by class GFUart. This is
//* derived from class GFDevice and inherits some of its
//* functionality from that class. There are a number
//* of extra data items and functions added which are
//* designed to represent the specifics of a UART as
//* opposed to the generalized class GFDevice. This
//* class is still not intended to be used however.
//* Its main purpose in life is to provide a platform
//* for creating classes to represent specific UART
//* devices, such as the Z80SIO or the I8250. class
//* serint is a class for setting up and using serial
//* interrupts.
//*
class GFUart: public GFDevice
{
  protected:
    GFUart( int com = 0, int no_reg = 0, unsigned short ubase = 0 );

    virtual int Test( unsigned char reg, unsigned char mask );
    virtual void ParityIs( char par );
    virtual void ClearInt( void );
    virtual int SetBaudRate( long baud );
    virtual long GetBaudRate( void );
    int LineBreak( int state );
    int LineControl( int line, int state = -1 );
    int SetDataFormat( long bd, char pr, int dl, int st );
    void GetDataFormat( long &bd, char &pr, int &dl, int &st );
    void UpdateLineStatus( GFlinestat &line );

    GFDeviceList *parities;
    GFDeviceList *stopbits;
    GFDeviceList *datalens;
    GFDeviceList *in_hands;
    GFDeviceList *out_hands;
    GFDeviceList *in_lines;
    GFDeviceList *out_lines;
    GFDeviceList *linestats;
    GFDeviceList *interrupts;
    unsigned char break_mask;
    int break_register;
};

//*
//* class GFI8250 definition
//*

//*
//* The class GFI8250 is designed to contain the hardware
//* dependant features of the MSDOS implementation. It
//* redefines certain of the functions of class Uart to take
//* account of the specifics related to the use of the INS8250
//* uart. The dlab data item is a flag to the set and get
//* functions to make sure that the state of the DLAB bit
//* is always 0 when not needed.
//* The DLAB bit is used to allow access to the baud rate
//* divisor registers only, and is always reset to 0 by
//* the library routines after it is used.
//* Note the array of GFSerint pointers. We currently
//* support up to 34 uarts under MSDOS. Limits under
//* OS/2 and Windows depend on the capability of their
//* respective device drivers.
//*
class GFI8250 : public GFUart
{
  friend class GFSerial;
	friend class GFDataFormat;
  friend class GFLineStatus;
	friend class GFSerint;

  public:
    GFI8250( int com = COM1,
             unsigned short ubase = USEDEFAULT,
             int irqno = USEDEFAULT,
             unsigned rxsize = GCPP_RXSIZE,
             unsigned txsize = GCPP_TXSIZE );
    GFI8250( GFI8250 &prev );
    ~GFI8250( void );
    int Set16550TriggerLevel( TRIGGER_LEVEL level );

  protected:
		int BlockingOn( void );
    int BlockingOff( void );
    void ParityIs( char par );
    void ClearInt( void );
    int SetBaudRate( long baud );
    long GetBaudRate( void );
    int IsIrqLine( int irq );
    void Initialize( int com );
    int Open( void );
    int Close( void );
    void GetBufferInfo( GFLinkInfo &sbc,
              int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int SetHandShake( int inhand, int outhand );
    int GetHandShake( int &inhand, int &outhand );
    int NewLinkSize( unsigned insize,
                     unsigned inroom,
                     unsigned outsize );
    int GetChar( void );
    unsigned GetBuffer( unsigned char *buffer, unsigned count );
    int PutChar( unsigned char b );
    unsigned PutString( const unsigned char* buffer );
    unsigned PutBuffer( const unsigned char* buffer, unsigned count );
    int PutImm( unsigned char b );
    void Write( int reg_no, unsigned char value );
    unsigned char Read( int reg_no );
    int ClearLink( int dir, unsigned numbytes = 0 );
    void Flush( void );
    void ReportLinkStatus( void (*ViewerOutput)(char *data) );
    virtual int SystemIdle( void );

    static unsigned char irqline[ GCPP_NUMPORTS+1 ];
    unsigned char dlab;
    int irqno;
    GFSerint *si_control;
    unsigned int rxqueue;
    unsigned int txqueue;
};

#define GFCOMLINK GFI8250

//*
//* class GFDigiPC definition
//*
class GFDigiPC : public GFI8250
{
  friend class GFSerial;
  friend class GFSerint;

  public:
    GFDigiPC( int com = COM3,
              unsigned short ubase = 0x100,
              int irqno = IRQ3,
              unsigned rxsize = GCPP_RXSIZE,
              unsigned txsize = GCPP_TXSIZE );
    static void MultiBoardSetup( int start = COM3,
                                 unsigned short str = 0x140,
                                 unsigned char c1 = 4,
                                 unsigned char c2 = 0,
                                 unsigned char c3 = 0,
                                 unsigned char c4 = 0 );

  protected:
    void Initialize( int com );

    static unsigned char card[4];
};

#endif
