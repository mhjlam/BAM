// ENCYCLO.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
//
// A popup encyclopedia.
//
// 5/11/95
//

#include "ENCYCLO.HPP"

#include "apifont.hpp"
#include "apires.hpp"
#include "apigraph.hpp"
#include "graphmgr.hpp"
#include "bam.hpp"
#include "bamfuncs.hpp"
#include "context.hpp"
#include "fontmgr.hpp"
#include "mouse.hpp"
#include "rect.hpp"
#include "sconfig.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"
#include "tilelib.hpp"
#include "units.hpp"
#include "unitlib.hpp"
#include "viewport.hpp"
#include "world.hpp"

#include <string.h>

#define	CLEANUP_DELAY	15
#define	NULLPAGE			(-1)

#define	ENCY_WIN_X		20
#define	ENCY_WIN_Y		38
#define	ENCY_BASE_PRI	21000

#define	ENCY_GENERIC_SQBRES			5000
#define	ENCY_CREATURE_SQBRES_BASE	5000
#define	ENCY_STRUCTURE_SQBRES		5400
#define	ENCY_ITEM_SQBRES	 			5500
#define	ENCY_TERRAIN_SQBRES			5600
#define	ENCY_INTERFACE_SQBRES		5700

//define char. in font for these symbols
#define	ENCY_UP_ARROW		29
#define	ENCY_LEFT_ARROW	30

// interface elements -> page
#define ENCY_INTER_SCENE_MAP		2
#define ENCY_MESSAGE_BOX			11
#define ENCY_INTER_PORTRAIT		6
#define ENCY_INTER_GROUP_PORTRAIT 34
#define ENCY_INTER_ITEM_PORT		7
#define ENCY_INTER_DEST_PORTRAIT	31
#define ENCY_INTER_TRANS_PAGE		1
#define ENCY_INTER_UPPER_BAR		26
#define WEIRD_BUTTON_1		23
#define WEIRD_BUTTON_2		24
#define WEIRD_BUTTON_3		25

//pos. in array of first transform portrait
#define ENCY_INTER_TRANS_BASE		13

//NOTE: THE INTERFACE REGION ARRAYS ARE FULL OF COORDS BASED ON 320X400
//viewport centering coords for posting interface graphic
#define ENCY_INTER_CTR_X	109
#define ENCY_INTER_CTR_Y	59

//item -> page
#define ENCY_WEIRD_WARD		24

//imaginary unit created to portray basal golem in tranformation
#define ENCY_METAPOD_UNIT		(ENCY_MAX_CREATURE +1)
//this unit's sqb
#define ENCY_METAPOD			17

#define ENCY_TURRET_PORTRAIT	2724
#define ENCY_INTER_TURRET	21

#define E1	0x01
#define E2	0x02
#define E3	0x04
#define E4	0x08

#define ENCY_PREV		-1
#define ENCY_NEXT		1

//for FindTerrPage()
enum
{
	ENCY_TERR_ENUM=0,
	ENCY_TERR_SQB
};

enum
{
	ENCY_STRUCT_ENUM=0,
	ENCY_STRUCT_SQB
};

int CompareTerr(void const *pparm1,void const *pparm2);
int CompareBuilding(void const *pparm1,void const *pparm2);

//=========================================================

