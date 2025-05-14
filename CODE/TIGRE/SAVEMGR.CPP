//
// SAVEMGR.CPP
//
// June 2, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// The main save manager functions.
//
// Things that you should know:
//
// - Grips that are assigned to objects that are on the stack
//   will NOT be saved or restored.
//
// - The recommended way of calling Save or Restore is from the
//   the main game loop.  Also, care should be made that there
//   are no grip items on the stack.  They will not be saved or restored.
//   If you don't want those items saved or restored, then
//   you should be fine, but chances are that you're headed
//   for trouble.
//
// - This next message probably shouldn't be here, but is too
//   important to just have in the resource manager.  The
//   resource manager will restore all locked resources to
//   the exact same grip.  Non-locked resources will not
//   be restored.  So, the rule is that if you are going to
//   save the grip to a resource, you MUST lock it.  Any violation
//   of this rule will probably result in crashing on restore
//   game.  Also, this means that locked resources MUST be unlocked
//   when done.
//
// - Don't put Class members that use grips (gSelf) on the stack
//   in AtSave routines.  This can cause a grip to be used that may
//   need to be restored.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "savemgr.hpp"

SaveMgr	saveMgr;

extern uint16		atSaveCnt;	
extern atSavePtr	atSaveArray[MAX_AT_SAVES];

#ifdef OS_MAC

FILE*	 OS_GetSaveFILE();
FILE*	 OS_PutSaveFILE(char* name);

#endif

#define NDEBUG

SaveMgr::SaveMgr(void)
{
	pSaveMgrFile = NULL;

	getFirstCalled = FALSE;
}


SaveMgr::~SaveMgr(void)
{
	if (pSaveMgrFile != NULL)
	{
		// the file is open.  close it.
		fclose(pSaveMgrFile);
		pSaveMgrFile = NULL;
	}
}


uint16
SaveMgr::Save(uint16 saveNum, uint16 versionNum, uint16 versionSubNum,
	uint16 buildID, char *saveName)
{
	uint16	result = SM_NO_ERROR;
	uint16	i;
	char		fileName[FILENAME_MAX];
	size_t	nameSize;

	#ifdef NDEBUG
	// BUGBUG! temporarily block access to savegame for release vers, since
	//	it's all under reconstruction - Van
	return(result);
	#endif

#ifdef OS_MAC
	pSaveMgrFile = OS_PutSaveFILE(saveName);
	if (pSaveMgrFile == HIT_CANCEL) return(0);	//all ok, we just hit cancel.
#else
	if (AResMgr())
	{
		i = 0;
		while (pResMgr->CopyResName(fileName, RES_SAVE, saveNum, i))
		{
			// Open the resource file
			pSaveMgrFile = fopen(fileName, "w+b");
			if (pSaveMgrFile)
			{
				break;
			}
			++i;
		}
	}
	else 
	{
		// no resource manager.  construct our own file name
		sprintf(fileName, "%d.sav", saveNum);
		pSaveMgrFile = fopen(fileName, "w+b");
	}
#endif

	if (pSaveMgrFile == NULL)
	{
		// we couldn't open the file
		result = SM_FILE_OPEN_ERROR;
	}
	else
	{
		// use a while loop so that we can break out if there is an error
		while (TRUE)
		{
			// write the version number
			if (fwrite(&versionNum, 1, sizeof(uint16), pSaveMgrFile) != sizeof(uint16))
 			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			if (fwrite(&versionSubNum, 1, sizeof(uint16), pSaveMgrFile) != sizeof(uint16))
 			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			if (fwrite(&buildID, 1, sizeof(uint16), pSaveMgrFile) != sizeof(buildID))
 			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			// write the size of the save game name
			// add 1 to the size because we are going to write the NULL terminator
			nameSize = strlen(saveName) + 1;

			if (fwrite(&nameSize, 1, sizeof(size_t), pSaveMgrFile) != sizeof(size_t))
 			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			if (fwrite(saveName, 1, nameSize, pSaveMgrFile) != nameSize)
 			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			if (ExecuteAtSaveFunctions(BEFORE_SAVE, pSaveMgrFile) != FALSE) 
			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			if (ExecuteAtSaveFunctions(DURING_SAVE, pSaveMgrFile) != FALSE) 
			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}
			if (ExecuteAtSaveFunctions(AFTER_SAVE, pSaveMgrFile) != FALSE) 
			{
				// write problem				
				result = SM_FILE_WRITE_ERROR;
				break;
			}

			break;
		}
	}

	// make sure that the file is closed
	if (pSaveMgrFile)
	{
		fclose(pSaveMgrFile);
		pSaveMgrFile = NULL;
	}

	return result;
}


// Version checking should be done at a higher level.
// Restore just does what it is told to do.
 
