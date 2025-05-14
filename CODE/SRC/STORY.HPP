//	story.hpp
//
//	Copyright 1994, Tachyon, Inc.
//
// Story Line intro screen.
//
//		10-06-94:
//


#ifndef story_hpp
#define story_hpp

#include "alldefs.hpp"
#include "palette.hpp"
#include "apires.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "context.hpp"
#include "scrimage.hpp"
#include "bamroom.hpp"
#include "text.hpp"

#include <time.h>

// 3 regions to each story 0 thru 14
#define  MAXREGIONS	   14
// true max regions
#define	MAXSTORYGUYS   15
// need a outline guy for 15 regions and 5 stories
#define	MAXOUTLINEGUYS 20
// need a flag marker for 15 regions and 5 stories
#define	MAXFLAGGUYS    20

#define  FLASH_INTERVAL	25
#define  CHAR_INTERVAL 6
#define  STORY_NAME_SIZE 22

//WARNING: Delay must be long enough to allow regions to flash a few times
//			  or let animated legend banner unfurl.
#define  STORY_DELAY CLOCKS_PER_SEC*2

class Story : public BAM_Room
{
	public:

	Story(void);
	~Story();

	bool		HandleMsg(Message* pMsg);
 	void		Setup();
	uchar		GetPixel(coord celXOff, coord celYOff, res_t srcType, 
								uint srcNum, uint srcCel);
	uint32	GetStory(uint32 aindex);
	uint32	GetRegion();
	void		Cycle();
	bool		ShowThisStory(int storyNum,int storyDone);


	BAM_Guy		back;		// Background guy
	grip			gback;
	uint			rNumBack;
	BAM_Guy		title;
	grip			gtitle;
	uint			rNumTitle;
	BAM_Guy		inset;
	grip			gInset;
	uint			rNumInset;

	grip			clut;

	SquibRes		sqbStory;
	int			sqbRes;
	int			sqbNum;
	char			*pTxt;
	char			storyNames[MAXOUTLINEGUYS][STORY_NAME_SIZE];
	int			storyDone;

	//region guys
	BAM_Guy		storyGuy[MAXSTORYGUYS];
	BAM_Guy		outlineGuy[MAXOUTLINEGUYS]; //includes region outlines

	//BAM_Guy		flagGuy[MAXFLAGGUYS];
	//int			flagPos[MAXFLAGGUYS*2];

	BAM_Guy		titleGuy[MAXFLAGGUYS];		 //only used in prev conquered regions after first pass!
	int			titlePos[MAXFLAGGUYS*2];	 // "		"
	grip			gtitleCel[MAXFLAGGUYS];		 // "		"
	uint			rNumTitleCel[MAXFLAGGUYS];	 // "		"

	int32			currentGuy; // for cycling
	int			lastVoiceNum;

	TPalette	pal;
	grip		gTCel; 	// Scratch Cel to copy pixels into for GetPixel()

	// button stuff
	BAM_Button	diskButton;

	clock_t		expireTime;
	int			hasSelected;
	int			legendPath[16];
	clock_t		flagFlashTimer;

	int			shot1; //one time only counter in HandleMsg()
	bool			firstPass; //used to track the first pass thru this room
	
	int			flashToggle;

	int			demoRegions[5];

};

#endif
