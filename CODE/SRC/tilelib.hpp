//	BAM Tile Librarian header file
#include "modex.hpp"

#ifndef tilelib_hpp
#define tilelib_hpp

// X-MODE
#define	GRAPHICS_MODE		MODEX_320X400
#define	SCREEN_WIDTH		320
#define	SCREEN_HEIGHT		400


// HIGH-RES
//#define	GRAPHICS_MODE		0
//#define	SCREEN_WIDTH		640
//#define	SCREEN_HEIGHT		400
//#define	TILE_WIDTH			40
//#define	TILE_HEIGHT			38
//#define	LBM_WIDE				640
//#define	LBM_HIGH				400



//-----------------------------------------------
enum blit_type				// copy directions for BlitCopy (only matters when Src && Dest overlap)
{
	BLIT_LR,
	BLIT_RL,
	BLIT_TB,
	BLIT_BT
};
//bool 		OldBlitCopy(uchar *Dest, uint16 DestWidth, uchar *Src, uint16 SrcWidth, uint16 CopyWidth, uint16 CopyHeight, uint16 SrcXOff, uint16 SrcYOff);
bool 		BlitCopy(uchar *Dest, uint16 DestWidth, uchar *Src, uint16 SrcWidth, uint16 CopyWidth, uint16 CopyHeight, uint16 SrcXOff, uint16 SrcYOff, blit_type xDir = BLIT_LR, blit_type yDir = BLIT_TB);
uint16	CountFreeGrips(void);

#endif

