//
// APIDLG.HPP
//
// November 12, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// API calls specific to dialogs.  Also common dialog routines.
//
//----[]-------------------------------------------------------------


#ifndef	apidlg_hpp
#define	apidlg_hpp


#include "dialog.hpp"
#include "types.hpp"


// Error handling
bool		AAlert(char* buffer);

// Basic user i/o functions
bool		AConfirm(char* szPrompt);
char*		AGetString(char* szPrompt, char* szDefault, char* buffer, int size);
int32		AGetNumber(char* szPrompt, int32 dDefault);


#endif
