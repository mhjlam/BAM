//
// 8track.hpp
//
// June 13, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
//	TTape reads 8TR files (eight track tapes). The 8TR itself in a
// inter-leave of upto 8 virtual files within a single 8TR physical file.
//
//	TTape can preload and cache, and is intended for streaming multiple 
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
//	TTape::ReadTrack() is analogous to fread().
//
//	TTape::PreLoadTime(milliseconds) should be called at idle.
//
//----[]-------------------------------------------------------------



#ifndef	eighttrack_hpp
#define	eighttrack_hpp

#include "types.hpp"
#include "api.hpp"
#include "soundmgr.hpp"
#include "stream.hpp"

#define	AUDIO_8_TRACK	0
#define	VIDEO_8_TRACK	1

#define	MAX_TRACKS		8
#define	SECTOR_SIZE		(16*1024)
#define	MEG_1		(1*1024*1024)
#define	MEG_3		(3*1024*1024)


const long	MAX_8TR_CACHE	= MEG_3;
const int	MAX_SECTORS		= MAX_8TR_CACHE / SECTOR_SIZE + 1;

//----[ Structures in files ]-------------------------------------------

struct	STrackHdr
{
	uint32		fullSize;
};

struct	STapeHdr
{
	uint16		numTracks;
	uint16		numSegments;
	STrackHdr	aTrackHdrs[MAX_TRACKS];
};



//----[ Run-time Structures ]-------------------------------------------

struct	STrack
{
	long				fullSize;			// size of track
	long				bytesToRead;		// count of bytes of track read from drive
	long				bytesDistributed;	// count of bytes copy thru ReadTrack()
	int				numSegments;
	unsigned char*	pSegmentArray;		// ptr to array of sector numbers, in physical file (segment) order
	bool				fIgnoreThisTrack;	// if set, don't cache this track
};

typedef	int	sector_t;


class TEightTrack : public TStreamBase
{
	public:

		virtual void		Open(res_t type, 
													uint num, 
													uint32 totalSize, 
													uint32 buffSize = STREAM_DEFAULT_buffSize,
													uint32 initSize = STREAM_DEFAULT_initSize, 
													uint32 offset = STREAM_DEFAULT_offset, 
													uint32 cycleBytes = STREAM_DEFAULT_cycleBytes);
		
		virtual void		Stream(uint fillBytes = 0);
		virtual int32		GetBytes(char *destBuffer, uint32 bufSize, void **streamBuf = NULL);
		virtual	void		Close ();

		virtual void		ZeroOffsetBufs();
		virtual void		ReInit(uint32 offset);
		
		virtual void		ClearFilePtr ();
		virtual bool		HasReachedEOF();
		virtual bool		StreamDone();
		virtual void		CloseSoundFile();

	public:
		TEightTrack();
		~TEightTrack();

		long		OpenRes (res_t, int resNum, long minBuffSize);
		long		ReadTrack (char* pBuf, int trackNum, long numBytes);

		long		PreLoadSize (long numBytes);
		long		PreLoadTime (long delayInClocks);

		int		GetNumTracks () { return numTracks; }

		// public only for tools
		long		Open (FILE* fp, long minBuffSize = MEG_1);

	protected:

		// BUGBUG: not Tigre save game compatible
		char*		pCache;								// ptr to cached sector array

		unsigned char		aSectorOwners[MAX_SECTORS];	// ptr to array of track numbers, parallel to pCache

		int		numSectors;
		int		numTracks;
		int		numSegments;
		int		segmentsToRead;

		FILE*		fpPhysFile;

		STrack	aTracks[MAX_TRACKS];

		void		IgnoreTrack(int trackNum);
		void		FreeTrackSectors (int trackNum);

		bool		CacheIsFull();
		sector_t	AllocSector (int track);
		void		FreeSector (sector_t s);

		bool		ReadNextSegment ();
		void		SetSegmentSector(int iTrack, sector_t sector);

		char*		GetSectorPtr (sector_t s);
		char*		MakePtr (int segment, int offset, int track, sector_t* pSector);
};


class T8TrackSound : public TSound
{
	public:
		virtual	TStreamBase*	GetStreamer();
//		TEightTrack*				Get8Track();

		friend class	TFlic8Track;
	protected:
		TEightTrack	streamer8Track;
};

#endif


