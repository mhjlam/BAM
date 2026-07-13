//
// MEM.HPP
//
// May 26, 1989
// (C) 1986-1991 by Walter Bright  All Rights Reserved.
// Modified December 7, 1993 by Tachyon, Inc.
//
// Memory utilities — stripped to what's still used.
//
//----[]-------------------------------------------------------------


#ifndef mem_hpp
#define mem_hpp 1

#include <stddef.h>

// Allocate size bytes using the system grip (survives ClearAllocations).
void * newSys(size_t size);

// Set to false to suppress unfreed-pointer report at exit.
extern bool fPrintUnfreedPtrs;

#endif /* MEM_H */
