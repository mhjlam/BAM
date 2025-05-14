//
// STREAM.CPP
//
// April 20, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// This file contains all of the routines needed to stream
// a file.  The ResMgr is used to determine where to get the
// file (Vols or local directory).  The file is opened and 
// closed independent of the ResMgr.
//
// To start the streaming, you must first have an instance of this
// class.  Call Open() to initiate.  If you don't pass the buffSize, 
// the default will be used.  To stop streaming call Close().  
//
// If you are using GetBytes() instead of manipulating
// the buffer yourself, zero (or less than the requested amount) will
// be returned when EOF is reached. 
//
// The buffer is filled by a call to Stream().  The best place to
// do this would be in the loop for your current area.  GetBytes() will
// call Stream() if more data is needed, but you probably don't want
// to use this as the method for streaming because there will be a delay
// while the file is read.  It would be better to call Stream on a
// regular basis so that GetBytes() will cause no delay and you can 
// get data in the timely manner that is usually required when streaming
// a file.
//
// FSeek and FTell will only function correctly if you call GetBytes
// to get retrieve bytes (as oppossed to accessing the buffers directly).
// If you want FSeek and FTell to work when accessing the buffers directly,
// you could maintained totalBytesRetrieved yourself.
//
// Note.  startOffset is the offset to the first byte of data.
//        endOffset is the offset to the byte 1 past the last byte.
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "memmgr.hpp"
#include "resmgr.hpp"
#include "stream.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

TStream::TStream()
{
	// set the file pointer to NULL
	pResFile = NULL;

	gBuf = 0;
	bufStatus = TS_BUF_UNUSED;
	buffersEmpty = FALSE;
}


TStream::~TStream()
{
	CleanUp();
}


// Open the file for streaming.
//
// Enter:	type			-	resource type
//				num			-	resource number
//				totalSize	-	total size (in bytes) of the file to be streamed
//				buffSize		-	size of the memory buffer to use for streaming
//				initSize		-	size to read into the buffer when the streaming
//									starts.  This allows you to use a large buffer,
//									but not slow down the start by filling it all.
//									If this is ZERO, then the whole buffer will be
//									filled.
//				offset		-	if non-zero, move the file pointer to that point
//									before streaming starts
//				cycleBytes	-	the number of bytes to read from the file each
//									pass

void
TStream::Open(res_t type, uint num, uint32 totalSize, uint32 buffSize, uint32 initSize, uint32 offset, uint32 cycleBytes)
{
	// make sure that the resource manager is here
	ASSERT(pResMgr);	 
	// make sure that the memory manager is here
	ASSERT(pMemMgr);	 

	bufLength = buffSize;
	totalStreamSize = totalSize;

	resourceType = type;
	resourceNum = num;
	cycleReadBytes = cycleBytes;

	// ask ResMgr for FILE, cued to resource
	pResFile = pResMgr->GetCuedResFile(type, num);

	eofReached = FALSE;

	// allocate our buffer
  	// (NOTE: If you want to have your own buffer, allocate
	//  the memory and store the grip in gBuf
	//  before you call TStream::Open)

	// clear the status
	// (clear all bits except the freemem bit)
	bufStatus &= TS_BUF_FREEMEM;

	if (gBuf)
	{
		// we already have a buffer. 
		// make sure that it's big enough
		if (pMemMgr->SizeOfGrip(gBuf) < bufLength)
		{
			APanic("Preallocated Stream Buffer too small.");
		}
	}
	else 
	{
		// allocate a buffer and set the bit so that the
		// memory will be freed later.
		TRACK_MEM("TStream_Buf");
		gBuf = AMalloc(bufLength); 
		bufStatus |= TS_BUF_FREEMEM;
	}

	// set the current buffer and position
	bytesStreamed = 0;
	totalBytesRetrieved = 0;

	startOffset = endOffset = 0;
	buffersEmpty = TRUE;

	// if offset has value other than zero, move the file pointer forward
	// by that many bytes
	//
	if (offset)
	{
		fseek(pResFile, offset, SEEK_CUR);
		bytesStreamed = totalBytesRetrieved = offset;
	}

	// fill the buffers
	if (initSize == 0)
	{
		// if zero, fill the entire buffer
		initSize = bufLength;	
	}

	Stream(initSize);
}

