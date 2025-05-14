// RESMGR.CPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apimem.hpp"
#include "comp.hpp"
#include "mono.hpp"
#include "resource.hpp"
#include "resmgr.hpp"
#include	"savemgr.hpp"
#include "debug.hpp"

//#define NDEBUG

bool	ResourceMgrSave(uint16 state, FILE *fp);

Debugger		ResMgrDebug;

ResourceMgr* pResMgr = NULL;
ResCfgEntry	resCfgTable[30];	// 30 resource types for now


// Default constructor
ResourceMgr::ResourceMgr(bool ignored)
{
	MapIdxRec*	mir;
	ChunkInfo*	pChunkTbl;
	char*			pMapTbl;
	FILE*			fMapIdx;
	FILE*			fMap;
	uint16		i;
	char			mapName[20];
	char			buffer[80];
	FILE*			pResFile;
	bool			foundStuff;

	ResMgrDebug.OpenFile("RESMGR.DBG");
	ResMgrDebug.OpenWindow(0, 13, 79, 19);
	ResMgrDebug.Out("ResourceMgr::ResourceMgr()\n");

	// warnings away
	ignored = ignored;

	fSearchStuffsFirst = TRUE;

	// setup exceptional ptr to ResourceMgr
	if (!pResMgr)
	{
		pResMgr = this;
		// setup for save
		AtSave(ResourceMgrSave);
	}

	oldest = 1;
	newest = 1;
	curMap = 0;
	TRACK_MEM("resList");
	resList = (List *) new SysList(100);
	resList->resizable = TRUE;
	resList->fDuplicates = FALSE;
	//resList.Resize(100);

	// load resource config file
	config.startLine = 2;
	config.endLine = (uint16) (RES_LAST + config.startLine);
	config.numParams = CFG_PARMS_MAX;
	if (!config.Load("res_cfg.hpp"))
	{
		APanic("Can't find res_cfg.");
	}
	SetName("ResourceMgr");



	numMaps = 0;

	// Attempt to open the stuff map index
	fMapIdx = fopen("map.idx", "rb");

	if (fMapIdx)
	{
		// Yes, there are stuffs.
		
		if (numMaps >= MAX_RES_MAPS)
		{
			APanic("Too many stuff files");	
		}

		// Read each map index record into the table
		while (fread(&tblMapIdx[numMaps], sizeof(MapIdxRec), 1, fMapIdx))
		{
			mir = &tblMapIdx[numMaps];

			// find where the stuff file is, and construct a full path
			i = 0;
			foundStuff = FALSE;
			while (CopyResName(tblMapPaths[numMaps], RES_STUFF, mir->name, i))
			{
				// Open the resource file
				pResFile = fopen(tblMapPaths[numMaps], "rb");
				if (pResFile)
				{
					foundStuff = TRUE;
					fclose(pResFile);
					break;
				}
				++i;
			}

			if (foundStuff == FALSE)
			{
				// the stuff file couldn't be located.
				// let's allow for stuff files to not be around
				continue;
			}

			// allocate space for that map's entries
			TRACK_MEM("Maps");
			gTblMaps[numMaps] = ASysMalloc(mir->numRecs * mir->esize);

			if (mir->numChunks)
			{
				TRACK_MEM("Chunks");
				gTblChunks[numMaps] = ASysMalloc(mir->numChunks * sizeof(ChunkInfo));
			}
			else
			{
				gTblChunks[numMaps] = 0;
			}

			// Now open the map file
			sprintf(mapName, "%s.map", mir->name);
			fMap = fopen(mapName, "rb");
			if (!fMap)
			{
				sprintf(buffer, "Can't open map file: %s", mapName);
				APanic(buffer);
			}

			if (mir->numChunks)
			{
				// Read the chunk info, if not stored as resources
				if (!mir->chunksAsRes)
				{
					pChunkTbl = ADerefAs(ChunkInfo, gTblChunks[numMaps]);
					for (i = 0; i < mir->numChunks; i++)
					{
						fread(pChunkTbl + i, sizeof(ChunkInfo), 1, fMap);
					}
				}
			}

			// Read the resource info
			// If we're loading an abbreviated map, just load the resource info.
			// Otherwise, load the whole record
			pMapTbl = ADerefAs(char, gTblMaps[numMaps]);
			for (i = 0; i < mir->numRecs; i++)
			{
				if (mir->abbrev)
				{
					fread(pMapTbl, sizeof(ResInfo), 1, fMap);
					pMapTbl += sizeof(ResInfo);
				}
				else
				{
					fread(pMapTbl, mir->esize, 1, fMap);
					pMapTbl += mir->esize;
				}
			}
			fclose(fMap);
			++numMaps;
		}
		fclose(fMapIdx);
	}
}


// Destructor to kill the resList
ResourceMgr::~ResourceMgr()
{
	uint16		i;
	uint16		c;
	node*			pResNode;
	Resource*	dgRes;

	c = resList->count;
	for (i = 1; i <= c; i++)
	{
		pResNode = resList->GetPNode(1);
		dgRes = ADerefAs(Resource, (grip) pResNode->value);
		delete dgRes;
	}
	delete resList;

	// Get rid of the map tables
	for (i = 0; i < numMaps; i++)
	{
		AFree(gTblMaps[i]);
		if (gTblChunks[i])
		{
			AFree(gTblChunks[i]);
		}
	}
	pResMgr = NULL;

	ResMgrDebug.Out("ResourceMgr::~ResourceMgr()\n");
}


// Does this resource exist?
bool
ResourceMgr::Exists(res_t type, uint num)
{
	char		szFileName[FILENAME_MAX];
	FILE*		pResFile;
	uint16	i = 0;
	bool		ret = FALSE;
	grip		gFi;

	if (SearchMaps(type, num, gFi))
	{
		ret = TRUE;
		AFree(gFi);
	}

	// if we didn't find it in a stuff, search for resource file
	if (!ret)
	{
		while (CopyResName(szFileName, type, num, i))
		{
			// Open the resource file
			pResFile = fopen(szFileName, "rb");
			if (pResFile)
			{
				ret = TRUE;
				fclose(pResFile);
				break;
			}
			++i;
		}
	}
	
	return ret;
}


