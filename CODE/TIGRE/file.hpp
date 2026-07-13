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


#include <stdio.h>
#include <SDL3/SDL.h>

/* Case-insensitive fopen for Linux: tries the path as-is first, then scans
 * the directory for a case-insensitive filename match.  Safe for write modes
 * (falls straight through to fopen on failure without scanning). */
FILE* ci_fopen(const char* path, const char* mode);

/* Returns the platform-specific user preferences directory
 * (~/.local/share/TachyonStudios/BloodAndMagic/ on Linux).
 * The directory is created automatically on first call. */
const char* get_pref_dir();

/* Opens filename inside the user preferences directory.
 * Used for all user-data files (saves, config, etc.) so they are
 * isolated from the read-only game data directory.
 * Returns SDL_IOStream* (call SDL_CloseIO when done). */
SDL_IOStream* prefs_fopen(const char* filename, const char* mode);

//-------------------------------------------------------------------

class	TFile
{
	public:
		TFile();
		~TFile();

		int	Open (const char* filename, const char* szAccess, int fatal = true);
		int	IsOpen () { return _fp != nullptr; }
		void	Close ();

		long	Size ();

		int 	Getch ();
		long 	Read (void *buffer, long size, long num);
		long	Write (void *buffer, long size, long num);
		int	Seek (long offset, int whence);

	protected:
		SDL_IOStream*	_fp;
		char		szFileName[80];
};




#endif