Encyclopedia::Encyclopedia()
{
	int	i,j;

	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = 0;
	gPal = 0;
	gInter = 0;
	doFade = FALSE;
	badClick = FALSE;
	questionMarkCursor = FALSE;

	structSort = FALSE;
	terrSort = FALSE;
	fAnotherPage = FALSE;
	anotherPage = 0;

	//starts at one so as to be equiv. to page number
	// this is the layout of the creature encyclopedia pages
	// Van's unitType layout of creatures can be converted to this format
	// with the function CvtCreature()
	int TcreaturePortNum[ENCY_MAX_CREATURE] =
	{
		  	0,	 //none
	  	2552,	 //BANSHEE
	  	2550,	 //BASAL GOLEM
	  	2556,	 //BRIGAND
	  	2558,	 //CLERIC
	  	2564,	 //DRUID
	  	2594,	 //ENCHANTER
	  	2566,	 //FURY
	  	2568,	 //GARGOYLE
	  	2570,	 //GHOUL
	  	2572,	 //GNOME
	  	2554,	 //GOBLIN
	  	2576,	 //GORGON
	  	2578,  //GRIFFIN
		2614,  //GUARDIAN
	  	2612,	 //HARPY
	  	2580,	 //JUGGERNAUT
		2616,  //METAPOD
	  	2582,	 //NYMPH
	  	2584,	 //OBELISK
	  	2586,	 //PALADIN
	  	2560,	 //PEASANT
	  	2588,	 //RANGER
	  	2590,	 //ROC
	  	2593,	 //ROC_EGG
	  	2610,	 //SNAPPING TURTLE
	  	2574,	 //STONE GOLEM
	  	2596,	 //TROLL
	  	2598,	 //WARRIOR
	  	2600,	 //WIZARD
	  	2602,	 //WOLF
	  	2604,	 //WRAITH
	  	2606,	 //WYRM
	  	2608   //ZOMBIE
	};

	for(i=0;i<ENCY_MAX_CREATURE;i++)
	{
		creaturePortNum[i] = TcreaturePortNum[i];
	}

	//for converting from enum to sqb
	int TcreatureCvt[ENCY_MAX_CREATURE] =
	{
		0,		//none
	  	2,		//BASAL GOLEM
	  	1,		//BANSHEE
	  	3,		//BRIGAND
	  	4,		//CLERIC
	  	5,		//DRUID
	  	6,		//ENCHANTER
	  	7,		//FURY
	  	8,		//GARGOYLE
	  	9,		//GHOUL
	  	10,	//GNOME
	  	11,	//GOBLIN
	  	26,	//(STONE) GOLEM
	  	12,	//GORGON
	  	13,	//GRIFFIN
		14,	//GUARDIAN
	  	15,	//HARPY
	  	16,	//JUGGERNAUT
	  	18,	//NYMPH
	  	19,	//OBELISK
	  	20,	//PALADIN
	  	21,	//PEASANT
	  	22,	//RANGER
	  	23,	//ROC
	  	24,	//ROC_EGG
	  	25,	//SNAPPING TURTLE
	  	27,	//TROLL
	  	28,	//WARRIOR
	  	29,	//WIZARD
	  	30,	//WOLF
	  	31,	//WRAITH
	  	32,	//WYRM
	  	33,	//ZOMBIE
		 0		//metapod (sqb = 18)
	};

	for(i=0;i<ENCY_MAX_CREATURE;i++)
	{
		creatureCvt[i] = TcreatureCvt[i];
	}


	//==============================================
	//for  sqb -> portrait reference
	//and limited port -> sqb reference (beware generic port 2720)
	//int TstructurePortNum[ENCY_MAX_STRUCTURE] =
	//{
	//   	0,
	//	2726,		//aqueduct
	//	2700,		//arbor lodge
	//	2742,		//banishing stone
	//	2702,		//barracks
	//	2674,		//bloodforge
	//	2720,		//bridge
	//	2704,		//camp
	//	2688,		//cauldron
	//	2708,		//crypt
	//	2738,		//foundation
	//	2714,		//gate
	//	2722,		//great wall
	//	2714,		//keep
	//	2666,		//mine
	//	2720,		//monument
	//	2744,		//portals
	//	2716,		//runestone
	//	2720,		//stockpile
	//	2720,		//structure
	//	2718,		//temple
	//	2724,		//turret
	//	2722,		//wall
	//	2736,		//wind wall
	//	2720,		//great column
	//	2720,		//great furnace
	//	2674		//well of immortals
	//};
	//
	//for(i=0;i<ENCY_MAX_STRUCTURE;i++)
	//{
	//	structurePortNum[i] = TstructurePortNum[i];
	//}


	//==============================================
	int TitemPortNum[ENCY_MAX_ITEM] =
	{
	   	0, //page 0 = unused
		2500,	//BERSERKER BREW
		2502,	//BOAR BURGER
		2504,	//BOG BOOTS
		2525,	//COMPONENT
		2506,	//DAEMON'S BANE
		2508,	//DOWRY CHEST
		2510,	//DRIFT DISC
		2512,	//FROST CAPE
		2544, //HARPY NET
		2514,	//HEALING SALVE
		2516,	//KEY
		2542,	//MANA ORB
		2518,	//MAP
		2540,	//MASON MIX
		2520,	//MIGHT MANTLE
		2522,	//SACRED URN
		2526,	//SHRUB SPRITE
		2528,	//STORM BRACERS
		2530,	//TELEPORT TOME
		2532,	//TRAILFINDER
		2534,	//VERDANT SHIELD
		2536,	//VIRTUE VEIL
		2538,	//WEIRD WAND
		2546  //WEIRD WARD
	};

	for(i=0;i<ENCY_MAX_ITEM;i++)
	{
		itemPortNum[i] =	TitemPortNum[i];
	}

	//for converting enum into sqb (currently sqb=page)
	int TitemCvt[ENCY_MAX_ITEM] =
	{	0,
		1,  	//BERSERKER_BREW
		2, 	//BOAR_BURGER
		3,  	//BOG_BOOTS
		5,  	//DAEMONS_BANE
		6,  	//DOWRY_CHEST
		7,  	//DRIFT_DISC
		8,  	//FROST_CAPE
		10,  	//HEALING_SALVE
		11,  	//KEY
		13,	//MAP
		15,	//MIGHT_MANTLE
		16,	//SACRED_URN
		4,		//COMPONENT
		17,	//SHRUB_SPRITE
		18,	//STORM_BRACERS
		19,	//TELEPORT_TOME
		20,	//TRAILFINDER
		21,	//VERDANT_SHIELD
		22,	//VIRTUE_VEIL
		23,	//WEIRD_WAND
		14,	//MASON_MIX
		12,	//MANA_ORB
		 9,	//HARPY_NET
	};

	for(i=0;i<ENCY_MAX_ITEM;i++)
	{
		itemCvt[i] =	TitemCvt[i];
	}


	//==============================================
	//not currently in use
	//int TterrainPortNum[ENCY_MAX_TERRAIN] =
	//{
	//		0,
	//	2650,	//BRAMBLES
	//	2668,	//BROKEN_LAND
	//	2652,	//CHASM
	//	2656,	//FLAME_SPOUT
	//	2658,	//FOLIAGE
	//	2686,	//HIGH_FOLIAGE
	//	2662,	//LAVA_FLOW or HOTBED
	//	2664,	//MARSH
	//	2684,	//OBSTRUCTION
	//	2672,	//OPEN_LAND
	//	2670,	//ROAD
	//	2592,	//ROC_NEST
	//	2660,	//ROUGH_LAND
	//	2676,	//RUINS
	//	2678,	//SWAMP
	//	2728,	//ATLANTEAN_RUIN or UNDERSEA_TOMB
	//	2680,	//WATER
	//	2720,	//PLATFORM
	//	2748,	//PRESSURE_PLATE
	//	2746,	//PRISON_PIT
	//	2748,	//TRIP_SWITCH
	//};
	////2682,		//WHIRLPOOL
	//
	//for(i=0;i<ENCY_MAX_TERRAIN;i++)
	//{
	//	terrainPortNum[i] =	TterrainPortNum[i];
	//}

	//STRUCTURE = 1 and TERRAIN = 3 in each first slot
	int TterrainCvt[TERRAINMAXTYPES][3] =
	{//ENUM,TYPE,SQB
	 0,0,0,		//UNDEFINED = 0,
	 1,3,1,		//BRAMBLES		//2650
	 2,3,3,		//CHASM			//2652
	 3,3,4,		//FLAME_SPOUT	//2656
	 4,3,5,		//FOLIAGE		//2658
	 5,3,13,		//ROUGH_LAND	//2660
	 6,3,7,		//LAVA_FLOW		//2662
	 7,3,8,		//MARSH			//2664
	 8,1,14,		//MINE			//2666
	 9,3,2,		//BROKEN_LAND	//2668
	10,3,11,		//ROAD			//2670

	11,3,10,		//OPEN_LAND		//2672
	12,1,5,		//PORTAL			//2674
	13,3,14,		//RUINS			//2676
	14,3,15,		//SWAMP			//2678
	15,3,17,		//WATER			//2680
	16,0,0,		//WHIRLPOOL		//2682
	17,3,6,		//HIGH_FOLIAGE	//2686
	18,3,9,		//OBSTRUCTION	//2684
	19,0,0,		//FOG_TERR
	20,3,12,		//ROC_NEST

	21,0,0,		//TERR21
	22,0,0,		//TERR22
	23,0,0,		//TERR23
	24,0,0,		//TERR24
	25,0,0,		//TERR25
	26,0,0,		//TERR26
	27,0,0,		//TERR27
	28,0,0,		//TERR28
	29,0,0,		//TERR29
	30,1,2,		//ARBOR_LODGE	//2700

	31,1,4,		//BARRACKS		//2702
	32,1,7,		//CAMP			//2704
	33,1,9,		//CRYPT			//2708
	34,1,11,		//GATE			//2714
	35,1,13,		//KEEP			//2714
	36,1,17,		//RUNESTONE		//2716
	37,1,20,		//TEMPLE			//2718
	38,1,19,		//STRUCTURE		//2720
	39,1,22,		//WALL			//2722
	40,1,15,		//MONUMENT

	41,1,6,		//BRIDGE
	42,1,18,		//STOCKPILE
	43,1,12,		//GREAT_WALL
	44,0,0,	//1,10,		//FOUND_ARBORLODGE
	45,0,0,	//1,10,		//FOUND_BARRACKS
	46,0,0,	//1,10,		//FOUND_CRYPT
	47,0,0,	//1,10,		//FOUND_RUNESTONE
	48,0,0,	//1,10,		//FOUND_TEMPLE
	49,1,8,		//CAULDRON		//2688
	50,3,16,		//ATLANTEAN_RUIN

	51,1,1,		//AQUEDUCT
	52,0,0,		//TERR52
	53,1,25,		//GREAT_FURNACE
	54,1,24,		//GREAT_COLUMN
	55,3,21,		//TRIP_SWITCH
	56,3,19,		//PRESSURE_PLATE
	57,1,23,		//WIND_WALL
	58,1,16,		//GUARDIAN_PORTAL
	59,1,16,		//GOLEM_PORTAL
	60,1,21,		//TURRET

	61,1,21,		//TURRET1
	62,1,21,		//TURRET2
	63,1,21,		//TURRET3
	64,1,21,		//TURRET4
	65,1,21,		//TURRET5
	66,1,21,		//TURRET6
	67,1,21,		//TURRET7
	68,1,21,		//TURRET8
	69,1,21,		//TURRET9
	70,1,21,		//TURRET10

	71,1,21,		//TURRET11
	72,1,21,		//TURRET12
	73,1,21,		//TURRET13
	74,1,21,		//TURRET14
	75,1,21,		//TURRET15
	76,1,16,		//PALADIN_PORTAL
	77,1,16,		//WARRIOR_PORTAL
	78,1,26,		//WELL_OF_IMMORTALS
	79,3,18,		//PLATFORM
	80,1,10,		//FOUND_GENERIC

	81,0,0,		//BRIDGE2
	82,1,3,		//BANISH_STONE
	83,3,20 		//PRISON_PIT
					//TERRAINMAXTYPES 84
	};

	for(i=0;i<TERRAINMAXTYPES;i++)
	{
		for(j=0;j<3;j++)
		{
			terrainCvt[i][j] =	TterrainCvt[i][j];
		}
	}


	//==============================================
	int TactiveInterGenRegions[ENCY_MAX_INTER_GENERAL][5] =
	{
		240,  0,319, 19, 27, //Research Button
		240, 20,319, 41, 28, //Experience gauge
		240, 42,319,165,  2, //Scenario Map -downsize 50%
		240,166,319,183,  3, //Mana Gauge
		240,184,281,213, 12, //Pause Button
		282,184,319,213, 14, //Options Button
		282,214,319,243, 15, //Encyclopedia Button
		240,244,281,273, 13, //Win/Lose Button
		282,274,319,303, 16, //Send Message Button
		  0,304, 39,319, 29, //Ownership banner
		 40,304,212,319,  5, //Main Name Bar
		213,304,319,319,  4, //Item Name Bar
		 38,320,121,334, 17, //Life Gauge
		 38,335,121,350, 18, //Attack Gauge
		 38,351,121,366, 19, //Defense Gauge
		 38,367,121,387, 20, //Move Gauge
		240,274,281,303,  9, //Remove Button
		282,244,319,273, 10, //GO SEE Button
		  0,388,319,399, 11, //Message Window
		  0,  0,  0,  0, 26, //Map Title Bar
		  0,  0,  0,  0, 34  //Group Portrait Special page -ugly, but it works
	};


	for(i=0;i<ENCY_MAX_INTER_GENERAL;i++)
	{
		for(j=0;j<5;j++)
		{
			activeInterGenRegions[i][j] = TactiveInterGenRegions[i][j];
		}
	}


	int TactiveInterSpecRegions[ENCY_MAX_INTER_SPECIFIC][6] =
	{
		122,320,156,382, 23,E1,    //MOVE TO
		157,320,191,382, 24,E1,    //ATTACK
		122,350,191,387, 25,E1,    //GUARD
		192,320,281,353, 21,E1,    //SPELL
		192,354,281,387, 22,E1,    //TRANSFORM
		122,320,159,387, 31,E2,    //Dest. Unit Portrait
		160,320,249,353, 32,E2,    //Transformation Gauge
		250,320,281,387, 32,E2,    //Transformation Gauge Cont.
		160,354,249,387, 33,E2,    //Cancel Transformation Button
		  0,320, 37,387,  6,E1|E2, //Portrait Button
		282,320,319,370,  7,E1|E2, //Item Portrait
		282,371,319,387,  8,E1|E2, //Drop Button
		  0,320, 37,387, 30,E3|E4, //Exit Menu Button
		122,320,159,387,  1,E3|E4, //Trans. opt. 1 Portrait Button
		160,320,197,387,  1,E3|E4, //Trans. opt. 2 Portrait Button
		198,320,235,387,  1,E3|E4, //Trans. opt. 3 Portrait Button
		236,320,273,387,  1,E3|E4, //Trans. opt. 4 Portrait Button
		274,320,319,387,  1,E3|E4  //Trans. opt. 5 Portrait Button
	};


	for(i=0;i<ENCY_MAX_INTER_SPECIFIC;i++)
	{
		for(j=0;j<6;j++)
		{
			activeInterSpecRegions[i][j] = TactiveInterSpecRegions[i][j];
		}
	}

	int TcopyInterGenRegions[ENCY_MAX_INTER_GENERAL][4] =
	{
		261,  0,319, 17,  //Research Button
		240, 20,319, 41,  //Experience gauge
		253, 42,319,165,  //Scenario Map -downsize 50%
		253,166,319,183,  //Mana Gauge
		253,184,281,213,  //Pause Button
		282,184,310,213,  //Options Button
		282,214,310,243,  //Encyclopedia Button
		253,244,281,273,  //Win/Lose Button
		282,274,310,303,  //Send Message Button
		  0,306, 39,316,  //Ownership banner
		 40,306,185,316,  //Main Name Bar
		218,306,319,316,  //Item Name Bar
		 43,320,116,332,  //Life Gauge
		 40,335,119,350,  //Attack Gauge
		 40,351,119,366,  //Defense Gauge
		 40,367,119,382,  //Move Gauge
		253,274,281,303,  //Remove Button
		282,244,310,273,  //GO SEE Button
		  0,  0,  0,  0,  //Message Window
		 40,  0,200, 32,  //Map Title Bar
		  0,320, 37,387   //Group Portrait Special page -ugly, but it works
	};


	for(i=0;i<ENCY_MAX_INTER_GENERAL;i++)
	{
		for(j=0;j<4;j++)
		{
			copyInterGenRegions[i][j] = TcopyInterGenRegions[i][j];
		}
	}


	int TcopyInterSpecRegions[ENCY_MAX_INTER_SPECIFIC][4] =
	{
		122,320,156,382,  //MOVE TO
		157,320,191,382,  //ATTACK
		122,350,191,387,  //GUARD
		192,320,281,353,  //SPELL
		192,354,281,387,  //TRANSFORM
		122,320,159,387,  //Dest. Unit Portrait
		162,323,249,350,  //Transformation Gauge
		252,323,279,384,  //Transformation Gauge Cont.
		160,354,249,387,  //Cancel Transformation Button
		  0,320, 37,387,  //Portrait Button
		282,320,319,370,  //Item Portrait
		282,371,319,387,  //Drop Button
		  0,320, 37,387,  //Exit Menu Button
		122,320,159,387,  //Trans. opt. 1 Portrait Button
		160,320,197,387,  //Trans. opt. 2 Portrait Button
		198,320,235,387,  //Trans. opt. 3 Portrait Button
		236,320,273,387,  //Trans. opt. 4 Portrait Button
		274,320,311,387   //Trans. opt. 5 Portrait Button
	};


	for(i=0;i<ENCY_MAX_INTER_SPECIFIC;i++)
	{
		for(j=0;j<4;j++)
		{
			copyInterSpecRegions[i][j] = TcopyInterSpecRegions[i][j];
		}
	}
}

