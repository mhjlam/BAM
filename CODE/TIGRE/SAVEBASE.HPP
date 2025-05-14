//
// SAVEBASE.CPP
//
// June 2, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// The AtSavefunction.
//
//----[]-------------------------------------------------------------

#ifndef	savebase_hpp
#define	savebase_hpp

#include "types.hpp"

// we have a limit on the number of atSave functions
#define	MAX_AT_SAVES		32

typedef	bool (*atSavePtr) (uint16, FILE *);

// save states
enum
{
	BEFORE_SAVE = 1,
	DURING_SAVE,
	AFTER_SAVE,
	BEFORE_RESTORE,
	DURING_RESTORE,
	AFTER_RESTORE
};

uint16	AtSave(atSavePtr savePtr);
void		RemoveAtSave(atSavePtr savePtr);

#endif

