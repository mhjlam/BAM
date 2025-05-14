//
// GRIPSMAX.hpp
//
// March 1, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//
//----[]-------------------------------------------------------------


#ifndef	gripsmax_hpp
#define	gripsmax_hpp

#define	GRIPS_MAX			3000

// Because save game needs to restore to the save grip numbers,
// we have two grip ranges.  One for systems grips and another
// for all other grips.  Systems grips are ones that are not save
// or restored.  Examples of these are sound drivers and resouce
// map files.  System grips start at 1 and go to SYSTEM_GRIP_MAX.
// All other grips start at SYSTEM_GRIP_MAX and continue to GRIPS_MAX. 

#define	FIRST_SYSTEM_GRIP		 1
#define	SYSTEM_GRIP_MAX		35

#define	FIRST_STANDARD_GRIP	SYSTEM_GRIP_MAX
#define	STANDARD_GRIP_MAX		GRIPS_MAX

#endif 

