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

#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <SDL3/SDL.h>

FILE* ci_fopen(const char* path, const char* mode)
{
	FILE* fp = fopen(path, mode);
	if (fp)
		return fp;

	/* Only do the case-insensitive scan for read modes — writes need the
	 * exact name and should not silently redirect to an existing uppercase file. */
	if (mode[0] != 'r')
		return 0;

	/* Split path into directory and filename components. */
	const char* slash = strrchr(path, '/');
	const char* fname = slash ? slash + 1 : path;

	char dir[512];
	if (slash) {
		size_t dlen = (size_t)(slash - path);
		if (dlen >= sizeof(dir) - 1) return 0;
		memcpy(dir, path, dlen);
		dir[dlen] = '\0';
	} else {
		strcpy(dir, ".");
	}

	DIR* d = opendir(dir);
	if (!d) return 0;

	char found[512];
	found[0] = '\0';
	struct dirent* entry;
	while ((entry = readdir(d))) {
		if (strcasecmp(entry->d_name, fname) == 0) {
			if (slash)
				snprintf(found, sizeof(found), "%s/%s", dir, entry->d_name);
			else
				snprintf(found, sizeof(found), "%s", entry->d_name);
			break;
		}
	}
	closedir(d);

	return found[0] ? fopen(found, mode) : nullptr;
}

/* Case-insensitive SDL_IOFromFile: same scan logic as ci_fopen but returns
 * SDL_IOStream*. Used by TFile::Open() so resource files are found regardless
 * of case on the Linux filesystem. */
static SDL_IOStream* ci_io_open(const char* path, const char* mode)
{
	SDL_IOStream* io = SDL_IOFromFile(path, mode);
	if (io)
		return io;

	if (mode[0] != 'r')
		return nullptr;

	const char* slash = strrchr(path, '/');
	const char* fname = slash ? slash + 1 : path;

	char dir[512];
	if (slash) {
		size_t dlen = (size_t)(slash - path);
		if (dlen >= sizeof(dir) - 1) return nullptr;
		memcpy(dir, path, dlen);
		dir[dlen] = '\0';
	} else {
		strcpy(dir, ".");
	}

	DIR* d = opendir(dir);
	if (!d) return nullptr;

	char found[512];
	found[0] = '\0';
	struct dirent* entry;
	while ((entry = readdir(d))) {
		if (strcasecmp(entry->d_name, fname) == 0) {
			if (slash)
				snprintf(found, sizeof(found), "%s/%s", dir, entry->d_name);
			else
				snprintf(found, sizeof(found), "%s", entry->d_name);
			break;
		}
	}
	closedir(d);

	return found[0] ? SDL_IOFromFile(found, mode) : nullptr;
}

const char* get_pref_dir()
{
	/* SDL_GetPrefPath creates the directory automatically on first call.
	 * Returns ~/.local/share/TachyonStudios/BloodAndMagic/ on Linux. */
	static char* dir = SDL_GetPrefPath("TachyonStudios", "BloodAndMagic");
	return dir;
}

SDL_IOStream* prefs_fopen(const char* filename, const char* mode)
{
	char path[512];
	snprintf(path, sizeof(path), "%s%s", get_pref_dir(), filename);
	return SDL_IOFromFile(path, mode);
}



//-------------------------------------------------------------------

TFile::TFile ()
{
	_fp = nullptr;
	szFileName[0] = '\0';
}

TFile::~TFile ()
{
	if (_fp)	Close ();
}



//-------------------------------------------------------------------

int
TFile::Open (const char* filename, const char* szAccess, int fatal)
{
	strcpy(szFileName, filename);
	_fp = ci_io_open(filename, szAccess);
	if (!_fp)
	{
		if (fatal)
		{
			char err[256];

			snprintf(err, sizeof(err), "Unable to open file: %s as '%s'\n", filename, szAccess);
			if(pMono)
				pMono->Out(err);
			APanic (err);
		}
		else
		{
			return false;
		}
	}

	return true;
}


void
TFile::Close ()
{
	SDL_CloseIO(_fp);
	_fp = nullptr;
}




//-------------------------------------------------------------------

long
TFile::Size ()
{
	return (long) SDL_GetIOSize(_fp);
}


int
TFile::Getch ()
{
	uint8_t b;
	if (SDL_ReadIO(_fp, &b, 1) != 1)
		return EOF;
	return (int)b;
}


long
TFile::Read (void *buffer, long size, long num)
{
	size_t bytes_to_read = (size_t)(size * num);
	if (bytes_to_read == 0)
		return 0;
	size_t bytes = SDL_ReadIO(_fp, buffer, bytes_to_read);
	if (bytes != bytes_to_read)
	{
		char	err[256];

		snprintf(err, sizeof(err), "TFile(%s)::Read(%p, %ld, %ld) - SDL_ReadIO() error\n",
			szFileName, buffer, size, num);
		if(pMono)
			pMono->Out(err);
		APanic(err);
	}
	return num;
}


long
TFile::Write (void *buffer, long size, long num)
{
	size_t bytes = SDL_WriteIO(_fp, buffer, (size_t)(size * num));
	return (size > 0) ? (long)(bytes / (size_t)size) : 0;
}


int
TFile::Seek (long offset, int whence)
{
	Sint64 result = SDL_SeekIO(_fp, offset, (SDL_IOWhence)whence);
	if (result < 0)
	{
		char	err[256];

		snprintf(err, sizeof(err), "TFile(%s)::Seek(%ld, %d) - SDL_SeekIO() error\n",
			szFileName, offset, whence);
		if(pMono)
			pMono->Out(err);
		APanic(err);
	}
	return 0;
}
