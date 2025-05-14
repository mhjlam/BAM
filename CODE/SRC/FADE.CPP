//
// FADE.CPP
//
// November 1, 1994
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include "fade.hpp"
#include "apires.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "graphmgr.hpp"
#include "bam.hpp"
#include "context.hpp"
#include "mouse.hpp"
#include "resource.hpp"

#define MAX_FADE_IMAGE_BYTES 128001

FadeTo::FadeTo()
{
	gDest  = 0;
	gDest2 = 0;

	//gTDest = 0;

	//TRACK_MEM("FadeBits");		gSaveBits = AMalloc(MAX_FADE_IMAGE_BYTES);
	//TRACK_MEM("FadeWorkBits");	gWorkBits = AMalloc(MAX_FADE_IMAGE_BYTES);

}


FadeTo::~FadeTo()
{
	//if(gTDest)
	//{
	//	AFlush(gTDest);
	//}

	//AFree(gSaveBits);
	//AFree(gWorkBits);
}


void
FadeTo::Setup(int newWidth, int newHeight, int newPri, grip gContext_P,
				  grip gPal_P, uint32 percentFade, Rectangle *exceptRect, int32 dr, int32 dg, int32 db)
{
	//CelHeader	*pTDest;
	//uchar			*pTDestData;
	//uchar			*pSaveBits;
	//uchar			*pWorkBits;
	//uchar			*pDataLine;

	CelHeader	*pDest,*pDest2;
	uchar			*pDestData,*pDestData2;
	uchar			*pData;
	int32			i,j,k, percent, width, exceptWidth;
	Rectangle	rDest;
	Rectangle	*prExcept;


	gContext = gContext_P;

	gPal = gPal_P;
	if (!gPal)
		APanic("FadeTo bad palette");

	// SETUP DEST STUFF =================================
	//assume dest is always dynamic!!!!!

	//lets make sure this is only done
	//now ref. this dest we will fade
	TRACK_MEM("Fade: top half buffer");
	gDest = ACreateCel(&rNumDest,0,0,newWidth,newHeight/2,CI_BLACK,newPri);
	pDest = (CelHeader*)AGetResData(gDest);
	pDestData =	(uchar*)pDest + sizeof(CelHeader);

	TRACK_MEM("Fade: bottom half buffer");
	gDest2 = ACreateCel(&rNumDest2,0,0,newWidth,newHeight-(newHeight/2),CI_BLACK,newPri);
	pDest2 = (CelHeader*)AGetResData(gDest2);
	pDestData2 = (uchar*)pDest2 + sizeof(CelHeader);

	rDestTop.Set(0, 0, newWidth-1, (newHeight/2)-1);
	rDestBottom.Set(0, newHeight/2, newWidth-1, newHeight-1);

	rExceptTop.Copy(exceptRect);
	rExceptBottom.Copy(exceptRect);
	rExceptTop.Clip(&rDestTop);
	rExceptBottom.Clip(&rDestBottom);

	pMouse->Hide();

	//pWorkBits = ADerefAs(uchar, gWorkBits);
	ARBlit(pGraphMgr->vgaDriver, pDestData,
			rDestTop.x1, rDestTop.y1, rDestTop.Wide(), rDestTop.High(), pGraphMgr->videoBufferSeg);

	ARBlit(pGraphMgr->vgaDriver, pDestData2,
			rDestBottom.x1, rDestBottom.y1, rDestBottom.Wide(), rDestBottom.High(), pGraphMgr->videoBufferSeg);

	pMouse->Show();

	//pWorkBits = ADerefAs(uchar, gWorkBits);
	//memcpy(pWorkBits,pSaveBits,MAX_FADE_IMAGE_BYTES);

	//TRACK_MEM("Fade: backup orig. cel");
	//gTDest = ACreateCel(NULL,0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1,CI_BLACK,10);
	//
	//pTDest = (CelHeader*)AGetResData(gTDest);
	////pTDestData = (uchar*)pTDest + sizeof(CelHeader);
	//
	////dest will be always be in flux so lets snapshot an original view
	//CopyCel(pTDest,0,0,DYNAMIC_CEL,gDest,&rDest,FALSE);
	
	//for(int curStep=1; curStep<=fadeSteps; curStep++)
	//{
	//	if (curStep >= fadeSteps)
	//	{
	//		// last step
	//		percent = percentFade;
	//	}
	//	else 
	//	{
	//		percent = (percentFade/fadeSteps) * curStep;
	//	}

	percent = percentFade;

		// update table for new colors to map to.
		MapPalette(percent,dr,dg,db);

		for(k = 0; k < 2; k++)
		{
			rDest.Copy((k == 0)?&rDestTop:&rDestBottom);
			prExcept = (k == 0)?&rExceptTop:&rExceptBottom;

			width = rDest.Wide();
			exceptWidth = prExcept->Wide();
			//pDataLine = pDestData;
			pData = (k == 0)?pDestData:pDestData2;

			for (j = rDest.y1; j < prExcept->y1; j++)
			{
				for (i = L2R(rDest.x1); i < L2R(rDest.x2 + 1); i++)
				{
					*pData = mapTable[*pData];
					pData++;
				}
			}
			for (; j<=prExcept->y2; j++)
			{
				for (i = L2R(rDest.x1); i < L2R(rDest.x2 + 1); i++)
				{
					if(i == L2R(prExcept->x1))
					{
				 		i += L2R(exceptWidth);
						pData += L2R(exceptWidth);
					}
					*pData = mapTable[*pData];
					pData++;
				}
			}
			for (; j<=rDest.y2; j++)
			{
				for (i = L2R(rDest.x1); i < L2R(rDest.x2 + 1); i++)
				{
					*pData = mapTable[*pData];
					pData++;
				}
			}
		}

		//ABlit(pGraphMgr->vgaDriver, pWorkBits,
		//		rDest.x1, rDest.y1, rDest.Wide(), rDest.High(),
		//		pGraphMgr->videoBufferSeg);

		//memcpy(pDestData,pWorkBits,newWidth*newHeight);

		SetRes(RES_CEL,rNumDest);
		SetPos(0,0);
		BAM_Guy::Setup(CT_ROST);
		SetPri(newPri);

		bottomGuy.SetContext(gContext);
		bottomGuy.SetRes(RES_CEL,rNumDest2);
		bottomGuy.SetPos(0,newHeight/2);
		bottomGuy.BAM_Guy::Setup(CT_ROST);
		bottomGuy.SetPri(newPri);

		//// update table for new colors to map to.
		//MapPalette(percent,dr,dg,db);
		//
		//for (j=rDest.y1; j<=rDest.y2; j++)
		//{
		//	pData = pDestData;
		//	for (i=rDest.x1; i<=rDest.x2; i++)
		//	{
		//		*pData = mapTable[*pData];
		//		pData++;
		//	}
		//	pDestData += pDest->width;
		//}
		//
		//AUpdateRect(&rDest);
	//}
}


