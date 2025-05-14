//
// COMP.HPP
//
// May 10, 1994
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Definitions for Compression calls.
//
//----[]-------------------------------------------------------------


#ifndef	comp_hpp
#define	comp_hpp

#include "tigre.hpp"
#include "file.hpp"

// These are the Master Compression Prototypes

uint32 AFluff(TFile* pInputFile, uint32 inputSize, char* pOutputBuffer,
					uint16 compType, char* pInputBuffer = NULL);

uint32 AFlatten(FILE* pOutputFile, uint32 inputSize, char* pInputBuffer,
					uint16 compType);

#endif
