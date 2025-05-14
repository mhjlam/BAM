//
// API.HPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Definitions for API calls.
//
//----[]-------------------------------------------------------------


#ifndef	api_hpp
#define	api_hpp


#include "types.hpp"
#include "os.hpp"


// Error Handling
void		_Panic(char* msg, char* fileName, int lineNum);

#ifdef NDEBUG
	#define ASSERT(ignore)	((void) 0)
#else
	//#undef assert
	//#define assert(e)	((void)((e) || (_Panic(#e,__FILE__,__LINE__),1)))
	// our own version of assert to be sure to use _Panic
	#define ASSERT(e)	((void)((e) || (_Panic(#e,__FILE__,__LINE__),1)))
#endif

#define	APanic(msg)	( (_Panic(msg,__FILE__,__LINE__)))
#define	fOpenPrefs(x,y)	(fopen(x, y))
extern	char*	szLastMessage;


// Miscellaneous Functions
uint		AMax(int a, int b);
uint		AMin(int a, int b);
//int		ARandom(int range);
//void		ASeedRandom(uint32 newSeed);
uint32	AFileLength(FILE* fp);
uint32	AFileLastAccessTime (char* fn);
uint32	AFileLastModTime (char* fn);

void 		AInitializePlatform();


// QSort
#undef	DEFINING_OUR_OWN_SORT
#ifdef	DEFINING_OUR_OWN_SORT
	typedef int	(CompProc)(const void* pElem1, const void* pElem2);
	void		OS_qsort(void* base, size_t n, size_t size, CompProc *ppCmp);
#else
	#define	OS_qsort(base,n,size,ppCmp)	qsort(base,n,size,ppCmp)
#endif


class	TRandomGenerator
{
	public:

		TRandomGenerator ();

		void	ReSeed (int newSeed);
		int	GetSeed(void);
		int	GetNumber (int range);
		bool	Save(uint16 state, FILE *pFile = NULL);


	protected:
		char	TRandomGeneratorDataStart;
		int32	seed;
		char	TRandomGeneratorDataEnd;
};

#endif
