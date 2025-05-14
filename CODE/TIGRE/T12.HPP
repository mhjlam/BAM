//
// T12.HPP
//
// February 8, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// T12 Data Compression
//
//		Tigre 12 bit LZ compression.  This method of compression is
//		basically the enhanced 15 bit compression from "The Data
//		Compression Book" (by Mark Nelson) modified to use only 12 bits.
//		Reducing the number of bits greatly reduced to amount of memory
//		needed to run the compression.
//
//		This compression starts out with a table that already has the
//		first 256 characters.  Therefore, "phrases" immediately start
//		building.  The compression codes start at 9 bits and increase
//		to 12 bits as the table values require more bits.  When the
//		table becomes full, it's reinitialized.
//		
//----[]-------------------------------------------------------------

#ifndef T12_HPP
#define T12_HPP

#include "tigre.hpp"

// T12 specific prototypes

uint32 T12_Fluff(FILE* pInputFile, uint32 inputSize, char* pOutputBuffer, char* pInputBuffer);
uint32 T12_Flatten(FILE* pOutputFile, uint32 inputSize, char* pInputBuffer);


#endif


