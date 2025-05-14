//
// txt2sqb.cpp
//
// February 16, 1995
// (C) 1995, Tachyon, Inc.  All rights reserved.
//
//	Compile source text file into a resource to be read by Squib objects.
//
//----[]-------------------------------------------------------------

#include <ctype.h>

#include	"apimem.hpp"
#include	"keylist.hpp"
#include	"mem.hpp"
#include	"text.hpp"


void		Error(char* sz1, char* sz2 = NULL);
int		GetLineNum (void);
size_t	SizeOfFile(char* szFileName);
char*		LoadFile (char* szFileName);
void		ParseSquibs(void);

uint16	wordData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char*		gszSource = NULL;
char*		gszStartSource = NULL;
int			resNum = 0;
size_t	fileSize;
KeyList	*squibList = NULL;
char		*pSquibTxt = NULL;
char		*pSquibData;

//-----------------------------------------------------------------
//
void
Cleanup(void)
{
	if (squibList)
	{
		squibList->Release();
		delete squibList;
		squibList = NULL;
	}

	if (gszStartSource)
	{
		mem_free (gszStartSource);
		gszStartSource = NULL;
	}

	if (pSquibTxt)
	{
		mem_free (pSquibTxt);
		pSquibTxt = NULL;
	}
}

//-----------------------------------------------------------------
//
void
Usage(void)
{
	printf("Invalid number of parameters\n\n");
	printf("Usage:     txt2sqb filename [filename ...]\n");
	printf("           filenames are .txt files (extension optional).\n");
	printf("Examples:  txt2sqb 1234.txt\n");
	printf("           txt2sqb 1234\n\n");

	Cleanup();
	exit(1);
}

int
main(int argc,char *argv[])
{
	unsigned	i;
	char		szFile[FILENAME_MAX];

	printf("\nTXT2SQB Version 1.2\nCopyright 1995 Tachyon, All Rights Reserved.\n\n");

	if (argc <2)
	{
		Usage();
		exit(1);
	}

	squibList = new KeyList(200, TRUE);

	for (i = 1; i < argc; i++)
	{
		sprintf (szFile, "%s", argv[i]);

		if (!strchr (szFile, '.'))
		{
			// they didn't add an extension.  
			// add .txt 
			strcat(szFile, ".txt");
		}

		resNum = atoi(szFile);

		if (!resNum)
		{
			printf("\tInvalid file name (%s). It should start with a number.\n\t\tFile not processed.\n", szFile);
		}
		else
		{
			// make sure that the squib list is empty.
			// this is quicker to do here instead of at the end
			// because we will only be processing one file most
			// of the time.  If this was at the end, it would have
			// to actually do some work which would be unnecessary
			// if we were only processing one file.
			squibList->Release();

			printf ("\tLoading %s\n", szFile);

			// load source text
			// (this will allocate memory and return the address)
			gszStartSource = gszSource = LoadFile (szFile);

			// allocate enough memory to hold all the reformated squibs
			TRACK_MEM("Squib data");
			pSquibData = pSquibTxt = (char *) mem_malloc (fileSize);
			ParseSquibs();
			mem_free (pSquibTxt);
			pSquibTxt = NULL;

			mem_free (gszStartSource);
			gszStartSource = NULL;
		}

	}

	Cleanup();

	return 0;
}

//-----------------------------------------------------------------
// Fatal error message
//
void
Error(char* sz1, char* sz2)
{
	int lineNum = GetLineNum ();

	if (sz2) {
		printf ("FATAL ERROR: %s%s at line %d, squib %d\n", sz1, sz2, lineNum, resNum);
	} else {
		printf ("FATAL ERROR: %s at line %d, squib %d\n", sz1, lineNum, resNum);
	}

	Cleanup();

	exit (1);
}




//-----------------------------------------------------------------
//
size_t
SizeOfFile(char* szFileName)
{
	FILE*		pFile;
	size_t	cBytes;

	pFile = fopen (szFileName, "rt");
	if (pFile == NULL) {
		Error ("Can't find ", szFileName);
	}

	fseek (pFile, 0, SEEK_END);
	cBytes = ftell (pFile);
	fseek (pFile, 0, SEEK_SET);

	fclose (pFile);

	return cBytes;
}



//-----------------------------------------------------------------
//
char*
LoadFile (char* szFileName)
{
	FILE*		pFile;
	size_t	cBytes;
	size_t	x;
	char*		pBytes;

	cBytes = fileSize = SizeOfFile (szFileName);
	if (!cBytes)
	{
		Error ("File size zero: ", szFileName);
	}

	TRACK_MEM("File data");
	pBytes = (char *) mem_malloc (cBytes+100);
	if (!pBytes) {
		Error ("Out of memory, file too large: ", szFileName);
	}
	memset (pBytes, 0, cBytes+100);

	pFile = fopen (szFileName, "rt");
	if (pFile == NULL) {
		Error ("Can't find ", szFileName);
	}

	x = fread (pBytes, 1, cBytes, pFile);
	if (x < 1) {
		Error ("Read error", NULL);
	}

	fclose (pFile);

	return pBytes;
}

