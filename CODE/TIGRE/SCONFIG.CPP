//
// SCONFIG.CPP
//
// March 15, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Config File format:
//
// CONFIG_LABEL      config_value
//
// i.e.
//
// MUSIC					1,2 
//
// CONFIG_LABEL must be one continuous name.
// CONFIG_LABEL and config_value must be seperated by at least
// one space or tab.  Also, they both must be on the same line.
// config_value can be almost any continuous string.
//
// ; is used for comments.  Since ; can also be used in a label
// or value, you must place comments after config_value and it must
// be seperated by at least one space or tab.  You can also place
// a comment on a line by itself.
//
// Config Array format:
//
// Each line in the file will use 1 grip.  Each grip is derefenced
// and parsed to find the label and value.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apimem.hpp"
#include "sconfig.hpp"


static int	atoiGetCharValue(char workChar);

// size of default list.  also the size that the list will grow
// by when the list size grows beyond it's current size.
#define	CONFIG_LIST_BUMP_SIZE	20

#define	SC_RETURN	13		// return
#define	SC_EOL		10		// end of line
#define	SC_TAB		9		// tab


SConfig::SConfig()
{
	// init the file name to NULL
	configFileName[0] = 0;
	fileLoaded = FALSE;

	gConfigArray = NULL;
}

SConfig::~SConfig()
{
	ResetConfigFile();	
}

void
SConfig::SetConfigFile(char* fileName)
{
	if (strlen(fileName) > SCONFIG_FNAME_MAX)
	{
		APanic("Filename too long!");
	}

	if (configFileName[0] != 0)
	{
		APanic("Config File still in Use."); 
	}

	if (fileName[0] == 0)
	{
		APanic("Invalid file name.");
	}

	strcpy(configFileName, fileName);

	// setup the array for the first time
	curSize = maxSize = 0;
	ReSizeArray(CONFIG_LIST_BUMP_SIZE);
}

void
SConfig::ResetConfigFile()
{
	// clear the current config file

	uint	i;
	grip	*pConfigArray;

	if (configFileName != 0)
	{
		// init the file name to NULL
		configFileName[0] = 0;

		// free the memory that is used.

		if (gConfigArray)
		{
			pConfigArray = ADerefAs(grip, gConfigArray);
			for (i = 0; i < curSize; i++)
			{
				AFree(*pConfigArray);
				pConfigArray++;
			}

			AFree(gConfigArray);
			gConfigArray = NULL;
		}

		fileLoaded = FALSE;
	}
}

// if file loaded, return TRUE
bool
SConfig::LoadConfig(char* fileName, bool gotoDOSOnError)
{
	SetConfigFile(fileName);
	return LoadConfig(gotoDOSOnError);
}

// if file loaded, return TRUE
bool
SConfig::LoadConfig(bool gotoDOSOnError)
{
	FILE	*pFile;
	char	line[MAX_CONFIG_LINE_SIZE];

	if (configFileName[0] == 0)
	{
		APanic("No config file name");
	}

	pFile = fopen(configFileName, "rt");

	if (pFile)
	{
		// read the file, line by line
		while(fgets(line, MAX_CONFIG_LINE_SIZE, pFile))
		{
			if (strlen(line) >= MAX_CONFIG_LINE_SIZE)
			{
				APanic("Config line too long");
			}

			SetEntry(line, NULL);
		}

		fileLoaded = TRUE;
		fclose(pFile);
	}
	else
	{
		if (gotoDOSOnError)
		{
			sprintf(line, "Config file not found: %s", configFileName);
			APanic(line);
		}
	}

	return fileLoaded;
}


// save using the current file name

void
SConfig::SaveConfig()
{
	if (configFileName[0] == 0)
	{
		APanic("No config file name");
	}

	SaveConfig(configFileName);
}

// save using the passed file name.

void
SConfig::SaveConfig(char* fileName)
{
	FILE		*pFile;
	char		line[MAX_CONFIG_LINE_SIZE];
	uint		i;
	grip		*pConfigArray;
	char		*pConfigLine;

	pFile = fopen(fileName, "w+t");

	if (!pFile)
	{
		sprintf(line, "Config file write error: %s", fileName);
		APanic(line);
	}
	
	if (gConfigArray)
	{
		pConfigArray = ADerefAs(grip, gConfigArray);
		for (i = 0; i < curSize; i++)
		{
			pConfigLine = ADerefAs(char, *pConfigArray);
			if (fputs(pConfigLine, pFile) == EOF)
			{
				APanic("Error writing config file");
			}
			pConfigArray++;
		}
	}

	fclose(pFile);
}

