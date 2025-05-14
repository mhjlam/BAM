//
// 8Track.cpp
//
// June 13, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
//	TEightTrack reads 8TR files (eight track tapes). The 8TR itself in a
// inter-leave of upto 8 virtual files within a single 8TR physical file.
//
//	TEightTrack can preload and cache, and is intended for streaming multiple 
// files simultaneously. For example, streaming a FLC and a WAV.
//
//	The 8TR format is: 
//		STapeHdr						// one header, S signifies "struct"
//			TrackOfNextSector		// one byte, designates which track the next sector belongs to
//			Sector					// fixed size sectors
//			...						// Byte/sector, continues to end of file.
//	
//	A Sector is 16k. Though this could change, a power of 2 is advisable.
//
// Within this file we use the term Segment, which is the same size as the
// Sector, 16k. The distinction is that segments refer to the 16k block from
// the orignal file (That is the wav, or flc or whatever). Segments therefore
// have a specific order. Segment one has the first 16k of the original file 
//	adn is followed by segment two which has the second 16k of the file, etc.
// Sectors on the other hand have on order. They are simple slices of the
// Tape's preload buffer (cache). Thus Segment may be in any sector from 
//	sector zero to sector numSectors-1.
//
//	TEightTrack::ReadTrack() is analogous to fread().
//
//	TEightTrack::PreLoadTime(ticks_t) should be called at idle.
//
//----[]-------------------------------------------------------------

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


#include "tigre.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "mem.hpp"
#include "memmgr.hpp"
#include "resmgr.hpp"
#include "8Track.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define	ALLOC	mem_malloc
#define	FREE	mem_free

#define	UNUSED_SECTOR	(-1)

//----[]-------------------------------------------------------------

TEightTrack::TEightTrack()
{
	pCache = NULL;
	numTracks = 0;
}


TEightTrack::~TEightTrack()
{
	Close();
}


//----[ Public functions ]---------------------------------------------


//---------------------------------------------------------------------
// Use resmgr to find, open and seek for resource
//	Return size of cache.
// No pre load in Open(), this is up to the user to call.

long
TEightTrack::OpenRes (res_t resourceType, int resourceNum, long minBuffSize)
{
	FILE*	pResFile;

	pResFile = pResMgr->GetCuedResFile(RES_8TRACK, resourceNum);

	// appease compiler warings
	resourceType = RES_8TRACK;
	
	return (Open(pResFile, minBuffSize));
}


//---------------------------------------------------------------------
//	Open tape with an already cued-up FILE. 
//	Alloc and preload buffers.
// Return size of cache.
// No pre load in Open(), this is up to the user to call.