uint32
TStream::ReadBuf(uchar *buffer, uint32 bytesToRead, uint32 cycleBytes)
{
	uint32	bytesRead = 0;
	uint32	bytesLeft;

	// detect eof here	
	// also don't do anything if we don't want any bytes

	if (!eofReached && bytesToRead) 
	{
		// read from the file

		// set the size to read.

		// only read what can be read in one cycle
		if (bytesToRead > cycleBytes)
		{
			bytesToRead = cycleBytes;
		}

		// we can't just read until EOF, because we may
		// be playing out of a stuff file.

		bytesLeft = totalStreamSize - bytesStreamed;
		if (bytesLeft < bytesToRead)
		{
			bytesToRead = bytesLeft;
		}
		
		if (bytesToRead)
		{
			bytesRead = fread(buffer, 1, bytesToRead, pResFile);
		}

		bytesStreamed += bytesRead;
		if (bytesStreamed >= totalStreamSize ||
				bytesRead < bytesToRead)
		{
			// we have either read the entire file or
			// had some sort of error.
			// set EOF
			eofReached = TRUE;
		}
	}

	return bytesRead;
}

// Stream
//
// If fillBytes is zero, then cycleBytes will be used.
// If fillBytes is non-zero, then that many bytes will be read if
// the buffer have can hold that many.
//
void
TStream::Stream(uint fillBytes)
{
	uint32	totalBytesRead = 0;
	uint32	bytesRead = 0;
	uint32	bytesToRead;
	uint32	cycleBytes;
	uchar		*buffer;

	uint32	bytesLeftInFile = totalStreamSize - bytesStreamed;

	if (StreamDone() ||
		(startOffset == endOffset && !buffersEmpty))
	{
		// we are done streaming or the buffers are full
		return;
	}

	// if a file is open, check if we need to stream
	if (pResFile && gBuf)
	{
		// get the pointer to our buffer
		buffer = ADerefAs(uchar, gBuf);

	 	if (buffersEmpty)
		{
			// since they are empty, set the offsets to be more efficient.
			startOffset = endOffset = 0;
		}

		if (fillBytes)
		{
			if (fillBytes > bufLength)
 			{
				// only read up to the size of the buffer
				cycleBytes = bufLength;
			}
			else
			{
				cycleBytes = fillBytes;
			}
		}
		else
		{
			cycleBytes = cycleReadBytes;
		}
	
		if (endOffset < startOffset)
 		{
			bytesToRead = startOffset - endOffset;

			if (bytesToRead < cycleReadBytes &&
				 bytesLeftInFile > bytesToRead) 
			{
				// let's not read until we have enough bytes needed
	 			return;
			}

			// there is an empty area between the end of start (lower than start).
			// fill this area.
			bytesRead = ReadBuf(buffer + endOffset, bytesToRead, cycleBytes);

			endOffset += bytesRead;
		}
		else
		{
			if (endOffset < bufLength)
			{
				// fill from here to the end of the buffer	
				bytesToRead = bufLength - endOffset;
				bytesRead = ReadBuf(buffer + endOffset, bytesToRead, cycleBytes);

				endOffset += bytesRead;
				cycleBytes -= bytesRead;
			}

			if (cycleBytes && !eofReached)
			{
				// we still can read some more				
				if (startOffset > 0)
				{
					// fill the beginning of the buffer 
					bytesToRead = startOffset;
					bytesRead = ReadBuf(buffer, bytesToRead, cycleBytes);

					endOffset = bytesRead;
				}
			}
		}

		if (bytesRead)
		{
		 	buffersEmpty = FALSE;
		}
	}
}


// Compress the buffers so that startOffset into our buffer
// is zero.  This is useful for streaming that requires extreme
// speed and reads a fixed amount each time (like sounds).

