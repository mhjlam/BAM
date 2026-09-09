//
// File.hpp
//
// 18 July, 1995
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	
//		File wrapper class
//
//-------------------------------------------------------------------


#ifndef	file_hpp
#define	file_hpp


#include	<stdio.h>

//-------------------------------------------------------------------

class	TFile
{
	public:
		TFile();
		~TFile();

		int	Open (char* filename, char* szAccess, int fatal = TRUE);
		int	IsOpen () { return (int)_fp; }
		void	Close ();

		long	Size ();

		int 	Getch ();
		long 	Read (void *buffer, long size, long num);
		long	Write (void *buffer, long size, long num);
		int	Seek (long offset, int whence);

	protected:
		FILE*		_fp;
		char		szFileName[80];
};




#endif