// Load a resource into memory, given a resource type and
// resource number.  Search is done in the order of the .MAP files
// indicated in the MAP.IDX file.  Also, searches are done using
// a binary search algorithm, and the .MAP files are sorted in
// order based on resource type and resource number.
//
grip
ResourceMgr::Load(res_t type, uint num, grip* pgFileInfo, bool fReturnErrorCode)
{
	char			szPanicBuffer[100];
	TFile			fInput;
	grip			gR, gFi;
	FileInfo*	pFi;
	uint16		i;
	Resource*	pRes;
	uint32		actualSize;
	MapIdxRec*	mir;
	bool			useSystemGrip = FALSE;
	grip 			gRh;
	grip 			gRes;
	int			resourceSize;
	uint			rn;
	bool			fFoundInStuff;

	// If already loaded, just set the time & we're done
	gR = IsLoaded(type, num, FALSE);
	if (gR)
	{
		pRes = ADerefAs(Resource, gR);
		pRes->time = newest++;
		return gR;
	}

	// If trying to load a dynamic and it isn't already in the
	// resList, we're in error.
	if (num & RES_DYNAMIC)
	{
		sprintf(szPanicBuffer, "Dynamic resource not found: %d", num & 0xffff);
		#ifndef NDEBUG
		ResMgrDebug.Out("Load(t%d n%u) %s\n", (int)type, num, szPanicBuffer);
		#endif
		if(fReturnErrorCode)
		{
//			pMono->Out(szPanicBuffer);
			return(NULL);
		}
		ResMgrDebug.CloseFile();
		APanic(szPanicBuffer);
	}

	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 55);
	config.CopyStr(szPanicBuffer, type, CFG_EXTENSION);
	pMono->Out("%8u.%3s", num, szPanicBuffer);
	pMono->RestoreWindow();
	#endif

	pFi = NULL;
	if(!fSearchStuffsFirst)
	{
		// this mode is to allow individual files to over-ride stuff files
		sprintf(szPanicBuffer, "FI %d:%u", (int)type, (uint)num);
		TRACK_MEM(szPanicBuffer);	gFi = AMalloc(sizeof(FileInfo));

		LoadFromSingle(gFi, &fInput, type, num);

		if(!fInput.IsOpen())
		{
			AFree(gFi);
			gFi = NULL;
			pFi = NULL;
		}
		else pFi = ADerefAs(FileInfo, gFi);
	}

	if(!pFi)
	{
		// Find the map entry.  See SearchMaps for search details.
		pFi = SearchMaps(type, num, gFi);

		if (pFi)
		{
		 	// found in stuff file
			//
			// Open the disk file (if a patch resource) or the stuff file and
			// position any pointers required
			// 
			//	WARNING: terminology here is confusing. A Patch resource is NOT
			// a single resource file. It is a creation of the MakeStuf tool.
			//	Single resource files are handled below.
			//
			fFoundInStuff = TRUE;
			mir = &tblMapIdx[curMap];
			if (mir->patch)
			{
				i = 0;
				if (!(fInput.Open(pFi->resName, "rb", FALSE)))
				{
					// resource not found in stuff, patch resource or single resource file
					sprintf(szPanicBuffer, "File not found: %s", pFi->resName);
					#ifndef NDEBUG
					ResMgrDebug.Out("Load(t%d n%u) %s\n", (int)type, num, szPanicBuffer);
					#endif
					if(fReturnErrorCode)
					{
//						pMono->Out(szPanicBuffer);
						return(NULL);
					}
					ResMgrDebug.CloseFile();
					APanic(szPanicBuffer);
				}
			}
			else
			{
				// fault-tolerance added below to make up for LANtastic's faulty
				// file sharing - Van

				int attemptsLeft = 3;
				while(!(fInput.Open(pFi->stfName, "rb", FALSE)))
				{
					if(attemptsLeft--)
					{
						sleep(1);
						continue;
					}
					else
					{
						// resource not found in stuff, patch resource or single resource file
						sprintf(szPanicBuffer, "File not found: %s", pFi->resName);
						#ifndef NDEBUG
						ResMgrDebug.Out("Load(t%d n%u) %s\n", (int)type, num, szPanicBuffer);
						#endif
						if(fReturnErrorCode)
						{
							pMono->Out(szPanicBuffer);
							return(NULL);
						}
						ResMgrDebug.CloseFile();
						APanic(szPanicBuffer);
					}
				}
				fInput.Seek(pFi->offset, SEEK_SET);
				fInput.Read(&rn, 1, sizeof(uint32));
			}
		}
		else
		{
			// not found in stuff file for stuff patch, look for
			// single resource files.

			fFoundInStuff = FALSE;

			sprintf(szPanicBuffer, "FI %d:%u", (int)type, (uint)num);
			TRACK_MEM(szPanicBuffer);	gFi = AMalloc(sizeof(FileInfo));

			LoadFromSingle(gFi, &fInput, type, num);
			pFi = ADerefAs(FileInfo, gFi);
			if(!fInput.IsOpen())
			{
				// resource not found in stuff, patch resource or single resource file
				sprintf(szPanicBuffer, "File not found: %s", pFi->resName);
				#ifndef NDEBUG
				ResMgrDebug.Out("Load(t%d n%u) %s\n", (int)type, num, szPanicBuffer);
				#endif
				AFree(gFi);
				if(fReturnErrorCode)
				{
					pMono->Out(szPanicBuffer);
					return(NULL);
				}
				ResMgrDebug.CloseFile();
				APanic(szPanicBuffer);
			}
		}
	}

	if (config.HasParam(type, "G"))
	{
		// this is a resource that is not going to be saved or
		// restored.  it uses system grips
		useSystemGrip = TRUE;
	}

	// Allocate the buffer for the resource header and verify information.
	TRACK_MEM("ResHdr buffer");
	gRh = AMalloc(rhSize);
	
	ResHeader* pRh = ADerefAs(ResHeader, gRh);
	if (!fFoundInStuff && config.HasParam(type, "H"))
	{
		// this resource is headerless.
		// setup a dummy header
		pRh->compType = CMP_NONE;
		pRh->size = pRh->uSize = fInput.Size();
		pRh->type = type;
		pRh->numHeaders = 0;
		pRh->sizeOfHeaders = 0;
		for (i = 0; i < RH_DATA_MAX; i++)
		{
			pRh->data[i] = 0;
		}
		pRh->genID = GENERATION;
	}
	else
	{
		// this resource has a header.  read it
		fInput.Read(pRh, rhSize, 1);
	}


//	// ... verify header validity
//	if (strncmp(pRh->id, "TIGR", 4))
//	{
//		sprintf(szPanicBuffer, "TIGRid: Resource is old generation: %s", pFi->resName);
//		APanic(szPanicBuffer);
//	}

	if (pRh->genID != GENERATION)
	{
		sprintf(szPanicBuffer, "genID: Resource is old generation: %s", pFi->resName);
		#ifndef NDEBUG
		ResMgrDebug.Out("Load(t%d n%u) %s\n", (int)type, num, szPanicBuffer);
		#endif
		if(fReturnErrorCode)
		{
			pMono->Out(szPanicBuffer);
			return(NULL);
		}
		ResMgrDebug.CloseFile();
		APanic(szPanicBuffer);
	}
	
	// ... verify resource types
	if (pRh->type != type)
	{
		sprintf(szPanicBuffer,
			"Resource type mismatch: %s.  Type is %d", pFi->resName, type);
		if(fReturnErrorCode)
		{
			pMono->Out(szPanicBuffer);
			return(NULL);
		}
		APanic(szPanicBuffer);
	}

	// find the total amount of space required to hold the sub headers and
	// data.
	resourceSize = pRh->sizeOfHeaders + pRh->uSize;

	// Now create a real Resource and transfer info from the header.
	sprintf(szPanicBuffer, "Res: %d.%d", num, type);
	TRACK_MEM(szPanicBuffer);

	if (useSystemGrip)
	{
		gRes = ASysMalloc(resourceSize + sizeof(Resource));
	}
	else
	{
		gRes = AMalloc(resourceSize + sizeof(Resource));
		// since this is in the save area, tell the memmgr not
		// to worry about it.
		ASetSaveStatus(gRes, FALSE);
	}

	pRes = resourceToCopy.CopyAndInit(gRes, type, num);
	uchar* pResBuf = pRes->Data();

	if (pRh->compType & CMP_TRLE)
	{
		pRes->compType |= CMP_TRLE;
		pRes->SetAttr(RA_COMPRESSED, TRUE);
	}
	if (pRh->compType & CMP_SRLE)
	{
		pRes->compType |= CMP_SRLE;
		pRes->SetAttr(RA_COMPRESSED, TRUE);
	}
	pRes->size = resourceSize;
	pRes->time = newest++;
	for (i = 0; i < RH_DATA_MAX; i++)
	{
		pRes->data[i] = pRh->data[i];
	}
	pRes->numHeaders = pRh->numHeaders;
	pRes->SetName(szPanicBuffer);

	// load the sub headers into our buffer
	if (pRh->sizeOfHeaders)
	{
		fInput.Read((char *) pResBuf, 1, pRh->sizeOfHeaders);
	}

	// decompress the resource data into our buffer
	actualSize = AFluff(&fInput, pRh->uSize, ((char *) pResBuf) + pRh->sizeOfHeaders, pRh->compType);

	fInput.Close();

	if (actualSize > pRh->uSize)
	{
		// The fluffed file was larger than the memory that we allocated.
		// We are probably very close to crashing.
		sprintf(szPanicBuffer, "Fluff too large: %s (just too damned fluffy)", pFi->resName);
		if(fReturnErrorCode)
		{
			pMono->Out(szPanicBuffer);
			return(NULL);
		}
		APanic(szPanicBuffer);
	}

	// Hook the resource into the resource list...
	resList->Add((void*) pRes->gSelf);

	// Lastly, get rid of the ResHeader buffer and take care of our grip
	// to the FileInfo structure from SearchMaps.
	AFree(gRh);
	if (pgFileInfo)
	{
		memcpy(pgFileInfo, &gFi, sizeof(grip));
	}
	else
	{
		AFree(gFi);
	}
	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 55);
	pMono->Out("            ");
	pMono->RestoreWindow();
	#endif
	return pRes->gSelf;
}

