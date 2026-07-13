//
// SCONFIG.HPP
//
// March 15, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Sound Configuration class definition.
//
//		The Configuration class handles reading game and hardware
//		configurations from a disk file.
//
//----[]-------------------------------------------------------------

#ifndef	sconfig_hpp
#define	sconfig_hpp

#include "types.hpp"

#define	SCONFIG_FNAME_MAX			50
#define	MAX_CONFIG_LINE_SIZE		140
#define	MAX_CONFIG_TOKEN_SIZE	100

class SConfig
{
	public:
		char		configFileName[SCONFIG_FNAME_MAX + 1];
		bool		fileLoaded;

		grip		gConfigArray;

		uint16	maxSize;
		uint16	curSize;

		uint		configEntryStartOffset;

					SConfig();
					~SConfig();

		char*		GetEntry(const char *pConfigName, char *pConfigEntry);
		int		GetEntryDecimal(const char *pConfigName, char *pConfigEntry);
		void		SetEntry(const char *pConfigName, const char *pConfigEntry);
		void		DeleteEntry(const char *pConfigName);

		void		SetConfigFile(const char *fileName);
		void		ResetConfigFile();
		bool		LoadConfig(const char *fileName, bool exitOnError = true);
		bool		LoadConfig(bool exitOnError = true);
		void		SaveConfig();
		void		SaveConfig(char *fileName);

	private:

		void		ReSizeArray(int deltaSize);
		char*		FindToken(char *charString, char *pToken);
		int16		FindEntry(const char *pConfigName, char *pConfigEntry);
};

int	atoi(void *str, int base);

#endif