void
SkipWhiteSpace(void)
{
	// skip white space to first char
	while (isspace(*gszSource)) 
	{
		gszSource++;
	}
}

void
ParseSquibs(void)
{
	int	squibId;
	char	mess[20];
	char	*pSquib;
	bool	working;
	char	*pSquibEnd;
	char	*pNextSquib;
	char	*pSquibWork;
	bool	compressText;
	int	squibSize;
	int	totalSquibSize;
	char	*pResource;
	char	*pAllocedMem;
	bool	createdRes;
	int	sizeOfSquib;
	int	offset;
	int	i;
	RSquibData *pRSquib;
	char	*pStartLine;
	char	*pNextLine;
	char	newChar;
	char	*pStartSquib;
	bool	newLine;

	while(TRUE)
	{
		// find the beginning of a squib
		pSquib = strchr (gszSource, '{');

		if (!pSquib)
		{
			// no squib was found.  we are done	
			break;
		}

		gszSource = pSquib + 1;

		// find the end of the squib
		pSquibEnd = NULL;
		pSquibWork = gszSource;
		while(TRUE)
		{
			if (*pSquibWork == '}')
			{
				if (*(pSquibWork + 1) != '}' &&
					 *(pSquibWork - 1) != '}')
				{
					// we have found the end
					pSquibEnd = pSquibWork;
					break;
				}
			}
			else
			{
				if (*pSquibWork == 0)
				{
					// we have reached EOF. stop.
					break;
				}
			}
			pSquibWork++;
		}

		// find the beginning of the next squib
		pNextSquib = NULL;
		pSquibWork = gszSource;
		while(TRUE)
		{
			if (*pSquibWork == '{')
			{
				if (*(pSquibWork + 1) != '{' &&
					 *(pSquibWork - 1) != '{')
				{
					// we have found the end
					pNextSquib = pSquibWork;
					break;
				}
			}
			else
			{
				if (*pSquibWork == 0)
				{
					// we have reached EOF. stop.
					break;
				}
			}
			pSquibWork++;
		}

		if (pSquibEnd == NULL)
		{
			Error ("No closing } for squib");
		}
		else
		{
			if (pNextSquib &&
			 	pSquibEnd > pNextSquib)
			{
				Error ("New Squib started before ending existing squib.");
			}
		}

		SkipWhiteSpace();

		if (*gszSource++ != ':')
		{
			Error ("Can't find id number colon", NULL);
		}

		SkipWhiteSpace();

		// get the id number
		squibId = atoi(gszSource);

		if (!squibId)
		{
			Error("No id number");
		}

		// check to see if this id has been used before
		if (squibList->FirstKey(squibId) != NULL)
		{
			sprintf(mess, "%d", squibId);
			Error("Id number already used: ", mess);		
		}

		// move the pointer past the number
		while(*gszSource != NULL && isdigit(*gszSource))
		{
			gszSource++;
		}

		// find the beginning of the text
		SkipWhiteSpace();

		if (*gszSource == 0)
		{
			Error("Premature EOF reached. (No closing })");		
		}

		// add to our squibs
		squibList->Add(pSquibData, squibId);
		pStartSquib = pSquibData;

		// now compress this char by char.
		// get rid of white space at the end of lines.
		// convert backslash codes to chars.

		sizeOfSquib = pSquibEnd - gszSource;
		working = TRUE;
		compressText = FALSE;
		pStartLine = pSquibData;

		while(working)
		{
			// set to be false
			newChar = 0xff;
			newLine = FALSE;

			switch(*gszSource)
			{
				case '{':
					// since we already scanned for the beginning of the
					// the next squib, this can only mean that they
					// want to print {
					// there will be another { to denote this.  
			  		// leave the first { and move the rest of the text
					compressText = TRUE;
					break;

				case '}':
					if (gszSource == pSquibEnd)
					{
						// end of string
						working = FALSE;
						newChar = 0;
					}
					else
					{
						// this is not the closing }, so this must
						// be a } that should be printed.
						// there will be another } to denote this.  
			  			// leave the first } and move the rest of the text
						compressText = TRUE;
					}
					break;

				case '\\':
					// special backslash codes
					switch (*(gszSource + 1))
					{
						case '\\':
							// they want to print a \. 
							// there is another \ to denote this.  
			  				// leave the first \ and move the rest of the text
							compressText = TRUE;
							break;

						case 'n':
							// the want a new line.
							// convert the \ to a newline
							newChar = '\n';
							// move the rest of the text over
							compressText = TRUE;
							break;

						default:
							Error("\ with no valid code");
							break;
					}

					break;

				case '\f':
				case '\r':
				case '\n':
					// we have reached the end of the line.
					// search backwards for the last valid char.
					pNextLine = pSquibData;
					pSquibData--;
					while (isspace(*pSquibData)) 
					{
						pSquibData--;
					}

					if (pSquibData > pStartLine)
					{
						pSquibData++;
						// add only on space
						newChar = ' ';
					}
					else
					{
						// this must be an empty line.
						// empty lines just go away.
						pSquibData = pNextLine;

						// don't do the char copy
						newChar = 0xfe;
					}

					pStartLine = pSquibData;
					newLine = TRUE;
					break;
			}

			switch(newChar)
			{
				case 0xff:
					// normal char copy
					*pSquibData = *gszSource;
					pSquibData++;
					break;

				case 0xfe:
					// do nothing
					break;

				default:
					// we have a replacement char
					*pSquibData = newChar;
					pSquibData++;
					break;

			}

			if (compressText)
			{
				// move the text over by 1
				sizeOfSquib--;
				gszSource++;

				compressText = FALSE;
			}

			if (newLine)
			{
				SkipWhiteSpace();	
			}
			else
			{
	 			sizeOfSquib--;
	 			gszSource++;
			}
		}

		// we have completed processing this squib.
		if ((pSquibData - 1) == pStartSquib)
		{
			// this is an empty squib
			*pSquibData = 0;
			pSquibData++;
		}
		else
		{
			// this is not an empty squib.  
			// get rid of the space that was left on the last line.
			pSquibData--;
//			*(pSquibData - 1) = 0;
			while(*pSquibData == ' ' || *pSquibData == 0)
			{
				pSquibData--;
			}

			// move 1 past the last char and null terminate
			pSquibData++;
			*pSquibData = 0;
			// move past the null
			pSquibData++;
		}
	}

	if (squibList->count == 0)
	{
		printf("\tNo squibs found\n");		
	}
	else
	{
		// sort the squibs
		squibList->SortByKey(SORT_ASCENDING);

		// find out how much memory the squibs will require
		totalSquibSize = sizeof(RSquibData);
		// we subtract 1 from the count, becuase 1 squib head is already
		// in the RSquibData structure
		totalSquibSize += (squibList->count - 1) * sizeof(RSquibHead); 
		
		for (pSquib = (char *) squibList->FirstValue();
			pSquib != NULL;
			pSquib = (char *) squibList->NextValue())
		{
			totalSquibSize += strlen(pSquib) + 1;
		}

		// allocate enough memory for the squib resource
		TRACK_MEM("Squib Res");
		pResource = pAllocedMem = (char *) mem_malloc(totalSquibSize);

		// now move everything into the memory

		// setup the Squib Header
		pRSquib = (RSquibData *) pResource;
		pRSquib->cSquibs = squibList->count; 

		// move the pResource point to the area containing the first
		// squib string
		pResource += sizeof(RSquibData);
		pResource += (squibList->count - 1) * sizeof(RSquibHead);

		// setup the starting offset
		offset = 0;

		// now move the squibs
		node	*pNode;
		for (pNode = (node *) squibList->First(), i = 0;
			pNode != NULL;
			pNode = (node *) squibList->Next(), i++)
		{
			pSquib = (char*) squibList->NodeValue(pNode);

			pRSquib->aSquibHeads[i].offset = offset;
			pRSquib->aSquibHeads[i].id = pNode->key;

			squibSize = strlen(pSquib) + 1;

			strncpy(pResource, pSquib, squibSize);
			offset += squibSize;
			pResource += squibSize;
		}
		
		// write out resource

		printf ("\tWriting resource %d.sqb.\n", resNum);

		// write out resource, writeres.cpp
		createdRes = CreateGeneric (RES_SQUIB, resNum, totalSquibSize, pRSquib, wordData, FALSE);
		if (!createdRes)
		{
			mem_free (pAllocedMem);
			pAllocedMem = NULL;
			Error ("failed to write resource", NULL);
		}

		if (pAllocedMem)
		{
			mem_free (pAllocedMem);
		}
	}
}


//-------------------------------------------------------------------
// count newlines up to current source ptr
//
int
GetLineNum ()
{
	char*	pLine;
	int	cLines = 0;

	pLine = gszStartSource;

	// count newlines up to current source ptr
	while (pLine < gszSource)
	{
		cLines++;
		pLine = strchr (pLine, '\n');
		if (pLine == NULL) {
			break;
		}
		pLine++;
	}

	return cLines;
}