void
SConfig::DeleteEntry(char* pConfigName)
{
	int		i;
	uint		moveBytes;
	grip		*pConfigArray;

	i = FindEntry(pConfigName, NULL);
	
	if (i != -1)
	{
		// an entry was found
		
		pConfigArray = ADerefAs(grip, gConfigArray);
		pConfigArray += i;

		// free memory for the entry
		AFree(*pConfigArray);

		// compress the array (we want to maintain the same order)

		// NOTE: 1 is added to i because we start counting at zero
		moveBytes = (curSize - (i + 1)) * sizeof(grip);
		if (moveBytes)
		{
			memmove(pConfigArray, pConfigArray + 1, moveBytes);	
		}

		// move pointer to the end and null terminate
		pConfigArray += (curSize - (i + 1));
		*pConfigArray = NULL;

		curSize--;

		// NOTE: we currently don't reduce the size of the gConfigArray
		// during deleting entries.  If we did, it would have to be
		// deleted in multiples of CONFIG_LIST_BUMP_SIZE.  Also, it
		// probably shouldn't be reduced to below 1 unit of 
		// CONFIG_LIST_BUMP_SIZE.  For now, I really don't think it's
		// worth the effort.
	}
	
}

// Locate a configName in the config file.
// If it isn't found, a NULL is placed in configEntry and the
// array pointer is returned.
// If it is found, the configEntry is copied into the passed
// configEntry array and the array pointer is returned.

char*
SConfig::GetEntry(char* pConfigName, char* pConfigEntry)
{
	if (FindEntry(pConfigName, pConfigEntry) == -1)
	{
		// no entry was found
		*pConfigEntry = 0;
	}

	return pConfigEntry;
}

// Locate a configName in the config file.
// If it isn't found, a NULL is placed in configEntry and a
// zero is returned.
// If it is found, the configEntry is copied into the passed
// configEntry array and numeric value of the entry is returned.
// NOTE: hex values (preceded by 0x) will automatically be converted)

int
SConfig::GetEntryDecimal(char* pConfigName, char* pConfigEntry)
{
	int	decValue;

	if (FindEntry(pConfigName, pConfigEntry) == -1)
	{
		// no entry was found
		*pConfigEntry = 0;
		decValue = 0;
	}
	else
	{
		if (pConfigEntry[1] == 'x')
		{
			// this is hex
			decValue = atoi(pConfigEntry + 2, 16); 
		}
		else
		{
			// this is plain decimal
			decValue = atoi(pConfigEntry);
		}
	}

	return decValue;
}


// Locate a configName in the config file.
// If it isn't found, the name and entry are added.
// If it is found, the configEntry is updated.

void
SConfig::SetEntry(char* pConfigName, char* pConfigEntry)
{
	int		i;
	uint		configEntryLength;
	grip		*pConfigArray;
	uint		totalSize;
	grip		gNewEntry;
	char		*pNewEntry;
	char		*pOldEntry;
	char		*pWorkEntry;
	char		pToken[MAX_CONFIG_TOKEN_SIZE + 1];
	char		tabArray[] = {SC_TAB,0};
	char		equalArray[] = {'=',0};
	char		returnArray[] = {SC_EOL,0};

	if (pConfigEntry == NULL)
	{
		// if we aren't passing an entry, this must be a file read.
		// don't check for an entry, just create a new one
		i = -1;
		configEntryLength = 0;
	}
	else 
	{
		i = FindEntry(pConfigName, pToken);
		configEntryLength = strlen(pConfigEntry);
	}
		

	if (i == -1)
	{
		// no entry was found.
		// find the next available index
		if (curSize >= maxSize)
		{
			// no more entries left.
			// add more
			ReSizeArray(CONFIG_LIST_BUMP_SIZE);
		}

		// since we start at index zero, use curSize for new index
		// then increment
		
		i = curSize;
		curSize++;
	}

	pConfigArray = ADerefAs(grip, gConfigArray);
	pConfigArray += i;

	if (*pConfigArray == NULL)
	{
		// new entry
		// total size = config name size + entrySize + 2 tab + 1 equal sign + 1 eol + null terminator
		totalSize = strlen(pConfigName) + configEntryLength + 5;
		TRACK_MEM("SConfig_Entry");
		gNewEntry = AMalloc(totalSize);

		pNewEntry = ADerefAs(char, gNewEntry);
		strcpy(pNewEntry, pConfigName);

		// If we have an entry, add it.  During the config file read,
		// we will not have an entry.  the entire line will be passed
		// as the config name.
		if (pConfigEntry != NULL)
		{
			strcat(pNewEntry, tabArray);
			strcat(pNewEntry, equalArray);
			strcat(pNewEntry, tabArray);
			strcat(pNewEntry, pConfigEntry);
			strcat(pNewEntry, returnArray);
		}

		// save the new entry grip
		*pConfigArray = gNewEntry;
	}
	else 
	{
		// revise an entry
		// preserve all the spacing

		pOldEntry = pWorkEntry = ADerefAs(char, *pConfigArray);

		// find the new total entry size
		// total size = old total size - old entry size + new entry size + null terminator
		totalSize = strlen(pOldEntry) - strlen(pToken) + configEntryLength + 1;

		TRACK_MEM("SConfig_Entry");
		gNewEntry = AMalloc(totalSize);
		pNewEntry = ADerefAs(char, gNewEntry);

		// since the old entry is no longer needed and is at least 1 char
		// long, let's put a null terminator at the start of the old
		// entry so that we can copy the old config name with strcpy.

		// NOTE: configEntryStartOffset was set through the call to 
		// FindEntry.
		pOldEntry += configEntryStartOffset;
		*pOldEntry = NULL;

		strcpy(pNewEntry, pWorkEntry);

		// the old config name section also contained the necessary space or
		// tab that is needed inbetween the name and entry.  
		// Now we will append the new entry
		strcat(pNewEntry, pConfigEntry);

		// there is a chance that there is a comment after the entry.
		// Let's copy anything else that was left on the end of the old
		// entry.

		pOldEntry += strlen(pToken);
		strcat(pNewEntry, pOldEntry);

		// free the memory used by the old entry
		AFree(*pConfigArray);

		// save the new entry grip
		*pConfigArray = gNewEntry;
	}
}


