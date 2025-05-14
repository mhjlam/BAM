//
// MOUSEINT.HPP
//
//	Interrupt based pointer
//
// November 24, 1993
// WATCOM: October 25, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	Mouse class definition.
//
//----[]-------------------------------------------------------------


#ifndef	mouseint_hpp
#define	mouseint_hpp


#include "rect.hpp"
#include "manager.hpp"
#include "types.hpp"
#include "mouse.hpp"


//---------[Mouse]-------------------------------------------------

class MouseInt : public Mouse
{
	public:

		uint			pauseCnt;
		coord			pauseX;
		coord			pauseY;

							MouseInt();
		virtual			~MouseInt();

		virtual	coord	GetX();
		virtual	coord	GetY();
		virtual	int	GetCel();
		virtual	res_t	GetResType();
		virtual	uint	GetResNum();

		virtual	void	SetX(coord setX);
		virtual	void	SetY(coord setY);

		void				MovePointer(void);

		// this is just here as a stub
		void				UpdatePointer();

		virtual	void	SaveUnderBits(void);
		virtual	void	RestoreUnderBits(void);
		virtual	void	DrawPointer(void);

		virtual	void	SetRes(res_t type, uint num, uint iCel = 1, uint iScale = 256);
		virtual	void	SetPri(int32 pri);
		virtual	void	SetScale(uint newScale);

		void				SetFenceRes(res_t horizType, uint rHorizNum, uint horizCel,
								res_t vertType, uint rVertNum, uint vertCel);
		virtual void	EnableDragging(bool fNewState);

		void				ClipLastRect(void);

		void				Pause(bool pauseIt);
		bool				Protect(bool protect, Rectangle *pRect = NULL);

		bool				Save(uint16 state, FILE *fp = NULL);

	protected:

		int32				clipRow;
		int32				clipCol;

		res_t			resType;		// Type of resource attached to us
		uint			resNum;		// Number of resource attached to us
		uint			cel;			// current cel
		uint			scale;		// % scale factor

		coord			orgX;
		coord			orgY;
		Rectangle	lastRect, rLastFenceRect;

		grip			gMouseImage;
		grip			gSaveBits, gSaveH1Bits, gSaveH2Bits, gSaveV1Bits, gSaveV2Bits;
		grip			gDrawBits;
		grip			gHFenceImage, gVFenceImage;
		bool			saveBitsSaved;
		uchar			*pMouseImage;
		uchar			*pHFenceImage, *pVFenceImage;
		uchar			*pSaveBits, *pSaveH1Bits, *pSaveH2Bits, *pSaveV1Bits,
							*pSaveV2Bits;
		uchar			*pDrawBits;

		Rectangle	rect;

		virtual	void	ShowPointer();
		virtual	void	HidePointer();
		virtual	void	HideFence(void);
};


#endif