Encyclopedia::~Encyclopedia()
{
	// everything that used to be here has been moved to Cleanup()
	// this is so we can immediately delete ourselves if the user
	// has mouse clicked on a part of the interface with no matching
	// encyclopedia page.
}

//main Setup function -it calls other setup functions if cursor is in viewport
//	otherwise it handles interface pages.
void
Encyclopedia::Setup(grip gPal_P,int mouseX, int mouseY)
{
	BAMItem	*pItem;
	int 		terrain;
	Unit		*pUnit;

	BAM_Ani	*pAni;
	grip		gAni;
	BAM_Ani	*pbestAni;
	grip		gbestAni = NULL;
	int		bestPri = 0;
	int		targX,targY;
	int		i;

	gPal = gPal_P;

	questionMarkCursor = TRUE;

	if(pWorld->vPort.rInner.Contains(mouseX,mouseY))
	{
		// +1 for viewX & viewY starting at -1 instead of 0
		targX = pWorld->vPort.ViewX + mouseX / TILE_WIDTH +1;
		targY = pWorld->vPort.ViewY + mouseY / TILE_HEIGHT +1;

		//is the target the upper map name bar?
		if(targY == -1)
		{
			doFade = TRUE;
		 	Setup(INTERFACE,ENCY_INTER_UPPER_BAR);
			return;
		}

		if(pWorld->vPort.fog[targY][targX] & FOG_CENTER)
		{
			SetupBadClick();
			return;
		}

		gAni = (grip)pWorld->vPort.lScreenAnims.FirstValue();

		while(gAni)
		{
			if(pMemMgr->CheckGrip(gAni) == GRIP_VALID)
			{
				pAni = ADerefAs(BAM_Ani, gAni);
				if (pAni->scrim.rect.Contains(mouseX,mouseY))
				{
					if(pAni->selfType == UNIT || pAni->selfType == ITEM &&
						pAni->scrim.priority > bestPri)
					{
						gbestAni = gAni;
						pbestAni = pAni;
						bestPri = pAni->scrim.priority;
					}
				}
			}
			gAni = (grip)pWorld->vPort.lScreenAnims.NextValue();
		}

		//we found something in the list
		if(gbestAni)
		{
			if(pbestAni->selfType == UNIT)
			{
				pUnit = ADerefAs(Unit,gbestAni);
				if(pUnit->type == ACOLYTE && pUnit->currAction == TRANSFORM)
					Setup(gPal,UNIT,ENCY_METAPOD_UNIT);
				else
				if(pUnit->type == UNIT_WEIRD_WARD)
				{
					//lets show its ITEM page
					Setup(gPal,ITEM,ENCY_WEIRD_WARD);
				}
				else
					Setup(gPal,UNIT,pUnit->type);

				return;

			}
			else if(pbestAni->selfType == ITEM)
			{
				pItem = ADerefAs(BAMItem,gbestAni);
				Setup(gPal,ITEM,pItem->type);
				return;
			}

			//should never get here.
			SetupBadClick();
			return;
		}
		else
		{
			terrain = pWorld->map.GetTerrainNum(targX, targY);

			Setup(gPal,TERRAIN,terrain);
			return;
		}
	}
	else
	{
		//in case var. anotherPage is set to a terrain or structure
		SortTerrains();
		SortStructures();

		//a reclick on the '?' button -bail
		if(mouseX > 262 && mouseX < 290 && mouseY > 206 && mouseY < 235)
		{
			delete this;
			return;
		}

		page = CvtInterface(mouseX,mouseY);

		if(!page)
		{
			SetupBadClick();
		}
		else
		{
			BAM_Button	*pButton;

			//is this page the portrait button?
			// or the destination portrait of a metapod?
			// or the transform choice button for a unit or a building?
			//then setup to show that page next.

			if(page == ENCY_INTER_PORTRAIT)
			{
				switch(pWorld->vPort.targType)
				{
					case UNIT:
						fAnotherPage = TRUE;
						if(pWorld->vPort.groupSize)
						{
							anotherMenuType = INTERFACE;
							anotherPage = ENCY_INTER_GROUP_PORTRAIT;
							break;
						}

						//lets get the transforming guy
						pUnit = (Unit*)pWorld->DerefSerial(pWorld->vPort.targSerial);

						anotherMenuType = CREATURE_BUTTON;
						//page = sqb so convert enum to sqb
						if(pWorld->statBoxCurrMode == TRANSFORMING)
							anotherPage = CvtCreature(ENCY_METAPOD_UNIT);
						else
							anotherPage = CvtCreature(pUnit->type);
						break;

					case BUILDING:
					case TERRAIN:
						//we know that the portrait shows whats under the target cursor
						fAnotherPage = TRUE;
						targX = pWorld->vPort.targX;
						targY = pWorld->vPort.targY;
						terrain = pWorld->map.GetTerrainNum(targX, targY);
						CvtTerrain(terrain,&anotherMenuType,&anotherPage);
						break;
				}
			}
			else
			if(page == ENCY_INTER_TRANS_PAGE &&
				pWorld->statBoxCurrMode == UNIT)
			{
				pButton = ADerefAs(BAM_Button, pWorld->gTransformButton[arrPos - ENCY_INTER_TRANS_BASE]);
				
				//lets check for turret
				if(pButton->scrim.resNum == ENCY_TURRET_PORTRAIT)
				{
				 	anotherMenuType = STRUCTURE_BUTTON;
					fAnotherPage = TRUE;
					anotherPage = FindStructPage(ENCY_STRUCT_SQB,ENCY_INTER_TURRET);
				}
				else
				{
					for(i=0;i<ENCY_MAX_CREATURE;i++)
					{
						if(pButton->scrim.resNum == creaturePortNum[i])
						{
							anotherMenuType = CREATURE_BUTTON;
							fAnotherPage = TRUE;
							anotherPage = i;
					 		break;
						}
					}
				}
			}
			else
			if(page == ENCY_INTER_TRANS_PAGE &&
				pWorld->statBoxCurrMode == BUILDING)
			{
				pButton = ADerefAs(BAM_Button, pWorld->gTransformButton[arrPos - ENCY_INTER_TRANS_BASE]);

				for(i=0;i<pWorld->pTileLib->totalTerrains;i++)
				{
					if(pButton->scrim.resNum == pWorld->pTileLib->terrains[i].portraitNum)
					{
						// we found the terrain enum

						anotherMenuType = STRUCTURE_BUTTON;
						fAnotherPage = TRUE;
						anotherPage = FindStructPage(ENCY_STRUCT_ENUM,i);
					 	break;
					}
				}
			}
			else
			if(page == ENCY_INTER_DEST_PORTRAIT &&
				pWorld->statBoxCurrMode == TRANSFORMING)
			{

				//lets get the transforming guy
				pUnit = (Unit*)pWorld->DerefSerial(pWorld->vPort.targSerial);

				anotherMenuType = CREATURE_BUTTON;
				//page = sqb so convert enum to sqb
				fAnotherPage = TRUE;
				anotherPage = CvtCreature(pUnit->metaPodType);

			}
			else
			//is this page the item button? then setup to show item too.
			if(page == ENCY_INTER_ITEM_PORT)
			{
				pButton = ADerefAs(BAM_Button, pWorld->gItemPortrait);

				for(i=0;i<ENCY_MAX_ITEM;i++)
				{
					if(pButton->scrim.resNum == itemPortNum[i])
					{
						anotherMenuType = ITEM_BUTTON;
						fAnotherPage = TRUE;
						anotherPage = i;
					 	break;
					}
				}
			}

			doFade = TRUE;
 			Setup(INTERFACE,page);
		}
	}
}