long
TEightTrack::Open (FILE* fp, long minBuffSize)
{
	STapeHdr		tapeHdr;	
	int			i;
	void*			pReserve;
	long			fullSize;
	long			maxBuffSize = MAX_8TR_CACHE;
	size_t		x;
	const size_t RESERVED_SPACE = MAX_SECTORS*MAX_TRACKS*4;

	// excessive error checks
	ASSERT (minBuffSize <= maxBuffSize);
	ASSERT (fp);
				                                                                      
	fpPhysFile = fp;
	
	memset (aSectorOwners, UNUSED_SECTOR, sizeof(aSectorOwners));

	//--------------------------------------------------------
	// attempt to alloc largest buffer that we can

	minBuffSize = (minBuffSize-1) / SECTOR_SIZE + 1;
	maxBuffSize = (maxBuffSize-1) / SECTOR_SIZE + 1;
	numSectors = maxBuffSize;

	// sector numbers must fit into a uchar
	ASSERT (numSectors < 256);

	// reserve space for other buffers, just to guarantee we don't
	// alloc every last byte available.
	TRACK_MEM("");
	pReserve = ALLOC (RESERVED_SPACE);

	do
	{
		TRACK_MEM("");
		pCache = (char*) ALLOC (numSectors * SECTOR_SIZE);	

		if (pCache == NULL)
		{
			if (numSectors == minBuffSize)
			{
				// could alloc even min buff
				ASSERT ("Could not alloc TEightTrack::pCache" == NULL);
			}

			// allocation failed, presamably it was too large
			numSectors -= 8;

			// check lower limit
			if (numSectors < minBuffSize)	numSectors = minBuffSize;
		}
	} while (!pCache);

	// free reserve 
	FREE (pReserve);


	//--------------------------------------------------------
	// convert header to runtime info

	// read tape header
	x = fread (&tapeHdr, sizeof(tapeHdr), 1, fpPhysFile);
	ASSERT (x == 1);

	memset (aTracks, 0, sizeof(aTracks));
	numTracks = tapeHdr.numTracks;
	numSegments = tapeHdr.numSegments;
	segmentsToRead = numSegments;

	// Does this look like a valid 8TR?
	if ( (numTracks<0) || (numTracks>8) || (numSegments<0) || (numSegments>6400) )
	{
		APanic ("This doesn't look like an 8track!");
	}

	for (i=0; i<numTracks; i++)
	{
		fullSize = aTracks[i].fullSize = tapeHdr.aTrackHdrs[i].fullSize;
		aTracks[i].bytesToRead = fullSize;

		// alloc file segment to sector cross-reference
		aTracks[i].numSegments = (fullSize-1) / SECTOR_SIZE + 1;

		TRACK_MEM("");
		aTracks[i].pSegmentArray = (unsigned char*) ALLOC (aTracks[i].numSegments + 8); // + 8 is unessential
		ASSERT (aTracks[i].pSegmentArray);
		memset (aTracks[i].pSegmentArray, UNUSED_SECTOR, aTracks[i].numSegments + 8);

		aTracks[i].bytesDistributed = 0;
		aTracks[i].fIgnoreThisTrack = FALSE;
	}

	return numSectors * SECTOR_SIZE;
}


//---------------------------------------------------------------------
//	Close tape. Free buffers, close file.

void
TEightTrack::Close()
{
	int	i;

	mem_check();

	fclose(fpPhysFile);
	
	if (pCache)	
	{
		FREE (pCache);
		pCache = 0;
	}

	for (i=0; i<numTracks; i++)
	{
		if(aTracks[i].pSegmentArray)
		{
			FREE (aTracks[i].pSegmentArray);
		}
	}

	numSectors = 0;
	numTracks = 0;
	numSegments = 0;
	segmentsToRead = 0;
	memset (aTracks, 0, sizeof(aTracks));

	mem_check();
}


void
TEightTrack::CloseSoundFile()
{
	// This function is provided for compatibility between TStream and
	//	TEightTrack. In TStream it closes the file. In TEightTrack, it
	// tells the eight-track it is done with the sound but leaves the file
	//	open so it can continue	to play the FLC.

	IgnoreTrack(AUDIO_8_TRACK);
}

void
TEightTrack::IgnoreTrack(int trackNum)
{
	aTracks[trackNum].fIgnoreThisTrack = TRUE;
	FreeTrackSectors (trackNum);
}

//---------------------------------------------------------------------
//	Distribute (hopefully) pre-loaded data to user. Returns number
// of bytes distributed.

long
TEightTrack::ReadTrack (char* pDst, int trackNum, long size)
{
	int		curSeg;
	int		curOff;
	int		lenToCopy;
	int		lenToBound;
	sector_t	sector;
	char*		pSrc;
	STrack*	pTrack;
	long		countDistributed = 0;

	pTrack = &aTracks[trackNum];

	// don't attempt to read any more than is in the original file
	size = AMin (size, pTrack->bytesToRead);

	while (size)
	{
		// real-mode-like addressing in scrambled sectors
		// get src ptr, segment, offset
		curSeg = pTrack->bytesDistributed / SECTOR_SIZE;
		curOff = pTrack->bytesDistributed % SECTOR_SIZE;
		lenToBound = SECTOR_SIZE - curOff;

		// Note that MakePtr() may force a load from disk if idling
		// has not preloaded the needed segment.
		pSrc = MakePtr (curSeg, curOff, trackNum, &sector);

		if (pSrc == NULL)
		{
			// we were unable to get any more. 
			// we are probably trying to read from disk during an interrupt,
			// which is a no-no.
			// let's get out of here
			break;
		}
		else
		{
			// read to sector boundary, size, or bytesToRead (which ever smallest)
			lenToCopy = AMin (lenToBound, size);

			// "distribute" bytes to reader
			memcpy (pDst, pSrc, lenToCopy);

			// updates
			countDistributed += lenToCopy;
			pTrack->bytesDistributed += lenToCopy;
			curOff += lenToCopy;
			pDst += lenToCopy;
			pTrack->bytesToRead -= lenToCopy;
			size -= lenToCopy;

			ASSERT (curOff <= SECTOR_SIZE);

			if (curOff == SECTOR_SIZE)
			{
				// free sector that has been distributed
				FreeSector (sector);

				// wrap to next sector
				curSeg++;
				curOff = 0;
			}
		}
	}

	return countDistributed;
}	