bool
ResourceMgr::LoadFromSingle(grip gFi, TFile *pFile, res_t type, uint num)
{
	int		loop1;
	FileInfo	*pFi;

	pFi = ADerefAs(FileInfo, gFi);
	loop1 = 0;
	while (CopyResName(pFi->resName, type, num, loop1))
	{
		// Open the resource file
		if (pFile->Open(pFi->resName, "rb", FALSE))
		{
			return(TRUE);
		}
		++loop1;
	}
	return(FALSE);
}

// Load a chunk from the stuff file.
bool
ResourceMgr::LoadChunk(uint16 chunk)
{
	uint16		i;
	res_t			rt;
	uint16		rn;
	ChunkInfo*	pChunk;
	grip			gChunk;
	grip			gChunkData;
	char*			pChunkData;
	uint32		chunkOffset;
	grip			gFi;
	FileInfo*	pFi;
	FILE*			fStuff;
	char			szPanicBuffer[100];
	bool			done = FALSE;
	ResHeader*	pRhChunk;
	ResHeader*	pRh;
	grip			gRh;
	grip 			gRes;
	Resource*	pRes;
	uint32		actualSize;
	bool			useSystemGrip;
	int			resourceSize;

	// Load the chunk
	gChunk = Load(RES_CHUNK, chunk, &gFi);
	pChunk = (ChunkInfo*) GetResData(gChunk);
	pFi = ADerefAs(FileInfo, gFi);

	// See if the map where the chunk was found is a patch map.
	// We can't have patched chunks.
	if (tblMapIdx[curMap].patch)
	{
		APanic("Cannot load chunk as patch");
	}

	// Since the offset gotten from the Load points to the _beginning_
	// of the chunk resource, we need to read the header to get the size
	// of the chunk resource, then skip over the actual resource itself.
	fStuff = fopen(pFi->stfName, "rb");
	if (!fStuff)
	{
		sprintf(szPanicBuffer, "Can't open file: %s", pFi->stfName);
		APanic(szPanicBuffer);
	}
	// Position to beginning of chunk resource
	fseek(fStuff, pFi->offset, SEEK_SET);
	TRACK_MEM("Res Header");
	gRh = AMalloc(rhSize);
	pRh = ADerefAs(ResHeader, gRh);
	// Read the resource number and header
	fread(&rn, 1, sizeof(uint16), fStuff);
	fread(pRh, 1, rhSize, fStuff);
	// Adjust the file pointer past the resource
	fseek(fStuff, pRh->uSize, SEEK_CUR);
	AFree(gRh);
	AFree(gFi);

	// Now read the whole chunk in
	TRACK_MEM("Chunk Header");
	gChunkData = AMalloc(pChunk->size);
	if (!gChunkData)
	{
		sprintf(szPanicBuffer, "Can't allocate enough for chunk %d", chunk);
		APanic(szPanicBuffer);
	}
	pChunkData = ADerefAs(char, gChunkData);
	fread(pChunkData, 1, pChunk->size, fStuff);
	fclose(fStuff);

	// Now zoom through the chunk and split it into the individual resources
	chunkOffset = 0;
	do
	{
		// Read the resource number
		memcpy(&rn, pChunkData + chunkOffset, sizeof(uint16));
		chunkOffset += sizeof(uint16);

		// Allocate the buffer for the resource header and verify information.
		pRhChunk = (ResHeader*) (pChunkData + chunkOffset);
		TRACK_MEM("ResHdr buffer");
		gRh = AMalloc(rhSize);
		pRh = ADerefAs(ResHeader, gRh);
		memcpy(pRh, pRhChunk, rhSize);
		rt = (res_t) pRh->type;

		// ... verify header validity
		if (pRh->genID != GENERATION)
		{
			sprintf(szPanicBuffer, "Resource is generation %d.  Should be generation %d", pRh->genID, GENERATION);
			APanic(szPanicBuffer);
		}
	
		// find the total amount of space required to hold the sub headers and
		// data.
		resourceSize = pRh->sizeOfHeaders + pRh->uSize;

		// Now create a real Resource and transfer info from the header.
		sprintf(szPanicBuffer, "Res: %d.%d", 0, rt);
		TRACK_MEM(szPanicBuffer);

		if (config.HasParam(rt, "G"))
		{
			// this is a resource that is not going to be saved or
			// restored.  it uses system grips
			useSystemGrip = TRUE;
			gRes = ASysMalloc(resourceSize + sizeof(Resource));
		}
		else
		{
			useSystemGrip = FALSE;
			gRes = AMalloc(resourceSize + sizeof(Resource));
			// since this is in the save area, tell the memmgr not
			// to worry about it.
			ASetSaveStatus(gRes, FALSE);
		}

		pRes = resourceToCopy.CopyAndInit(gRes, rt, rn);
		uchar* pResBuf = pRes->Data();

		if (pRh->compType & CMP_TRLE)
		{
			pRes->compType |= CMP_TRLE;
			pRes->SetAttr(RA_COMPRESSED, TRUE);
		}
		if (pRh->compType & CMP_SRLE)
		{
			pRes->compType |= CMP_SRLE;
			pRes->SetAttr(RA_COMPRESSED, TRUE);
		}
		pRes->size = resourceSize;
		pRes->time = newest++;
		for (i = 0; i < RH_DATA_MAX; i++)
		{
			pRes->data[i] = pRh->data[i];
		}
		pRes->numHeaders = pRh->numHeaders;
		chunkOffset += rhSize;

		// load the sub headers into our buffer
		if (pRh->sizeOfHeaders)
		{
			memcpy(pResBuf, pChunkData + chunkOffset, pRh->sizeOfHeaders);
			chunkOffset += pRh->sizeOfHeaders;
		}

		// decompress the resource data into our buffer
		if (pRh->compType == CMP_T12)
		{
			actualSize = AFluff(NULL, pRh->uSize, (char *) pResBuf + pRh->sizeOfHeaders, pRh->compType, pChunkData + chunkOffset);
			if (actualSize > pRh->uSize)
			{
				// The fluffed file was larger than the memory that we allocated.
				// We are probably very close to crashing.
				sprintf(szPanicBuffer, "Fluff too large: %s (just too damned fluffy)", pFi->resName);
				APanic(szPanicBuffer);
			}
		}
		else
		{
			memcpy(pResBuf + pRh->sizeOfHeaders, pChunkData + chunkOffset, pRh->uSize);
		}
		chunkOffset += pRh->uSize;

		// Hook the resource into the resource list...
		resList->Add((void*) pRes->gSelf);

		// Lastly, get rid of the ResHeader buffer
		AFree(gRh);

	} while (chunkOffset < pChunk->size);

	// All resources have been separated & registered with the resource
	// manager, so get rid of our original chunk buffer
	AFree(gChunkData);
	return TRUE;
}


// Utilities that need direct control over reading the resource file,
//	this function opens and seeks to the beginning of the resource
// file containing the desired resource, whether it is in a stuff
//	file or a single resource file, or an original file (flc or wav). 
//
//	This function is currently used by TStream, TEightTrack.
//
//	BUGBUG: This function is NOT used by resmgr (yet) and the code is
// some what older than ResMgr::Load. Because of this the logic
// in choosing between stuff for SRF's (and "patches") is not identical.
//	This function should be updated to match ResMgr::Load, and should be
// called by ResMgr::Load (without the flush).

