//	FLICSMK.HPP
//
//	Copyright 1996, Tachyon, Inc.
//
// Play a smacker file
//
// 1/15/96

#ifndef FLICSMACKER_HPP
#define FLICSMACKER_HPP

#include <stdint.h>

#include "tigre.hpp"
#include "text.hpp"
#include "palette.hpp"
#include "writeres.hpp"

#include "smack.h"
#include "svga.h"


//Script data structure
struct SmkScript
{
	uint32_t	firstFrame;
	uint32_t	lastFrame;
	uchar	action;
	uchar	aux1; //for action 'v' track number	(1-7)
	uchar	aux2; //for action 'v' percent volume (0-100)
};

#define	TEXT_SCRIPT_SIZE	50

//Text overlay script data structure
struct SmkTextScript
{
	uint32_t	firstFrame;
	uint32_t	lastFrame;
	uchar	action;

	uint32_t	font;
	uint32_t	sqbRes;
	uint32_t	sqbNum;
	uint32_t	textX;
	uint32_t	textY;
	uint32_t	wrapWidth;
	uchar	just;
	uint32_t	coverAni;
	uint32_t	coverCel;
	uint32_t	coverX;
	uint32_t	coverY;

	//font colors to use
	uint32_t	c[8];
};


class TFlicSmacker : public TMovable
{
	public:

	TFlicSmacker();
	~TFlicSmacker();

	int		Play(int cineNum, int extraBuf=0, int simSpeed=0,
					  int startFrame=1, bool skipFrames=true);
	void		ChangeVolume(SmkScript* pChgVol, Smack* smk, uint32_t frameNo);
	void		ChangePalette(SmkScript* pChgPal, Smack* smk, uint32_t frameNo);
	void		Fader(int upDown);
	void		ChangeText(SmkTextScript *pChgText, int frameNum);

	SmkScript	changePal[100];		//Palette change data
	SmkScript 	changeVol[20];			//Volume change data
	SmkTextScript *changeText;			//Text overlay change data

	uint16_t	soundVolume;
	uint16_t	soundPan;
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
