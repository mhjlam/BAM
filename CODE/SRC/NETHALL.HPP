// NETHALL.HPP
//
//	Copyright 1995 Tachyon, Inc.
//
//
// Display Hall of Net Champions.
//
// 10/8/95
//

#ifndef nethall_hpp
#define nethall_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"

// number of entries we can display
#define	OPP_LIST_SIZE 8

//lets keep more entries than we can actually show
#define	MAX_OPP_LIST_SIZE 16

struct NetCampaignHeader
{
  	int	entries;
	int	mostRecent;
	char	pad[40];
};

struct NetOpp
{
	char	oppName[40];
	int	oppCharId;
	int	oppNumWin;
	int	pad1;
};

struct NetCampaign
{
	int	curCharId;
	bool	deleted;
	int	missionsDone;
	int	missionsWon;
	int	missionsLost;
	int	curCover,curFace,curBody,curBanner,curCat;
	char	curName[40];
	int	campaignScore;

	int 	lawful,neutral,chaotic;
	int 	evil,nutral,good;

	//bool	lastFightAWin;
	//int		lastOppCharId;

	//opponents you've beaten
	int	oppBeatenNum;
	struct NetOpp	win[MAX_OPP_LIST_SIZE];

	//opponents you've lost to
	int	oppLostToNum;
	struct NetOpp	lose[MAX_OPP_LIST_SIZE];

	char	pad[40];
};

class NetHall : public BAM_Room
{
	public:

	NetHall(void);
	~NetHall();

	bool	HandleMsg(Message* pMsg);
 	void	Cycle();
 	void	Setup();
	void	SetupHalls();	//Setup all hall panels
	void	ResetHalls();
	void	LoadHall(int mode);

	bool	SetupCampaign();
	void	WriteCamp();

	//now global funcs
	//int		CompareCamp(void const *pparm1,void const *pparm2);
	//int		CompareNetCamp(void const *pparm1,void const *pparm2);

	void	DisplayCampaign();
	void	ConfigButtons(int dir=0);
	int	FindValid(int campCnt_P,int dir=0);
	void	CheckMostRecent();
	void	LoadNameRank();
	void	LoadCampaignScores();
	void	LoadWinLoseStats();
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
	grip			gWinLoseStat;
	uint			rNumWinLoseStat;
	grip			gCurrent;
	uint			rNumCurrent;
	BAM_Guy		campNameGuy;
	BAM_Guy		campRankGuy;
	BAM_Guy		campScoreGuy;
	BAM_Guy		winLoseStatGuy;
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

	//BAM_Button	diskButton;
	BAM_Button	leftButton;
	BAM_Button	rightButton;
	BAM_Button	delButton;
	BAM_Button	exitButton;

	FILE 			*campFile;
	grip			gCampArr;
	NetCampaignHeader	header;
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

	int			xPosNames[2];
	int			xPosScores[2];
	int			yPos[8];

	//often used temp vars.
	char			tmpStr[40];
	NetCampaign	*pCampArr;
	NetCampaign	*pCamp;	//used alot as tmp ptr into CampArr
	CelHeader	*pCH;
	char			*pTxt;

};

int NetCompareCamp(void const *pparm1,void const *pparm2);
int NetCompareOpp(void const *pparm1,void const *pparm2);

#endif