//---------------------------------------------------------------------
//	Pre load data from file for a specified period of time, this should
//	be called at idle. Sectors are always read in complete 16k chunks.
//	Time is in clocks.
//	Returns number of bytes read.

long
TEightTrack::PreLoadTime (long delayInClocks)
{
	long		t;
	long		count = 0;
	long		bytesRead = 0;

	t = clock() + delayInClocks;

	while (t > clock())
	{
		bytesRead = PreLoadSize (SECTOR_SIZE);
		if (!bytesRead)
		{
			// can't read anymore, anyhow
			break;
		}

		count += bytesRead;
	}

	return count;
}



//----[ Protected functions ]---------------------------------------------

//---------------------------------------------------------------------
//	Pre load a specific amount of data from file.
//	Returns number of bytes read, which could be short is cache is full.

long
TEightTrack::PreLoadSize (long size)
{
	long		count = 0;

	while (count < size)
	{
		///////////////////////
		// BUGBUG: SLOW!
		//mem_check();
		///////////////////////

		// read in a sector
		if (!ReadNextSegment ())
		{
			// read failed, cache may be full
			return count;
		}

		count += SECTOR_SIZE;
	}

	return count;
}


//---------------------------------------------------------------------
//	verify that we can alloc a track

bool	
TEightTrack::CacheIsFull()
{
	int	i;

	for (i=0; i<numSectors; i++)
	{
		if (aSectorOwners[i] == (unsigned char) UNUSED_SECTOR)
		{
			// found unclaimed sector, claim it...
			return FALSE;
		}
	}

	// unable to allocate, cache must be full.
	return TRUE;
}


sector_t
TEightTrack::AllocSector (int track)
{
	int	i;

	// BUGBUG: hashing would work well here
	for (i=0; i<numSectors; i++)
	{
		if (aSectorOwners[i] == (unsigned char) UNUSED_SECTOR)
		{
			// found unclaimed sector, claim it...
			aSectorOwners[i] = (unsigned char) track;
			return (sector_t) i;
		}
	}

	// unable to allocate, cache must be full.
	return -1;
}



void
TEightTrack::FreeSector (sector_t s)
{
	aSectorOwners[s] = (unsigned char) UNUSED_SECTOR;
}


void
TEightTrack::FreeTrackSectors (int track)
{
	int	i;

	for (i=0; i<numSectors; i++)
	{
		if (aSectorOwners[i] == track)
		{
			FreeSector(i);
		}
	}
}

//---------------------------------------------------------------------
//	We have loaded a sector with a segment of this track. This function
//	tells the track what the sector correspends to the segment

void
TEightTrack::SetSegmentSector(int iTrack, sector_t sector)
{
	int		i;
	STrack*	pTrack = &aTracks[iTrack];

	// find first unloaded segment. (all segments are loaded sequentially
	for (i=0; i<pTrack->numSegments; i++)
	{
		if (pTrack->pSegmentArray[i] == (unsigned char) UNUSED_SECTOR)
		{
			pTrack->pSegmentArray[i] = (char) sector;
			return;
		}
	}

	//
	APanic ("Track overflow");
}



//---------------------------------------------------------------------
//	Attempt to read next 16k from 8TR. Return boolean success/failure.