// this function will attempt to find the configName in
// the config file.
// If the configName is found, the entry is copied into
// the configEntry pointer.  Also, the index into the list is returned.
// If the configName isn't found, -1 is returned.
int16
SConfig::FindEntry(char* pConfigName, char* pConfigEntry)
{
	uint16	i;
	char		*pConfigWork;
	char		*pStartLine;
	grip		*pConfigArray;
	char		pToken[MAX_CONFIG_TOKEN_SIZE + 1];

//	if (!fileLoaded)
//	{
//		LoadConfig(FALSE);
//	}

	if (gConfigArray == NULL ||
		 curSize == 0)
	{
		// no entries
		return -1;
	}

	pConfigArray = ADerefAs(grip, gConfigArray);

	for (i = 0; i < curSize; i++)
	{
		pStartLine = pConfigWork = ADerefAs(char, *pConfigArray); 	

		// find first real char
		pConfigWork = FindToken(pConfigWork, pToken);

		if (pConfigWork != NULL)
		{
			// compare
			if (!strcmp(pToken, pConfigName))
			{
				// found the config name
				// now look for the entry
				pConfigWork = FindToken(pConfigWork, pToken);

				if (pConfigWork != NULL &&
					 *pToken == '=')
				{
					// skip past equal signs
					pConfigWork = FindToken(pConfigWork, pToken);
				}

				if (pConfigWork != NULL)
				{
					// a config entry was found.  time to go

					if (pConfigEntry)
					{
						// if an entry array was passed, copy the token into it
						strcpy(pConfigEntry, pToken);
					}

					// first find out the offset from the start of the line
					// to the beginning of the entry
					configEntryStartOffset = pConfigWork - pStartLine - strlen(pToken);
					break;
				}
			}
		}

		pConfigArray++;
	}

	if (i == curSize)
	{
		// we reached the end without finding an entry
		return -1;
	}
	else
	{
		// we found an entry.  return the index
		return i;
	}
}

// Scan forward to find the first byte of a token
// If a token is found, the pointer to the position after the token
// is returned.  Also, the token is put into pToken.
// If a token isn't found, NULL is returned

