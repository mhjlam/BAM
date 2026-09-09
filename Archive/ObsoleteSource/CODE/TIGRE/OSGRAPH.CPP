//
//	OSGRAPH.CPP
//
// March 2, 1994
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	DOS Platform specific functions relating to the graphics system
//
//----[]-------------------------------------------------------------


#include <stddef.h>
#include <i86.h>

#include "api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "graphmgr.hpp"
#include "os.hpp"
#include "osgraph.hpp"
#include "rle.hpp"
//#include "vesa.hpp"


void
OS_InitializePlatform()
{
	// just a stub. This is mostly for the MAC
}



//-----------------------------------------------------------------
//	VGABuffer is a multi-platform structure which uses
// bitmap_t, which is platform specific.  In Tigre/DOS
//	bitmap_t is a grip to an array of uchars.
//-----------------------------------------------------------------

bitmap_t
OS_CreateBitMap(coord w, coord h)
{
	TRACK_MEM("OS_CreateBitmap");
	return AMalloc(w * h);
}


void
OS_DestroyBitMap(bitmap_t bm)
{
	if (bm)
	{
		AFree(bm);
	}
}

uchar*
OS_DerefBitMap(bitmap_t bm)
{
	return ADerefAs(uchar, bm);
}



//---[ Platform-specific graphics functions ]------------------------

// Set up the graphics system & set the appropriate data in the graph manager
//
int
OS_InitGraphics(GraphicsMgr* pGM, uint mode)
{
	if (ADetectVideo())
	{
		// we have a vga card
		pGM->vgaDriver = mode;

		if (AInitVideo(pGM->vgaDriver, mode, pGM->videoBufferSeg,
						pGM->maxX, pGM->maxY, pGM->vgaBankNoIncrement,
						pGM->vidMode) == 0)
		{
			// video mode didn't set
			APanic("Can't init video.");
		}

		--pGM->maxX;
		--pGM->maxY;

		// install shutdown function
		atexit(AShutDownVideo);
	}
	else
	{
		APanic("VGA card not found.");
	}

	return mode;
}