FILE*	
ResourceMgr::GetCuedResFile(res_t resourceType, int resourceNum)
{
	FILE*			pResFile = NULL;
	uint			i;
	grip			gR;
	char			szPanicBuffer[40 + FILENAME_MAX];
	uint 			rn;
	bool			fFoundInStuff;
	MapIdxRec*	mir;
	grip			gFi = 0;
	FileInfo*	pFi;

	// make sure that the resource manager is here
	ASSERT(pResMgr);	 
	// make sure that the memory manager is here
	ASSERT(pMemMgr);	 

	// check to see if the file we want to stream is already loaded
	gR = IsLoaded(resourceType, resourceNum);

	if (gR)
	{
		// The entire file is already loaded.
		// Since we are here, some programmer has asked us
		// to stream this file.  Therefore, let's flush
		// this file and not waste the extra memory.
		Flush(gR);
	}

	// Find the map entry.  See SearchMaps for search details.
	pFi = pResMgr->SearchMaps(resourceType, resourceNum, gFi);

	if (pFi)
	{
	 	// found in stuff file
		//
		// Open the disk file (if a patch resource) or the stuff file and
		// position any pointers required
		// 
		//	WARNING: terminology here is confusing. A Patch resource is NOT
		// a single resource file. It is a creation of the MakeStuf tool.
		//	Single resource files are handled below.
		//
		fFoundInStuff = TRUE;

		mir = &pResMgr->tblMapIdx[pResMgr->curMap];
		if (mir->patch)
		{
			i = 0;
			pResFile = fopen(pFi->resName, "rb");
			if (!pResFile)
			{
				// resource not found in stuff, patch resource or single resource file
				sprintf(szPanicBuffer, "File not found: %s.*", pFi->resName);
				APanic(szPanicBuffer);
			}
		}
		else
		{
			pResFile = fopen(pFi->stfName, "rb");
			if (!pResFile)
			{
				// resource not found in stuff, patch resource or single resource file
				sprintf(szPanicBuffer, "File not found: %s.*", pFi->resName);
				APanic(szPanicBuffer);
			}
			fseek(pResFile, pFi->offset, SEEK_SET);
			fread(&rn, 1, sizeof(uint), pResFile);

			//just hand back the ptr -let them worry about the resource
			//header -if any

			//fseek(pResFile, rhSize, SEEK_CUR); 
		}
	}
	else
	{
		// not found in stuff file for stuff patch, look for
		// single resource files.

		fFoundInStuff = FALSE;

		// no stuff files
		TRACK_MEM("File Info");	gFi = AMalloc(sizeof(FileInfo));

		pFi = ADerefAs(FileInfo, gFi);
		i = 0;
		while (pResMgr->CopyResName(pFi->resName, resourceType, resourceNum, i))
		{
			// Open the resource file
			pResFile = fopen(pFi->resName, "rb");
			if (pResFile)
			{
				break;
			}
			++i;
		}
		if (!pResFile)
		{
			sprintf(szPanicBuffer, "File not found: %s.*", pFi->resName);
			AFree(gFi);
			APanic(szPanicBuffer);
		}
	}

	// free the file info memory if we allocated it.
	if (gFi)
	{
		AFree(gFi);
		gFi = 0;
	}

	return pResFile;
}


// Flush a resource from memory, even if locked.
void
ResourceMgr::Flush(grip gRes, bool delres)
{
	uint16		oldCount;
	uint16		newCount;
	Resource* 	dgRes;

	oldCount = resList->count;
	newCount = resList->Delete((void*) gRes);

	// if the newCount is different, then the resource was in the list
	if (oldCount != newCount)
	{
		dgRes = ADerefAs(Resource, gRes);

		#ifndef NDEBUG
		ResMgrDebug.Out("ResMgr:Flush(g%d %s r%d t%d)\n", gRes, delres?
			"DEL": "NODEL", dgRes->number, dgRes->type);
		#endif

		// If the deleted resource was the oldest, update the oldest value
		if (dgRes->time == oldest)
		{
			FindOldest();
		}

		// If the deleted resource was the newest, update the newest value
		if (dgRes->time == newest)
		{
			FindNewest();
		}

		// We may be instructed to delete the resource as well
		if (delres)
		{
			delete dgRes;
		}
	}
}


// Lock a resource
// Locks are cumulative
void
ResourceMgr::Lock(grip gRes)
{
	((Resource*) ADeref(gRes))->Lock();
}


// Unlock a resource
// If 'force' is passed and TRUE, locks will be zeroed
void
ResourceMgr::Unlock(grip gRes, bool force)
{
	((Resource*) ADeref(gRes))->Unlock(force);
}


// remove all unlocked resources
void
ResourceMgr::PurgeAllUnlocked(void)
{
	Resource*	pRes;
	size_t		total = 0;

	#ifndef NDEBUG
	ResMgrDebug.Out("ResMgr::PurgeAllUnlocked()\n");
	#endif
	pRes = FindOldestUnlocked();
	while(pRes)
	{
		total += (pRes->size / 1024);
		Flush(pRes->gSelf);
		pRes = FindOldestUnlocked();
	}
	#ifndef NDEBUG
	ResMgrDebug.Out("ResourceMgr::PurgeAllUnlocked() - %dk purged\n", total);
	#endif
}

// Purge resources starting with the oldest until 'size' K
//	of memory has been freed
// Returns number of Kbytes purged
size_t
ResourceMgr::Purge(uint16 sizeInK)
{
	size_t		total = 0;
	Resource*	pRes;
	char			string1[80];

	sprintf(string1, "ResMgr::Purge(%dk) -", sizeInK);
	pMono->Out(string1);

	while (total < sizeInK)
	{
		pRes = FindOldestUnlocked();
		if (pRes)
		{
			total += (pRes->size / 1024);
			sprintf(string1, " g%d(%dk)\n", pRes->gSelf, pRes->size / 1024);
			pMono->Out(string1);
			Flush(pRes->gSelf);
		}
		else
		{
			// no resource was found. list must be empty.  let's leave
			break;
		}
	}
	sprintf(string1, "\n%dk free'd (%dk requested)\n", total, sizeInK);
	pMono->Out(string1);
	return total;
}


// Check resource list to see if specified resource has been loaded.
// If so, return its grip.  If not, return NULL.
grip
ResourceMgr::IsLoaded(grip gRes, bool force)
{
	if (!resList->Find((void*) gRes))
	{
		if (force)
		{
			Resource* dgRes = ADerefAs(Resource, gRes);
			Load(dgRes->type, dgRes->number);
		}
		else
		{
			return FALSE;
		}
	}
	return gRes;
}


// Check resource list to see if specified resource has been loaded.
// If so, return its grip.  If not, return NULL.
grip
ResourceMgr::IsLoaded(res_t type, uint num, bool force)
{
	uint16		i;
	Resource*	dgRes;
	node*			dgResNode;

	dgResNode = (node *)pMemMgr->Deref(resList->gData);

	for (i = 1; i <= resList->count; i++, dgResNode++)
	{
		dgRes = ADerefAs(Resource, (grip) dgResNode->value);
		if (dgRes->type == type && dgRes->number == num)
		{
			return (grip) dgResNode->value;
		}
	}

	if (force)
	{
		return Load(type, num);
	}

	return FALSE;
}


// Return whether the specified resource is compressed or not
bool
ResourceMgr::IsCompressed(grip gRes)
{
	Resource* dgRes = ADerefAs(Resource, gRes);
	return (dgRes->CheckAttr(RA_COMPRESSED));
}


