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
#define	MODEX_320X400	1
#define	MODEX_320X200	2

extern "C"
{
	extern	uchar		*pVGAMem;
	extern	uchar		*pVGAMemPage0;
	extern	uchar		*pVGAMemPage1;
	void	SetXMode(void);
	void	XModeFlipPage(void);
	void	XModeUploadUILayer(const uint8_t* indexed);
	void	FillScreen(int penNum);
}

/* Poll SDL events and feed them into the TIGRE event/mouse system.
 * Call once per game cycle, before pEventMgr->PublishNext(). */
void ASDLPumpEvents(void);

/* World tile texture cache — GPU-resident RGB24 texture covering the full world map.
 * Background tiles are baked here and rendered underneath the sprite layer each frame.
 * Tile pixel dimensions: TILE_WIDTH × TILE_HEIGHT = 20 × 38. */
void OS_WorldTextureCreate(int w_px, int h_px);
void OS_WorldTextureDestroy(void);
void OS_WorldTextureUpdateTile(int wx, int wy, const uint8_t* indexed_20x38, const uint8_t* clut_or_null);
void OS_WorldTextureSetView(float world_x, float world_y, float game_x, float game_y, float w, float h);
bool OS_WorldTexturePaletteDirty(void);

#endif
