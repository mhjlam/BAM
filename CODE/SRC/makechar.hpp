// MAKECHAR.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A room in which to create a character for legendary campaign.
//
// 12/12/94
//

#ifndef makechar_hpp
#define makechar_hpp

#include "bam.hpp"
#include "bamroom.hpp"

#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"


// DON'T CHANGE THESE MAXS WITHOUT CHANGING FIXED ARRAYS IN CONTRUCTOR
#define MAX_COVER			17
#define MAX_FACE_MEN		9
#define MAX_FACE_WOMEN	8
#define MAX_FACE_OTHER	12
#define MAX_FACE_ALL	(MAX_FACE_MEN+MAX_FACE_WOMEN+MAX_FACE_OTHER)
#define MAX_BODY			10

//defines number of banner colors
#define MAX_BANNER		4

enum charCat_t
{
	C_COVER=0,
	C_FACE,
	C_BODY,
	C_BANNER
};

enum dir_t
{
	C_DECREMENT,
	C_INCREMENT
};

typedef uint	gender_t;

const gender_t	G_MEN		= 0x0001;
const gender_t	G_WOMEN	= 0x0002;
const gender_t	G_OTHER	= 0x0004;
const gender_t	G_ALL		= 0x0007;


class MakeChar : public BAM_Room
{
	public:

	MakeChar(void);
	~MakeChar(void);

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Setup();
	virtual void		LoadCover();
	virtual void		LoadFace();
	virtual void		LoadBody();
	virtual void		LoadBanner();
	virtual void		AdvCover(dir_t dir);
	virtual void		AdvFace(dir_t dir);
	virtual void		AdvBody(dir_t dir);
	virtual void		AdvBanner(dir_t dir);
	virtual void		UpdateGender();
	virtual void		UpdateRandom();
	virtual void		RandomName();
	virtual int			MyRandom(int max, int oldnum);
	virtual void		ShiftPal(int bannerColor);

	BAM_Guy		back;
	TPalette		pal;

	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			gPortBord;

	SquibRes		sqbMakeChar;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	BAM_Guy		curCoverGuy;
	BAM_Guy		curFaceGuy;
	BAM_Guy		curBodyGuy;
	BAM_Guy		curBannerGuy;

	BAM_Button	diskB;
	BAM_Button	leftArrowB[4];
	BAM_Button	rightArrowB[4];
	BAM_Button	randomCharB;
	BAM_Button	manB;
	BAM_Button	womanB;
	BAM_Button	otherB;
	BAM_Button	customNameB;
	BAM_Button	randomNameB;
	BAM_Button	doneB;

	BAM_Box		nameBox;
	grip			gNameText;

	int			curCover;
	int			curFace;
	int			curBody;
	int			curNameNum;
	int			maxMenNameNum;
	int			maxWomenNameNum;
	int			maxOtherNameNum;

	gender_t		coverGen[MAX_COVER];
	gender_t		bodyGen[MAX_BODY];

	int			firstCoverAnim;
	int			firstMenFaceAnim;
	int			firstWomenFaceAnim;
	int			firstOtherFaceAnim;
	int			firstBodyAnim;

	bool			customNameSelected;

};

#endif
