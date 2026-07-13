//
// FADE.HPP
//
// November 1, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#ifndef fade_hpp
#define fade_hpp

#include "api.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "palette.hpp"
#include "rect.hpp"
#include "tigre.hpp"

#include <time.h>

class FadeTo : public BAM_Guy
{
	public:	
		FadeTo();
		~FadeTo();

		//the whole screen blitted into one buffer is just to big, so...
		//(due to memory frag. some systems fail upon request of such a large alloc)
		grip			gDest;		//holds top half of screen
		uint			rNumDest;

		grip			gDest2;		//holds bottom half of screen
		uint			rNumDest2;
		BAM_Guy		bottomGuy;

		grip			gPal;
		uchar			mapTable[256];

		Rectangle	rDestTop;
		Rectangle	rDestBottom;
		Rectangle	rExceptTop;
		Rectangle	rExceptBottom;

		//Gun				origGun[256];
		//grip			gDest;
		//grip			gTDest;
		//grip			gSaveBits;
		//grip			gWorkBits;

		//void			Setup(grip gPal, grip gDest_P, uint32 maxPercent, int32 dr=0, int32 dg=0, int32 db=0);
		//void			Setup(grip gPal, uint32 maxPercent, int32 dr=0, int32 dg=0, int32 db=0);

		void			Setup(int newWidth, int newHeight, int newPri, grip gContext_P,
				  				grip gPal_P, uint32 percentFade, Rectangle *exceptRect, int32 dr=0, int32 dg=0, int32 db=0);
		void			MapPalette(int32 percent, int32 destR,int32 destG,int32 destB);
		uchar			MatchColor(int32 red, int32 green, int32 blue,int32 sColor=0,int32 eColor=255);
		void			FadeUp();

};

#endif
