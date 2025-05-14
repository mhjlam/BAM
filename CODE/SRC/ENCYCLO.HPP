// ENCYCLO.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup encyclopedia.
//
// 5/11/95
//

#ifndef encyclo_hpp
#define encyclo_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "encymenu.hpp"

#include "mouse.hpp"
#include "rect.hpp"
#include "tigre.hpp"
#include "text.hpp"

//array sizes for each cat == (pages in cat)+1
#define ENCY_MAX_CREATURE	 34

//#define ENCY_MAX_STRUCTURE	 27	(unique numbers, but there are repeats)
// 40's a rough number -the sort will reset the page max
#define ENCY_MAX_STRUCTURE	 40

#define ENCY_MAX_ITEM		 25
#define ENCY_MAX_TERRAIN	 22

#define ENCY_MAX_INTER_GENERAL	21
#define ENCY_MAX_INTER_SPECIFIC 18

//note: multiple entries may point to same page
#define ENCY_MAX_INTERFACE_PAGES	 33


// For Sound or Music
class Encyclopedia : public BAM_RM_Popup
{
	public:

	Encyclopedia(void);
	~Encyclopedia();

	bool		HandleMsg(Message* pMsg);
	void		Setup(grip gPal_P, int mouseX, int mouseY);
	void		Setup(grip gPal_P, TargetType TType, int enumNum);
 	void		Setup(encyclo_t menuType_P, int page_P=0);
 	void		SetupBadClick();
	void		Cycle();
	void		Cleanup();
	void		CleanupBadClick();

	void		NextPage();
	void		SetupCreaturePage();
	void		SetupStructurePage();
	void		SetupItemPage();
	void		SetupTerrainPage();
	void		SetupInterfacePage();
	int 		CvtCreature(int enumNum);
	int 		CvtItem(int enumNum);
	void		CvtTerrain(int enumNum, encyclo_t *menuType, int *page);
	int 		CvtInterface(int mouseX, int mouseY);

	int		FindCreatureEnum(int pageL);
	int		FindItemEnum(int pageL);

	int		FindTerrPage(int type, int enumOrSqb);
	void		SortTerrains();

	int		FindStructPage(int type, int enumOrSqb);
	void		SortStructures();

	void		SetPage(int dir);

	FadeTo			fadeTo;
	bool				doFade;
	grip				gPal;
	grip				gInter;
	uint				rNumInter;
	Rectangle		rInter;

	Rectangle		oldMouseLimits;
	res_t				oldMouseResType;
	uint32			oldMouseResNum;
	uint32			oldMouseCel;

	SquibRes			sqbEncyclo;
	int				sqbRes;
	int				sqbNum;
	char 				*pTxt;

	encyclo_t		menuType;
	int				page,maxPageNum;
	int				arrPos;	//used for INTERFACE pages only
	bool				fFoundGenInterArr;	//in which Interface array did we
													//find the page
	int				animNum;

	BAM_Guy			back;
	grip				gback;
	CelHeader		*pbackCH;
	uint				rNumBack;
	grip				gbackAnim;
	Rectangle		rback;
	BAM_Guy			topBorder;
	BAM_Guy			sideBorder;

	int				prevFont;
	uchar				saveColor[7];

	BAM_Button		button[3];

	int 				creatureCvt[ENCY_MAX_CREATURE];
	int 				itemCvt[ENCY_MAX_ITEM];
	int 				terrainCvt[TERRAINMAXTYPES][3];

	int 				creaturePortNum[ENCY_MAX_CREATURE];
	int 				itemPortNum[ENCY_MAX_ITEM];
	//int 				structurePortNum[ENCY_MAX_STRUCTURE];
	//int 				terrainPortNum[ENCY_MAX_TERRAIN];

	int 				activeInterGenRegions[ENCY_MAX_INTER_GENERAL][5];
	int 				copyInterGenRegions[ENCY_MAX_INTER_GENERAL][4];

	int 				activeInterSpecRegions[ENCY_MAX_INTER_SPECIFIC][6];
	int 				copyInterSpecRegions[ENCY_MAX_INTER_SPECIFIC][4];

	int				structureSort[ENCY_MAX_STRUCTURE][2];
	bool				structSort;
	int				structSortSize;

	int				terrainSort[ENCY_MAX_TERRAIN][2];
	bool				terrSort;
	int				terrSortSize;

	bool				questionMarkCursor; //is this how we entered encyclo?
	bool				badClick;
	clock_t			cleanupTime;
	clock_t			newTime;

	encyclo_t		anotherMenuType;
	bool				fAnotherPage;
	int				anotherPage;
	int				anotherPort;
};

#endif