void
TStream::ZeroOffsetBufs()
{
	uint32	tempEndOffset;
	uchar		*buffer;
	uint32	moveBytes;

	if (startOffset == 0 && 
		 endOffset == bufLength)
	{
		// this doesn't need to be done		
		return;
	}

	// get the pointer to our buffer
	buffer = ADerefAs(uchar, gBuf);

	while (startOffset != 0)
	{
		if (endOffset < startOffset)
 		{
			// we have two chunks that wrap around

			// move the high chunk into the hole
			memmove(buffer + endOffset, buffer + startOffset, bufLength - startOffset);  
			tempEndOffset = endOffset + (bufLength - startOffset);
	
			// move the lower chunk high above the high chunk
			if (endOffset  < (bufLength - tempEndOffset))
			{
				// we can move this entire block above
				moveBytes = endOffset;			
			}
			else
			{
				// we can only move part of this block
				moveBytes = bufLength - tempEndOffset; 
			}

			memmove(buffer + tempEndOffset, buffer, moveBytes); 

			if (moveBytes == endOffset)
			{
				// we moved the whole thing
				startOffset = endOffset;
				endOffset = tempEndOffset + moveBytes;
			}
			else
			{
				// we didn't move the whole thing.  move
				// the remaining low chunk towards the bottom to leave room
				// to move the main chunk down again.

				memmove(buffer, buffer + moveBytes, endOffset - moveBytes);

				startOffset = endOffset;
				endOffset -= moveBytes;
			}
		}
		else
		{
			// this is one continuous block. move it to offset 0.
			memmove(buffer, buffer + startOffset, endOffset - startOffset);
  			endOffset -= startOffset;
			startOffset = 0;
		}
	}

	// now the chunk is at offset 0.  now fill in the rest of the buffer.
	Stream(TRUE);
}



// Try to copy the requested size of bytes from the stream
// into the passed buffer.  The returned value is the actual bytes copied.
// If not enough bytes are available (EOF reached), less than the
// requested amount will be returned.
// If destBuffer is NULL, just move past bufSize bytes.
// If streamBuf is not NULL, then don't copy the bytes, just past
// back the pointer to the area and advance the offsets as if the
// data was used.
//
int32
TStream::GetBytes(char *destBuffer, uint32 bufSize, void **streamBuf)
{
	int32		bytesRetrieved = 0;
	uint32	tempSize;
	uint32	srcBufLeft;
	uint32	destBufLeft = bufSize;
	uint16	processorFlags;
	uchar		*buffer;

	// get the pointer to our buffer
	buffer = ADerefAs(uchar, gBuf);

	#ifdef	OS_DOS
		// get the processor flags
		#pragma aux GET_PROC_FLAGS =		\
			"push		ax"						\
			"pushf"								\
			"pop		ax"						\
			"mov		processorFlags,ax"	\
			"pop		ax";
		GET_PROC_FLAGS();
	#else
		// MAC, etc
		processorFlags = INTS_ENABLED_BIT;
	#endif

	while (TRUE)
	{
		if (buffersEmpty)
		{
			// no more bytes in the buffer

			// since we are out of data, let's make our lives easier and
			// change the offsets back to 0
			startOffset = endOffset = 0;

			if (eofReached)
			{
				// we have no more bytes to read.  exit.
				break;
			}
			else
			{
				// read some more
				if (processorFlags & INTS_ENABLED_BIT)
				{
					// interrupts enabled.  read
					Stream();
				}
				else
				{
					// interrupts diabled.  don't read!
					// let's get out of here
					break;
				}
			}

			if (StreamDone())
			{
				// we tried to get more, and there was no more to get
				break;
			}
		}

		if (endOffset <= startOffset)
 		{
			// we have two chunks that wrap around
			srcBufLeft = bufLength - startOffset;
		}
		else
		{
			// we have one continguous chunk
			srcBufLeft = endOffset - startOffset;
		}

		if (srcBufLeft < destBufLeft)
		{
			// we don't have enough left in this buffer.
			// let's use everything that's left in this buffer
			// and then goto the next buffer.
			tempSize = srcBufLeft;
		}
		else
		{
			tempSize = destBufLeft;
		}

		if (streamBuf)
		{
			// we just want to return the pointer to the buffer
			*streamBuf = buffer + startOffset;
		}
		else
		{
			// only copy to the buffer if we have one 
			if (destBuffer)
			{
				memcpy(destBuffer, buffer + startOffset, tempSize);
				destBuffer += tempSize;
			}
		}
		bytesRetrieved += tempSize;
		startOffset += tempSize;
		destBufLeft -= tempSize;

		if (startOffset == endOffset)
		{
			// we have emptied the buffer
			buffersEmpty = TRUE;
		}

		if (startOffset >= bufLength &&
			startOffset != endOffset)
		{
			// we have used up the buffer on the top side.  time to wrap
			// around since we have more data.
			startOffset = 0;
		}

		if (bytesRetrieved >= bufSize)
		{
			// we're done
			break;
		}

		if (streamBuf)
		{
			// if they need a continous buffer, don't look for more
			break;
		}
	}

	totalBytesRetrieved += bytesRetrieved;
	return bytesRetrieved;
}

