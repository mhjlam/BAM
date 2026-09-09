//*
//* _COMLINK.HPP
//*
//* Contains:
//* 			 class GFComLink
//*
//*
//* The Greenleaf Comm++ Library
//*
//* Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
//*
//* MODIFICATIONS
//*   August 8, 1994 - Changed the GF_DLL to CPP_BUILDING_DLL to make it
//* 				   more apperant what we are doing.
//*

#ifndef _COMLINK_HPP
#define _COMLINK_HPP

#include "cppcomp.h"
#include "compplib.hpp"

struct GFLinkInfo;
struct GCPP_CLASS_TYPE GFlinestat;

#if defined( GCPP_MICROSOFT ) && defined( GCPP_BUILDING_DLL )
extern unsigned short inuse[ GCPP_NUMPORTS ];
#endif

//*
//* Here is the definition of class GFComLink. This is the base
//* class for interfacing to the communications port itself.
//* It has six protected members concerned with the size of the
//* transmit and recieve queues (if any), whether characters
//* recieved should be echoed to the output buffer or vice versa
//* and with the current status of the link.
//* All of the functions in this class are virtual. This class is
//* not intended to be used except as an abstract base class and
//* therefore a protected constructor is used.
//*
class GCPP_CLASS_TYPE GFComLink
{
  friend class GCPP_CLASS_TYPE GFSerial;
  friend class GCPP_CLASS_TYPE GFDataFormat;
  friend class GCPP_CLASS_TYPE GFLineStatus;

  public:
	virtual GCPP_PROTO ~GFComLink( void );
	virtual int GCPP_PROTO SystemIdle( void );

  protected:
	GCPP_PROTO GFComLink( void );
	virtual void GCPP_PROTO Flush( void );
	void GCPP_PROTO SetEcho( int d );
	void GCPP_PROTO SetBack( int d );
	int GCPP_PROTO GetError( void );
	int GCPP_PROTO PeekError( void );
	virtual void GCPP_PROTO Initialize( int com );
	virtual int GCPP_PROTO Open( void );
	virtual int GCPP_PROTO Close( void );
        virtual void GCPP_PROTO GetBufferInfo( GFLinkInfo GCPP_DLL_FAR &sbc,
               int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
	virtual int GCPP_PROTO NewLinkSize( unsigned insize,
										unsigned inroom,
										unsigned outsize = 0 );
	virtual int GCPP_PROTO SetHandShake( int inhand, int outhand );
	virtual int GCPP_PROTO GetHandShake( int GCPP_DLL_FAR &inhand,
										 int GCPP_DLL_FAR &outhand );
	virtual int GCPP_PROTO LineBreak( int state );
	virtual int GCPP_PROTO LineControl( int line, int state = -1 );
	virtual int GCPP_PROTO SetDataFormat( long bd,
										  char pr,
										  int dl,
										  int st );
	virtual void GCPP_PROTO GetDataFormat( long GCPP_DLL_FAR &bd,
										   char GCPP_DLL_FAR &pr,
										   int GCPP_DLL_FAR &dl,
										   int GCPP_DLL_FAR &st );
	virtual void GCPP_PROTO UpdateLineStatus( GFlinestat GCPP_DLL_FAR &line );
	virtual int GCPP_PROTO PutChar( unsigned char b );
	virtual unsigned GCPP_PROTO PutString( const unsigned char GCPP_DLL_FAR *string );
	virtual unsigned GCPP_PROTO PutBuffer( const unsigned char GCPP_DLL_FAR *buf,
										   unsigned count );
	virtual int GCPP_PROTO PutImm( unsigned char b );
	virtual int GCPP_PROTO GetChar( void );
	virtual unsigned GCPP_PROTO GetBuffer( unsigned char GCPP_DLL_FAR *buffer,
										   unsigned count );
	virtual int GCPP_PROTO ClearLink( int dir, unsigned numbytes = 0 );
	virtual void GCPP_PROTO ReportLinkStatus( VIEWER_OUTPUT ViewerOutput );
		virtual int GCPP_PROTO BlockingOn( void );
		virtual int GCPP_PROTO BlockingOff( void );

#if !( defined( GCPP_MICROSOFT ) && defined( GCPP_BUILDING_DLL ) )
  static unsigned short inuse[ GCPP_NUMPORTS ];
#endif
	int comID;
	int echo;
	int back;
	int lasterror;
};
#endif
