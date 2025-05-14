// NETCHAR.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A room in which to create a character for NET game.
//
// 7/31/95
//

#ifndef netchar_hpp
#define netchar_hpp

#include "alldefs.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "makechar.hpp"
#include "nethall.hpp"

#include "api.hpp"
#include "palette.hpp"
#include "text.hpp"

#define	NUM_CLUTS 4

//menus
enum netchar_t
{
	NC_MAIN,
	NC_LOAD,
	NC_MAKE,
	NC_WAIT,	//for other player to finish
	NC_LAST
};

//room state
enum
{
	ST_INIT,
	ST_DATA,
	ST_DONE
};

struct NetcharPacket
{
	BamPacketHdr BPHdr;	
 	int			frameNum;

	int			charId;
	int			bodyAnim;
	int			faceAnim;
	int			coverAnim;
	//int			banner;
	char			name[40];
};


class NetChar : public BAM_Room
{
	public:

	NetChar(void);
	~NetChar(void);

 	void			Setup();
	void			Cycle();
	bool			HandleMsg(Message* pMsg);

	void			LoadMenu(netchar_t newMenu);
	void			SetupMakeCharMenu();
	void			SetupLoadCharMenu();
	void			SetupWaitMenu();
	void			SetEnemyBannerColor();
	void			RefreshEnemyChar();
	void			UpdateMainDisplay();
	void			LoadCharPacket();
	int			GetNewCharId();
	void			ClearGlobalVars();

	int			GetSaveNum(char *fileName);
	void			CheckChar();
	int			MyRandom(int max, int oldnum= -1);

	TPalette		pal;

	bool			clearWaitMsg;

	BAM_Guy		back;
	grip			gback;
	uchar			*pback;
	CelHeader	*pbackCH;
	uint			rNumBack;

	//make char menu
	BAM_Guy		backMake;
	grip			gbackMake;
	uint			rNumBackMake;
	uchar			*pbackMake;
	CelHeader	*pbackMakeCH;

	//load char menu
	BAM_Guy		backLoad;
	grip			gbackLoad;
	uint			rNumBackLoad;
	uchar			*pbackLoad;
	CelHeader	*pbackLoadCH;

	//wait menu
	BAM_Guy		backWait;
	grip			gbackWait;
	uint			rNumBackWait;
	uchar			*pbackWait;
	CelHeader	*pbackWaitCH;

	SquibRes		sqbNetChar;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;

	netchar_t	curMenu;
	bool			menuDrawn[NC_LAST];

	grip			clut[NUM_CLUTS];

	BAM_Button	makeCharB;
	BAM_Button	loadCharB;
	BAM_Button	loadGameB;
	BAM_Button	startGameB;
	BAM_Button	disCancelB;

	int							currFrame;
	struct sPacket				sPacket1;
	struct NetcharPacket		ncPacket;
	BAMPacketType				packetType;

	int							sendErrorCnt;
	int							lastSendError;
	ticks_t						sendExpireTicks;

	bool							attemptRestore;
	uint16						newVerNum,newVerSubNum;

	int							roomState;
	bool							enemyDone;
	bool							meDone;
	bool							bothDone;

	bool							errorPopupCreated;

	//holds main screen stats to be restored if player cancelled from load screen
	//NO -for now lets just keep last loaded char
	//int			backupCharId;
	//int			backupCoverAnim;
	//int			backupFaceAnim;
	//int			backupBodyAnim;
	//int			backupBanner;
	//int			backupCat;
	//char			backupName[40];

	//==============================================================
	//MAKE CHAR STUFF
	//void		SetupMakeChar();

	int			GetCoverAnim(int cover);
	int			GetFaceAnim(int cat, int face);
	int			GetBodyAnim(int body);

	void			LoadCover();
	void			LoadFace();
	void			LoadBody();
	void			LoadBanner();

	void			RandomCover();
	void			RandomFace();
	void			RandomBody();
	void			RandomBanner();

	void			AdvCover(dir_t dir);
	void			AdvFace(dir_t dir);
	void			AdvBody(dir_t dir);
	void			AdvBanner(dir_t dir);

	void			UpdateGender();
	void			UpdateRandom();
	void			LoadName(int sqbRes);
	int			RandomName();	//it returns a sqbRes
	//void		ShiftPal(int bannerColor);

	BAM_Guy		curCoverGuy;
	BAM_Guy		curFaceGuy;
	BAM_Guy		curBodyGuy;
	BAM_Guy		curBannerGuy;


	//make char buttons
	BAM_Button	leftArrowB[4];
	BAM_Button	rightArrowB[4];
	BAM_Button	randomCharB;
	BAM_Button	manB;
	BAM_Button	womanB;
	BAM_Button	otherB;
	BAM_Button	customNameB;
	BAM_Button	randomNameB;
	//BAM_Button	diskB;
	//BAM_Button	doneB;

	BAM_Box		nameBox;
	grip			gNameText;

	int			curCover;
	int			curFace;
	int			curBody;
	int			curNameNum;

	//for centralized tracking when user has changed his char
	int			oldCharId;
	int			oldCoverAnim;
	int			oldFaceAnim;
	int			oldBodyAnim;
	char			oldName[40];

	int			maxMenNameNum;
	int			maxWomenNameNum;
	int			maxOtherNameNum;

	gender_t		coverGen[MAX_COVER];
	gender_t		bodyGen[MAX_BODY];

	//==============================================================
	//LOAD CHAR STUFF

	//load char buttons
	BAM_Button	leftLoadB;
	BAM_Button	rightLoadB;

	bool			SetupCampaign();
	void			GetLoadCharStats();
	void			DisplayLoadChar();
	void			ConfigLoadArrowButtons();

	FILE 			*campFile;
	grip			gCampArr;
	NetCampaignHeader	header;
	int			campCnt;			//array index to currently displayed campaign

	NetCampaign	*pCampArr;
	NetCampaign	*pCamp;	//used alot as tmp ptr into CampArr
};

#endif
