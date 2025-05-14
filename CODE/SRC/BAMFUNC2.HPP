// BAMFUNC2.CPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
// Global functions for BAM and MAPEDIT
//
//	08-10-95
//

#ifndef BAMFUNCS2_HPP
#define BAMFUNCS2_HPP

#include "types.hpp"
#include "writeres.hpp"
#include "rect.hpp"

void	CopyCel(CelHeader *pDest, int32 destX, int32 destY,
			res_t resType, uint resNum, uint32 celNum, bool fHasSkip, grip gNewClut = NULL, bool fUseOrgs = FALSE);
void	CopyCel(CelHeader *pDest, int32 destX, int32 destY,
			res_t resType, uint resNum, uint32 celNum, Rectangle *rSrc, bool fHasSkip, grip gNewClut = NULL);
void	CopyCel(CelHeader *pDest, int32 destX, int32 destY,
			res_t resType, uint resNum, uint32 celNum, uint32 srcX,
			uint32 srcY, uint32 srcX2, uint32 srcY2, bool fHasSkip, grip gNewClut = NULL, bool fUseOrgs = FALSE);
void	SetFontColors(int newBack, int newFore1, int newFore2);
void	SetFontColors(int32 newBack, int32 newFore1, int32 newFore2, int32 newFore3,
						  int32 newFore4);
void	SetFontColors(int32 newBack, int32 newFore1, int32 newFore2, int32 newFore3,
						  int32 newFore4, int32 newFore5, int32 newFore6);

#endif