void
Encyclopedia::Setup(grip gPal_P,TargetType TType, int enumNum)
{
	encyclo_t menuType;
	int		 page;

	gPal = gPal_P;
	doFade = TRUE;

	switch(TType)
	{
		case TERRAIN:
			CvtTerrain(enumNum,&menuType,&page);
			if(page == NULLPAGE)
				SetupBadClick();
			else
				Setup(menuType,page);
			break;

		case ITEM:
			menuType = ITEM_BUTTON;
			page = CvtItem(enumNum);
			if(!page)
				SetupBadClick();
			else
				Setup(menuType,page);
			break;

		case UNIT:
			menuType = CREATURE_BUTTON;
			page = CvtCreature(enumNum);
			if(!page)
				SetupBadClick();
			else
				Setup(menuType,page);
			break;

		default:
			APanic("Encyclo: Bad TargetType");
			break;
	}
}

void
Encyclopedia::SetupBadClick()
{
	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		mode = M_MODELESS;
	else
		mode = M_MODAL;

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,8);

	badClick = TRUE;
	cleanupTime = clock() + CLEANUP_DELAY;

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void *)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;			// grip of currently active button, if any
}


void
Encyclopedia::Setup(encyclo_t menuType_P, int page_P)
{
	BAM_Guy		*pGuy;
	BAM_Button  *pButton;
	uchar			*pback;
	CelHeader	*pbackAnimCH;
	int			i;

	menuType = menuType_P;

	//if item or creature default starting page to 1 else 0.
	if(menuType == ITEM_BUTTON || menuType == CREATURE_BUTTON)
		page = 1;
	else
		page = 0;

	if(page_P)
		page = page_P;


	//======================================================
	prevFont = pFontMgr->curFontNum;
	if(prevFont != 9050)
		pFontMgr->SetRes(9050);

	//======================================================

	//ADump();
	//pMemMgr->Dump(1, "Start Encyclopedia::Setup");

	if(bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
	{
		mode = M_MODELESS;
	}
	else
	{
		mode = M_MODAL;
		bGlobal.roomMgr.curRoom->Pause(TRUE);
	}

	oldMouseResType = pMouse->GetResType();
	oldMouseResNum = pMouse->GetResNum();
	oldMouseCel = pMouse->GetCel();
	pMouse->SetRes(RES_ANIM,POINTER_RES,1);
	oldMouseLimits.Copy(&pMouse->mouseLimits);

	//======================================================
	switch(menuType)
	{
		case CREATURE_BUTTON:
		case STRUCTURE_BUTTON:
		case ITEM_BUTTON:
			animNum = 8300;
			break;

		case TERRAIN_BUTTON:
			animNum = 8310;
			break;
		case INTERFACE:
			animNum = 8312;
			break;
	}


	TRACK_MEM("Encyclo: background anim");
	gbackAnim = ALoad(RES_ANIM,animNum);
	pbackAnimCH = (CelHeader *)AGetResData(gbackAnim);
	rback.Set(ENCY_WIN_X,ENCY_WIN_Y,ENCY_WIN_X+pbackAnimCH->width-1,ENCY_WIN_Y+pbackAnimCH->height-1);

	//---------------------------------------
	//already faded by encymenu unless entering from ('?') mode
	if(menuType == INTERFACE)
	{
	 //we must get a copy of the screen before we fade screen -unless
	 // its a weird button (we'll load an anim for those) or the message box
	 if(page != WEIRD_BUTTON_1 &&
	 	 page != WEIRD_BUTTON_2 &&
	 	 page != WEIRD_BUTTON_3 &&
	 	 page != ENCY_MESSAGE_BOX)
	 {
		CelHeader	*pDest,*pInter;
		uchar			*pDestData,*pInterData;
		grip			gDest;
		uint			rNumDest;
		Rectangle	rDest;

		//special case
		if(page == ENCY_INTER_UPPER_BAR)
		{
			arrPos = 19;
			fFoundGenInterArr = TRUE;
		}

		// SETUP TEMP CEL TO ARBLIT TO =================================
		//assume dest is always dynamic!!!!!

		TRACK_MEM("Encyclo: interface temp cel");
		gDest = ACreateCel(&rNumDest,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,CI_BLACK,200);
		pDest = (CelHeader*)AGetResData(gDest);
		pDestData =	(uchar*)pDest + sizeof(CelHeader);

		rDest.Set(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);

		pMouse->Hide();

		ARBlit(pGraphMgr->vgaDriver, pDestData,
				rDest.x1, rDest.y1, rDest.Wide(), rDest.High(), pGraphMgr->videoBufferSeg);

		pMouse->Show();

		//----------------------------------------------------
		// clip section we need latter in SetupInterfacePage()
		if(fFoundGenInterArr)
		{
			rInter.Set(copyInterGenRegions[arrPos][0],
					  	copyInterGenRegions[arrPos][1],
					  	copyInterGenRegions[arrPos][2],
					  	copyInterGenRegions[arrPos][3]);
		}
		else
		{
			rInter.Set(copyInterSpecRegions[arrPos][0],
					  	copyInterSpecRegions[arrPos][1],
					  	copyInterSpecRegions[arrPos][2],
					  	copyInterSpecRegions[arrPos][3]);
		}

		if(page == ENCY_INTER_SCENE_MAP)
		{
			// scenario map is scaled to 50% of the main picture.
			int	   lineNo, colNo;
								
			TRACK_MEM("Encyclo: interface scenario map cel");
			gInter = ACreateCel(&rNumInter,0,0,(rInter.Wide())/2,(rInter.High())/2,CI_BLACK,200);
			pInter = (CelHeader*)AGetResData(gInter);
			pInterData =	(uchar*)pInter + sizeof(CelHeader);

			for (lineNo = rInter.y1; lineNo < rInter.y2; lineNo += 2)
			{
				for (colNo = rInter.x1; colNo < rInter.x2; colNo += 2)
				{
					*(pInterData++) = *(pDestData + ((lineNo * L2R(rDest.Wide())) + colNo));
					#ifdef OS_MAC
						*(pInterData++) = *(pDestData + ((lineNo * L2R(rDest.Wide())) + colNo));
					#endif
				}
			}
		}
		else
		{
			TRACK_MEM("Encyclo: interface cel");
			gInter = ACreateCel(&rNumInter,0,0,rInter.Wide(),rInter.High(),CI_BLACK,200);
			pInter = (CelHeader*)AGetResData(gInter);
			pInterData =	(uchar*)pInter + sizeof(CelHeader);

			CopyCel(pInter,0,0,RES_CEL,rNumDest,1,&rInter,FALSE);
		}

		ADelete(gDest);
	 }
	}

	//---------------------------------------
	// setup background cel filled with black
	TRACK_MEM("Encyclo: background cel");
	gback = ACreateCel(&rNumBack,0,0,pbackAnimCH->width,pbackAnimCH->height,CI_BLACK,ENCY_BASE_PRI + 10);
	pback = AGetResData(gback);
	pbackCH = (CelHeader *)pback;

	//copy backAnim into our dynamic cel -this way we can still write direct
	CopyCel(pbackCH,0,0,RES_ANIM,animNum,1,FALSE);

	pGuy = &back;
	pGuy->SetRes(RES_CEL,rNumBack);
	pGuy->SetPos(ENCY_WIN_X,ENCY_WIN_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(ENCY_BASE_PRI + 10);

	pMouse->SetLimits(&rback);

	if(doFade)
	{
		//if we have to do a fade here then we need to set these up too
		// -this means it hasn't already been done by encymenu room
		pGuy = &topBorder;
		pGuy->SetRes(RES_ANIM,116,5);
		pGuy->SetPos(0,0);
		pGuy->SetContext(gSelf);
		pGuy->Setup(CT_ROST);
		pGuy->SetPri(ENCY_BASE_PRI-20);

		pGuy = &sideBorder;
		pGuy->SetRes(RES_ANIM,118,1);
		pGuy->SetPos(0,25);
		pGuy->SetContext(gSelf);
		pGuy->Setup(CT_ROST);
		pGuy->SetPri(ENCY_BASE_PRI-19);


		//need to get these new interface pieces onto the screen BEFORE
		//the do the screen capture for a fade down
		AAnimate();

		//we need to fade if encymenu didn't do it and not netgame
		if(bGlobal.storyLine != NETGAME)
		{
			// 0 percent fade down -just remaps colors to blue-gray range.
			fadeTo.Setup(320,400,ENCY_BASE_PRI - 10,gSelf,gPal,0,&rback);
		}
	}

	//---------------------------------------
	saveColor[0]  = AFontColor(FNT_BACK_COLOR);
	for(i=1;i<7;i++)
	{
		saveColor[i] = AFontColor(FNT_FORE_COLOR + i);
	}

	switch(menuType)
	{
		case CREATURE_BUTTON:
			maxPageNum = ENCY_MAX_CREATURE-1;
			SetupCreaturePage();
			break;

		case STRUCTURE_BUTTON:
			//maxPageNum = ENCY_MAX_STRUCTURE-1;
			SetupStructurePage();
			//we know that now structures have been sorted
			maxPageNum = structSortSize;	//needed for page wrap
			break;

		case ITEM_BUTTON:
			maxPageNum = ENCY_MAX_ITEM-1;
			SetupItemPage();
			break;

		case TERRAIN_BUTTON:
			//maxPageNum = ENCY_MAX_TERRAIN-1;
			SetupTerrainPage();
			//we know that now terrains have been sorted
			maxPageNum = terrSortSize;
			break;

		case INTERFACE:
			maxPageNum = ENCY_MAX_INTERFACE_PAGES;
			SetupInterfacePage();
			break;
	}

	//---------------------------------------
	//Button Setup for DONE button

	//load large font
	pFontMgr->SetRes(9050);

	pButton = &button[0];
	pButton->Create(ENCY_WIN_X+84, ENCY_WIN_Y+236, ENCY_BASE_PRI + 11, RES_ANIM, 8306, 1, gSelf, ENCY_WIN_X, ENCY_WIN_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rNumBack);					// draw into background cel
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,10);	//'DONE' text
	if(!pTxt)
		pTxt = " ";
	pButton->SetCelText(1, pTxt);
	pButton->SetColors(1, 93, 90);				// inactive colors
	pButton->SetCelText(2, pTxt);
	pButton->SetColors(2, 155, 142);				// active colors
	pButton->Draw();


	if(menuType != INTERFACE)
	{
		//Button Setup for PREV button
		pButton = &button[1];
		pButton->Create(ENCY_WIN_X+0, ENCY_WIN_Y+236, ENCY_BASE_PRI + 11, RES_ANIM, 8302, 1, gSelf, ENCY_WIN_X, ENCY_WIN_Y);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_TOP);
		pButton->SetTextOffSet(21,5);
		pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,11);	//'PREV' text
		if(!pTxt)
			pTxt = " ";
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Draw();


		//Button Setup for NEXT button
		pButton = &button[2];
		pButton->Create(ENCY_WIN_X+170, ENCY_WIN_Y+236, ENCY_BASE_PRI + 11, RES_ANIM, 8304, 1, gSelf, ENCY_WIN_X, ENCY_WIN_Y);
		pButton->SetupReplies(REPLY_DESELECTED);
		pButton->fIsToggle = FALSE;
		pButton->SetOwnerCel(rNumBack);					// draw into background cel
		pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_TOP);
		pButton->SetTextOffSet(2,5);
		pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,12);	//'NEXT' text
		pButton->SetCelText(1, pTxt);
		pButton->SetColors(1, 93, 90);				// inactive colors
		pButton->SetCelText(2, pTxt);
		pButton->SetColors(2, 155, 142);				// active colors
		pButton->Draw();
	}

	//since we had to make a special call to AAnimate above
	//we need to re-update the rect now that we've written our text
	AUpdateRect(&rback);

	Activate(TRUE);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void *)gSelf, L_FRONT, NULL);
	
	gCurControl	=	NULL;			// grip of currently active button, if any
}