// Create a dynamic cel resource.  Dynamic cels are identified by a
// res_t of RES_CEL and a number equal to the new resource's gSelf.
//
grip
ResourceMgr::CreateCel(CelArrayElement* celData, uint* resNum)
{
	uint16		i;
	Resource*	pRes;
	uchar*		pResData;
	uint32		celSize;
	char			buffer[80];
	grip 			gRes;
	uint			resourceNum;

	if (!resNum)
	{
		resNum = &resourceNum;
	}

	// Create a new resource.
	sprintf(buffer, "CEL: %d,%d", celData->celHeader.orgX, celData->celHeader.orgY);
	TRACK_MEM(buffer);

	celSize = celData->celHeader.height * L2R(celData->celHeader.width);

	gRes = AMalloc(sizeof(Resource) + celSize + chSize);
	// since this is in the save area, tell the memmgr not
	// to worry about it.
	ASetSaveStatus(gRes, FALSE);

	pRes = resourceToCopy.CopyAndInit(gRes, RES_CEL, 0);
	pResData = pRes->Data();

	pRes->number = RES_DYNAMIC | pRes->gSelf;
	*resNum = pRes->number;
	for (i = 0; i < RH_DATA_MAX; i++)
	{
		pRes->data[i] = 0;
	}
	pRes->numHeaders = 1;
	pRes->SetAttr(RA_DYNAMIC, TRUE);
	pRes->size = celSize + chSize;
	pRes->time = newest++;

	// Set the offset in the cel header & copy the data passed to a
	// new allocation.
	celData->celHeader.offset = 0;
	sprintf(buffer, "Res: %d.%d", pRes->number, pRes->type);
	TRACK_MEM(buffer);

	memcpy(pResData, &celData->celHeader, chSize);
	memcpy(pResData + chSize, celData->celData, celSize);

	// Add it to the resource list and return a grip to it.
	resList->Add((void*) pRes->gSelf);
	pRes->Lock();

	#ifndef NDEBUG
	ResMgrDebug.Out("CreateCel() %u\n", (uint)pRes->gSelf);
	#endif

	return pRes->gSelf;
}


// Create a dynamic anim resource.  Dynamic anims are identified by a
// res_t of RES_ANIM and a number equal to the new resource's gSelf.
//
grip
ResourceMgr::CreateAnim(uint numCels, CelArrayElement* celData[], uint* resNum)
{
	uint16		i;
	Resource*	pRes;
	uchar*		pResData;
	uint32		celSize;
	char			buffer[80];
	grip 			gRes;
	uint32		offset;
	uint			resourceNum;

	if (!resNum)
	{
		resNum = &resourceNum;
	}

	// Create a new resource.
	sprintf(buffer, "ANIM: %d,%d",
		celData[0]->celHeader.orgX, celData[0]->celHeader.orgY);
	TRACK_MEM(buffer);

	// Calculate the total size & all the cel header offsets
	celSize = 0;
	for (i = 0; i < numCels; i++)
	{
		celData[i]->celHeader.offset = celSize;
		celSize += celData[i]->celHeader.height * L2R(celData[i]->celHeader.width);
	};

	gRes = AMalloc(sizeof(Resource) + celSize + chSize * numCels);
	// since this is in the save area, tell the memmgr not
	// to worry about it.
	ASetSaveStatus(gRes, FALSE);

	pRes = resourceToCopy.CopyAndInit(gRes, RES_ANIM, 0);
	pResData = pRes->Data();

	pRes->number = RES_DYNAMIC | pRes->gSelf;
	*resNum = pRes->number;
	for (i = 0; i < RH_DATA_MAX; i++)
	{
		pRes->data[i] = 0;
	}
	pRes->numHeaders = numCels;
	pRes->SetAttr(RA_DYNAMIC, TRUE);
	pRes->size = celSize + chSize * numCels;
	pRes->time = newest++;

	// Copy all the cel headers in
	offset = 0;
	for (i = 0; i < numCels; i++)
	{
		memcpy(pResData + offset, &celData[i]->celHeader, chSize);
		offset += chSize;
	}

	// Copy the data in to a new allocation.
	for (i = 0; i < numCels; i++)
	{
		celSize = celData[i]->celHeader.height * L2R(celData[i]->celHeader.width);
		memcpy(pResData + offset, celData[i]->celData, celSize);
		offset += celSize;
	}

	// Add it to the resource list and return a grip to it.
	resList->Add((void*) pRes->gSelf);
	pRes->Lock();
	return pRes->gSelf;
}


// Return the grip to a newly-created dynamic resource whose contents will be
// copied from another resource (dynamic or loaded).  Parameters are:
//		rType		 - type of resource to be copied
//		resNum	 - resource number of resource to be copied
//		newResNum - where resource number of new cel will be stored
//		newRType	 - type of new resource (default is same as old)
//		unit		 - unit number of resource to copy (1-relative)
//
//	Example: To clone an anim:
//
//		CloneRes(RES_ANIM, nnn, &newResNum);
//
// Example: To create a dynamic cel from the second cel of an anim:
//
//		CloneRes(RES_ANIM, nnn, &newResNum, RES_CEL, 2);
//
grip
ResourceMgr::CloneRes(res_t rType, uint resNum, uint* newResNum, res_t newRType, uint unit)
{
	Resource*	pRes;
	grip			gRes;
	uchar*		pResData;

	Resource*	pNewRes;
	grip			gNewRes;
	uchar*		pNewResData;

	uint			i;
	char			buffer[80];
	uint			numHeaders;
	uint			headerSize;
	uint32		dataSize;
	uint32		totalSize;
	uint32		allHdrSize;
	uchar*		pHdr;
	uchar*		pData;
	uint			resourceNum;

	if (!newResNum)
	{
		newResNum = &resourceNum;
	}

	// Get a grip on the resource to be copied
	gRes = pResMgr->Load(rType, resNum);
	pRes = ADerefAs(Resource, gRes);
	pResData = GetResData(gRes);

	// Figure out what kind of headers we're dealing with
	switch (rType)
	{
		case RES_ANIM:
		case RES_CEL:
			headerSize = sizeof(CelHeader);
			break;
	}

	// Calculate the total size of the new resource
	// Size will different if the resource types are different or if
	// we're picking up only one unit from the source resource.
	if (rType != newRType || unit)
	{
		switch (rType)
		{
			case RES_ANIM:
			case RES_CEL:
				pHdr = pResData + headerSize * (unit - 1);
				numHeaders = 1;
				dataSize = L2R(((CelHeader*) pHdr)->width) * ((CelHeader*) pHdr)->height;
				totalSize = dataSize + headerSize;
				allHdrSize = headerSize;
				pData = pResData + headerSize * pRes->numHeaders + ((CelHeader*) pHdr)->offset;
				break;
		}
	}
	else
	{
		totalSize = pRes->size;
		numHeaders = pRes->numHeaders;
		allHdrSize = numHeaders * headerSize;
		dataSize = totalSize - allHdrSize;
		pHdr = pResData;
		pData = pResData + allHdrSize;
	}

	// Create the buffer for the new resource
	sprintf(buffer, "Clone: %d/%d", rType, resNum);
	TRACK_MEM(buffer);
	gNewRes = AMalloc(sizeof(Resource) + totalSize);
	// Since this is in the save area, tell the memmgr not
	// to worry about it.
	ASetSaveStatus(gNewRes, FALSE);

	// Copy the existing resource object into the new one
	pNewRes = resourceToCopy.CopyAndInit(gNewRes, newRType, 0);
	pNewResData = pNewRes->Data();

	// Set the resource number & dynamic flag
	pNewRes->number = RES_DYNAMIC | pNewRes->gSelf;
	*newResNum = pNewRes->number;

	// Copy the resource information we need
	pNewRes->SetAttr(RA_DYNAMIC, TRUE);
	pNewRes->size = totalSize;
	pNewRes->time = newest++;
	for (i = 0; i < RH_DATA_MAX; i++)
	{
		pNewRes->data[i] = 0;
	}
	pNewRes->numHeaders = numHeaders;

	// Copy the resource data to the new resource
	if (unit)
	{
		memcpy(pNewResData, pHdr, headerSize);
		// We only copied one unit, so the offset now becomes 0
		((CelHeader*) pNewResData)->offset = 0;
		memcpy(pNewResData + headerSize, pData, dataSize);
	}
	else
	{
		memcpy(pNewResData, pHdr, allHdrSize);
		memcpy(pNewResData + allHdrSize, pData, dataSize);
	}

	// Add the new resource to the resource list and return a grip to it.
	resList->Add((void*) pNewRes->gSelf);
	pRes->Lock();
	return pNewRes->gSelf;
}