char*
SConfig::FindToken(char* pCharString, char* pToken)
{
	char* returnValue = NULL;
	uint	i;

	while(TRUE)
	{
		if (*pCharString == ';' ||
			 *pCharString == SC_EOL ||
			 *pCharString == SC_RETURN ||
			 *pCharString == 0)
		{
			// start of a comment or end of line.  no token after this
			break;
		}

		if (*pCharString == ' ' ||
			 *pCharString == SC_TAB) 
		{
			pCharString++;			
		}
		else 
		{
			returnValue = pCharString;
			break;
		}
	}

	if (returnValue)
	{
		// a token was found

		i = 0;
		// copy the token into the passed array
		while(TRUE)
		{
			if (*pCharString == 0 ||
				 *pCharString == SC_EOL ||	
				 *pCharString == SC_RETURN ||
				 *pCharString == ' ' ||
				 *pCharString == SC_TAB)
			{
				// we are done
				break;
			}

			if (i >= MAX_CONFIG_TOKEN_SIZE)
			{
				APanic("Token too big!");	
			}

			*pToken = *pCharString;
			i++;
			pCharString++;
			pToken++;
		}

		// null terminate the token
		*pToken = 0;

		// set returnValue to point to the position right after the token
		returnValue = pCharString;
	}

	return returnValue;
}


// change the size of the current config array
void
SConfig::ReSizeArray(int deltaSize)
{
	grip	gNewArray;
	int	newSize;
	char	*pNewArray;
	int	copySize;
	int	clearSize;

	newSize = maxSize + deltaSize;

	// don't allow the array to become an invalid size
	if (newSize < 1)
 	{
		APanic("Config size less than 1");
	}

	if (curSize > newSize)
	{
		APanic("New array size too small");
	}

	// allocate the new array
	TRACK_MEM("SConfig_Entry");
	gNewArray = AMalloc(newSize * sizeof(grip));
	
	pNewArray = ADerefAs(char, gNewArray);

	// copy the existing data into the new array
	if (gConfigArray)
	{
		copySize = curSize * sizeof(grip);
		memcpy(pNewArray, ADerefAs(char, gConfigArray), copySize);

		// free the old array
		AFree(gConfigArray);
	}
	else
	{
		// no array to copy from (new array)
		copySize = 0;
	}

	// save the new array grip
	gConfigArray = gNewArray;

	// set the rest of the grips to NULL
	pNewArray += copySize;
	clearSize = ASizeOfGrip(gNewArray) - copySize;
	if (clearSize > 0)
	{
		memset(pNewArray, NULL, clearSize);
	}

	// set the new array size
	maxSize = (uint16) newSize;
}


// The atoi(void*, int) is thrown in here because only the sound
// stuff is using it right now.

// this routine converts a single char to an int, taking
// into account a number base up to 36.  If the
// char is not valid, -1 is returned. 

int
atoiGetCharValue(char workChar)
{
	int	digitValue;

	// set to an unrealistic value
	digitValue = -1;;

	if (workChar >= '0' && workChar <= '9')
	{
		digitValue = workChar - '0';
	}
	else
	{
		if (workChar >= 'a' && workChar <= 'z')
  		{
			digitValue = workChar - 'a' + 10;
		}
		else
		{
			if (workChar >= 'A' && workChar <= 'Z')
  			{
				digitValue = workChar - 'A' + 10;
			}
		}
	}

	return digitValue;
}


// Since atoi only converts from decimal, here is our own routine.
// This handles number bases 2 through 36.

int
atoi(void* str, int base)
{
	int	value = 0;
	int	sign = 1;
	int	digitValue;
	int	digitPosition;
	int	baseMultiplier;

	char	*workStr = (char*) str;
	char	*startNumStr;

	if (base < 2 || base > 36)
	{
		// invalid base.  return zero
		// (I suppose that we could crash here, but that
		//  might cause more problems than returning zero.)
		return 0;
	}

	// scan through leading spaces, tabs.

	while(*workStr == ' ' || *workStr == '\t')
	{
		workStr++;
	}

	// check for sign value
	if (*workStr == '-')
	{
		sign = -1;
		workStr++;
	}
	else
	{
		if (*workStr == '+')
		{
			// sign is already positive, so just move the pointer
			workStr++;
		}
	}

	startNumStr = workStr;

	// scan to the end of the number
	while(TRUE)
	{
		if (*workStr == NULL)
		{
			// we have reached the end
			break;
		}
		else
		{
			digitValue = atoiGetCharValue(*workStr);
			if (digitValue == -1 ||
				 digitValue > base)
			{
				// the current char is not valid.  we are done.
				break;
			}
		}

		workStr++;
	}

	if (workStr != startNumStr)
	{
		// we have a number.
		// back up one char so that we know where the end of the number is.
 		workStr--;
		
		// start at the end and work towards the beginning		
		digitPosition = 0;
		baseMultiplier = 1;

		while(workStr >= startNumStr)
		{
			value += atoiGetCharValue(*workStr) * baseMultiplier;
			baseMultiplier *= base;
 			digitPosition++;
			workStr--;
		}
	}

	return value * sign;
}

