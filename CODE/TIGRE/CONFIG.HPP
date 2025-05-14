//
// CONFIG.HPP
//
// February 8, 1994
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Configuration class definition.
//
//		The Configuration class handles reading game and hardware
//		configurations from a disk file.
//
//----[]-------------------------------------------------------------


#ifndef	config_hpp
#define	config_hpp


#include "tigre.hpp"


#define	CFG_TOKEN_MAX 	40		// Maximum length of tokens
#define	CFG_ENTRY_MAX	30		// Maximum entries (lines) per config (file)
#define	CFG_PARMS_MAX	10		// Maximum parameters per line (after subject)

typedef	char	szEntry[CFG_TOKEN_MAX];


//	Configuration manager class declaration
class Configuration 
{
	public:
		Configuration(uint16 sl = 0, uint16 el = 999, uint16 np = 1);
		Configuration(char* szCfgFileName, uint16 sl = 0, uint16 el = 999, uint16 np = 1);

		uint16	startLine;	// File line # to start reading
		uint16	endLine;		// File line # to end reading
		uint16	numParams;	// Number of parameters to parse per line

		bool		Load(char* szCfgFileName);

		int16		HasEntry(char* szSubject);
		bool		HasParam(uint16 sub, char* szParam);
		char*		CopyStr(char* szDest, char* szSub, uint16 param = 0);
		char*		CopyStr(char* szDest, uint16 sub, uint16 param = 0);
		int32		GetNum(char* szSubject);
		int16		FindSubject(char* szParam, uint16 param);

		void		SetStr(char* szSubject, char* szParam, uint16 param = 0);
		void		WriteConfig(char*	szFileName);


	protected:

		// arrays of char arrays
		char		szSubjects[CFG_ENTRY_MAX][CFG_TOKEN_MAX];
		char		szParams[CFG_ENTRY_MAX][CFG_PARMS_MAX][CFG_TOKEN_MAX];
};



#endif