// Return a pointer to the specified resource's actual data
uchar*
ResourceMgr::GetResData(grip gRes)
{
	return ((ADerefAs(Resource, gRes))->Data());
}


// Build resource file name, using one path of the whole search path
// specified in the config file per invocation of this function.  'idx'
// indicates which individual path we will use on each invocation.
// Construct the filename using the num and type.  Then add the path 

bool
ResourceMgr::CopyResName(char* szFileName, res_t type, uint num, uint idx)
{
	char		ext[EXT_MAX];
	char		rawpath[255];
	char*		path;
	char*		p;
	uint16	i;
	int		pathLen;

	// get extension from config file
	config.CopyStr(ext, type, CFG_EXTENSION);
	
	// get whole path from config file & search for path we want
	config.CopyStr(rawpath, type, CFG_PATH);
	path = rawpath;
	for (i = 0; i < idx; i++)
	{
		path = strchr(path, ';');
		if (!path)
		{
			return FALSE;
		}
		++path;
	}

	// Now that we've found the right path, terminate it if there's another one
	p = strchr(path, ';');
	if (p)
	{
		*p = NULL;
	}
	
	// If the path consists of just a '$', it is a path that will be replaced
	// by the CD-ROM drive letter during the install.  Since it has not yet
	// been replaced, skip it.
	if (*path == '$')
	{
		return FALSE;
	}

	#ifdef	OS_DOS
		if (*(path + strlen(path) - 1) != ':')
		{
			strcat(path, "\\");
		}
		pathLen = sprintf(szFileName, "%s%u.%s", path, num, ext);
	#endif

	#ifdef	OS_MAC
		if (path[0] == '.')
		{
			pathLen = sprintf(szFileName, "%u.%s", num, ext);
		}
		else
		{
			pathLen = sprintf(szFileName, "%s:%u.%s", path, num, ext);
		}
	#endif

	// check that we didn't over-write memory
	if (pathLen > kFInfoMaxPath)	APanic ("Res Path is too long");

	return TRUE;
}


// Build resource file name, using one path of the whole search path
// specified in the config file per invocation of this function.  'idx'
// indicates which individual path we will use on each invocation.
// szFileName should contain the filename without extension.
// szFileNameAndPath is were the filename with extension and path will be put.

bool
ResourceMgr::CopyResName(char* szFileNameAndPath, res_t type, char *szFileName, uint idx)
{
	char		ext[EXT_MAX];
	char		rawpath[255];
	char*		path;
	char*		p;
	uint16	i;

	// get extension from config file
	config.CopyStr(ext, type, CFG_EXTENSION);

	// get whole path from config file & search for path we want
	config.CopyStr(rawpath, type, CFG_PATH);
	path = rawpath;
	for (i = 0; i < idx; i++)
	{
		path = strchr(path, ';');
		if (!path)
		{
			return FALSE;
		}
		++path;
	}

	// Now that we've found the right path, terminate it if there's another one
	p = strchr(path, ';');
	if (p)
	{
		*p = NULL;
	}
	
	// If the path consists of just a '$', it is a path that will be replaced
	// by the CD-ROM drive letter during the install.  Since it has not yet
	// been replaced, skip it.
	if (*path == '$')
	{
		return FALSE;
	}

	#ifdef	OS_DOS
		if (*(path + strlen(path) - 1) != ':')
		{
			strcat(path, "\\");
		}
		sprintf(szFileNameAndPath, "%s%s.%s", path, szFileName, ext);
	#endif

	#ifdef	OS_MAC
		if (path[0] == '.')
		{
			sprintf(szFileNameAndPath, "%s.%s", szFileName, ext);
		}
		else
		{
			sprintf(szFileNameAndPath, "%s:%s.%s", path, szFileName, ext);
		}
	#endif

	return TRUE;
}



//----[ Private Functions ]------------------------------------------

// Find the oldest resource in the list and update 'oldest'
Resource*
ResourceMgr::FindOldest(uint16 start)
{
	uint16 		retOld = 0xffff;
	uint16		i;
	Resource*	pRetRes = NULL;
	Resource*	dgRes;
	node*			dgResNode;

	if (resList->count == 0)
	{
		// no resources loaded	
		retOld = 1;
	}
	else
	{
		for (i = start; i <= resList->count; i++)
		{
			dgResNode = resList->GetPNode(i);
			dgRes = ADerefAs(Resource, (grip) dgResNode->value);
			if (dgRes->time < retOld)
			{
				retOld = dgRes->time;
				pRetRes = dgRes;
			}
		}
	}

	// We only want to set the oldest if we're searching from the beginning
	if (start == 1)
	{
		oldest = retOld;
	}

	return pRetRes;
}


// Find the oldest purgeable resource in the list, don't update 'oldest'
//	since Flush() will do that for us.
Resource*
ResourceMgr::FindOldestUnlocked()
{
	int 			retOld = INT_MAX;
	int			i;
	Resource*	pRetRes = NULL;
	Resource*	pRes;
	node*			pResNode;

	if (resList->count == 0)
	{
		// no resources loaded	
		return NULL;
	}

	for (i = 1; i <= resList->count; i++)
	{
		pResNode = resList->GetPNode(i);
		pRes = ADerefAs(Resource, (grip) pResNode->value);	
		if (pRes->time < retOld)
		{
			if (pRes->lockCount == 0)
			{
				retOld = pRes->time;
				pRetRes = pRes;
			}
		}
	}

	return pRetRes;
}


// Find the newest resource in the list and update 'newest'
Resource*
ResourceMgr::FindNewest()
{
	uint16 		retNew = 0;
	uint16		i;
	Resource*	pRetRes = NULL;
	Resource*	dgRes;
	node*			dgResNode;

	if (resList->count == 0)
	{
		// no resources loaded	
		retNew = 1;
	}
	else
	{
		for (i = 1; i <= resList->count; i++)
		{
			dgResNode = resList->GetPNode(i);
			dgRes = ADerefAs(Resource, (grip) dgResNode->value);
			if (dgRes->time > retNew)
			{
				retNew = dgRes->time;
				pRetRes = dgRes;
			}
		}
	}

	newest = retNew;
	return pRetRes;
}


// Dump reslist to text file
void
ResourceMgr::Dump(uint16 dumpNum, char *title, bool overWrite)
{
	#ifndef	NDEBUG

	FILE*	pFile;
	Resource*	pRes;
	node*			pResNode;
	unsigned		i;
	char			*dumpTitle;
	char			dumpName[30];

	sprintf(dumpName, "res%05d.dmp", dumpNum);
	if (overWrite)
	{
		// trunc any existing file
		pFile = fOpenPrefs(dumpName, "wt");
	}
	else
	{
		// append to any existing file
		pFile = fOpenPrefs(dumpName, "at");
	}

	if (!pFile)
	{
		APanic ("can't open resmgr.dmp");
	}

	if (title == NULL)
	{
		dumpTitle = "";
	}
	else
	{
		dumpTitle = title;
	}

	fprintf (pFile, "\n\nResource manager dump: %s\n", dumpTitle);

	for (i = 1; i <= resList->count; i++)
	{
		pResNode = resList->GetPNode(i);
		pRes = ADerefAs(Resource, (grip) pResNode->value);	
		fprintf (pFile, "\nRes node %s#%d type=%d num=%d lock=%d size=%d", 
				pRes->IsDynamic()? "DYNA": "STAT", (int) i, (int) pRes->type,
				(int) pRes->number, (int) pRes->lockCount, (int)pRes->size);
	}
	fprintf (pFile, "\n\nEnd dump: %d resources\n\n", resList->count);
	fclose(pFile);

	#else
	 dumpNum = dumpNum;
	 title = title;
	 overWrite = overWrite;
	#endif	// NDEBUG
}