//---------------------------------------------------------------------------
//
// The following commented code is the remainder of the old OS_Blit function.
// It will be used to construct the Video7Blit assembly routine, if required.
//
//---------------------------------------------------------------------------
//
// blit vgabuf data to screen
//void
//OS_Blit(void* pv, bitmap_t gBitmapData)
//{
//	pv = pv;							// DEBUG
//	gBitmapData = gBitmapData;	// DEBUG
//
//	#define	BANK_DATA_SIZE				8
//	#define	EXT_CTRL_ADDRESS_REG		0x03C4
//	#define	EXT_CTRL_DATA_REG			0x03C5
//	#define	EXTENDED_PAGE_SELECT		0x00F9
//	#define	MISC_OUTPUT_REG_READ		0x03CC
//	#define	MISC_OUTPUT_REG_WRITE	0x03C2
//	#define	BANK_SELECT					0x00F6
//
//	#define ExtensionsOff()
//		"mov	ax,00AE06h" 					\
//		"mov	dx,EXT_CTRL_ADDRESS_REG"	\
//		"out	dx,ax"
//
//	#define ExtensionsOn()
//		"mov	ax,00EA06h"	 					\
//		"mov	dx,EXT_CTRL_ADDRESS_REG"	\
//		"out	dx,ax"
//
//	#define V7SetBankNumber()
//		"ExtensionsOn()"						\
//		"mov	ah,cl"							\
//		"and	ah,001h"							\
//		"mov	al,EXTENDED_PAGE_SELECT"	\
//		"mov	dx,EXT_CTRL_ADDRESS_REG"	\
//		"out	dx,ax"							\
//		"mov	dx,MISC_OUTPUT_REG_READ"	\
//		"in	al,dx"							\
//		"mov	ah,cl"							\
//		"and	ah,002h"							\
//		"shl	ah,4"								\
//		"and	al,0DFh"							\
//		"or	al,ah"							\
//		"mov	dx,MISC_OUTPUT_REG_WRITE"	\
//		"out	dx,al"							\
//		"mov	al,BANK_SELECT"				\
//		"mov	dx,EXT_CTRL_ADDRESS_REG"	\
//		"out	dx,al"							\
//		"inc	dx"								\
//		"in	al,dx"							\
//		"and	al,0F0h"							\
//		"and	cl,00Ch"							\
//		"or	al,cl"							\
//		"shr	cl,2"								\
//		"or	al,cl"							\
//		"out	dx,al"							\
//		"ExtensionsOff()"
//
//
//	// convert passed-in void ptr to vgabuffer ptr
//	VGABuffer*	pVBuf = (VGABuffer*) pv;
//
//	uchar*	pData;
//	uint16	eSRegister;
//	uint32	vBITPtr;
//	uint16	widthInThisBank;
//	uchar		bankNo;
//	uint16	bufWidth = pVBuf->width;
//	uint16	bufHeight = pVBuf->height;
//	uint32	bufX = pVBuf->x;
//	uint32	bufY = pVBuf->y;
//	uint16	vBufSeg = pGraphMgr->videoBufferSeg;
//	uint16	bankNoIncrement = pGraphMgr->vgaBankNoIncrement;
//
//	if (!gBitmapData)
//	{
//		// get default data
//		pData = pVBuf->GetBuffer();
//	}
//	else
//	{
//		// get passed in buffer
//		pData = OS_DerefBitMap(gBitmapData);
//	}
//
//	asm
//	{
//		mov	eSRegister,es					//Save ES register
//		mov	es,vBufSeg						//ES->video buffer segment
//	}
//
//	switch(pGraphMgr->vgaDriver)
//	{
//	case DRV_VIDEO7:
//		asm
//		{
//			mov	esi,pData					//ESI->blit source buffer
//			lea	ebx,bankData				//EBX->video bank info table
//			mov	eax,bufY						//EAX=rectangle's first line Y coordinate
//			shl	eax,3							//EAX=offset to first line's bank info entry
//			add	ebx,eax						//EBX->first line's bank info entry
//
//			movzx	ecx,[ebx].bankNumber		//ECX=bank number	of first line
//
//			V7SetBankNumber()					//Set bank number of first line
//
//			movzx	edx,bufHeight				//EDX=rectangle's height in lines
//
//		V7NextLine:
//			movzx	edi,[ebx].yOffset			//EDI=line's offset into bank
//			add	edi,bufX						//EDI=line's first pixel offset into bank
//			movzx	ecx,[ebx].transitionX	//ECX=bank transition's X coordinate (0 indicates no transition)
//			or		ecx,ecx						//Is there a bank transition in this line?
//			jnz	V7BankTransitionLine		//Yes
//
//			mov	eax,edi						//EAX=line's first pixel's offset into buffer
//			and	eax,3							//Is pixel on a double word boundary?
//			jz		V7SetDWordWidth			//Yes
//
//			mov	ecx,4							//ECX=number of pixel's per double word
//			sub	ecx,eax					   //ECX=offset to next double word
//			mov	eax,ecx						//EAX=offset to next double word
//			cmp	ax,bufWidth					//Is next double word outside of rectangle?
//			jb		V7AlignOnDWord				//No
//
//			movzx	ecx,bufWidth				//ECX=rectangle's width in pixels 
//			mov	eax,ecx						//EAX=rectangle's width	in pixels
//
//		V7AlignOnDWord:
//			rep	movsb							//Draw pixel by pixel to next double word
//
//		V7SetDWordWidth:
//			movzx	ecx,bufWidth				//ECX=rectangle's width in pixels
//			sub	ecx,eax					  	//ECX=rectangle's remaining width in pixels
//			mov	eax,ecx						//EAX=rectangle's remaining width in pixels
//			shr	ecx,2							//ECX=rectangle's remaining width in double words
//			rep	movsd							//Draw 4 pixels at a time
//			mov	ecx,eax						//ECX=rectangle's remaining width prior to double word draw
//			and	ecx,3							//ECX=rectangle's remaining width in pixels
//			rep	movsb							//Draw remaining pixels pixel by pixel
//
//		V7EndOfLine:
//			add	ebx,BANK_DATA_SIZE		//EBX->next line's bank info entry
//			dec	edx							//Are there more lines to blit?
//			jnz	V7NextLine					//Yes
//
//			jmp	V7BRTSExit					//Blit of rectangle is complete
//
//		V7BankTransitionLine:
//			mov	widthInThisBank,0			//Default to line not in current bank
//			mov	eax,ecx						//EAX=X coordinate of transition point
//			sub	eax,bufX						//Is part of line in current bank?
//			jbe 	V7AfterBankTransition	//No
//
//			movzx	ecx,bufWidth				//ECX=rectangle's width in pixels
//			cmp	ecx,eax						//Is line completely in current bank?
//			jbe	V7SetWidthInThisBank		//Yes
//
//			mov	ecx,eax						//ECX=line's width in current bank
//
//		V7SetWidthInThisBank:
//			mov	widthInThisBank,cx		//Save line's width in current bank
//			mov	eax,edi						//EAX=line's starting pixel offset
//			and	eax,3							//Is pixel on a double word boundary?
//			jz		V7SetDWordWidthBT			//Yes
//
//			mov	ecx,4							//ECX=number of pixel's per double word
//			sub	ecx,eax						//ECX=offset to next double word
//			mov	eax,ecx						//EAX=offset to next double word
//			cmp	ax,bufWidth					//Is next double word outside of rectangle?
//			jb		V7AlignOnDWordBT			//No
//
//			movzx	ecx,bufWidth				//ECX=rectangle's width in pixels
//			mov	eax,ecx						//EAX=rectangle's width	in pixels
//
//		V7AlignOnDWordBT:
//			rep	movsb							//Draw pixel by pixel to next double word
//
//		V7SetDWordWidthBT:
//			movzx	ecx,widthInThisBank		//ECX=rectangle's width in pixels in current bank
//			sub	ecx,eax						//ECX=rectangle's remaining width in pixels in current bank
//			mov	eax,ecx						//EAX=rectangle's remaining width in pixels in current bank
//			shr	ecx,2							//ECX=rectangle's remaining width in double words in current bank
//			rep	movsd							//Draw 4 pixel's at a time
//			mov	ecx,eax						//ECX=rectangle's remaining width in current bank prior to double word draw
//			and	ecx,3							//ECX=rectangle's remaining width in pixels in current bank
//			rep	movsb							//Draw remaining pixels in current bank pixel by pixel
//
//		V7AfterBankTransition:
//			mov	bufHeight,dx				//Save number of lines left to blit
//
//			movzx	ecx,[ebx].bankNumber		//ECX=current bank number
//			inc	ecx							//ECX=next bank number
//
//			V7SetBankNumber()					//Set bank number of next bank
//
//			movzx	edx,bufHeight				//EDX=number of lines left to blit
//			xor	edi,edi						//EDI=new bank's first pixel's offset
//			movzx	ecx,bufWidth				//ECX=rectangle's width in pixels
//			sub	cx,widthInThisBank		//Is line complete?
//			jz		V7EndOfLine					//Yes
//
//			test	widthInThisBank,0xFFFF	//Check to see if part of line was in previous bank?
//			mov	widthInThisBank,cx		//Set line's width in current bank
//			jnz	V7CrossBankBoundary		//Part of line was in previous bank
//
//			mov	edi,bufX						//EDI=rectangle's upper left X coordinate
//			sub	di,[ebx].transitionX		//EDI=rectangle's first pixel offset in current bank
//			mov	eax,edi						//EAX=rectangle's first pixel offset in current bank
//			and	eax,3							//Is pixel on a double word boundary?
//			jz		V7SetDWordWidthAT			//Yes
//
//			mov	ecx,4							//ECX=number of pixel's per double word
//			sub	ecx,eax						//ECX=offset to next double word
//
//		V7CrossBankBoundary:
//			mov	eax,ecx						//EAX=offset to next double word
//			cmp	ax,bufWidth					//Is next double word outside of rectangle?
//			jb		V7AlignOnDWordAT			//No
//
//			movzx	ecx,bufWidth				//ECX=rectangle's width in pixels
//			mov	eax,ecx						//EAX=rectangle's width	in pixels
//
//		V7AlignOnDWordAT:
//			rep	movsb							//Draw pixel by pixel to next double word
//
//		V7SetDWordWidthAT:
//			movzx	ecx,widthInThisBank		//ECX=rectangle's width in pixels in current bank
//			sub	ecx,eax						//ECX=rectangle's remaining width in pixels
//			mov	eax,ecx						//EAX=rectangle's remaining width in pixels
//			shr	ecx,2							//ECX=rectangle's remaining width in double words
//			rep	movsd							//Draw 4 pixels at a time
//			mov	ecx,eax						//ECX=rectangle's remaining width prior to double word draw
//			and	ecx,3							//ECX=rectangle's remaining width in pixels
//			rep	movsb							//Draw remaining pixels pixel by pixel
//			add	ebx,BANK_DATA_SIZE		//EBX->next line's bank info entry
//			dec	edx							//Are there more lines to blit?
//			jnz	V7NextLine					//Yes
//
//		V7BRTSExit:
//		}
// 	bufHeight = 0;								//DON'T REMOVE - this dummy statement makes switch work with asm code
//	break;
//}



