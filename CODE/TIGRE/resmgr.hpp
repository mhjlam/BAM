//
// RESMGR.HPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	ResourceMgr class definition.
//
// 	The ResourceMgr manages allocating space for and loading
//		resources into memory.  It also maintains the resource list(s).
//
//----[]-------------------------------------------------------------


#ifndef	resmgr_hpp
#define	resmgr_hpp

#include <unordered_map>
#include "config.hpp"
#include "file.hpp"
#include	"manager.hpp"
#include "rect.hpp"
#include "resource.hpp"
#include "types.hpp"
#include "writeres.hpp"

#define	MAX_RES_MAPS	16

enum
{
	CFG_COMMENT,
	CFG_NUMBER,
	CFG_EXTENSION,
	CFG_PATH,
	CFG_FLAGS1,
	CFG_FLAGS2
};


// These structs are read directly from disk (MAP.IDX / *.MAP files).
// Watcom used 1-byte enums and no padding; force packed layout.
#pragma pack(push, 1)

struct ResInfo
{
	unsigned char rt;			// res_t stored as 1 byte by Watcom (values 0–24)
	uint		rn;
};

struct MapRec
{
	ResInfo	r;
	uint16	numOffs;				// number of offsets (copies of this res)
	uint32	offsTable;			// start of offsets table
};

struct MapIdxRec
{
	char		name[9];				// name of map file (w/o path or ext)
	uint16	esize;				// size of map entry in bytes
	uint16	numRecs;				// number of entries in map file
	uint16	numChunks;			// number of chunks in map file
	bool		chunksAsRes;		// chunks are stored as resources
	bool		abbrev;				// this map file should be abbreviated when loaded
	bool		patch;				// this map does not have a stuff file
};

struct ChunkInfo
{
	uint16	c;						// chunk number
	uint16	m;						// map number (in map.idx order)
	bool		global;				// chunk is global
	uint32	size;					// total size of chunk
};

#pragma pack(pop)

// max length of path in a fileInfoRec
#define	kFInfoMaxPath	79
struct FileInfo
{
	ResInfo	r;
	char		resName[kFInfoMaxPath+1];		// name of file, including extension
	char		stfName[kFInfoMaxPath+1];		// name of stuff file, if any
	uint32	offset;								// offset into stuff file, if any
};

struct ResCfgEntry
{
	char		extension[4];
	uint16	flags;
};

extern ResCfgEntry resCfgTable[];


// Resource types are defined in TIGRE.HPP

class ResourceMgr : public Manager, public Object
{
	public:
		using Manager::operator new;
		ResourceMgr(bool filebased = false);
		~ResourceMgr();

		// key: (type << 32) | number — uniquely identifies a loaded resource
		static uint64_t res_key(res_t type, uint num) {
			return ((uint64_t)(uint32_t)type << 32) | (uint32_t)num;
		}
		std::unordered_map<uint64_t, grip> loadedResources;

		uint16		oldest;			// oldest resource in the list
		uint16		newest;			// newest resource in the list
		uint16		numMaps;			// number of maps in map index file
		uint16		curMap;			// current map (used by find)
		uint16		curEntry;		// current map entry (used by find)
		bool			fSearchStuffsFirst;	// for Load()

		MapIdxRec	tblMapIdx[MAX_RES_MAPS];	// map index table
		/* FILENAME_MAX is 4096 on Linux; 16*4096 = 65536 bytes would push
		 * resourceToCopy's offset past USHRT_MAX, breaking the grip system.
		 * DOS filenames were at most ~80 chars; 256 is plenty. */
		char			tblMapPaths[MAX_RES_MAPS][256];	// full paths to stuff files
		grip			gTblMaps[MAX_RES_MAPS];		// map table (all resources for each map)
		grip			gTblChunks[MAX_RES_MAPS];	// chunk table (all chunks for each map)

		Configuration		config;

		// Resource member used to copy into resource allocations
		Resource	resourceToCopy;

		virtual	grip		Load(res_t type, uint num, grip* pgFileInfo = nullptr,
									bool fReturnErrorCode = false);
		virtual	bool		LoadChunk(uint16 chunk), LoadFromSingle(grip gFi,
									TFile *pFile, res_t type, uint num);

		virtual	bool		Exists(res_t type, uint num);
		virtual	void		Flush(grip gRes, bool delres = true);
		virtual	void		Lock(grip gRes);
		virtual	void		Unlock(grip gRes, bool force = false);
		virtual	size_t	Purge(uint16 size);
		virtual	void		PurgeAllUnlocked(void);
		virtual	grip		IsLoaded(grip gRes, bool force = false);
		virtual	grip		IsLoaded(res_t type, uint num, bool force = false);
		virtual	bool		IsCompressed(grip gRes);
		virtual	grip		CreateCel(CelArrayElement* celData, uint* resNum = nullptr);
		virtual	grip		CloneRes(res_t rType, uint resNum, uint* newResNum, res_t newRType, uint unit = 0);
		virtual	grip		CreateAnim(uint numCels, CelArrayElement* celData[], uint* resNum = nullptr);
		virtual	uchar*	GetResData(grip gRes);

		virtual	FILE*		GetCuedResFile(res_t resourceType, int resourceNum);

		bool		CopyResName(char* szFileName, res_t type, uint num, uint idx);
		bool		CopyResName(char* szFileNameAndPath, res_t type, char *szFileName, uint idx);
		virtual	FileInfo*	SearchMaps(res_t rt, uint rn, grip& g);

		void		Dump(uint16 dumpNum = 1, const char *title = nullptr, bool overWrite = false);
		void		DumpLocked();
		int		ReportUseage(res_t resType);

	protected:

		virtual	Resource*	FindOldest(uint16 start = 1);
		virtual	Resource*	FindOldestUnlocked();
		virtual	Resource*	FindNewest();
		virtual	MapRec*		FindMapEntry(uint16 map, res_t rt, uint rn);
		virtual	MapRec*		GetMapEntry(uint16 map, uint16 entry, grip& g);
};

extern ResourceMgr* pResMgr;


//----[AutoRes]----------------------------------------------------
//
//	Loads Resource, and keeps locked until destroyed

class AutoRes : public Object
{
	public:

		AutoRes();
		~AutoRes();

		virtual void	Set(res_t type, uint num);
		virtual void*	Lock();
		virtual void*	Lock(res_t type, uint num);
		virtual void	Unlock();
		void			Reset() { gRes = grip{}; }	// safe clear after ClearAllocations

 	protected:

		res_t		type;
		uint		num;
		grip		gRes;

		void		*pLastDeref;	// last dereferenced pointer (debug aid)
		grip		gLastDeref;		// grip of last dereference (debug aid)
};


#endif


/*
//----[About Resources]----------------------------------------------

The following list represents the resource types:

	Define		 Ext	Purpose
	------		 ---	--------------------------------------------
	RES_CEL		.CEL	Individual cel.  A single compressed bitmap.
	RES_ANIM	  	.ANI	Animation.  A collection of cels that may be
							animated in series.
	RES_PIC		.PIC	Picture.  A collection of bitmaps that make
							up a background picture w/priorities.
	RES_MIDI		.MID	Midi sound file.
	RES_DAC		.DAC	Digital sample sound file.
	RES_DATA		.DAT	Data file.  Programmer-defined format.
	RES_FONT		.FON	Font file.  .CEL format.
	RES_SQUIB	.SQB	Text message file.
	RES_CONV		.CNV	Conversation file.
*/