void
TStream::Close()
{
	CleanUp();	
}


void
TStream::CloseSoundFile()
{
	// This function is provided for compatibility between TStream and
	//	TEightTrack. In TStream it closes the file. In TEightTrack, it
	// tells the eight-track it is done with the sound but leaves the file
	//	open so it can continue	to play the FLC.
	Close();
}


void
TStream::CleanUp()
{
	if (pResFile)
	{
		// file still open.  close it.
		fclose(pResFile);
		pResFile = NULL;
	}

	// if the memory manager is here, free any allocated memory
	if (AMemMgr())
	{
		// if the streamer allocated the buffer, free it
		if (gBuf && (bufStatus & TS_BUF_FREEMEM))
		{
			AFree(gBuf);
			gBuf = 0;
			bufStatus ^= TS_BUF_FREEMEM;
		}
	}
}

// report the size of the next continuous buffer.
int32
TStream::NextChunkSize()
{
	int32	chunkSize;

	if (StreamDone())
	{
		// nothing left
		chunkSize = 0;
	}
	else
	{
		if (startOffset < endOffset)
		{
			chunkSize = endOffset - startOffset;
		}
		else
		{
			chunkSize = bufLength - startOffset;
		}
	}

	return chunkSize;
}


bool
TStream::StreamDone()
{
	bool	doneStreaming;

	if (eofReached && 
		 buffersEmpty)
	{
		doneStreaming = TRUE;		
	}
	else
	{
		doneStreaming = FALSE;
	}

	return doneStreaming;
}


// Reinit the stream.  The offset is where to fast forward
// the file to.  This routine will probably only be used
// by restore game.  Since this routine will attempt to
// close the open file, and on restore game the file pointer
// will probably be invalid, the file pointer should be NULLed
// out if using this routine with restore game.
//
void
TStream::ReInit(uint32 offset)
{
	if (pResFile)
	{
		// file still open.  close it.
		fclose(pResFile);
		pResFile = NULL;
	}

	// empty the bufs
	startOffset = endOffset = 0;
	buffersEmpty = TRUE;

	Open(resourceType, resourceNum, totalStreamSize, bufLength, offset);
}

// this is very similiar to "C" fseek.  In fact, the where values should
// be the same as fseek (SEEK_SET, SEEK_CUR, SEEK_END).
// If ERROR, a non zero value is returned
int
TStream::FSeek(int offset, int where)
{
	int		result;
	uint32	bytesDiff;

	// if a file is open, attempt to do the seek
	if (pResFile)
	{
		switch(where)
		{
//			case SEEK_SET:
//				break;

			case SEEK_CUR:
				offset += totalBytesRetrieved;
				break;

			case SEEK_END:
				offset = totalStreamSize - offset;
				break;
		}

 		if (offset < totalBytesRetrieved)
		{
			// move backwards
			result = fseek(pResFile, offset, SEEK_SET);

			// if there was no problem, finish this up
			if (result == 0)
			{
				// set the current buffer and position
				startOffset = endOffset = 0;
				bytesStreamed = totalBytesRetrieved = offset;
				buffersEmpty = TRUE;

				// fill the buffers
				Stream();
			}
		}
		else
		{
			// move forward
			bytesDiff = offset - totalBytesRetrieved;
			if (bytesDiff)
			{
				if (GetBytes(NULL, bytesDiff) == bytesDiff)
 				{
					// no problem
					result = 0;
				}
				else
				{
					// something went wrong
					result = -1;
				}
			}
		}
	}
	else
	{
		result = -1;	
	}

	return result;
}


// If ERROR, -1 is returned
int
TStream::FTell(void)
{
	int	result;

	// if a file is open, do the tell
	if (pResFile)
	{
		result = totalBytesRetrieved;		
	}
	else
	{
		result = -1;	
	}

	return result;
}

bool
TStream::HasReachedEOF()
{
	return eofReached;
}

void
TStream::ClearFilePtr()
{
	pResFile = NULL;
}