//---[ Platform-specific palette functions ]-------------------------

void
OS_SetPalette(char* gunsArray, uint startGun, uint endGun)
{
	ASetPalette(gunsArray, startGun, endGun);
}


void
OS_GetPalette(char* gunsArray, uint startGun, uint endGun)
{
	AGetPalette(gunsArray, startGun, endGun);
}


//---[ Platform-optimized copy-pixels functions ]--------------------


const	uchar		SKIP_BYTE = 254;
#define			MAX_H_RES	640


static void	CopyPixelsNormal (RCopyPixels* p);
static void	CopyPixelsScaledRLE (RCopyPixels* p);
static void	CopyPixelsRLE (RCopyPixels* p);
static void	CopyPixelsScaled (RCopyPixels* p);


void
OS_CopyPixels (RCopyPixels* p, bool turnInterruptsOff)
{
//	if (turnInterruptsOff)
//	{
//		_disable();	
//	}

	switch (p->_flags)
	{
		case CP_RLE|CP_NOSKIP:
		case CP_RLE:
			CopyPixelsRLE (p);
			break;
		
		case CP_NORMAL:
		case CP_NORMAL|CP_NOSKIP:
			CopyPixelsNormal (p);
			break;
		
		case CP_SCALED|CP_NOSKIP:
		case CP_SCALED:
			CopyPixelsScaled (p);
			break;

		case CP_SCALED|CP_RLE|CP_NOSKIP:
		case CP_SCALED|CP_RLE:
			CopyPixelsScaledRLE (p);
			break;

		default:
			APanic("unknown CopyPixels request");
			break;
	}					// end switch

//	if (turnInterruptsOff)
//	{
//		_enable();	
//	}
}