//mapPallete() -returns a table which has mapped each color in the current
//					palette to the closest matching color in the palette.
//					Matching criteria are the given params.

void
FadeTo::MapPalette(int32 percent,int32 destR,int32 destG,int32 destB)
{
	Gun		*pGun;
	int		x;
	int32 	r,g,b;
	uchar		*pMap=mapTable,dcolor;
	int32		origR,origG,origB;
	TPalette	*pPal;
	
	pPal = ADerefAs(TPalette,gPal);
	pGun = pPal->guns;

	if(percent != 100)
	{
		for(x=0;x<256;x++)
		{
			origR = (int32)pGun->r;
			origG = (int32)pGun->g;
			origB = (int32)pGun->b;

			r = origR + (((destR - origR)*percent) / 100);
			g = origG + (((destG - origG)*percent) / 100);
			b = origB + (((destB - origB)*percent) / 100);

			// lets map all colors to the limited range (0-191) no make that (0-15).
			*pMap = MatchColor(r,g,b,0,15);
			pMap++;
			pGun++;
		}
	}
	else
	{
		dcolor = MatchColor(destR,destG,destB,0,15);
		for(x = 0; x < 256; x++)
		{
			// lets map all colors to the dest. color
			mapTable[x] = dcolor;
			pGun++;

		}

	}

	#ifdef OS_MAC
		mapTable[CI_BLACK] = CI_BLACK;
		mapTable[CI_WHITE] = CI_WHITE;
	#endif
}

// find a color in the palette closest to rgb value given
uchar
FadeTo::MatchColor(int32 red, int32 green, int32 blue, int32 sColor, int32 eColor)
{
	uint32		best = 10000000, least;
	int32			tr,tg,tb;
	uchar			color;
	int32			endColor;
	Gun			*pGun;
	int32	origR,origG,origB;
	TPalette		*pPal;
	int			t;
	
	pPal = ADerefAs(TPalette,gPal);
	pGun = pPal->guns;

	// default to black
	color = CI_BLACK;

	// don't look at skip color or higher for main loop
	if (eColor >= 254)
		endColor = 253;
	else
		endColor = eColor;

	for (t=sColor;t<=endColor;t++,pGun++)
	{
		origR = (int32)pGun->r;
		origG = (int32)pGun->g;
		origB = (int32)pGun->b;

		tr = origR - red;
		tg = origG - green;
		tb = origB - blue;
		least = tr*tr + tg*tg + tb*tb;

		if (least<best)
		{
			best = least;
			color = (uchar)t;
		}
	}

	if (eColor > 254)
	{
		//we know the prev loop finished at 254
		pGun++; //now at 255

		origR = (int32)pGun->r;
		origG = (int32)pGun->g;
		origB = (int32)pGun->b;

		// lets check the last color after skip color
		tr = origR - red;
		tg = origG - green;
		tb = origB - blue;
		least = tr*tr + tg*tg + tb*tb;

		if (least<best)
		{
			best = least;
			color = (uchar)t;
		}

	}

	return color;
}


void
FadeTo::FadeUp()
{
	Cleanup();

	bottomGuy.Cleanup();

	if(gDest)
		AFlush(gDest);

	if(gDest2)
		AFlush(gDest2);


//	uchar		*pSaveBits;
//
//	pMouse->Hide();
//
//	pSaveBits = ADerefAs(uchar,gSaveBits);
//	ABlit(pGraphMgr->vgaDriver, pSaveBits,
//			rDest.x1, rDest.y1, rDest.Wide(), rDest.High(),
//			pGraphMgr->videoBufferSeg);
//
//	pMouse->Show();

//	CelHeader	*pDest;
//
//	//copy the orig. snapshot back to dest
//	pDest = (CelHeader*)AGetResData(gDest);
//	CopyCel(pDest,0,0,DYNAMIC_CEL,gTDest,&rDest,FALSE);
//
//	AUpdateRect(&rDest);
}
