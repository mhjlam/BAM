//
// SAVEBASE.CPP
//
// June 2, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// The AtSave function.  This has been broken off from
// the main SaveMgr file.  As a result, the bulk of the save
// code will only be linked if the actual Save or Restore functions
// are called.
//
//----[]-------------------------------------------------------------

#include "savebase.hpp"

uint16		atSaveCnt = 0;	
atSavePtr	atSaveArray[MAX_AT_SAVES];

// just like atexit, 
// the last function added with be called first. (LIFO)
// if no error, zero is returned. 
// if error, non-zero returned.

uint16	
AtSave(atSavePtr savePtr)
{
	uint32	i;

	if (atSaveCnt >= MAX_AT_SAVES) 
	{
		// we can hold no more
		return(1);
	}
	else
	{
		//  check to see if it's already here
		for (i = 0; i < atSaveCnt; i++)
		{
			if (savePtr == atSaveArray[i])
			{
				// this one's already in the list.  don't add it
				break;
			}
		}

		// if i is equal to atSaveCnt, then we didn't find this already
		// in the list.
		if (i == atSaveCnt)
		{
			// add new one 	
			atSaveArray[atSaveCnt] = savePtr;
 			atSaveCnt++;
		}

		return 0;
	}
}


// remove an at save function pointer from the array
void
RemoveAtSave(atSavePtr savePtr)
{
	uint32	i,j,k;

	//  check to see if it's already here
	for (i = 0; i < atSaveCnt; i++)
	{
		if (savePtr == atSaveArray[i])
		{
			// we found it.  remove this one by compressing the rest
			// on the array
			atSaveCnt--;
			for (j = i; j < atSaveCnt; j++)
			{
				k = j + 1;
				atSaveArray[j] = atSaveArray[k];
			}

			// we are done
			break;
		}
	}
}