static void
CopyPixelsNormal (RCopyPixels* p)
{
	// variables in order of performance
//	register int		col;
//	register uchar		pixel;
	register uchar*	pBufData;
	register uchar*	pCelData;
	register uint32	celIndex;
	register uint32	bufIndex;			// index from pBufData to pDst
	int					row;
	int					rWidth;
	int					vbufWidth;
	int					celWidth;
//	uint16				eSReg;

	Rectangle*	pR = (Rectangle*) p->_vpRectFillArea;

	rWidth = pR->Wide();

	// use register/local variables
	pBufData = p->_pBufData;
	pCelData = p->_pCelData;
	vbufWidth = p->_vbufWidth;
	celWidth = p->_celWidth;

	celIndex = p->_celY * p->_celWidth + p->_celX;
	bufIndex = pR->y1 * vbufWidth + pR->x1;

	for (row = pR->y1; row <= pR->y2; row++)
	{
		if (p->_flags & CP_NOSKIP)
		{
			memcpy(pBufData + bufIndex, pCelData + celIndex, rWidth);
		}
		else
		{
		
			#if 0
				// generic c version
				for (col = 0; col < rWidth; col++)
				{
					if ((pixel = *(pCelData + celIndex + col)) != SKIP_BYTE)
					{
						*(pBufData + bufIndex + col) = pixel;
					}
				}
			#else
				CopyPixels(pCelData, pBufData, rWidth, TRUE, celIndex, bufIndex);
			#endif
		}
		bufIndex += vbufWidth;
		celIndex += celWidth;
	}
}



static void
CopyPixelsRLE (RCopyPixels* p)
{
	register uchar*	pDst;
	register int		col;
	register uchar		pixel;
	register int		celX;
	register int		rWidth;

	Rectangle*			pR = (Rectangle*) p->_vpRectFillArea;
	int					row;
	uint32				stackCheck = 0xfadefade;
	uchar					line[MAX_H_RES];
	uint					maxExpand;
	ScanOffsetTable	*pTable;

	rWidth = pR->Wide();

	// use register variables
	pDst = p->_pDst;
	celX = p->_celX;

	// get cel data ptr
	pTable = (ScanOffsetTable*) (p->_pCelData);

	// don't expand any more than needed
	maxExpand = AMin(p->_celWidth, (celX + rWidth));

	// expand each full scan line, copy needed segment into blit buffer
	for (row = pR->y1; row <= pR->y2; row++)
	{
		ExpandRLE(line, ScanAddress(pTable, row + p->_celY - pR->y1), maxExpand);

		if (p->_flags & CP_NOSKIP)
		{
			memcpy(pDst, line + celX, rWidth);
		} 
		else 
		{
			for (col = 0; col < rWidth; col++)
			{
				if ((pixel = line[celX + col]) != SKIP_BYTE)
				{
					pDst[col] = pixel;
				}
			}
		}
		pDst += p->_vbufWidth;
	}
		
	// verify that ExpandRLE() didn't write beyond is output buffer
	ASSERT (stackCheck == 0xfadefade);
}


