//
// TEXT.CPP
//
// February 3, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	Various objects which retrieve or display text
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apimem.hpp"
#include "context.hpp"
#include "mono.hpp"
#include "resmgr.hpp"
#include "savebase.hpp"
#include "text.hpp"


//----------------------------------------------------------
//	Squib
//----------------------------------------------------------

int	squibLanguageNum = LANG_DEFAULT;

void SetLanguage(int newLangNum)
{
	squibLanguageNum = newLangNum;
}

SquibRes::SquibRes(void)
{
	classID = CID_SQUIB;
}

// unlock whatever resource is currently locked by us
void
SquibRes::Cleanup(void)
{
	if(gRes)
	{
		if(pMemMgr->CheckGrip(gRes) == GRIP_VALID)
		{
			Unlock();
		}
	}
}

bool
SquibRes::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&squibDataStart, 1, (int)((char*)&squibDataEnd -
				(char*)&squibDataStart), pFile);
			break;

		case DURING_RESTORE:
			fread(&squibDataStart, 1, (int)((char*)&squibDataEnd -
				(char*)&squibDataStart), pFile);
			break;
	}
	return(true);
}

// Load and lock a squib res, return ptr to a squib. Will remain
//	locked until calling Unlock or destroyed.
// If handErrorBack is true, nullptr returned on error
// If handErrorBack is false (default), APanic on error
//
char*
SquibRes::Load(uint resNum, int squibId, bool handErrorBack)
{
	RSquibData*	pData;
	uint			offset = 0;
	char*			sz;
	int			low;
	int			high;
	int			midpoint;

	// tweak resource number for easy localization
	resNum += squibLanguageNum;

	pData = (RSquibData*) AutoRes::Lock(RES_SQUIB, resNum);

	// the id numbers are sorted so that we can do a b-search.
	low = 0;
	high = pData->cSquibs - 1;

	do {
		midpoint = (low + high)/2;

		if (squibId < pData->aSquibHeads[midpoint].id)
		{
			high = midpoint - 1;
		}
		else
		{
			low = midpoint + 1;
		}

	} while(squibId != pData->aSquibHeads[midpoint].id && high >= low);

	if (squibId == pData->aSquibHeads[midpoint].id)
	{
		// found the squib
		offset = pData->aSquibHeads[midpoint].offset;

		sz = (char*) pData + sizeof(RSquibData) + 
					(sizeof(RSquibHead) * (pData->cSquibs - 1));
		sz += offset;
	}
	else
	{
		// didn't find squib id
		if (handErrorBack)
		{
			// they want to handle the error
			sz = nullptr;	
		}
		else
		{
			char	sz[40];
			snprintf(sz, sizeof(sz), "Can't find squib %d - %d", resNum, squibId);
			APanic(sz);
		}
	}


	return sz;
}