bool
TEightTrack::ReadNextSegment ()
{
	char		track;
	sector_t	sector;	
	size_t	x;


	if (!segmentsToRead)
	{
		// read all there is to read
		return FALSE;
	}

	// BUGBUG, this is an inefficient way to check befor reading track byte
	if (CacheIsFull())
	{
		return FALSE;
	}

	// read byte, gives track number
	x = fread (&track, 1, 1, fpPhysFile);
	ASSERT (x == 1);

	// if this fails, we are out of synch with file
	ASSERT (track < 8);

	// update count
	segmentsToRead--;

	if (aTracks[track].fIgnoreThisTrack)
	{
		// We are ignoring this track. This can happen if
		// there is no sound driver.
		fseek (fpPhysFile, SECTOR_SIZE, SEEK_CUR);
		return TRUE;
	}

	// alloc sector, read into it
	sector = AllocSector (track);

	// CacheIsFull() should have caught this, but just to be sure
	ASSERT (sector != -1);

	SetSegmentSector(track, sector);

	// read segment
	x = fread (GetSectorPtr(sector), SECTOR_SIZE, 1, fpPhysFile);
	ASSERT (x == 1);

	// return successful
	return TRUE;
}



char*
TEightTrack::GetSectorPtr (sector_t s)
{
	return pCache + (s * SECTOR_SIZE);
}


char*
TEightTrack::MakePtr (int seg, int off, int iTrack, sector_t* pSector)
{
	unsigned char	sector = (unsigned char) UNUSED_SECTOR;

	uint16	processorFlags;

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

	do 
	{
		// get sector id from track's segment to sector cross-reference.
		sector = aTracks[iTrack].pSegmentArray[seg];

		if (sector == (unsigned char) UNUSED_SECTOR)
		{
			if (processorFlags & INTS_ENABLED_BIT)
			{
				// interrupts enabled.  read
				// sector is not cached, load another segment, and try again
				PreLoadSize (SECTOR_SIZE);
			}
			else
			{
				// interrupts diabled.  don't read!
				return NULL;
			}
		}
	} while (sector == (unsigned char) UNUSED_SECTOR);

	*pSector = (sector_t) sector;

	// calc and return physical ptr into sector
	return (pCache + (sector * SECTOR_SIZE + off));
}



//----[ TStreamBase Functions ]-------------------------------------------
//
// The functions below are provide to implement the common abstract layer
//	TStreamBase for TEightTrack. This give the sound mgr a common interface
//	to both TStreams and TEightTracks.


void
TEightTrack::Open(res_t type, 
		uint num, 
		uint32 totalSize, 
		uint32 buffSize,
		uint32 initSize,
		uint32 offset,
		uint32 cycleBytes)
{
	OpenRes (type, num, buffSize);
	PreLoadSize (initSize);

	// "seek" past initial offset
	if (offset)
	{
		TRACK_MEM("TEightTrack tmp alloc");		char*	pJunk = new char[offset];
		// BUGBUG: Assumes that "offset" is used only by AUDIO_8_TRACK!
		ReadTrack (pJunk, AUDIO_8_TRACK, offset);
		delete pJunk;
	}

	// appease the compiler
	totalSize = 0;
	cycleBytes = 0;
}		


void
TEightTrack::Stream(uint fillBytes)
{
	PreLoadSize(fillBytes+1);
}

int32
TEightTrack::GetBytes(char *pDst, uint32 bufSize, void **streamBuf)
{
	// TEight Track always copies
	ASSERT (pDst);

	// appease the compiler
	streamBuf = NULL;

	return (int32) ReadTrack (pDst, AUDIO_8_TRACK, bufSize);
}


void
TEightTrack::ZeroOffsetBufs()
{
	// not implemented by TEightTrack
}

void
TEightTrack::ReInit(uint32 offset)
{
	// not implemented by TEightTrack
	ASSERT (0 == "Not Implemented");

	// appease the compiler
	offset = 0;
}


void
TEightTrack::ClearFilePtr ()
{
	// a save game thing
	fpPhysFile = NULL;
}

bool
TEightTrack::HasReachedEOF()
{
	if (segmentsToRead)
	{
		return FALSE;
	}
	return TRUE;
}

bool
TEightTrack::StreamDone()
{
	if (aTracks[AUDIO_8_TRACK].bytesToRead == 0 &&
		 HasReachedEOF())
	{
		return TRUE;
	}
	return FALSE;
}


TStreamBase*
T8TrackSound::GetStreamer()
{
	return &streamer8Track;
};

