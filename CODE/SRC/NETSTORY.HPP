//	netstory.hpp
//
//	Copyright 1995, Tachyon, Inc.
//
// net/modem region map selection screen
//
//	8-28-95: Kevin Sherrill
//


#ifndef netstory_hpp
#define netstory_hpp

#include "alldefs.hpp"
#include "api.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "context.hpp"
#include "palette.hpp"
#include "scrimage.hpp"
#include "text.hpp"

#include <time.h>

// 3 regions to each story 0 thru 14
#define  NETMAXREGIONS	   14
// true max regions
#define	NETMAXSTORYGUYS   15
// need a outline guy for 15 regions and 5 stories
#define	NETMAXOUTLINEGUYS 20
// need a flag marker for 15 regions and 5 stories
#define	NETMAXFLAGGUYS    20

#define  FLASH_INTERVAL	25
#define  STORY_NAME_SIZE 22

// the following have been integrated into BAMPacketType in alldefs.hpp
//enum netStoryPacket_t
//{
//	NS_EMPTY,
//	NS_DONE,
//	NS_UNDONE,
//	NS_LAST
//};

enum
{
	NS_LOCAL,
	NS_ENEMY
};

struct NetStoryPacket
{
 	int	frameNum;
	int	newRand;
	int	coordX;
	int	coordY;
	int	curGuy;
};


class NetStory : public BAM_Room
{
	public:

	NetStory(void);
	~NetStory();

	bool		HandleMsg(Message* pMsg);
 	void		Setup();
	uchar		GetPixel(coord celXOff, coord celYOff, res_t srcType, 
							uint srcNum, uint srcCel);
	uint32	GetStory(uint32 aindex);
	uint32	GetRegion();
	void		Cycle();
	void		CycleEnemy();
	bool		SyncWithRemote();
	void		ResetCurrentGuy(int side, int newGuy);


	BAM_Guy		back;		// Background guy
	grip			gback;
	uint			rNumBack;

	BAM_Guy		inset;

	BAM_Guy		title;
	grip			gtitle;
	uint			rNumTitle;

	BAM_Guy		enemyTitle;
	grip			gEnemyTitle;
	uint			rNumEnemyTitle;

	grip			clut[2];

	SquibRes		sqbStory;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;
	char			storyNames[NETMAXOUTLINEGUYS][STORY_NAME_SIZE];

	//region guys
	BAM_Guy		storyGuy[NETMAXSTORYGUYS];
	BAM_Guy		outlineGuy[NETMAXOUTLINEGUYS]; //includes region outlines
	BAM_Guy		flagGuy[NETMAXFLAGGUYS];
	int			flagPos[NETMAXFLAGGUYS*2];
	int			titlePos[NETMAXFLAGGUYS*2];
	BAM_Guy		enemyFlagGuy[NETMAXFLAGGUYS];

	BAM_Guy		enemyCursor;
	int			enemyCurX,enemyCurY;

	BAM_Button	diskButton;

	int32			currentGuy;		 	//	holds region number cursor is now above
	int32			enemyCurrentGuy;	// holds region number enemy cursor is above

	TPalette	pal;
	grip		gTCel; 	// Scratch Cel to copy pixels into for GetPixel()

	bool			hasSelected;		//region user has clicked
	bool			enemyHasSelected;

	clock_t		flagFlashTimer;
	clock_t		enemyFlagFlashTimer;

	int			flashToggle;
	int			enemyFlashToggle;

	int							currFrame;
	struct sPacket				sPacket1;
	struct NetStoryPacket	nsPacket;
	BAMPacketType				packetType;	//formerly type netStoryType_t - Van
	bool							worldEnder;

	bool							syncedRand;
	int							curRand;
	int							newRand;
	int							receivedRand;

	int							sendErrCnt;
	bool							syncingNewRoom;

	coord 						theX;
	coord 						theY;

	int							errorSqbNum;
};

#endif
