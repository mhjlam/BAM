//
// TYPES.HPP
//
// September 22, 1994
// (C) 1993, Tachyon, Inc.  All rights reserved.
//
//	Type definitions.
//
//----[]-------------------------------------------------------------


#ifndef types_hpp
#define types_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "res_cfg.hpp"

// Define a global platform definition
#if defined(_WINDOWS) || defined(_WINDOWS_)
	#define	OS_WINDOWS
#else
	#define OS_DOS
#endif

// Turn off some warnings that we know are acceptable
#pragma warning 549 9;	// sizeof operand contains compiler generated info
#pragma warning 687 9;	// expression for while is always true
#pragma warning 688 9;	// testing expression for 'for' is always true
#pragma warning 368 9;	// conditional expression in if statement is always false 
#pragma warning 13 9;	// unreachable code

// Standard typedefs

typedef char				int8;
typedef unsigned char	uint8;
typedef short				int16;
typedef unsigned short 	uint16;
typedef int					int32;
typedef unsigned int		uint;
typedef unsigned int		uint32;
typedef unsigned long	ticks_t;

typedef uint				grip;
typedef void*				dgrip;
typedef unsigned char	uchar;
typedef int					coord;


// Dialog draw to dynamic cel procedure ptr
typedef	void	(*DrawProc)(grip gDynCel);


#if !TRUE
	enum bool
	{
		FALSE, TRUE
	};
#endif


// Message & Notice Types
typedef uint16	evt_t;

enum notice_t
{
	N_NULL,
	N_CUE,					// tell script to advance state
	N_SETUP,
	N_QUIT,					// tell context to quit
	N_CLICK,					// click messages for responders
	N_DO_CONV_EXCHANGE,	
	N_NEXT_CONV_EXCHANGE,
	N_CHANGE_LOCATION,
	N_TIMER_FIRED,
	N_PAUSE_CLOCK,
	N_LAST
};

enum msg_t
{
	MSG_NULL,
	MSG_EVENT,
	MSG_NOTICE,
	MSG_ABORTED
};


// Text justification
enum justify
{
	DG_JUST_LEFT,
	DG_JUST_CENTER,
	DG_JUST_RIGHT,
	DG_JUST_TOP,
	DG_JUST_BOTTOM
};


// Control types
enum ctrl_t
{
	CTRL_BUTTON,		// Standard button
	CTRL_TEXT,			// Edit field
	CTRL_LABEL,			// Non-editable text
	CTRL_RBUTTON,		// Radio button (mutually exclusive)
	CTRL_CHKBOX,		// Check box
	CTRL_LISTBOX		// List box, pick list, etc
};


// "Modality", "Modalnes", "Modalage", "Modalocity", et al.
enum modalType
{
	M_INACTIVE,
	M_MODELESS,
	M_SEMI,
	M_MODAL
};


// Graphics

// Used in VGABuffer, bitmap_t is a platform specific reference to
// a pixel map.
typedef	grip			bitmap_t;


#endif

