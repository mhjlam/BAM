// HALL.HPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
//
// Display Hall of Legends.
//
// 3/8/95
//

#ifndef hall_hpp
#define hall_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"

#define SCROLL_STEP_SIZE 80
#define SCROLL_STEPS 4
#define CLUT3_START			208
#define CLUT4_START			216
#define CLUT5_START			224
#define CLUT6_START			232
#define SLASH 0
#define DOT   1

struct CampaignHeader
{
  	int	entries;
	int	mostRecent;
	char	pad[40];
};


struct Campaign
{
	int	curCharId;
	bool	deleted;
	int	missionsDone;
	int	curPath; // holds legend path index
	int	curCover,curFace,curBody,curBanner;
	char	curName[40];
	int	campaignScore,maxCampaignScore;
	int	levelScore[16];
	char	pad[40];
};

class Hall : public BAM_Room
{
	public:

	Hall(void);
	~Hall();

	bool	HandleMsg(Message* pMsg);
 	void	Cycle();
 	void	Setup();
	void	SetupBanner();

	void	SetupHalls();	//Setup all hall panels
	void	ResetHalls();
	void	LoadHall(int mode);

	bool	SetupCampaign();
	//int		CompareCamp(void const *pparm1,void const *pparm2);
	void	WriteCamp();
	void	NextRoom();

	void	DisplayCampaign();
	void	ConfigButtons(int dir=0);
	int		FindValid(int campCnt_P,int dir=0);
	void	CheckMostRecent();
	void	LoadNameRank();
	void	LoadCampaignScores();
	void	LoadLevelScores();
	void	ClearAll();

	BAM_Guy		back;
	TPalette		pal;
	bool			immedBail;

	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	grip			clut[2];

	SquibRes		sqbHall; //holds generic text for this room - 6001.sqb
	int			sqbRes;
	int			sqbNum;

	grip			gCampName;
	uint			rNumCampName;
	grip			gCampRank;
	uint			rNumCampRank;
	grip			gCampScore;
	uint			rNumCampScore;
	grip			gMaxCampScore;
	uint			rNumMaxCampScore;
	grip			gLevelScore;
	uint			rNumLevelScore;
	grip			gCurrent;
	uint			rNumCurrent;
	BAM_Guy		campNameGuy;
	BAM_Guy		campRankGuy;
	BAM_Guy		campScoreGuy;
	BAM_Guy		maxCampScoreGuy;
	BAM_Guy		levelScoreGuy;
	BAM_Guy		currentGuy; //holds most recent marker

	grip			gLeftHall;
	uint			rNumLeftHall;
	grip			gCenterHall;
	uint			rNumCenterHall;
	grip			gRightHall;
	uint			rNumRightHall;
	BAM_Guy		leftHallGuy;
	BAM_Guy		centerHallGuy;
	BAM_Guy		rightHallGuy;
	int			leftIdx,rightIdx; //hold indexes to valid campaigns
														 //or -1.

	BAM_Button	diskButton;
	BAM_Button	leftButton;
	BAM_Button	rightButton;
	BAM_Button	delButton;
	BAM_Button	exitButton;

	FILE 			*campFile;
	grip			gCampArr;
	CampaignHeader	header;
	int			campCnt;			//array index to currently displayed campaign
	bool			duringPlay;		//is player in an active campaign?
	int			deleteCnt;

	bool			buttonsLocked;
	bool			scrollingDone;
	bool			displayCampaign;
	int			scrollStepCnt;
	int			scrollDir; //-1=left,1=right
	//int			prevScrollDir;
	int			centerColumnMode; //1=two columns, 2=one left column, 3=one right column
	int			centerColorMode; //0=unit color range at 192, 1=unit color range at 200

	int			legendPath[16];

	int			xPosNames[2];
	int			xPosScores[2];
	int			xPosMaxScores[2];
	int			yPos[8];

	//often used temp vars.
	char			tmpStr[40];
	Campaign		*pCampArr;
	Campaign		*pCamp;	//used alot as tmp ptr into CampArr
	CelHeader	*pCH;
	char			*pTxt;

};

#endif
