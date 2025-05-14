//
// CONFIG.CPP
//
// February 8, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Configuration class.
//
//----[]-------------------------------------------------------------


#include <string.h>

#include "config.hpp"



// Default constructor
//
Configuration::Configuration(uint16 sl, uint16 el, uint16 np)
{
	startLine = sl;
	endLine = el;
	numParams = np;

	// construct from default config file
	Load("tigre.cfg");
}


// Constructor specifying config file name
//
Configuration::Configuration(char* szCfgFileName, uint16 sl, uint16 el, uint16 np)
{
	startLine = sl;
	endLine = el;
	numParams = np;
	
	Load(szCfgFileName);
}



// Load a file name
//
bool
Configuration::Load(char* szCfgFileName)
{
	FILE*	pFile;
	char	line[100];
	int	i, j;
	char	buffer[40];
	char*	pComma;

	// empty strings
	memset(szSubjects, NULL, CFG_ENTRY_MAX * CFG_TOKEN_MAX);
	memset(szParams, NULL, CFG_ENTRY_MAX * CFG_PARMS_MAX * CFG_TOKEN_MAX);

	pFile = fopen(szCfgFileName, "rt");

	if (!pFile)
	{
		return FALSE;
	}

	// Find starting line
	for (i = 0; i < startLine; i++)
	{
		if (fgets(line, 100, pFile) == NULL)
		{
			return FALSE;
		}
	}

	// For each line of file
	for (i = 0; i < endLine - startLine + 1; i++)
	{
		if (fgets(line, 100, pFile) == NULL)
		{
			break;
		}

		// read tokens into data arrays
		strcpy(buffer, "%s");
		for (j = 0; j < numParams; j++)
		{
			strcat(buffer, "%s");
		}
		sscanf(line, buffer, szSubjects[i],
									szParams[i][0],
									szParams[i][1],
									szParams[i][2],
									szParams[i][3],
									szParams[i][4],
									szParams[i][5],
									szParams[i][6],
									szParams[i][7],
									szParams[i][8],
									szParams[i][9]);
		pComma = strchr(szSubjects[i], ',');
		if (pComma)
		{
			*pComma = 0;
		}
	}

	fclose(pFile);
	return TRUE;
}



//--------------------------------------------------------------
//	Configuration retrieval methods
//--------------------------------------------------------------


// Return subject index if subject exists, regardless of param.
//
int16
Configuration::HasEntry(char* szSub)
{
	int16	i;

	// scan data array
	for (i = 0; i < CFG_ENTRY_MAX; i++)
	{
		if (!strcmp(szSub, szSubjects[i]))
		{
			return i;
		}
	}
	return -1;
}


// Return true if a param exists for a specific subject number.
//
bool
Configuration::HasParam(uint16 sub, char* szParam)
{
	int	i;

	// scan data array
	for (i = 0; i < CFG_PARMS_MAX; i++)
	{
		if (!strcmp(szParams[sub][i], szParam))
		{
			return TRUE;
		}
	}
	return FALSE;
}


// Copy a param into the destination string, given a subject string.
//
char*
Configuration::CopyStr(char* szDest, char* szSub, uint16 param)
{
	int	i;

	// empty dest string
	*szDest = 0;

	// scan data array
	for (i = 0; i < CFG_ENTRY_MAX; i++)
	{
		if (!strcmp(szSub, szSubjects[i]))
		{
			strcpy(szDest, szParams[i][param]);
			return szDest;
		}
	}
	return NULL;
}


// Copy a param into the destination string, given a subject number.
// If the param number is -1, the subject itself will be copied.
//
char*
Configuration::CopyStr(char* szDest, uint16 sub, uint16 param)
{
	// empty dest string
	*szDest = 0;

	if (sub < CFG_ENTRY_MAX)
	{
		if (sub == -1)
		{
			strcpy(szDest, szSubjects[sub]);
		}
		else
		{
			strcpy(szDest, szParams[sub][param]);
		}
		return szDest;
	}
	else
	{
		return NULL;
	}
}


// Return param as number, if no number, returns -1.
//
int32
Configuration::GetNum(char* szSubject)
{
	char	sz[CFG_TOKEN_MAX];

	if (CopyStr(sz, szSubject))
	{
		return atoi(sz);
	}
	
	return -1;
}


// Find the subject that has the specified parameter in the
// specified position.
//
int16
Configuration::FindSubject(char* szParam, uint16 param)
{
	uint16	i;

	for (i = 0; i < CFG_ENTRY_MAX; i++)
	{
		if (!strcmp(szParams[i][param], szParam))
		{
			return i;
		}
	}
	return -1;
}



//--------------------------------------------------------------
//	Configuration alteration methods
//--------------------------------------------------------------


// Update the value of a config, or add a new one.
//
void
Configuration::SetStr(char* szSub, char* szParam, uint16 param)
{
	int	i;

	// scan data array
	for (i = 0; i < CFG_ENTRY_MAX; i++)
	{
		if (!strcmp(szSub, szSubjects[i]))
		{
			strcpy (szParams[i][0], szParam);
			return;
		}

		// if at end of array, add new entry
		if (!strlen(szSubjects[i])) 
		{
			strcpy (szSubjects[i], szSub);
			strcpy (szParams[i][param], szParam);
			return;
		}
	}
}


// Write all configs out as a new config file.
//
void
Configuration::WriteConfig(char* szNewName)
{
	int	i;
	FILE*	pFile;

	pFile = fopen(szNewName, "wt");

	// scan data array
	for (i = 0; i < CFG_ENTRY_MAX; i++)
	{
		if (!strlen(szSubjects[i]))
		{
			break;
		}

		fprintf(pFile, "%s\t%s\n", szSubjects[i], szParams[i][0]);
	}

	fclose(pFile);	
}