void
Encyclopedia::NextPage()
{
	pbackCH = (CelHeader *)AGetResData(gback);
	
	//clear background with fresh unwritten copy
	CopyCel(pbackCH,0,0,RES_ANIM,animNum,1,FALSE);

 	button[0].SetState(S_CHANGED,TRUE);
 	button[1].SetState(S_CHANGED,TRUE);
 	button[2].SetState(S_CHANGED,TRUE);

	//NOTE:buttons now have own internal font
	//pFontMgr->SetRes(9050);

	//NOTE:buttons will not draw if they've not been created -no rOwnerCel
 	button[0].Draw();
	button[1].Draw();
 	button[2].Draw();

	AUpdateRect(&rback);

	switch(menuType)
	{
		case CREATURE_BUTTON:
			maxPageNum = ENCY_MAX_CREATURE-1;
			SetupCreaturePage();
			break;

		case STRUCTURE_BUTTON:
			SetupStructurePage();
			maxPageNum = structSortSize;	//needed for page wrap
			break;

		case ITEM_BUTTON:
			maxPageNum = ENCY_MAX_ITEM-1;
			SetupItemPage();
			break;

		case TERRAIN_BUTTON:
			SetupTerrainPage();
			maxPageNum = terrSortSize;
			break;

		case INTERFACE:
			SetupInterfacePage();
			maxPageNum = 0;	//not valid here
			break;
	}
}


