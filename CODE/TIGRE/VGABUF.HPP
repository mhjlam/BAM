//
// VGABUF.HPP
//
// December 28, 1993
// Brian K. Hughes
// WATCOM: September 28, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	VGA Buffers are used to hold "work in progress" while calculating
// cels to be displayed.  A cel is clipped, un-RLE'd into a VGABuf,
// then blitted to visual.
//
//----[]-------------------------------------------------------------

#ifndef	vgabuf_hpp
#define	vgabuf_hpp

#include "rect.hpp"
#include "types.hpp"


//----------------------------------------------------------
//

class VGABuffer : public Object
{
	public:
		coord		x;
		coord		y;
		uint		width;
		uint		height;
		uint	bufWidth;	//normally protected -but needed by shortcut in mouseint.cpp
		uint	bufHeight;
		size_t	size;

		bitmap_t	offscreenBMap;

		VGABuffer(uint w, uint h, coord newX = 0, coord newY = 0);
		VGABuffer(void);
		~VGABuffer();

		void	Construct(uint w, uint h, coord newX = 0, coord newY = 0);
		void	Blit(bitmap_t gBitMapData=0);
		void	Load(Rectangle* r, coord celX, coord celY, grip gRes, uint cel, Scrimage* pScrim = NULL, uint flags = 0, bool fIgnoreSkip = TRUE, uint scale = 256);
		void	SetOrigin(coord newX, coord newY);
		uchar	GetPixel(Rectangle* pR, coord celXOff, coord celYOff, grip gRes, uint cel, Scrimage* pScrim);
		void	ReleaseBuffer(void);

		uchar	*GetBuffer();

		uchar*	GetResUnitData(grip gRes, uint celNum = 0);
		void		GetCelRect(grip gRes, uint celNum, Rectangle* pR);
};




// Scaling is the domain of the vgabuf, since the same logic most be used
// when scales coords as when scaling images.

#define	ScaleCoord(dim, scale)		((coord) (((dim) * (scale)) >> 8))
#define	UnScaleCoord(dim, scale)	((coord) (((dim) << 8) / (scale)))


//	Scaling is talkeed about in terms of percentage, (1/100), but
//	is calculated in code as (1/256), here are coversion macros

#define	PercentToScale(percent)	((uint16) ((percent * 256) / 100))
#define	ScaleToPercent(scale)	((uint16) ((scale   * 100) / 256))


#endif