// Dump list of locked res to debug monitor
void
ResourceMgr::DumpLocked()
{
	#ifndef	NDEBUG

	Resource*	pRes;
	node*			pResNode;
	unsigned		i;
	int			count = 0;

	pMono->Out("\nList of locked resources");

	for (i = 1; i <= resList->count; i++)
	{
		pResNode = resList->GetPNode(i);
		pRes = ADerefAs(Resource, (grip) pResNode->value);	
		if(pRes->lockCount)
		{
			count++;
			pMono->Out("\nRes node #%d type=%d num=%d lock=%d size=%d", 
				(int) i, (int) pRes->type, (int) pRes->number, (int) pRes->lockCount,
				(int)pRes->size);
		}
	}

	pMono->Out("\nCount of locked resources = %d\n", count);

	#endif	//	NDEBUG
}

// report total K in use by resources of type X
int
ResourceMgr::ReportUseage(res_t resType)
{
	#ifndef	NDEBUG

	Resource*	pRes;
	node*			pResNode;
	unsigned		i, totalK = 0;

	for (i = 1; i <= resList->count; i++)
	{
		pResNode = resList->GetPNode(i);
		pRes = ADerefAs(Resource, (grip) pResNode->value);	
		if(pRes->type == resType || resType == RES_LAST)
		{
			totalK += pRes->size + sizeof(Resource);
		}
	}
	return(totalK / 1024);
	#else
		resType = resType;
		return(0);
	#endif	//	NDEBUG
}

// Search the maps, in the prescribed order, for a resource.
// Order of precedence for searching is:
//		1. Patch maps, in map index array order
//		2. Map of most recently read stuff file
//		3. Non-patch maps, in map index array order
//
// Returns a FileInfo structure that contains information about the resource,
// or NULL if unsuccessful.
//
// WARNING!!!! g will be allocated if the resource if found.
//             The caller must free it!!!!!

FileInfo*
ResourceMgr::SearchMaps(res_t rt, uint rn, grip& g)
{
	uint16		map;
	MapIdxRec*	mir;
	MapRec*		mr;
	FileInfo*	fi;
	uint16		i;

	// can not assume that there are stuff files
	if (numMaps == 0)
	{
		g = 0;
		return NULL;
	}

	// Allocate space for the FileInfo structure
	TRACK_MEM("File Info");
	g = AMalloc(sizeof(FileInfo));
	fi = ADerefAs(FileInfo, g);

	// Patch maps first
	for (map = 0; map < numMaps; map++)
	{
		mir = &tblMapIdx[map];
		if (mir->patch)
		{
			mr = FindMapEntry(map, rt, rn);
			if (mr)
			{
				fi->r.rt = rt;
				fi->r.rn = rn;
				i = 0;
				CopyResName(fi->resName, rt, rn, i);
				strcpy(fi->stfName, "");
				fi->offset = 0;
				return fi;
			}
		}
	}

	// If no luck, try current map, unless current map is a patch map
	mir = &tblMapIdx[curMap];
	if (!mir->patch)
	{
		mr = FindMapEntry(curMap, rt, rn);
		if (mr)
		{
			fi->r.rt = rt;
			fi->r.rn = rn;
			i = 0;
			CopyResName(fi->resName, rt, rn, i);
			strcpy(fi->stfName, tblMapPaths[curMap]);
			fi->offset = mr->offsTable;
			#ifndef NDEBUG
			ResMgrDebug.Out("SearchMaps(): currMap:t%d n%u\n", (int)rt, (uint)rn);
			#endif
			return fi;
		}
	}

	// If still no luck, try the non-patch maps
	for (map = 0; map < numMaps; map++)
	{
		mir = &tblMapIdx[map];
		if (!mir->patch && map != curMap)
		{
			mr = FindMapEntry(map, rt, rn);
			if (mr)
			{
				fi->r.rt = rt;
				fi->r.rn = rn;
				i = 0;
				CopyResName(fi->resName, rt, rn, i);
				strcpy(fi->stfName, tblMapPaths[map]);
				fi->offset = mr->offsTable;
				#ifndef NDEBUG
				ResMgrDebug.Out("SearchMaps(): map %d:t%d n%u\n", map, (int)rt, (uint)rn);
				#endif
				return fi;
			}
		}
	}

	// if we are here, then we didn't find anything.
	// free the grip we allocated
	AFree(g);
	g = 0;

	return NULL;
}


// Find an entry in a map table, given the map, resource type, and
// resource number.  Uses a binary search algorithm.
MapRec*
ResourceMgr::FindMapEntry(uint16 map, res_t rt, uint rn)
{
	uint16		hi, lo, hit = -1, lasthit;
	uint32		tmphit, tmpsrch;
	MapIdxRec*	mir;
	MapRec*		mr;
	char*			pTable;
	uint16		offset = 0;
	bool			done = FALSE;

	mir = &tblMapIdx[map];
	pTable = ADerefAs(char, gTblMaps[map]);

	lo = 0;
	hi = (uint16) (mir->numRecs - 1);
	tmpsrch = rt << 16 | rn;

	while (!done)
	{
		lasthit = hit;
		hit = (uint16) ((hi - lo) / 2 + lo);

		// If our pointers haven't moved we may be done
		if (lasthit == hit)
		{
			// Our pointer hasn't moved, yet we may have missed the next higher
			// position.
			if (hit != hi - 1)
			{
				done = TRUE;
				continue;
			}
			++hit;
			lo = hi = hit;
		}

		mr = (MapRec*) (pTable + mir->esize * hit);
		tmphit = mr->r.rt << 16 | mr->r.rn;
		if (tmphit == tmpsrch)
		{
			curMap = map;
			curEntry = hit;
			return mr;
		}
		else if (tmphit > tmpsrch)
		{
			hi = hit;
		}
		else
		{
			lo = hit;
		}
	}
	return NULL;
}


// Find a map entry and return either 1) the entry in the memory-
// resident tables, or 2) the entry loaded from the map file on disk
// (if abbreviated maps).
MapRec*
ResourceMgr::GetMapEntry(uint16 map, uint16 entry, grip& g)
{
	MapIdxRec*	mir;
	char*			pTable;
	FILE*			fMap;
	char			buffer[40];
	MapRec*		mr;

	mir = &tblMapIdx[map];
	if (mir->abbrev)
	{
		sprintf(buffer, "%s.map", mir->name);
		fMap = fopen(buffer, "rb");
		if (fMap)
		{
			fseek(fMap, mir->esize * entry, SEEK_SET);
			TRACK_MEM("Map Index");
			g = AMalloc(mir->esize);
			mr = ADerefAs(MapRec, g);
			fread(mr, 1, mir->esize, fMap);
			fclose(fMap);
		}
		else
		{
			sprintf(buffer, "Cannot open file: %s.map", mir->name);
			APanic(buffer);
		}
	}
	else
	{
		pTable = ADerefAs(char, gTblMaps[map]);
		mr = (MapRec*) (pTable + mir->esize * entry);
		g = 0;
	}
	return mr;
}


//   The resource manager will restore all locked resources to
//   the exact same grip.  Non-locked resources will not
//   be restored.  So, the rule is that if you are going to
//   save the grip to a resource, you MUST lock it.  Any violation
//   of this rule will probably result in crashing on restore
//   game.  Also, this means that locked resources MUST be unlocked
//   when done.  Savable resources will be saved and restored to
//   the same grip.