void
Encyclopedia::SetupCreaturePage()
{
	int	textLines,textWidth;
	int 	portNum;

	//get portrait num
	portNum = creaturePortNum[page];

	//load portrait
	CopyCel(pbackCH,10,6,RES_ANIM,portNum,1,FALSE);

	//load large font
	pFontMgr->SetRes(9050);
	SetFontColors(SKIP_COLOR,87,114,87,114,183,187);

	//---------------
	//write headers
	sqbRes = ENCY_CREATURE_SQBRES_BASE+(page*10);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,7);			//'DESCRIPTION' text
	pFontMgr->SetString(3,143, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(sqbRes,8);		//spell header text
	pFontMgr->SetString(3,177, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,9);			//'TRAITS' text
	pFontMgr->SetString(115,6, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pFontMgr->SetRes(9062);
	SetFontColors(SKIP_COLOR,67,65,62,60,60,CI_BLACK);
	pTxt = sqbEncyclo.Load(sqbRes,1);		//unit name text
	pFontMgr->SetString(11,81, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	//---------------
	//load small font
	pFontMgr->SetRes(9060);
	pFontMgr->point += 1;
	SetFontColors(SKIP_COLOR,183,187,16,23,183,187);

	//---------------
	//write stats headers
	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,2);			//'attack' text
	pFontMgr->SetString(71,12, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,3);			//'range' text
	pFontMgr->SetString(71,24, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,4);			//'defense' text
	pFontMgr->SetString(71,36, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,5);			//'move' text
	pFontMgr->SetString(71,48, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,6);			//'life' text
	pFontMgr->SetString(71,60, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);


	//---------------
	//write stats
	pTxt = sqbEncyclo.Load(sqbRes,2);			//attack text
	pFontMgr->SetString(64,12, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(sqbRes,3);			//range text
	pFontMgr->SetString(64,24, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(sqbRes,4);			//defense text
	pFontMgr->SetString(64,36, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(sqbRes,5);			//move text
	pFontMgr->SetString(64,48, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(sqbRes,6);			//life text
	pFontMgr->SetString(0,60, pTxt, (uchar *)pbackCH, 69, NULL, DG_JUST_RIGHT);

	//--------------
	//write text
	pTxt = sqbEncyclo.Load(sqbRes,100);			//num of transform text lines
	textLines = atoi(pTxt);
	if(textLines)
	{
		char txt[2] = "\x1D";
		pFontMgr->SetString(11,104, txt, (uchar *)pbackCH, pbackCH->width, NULL);

		pTxt = sqbEncyclo.Load(sqbRes,101);			//transform text
		pFontMgr->SetString(17,104, pTxt, (uchar *)pbackCH, 90, NULL);
	}

	pTxt = sqbEncyclo.Load(sqbRes,7);			//char desc text
	pFontMgr->SetString(3,154, pTxt, (uchar *)pbackCH, 107, NULL);

	// lets see if the sqb exists and check for NULL ptr ourselves
	pTxt = sqbEncyclo.Load(sqbRes,10,TRUE);			//width of spell desc text
	if(pTxt)
	{
		textWidth = atoi(pTxt);
	}
	else
	{
		textWidth = 107;
	}
	pTxt = sqbEncyclo.Load(sqbRes,9);			//spell desc text
	pFontMgr->SetString(3,188, pTxt, (uchar *)pbackCH, textWidth, NULL);

	pTxt = sqbEncyclo.Load(sqbRes,11);			//trait text
	//pFontMgr->SetString(115,23, pTxt, (uchar *)pbackCH, 219, NULL);
	//this line dependent on fontmgr change
	pFontMgr->SetString(115,23, pTxt, (uchar *)pbackCH, 107, NULL);

	//restore proper font
	pFontMgr->SetRes(9050);
}


void
Encyclopedia::SetupStructurePage()
{
	int	i,enumNum;
	char	*pT;

	SortStructures();

	//page is already a zero-based num in sorted array

	sqbNum = structureSort[page][1];
	enumNum = structureSort[page][0];

	//get portrait num
	int portNum = pWorld->pTileLib->terrains[enumNum].portraitNum;


	//load portrait
	CopyCel(pbackCH,34,6,RES_ANIM,portNum,1,FALSE);

	//load large font
	pFontMgr->SetRes(9050);

	SetFontColors(SKIP_COLOR,87,114,87,114,183,187);

	//---------------
	//write headers
	sqbNum = (sqbNum-1)*10;

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,7);			//'DESCRIPTION' text
	pFontMgr->SetString(3,148, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,9);			//'TRAITS' text
	pFontMgr->SetString(115,11, pTxt, (uchar *)pb)ckCH, pbackCH->width, NULL);

	pFontMgr->SetRes(9062);
	SetFontColors(SKIP_COLOR,67,65,62,60,60,CI_BLACK);
	pTxt = sqbEncyclo.Load(ENCY_STRUCTURE_SQBRES,sqbNum + 1);		//name text
	//pTxt = pWorld->pTileLib->terrains[enumNum].szName;
	pFontMgr->SetString(0,102, pTxt, (uchar *)pbackCH, 107, NULL, DG_JUST_CENTER);

	//---------------
	//load small font
	pFontMgr->SetRes(9060);
	pFontMgr->point += 1;
	SetFontColors(SKIP_COLOR,183,187,16,23,183,187);

	//--------------
	//write text
	pTxt = sqbEncyclo.Load(ENCY_STRUCTURE_SQBRES,sqbNum + 2);			//damage text
	pFontMgr->SetString(0,79, pTxt, (uchar *)pbackCH, 107, NULL, DG_JUST_CENTER);

	pTxt = sqbEncyclo.Load(ENCY_STRUCTURE_SQBRES,sqbNum + 3);			//desc text
	pFontMgr->SetString(3,164, pTxt, (uchar *)pbackCH, 107, NULL);

	pTxt = sqbEncyclo.Load(ENCY_STRUCTURE_SQBRES,sqbNum + 4);			//trait text
	pT = pTxt;
	//replace all '>' chars in sqb with ENCY_LEFT_ARROW
	for(i=0;i<strlen(pTxt);i++)
	{
		if(*pT == '>')
		{
			*pT = ENCY_LEFT_ARROW;
		}
		pT++;
	}

	//pFontMgr->SetString(115,27, pTxt, (uchar *)pbackCH, 219, NULL);
	//this line dependent on fontmgr change
	pFontMgr->SetString(115,27, pTxt, (uchar *)pbackCH, 107, NULL);


	//restore proper font for buttons
	pFontMgr->SetRes(9050);
}


void
Encyclopedia::SetupItemPage()
{
	//get portrait num
	int portNum = itemPortNum[page];

	//load portrait
	CopyCel(pbackCH,34,6,RES_ANIM,portNum,2,FALSE);

	//load large font
	pFontMgr->SetRes(9050);

	SetFontColors(SKIP_COLOR,87,114,87,114,183,187);

	//---------------
	//write headers
	sqbNum = (page-1)*10;

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,7);			//'DESCRIPTION' text
	pFontMgr->SetString(3,148, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,9);			//'TRAITS' text
	pFontMgr->SetString(115,11, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	pFontMgr->SetRes(9062);
	SetFontColors(SKIP_COLOR,67,65,62,60,60,CI_BLACK);
	pTxt = sqbEncyclo.Load(ENCY_ITEM_SQBRES,sqbNum + 1);		//unit name text
	pFontMgr->SetString(0,102, pTxt, (uchar *)pbackCH, 107, NULL, DG_JUST_CENTER);

	//---------------
	//load small font
	pFontMgr->SetRes(9060);
	pFontMgr->point += 1;
	SetFontColors(SKIP_COLOR,183,187,16,23,183,187);

	//--------------
	//write text
	pTxt = sqbEncyclo.Load(ENCY_ITEM_SQBRES,sqbNum + 2);			//damage text
	pFontMgr->SetString(0,79, pTxt, (uchar *)pbackCH, 107, NULL, DG_JUST_CENTER);

	pTxt = sqbEncyclo.Load(ENCY_ITEM_SQBRES,sqbNum + 3);			//desc text
	pFontMgr->SetString(3,164, pTxt, (uchar *)pbackCH, 107, NULL);

	pTxt = sqbEncyclo.Load(ENCY_ITEM_SQBRES,sqbNum + 4);			//trait text
	//pFontMgr->SetString(115,27, pTxt, (uchar *)pbackCH, 219, NULL);
	//this line dependent on fontmgr change
	pFontMgr->SetString(115,27, pTxt, (uchar *)pbackCH, 107, NULL);


	//restore proper font for buttons
	pFontMgr->SetRes(9050);
}


void
Encyclopedia::SetupTerrainPage()
{
	int 	portNum,leftSqb,rightSqb;
	int	leftEnum=0,rightEnum=0;
	bool	rightSideOn = TRUE;
	int	sqbNum2;

	SortTerrains();

	//page is already a zero-based num in sorted array

	if(page % 2)
	{
		//odd  = right side of page

	 	leftSqb  = terrainSort[page-1][1];
		leftEnum  = terrainSort[page-1][0];

	 	rightSqb = terrainSort[page][1];
		rightEnum = terrainSort[page][0];
	}
	else
	{
		//even = left side of page

	 	leftSqb  = terrainSort[page][1];
		leftEnum = terrainSort[page][0];

		//check for wrap
		if( (page+1) >= terrSortSize)
			rightSideOn = FALSE;

		if(rightSideOn)
		{
	 		rightSqb = terrainSort[page+1][1];
			rightEnum = terrainSort[page+1][0];
		}
	}

	//get portrait num
	portNum = pWorld->pTileLib->terrains[leftEnum].portraitNum;

	//load left portrait
	CopyCel(pbackCH,34,6,RES_ANIM,portNum,1,FALSE);


	//right side is blank on last page
	if(rightSideOn)
	{
		portNum = pWorld->pTileLib->terrains[rightEnum].portraitNum;

		//load right portrait
		CopyCel(pbackCH,146,6,RES_ANIM,portNum,1,FALSE);
	}

	//load large font
	pFontMgr->SetRes(9050);

	SetFontColors(SKIP_COLOR,87,114,87,114,183,187);

	//---------------
	//write headers
	sqbNum =  (leftSqb-1) * 10;
	sqbNum2 = (rightSqb-1) * 10;

	pTxt = sqbEncyclo.Load(ENCY_GENERIC_SQBRES,9);			//'TRAITS' text
	pFontMgr->SetString(3,148, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	if(rightSideOn)
	{
		pFontMgr->SetString(115,148, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);
	}

	//left side
	pFontMgr->SetRes(9062);
	SetFontColors(SKIP_COLOR,67,65,62,60,60,CI_BLACK);
	//pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum + 1);		//terrain name text
	//lets get it strait from tile lib
	//pTxt = pWorld->pTileLib->terrains[leftEnum].szName;
	pTxt = sqbEncyclo.Load(SQUIB_WORLD_TERRAIN,leftEnum);		//terrain name text
	pFontMgr->SetString(0,102, pTxt, (uchar *)pbackCH, 107, NULL, DG_JUST_CENTER);

	if(rightSideOn)
	{
		//right side
		//pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum2 + 1);		//terrain name text
		//lets get it strait from tile lib
		//pTxt = pWorld->pTileLib->terrains[rightEnum].szName;
		pTxt = sqbEncyclo.Load(SQUIB_WORLD_TERRAIN,rightEnum);		//terrain name text
		pFontMgr->SetString(112,102, pTxt, (uchar *)pbackCH, 107, NULL, DG_JUST_CENTER);
	}

	//---------------
	//load small font
	pFontMgr->SetRes(9060);
	pFontMgr->point += 1;
	SetFontColors(SKIP_COLOR,183,187,16,23,183,187);

	//--------------
	//write text
	//left side
	pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum + 2);			// defense text
	pFontMgr->SetString(38,79, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	//left side
	pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum + 3);			// move text
	pFontMgr->SetString(38,89, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

	//left side
	pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum + 4);			//trait text
	pFontMgr->SetString(4,164, pTxt, (uchar *)pbackCH, 107, NULL);

	if(rightSideOn)
	{
		//right text
		pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum2 + 2);			// defense text
		pFontMgr->SetString(150,79, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

		//right text
		pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum2 + 3);			// move text
		pFontMgr->SetString(150,89, pTxt, (uchar *)pbackCH, pbackCH->width, NULL);

		//right side
		pTxt = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum2 + 4);			//trait text
		pFontMgr->SetString(116,164, pTxt, (uchar *)pbackCH, 107, NULL);
	}

	//restore proper font for buttons
	pFontMgr->SetRes(9050);
}


void
Encyclopedia::SetupInterfacePage()
{
	CelHeader	*pInter;
	uchar			*pInterData;
	Rectangle	rDest,rInter;
	int			iWidth,iHeight;
	int			pasteX,pasteY;
	bool			isSkip = FALSE;
	res_t			resType = RES_CEL;

	if(page != ENCY_MESSAGE_BOX)
	{
		//is it a weird shaped button?
		if(page >= WEIRD_BUTTON_1 && page <= WEIRD_BUTTON_3)
		{
			isSkip = TRUE;

			switch(page)
			{
				case WEIRD_BUTTON_1:
					rNumInter = 109;
					break;
				case WEIRD_BUTTON_2:
					rNumInter = 110;
					break;
				case WEIRD_BUTTON_3:
					rNumInter = 111;
					break;
			}

			gInter = ALoad(RES_ANIM,rNumInter);
			resType = RES_ANIM;
		}

		pInter = (CelHeader*)AGetResData(gInter);
		pInterData =	(uchar*)pInter + sizeof(CelHeader);
		iWidth  = pInter->width;
		iHeight = pInter->height;

		pasteX = ENCY_INTER_CTR_X - (iWidth/2);
		pasteY = ENCY_INTER_CTR_Y - (iHeight/2);

		CopyCel(pbackCH,pasteX,pasteY,resType,rNumInter,1,isSkip);
	}


	//--------------
	//write text
	sqbNum = (page-1)*10;

	//set for title
	pFontMgr->SetRes(9050);
	SetFontColors(SKIP_COLOR,183,187,16,23);

	if(page == ENCY_INTER_TRANS_PAGE)
	{
		//special handling of sqb 0 -'trans. opt. # port. button'
		//char			tstr[80];
		//char			transStr[4];
		//sprintf(transStr,"%d",arrPos - ENCY_INTER_TRANS_BASE + 1);
		//ReplaceName(tstr,ENCY_INTERFACE_SQBRES,sqbNum + 1,transStr);
		//pFontMgr->SetString(22,95, tstr, (uchar *)pbackCH, 177, DG_JUST_CENTER);

		//load small font
		pFontMgr->SetRes(9060);
		pFontMgr->point += 1;
		SetFontColors(SKIP_COLOR,183,187,16,23);

		pTxt = sqbEncyclo.Load(ENCY_INTERFACE_SQBRES,sqbNum + 2);	//desc text
		pFontMgr->SetString(22,102, pTxt, (uchar *)pbackCH, 177, NULL);
	}
	else
	{
		//pTxt = sqbEncyclo.Load(ENCY_INTERFACE_SQBRES,sqbNum + 1);	//title text
		//pFontMgr->SetString(22,95, pTxt, (uchar *)pbackCH, 177, DG_JUST_CENTER);

		//load small font
		pFontMgr->SetRes(9060);
		pFontMgr->point += 1;
		SetFontColors(SKIP_COLOR,183,187,16,23,183,187);

		pTxt = sqbEncyclo.Load(ENCY_INTERFACE_SQBRES,sqbNum + 2);	//desc text
		pFontMgr->SetString(22,102, pTxt, (uchar *)pbackCH, 177, NULL);
	}

	//restore proper font for buttons
	pFontMgr->SetRes(9050);
}


bool
Encyclopedia::HandleMsg(Message *pMsg)
{
	char		mess[100];

	// pass on to receivers first
	if (BAM_Room::HandleMsg(pMsg))
	{
		return(TRUE);
	}
	else
	{
		switch (pMsg->type)
		{
			case MSG_NOTICE:
				// here we are only interested in this type of notice.
				if (pMsg->notice.type == N_CONTROL_REPLY)
				{
					// a reply from one of our buttons.  Determine exact meaning of msg.
					if ((uint16)pMsg->notice.param == REPLY_DESELECTED)
					{
						if(pMsg->notice.gSource == button[0].gSelf)
						{
							pMono->Out("\nDONE button confirmed.");
							if(!fAnotherPage)
								Cleanup();
							else
							{
								menuType = anotherMenuType;
								page = anotherPage;
								fAnotherPage = FALSE;

								//reset background ani
								switch(menuType)
								{
									case CREATURE_BUTTON:
									case STRUCTURE_BUTTON:
									case ITEM_BUTTON:
										animNum = 8300;
										break;

									case TERRAIN_BUTTON:
										animNum = 8310;
										break;
									case INTERFACE:
										animNum = 8312;
										break;
								}

								anotherPage = 0;

								NextPage();
							}

							return TRUE;
						}
						else if(pMsg->notice.gSource == button[1].gSelf)
						{
							pMono->Out("\nPREV button confirmed.");
							SetPage(ENCY_PREV);

							NextPage();
							return TRUE;
						}
						else if(pMsg->notice.gSource == button[2].gSelf)
						{
							pMono->Out("\nNEXT button confirmed.");
							SetPage(ENCY_NEXT);

							NextPage();
							return TRUE;
						}
					}
					else
         		{
            		sprintf(mess, "Unrecognized button notice reply: %d", pMsg->notice.param);
            		APanic(mess);
         		}
				}
				break;


			case MSG_EVENT:
				if(badClick)
					return TRUE;	//ya see -it eats these things

				switch(pMsg->event.type)
				{
					case E_MOUSE_UP:
						//right mouse button is deselect
						if(pMsg->event.value == RIGHT_BTN)
						{
						  	Cleanup();
						  	return TRUE;
						}

						// the following is done to make sure that the active button
						// (if any) receives the MOUSE_UP event even if mouse has
						// wandered out of the rect
						if(gCurControl)
						{
							Object	*pObject;
							pObject = ADerefAs(Object, gCurControl);
							if(pObject->HandleMsg(pMsg))
								return(TRUE);
						}
						break;

					case E_KEY_DOWN:
						switch (pMsg->event.value)
						{
							//pass these keys thru
							case K_F1:
							case K_F2:
							case K_F3:
							case K_F4:
							case K_F5:
							case K_F6:
							case K_F7:
							case K_F8:
							case K_F9:
							case K_F10:
							case K_F11:
							case K_F12:
								return pBam->HandleMsg(pMsg);
								break;

							case K_X:
								if(pMsg->event.modifiers & MOD_ALT)
								{
									Cleanup();
									pContextMgr->Quit();
									return TRUE;
								}
								break;

							case K_ESC:
							case K_Q:
							case K_SLASH:
							case K_D:		//done
							case K_DOWN:
								Cleanup();
								return TRUE;
								break;

							case K_P:	//prev
							case K_LEFT:
								FakeMouseClick(button[1].gSelf);
								break;

							case K_N:	//next
							case K_RIGHT:
								FakeMouseClick(button[2].gSelf);
								break;
						}
						break;
				}
				break;
		}//endswitch
	}
	return TRUE;	//no pass-thru
}


void
Encyclopedia::Cycle()
{
	if(badClick)
	{
		newTime = clock();
		if(newTime > cleanupTime)
			CleanupBadClick();
	}
}


void
Encyclopedia::Cleanup()
{
	if(gInter)
	{
		ADelete(gInter);
	}

	if(bGlobal.storyLine != NETGAME)
	{
		if(doFade)
		{
			fadeTo.FadeUp();
		}

		bGlobal.roomMgr.curRoom->Pause(FALSE);

		//calling pause causes currentVerb in viewport to be reset to
		// 'NO_ACTION', so lets reset it to what it was before a call
		// was made to encyclo

		if(questionMarkCursor)
			pWorld->vPort.SetCurrentVerb(pWorld->vPort.oldCurrentVerb);

	}

	Activate(FALSE);

	if(prevFont != 9050)
		pFontMgr->SetRes(prevFont);

	pMouse->SetLimits(&oldMouseLimits);
	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	delete this;
}


void
Encyclopedia::CleanupBadClick()
{
	Activate(FALSE);

	pMouse->SetRes(oldMouseResType,oldMouseResNum,oldMouseCel);

	delete this;
}


int
Encyclopedia::CvtCreature(int enumNum)
{
	if(enumNum == ENCY_METAPOD_UNIT)
	{
		//METAPOD_UNIT id
		return ENCY_METAPOD;
	}

	if(enumNum < ENCY_MAX_CREATURE)
		return creatureCvt[enumNum];

	// 0=fail
	return 0;
}


int
Encyclopedia::CvtItem(int enumNum)
{
	if(enumNum == ENCY_WEIRD_WARD)
		return(ENCY_WEIRD_WARD);

	// 0=fail
	return itemCvt[enumNum];
}


void
Encyclopedia::CvtTerrain(int enumNum, encyclo_t *menuType, int *page)
{
	int	i;


	//lets look up matching sqb for the enum
	*menuType = (encyclo_t) terrainCvt[enumNum][1];
	*page = -1;	//default

	if(*menuType == TERRAIN_BUTTON)
	{
		SortTerrains();

		//lets ref into sorted array for page
		for(i=0;i<terrSortSize;i++)
		{
			if(terrainSort[i][0] == enumNum)
			{
				//we found 'page' -for now we'll ignore that there's two terrains
				//per page.
				*page = i;
				break;
			}
		}
	}
	else
	{
		SortStructures();

		//lets ref into sorted array for page
		for(i=0;i<structSortSize;i++)
		{
			if(structureSort[i][0] == enumNum)
			{
				*page = i;
				break;
			}
		}
	}
}

//find appro. interface page in encyclopedia
int
Encyclopedia::CvtInterface(int x,int y)
{
	uchar pix;
	int	i,anim;
	int	statMode;
	int	tmpPage;

	arrPos = 0; //default

	//get page by:

	//loop thru constant boxes
	for(i=0;i<ENCY_MAX_INTER_GENERAL;i++)
	{
		//did we hit?
		if(x >= activeInterGenRegions[i][0] &&
			y >= activeInterGenRegions[i][1] &&
			x <= activeInterGenRegions[i][2] &&
			y <= activeInterGenRegions[i][3])
		{
			//var used when multiple regions point to same page
			fFoundGenInterArr = TRUE;
			arrPos = i;

			//return the page number embedded in this array
			return (activeInterGenRegions[i][4]);
		}
	}


	//loop thru boxes dependent on world's menu mode
	for(i=0;i<ENCY_MAX_INTER_SPECIFIC;i++)
	{
		//did we hit a rect?
		if(x >= activeInterSpecRegions[i][0] &&
			y >= activeInterSpecRegions[i][1] &&
			x <= activeInterSpecRegions[i][2] &&
			y <= activeInterSpecRegions[i][3])
		{
			//we must also check world's mode
			switch(pWorld->statBoxCurrMode)
			{
				case TERRAIN:
					statMode = E1;
					break;

				case TRANSFORMING:
					statMode = E2;
					break;

				case UNIT:
					statMode = E3;
					break;

				case BUILDING:
					statMode = E4;
					break;

				default:
					//we didn't find region -so lets pass a shutdown code
					return 0;
			}

			//did the rect have the right mode?
			if(statMode & activeInterSpecRegions[i][5])
			{
				tmpPage = activeInterSpecRegions[i][4];

				//is it one of the weird shaped buttons?
				if(pWorld->statBoxCurrMode == TERRAIN &&
					tmpPage >= WEIRD_BUTTON_1 && tmpPage <= WEIRD_BUTTON_3)
				{
					//are we really on it?
					switch(tmpPage)
					{
				 		case WEIRD_BUTTON_1:
							anim = 109;
							break;
				 		case WEIRD_BUTTON_2:
							anim = 110;
							break;
				 		case WEIRD_BUTTON_3:
							anim = 111;
							break;
					}

					pix = GetPixel((coord)x-activeInterSpecRegions[i][0],
										(coord)y-activeInterSpecRegions[i][1],
										RES_ANIM,anim,1);
					if(pix == SKIP_COLOR)
					{
						//not the one we want
						continue;
					}
				}

				//var used when multiple regions point to same page
				fFoundGenInterArr = FALSE;
				arrPos = i;

				//return the page number embedded in this array
				return (tmpPage);
			}
		}
	}

	//we didn't find region -so lets pass a shutdown code
	return 0;
}


int
Encyclopedia::FindCreatureEnum(int pageL)
{
	int	i;

	//currently sqb=page  -so find the sqb in the array and return index (enum)
	for(i=0;i<ENCY_MAX_CREATURE;i++)
	{
		if(creatureCvt[i] == pageL)
		{
			return(i);
		}
	}

	return(0);
}


int
Encyclopedia::FindItemEnum(int pageL)
{
	int	i;

	//currently sqb=page  -so find the sqb in the array and return index (enum)
	for(i=0;i<ENCY_MAX_ITEM;i++)
	{
		if(itemCvt[i] == pageL)
		{
			return(i);
		}
	}

	return(0);
}


int
Encyclopedia::FindTerrPage(int type, int enumOrSqb)
{
	int	i;
	int	subIdx;

	if(!enumOrSqb)
		return(0);

	if (type == ENCY_TERR_SQB)
		subIdx = 1;
	else
		subIdx = 0;

	for(i=0;i<terrSortSize;i++)
	{
		if(terrainSort[i][subIdx] == enumOrSqb)
		{
			return(i);
		}
	}

	return(0);
}


int
Encyclopedia::FindStructPage(int type, int enumOrSqb)
{
	int	i;
	int	subIdx;

	if(!enumOrSqb)
		return(0);

	if (type == ENCY_STRUCT_SQB)
		subIdx = 1;
	else
		subIdx = 0;

	for(i=0;i<structSortSize;i++)
	{
		if(structureSort[i][subIdx] == enumOrSqb)
		{
			return(i);
		}
	}

	return(0);
}


void
Encyclopedia::SetPage(int dir)
{
	int	enumNum, mapNum, oldPage;
	bool	validPage = FALSE;
	grip	gUnit;
	Unit	*pUnit;

	oldPage = page;

	//we have to account for two terrains per page!
	if(menuType == TERRAIN_BUTTON)
		dir *= 2;

	do
	{
		// some use page as a zero-based number
		if(menuType == STRUCTURE_BUTTON || menuType == TERRAIN_BUTTON)
		{
			if((page + dir) < 0)
				page = maxPageNum-1;
			else
			if((page + dir) >= maxPageNum)
				page = 0;
			else
				page += dir;
		}
		else
		{
			if((page + dir) <= 0)
				page = maxPageNum;
			else
			if((page + dir) > maxPageNum)
				page = 1;
			else
				page += dir;
		}


		mapNum = atoi(pBam->scenarioName);

		//our exception list
		//in certain maps some things shouldn't be shown
		switch(menuType)
		{
			case CREATURE_BUTTON:
				//note: at present the creature sqb's are in alpha order and
				//are equiv to pages
				enumNum = FindCreatureEnum(page);

				switch(enumNum)
				{
					case GUARDIAN:
						if(mapNum == 9410)
							validPage = TRUE;
						break;

					case JUGGERNAUT:
						if(mapNum == 9295 || mapNum == 9330)
							validPage = TRUE;
						else
						if(mapNum == 9292 || mapNum == 9332)
						{
							//there's a random change Juggernaut will exist
							//in these maps -if we find him, show him.
					
							gUnit = (grip)pWorld->unitLib.lUnits[SIDE3].FirstValue();

							while(gUnit)
							{
								if(pMemMgr->CheckGrip(gUnit) == GRIP_VALID)
								{
									pUnit = ADerefAs(Unit, gUnit);
									if (pUnit->type == JUGGERNAUT)
									{
										validPage = TRUE;
									}
								}
								gUnit = (grip)pWorld->unitLib.lUnits[SIDE3].NextValue();
							}
						}
					break;

					case ENCHANTER:
					case HARPY:
					case GOBLIN:
						if(mapNum == 9350 || mapNum == 9352 || mapNum == 9355 ||
							mapNum == 9370 || mapNum == 9372 || mapNum == 9375 ||
							mapNum == 9390 || mapNum == 9392 || mapNum == 9395)
							validPage = TRUE;
						break;

					default:
						validPage = TRUE;

				}
				break;

			case ITEM_BUTTON:
				//note: at present the item sqb's are in alpha order and
				//are equiv to pages

				//take this page (sqb) and look up the matching enum
				enumNum = FindItemEnum(page);

				switch(enumNum)
				{
					case DOWRY_CHEST:
					case KEY:
						if(mapNum == 9190)
							validPage = TRUE;
						break;

					case COMPONENT:
						if(mapNum == 9315)
							validPage = TRUE;
						break;

					case HARPY_NET:
						if(mapNum == 9350 || mapNum == 9352 || mapNum == 9355 ||
							mapNum == 9370 || mapNum == 9372 || mapNum == 9375 ||
							mapNum == 9390 || mapNum == 9392 || mapNum == 9395)
							validPage = TRUE;
						break;

					default:
						validPage = TRUE;
						break;
				}
				break;

			case STRUCTURE_BUTTON:
				//all sorted structure pages are valid
				//but there could be multiple entries for each page

				if(structureSort[page][1] == structureSort[oldPage][1])
					validPage = FALSE;	//same, so try another
				else
					validPage = TRUE;
				break;

			case TERRAIN_BUTTON:
				//all sorted terrain pages are valid
				validPage = TRUE;
				break;

			case INTERFACE:
				//shouldn't ever get here -no prev/next buttons for interface pages
				validPage = TRUE;
				break;
		}

	} while(!validPage);
}


void
Encyclopedia::SortTerrains()
{
	int	i,j=0, mapNum, enumNum;
	bool	validPage;

	if(!terrSort)
	{
		mapNum = atoi(pBam->scenarioName);

		//get all terrain entries from terrainCvt
		for(i=0;i<TERRAINMAXTYPES;i++)
		{
			if(terrainCvt[i][1] == 3)
			{
				validPage = FALSE;

				//our exception list
				//in certain maps some things shouldn't be shown
				enumNum = terrainCvt[i][0];

				switch(enumNum)
				{
					case ATLANTEAN_RUIN: //undersea tomb
					case PRISON_PIT:
						if(mapNum == 9232 || mapNum == 9235)
							validPage = TRUE;
						break;

					case PRESSURE_PLATE:
					case TRIP_SWITCH:
						if(mapNum == 9310 || mapNum == 9312 || mapNum == 9315)
							validPage = TRUE;
						break;

					case TERR53:	//great_furnace
					case TERR54:	//great_column
						if(mapNum == 9330 || mapNum == 9332 || mapNum == 9335)
							validPage = TRUE;
						break;

					case PLATFORM:
						if(mapNum == 9410)
							validPage = TRUE;
						break;

					default:
						validPage = TRUE;
				}

				//last double check
				//if we find portraitNum is 0, then this isn't a valid terrain
				//for this map
				if(validPage && !(pWorld->pTileLib->terrains[enumNum].portraitNum))
					validPage = FALSE;


				//only valid pages are allowed into sorted array
				if(validPage)
				{
					//enum and sqbNum into array
					terrainSort[j][0] = terrainCvt[i][0];
					terrainSort[j][1] = terrainCvt[i][2];
					j++;
				}
			}
		}

		//reset max pages to sorted array size
		//maxPageNum = j;
		terrSortSize = j;

		qsort((void*)terrainSort,j,sizeof(int)*2, CompareTerr);
		terrSort = TRUE;
	}
}



int
CompareTerr(void const *pparm1,void const *pparm2)
{
	int			*parm1,*parm2;
	char			*pTxt1,*pTxt2;
	SquibRes		sqbEncyclo;
	int			sqbNum1,sqbNum2;

	parm1 = (int*)pparm1;
	parm2 = (int*)pparm2;

	//get second element in row, zero-base it (-1), x10 to get base sqbnum
	//for that terrain and add 1 to get its title
	sqbNum1 = ((((*(parm1+1)) - 1) * 10) + 1);
	sqbNum2 = ((((*(parm2+1)) - 1) * 10) + 1);

	//the second element in row is the sqb
	pTxt1 = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum1);
	pTxt2 = sqbEncyclo.Load(ENCY_TERRAIN_SQBRES,sqbNum2);

	return(strcmp(pTxt1,pTxt2));
}



