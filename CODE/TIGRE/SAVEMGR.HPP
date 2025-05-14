//
// SAVEMGR.HPP
//
// June 2, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Save Manager class definition.
//
//----[]-------------------------------------------------------------

#ifndef	savemgr_hpp
#define	savemgr_hpp

#include <dos.h>

#include "tigre.hpp"
#include "savebase.hpp"

#define HIT_CANCEL	NULL

// return codes for SaveMgr functions
enum
{
	SM_NO_ERROR = 0,
	SM_FILE_OPEN_ERROR,
	SM_FILE_READ_ERROR,
	SM_FILE_WRITE_ERROR,
	SM_SAVE_NAME_TRUNC_ERROR,
	SM_NO_FILE_FOUND,
	SM_RESTORE_LEFTOVERS,
	SM_OFFSET_PROBLEM,
	SM_VERSION_ERROR
};

class SaveMgr
{
	public:
				SaveMgr(void);
				~SaveMgr(void);

		uint16	Save(uint16 saveNum, uint16 versionNum, uint16 versionSubNum,
						uint16 buildID, char *saveName);
		uint16	Restore(uint16 saveNum, uint16 versionNum, uint16 versionSubNum,
						uint16 buildID);

		uint16	GetFirstSave(char *fileName);
		uint16	GetNextSave(char *fileName);
		uint16	GetSaveInfo(char *fileName, char *saveName, uint16 maxNameSize, 
						uint16 *versionNum, uint16 *versionSubNum);

	private:
		find_t	fileInfo;
		FILE		*pSaveMgrFile;
		bool		getFirstCalled;

		bool		ExecuteAtSaveFunctions(uint16 saveState, FILE *pSave);
		uint16	ProcessFind(char *fileName, unsigned rc);
};

extern SaveMgr	saveMgr;

#endif