bool
ResourceMgrSave(uint16 state, FILE *fp)
{
	bool		error = FALSE;
	grip		gRes;
	grip		gWork;
	Resource*	dgRes;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
			for (gRes = (grip) pResMgr->resList->FirstValue();
				gRes != NULL;
				gRes = (grip) pResMgr->resList->NextValue())
			{
				dgRes = ADerefAs(Resource, gRes);

				// only look at resources that are not using systems grips
				if (dgRes->CheckAttr(RA_SYS_GRIP) == FALSE)
				{
					// save "saveable" resources
					if (dgRes->CheckAttr(RA_SAVEABLE) ||
					 	dgRes->CheckAttr(RA_DYNAMIC))
					{
						// save type
						gWork = 1;
						if (fwrite(&gWork, 1, sizeof(grip), fp) != sizeof(grip))
						{
							// problem
							error = TRUE;
							break;
						}

						// save grip number and grip data
						if ((error = ASaveGripNum(gRes, fp)) != 0)
						{
							break;
						}
					}
					else
					{
						// save locked resource info
						if (dgRes->CheckAttr(RA_LOCKED))
						{
							// since this is a loadedable resource,
							// we only need to save the resource class, not
							// the resource data.

							// save type
							gWork = 2;
							if (fwrite(&gWork, 1, sizeof(grip), fp) != sizeof(grip))
							{
								// problem
								error = TRUE;
								break;
							}

							if (fwrite(dgRes, 1, sizeof(Resource), fp) != sizeof(Resource))
							{
								// problem
								error = TRUE;
								break;
							}
						}
					}
				}

				// dump a list of resources to reload
			}

			// terminate the grip list
			gWork = 0xffff;
			if (fwrite(&gWork, 1, sizeof(grip), fp) != sizeof(grip))
			{
				// problem
				error = TRUE;
				break;
			}

			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			// dump all existing resources (except ones with G flag)
			for (gRes = (grip) pResMgr->resList->FirstValue();
				gRes != NULL;
				gRes = (grip) pResMgr->resList->NextValue())
			{
				dgRes = ADerefAs(Resource, gRes);
				if (dgRes->CheckAttr(RA_SYS_GRIP) == FALSE)
				{
					delete dgRes;
					pResMgr->resList->Delete((void *) gRes);
				}
			}
			break;

		case DURING_RESTORE:
			// restore saved resources

			grip		gTempResource;
			Resource	*pTempResource;

			TRACK_MEM("TmpResource");	gTempResource = ASysMalloc(sizeof(Resource));
			pTempResource = ADerefAs(Resource, gTempResource);

			while (TRUE)
			{
				if (fread(&gWork, 1, sizeof(grip), fp) != sizeof(grip))
				{
					// we had a problem.  time to leave
					error = TRUE;
					break;
				}
				else
				{
					if (gWork == 0xffff)
					{
						// we are through
						break;
					}
					else
					{
						// based on the type, do the right thing

						if (gWork == 1)
						{
							// restore a saved resource
							if (ARestoreGripNum(&gWork, fp) != 0)
							{
								// problem;
								error = TRUE;
								break;
							}

							// set the time stamp
							dgRes = ADerefAs(Resource, gWork);
							dgRes->time = pResMgr->newest++;

							pResMgr->resList->Add((void *) gWork);
						}
						else
						{
							if (gWork == 2)
							{
								// load a locked resource
								if (fread(pTempResource, 1, sizeof(Resource), fp) != sizeof(Resource))
								{
									// problem
									error = TRUE;
									break;
								}

								// load the resource
								gWork = pResMgr->Load(pTempResource->type, pTempResource->number);

								// now change the grip to the old locked grip
								if (AChangeGrip(gWork, pTempResource->gSelf) == TRUE)
								{
									// problem
									error = TRUE;
									break;
								}
								dgRes = ADerefAs(Resource, pTempResource->gSelf);
								// now, we also need to fix gSelf
								dgRes->gSelf = pTempResource->gSelf;

								// remove the old grip from the list
								pResMgr->resList->Delete((void *) gWork);
								// add the new grip to the list
								pResMgr->resList->Add((void *) pTempResource->gSelf);

								// set a few things
								dgRes->lockCount = pTempResource->lockCount;
								dgRes->attr = pTempResource->attr;
							}
							else
							{
								// this is an invalid type
								error = TRUE;
								break;
							}
						}
					}
				}
			}

			AFree(gTempResource);

			// load all saved resources

			// load locked resource info and load appropriate resources

			break;

		case AFTER_RESTORE:
			break;
	}
	return error;
}


//----[AutoRes]----------------------------------------------
//
//	Loads Resource, and keeps locked until destroyed


AutoRes::AutoRes()
{
	type = (res_t) 0;
	num = 0;
	gRes = 0;

	pLastDeref = NULL;
	gLastDeref = NULL;
}


AutoRes::~AutoRes()
{
	if (gRes)
	{
		if(pMemMgr->CheckGrip(gRes) == GRIP_VALID)
		{
			Resource*	pRes = ADerefAs(Resource, gRes);
			pRes->Unlock();

			//WE CAN'T DO THIS -TO MUCH OVERHEAD IF SORTING PHRASES
         //YES, THE COMPARE FUNCTIONS CREATE A TEMP. SQB RES -SO WE CAN'T HAVE THIS ONE EITHER
         //if(!pRes->lockCount)
         //{
         //      //lets just get these prev locked res. out of the middle of mem
         //      pResMgr->Flush(gRes);
         //}
		}
		else
			pMono->Out("ERROR! ~AutoRes() gRes invalid!  r%d\n", num);

		//gRes = 0; //why clear it? -we're destructing!
	}
}


void
AutoRes::Set(res_t t, uint id)
{
	type = t;
	num = id;
}


void*
AutoRes::Lock(res_t t, uint id)
{
	Set(t, id);
	return Lock();
}


void*
AutoRes::Lock()
{
	Resource*	pRes;

	// check to see if we have something previously locked
	Unlock();

	// load and lock, will remain locked until unlock or destructor
	gRes = pResMgr->Load(type, num);
	pRes = ADerefAs(Resource, gRes);
	pRes->Lock();

	// BUGBUG - trying to catch a bug.  -Van
	gLastDeref = gRes;
	pLastDeref = (void *)pRes;

	#ifndef NDEBUG
//	ResMgrDebug.Out("AutoRes::Lock() g%d t%d n%u\n", gRes, (int)type, num);
	#endif
	return pRes->Data();
}


void
AutoRes::Unlock()
{
	#ifndef NDEBUG
//	ResMgrDebug.Out("AutoRes::Unlock() g%d\n", gRes);
	#endif
	if (gRes)
	{
		// BUGBUG - We have a problem with this grip's allocation having
		// been already de-alloc'ed.  This is a temporary kludge to
		// protect from crashing for E3.  -Van

		if(pMemMgr->CheckGrip(gRes) == GRIP_VALID)
		{		
			Resource*	pRes = ADerefAs(Resource, gRes);

			#ifndef NDEBUG
			if(gRes == gLastDeref)
			{
				if(pLastDeref != (void *)pRes)
				{
					ResMgrDebug.Out("AutoRes::Unlock() - warning!  *pRes changed!\n");
				}
			}
			#endif

			pRes->Unlock();

         //WE CAN'T DO THIS -TO MUCH OVERHEAD IF SORTING PHRASES
         //if(!pRes->lockCount)
         //{
         //      //lets just get these prev locked res. out of the middle of mem
         //      pResMgr->Flush(gRes);
         //}
			gRes = 0;
		}
		else
		{
			// our former resource has already been purged despite our
			// Lock()!  Grrrr!

			#ifdef NDEBUG
			pMono->Out("ERROR! AutoRes::Unlock() invalid grip!  g%d r%d\n", gRes, num);
			gRes = 0;
			#else
			ResMgrDebug.Out("AutoRes::Unlock() invalid grip!  g%d r%d\n", gRes, num);
			ResMgrDebug.CloseFile();
			APanic("AutoRes::Unlock() invalid gRes!\n");
			#endif
		}
	}
}
