//	FLICSMK.HPP
//
//	Copyright 1996, Tachyon, Inc.
//
// Play a smacker file
//
// 1/15/96

#ifndef FLICSMACKER_HPP
#define FLICSMACKER_HPP

#include "tigre.hpp"
#include "text.hpp"
#include "palette.hpp"
#include "writeres.hpp"

#include "smack.h"
#include "svga.h"


//Script data structure
struct SmkScript
{
	u32	firstFrame;
	u32	lastFrame;
	uchar	action;
	uchar	aux1; //for action 'v' track number	(1-7)
	uchar	aux2; //for action 'v' percent volume (0-100)
};

#define	TEXT_SCRIPT_SIZE	50

//Text overlay script data structure
struct SmkTextScript
{
	u32	firstFrame;
	u32	lastFrame;
	uchar	action;

	u32	font;
	u32	sqbRes;
	u32	sqbNum;
	u32	textX;
	u32	textY;
	u32	wrapWidth;
	uchar	just;
	u32	coverAni;
	u32	coverCel;
	u32	coverX;
	u32	coverY;

	//font colors to use
	u32	c[8];
};


class TFlicSmacker : public TMovable
{
	public:

	TFlicSmacker();
	~TFlicSmacker();

	int		Play(int cineNum, int extraBuf=0, int simSpeed=0,
					  int startFrame=1, bool skipFrames=TRUE);
	void		ChangeVolume(SmkScript* pChgVol, Smack* smk, u32 frameNo);
	void		ChangePalette(SmkScript* pChgPal, Smack* smk, u32 frameNo);
	void		Fader(int upDown);
	void		ChangeText(SmkTextScript *pChgText, int frameNum);

	SmkScript	changePal[100];		//Palette change data
	SmkScript 	changeVol[20];			//Volume change data
	SmkTextScript *changeText;			//Text overlay change data

	u16		soundVolume;
	u16		soundPan;
	bool		activatePalette;		//Global palette activation flag
	char 		prevVideoMode;			//Global previous video mode save area
	bool		fFlicDone;

	Smack* 	smk;
	Gun		guns[256];
	Gun		destGuns[256];	//for smooth fade-in
	int		trackVol[7];   //for individual track volumes as percentage
	grip			gDecBuf;
	CelHeader	*pDecBufCH;
	void			*pDecBuf;

	int		cineNum;
	int		forceCnt;
	SquibRes sqbCine;

};

#endif
