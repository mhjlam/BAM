// ASSESS.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// Assessment statistics for player's victory.
//
// 2/10/95

#ifndef assess_hpp
#define assess_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"

class Pen_Guy : public BAM_Guy
{
	public:
	int			curWord;
	int			letterDone;

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Cycle();
};


class Assess : public BAM_Room
{
	public:

	Assess(void);
	~Assess();

	bool	HandleMsg(Message* pMsg);
 	void	Setup();
 	void	Cycle();
	void	LoadArr(int lineIndex,int sideIndex,char* word, int wordSiz=4);
	void	SetupArr();
	void	CalcScores();
	int 	GetObjAchievedScore(int level);
	void	SetupPen(Pen_Guy* pPen);
	void	Cleanup();

	BAM_Guy		back;
	TPalette		pal;
	#ifdef INTERACTIVE_DEMO
	TMusic		tMusic;
	#else
	TSound		*pDigiMusic;
	#endif
	int			launchMusic;

	grip			gback;

	Pen_Guy		pen1;
	Pen_Guy		pen2;

	SquibRes		sqbAssess;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	BAM_Button	diskButton;

	int			objAchievedScore[5];
	int			levelScore;
	int			maxLevelScore;
	int			campaignScore;
	int			maxCampaignScore;
	int			bonusXP;
	int			legendPath[16];

	bool			scoresWritten;
	bool			fastForward;
	bool			dualWrite;
	int			curLine;
	int			curLineMax;
	int			curWord;
	int			letterCnt;
	int			wordSize;

	int			letterArr[8][2][5];
	int			letterPosX[2][4];
	int			letterPosAltX[2][5];
	int			letterPosY[8];
	grip			clut[2];
	int			playerSide,enemySide;

	grip			gLetter[10];
	uint			letter[10];
	int			shot;
};

#endif
