//
// MODEX.HPP
//
// November, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// C definitions of modex variables and prototypes
//
//----[]-------------------------------------------------------------


#ifndef	ModeX_hpp
#define	ModeX_hpp

#include "tigre.hpp"

#define	SCREEN_WIDTH					320
#define	SCREEN_HEIGHT					400
#define	X_MODE_PAGE_MASK				0x8000
#define	X_MODE_PAGE_0					0xA0000
#define	X_MODE_PAGE_1					0xA8000

#define	MODEX_320X400	1
#define	MODEX_320X200	2

extern "C"
{
	extern	uchar		*pVGAMem;
	extern	uchar		*pVGAMemPage0;
	extern	uchar		*pVGAMemPage1;
	extern	uint16	curDrawPage;

	void	SetXMode(void);
	void	XModeFlipPage(void);
	void	FillScreen(int penNum);
	void	SyncWithVBL();
}

#endif
