//
// API.CPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Portable global API calls
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "mono.hpp"
#include "savebase.hpp"
#include "types.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


//---------------------------------------------------------------

// Return the maximum of a and b
uint
AMax(int a, int b)
{
	return (a >= b ? a : b);
}


// Return the minimum of a and b
uint
AMin(int a, int b)
{
	return (a <= b ? a : b);
}


//// Return random between 0 and range-1
//int
//ARandom(int range)
//{
//	return rand() % range;
//}
//
//
//// Change random seed
//void
//ASeedRandom(uint32 newSeed)
//{
//	srand(newSeed); 
//}


// Return the length of a file in bytes
uint32
AFileLength(FILE* fp)
{
	// ANSI compatible filelength function
	long		origPos;
	uint32	ret;

	origPos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	ret = ftell(fp);
	fseek(fp, origPos, SEEK_SET);

	return ret;
}




// Don't mess with these numbers!
// They are the result of research and work just fine!
// (From October 1988 Volume 31, Number 10 - Communications of the ACM,
//  "Random Number Generators: Good Ones Are Hard To Find" by
//  Stephen K. Park and Keith W. Miller)
const int32 TRAND_A = 16807;
const int32 TRAND_M = 0x7fffffff;
const int32 TRAND_Q = (TRAND_M / TRAND_A);
const int32 TRAND_R = (TRAND_M % TRAND_A);

TRandomGenerator::TRandomGenerator()
{
	seed = 1;
}

bool
TRandomGenerator::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&TRandomGeneratorDataStart, 1, (int)&TRandomGeneratorDataEnd -
				(int)&TRandomGeneratorDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&TRandomGeneratorDataStart, 1, (int)&TRandomGeneratorDataEnd -
				(int)&TRandomGeneratorDataStart, pFile);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

int
TRandomGenerator::GetSeed(void)
{
	return(seed);
}

void
TRandomGenerator::ReSeed (int newSeed)
{
	if (newSeed > 0 &&
	    newSeed < 0x7fffffff)
	{
		// within a valid range		
		seed = newSeed;
	}
}

int
TRandomGenerator::GetNumber (int range) 
{
	int32	low;
	int32	high;
	int32	test;

	high = seed / TRAND_Q;
	low = seed % TRAND_Q;
	test = TRAND_A * low - TRAND_R * high;
	if (test > 0)
	{
		seed = test;
	}
	else
	{
		seed = test + TRAND_M;
	}

	return seed % range;
}
		

