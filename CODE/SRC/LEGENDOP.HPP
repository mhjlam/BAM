// LEGENDOP.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup screen to show legendary opponent for this map.
//
// 10/28/95
//

#ifndef legendOpp_hpp
#define legendOpp_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"
#include "makechar.hpp"


class LegendOpponent : public BAM_Room
{
	public:

	LegendOpponent(void);
	~LegendOpponent();

	bool 	HandleMsg(Message* pMsg);
 	void 	Setup(grip gPal_P,bool fadeup=FALSE);
	void 	Cleanup();
	void 	ReplaceName(char *destStr, int res, int num);
	void 	ClearText();

	//for creating random ENEMY
	void	RandomCover();
	void	RandomFace();
	void	RandomBody();
	void	RandomBanner();
	int	RandomName();	//it returns a sqbRes
	int	MyRandom(int max, int oldnum= -1);

	bool	LastNameMatch(int nameNum, int nameRes);
	bool	LastFaceMatch(int faceAnim);

	//FadeTo	fadeTo;

	grip			gPal;
	grip			clut;	//to get to second color range

	BAM_Guy	back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	int			prevFont;

	SquibRes		sqbLegendOpp;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	uint32		oldMouseResNum;
	uint32		oldMouseCel;

	BAM_Button	buttonDone;

	int			legendPath[16];
	int			curSqbNum;

	int			maxMenNameNum;
	int			maxWomenNameNum;
	int			maxOtherNameNum;

	gender_t		coverGen[MAX_COVER];
	gender_t		bodyGen[MAX_BODY];
	gender_t		mapGen[15][3];

	int			enemyCover;
	int			enemyFace;
	int			enemyBody;
	int			enemyNameNum;
};

#endif