uint16
SaveMgr::Restore(uint16 saveNum, uint16 versionNum, uint16 versionSubNum,
	uint16 buildID)
{
	uint16	result = SM_NO_ERROR;
	uint16	i;
	char		fileName[FILENAME_MAX];
	uint16	verNum, verSubNum, buildNum;
	size_t	nameSize;

#ifdef OS_MAC
	pSaveMgrFile = OS_GetSaveFILE();
	if (pSaveMgrFile == HIT_CANCEL) return(0);	//all ok, we just hit cancel.
#else
	if (AResMgr())
	{
		i = 0;
		while (pResMgr->CopyResName(fileName, RES_SAVE, saveNum, i))
		{
			// Open the resource file
			pSaveMgrFile = fopen(fileName, "rb");
			if (pSaveMgrFile)
			{
				break;
			}
			++i;
		}
	}
	else 
	{
		// no resource manager.  construct our own file name
		sprintf(fileName, "%d.sav", saveNum);
		pSaveMgrFile = fopen(fileName, "rb");
	}
#endif

	if (pSaveMgrFile == NULL)
	{
		// we couldn't open the file
		result = SM_FILE_OPEN_ERROR;
	}
	else
	{
		// use a while loop so that we can break out if there is an error
		while (TRUE)
		{
			// read the version number
			if(fread(&verNum, 1, sizeof(verNum), pSaveMgrFile) != sizeof(uint16))
			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if(fread(&verSubNum, 1, sizeof(verSubNum), pSaveMgrFile) != sizeof(uint16))
			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if(fread(&buildNum, 1, sizeof(buildNum), pSaveMgrFile) != sizeof(buildNum))
			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			#ifndef NDEBUG
			if(buildNum != buildID)
			{
				// old save-game, potentially incompatible.  Play it safe.
				result = SM_VERSION_ERROR;
				break;
			}
			#endif

//			// skip past the version num and subnum
//			// read the version number
//			if (fseek(pSaveMgrFile, sizeof(uint16) * 2, SEEK_CUR) != 0)
// 			{
//				// seek problem				
//				result = SM_FILE_READ_ERROR;
//				break;
//			}

			// read the size of the save game name

			if (fread(&nameSize, 1, sizeof(size_t), pSaveMgrFile) != sizeof(size_t))
 			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			// skip past the save game name

			if (fseek(pSaveMgrFile, nameSize, SEEK_CUR) != 0)
 			{
				// seek problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if (ExecuteAtSaveFunctions(BEFORE_RESTORE, pSaveMgrFile) != FALSE) 
			{
				// write problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if (ExecuteAtSaveFunctions(DURING_RESTORE, pSaveMgrFile) != FALSE) 
			{
				// write problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			// fix up all the grip offsets (hopefully nobody has tried to
			// use the grips yet)
			if (pMemMgr->RestoreGripOffsets() != 0)
			{
				// some problem
				result = SM_OFFSET_PROBLEM;
				break;
			}

			if (ExecuteAtSaveFunctions(AFTER_RESTORE, pSaveMgrFile) != FALSE) 
			{
				// write problem				
				result = SM_FILE_READ_ERROR;
				break;
			}


			// if were not at EOF, something went wrong.
			if (fread(&i, 1, 1, pSaveMgrFile) == 1)
 			{
				// since we were able to read a byte, we weren't at EOF
				result = SM_RESTORE_LEFTOVERS;
				break;
			}

			// we're done. 
			break;
		}
	}

	// make sure that the file is closed
	if (pSaveMgrFile)
	{
		fclose(pSaveMgrFile);
		pSaveMgrFile = NULL;
	}

	return result;
}


bool
SaveMgr::ExecuteAtSaveFunctions(uint16 saveState, FILE *pSave)
{
	int	i;
	bool	error;

	// last one added is called first...and so on.
	for (i = atSaveCnt - 1; i >= 0; i--)
	{
		error = atSaveArray[i](saveState, pSave);
		if (error)
		{
			return TRUE;
		}
	}

	// no errors
	return FALSE;
}


//--------------------------------------------------------------
//--------------------------------------------------------------
//	Functions used by Save Game interface, will vary per platform
//--------------------------------------------------------------
//--------------------------------------------------------------

#ifdef	OS_DOS

// This will find the name of the first save game and return it
// in fileName.  If there is a file, 0 will be returned.  If
// there is no file, a non-zero value will be returned.
 
uint16
SaveMgr::GetFirstSave(char *fileName)
{
	uint16	result;
	char		ext[EXT_MAX];
	char		rawPath[255];
	char*		path;
	char*		p;
	unsigned	rc;

	if (AResMgr())
	{
		// find the path for save games (we only use the first path)

		// get extension from config file
		pResMgr->config.CopyStr(ext, RES_SAVE, CFG_EXTENSION);
	
		// get whole path from config file & search for path we want
		pResMgr->config.CopyStr(rawPath, RES_SAVE, CFG_PATH);
		path = rawPath;

		// we only use the first path.  null terminate if there is more than one.
		p = strchr(rawPath, ';');
		if (p)
		{
			*p = NULL;
		}

		if (*(path + strlen(path) - 1) != ':')
		{
			strcat(path, "\\");
		}
		sprintf(fileName, "%s*.%s", path, ext);
	}
	else
	{
		// no resmgr.  just save local
		strcpy(fileName,"*.sav");
	}

	rc = _dos_findfirst(fileName, _A_NORMAL, &fileInfo);
	getFirstCalled = TRUE;

	result = ProcessFind(fileName, rc);

	return result;
}


// This will find the name of the next save game and return it
// in fileName.  If there is a file, 0 will be returned.  If
// there is no file, a non-zero value will be returned.
// NOTE: GetFirstSave MUST be called before this is called.
 
uint16
SaveMgr::GetNextSave(char *fileName)
{
	uint16	result;
	unsigned	rc;

	if (getFirstCalled == FALSE)
	{
		// they haven't call GetFirstSave.
		// return an error
		// no file.
		*fileName = NULL;
		result = SM_NO_FILE_FOUND;
	}
	else
	{
		rc = _dos_findnext(&fileInfo);
		result = ProcessFind(fileName, rc);
	}

	return result;
}


// Process the SaveMgr variable fileInfo after a call to findfirst or findnext.
// The passed fileName is the last file that was used.  The new file name
// will be appended to fileName

uint16
SaveMgr::ProcessFind(char *fileName, unsigned rc)
{
	uint16	result = SM_NO_ERROR;
	char		*p;

	if (rc)
	{
		// if return code (rc) is non-zero, no file was found
		*fileName = NULL;
		result = SM_NO_FILE_FOUND;
	}
	else
	{
		// find the place to append the file name onto
		p = strrchr(fileName, '\\');
		if (p != NULL)
		{
			// we found something.
			// move past the directory char
			p++;
		}
		else
		{
			// we didn't find anything.
			// start at the beginning
			p = fileName;
		}

		// null terminate so that we can append the file name
		*p = NULL;

		strcat(p, fileInfo.name);
	}

	return result;
}


// Enter this routine with the file name of the save game
// file to be examined.
//
// saveName       - the array to place the save game name.
// maxNameSize    - the size of the saveName array.  This is to prevent overrun.
// *versionNum    - the version number will be placed in this.
// *versionSubNum - the sub version number will be placed in this.

uint16
SaveMgr::GetSaveInfo(char *fileName, char *saveName, uint16 maxNameSize, 
				uint16 *versionNum, uint16 *versionSubNum)
{
	FILE	*pFile = NULL;
	uint16	result = SM_NO_ERROR;
	size_t	nameSize;
		
	pFile = fopen(fileName, "rb");

	if (pFile == NULL)
	{
		// we couldn't open the file
		result = SM_FILE_OPEN_ERROR;
	}
	else
	{
		// use a while loop so that we can break out if there is an error
		while (TRUE)
		{
			// read the version number
			if (fread(versionNum, 1, sizeof(uint16), pFile) != sizeof(uint16))
 			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if (fread(versionSubNum, 1, sizeof(uint16), pFile) != sizeof(uint16))
 			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

		 	// skip past build ID
			if(fseek(pFile, sizeof(uint16), SEEK_CUR) != 0)
 			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			// get the size of the save game name

			if (fread(&nameSize, 1, sizeof(size_t), pFile) != sizeof(size_t))
 			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if (nameSize > maxNameSize)
			{
				// the save name is larger than the buffer they passed.
				// trunc it.  This error is not fatal, so set the error code
				// and move on.
				result = SM_SAVE_NAME_TRUNC_ERROR;
				nameSize = maxNameSize;
			}

			// read the save game name
			if (fread(saveName, 1, nameSize, pFile) != nameSize)
 			{
				// read problem				
				result = SM_FILE_READ_ERROR;
				break;
			}

			if (result == SM_SAVE_NAME_TRUNC_ERROR) 
			{
				// this name is being truncated.  null terminate
				saveName[nameSize - 1] = 0;
			}

			break;
		}
		
	}

	// if the file is open, close it
	if (pFile != NULL)
	{
		fclose(pFile);
	}

	return result;
}

#endif	// OS_DOS


#ifdef	OS_MAC

uint16
SaveMgr::GetFirstSave(char *fileName)
{
	APanic("Not currently implemented on the Mac");
	return 0;
}

uint16
SaveMgr::GetNextSave(char *fileName)
{
	APanic("Not currently implemented on the Mac");
	return 0;
}

uint16
SaveMgr::ProcessFind(char *fileName)
{
	APanic("Not currently implemented on the Mac");
	return 0;
}


// Enter this routine with the file name of the save game
// file to be examined.
//
// saveName       - the array to place the save game name.
// maxNameSize    - the size of the saveName array.  This is to prevent overrun.
// *versionNum    - the version number will be placed in this.
// *versionSubNum - the sub version number will be placed in this.

uint16
SaveMgr::GetSaveInfo(char *fileName, char *saveName, uint16 maxNameSize, 
				uint16 *versionNum, uint16 *versionSubNum)
{
	APanic("Not currently implemented on the Mac");
	return 0;
}

#endif	// OS_MAC
