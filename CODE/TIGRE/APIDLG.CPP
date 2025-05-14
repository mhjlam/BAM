//
// APIDLG.CPP
//
// November 12, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// API calls specific to dialogs.  Also common dialog routines.
//
//----[]-------------------------------------------------------------


#include "apidlg.hpp"
#include "apimem.hpp"
#include "dialog.hpp"
#include "types.hpp"


bool
AAlert(char* msg)
{
	#define		CONFIRMED	1
	Dialog		d;
	TControlInfo	ci;

	d.Initialize(0, 0, DG_DFT_WINDOW);

	ci.x = 5;
	ci.y = 5;
	ci.cap = msg;
	d.AddLabel(&ci);

	ci.y = 25;
	ci.cap = "Ok";
	ci.value = CONFIRMED;
	ci.key = K_Y;
	ci.exit = TRUE;
	d.AddButton(&ci);

	d.Activate(M_MODAL);
	d.JustifyWindow(DG_JUST_CENTER, DG_JUST_CENTER);
	return (d.EventLoop() == CONFIRMED) ? TRUE : FALSE;
}




//---------------------------------------------------------------
// Basic user i/o functions
//---------------------------------------------------------------

#ifndef	NDEBUG

// Return user's response to prompt
bool
AConfirm(char* szPrompt)
{
	#define		CONFIRMED	1
	#define		DENIED		(-1)
	Dialog		d;
	TControlInfo	ci;

	d.Initialize(0, 0, DG_DFT_WINDOW);

	ci.x = 5;
	ci.y = 5;
	ci.cap = szPrompt;
	d.AddLabel(&ci);

	ci.x = 55;
	ci.y = 25;
	ci.cap = "No";
	ci.value = DENIED;
	ci.key = K_N;
	ci.exit = TRUE;
	d.AddButton(&ci);

	ci.x = 5;
	ci.cap = "Yes";
	ci.value = CONFIRMED;
	ci.key = K_Y;
	d.AddButton(&ci);

	d.Activate(M_MODAL);
	d.JustifyWindow(DG_JUST_CENTER, DG_JUST_CENTER);
	return (d.EventLoop() == CONFIRMED) ? TRUE : FALSE;
}


// Return user's response to prompt
char*
AGetString(char* szPrompt, char* szDefault, char* buffer, int size)
{
	Dialog		d;
	grip			gTextBox;
	TTextBox*	pTextBox;
	TControlInfo	ci;

	d.Initialize(0, 0, DG_DFT_WINDOW);

	ci.x = 5;
	ci.y = 5;
	ci.cap = szPrompt;
	d.AddLabel(&ci);

	ci.x = 10;
	ci.y = 25;
	ci.cap = szDefault;
	ci.len = 20;
	gTextBox = d.AddTextBox(&ci);
	pTextBox = ADerefAs(TTextBox, gTextBox);
	
	d.Activate(M_MODAL);
	d.JustifyWindow(DG_JUST_CENTER, DG_JUST_CENTER);
	pTextBox->Select();

	// get string from edit box
	strncpy(buffer, pTextBox->editLine, size);
	buffer[size - 1] = NULL;

	d.Deactivate();
	return buffer;
}


// Return user's response to prompt
int32
AGetNumber(char* szPrompt, int32 dDefault)
{
	Dialog		d;
	grip			gTextBox;
	TTextBox*	pTextBox;
	int			ret;
	char			sz[40];
	TControlInfo	ci;

	d.Initialize(0, 0, DG_DFT_WINDOW);

	ci.x = 5;
	ci.y = 5;
	ci.cap = szPrompt;
	d.AddLabel(&ci);

	ci.x = 10;
	ci.y = 25;
	ci.cap = itoa(dDefault, sz, 10);
	ci.len = 20;
	gTextBox = d.AddTextBox(&ci);
	pTextBox = ADerefAs(TTextBox, gTextBox);
	
	d.Activate(M_MODAL);
	d.JustifyWindow(DG_JUST_CENTER, DG_JUST_CENTER);
	pTextBox->Select();

	// get string from edit box
	ret = atoi(pTextBox->editLine);

	d.Deactivate();
	return (int32) ret;
}

#endif
