//
// COMP.CPP
//
// May 10, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Compression routines.
//
//----[]-------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comp.hpp"
#include "file.hpp"
#include "t12.hpp"
#include "writeres.hpp"

// Compress a resource from a buffer into a file.

uint32
AFlatten(FILE* pOutputFile, uint32 inputSize, char* pInputBuffer,
				uint16 compType)
{
	uint32 compressedSize = 0;

	// mask out the compression types that we don't deal with here
	compType &= ~(CMP_NONE | CMP_TRLE); 
	
	switch (compType)
	{
		case CMP_T12:
			compressedSize = T12_Flatten(pOutputFile, inputSize, (char *) pInputBuffer);
			break;

		default:
			compressedSize = fwrite(pInputBuffer, 1, inputSize, pOutputFile);
	}

	return compressedSize;
}


// Decompress a resource into a buffer.  The input comes from either
// a file (if pInputFile is not NULL) or from a memory buffer (if
// pInputBuffer is not NULL).  If pInputBuffer is not specified, it
// is assumed NULL and input will be taken from a file.
//
uint32 
AFluff(TFile* pInputFile, uint32 inputSize, char* pOutputBuffer,
			uint16 compType, char* pInputBuffer)
{
	uint32	uncompressedSize = 0;

	// mask out the compression types that we don't deal with here
	compType &= ~(CMP_NONE | CMP_TRLE); 
	
	switch (compType)
	{
		case CMP_T12:
			uncompressedSize = T12_Fluff( (FILE*)pInputFile->IsOpen(),
													inputSize,
													(char *) pOutputBuffer,
													pInputBuffer);
			break;

		default:
			// no compression at this level.
			// just read into the buffer
			if (pInputFile)
			{
				pInputFile->Read(pOutputBuffer, 1, inputSize);
			}
			else
			{
				memcpy(pOutputBuffer, pInputBuffer, inputSize);
			}
	}

	return uncompressedSize;
}