void
Encyclopedia::SortStructures()
{
	int	i,j=0, mapNum, enumNum;
	bool	validPage;

	if(!structSort)
	{
		mapNum = atoi(pBam->scenarioName);

		//get all terrain entries from terrainCvt
		for(i=0;i<TERRAINMAXTYPES;i++)
		{
			if(terrainCvt[i][1] == 1)
			{
				validPage = FALSE;

				//our exception list
				//in certain maps some things shouldn't be shown
				enumNum = terrainCvt[i][0];

				switch(enumNum)
				{
					case AQUEDUCT:
						if(mapNum == 9290)
							validPage = TRUE;
						break;
					case BANISH_STONE:
						if(mapNum == 9232 || mapNum == 9235)
							validPage = TRUE;
						break;
					case CAMP:
						if(mapNum == 9190)
							validPage = TRUE;
						break;
					case CAULDRON:
						if(mapNum == 9350 || mapNum == 9352 || mapNum == 9355 ||
							mapNum == 9370 || mapNum == 9372 || mapNum == 9375 ||
							mapNum == 9390 || mapNum == 9392 || mapNum == 9395)
							validPage = TRUE;
						break;
					case MINE:
						if(mapNum == 9250 || mapNum == 9252 || mapNum == 9255)
							validPage = TRUE;
						break;
					case GUARDIAN_PORTAL:
					case GOLEM_PORTAL:
					case PALADIN_PORTAL:
					case WARRIOR_PORTAL:
					case WELL_OF_IMMORTALS:
					case WIND_WALL:
						if(mapNum == 9410)
							validPage = TRUE;
						break;

					default:
						validPage = TRUE;
				}


				//last double check
				//if we find portraitNum is 0, then this isn't a valid structure
				//for this map
				if(validPage && !(pWorld->pTileLib->terrains[enumNum].portraitNum))
					validPage = FALSE;


				//only valid pages are allowed into sorted array
				if(validPage)
				{
					//enum and sqbNum into array
					structureSort[j][0] = terrainCvt[i][0];
					structureSort[j][1] = terrainCvt[i][2];
					j++;
				}
			}
		}

		//reset max pages to sorted array size
		//maxPageNum = j;
		structSortSize = j;

		qsort((void*)structureSort,j,sizeof(int)*2, CompareBuilding);
		structSort = TRUE;
	}
}



int
CompareBuilding(void const *pparm1,void const *pparm2)
{
	int			*parm1,*parm2;
	char			*pTxt1,*pTxt2;
	SquibRes		sqbEncyclo;
	int			sqbNum1,sqbNum2;

	parm1 = (int*)pparm1;
	parm2 = (int*)pparm2;

	//get second element in row, zero-base it (-1), x10 to get base sqbnum
	//for that structure and add 1 to get its title
	sqbNum1 = ((((*(parm1+1)) - 1) * 10) + 1);
	sqbNum2 = ((((*(parm2+1)) - 1) * 10) + 1);

	//the second element in row is the sqb
	pTxt1 = sqbEncyclo.Load(ENCY_STRUCTURE_SQBRES,sqbNum1);
	pTxt2 = sqbEncyclo.Load(ENCY_STRUCTURE_SQBRES,sqbNum2);

	return(strcmp(pTxt1,pTxt2));
}
