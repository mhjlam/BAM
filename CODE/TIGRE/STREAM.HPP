//
// STREAM.HPP
//
// April 20, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	TStream class definition.
//
// TStream is used to stream files.
//
//----[]-------------------------------------------------------------


#ifndef	stream_hpp
#define	stream_hpp

#include "types.hpp"
//#include "resmgr.hpp"

#define	TSTREAM_BUFS	2

// buffer status defines
#define	TS_BUF_UNUSED		0x00
#define	TS_BUF_FREEMEM		0x04


// WARNING: virtual functions with default paramters are a dangerous
// thing. Therefore, we try to insure that all descendents use the
// same default values
const uint32	STREAM_DEFAULT_buffSize = 4096;
const uint32	STREAM_DEFAULT_initSize = 0;
const uint32	STREAM_DEFAULT_offset = 0;
const uint32	STREAM_DEFAULT_cycleBytes = (16 * 1024);
		

#define	INTS_ENABLED_BIT	0x200


//----[TStreamBase]--------------------------------------------------------
//
// This class provides a common abstraction level for different kinds
// of streamers. Currenty is supports TStream and TEightTrack.

class TStreamBase
{
	public:

								TStreamBase() { }
		virtual				~TStreamBase() { }

		virtual void		Open(res_t type, 
													uint num, 
													uint32 totalSize, 
													uint32 buffSize = STREAM_DEFAULT_buffSize,
													uint32 initSize = STREAM_DEFAULT_initSize, 
													uint32 offset = STREAM_DEFAULT_offset, 
													uint32 cycleBytes = STREAM_DEFAULT_cycleBytes) = 0;
		
		virtual void		Stream(uint fillBytes = 0) = 0;
		virtual int32		GetBytes(char *destBuffer, uint32 bufSize, void **streamBuf = NULL) = 0;
		virtual void		Close() = 0;

		virtual void		ZeroOffsetBufs() = 0;
		virtual void		ReInit(uint32 offset) = 0;

		virtual void		ClearFilePtr () = 0;
		virtual bool		HasReachedEOF() = 0;
		virtual bool		StreamDone() = 0;
		virtual void		CloseSoundFile() = 0;
};



//----[TStream]--------------------------------------------------------

class TStream : public TStreamBase
{
	public:

		TStream();
		~TStream();

		virtual	void		Open(res_t type, 
													uint num, 
													uint32 totalSize, 
													uint32 buffSize = STREAM_DEFAULT_buffSize,
													uint32 initSize = STREAM_DEFAULT_initSize, 
													uint32 offset = STREAM_DEFAULT_offset, 
													uint32 cycleBytes = STREAM_DEFAULT_cycleBytes);
		
		virtual void		Stream(uint fillBytes = 0);
		virtual uint32		ReadBuf(uchar *buffer, uint32 bytesToRead, uint32 cycleBytes);
		virtual int32		GetBytes(char *destBuffer, uint32 bufSize, void **streamBuf = NULL);
		virtual int32		NextChunkSize();
		virtual void		Close();
		virtual void		CloseSoundFile();

		virtual void		ZeroOffsetBufs();
		virtual void		ReInit(uint32 offset);
		virtual void		ClearFilePtr ();
		virtual bool		HasReachedEOF();

		bool					StreamDone();
		int					FSeek(int offset, int where);
		int					FTell(void);

	private:

		void		CleanUp();

		FILE*		pResFile;
		bool		eofReached;

		uint32	bufLength;
		uint32	totalStreamSize;
		uint32	bytesStreamed;
		int		totalBytesRetrieved;

		bool		buffersEmpty;

		grip		gBuf;
		uint32	bufStatus;

		uint32	startOffset;
		uint32	endOffset;
		uint32	cycleReadBytes;

		res_t		resourceType;
		uint		resourceNum;
};



#endif