static void
CopyPixelsScaled (RCopyPixels* p)
{
	register uchar*	pSrc;
	register uchar*	pDst;
	register int		col;
	register uchar		pixel;
	register int		scale;

	Rectangle*	pR = (Rectangle*) p->_vpRectFillArea;
	int			rWidth;
	int			row;
	int			endCol;
//	uint16		maxExpand;
	uint32		celIndex;
	uint32		bufIndex;			// index from pBufData to pDst
	uchar*		pBufData;
	uchar*		pCelData;

	// since each pixel must be scaled, and since background cels
	// usually won't be scaled, no attempt is made to
	// optimize for CP_NOSKIP

	rWidth = pR->Wide();

	// use register variables
	pDst = p->_pDst;
	scale = p->_scale;
	pBufData = p->_pBufData;
	pCelData = p->_pCelData;

	endCol = p->_celX + rWidth;
	bufIndex = pR->y1 * p->_vbufWidth + pR->x1;

	for (row = pR->y1; row <= pR->y2; row++)
	{	
		// scale Y coord to find correct row
		celIndex = (UnScaleCoord(row + p->_celY - pR->y1, scale) * p->_celWidth); 

		pDst = pBufData + bufIndex;
		pSrc = pCelData + celIndex;

		for (col = p->_celX; col < endCol; col++)
		{
			if ((pixel = pSrc[ UnScaleCoord(col, scale) ]) != SKIP_BYTE)
			{
				*pDst = pixel;
			}
			pDst++;
		}
		bufIndex += p->_vbufWidth;
	}
}



static void
CopyPixelsScaledRLE (RCopyPixels* p)
{
	register int		col;
	register int		celX;
	register uchar		pixel;
	register int		scale;
//	register uchar*	pSrc;
	register uchar*	pDst;
	uchar*	pBufData;
	uchar*	pCelData;

	Rectangle*			pR = (Rectangle*) p->_vpRectFillArea;
	int					rWidth;
	int					row;
	int					endCol;
	uint32				stackCheck = 0xfadefade;
	uchar					line[MAX_H_RES];
	uint					maxExpand;
	ScanOffsetTable	*pTable;

	// since each pixel must be scaled, and since background cels
	// usually won't be scaled, no attempt is made to
	// optimize for CP_NOSKIP

	rWidth = pR->Wide();

	// use register variables
	pDst = p->_pDst;
	pBufData = p->_pBufData;
	pCelData = p->_pCelData;
	scale = p->_scale;
	celX = p->_celX;

	// get cel data ptr
	pTable = (ScanOffsetTable*) (pCelData);

	// don't expand any more than needed
	maxExpand = p->_celWidth;
	maxExpand = AMin(p->_celWidth, UnScaleCoord(celX + rWidth, scale));

	// Expand first line
	ExpandRLE(line, 
			ScanAddress(pTable, UnScaleCoord(p->_celY, scale)),
			maxExpand);

	// init loop iter
	row = pR->y1;
	endCol = celX + rWidth;

	// expand each full scan line, copy needed segment into blit buffer
	while (TRUE)
	{
		for (col = celX; col < endCol; col++)
		{
			if ((pixel = line[ UnScaleCoord(celX+col, scale) ]) != SKIP_BYTE)
			{
				pDst[col] = pixel;
			}
		}

		// are we done?
		if (++row > pR->y2)
		{
			break;
		}

		//// see if scaling requires new source scan or not
		//if (UnScaleCoord(row, p->_scale) > UnScaleCoord(row-1, scale))
		//{
			ExpandRLE(line, 
				ScanAddress(pTable, UnScaleCoord(p->_celY + row - pR->y1, scale)), 
				maxExpand);
		//}

		pDst += p->_vbufWidth;
	}
		
	// verify that ExpandRLE() didn't write beyond is output buffer
	ASSERT (stackCheck == 0xfadefade);

}
