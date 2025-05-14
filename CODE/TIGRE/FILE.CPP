//
// File.cpp
//
// 18 July, 1995
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	
//		File wrapper class
//
//-------------------------------------------------------------------



#include "types.hpp"
#include "api.hpp"
#include "file.hpp"
#include "mono.hpp"



//-------------------------------------------------------------------

TFile::TFile ()
{
	_fp = NULL;
	szFileName[0] = NULL;
}

TFile::~TFile ()
{
	if (_fp)	Close ();
}



//-------------------------------------------------------------------

int
TFile::Open (char* filename, char* szAccess, int fatal) 
{
	strcpy(szFileName, filename);
	_fp = fopen (filename, szAccess);
	if (!_fp)
	{
		if (fatal)
		{
			char err[256];

			sprintf(err, "Unable to open file: %s as '%s'\n", filename, szAccess);
			if(pMono)
				pMono->Out(err);
			APanic (err);
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}


void
TFile::Close () 
{
	fclose (_fp);
	_fp = NULL;
}




//-------------------------------------------------------------------

long
TFile::Size () 
{
	return (long) AFileLength (_fp);
}


int 
TFile::Getch () 
{
	return (fgetc (_fp));
}


long 
TFile::Read (void *buffer, long size, long num) 
{ 
	size_t	x;
	x = fread (buffer, size, num, _fp);
	if (x != num)
	{
		char	err[256];

		sprintf(err, "TFile(%s)::Read(%x, %ld, %ld) - fread() error\n",
			szFileName, buffer, size, num);
		if(pMono)
			pMono->Out(err);
		APanic(err);
	}
	return x;
}


long
TFile::Write (void *buffer, long size, long num) 
{
	return (fwrite (buffer, size, num, _fp));
}


int
TFile::Seek (long offset, int whence) 
{
	int	err;
	err = fseek (_fp, offset, whence);
	if(err)
	{
		char	err[256];

		sprintf(err, "TFile(%s)::Seek(%ld, %d) - fseek() error\n",
			szFileName, offset, whence);
		if(pMono)
			pMono->Out(err);
		APanic(err);
	};
	return (fseek (_fp, offset, whence));
}


