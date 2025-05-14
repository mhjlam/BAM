//	Copyright 1994, Tachyon, Inc.
//
//


#include "ctype.h"

#include "apigraph.hpp"
#include "apires.hpp"
#include "apifont.hpp"
#include "apievt.hpp"
#include "debug.hpp"
#include	"mouse.hpp"
#include	"resource.hpp"
#include "scrimage.hpp"
#include "soundmgr.hpp"

#include "bam_dg.hpp"
#include "bam.hpp"
#include "bampopup.hpp"
#include "encymenu.hpp"
#include "encyclo.hpp"
//#include "flic.hpp"
#include "items.hpp"
#include "legendop.hpp"
#include "maps.hpp"
#include "savemgr.hpp"
#include "savemenu.hpp"
#include "spendexp.hpp"
#include "tutorial.hpp"
#include "units.hpp"
#include "viewport.hpp"
#include "winlose.hpp"
#include "winlose2.hpp"
#include "world.hpp"
#include "worldmap.hpp"

#ifdef NDEBUG
//#undef NDEBUG
#endif

#define MM_MAIN_SQB		7010

char pVerbNames[][30] =
{"NOTHING", "MOVE TO", "ATTACK", "GUARD", "DISBAND", "SKILL",	"TRANSFORM", "DROP",
 "SUMMON", "DIE", "BERSERK", "USE_ITEM", "HEAL", "FOLLOW_THE_LEADER",
 "TRANSF2BLDG", "REPAIR", "FOLLOW", "AVOID", "SCARE", "WAIL", "STEAL", "ABORT", "VENOM", "GET_STONED",
 "QUESTION", "BLOW BACK", "ADD ANI"};

extern char 	szFuncNames[][20];
extern int		offsets[][2];
extern Debugger BamDebug;
bool startup_game;
extern void    MouseHandler( int draw_mouse );

int mainButtonCoords[MAINBTN_TOTAL][2] = {253, 184, 	282, 214, 	253, 244,
	282, 184, 	253, 214, 	282, 274};
int mainButtonAnis[MAINBTN_TOTAL] = {126, 128, 124, 129, 125, 127};
char mainButtonKeys[MAINBTN_TOTAL] = {NULL, NULL, NULL, NULL, K_SLASH, NULL};

int verbButtonCoords[7][2] = {122, 320, 	157, 320, 	122, 350, 	253, 274,
	194, 322, 	194, 356,	284, 373};
int verbButtonAnis[7] = {109, 110, 111, 122, 108, 107, 115};
int transformButtonCoords[5][2] = {122, 320,		160, 320,	198, 320,
	236, 320,	274, 320};
int acolyteBldgAnis[5] = {2700, 2702, 2708, 2716, 2718};
#define CHEAT_MAX_LEN	25
#ifndef NDEBUG
	#define SHOW_OFF_TIME	(TICKS_PER_SEC * 60 * 10)
#else
	#define SHOW_OFF_TIME	(TICKS_PER_SEC * 60 * 3)
#endif

enum cheatCodes
{
	CHEAT_DEFOG = 0, CHEAT_MANA, CHEAT_RESEARCH, CHEAT_ACOLYTE, CHEAT_CLERIC, CHEAT_DRUID, 
	CHEAT_ENCHANTER, CHEAT_FURY, CHEAT_GARGOYLE, CHEAT_GHOUL, CHEAT_GNOME, CHEAT_GOBLIN, CHEAT_GOLEM,
	CHEAT_GRIFFIN, CHEAT_GUARDIAN, CHEAT_JUGGERNAUT, CHEAT_HARPY, CHEAT_NYMPH, CHEAT_PALADIN, CHEAT_RANGER,
	CHEAT_WARRIOR, CHEAT_WIZARD, CHEAT_WRAITH, CHEAT_WYRM, CHEAT_ZOMBIE, CHEAT_ITEM, CHEAT_PYRO,
	CHEAT_CYBORG, CHEAT_BERSERK, CHEAT_MAX
};
char pCheatCodes[CHEAT_MAX][CHEAT_MAX_LEN] =
{
	"FOG? WHAT FOG?", "BOOST", "ELMINSTER", "ACOLYTE", "FATHER", "WOLVERINE", "GREMLIN", "FATAL ATTRACTION",
	"FLYING MONKIES",	"RAISE DEAD", "ALASKA", "JARETH", "CONCRETE", "MERV", "BODY GUARD", "MICROSQUISH",
	"MOTHER IN LAW",	"SEDUCTION",	"LANCELOT",	"YOGI", "NEEDS FOOD BADLY", "MERLIN", "SHADOW", "SMOG",
	"DEAD FLESH", "ITEMXX", "CANDYGRAM", "WOPPER", "QUARTERS"
};
char pCheatCodeBuffer[CHEAT_MAX_LEN];
#define MAX_PATHER_MARKERS	50
grip	gPathAni[MAX_PATHER_MARKERS];
bool	fShowCurrentUnitPath = FALSE;

void ReportFreeMem(void);
World::World(void) : lBackgroundAnis(100, TRUE)
{
	TPalette		*pPal;
	Guy			*pGuy;
	BAM_Button	*pButton;
	BAM_Box		*pBox;
	WorldMap		*pWorldMap;
	uint32		loop1;
	grip			gCel;
	int			i;

	classID = CID_WORLD;
	if(bGlobal.replayMap)
	{
		// we're doing a ReplayMap
		bGlobal.randGen.ReSeed(bGlobal.replaySeed);
		bGlobal.randGen2.ReSeed(bGlobal.replaySeed2);
	}
	else
	{
		// save current RNG seeds, in case of replay
		bGlobal.replaySeed = bGlobal.randGen.GetSeed();
		bGlobal.replaySeed2 = bGlobal.randGen2.GetSeed();
	}

	pWorld = this;
	memset(gPathAni, NULL, sizeof(gPathAni));
	bGlobal.gWorld = gSelf;
	tShowOffTimer = NULL;

	currFrame = 0;
	for(i = 0; i < 5; i++)
	{
		TRACK_MEM("WorldCluts");	clut[i] = AMalloc(CLUT_SIZE);
	}

	memset(actPool1.total, NULL, sizeof(actPool1.total));
	memset(actPool2.total, NULL, sizeof(actPool2.total));
	mapResNum = 0;
	tileResNum = 0;
	musicNum = 0;
	memset(unitsCreated, NULL, sizeof(unitsCreated));
	memset(unitsLost, NULL, sizeof(unitsLost));
	memset(unitsSlain, NULL, sizeof(unitsSlain));
	memset(structuresDestroyed, NULL, sizeof(structuresDestroyed));
	memset(sitesControlled, NULL, sizeof(sitesControlled));

	memset(winCons, NULL, sizeof(winCons));

	aiOn = TRUE;

	lastUnitSerialDrawn = NULL;
	for(loop1 = 0; loop1 < 5; loop1++)
	{
		mana[loop1] = 0;
		lastMana[loop1] = 0;
	}
	lastManaCel = 0;
	playerSide = SIDE1;
	tNextManaBonus = 0;
	tNextWinCheck = 0;

	TRACK_MEM("Portrait");	pButton = new BAM_Button;
	gPortrait = pButton->gSelf;

	TRACK_MEM("WorldMap");	pWorldMap = new WorldMap;
	gWorldMap = pWorldMap->gSelf;

	TRACK_MEM("TPalette");	pPal = new TPalette;
	gPal = pPal->gSelf;

	TRACK_MEM("InterfaceScreen");	pGuy = new Guy;
	gInterfaceScreen = pGuy->gSelf;

	TRACK_MEM("ISCel");	gCel = ACreateCel(&rISCel, 0, 0, 320, 401, CI_BLACK);
	
	for(loop1 = 0; loop1 < 7; loop1++)
	{
		TRACK_MEM(&pVerbNames[loop1 + 1][0]);	pButton = new BAM_Button;
		gVerbButton[loop1] = pButton->gSelf;
	}
	for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
	{
		TRACK_MEM("TransformButton");	pButton = new BAM_Button;
		gTransformButton[loop1] = pButton->gSelf;
	}

	TRACK_MEM("XPButton");	pButton = new BAM_Button;
	gSpendXPButton = pButton->gSelf;

	TRACK_MEM("ItemPortrait");	pButton = new BAM_Button;
	gItemPortrait = pButton->gSelf;

	TRACK_MEM("NoteButton");		pButton = new BAM_Button;
	gNoteButton = pButton->gSelf;
//	TRACK_MEM("NoteSend");			pButton = new BAM_Button;
//	gNoteSend = pButton->gSelf;
//	TRACK_MEM("NoteEdit");			pButton = new BAM_Button;
//	gNoteExit = pButton->gSelf;
	TRACK_MEM("NoteBox");			pBox = new BAM_Box;
	gNoteBox = pBox->gSelf;
	gNoteBoxText = NULL;
	
	for(loop1 = 0; loop1 < MAINBTN_TOTAL; loop1++)
	{
		TRACK_MEM("MainButton");	pButton = new BAM_Button;
		gMainButton[loop1] = pButton->gSelf;
	}

	for(loop1 = 0; loop1 < MSGBOX_MAX; loop1++)
		MsgBox[loop1] = NULL;
	MsgBoxOn = TRUE;
	TRACK_MEM("GoSeeButton");		pButton = new BAM_Button;
	gGoSeeButton = pButton->gSelf;
	
	ConstructEditMode();

	// initialization stuff in Restart()
	Activate(TRUE);
	msgMask = E_MOUSE_DOWN | E_MOUSE_UP | E_KEY_DOWN;
	gCurControl	=	NULL;									// grip of currently active button, if any
	fIsPaused = FALSE;
   dragSuspendCnt = 0;
}

void
World::ConstructEditMode(void)
{
//	char	*pText;
//	BAM_Box	*pBox;

	gBoxText = NULL;
	gBox = NULL;

//	TRACK_MEM("NoteBoxText");	gBoxText = AMalloc(80);
//	pText = ADerefAs(char, gBoxText);
//	strcpy(pText, "<empty>");

//	TRACK_MEM("BAMBox");				pBox = new BAM_Box;
//	gBox = pBox->gSelf;
}

World::~World()
{
	int	i;

	bGlobal.gWorld = NULL;

	for(i = 0; i < 5; i++)
	{
		AFree(clut[i]);
		clut[i] = 0;
	}

	//In case world in going down in a paused state
	ForceResumeTicks();

	pWorld = NULL;
}

void
World::MsgBoxDraw(void)
{
	uint32		loopY, loop1;
	CelHeader	*pCel;
	uchar			*pCelData, *pCelDataCur;
	BoxMsg		*pMsg;
	uint32		width, height, x, y;
	BAM_Button	*pButton;

	if(!MsgBoxOn)
		return;

	// area of MsgBox (box for msgs)
	rTemp1.Set(0, 388, 319, 399);

	pFontMgr->SetRes(9050);

	if(MsgBox[MSGBOX_MAX - 1] && !MsgBox[MSGBOX_MAX - 2])
	{
		pButton = ADerefAs(BAM_Button, gGoSeeButton);
		pButton->Listen(TRUE);
		pButton->Select(FALSE);
	}

//	if(pBam->fMapEdit)
		SetFontColors(CI_BLACK, 35, 20, 35, 20, 35, 20);
//		SetFontColors(CI_BLACK, 93, 90);

	pCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	pCelDataCur = pCelData + L2R(pCel->width) * rTemp1.y1 + L2R(rTemp1.x1);

	width = L2R(rTemp1.Wide());
	height = rTemp1.High();
	for(loopY = 0; loopY < height; loopY++, pCelDataCur += L2R(pCel->width))
		memset(pCelDataCur, CI_BLACK, width);

	x = rTemp1.x1 + 2;
	y = rTemp1.y1 + 1;
	for(loop1 = 0; loop1 < MSGBOX_MAX; loop1++, y += 12)
	{
//		if(pBam->fMapEdit)
//		{
//			if(loop1 == MSGBOX_MAX - 2)
//				SetFontColors(CI_BLACK, CI_WHITE, CI_BLACK);
//		}

		if(loop1 == MSGBOX_MAX - 1)
			SetFontColors(CI_BLACK, 166, 37, 166, 37, 166, 37);

		if(MsgBox[loop1])
		{
			pMsg = ADerefAs(BoxMsg, MsgBox[loop1]);
			ASetString(x, y, pMsg->text, (uchar *)pCel, pCel->width);
		}
	}
	AUpdateRect(&rTemp1);
}

void
World::MsgBoxAdd(char *msg, uint32 newMapX, uint32 newMapY)
{
	int			loop1;
	BoxMsg		*pMsg;

	if(MsgBox[0])
		ADelete(MsgBox[0]);

	for(loop1 = 0; loop1 < MSGBOX_MAX - 1; loop1++)
		MsgBox[loop1] = MsgBox[loop1 + 1];
	TRACK_MEM("BoxMsg");	pMsg = new BoxMsg;
	MsgBox[MSGBOX_MAX - 1] = pMsg->gSelf;
	memcpy(pMsg->text, msg, 53);
	pMsg->text[54] = NULL;
	pMsg->mapX = newMapX;
	pMsg->mapY = newMapY;
}

void
World::NoteExit(void)
{
	NoteCleanup();
}

void
World::NoteSend(void)
{
	BAM_Box 	*pBox;
	char		*pText;
	
	pBox = ADerefAs(BAM_Box, gNoteBox);
	pBox->ReturnString();

	pText = ADerefAs(char, gNoteBoxText);
	strupr(pText);

	SPacket1.header.ID = PLAYER_MSG;
	SPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player //BUGBUG changed by RW
	SPacket1.header.len = strlen(pText) + 1;
	SPacket1.pData = pText;
	pCommMgr->SendData(&SPacket1);

	#ifndef NDEBUG
	pMono->Out("Msg to other player: \"%s\"\n", pText);
	#endif

	NoteCleanup();	
}

void
World::NoteCleanup(void)
{
	BAM_Button	*pButton;
	BAM_Box		*pBox;

	pBox = ADerefAs(BAM_Box, gNoteBox);
	pBox->Select(FALSE);
	pBox->Listen(FALSE);

//	pButton = ADerefAs(BAM_Button, gNoteExit);
//	pButton->Select(FALSE);
//	pButton->Listen(FALSE);
//	pButton->SetCel(3);
//	pButton->Draw();

//	pButton = ADerefAs(BAM_Button, gNoteSend);
//	pButton->Select(FALSE);	// dont wait for MOUSE_UP
//	pButton->Listen(FALSE);

	pButton = ADerefAs(BAM_Button, gNoteButton);
	pButton->Listen(FALSE);

	// re-activate these guys
	pButton = ADerefAs(BAM_Button, gGoSeeButton);
	pButton->SetOwnerCel(rISCel);
	if(MsgBox[MSGBOX_MAX - 1])
	{
		pButton->Listen(TRUE);
		pButton->Select(FALSE);
	}
	else
	{
		pButton->Listen(FALSE);
		pButton->SetCel(3);
		pButton->Draw();
	}
	pButton = ADerefAs(BAM_Button, gVerbButton[DISBAND - 1]);
	pButton->SetOwnerCel(rISCel);
	pButton->SetState(S_CHANGED, TRUE);
	pButton->Draw();

	// turn the msg box back on
	MsgBoxOn = TRUE;
	MsgBoxDraw();
	AFree(gNoteBoxText);
	gNoteBoxText = NULL;
}

void
World::Note(void)
{
	BAM_Button	*pButton;
	BAM_Box		*pBox;
	char			*pText;

//	pButton = ADerefAs(BAM_Button, gNoteButton);
//	pButton->Listen(FALSE);

	// MOUSE-UP event will be gobbled by Box, so force NOTE button to deselect manually
//	pButton = ADerefAs(BAM_Button, gMainButton[MAINBTN_SEND]);
//	pButton->Select(FALSE);

	// we're temporarily replacing these guys
	pButton = ADerefAs(BAM_Button, gGoSeeButton);
	pButton->Listen(FALSE);
	pButton->SetOwnerCel(NULL);	// prevent further redraws until we say it's okay
	pButton = ADerefAs(BAM_Button, gVerbButton[DISBAND - 1]);
	pButton->Listen(FALSE);
	pButton->SetOwnerCel(NULL);	// prevent further redraws until we say it's okay

	// and turning these guys on
//	pButton = ADerefAs(BAM_Button, gNoteSend);
//	pButton->Listen(TRUE);
//	pButton->Select(FALSE);
//	ResetMsgPri(gNoteSend, PRIORITY_MAX);

//	pButton = ADerefAs(BAM_Button, gNoteExit);
//	pButton->Listen(TRUE);
//	pButton->Select(FALSE);
//	ResetMsgPri(gNoteExit, PRIORITY_MAX - 1);

	MsgBoxOn = FALSE;

	TRACK_MEM("NoteBoxText");	gNoteBoxText = AMalloc(80);
	pText = ADerefAs(char, gNoteBoxText);
	strcpy(pText, "");
	pBox = ADerefAs(BAM_Box, gNoteBox);
	pBox->SetColors(CI_BLACK, 64, 74, 64, 74, 64, 74, 155, 142);
	pBox->SetBackCopyColor(CI_BLACK);
	pBox->Create(0, 388, 319, 399, PRI_INTERFACE + 1, gNoteBoxText, 53, gSelf, rISCel);
	pBox->SetupReplies(REPLY_DESELECTED);	// let us know when text changes
	pBox->Select(TRUE);

	// leave room in list for Send and Exit buttons to get in ahead of box
	ResetMsgPri(gNoteBox, PRIORITY_MAX - 2);
}

void
World::GoSee(void)
{
	BoxMsg	*pMsg;
	WorldMap	*pWorldMap = ADerefAs(WorldMap, gWorldMap);

	if(!MsgBox[MSGBOX_MAX - 1])
		return;

	pMsg = ADerefAs(BoxMsg, MsgBox[MSGBOX_MAX - 1]);
	vPort.MoveView(pMsg->mapX - VPORT_WIDTH / 2, pMsg->mapY - VPORT_HEIGHT / 2);
	vPort.MoveCursor(pMsg->mapX, pMsg->mapY);
	pWorldMap->MoveCursor(vPort.ViewX, vPort.ViewY);
}

/*
void
World::ScanForNewMana(uint32 side)
{
	int32	manaBonus = MANA_MEDITATE;		// auto bonus just for having a portal
	grip	gUnit;
	Unit	*pUnit;

	gUnit = (grip)unitLib.lUnits[side].FirstValue();
	while(gUnit)
	{
//		if(pMemMgr->CheckGrip(gUnit) != GRIP_VALID)
//			pMono->Out("World::ScanForNewMana() - error, invalid grip %d in lUnits\n", gUnit);
//		else
//		{
			pUnit = ADerefAs(Unit, gUnit);
			if(pUnit->type == ACOLYTE && pUnit->currAction == GUARD)
			{
				pUnit->currMana = AMin((pUnit->currMana + MANA_MEDITATE), MANA_ACOLYTE_MAX);
				if(pUnit->fCursorTracking && pUnit->player == playerSide)
					SetVerbButtons(vPort.targType, vPort.targSerial);
			}
//		}
		gUnit = (grip)unitLib.lUnits[side].NextValue();
	}
	AdjustManaLevel(side, MANA_MEDITATE);	// because of portal
}
*/

bool
World::SpendMana(int player, int manaNeeded, int tileX, int tileY)
{
	int currMana;

	currMana = GetManaLevel(player);

	#ifndef NDEBUG
	BamDebug.Out("SpendMana(p%d m%d x%d y%d) %d avail\n", player, manaNeeded,
		tileX, tileY, currMana);
	#endif

	if(currMana >= manaNeeded)
	{
		AdjustManaLevel(player, -manaNeeded);
		return(TRUE);
	}
	else
	{
		if(player == playerSide && bGlobal.storyLine != SHOW_OFF)
		{
			pWorld->Announce(tileX, tileY, SQUIB_WORLD_MSGS, (int)MSG_NO_MANA);
		}
		return(FALSE);
	}
}

void
World::AddAction(uint32 targetSerial, action newAction, int newX, int newY, int aux1)
{
	int	targetSide;
	Unit	*pUnit;
	grip	gTarget;

	if(newAction == SUMMON || newAction == ADD_COSMETIC_ANI)
	{
		targetSide = targetSerial;
		gTarget = NULL;	
	}
	else
	{
		pUnit = (Unit *)DerefSerial(targetSerial);
		gTarget = pUnit->gSelf;
		targetSide = pUnit->player;
	}

	if(pAltPool->total[targetSide] >= ACTION_POOL_SIZE)
	{
		pMono->Out("World::AddLocalAction() - ACTION_POOL_SIZE exceeded on side[%d]\n", targetSide);
		return;
	}
	pAltPool->gTargets[targetSide][pAltPool->total[targetSide]] = gTarget;
	pAltPool->actions[targetSide][pAltPool->total[targetSide]] = newAction;
	pAltPool->coords[targetSide][pAltPool->total[targetSide]][0] = newX;
	pAltPool->coords[targetSide][pAltPool->total[targetSide]][1] = newY;
	pAltPool->actionAux1[targetSide][pAltPool->total[targetSide]] = aux1;
	pAltPool->total[targetSide]++;

//	#ifndef NDEBUG
//	BamDebug.Out("World::AddAction() - altPool0x%x == %d\n",
//		(int)pAltPool, pAltPool->total[targetSide]);
//	#endif
}

void
World::AddCosmeticAni(int rAni, int tileX, int tileY, int tileXOff, int tileYOff)
{
	if(tileXOff || tileYOff);

	AddLocalAction(NULL, ADD_COSMETIC_ANI, tileX, tileY, rAni);
}

void
World::AddLocalAction(uint32 targetSerial, action newAction, int newX, int newY, int aux1)
{
	#ifndef NDEBUG
	BamDebug.Out("AddLoc: s%d %s(%d)@%d/%d aux%d\n", targetSerial,
		&pVerbNames[newAction][0], (int)newAction, newX, newY, aux1);
	#endif

	switch(newAction)
	{
		case SUMMON:
		case ADD_COSMETIC_ANI:
			AddAction(playerSide, newAction, newX, newY, aux1);
			break;

		default:
			AddAction(targetSerial, newAction, newX, newY, aux1);
			break;
	}
}

void
World::AddRemoteAction(uint32 targetSerial, action newAction, int newX,
	int newY, int aux1)
{
	if(newAction == SUMMON || newAction == ADD_COSMETIC_ANI)
	{
		targetSerial = (playerSide == SIDE1)? SIDE2: SIDE1;	// kludge
	}

	#ifndef NDEBUG
	BamDebug.Out("AddRem: s%d %s(%d)@%d/%d aux%d\n", targetSerial,
		&pVerbNames[newAction][0], (int)newAction, newX, newY, aux1);
	#endif
	AddAction(targetSerial, newAction, newX, newY, aux1);
}

// in case of multiplayer connection failure..
void
World::AITakeOver(void)
{
	char *pString;
	int	otherSide = (pBam->playerSide == SIDE1)? SIDE2: SIDE1;

	// unable to sync with other side - let AI take over enemy side
	pBam->playerTypes[otherSide] = PLAYER_COMPUTER;
	pString = squib1.Load(SQUIB_WORLD_MSGS, (int)MSG_AI_TAKEOVER);
	MsgBoxAdd(pString, map.portalCoords[otherSide][0], map.portalCoords[otherSide][1]);
	MsgBoxDraw();
	ai.Setup(otherSide);

//	// end game
//	tWorldEnds = ATicks();
//	worldEnder = (playerSide == SIDE1)? SIDE2: SIDE1;
}

// Receive info on commands remote is going to execute on NEXT frame, so that
// on that frame we can do them too
bool
World::SyncReceive(void)
{
	if(bGlobal.storyLine != NETGAME || bGlobal.netDisconnect)
	{
		// no error condition to worry about
		return(TRUE);
	}

	if(worldEnder)
	{
		// no error, just don't worry about syncing since we're wrapping up anyway
		return(TRUE);
	}

	int		loop1, coordX, coordY, aux1, error, int1;
	uint32	serialNum;
	struct	BAMActionPacket	*pActions;
	struct	BamPacketHdr		*pBPHdr;
	bool		fFramePacketReceived = FALSE;
	action	action1;
	Unit		*pUnit;

	bool	fOtherPlayers = FALSE;
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		if(pBam->playerTypes[loop1] >= 0)
		{
			fOtherPlayers = TRUE;
			break;
		}
	if(!fOtherPlayers)
	{
		return(TRUE);
	}

	// now it's their turn - wait for incoming packet(s)

	// note: more than one packet type can be received and accepted here,
	// but only one type may end the loop

	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 40);
	pMono->Out("SR: W4 %5d", currFrame + 1);
	pMono->RestoreWindow();
	#endif

	while(!fFramePacketReceived)
	{
		MouseHandler(FALSE);

		// determine target time for timeout
		loop1 = clock() + CLOCKS_PER_SEC * ((currFrame == 0)? 20: 10);

		// start waiting for a packet
		do
		{
			int errVal = pCommMgr->GetError();

			if(errVal != 0)
			{
//				DebugOut ("ERR%d ", errVal);
			}
			
			if(pCommMgr->DataAvailable())
			{
				// nullify timeout time to signify a packet has arrived
				loop1 = 0;
			}
			else
			{
				#ifndef NDEBUG
//				pMono->Out("WAITING for frame%3d\r", currFrame, loop1 - clock());
				#endif
			}
		} while(loop1 && clock() < loop1);

		// if exited without clearing timer
		if(loop1)
		{
			#ifndef NDEBUG
//			DebugOut("TIMEOUT waiting for packet\n");
			#endif
  			bGlobal.netDisconnect = TRUE;
			return(FALSE);
		}

		SPacket1.pData = sPacketData;
		error = pCommMgr->ReceiveData(&SPacket1);
		if(error)
		{
			#ifndef NDEBUG
//			DebugOut("RECV:ERR%d\n", (int)error);
			#endif
			continue;
		}
		#ifndef NDEBUG
//		DebugOut("Packet received\n");
		#endif

		switch(SPacket1.header.ID)
		{
			case PLAYER_LEAVING:
				// do I still have a remote opponent?
				int1 = (playerSide == SIDE1)? SIDE2: SIDE1;
				if(pBam->playerTypes[int1] >= 0)	// if remote player
				{
					// assume only one other player - sync failed
					#ifndef NDEBUG
//					DebugOut("PLAYER_LEAVING\n");
					#endif
					return(FALSE);					
				}
				break;
				
			case PAUSE_ON:
				if(!fIsPaused)
				{
					// prevent PauseRoom() from sending its own packet
					fPauseViaNet = TRUE;
					Pause(TRUE);
				}
				break;

			case PAUSE_OFF:
				if(fIsPaused)
				{
					// prevent PauseRoom() from sending its own packet
					fPauseViaNet = TRUE;
					Pause(FALSE);

					// insure pause button is cleared visually
					BAM_Button	*pButton = ADerefAs(BAM_Button, gMainButton[MAINBTN_FREEZE]);
					if(pButton->currState != TRUE)
						pButton->Select(FALSE);
				}
				break;

			case PLAYER_MSG:
				aux1 = pBam->playerSide == SIDE1? SIDE2: SIDE1;
				LaunchSound(SND_HERALD_TRUMPET, map.portalCoords[aux1][0],
					map.portalCoords[aux1][1]);
				MsgBoxAdd((char *)SPacket1.pData, map.portalCoords[aux1][0], map.portalCoords[aux1][1]);
				MsgBoxDraw();
				break;

			case UNIT_ACTION:
				// parse into action pool
				pActions = (BAMActionPacket *)SPacket1.pData;
				#ifndef NDEBUG
//				DebugOut("RECV: frame%d ticks%d randTest%d\n",
//					pActions->frameNum, pActions->currTicks, pActions->randTest);
				#endif

				if(pActions->frameNum < currFrame)
				{
					#ifndef NDEBUG
//					DebugOut("WARN: old frame!\n");
					#endif
					break;
				}
				else if(pActions->frameNum > currFrame)
				{
					#ifndef NDEBUG
//					DebugOut("ERR: premature frame!\n");
					#endif
					return(FALSE);
				}

				if(fFramePacketReceived)
				{
					#ifndef NDEBUG
//					DebugOut("WARN: currFrame packet already received\n");
					#endif
					break;
				}

//				// do we need to get our random # seeds in sink?
//				if(currFrame == 0 && pCommMgr->GetUserID() != 0)
//					ASeedRandom(pActions->aux1);

				if(pActions->currTicks > ATicks())
				{
					// other machine's tick counter is ahead of ours (suggests other machine is slower, and
					// called UpdateTicks() later than we did).
					// Update our tick counter manually to match theirs, then go on with life.
					#ifndef NDEBUG
//					DebugOut("WARN: ATicks() local%d remote%d\n",
//						ATicks(), pActions->currTicks);
					#endif

					ASetTicks(pActions->currTicks);
				}

				// verify that our RNG was still in sync with remote's RNG
				// as of last packet transmission

				#ifndef NDEBUG
//				DebugOut("RNG sync test: calls%d loc%d rem%d\n",
//					bGlobal.randGenCalls, bGlobal.randTestValue, pActions->randTest);
				#endif

				if(pActions->randTest != bGlobal.randTestValue)
				{
					char string1[80];
					sprintf(string1, "RNG sync test failure. Calls==%d\n", bGlobal.randGenCalls);
					#ifndef NDEBUG
					BamDebug.Out(string1);
					#endif

					vPort.DumpGAnims();

					#ifndef NDEBUG
					BamDebug.CloseFile();
					#endif
					APanic(string1);	// BUGBUG: replace with more tolerant code for shipping
//					ASeedRandom(42);	// both machines will (hopefully) reseed
				}

				int1 = (playerSide == SIDE1)? SIDE2: SIDE1;
				if(GetManaLevel(int1) != pActions->currMana)
				{
					#ifndef NDEBUG
					BamDebug.Out("WARN: Our mana local%d remote%d\n",
						GetManaLevel(int1), pActions->currMana);
					#endif
					SetManaLevel((playerSide == SIDE1)? SIDE2: SIDE1, pActions->currMana);
				}

	  			#ifndef NDEBUG
//				DebugOut("%d actions received\n", pActions->totalActions);
				#endif

				for(loop1 = 0; loop1 < pActions->totalActions; loop1++)
				{
					serialNum = pActions->actions[loop1].unitSerial;
					action1 = (action)(pActions->actions[loop1].newAction);
					coordX = pActions->actions[loop1].coordX;
					coordY = pActions->actions[loop1].coordY;
					aux1 = pActions->actions[loop1].aux1;

//					#ifndef NDEBUG
//					BamDebug.Out("SR: %d] s%d %s(%d)@%d/%d\n", loop1, serialNum,
//						&pVerbNames[action1][0], (int)action1, coordX, coordY);
//					#endif

					if(action1 == SUMMON || action1 == ADD_COSMETIC_ANI)
					{
						AddRemoteAction(serialNum, action1, coordX, coordY, aux1);
					}
					else if(serialNum)
					{
						pUnit = (Unit *)ValidateTarget(serialNum);
						if(!pUnit)
						{
							#ifndef NDEBUG
							BamDebug.Out("%d] unit s%d invalid\n", loop1, serialNum);
							#endif
							continue;
						}
						AddRemoteAction(serialNum, action1, coordX, coordY, aux1);
					}
				}

				fFramePacketReceived = TRUE;
				break;

			case BMSG_SAVE:
				if(netSaveNum)
					APanic("World: var. netSaveNum shouldn't be set yet");
				pBPHdr = (BamPacketHdr*) SPacket1.pData;
				netSaveNum = NET_SAVEGAME_NUM;
				netSerialNum = pBPHdr->serialNum;
				sprintf(saveMessage,"%d",netSerialNum);
				break;
		}
	}
	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 40);
	pMono->Out("            ");
	pMono->RestoreWindow();
	#endif
	return(TRUE);
}

// Send info on commands we're going to execute on NEXT frame, so that on
// that frame remote machine can execute them too.
bool
World::SyncSend(void)
{
	if(bGlobal.storyLine != NETGAME || bGlobal.netDisconnect)
	{
		// no error condition to worry about
		return(TRUE);
	}

	if(worldEnder)
	{
		// no error, just don't worry about syncing since we're wrapping up anyway
		return(TRUE);
	}

	int32		loop1, totalActions, error;
	action	action1;
	Unit		*pUnit;
	ticks_t	currTicks;

	bool	fOtherPlayers = FALSE;
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		if(pBam->playerTypes[loop1] >= 0)
		{
			fOtherPlayers = TRUE;
			break;
		}
	if(!fOtherPlayers)
	{
		return(TRUE);
	}

	pComm->ClearError();		// why?  Cuz Russell said to.

	// prepare the packet
	actionPacket1.frameNum = currFrame + 1;

	// we'll verify later that local && remote RNGs are sync'ed
	actionPacket1.randTest = ARandom(32768);
	bGlobal.randTestValue = actionPacket1.randTest;

	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 40);
	pMono->Out("SS:    %5d", currFrame + 1);
	pMono->RestoreWindow();
	#endif

	#ifndef NDEBUG
	BamDebug.Out("SyncSend() cFrm%d 4Frm%d randT%d m1:%d m2:%d", currFrame,
		currFrame + 1, bGlobal.randTestValue, pWorld->mana[SIDE1], pWorld->mana[SIDE2]);
	#endif

	// bundle all our new action changes for an outgoing message
	actionPacket1.currMana = GetManaLevel(playerSide);
	actionPacket1.totalActions = 0;
	totalActions = pAltPool->total[playerSide];
	for(loop1 = 0; loop1 < totalActions; loop1++)
	{
		action1 = pAltPool->actions[playerSide][loop1];
		actionPacket1.actions[loop1].coordX = pAltPool->coords[playerSide][loop1][0];
		actionPacket1.actions[loop1].coordY = pAltPool->coords[playerSide][loop1][1];
		actionPacket1.actions[loop1].newAction = action1;
		actionPacket1.actions[loop1].aux1 = pAltPool->actionAux1[playerSide][loop1];

		if(action1 == SUMMON || action1 == ADD_COSMETIC_ANI)
		{
//			actionPacket1.actions[loop1].unitSerial = playerSide;	// side identification
//			actionPacket1.actions[loop1].aux1 = (int32)ACOLYTE;
		}
		else
		{
			pUnit = ADerefAs(Unit, pAltPool->gTargets[playerSide][loop1]);
			actionPacket1.actions[loop1].unitSerial = pUnit->serialNum;
		}
		actionPacket1.totalActions++;
	}

	// prepare packet header
	SPacket1.header.ID = UNIT_ACTION;
	SPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player //BUGBUG changed by RW
	SPacket1.header.len = sizeof(BAMActionPacket) - (sizeof(BAMActionPacketCel) * PACKET_MAX_ACTIONS) +
		sizeof(BAMActionPacketCel) * actionPacket1.totalActions;
	SPacket1.pData = &actionPacket1;
	
	currTicks = ATicks();
	#ifndef NDEBUG
	BamDebug.Out("ticks%d %dActions %dBytes\n", currTicks, actionPacket1.totalActions,
		SPacket1.header.len + sizeof(SPacket1));
	#endif
	// at this moment our tick counters MAY be out of sync (in case we got here before the edge of the
	// next tick, but the other machine arrived late).
	// We'll repair this condition when we receive their packet, if we're
	// the ones who turn out to be behind

	// inform other side of our tick counter, in case they need to catch up
	actionPacket1.currTicks = currTicks;

	loop1 = 0;
	do
	{
		pCommMgr->SendData(&SPacket1, FALSE);
		error = pCommMgr->GetError();

		if(!error)
		{
			#ifndef NDEBUG
//			DebugOut("SENT\n");
			#endif
			continue;
		}

		#ifndef NDEBUG
//		DebugOut("ERR%d-%s\n", error, pErrorStrings[error]);
		#endif
		pComm->ClearError();
  		loop1++;
		if(currFrame == 0 && loop1 <= 10)
		{
			// startup frame, so be more forgiving.  Other machine
			// may still be setting up
			sleep(1);
		}
		else if(++loop1 >= 3)
		{				
			#ifndef NDEBUG
//			DebugOut("SyncSend() TIMEOUT\n");
  			#endif
			bGlobal.netDisconnect = TRUE;
			return(FALSE);
		}

		// WARNING: it's tempting, but do NOT call PublishNext() from within
		// this loop!  It may de-sync us.

		if(pContextMgr->fQuitting)
				return(TRUE);
	} while(error);

	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 40);
	pMono->Out("            ");
	pMono->RestoreWindow();
	#endif
	return(TRUE);
}

bool
World::SyncActionsWithRemote(void)
{
/*	int32		loop1, totalActions, coordX, coordY, aux1, error, randTest, int1;
	bool		fOtherPlayers, fWaitingForPacket, fFramePacketReceived;
	Unit		*pUnit;
	uint32	serialNum;
	action	action1;
	ticks_t	currTicks;
	struct	BAMActionPacket	*pActions;
	struct	BamPacketHdr		*pBPHdr;

	if(worldEnder)
	{
		// game already delcared over, just running end animations.
		// In case there is a sync error occuring, stop trying to sync
		currFrame++;
		return(TRUE);
	}

	// determine if there is anyone else with whom to talk
	fOtherPlayers = FALSE;
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		if(pBam->playerTypes[loop1] >= 0)
		{
			fOtherPlayers = TRUE;
			break;
		}
	if(!fOtherPlayers)
	{
		currFrame++;
		return(TRUE);
	}

	DebugOut("\nSync() frame %d\n", currFrame);
	pComm->ClearError();		// why?  Cuz Russell said to.

// the following is handled by Kevin's code, or at main() started if using -NET
//	if(currFrame == 0 && pCommMgr->GetUserID() == 0)	// 1st frame && 1st machine?
//	{
//		uint32 clockVal = clock();
//		ASeedRandom(clockVal);
//		actionPacket1.aux1 = (int32)clockVal;	// other machine will re-seed on same value
//	}

	//if net game save requested notify other side
	if(netSaveNum)
	{
		BPHdr.sourceRoom = bGlobal.roomMgr.curRoomNum;
		BPHdr.serialNum = netSerialNum;
		SPacket1.header.ID = BMSG_SAVE;
		SPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player //BUGBUG changed by RW
		SPacket1.header.len = sizeof(BamPacketHdr);
		SPacket1.pData = &BPHdr;
		DebugOut("Sync() - sending SaveGame packet\n");
		pCommMgr->SendData(&SPacket1);
	}

	actionPacket1.frameNum = currFrame;
	randTest = ARandom(32768);
	actionPacket1.randTest = randTest;	// will be compared for
	// bundle all our new action changes for an outgoing message
	actionPacket1.currMana = GetManaLevel(playerSide);
	actionPacket1.totalActions = 0;
	totalActions = pCurrPool->total[playerSide];
	for(loop1 = 0; loop1 < totalActions; loop1++)
	{
		action1 = pCurrPool->actions[playerSide][loop1];
		coordX = pCurrPool->coords[playerSide][loop1][0];
		coordY = pCurrPool->coords[playerSide][loop1][1];
		aux1 = pCurrPool->actionAux1[playerSide][loop1];
		actionPacket1.actions[loop1].coordX = coordX;
		actionPacket1.actions[loop1].coordY = coordY;
		actionPacket1.actions[loop1].newAction = action1;
		actionPacket1.actions[loop1].aux1 = aux1;

		if(action1 == SUMMON || action1 == ADD_COSMETIC_ANI)
		{
//			actionPacket1.actions[loop1].unitSerial = playerSide;	// side identification
//			actionPacket1.actions[loop1].aux1 = (int32)ACOLYTE;
		}
		else
		{
			pUnit = ADerefAs(Unit, pCurrPool->gTargets[playerSide][loop1]);
			actionPacket1.actions[loop1].unitSerial = pUnit->serialNum;
		}
		actionPacket1.totalActions++;
	}

	// prepare packet header
	SPacket1.header.ID = UNIT_ACTION;
	SPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player //BUGBUG changed by RW
	SPacket1.header.len = sizeof(BAMActionPacket) - (sizeof(BAMActionPacketCel) * PACKET_MAX_ACTIONS) +
		sizeof(BAMActionPacketCel) * actionPacket1.totalActions;
	SPacket1.pData = &actionPacket1;
	
	// make sure ticks are as up-to-date as possible
	UpdateTicks();
	currTicks = ATicks();
	DebugOut("ticks %d. ", currTicks);
	// at this moment our tick counters MAY be out of sync (in case we got here before the edge of the
	// next tick, but the other machine arrived late).
	// We'll repair this condition when we receive their packet, and vice versa.

	// inform other side of our tick counter, in case they need to catch up
	actionPacket1.currTicks = currTicks;

	// send off the packet
	fWaitingForPacket = TRUE;
	loop1 = 0;
	DebugOut("Sync() - sending frame packet.. ");
	while(fWaitingForPacket)
	{
		DebugOut("Sending frame%d..", actionPacket1.frameNum);
		pCommMgr->SendData(&SPacket1);
		error = pCommMgr->GetError();
		if(error)
		{
			DebugOut("ERROR.. ");
			DebugOut(" GetError() == %d\n", error);
			pComm->ClearError();
	  		loop1++;
			if(currFrame == 0 && loop1 <= 10)
			{
				// startup frame, so be more forgiving.  Other machine
				// may just be very slow.
				sleep(1);
			}
			else if(++loop1 >= 3)
			{				
				DebugOut("ABORT\n");
				DebugOut("Too many retries on send, aborting..\n");
	  			bGlobal.netDisconnect = TRUE;
				return(FALSE);
			}

			// BUGBUG Could we be allowed network de-sync as of this point?
//			pEventMgr->PublishNext();

			if(pContextMgr->fQuitting)
				return(TRUE);
		}
		else fWaitingForPacket = FALSE;
	}
	DebugOut("SENT\nReceiving packet.. ");

//	if(actionPacket1.totalActions)
		DebugOut("  %d actions sent\nReceiving.. ", actionPacket1.totalActions);
//	else pMono->Out ("\n");

	// now it's their turn - wait for incoming packet(s)
	fWaitingForPacket = TRUE;
	fFramePacketReceived = FALSE;
	while(fWaitingForPacket)
	{
		if(fFramePacketReceived)
		{
			fWaitingForPacket = FALSE;
			continue;
		}

		loop1 = clock() + CLOCKS_PER_SEC * 10;
		do
		{
			int errVal = pCommMgr->GetError();

			if(errVal != 0)
			{
				DebugOut("ERROR %d waiting for packet\n", errVal);
			}
			
			if(pCommMgr->DataAvailable())
			{
//				DebugOut("pCommMgr->DataAvailable() == TRUE\n");
				loop1 = 0;
			}
			else
			{
				DebugOut("\rReceiving.. %3d\r", currFrame, loop1 - clock());
			}
		} while(loop1 && clock() < loop1);

		// if exited without clearing timer
		if(loop1)
		{
			DebugOut("\n  Timeout waiting for pCommMgr->DataAvailable();\n");
  			bGlobal.netDisconnect = TRUE;
			return(FALSE);
		}

		SPacket1.pData = sPacketData;
		error = pCommMgr->ReceiveData(&SPacket1);
		DebugOut("RECEIVED\n");
		if(error)
		{
			DebugOut(" ERROR from ReceiveData()\n");
			continue;
		}

		switch(SPacket1.header.ID)
		{
			case PLAYER_LEAVING:
				// do I still have a remote opponent?
				int1 = (playerSide == SIDE1)? SIDE2: SIDE1;
				if(pBam->playerTypes[int1] >= 0)	// if remote player
				{
					// assume only one other player - sync failed
					DebugOut("SyncActions(): received PLAYER_LEAVING\n");
					int1 = (pBam->playerSide == SIDE1)? SIDE2: SIDE1;
					pBam->playerTypes[int1] = PLAYER_COMPUTER;
					ai.Setup(int1);
					return(FALSE);					
				}
				break;
				
			case PAUSE_ON:
				if(!fIsPaused)
				{
					// prevent PauseRoom() from sending its own packet
					fPauseViaNet = TRUE;
					Pause(TRUE);
				}
				break;

			case PAUSE_OFF:
				if(fIsPaused)
				{
					// prevent PauseRoom() from sending its own packet
					fPauseViaNet = TRUE;
					Pause(FALSE);

					// insure pause button is cleared visually
					BAM_Button	*pButton = ADerefAs(BAM_Button, gMainButton[MAINBTN_FREEZE]);
					if(pButton->currState != TRUE)
						pButton->Select(FALSE);
				}
				break;

			case PLAYER_MSG:
				aux1 = pBam->playerSide == SIDE1? SIDE2: SIDE1;
				MsgBoxAdd((char *)SPacket1.pData, map.portalCoords[aux1][0], map.portalCoords[aux1][1]);
				MsgBoxDraw();
				break;

			case UNIT_ACTION:
				// parse into action pool
				pActions = (BAMActionPacket *)SPacket1.pData;
				if(pActions->frameNum < currFrame)
				{
					DebugOut("  Warning: Old frame #%d received - ignoring\n", pActions->frameNum);
					break;
				}
				if(pActions->frameNum > currFrame)
				{
					DebugOut("  Error:  Next frame #%d received to soon!\n", pActions->frameNum);
					return(FALSE);
				}

				if(fFramePacketReceived)
				{
					DebugOut("  Packet for current frame already received! Ignoring..\n");
					break;
				}

//				// do we need to get our random # seeds in sink?
//				if(currFrame == 0 && pCommMgr->GetUserID() != 0)
//					ASeedRandom(pActions->aux1);

				if(pActions->currTicks > ATicks())
				{
					// other machine's tick counter is ahead of ours (suggests other machine is slower, and
					// called UpdateTicks() later than we did).
					// Update our tick counter manually to match theirs, then go on with life.
					DebugOut("pActions->currTicks == %d, updating.\n", pActions->currTicks);

					ASetTicks(pActions->currTicks);
				}
				if(pActions->randTest != randTest)
				{
					char string1[80];
					sprintf(string1, "RNG sync test failure. Calls==%d\n", bGlobal.randGenCalls);
					BamDebug.Out(string1);

					vPort.DumpGAnims();

					BamDebug.CloseFile();
					APanic(string1);	// BUGBUG: replace with more tolerant code for shipping
//					ASeedRandom(42);	// both machines will (hopefully) reseed
				}

				if(GetManaLevel((playerSide == SIDE1)? SIDE2: SIDE1) != pActions->currMana)
				{
					DebugOut("Mana level de-sync for opposing side.  Fixing local..\n");
					SetManaLevel((playerSide == SIDE1)? SIDE2: SIDE1, pActions->currMana);
				}

				DebugOut("%d actions received\n", pActions->totalActions);

				for(loop1 = 0; loop1 < pActions->totalActions; loop1++)
				{
					serialNum = pActions->actions[loop1].unitSerial;
					action1 = (action)(pActions->actions[loop1].newAction);
					coordX = pActions->actions[loop1].coordX;
					coordY = pActions->actions[loop1].coordY;
					aux1 = pActions->actions[loop1].aux1;

//					DebugOut("target serial#%d newAction %d:%s @ %d:%d\n", serialNum, action1, &pVerbNames[action1][0],
//						coordX, coordY);

					if(action1 == SUMMON || action1 == ADD_COSMETIC_ANI)
					{
						AddRemoteAction(serialNum, action1, coordX, coordY, aux1);
					}
					else if(serialNum)
					{
						pUnit = (Unit *)ValidateTarget(serialNum);
						if(!pUnit)
						{
							DebugOut("  Error, invalid unit!\n");
							continue;
						}
						AddRemoteAction(serialNum, action1, coordX, coordY, aux1);
					}
				}

				fFramePacketReceived = TRUE;
				break;

			case BMSG_SAVE:
				if(netSaveNum)
					APanic("World: var. netSaveNum shouldn't be set yet");
				pBPHdr = (BamPacketHdr*) SPacket1.pData;
				netSaveNum = NET_SAVEGAME_NUM;
				netSerialNum = pBPHdr->serialNum;
				sprintf(saveMessage,"%d",netSerialNum);
				break;
		}
	}

	currFrame++;
	*/
	return(TRUE);
}

void
World::SwapActionPools(void)
{
	t_actionPool	*pTemp;

	pTemp = pCurrPool;
	pCurrPool = pAltPool;
	pAltPool = pTemp;
	fPoolFlip = (bool)!fPoolFlip;
}

void
World::ProcessActions(void)
{
	int32		actionLoop, sideLoop, loop1, newX, newY, tileX, tileY, aux1;
	action	action1;
	Unit		*pUnit;
	grip		gUnit;
	bool		fFinished;
	MapSpace	*pSpace;

	fFinished = FALSE;
//	#ifndef NDEBUG
//	DebugOut("ProcessActions() SIDE1:%d SIDE2:%d currPool0x%x\n",
//		pCurrPool->total[SIDE1], pCurrPool->total[SIDE2], (int)pCurrPool);
//	#endif

	for(actionLoop = 0; actionLoop < ACTION_POOL_SIZE && !fFinished; actionLoop++)
	{
		fFinished = TRUE;
		for(sideLoop = 0; sideLoop < TOTAL_SIDES; sideLoop++)
		{
			if(actionLoop >= pCurrPool->total[sideLoop])
	  		{
				// no further actions to process for this side - try next
				continue;
			}

			newX = pCurrPool->coords[sideLoop][actionLoop][0];
			newY = pCurrPool->coords[sideLoop][actionLoop][1];
			action1 = pCurrPool->actions[sideLoop][actionLoop];
			gUnit = pCurrPool->gTargets[sideLoop][actionLoop];
			aux1 = pCurrPool->actionAux1[sideLoop][actionLoop];

			#ifndef NDEBUG
			if(bGlobal.storyLine == NETGAME)
			{
				BamDebug.Out("ProcAct: SIDE%d %d] g%d %s(%d)@%d/%d aux%d\n",
					sideLoop, actionLoop, gUnit, &pVerbNames[action1][0],
					(int)action1, newX, newY, aux1);
			}
			#endif

			if(newX < 0 || newX >= WORLD_WIDTH || newY < 0 || newY >= WORLD_HEIGHT)
			{
				#ifndef NDEBUG
				APanic("World::ProcessActions() - pool contains invalid coords");
				#else
				pMono->Out("World::ProcessActions() - pool contains invalid coords");
				continue;
				#endif
			}

			if(action1 == SUMMON)
			{
				if((unitType)aux1 == JUGGERNAUT)
					Summon((unitType)aux1, newX, newY, SIDE3);	// always SIDE3
				else
				{
					pSpace = map.GetStructure(newX, newY);
					if(pSpace && pSpace->h.func == FUNC_PORTAL)
					{
						// override
						if(unitLib.GetUnitGrip(newX, newY))
						{
							// blocked - find a nearby spot
							tileX = -1;
							for(loop1 = 0; loop1 < 5 && tileX == -1; loop1++)
							{
								map.GetSurroundingCoords(newX, newY, loop1, tileX,
									tileY, TRUE);
								do
								{
									if(!unitLib.GetUnitGrip(tileX, tileY))
									{
										tileX = -1;
										continue;
									}
								}
								while(tileX == -1 && pWorld->map.GetSurroundingCoords(newX,
									newY, loop1, tileX, tileY));
							}
						}
						else
						{
							tileX = newX;
							tileY = newY;
						}
						if(tileX != -1)
							Summon((unitType)aux1, tileX, tileY, pSpace->h.owner);
					}
					else Summon((unitType)aux1, newX, newY, sideLoop);
				}
				fFinished = FALSE;

//				DebugOut("Action #%d side%d: x%d y%d SUMMON %d\n", (int)actionLoop, (int)sideLoop,
//					(int)newX, (int)newY, aux1);

				continue;
			}
			else if(action1 == ADD_COSMETIC_ANI)
			{
				fFinished = FALSE;
				pMono->Out("Launch COSM_ANI %d @%d/%d %s\n", aux1, newX, newY,
					sideLoop == pBam->playerSide? "VISIBLE": "INVISIBLE");

				newX = (sideLoop == pBam->playerSide)? newX: -1;

				grip		gAni;
				BAM_Ani	*pAni;

				TRACK_MEM("ActionAni");	gAni = vPort.NewAni(RES_ANIM, aux1, 1,
					PRI_INTERFACE, TRUE, FALSE, 3, newX, newY, FALSE);
				pAni = ADerefAs(BAM_Ani, gAni);
				vPort.CheckAni(gAni);

				continue;
			}

			if(!gUnit)
			{
//				DebugOut("Action #%d side%d: x%d y%d invalid gTarget\n", (int)actionLoop, (int)sideLoop,
//					(int)newX, (int)newY);
				fFinished = FALSE;
				continue;
			}

			if(pMemMgr->CheckGrip(gUnit) != GRIP_VALID)
			{
				pMono->Out("World::ProcessActions() - side[%d] grip %d (%d of %d) invalid!\n",
					sideLoop, gUnit, actionLoop + 1, pCurrPool->total[sideLoop]);
//				DebugOut("Action #%d side%d: x%d y%d invalid gTarget\n", (int)actionLoop, (int)sideLoop,
//					(int)newX, (int)newY);
				fFinished = FALSE;
				continue;
			}
			pUnit = ADerefAs(Unit, gUnit);

			// some actions are performed automatically, and not through the action processing chain
//			DebugOut("Action #%d side%d: s%d x%d y%d a%d\n", (int)actionLoop, (int)sideLoop,
//				pUnit? pUnit->serialNum: NULL, (int)newX, (int)newY, (int)action1);

			switch(action1)
			{
				case ABORT:
					if(pUnit->primaryAction == TRANSFORM)
					{
						// abort transformation
						pUnit->fActionEnding = TRUE;
						pUnit->tNextStateCheck = ATicks();
					}
					break;

				case TRANSFORM:
					pUnit->Transform((unitType)aux1);
					break;

				case DROP:
					pUnit->DropItem();
					break;

				case TRANSFORM_TO_BLDG:
					if(pUnit->currTerrain == FOUND_GENERIC)
					{
						pSpace = map.GetStructure(pUnit->tileX, pUnit->tileY);
						if(pSpace->IsFoundation())
							pSpace->BuildFromFoundation((funcType)aux1);
					}
					else
					{
						// only one option - turret mode will be assumed
						pUnit->SetPrimaryAction(TRANSFORM_TO_BLDG);
					}
					break;
			
				default:
					// we can assume this is a user-prompted action.
					// therefore, make sure any defendX criteria are now cleared.
					if(pUnit->defendTargetAction)
					{
						pUnit->defendTargetAction = NO_ACTION;
					}

					pUnit->SetPrimaryAction(action1, newX, newY);
					break;
			}
			fFinished = FALSE;
		}
	}
	memset(pCurrPool->total, NULL, sizeof(pCurrPool->total));
}

void
World::Cycle(void)		
{
	int			loop1, loop2, count, tileX, tileY;
	ticks_t		currTicks = ATicks();
	Unit			*pUnit;
	unitType		unit1;
	grip			gUnit, gUnit2, gSpace, gAni;
	List			*pList;
	MapSpace		*pSpace;
	WorldMap		*pWorldMap;

	#ifndef NDEBUG
//	DebugOut("World::Cycle() frame%d ticks%d\n", currFrame, currTicks);
	#endif

#ifndef NDEBUG
	if(fShowCurrentUnitPath)
	{	
//		int newX = vPort.CursorX + 1, newY = vPort.CursorY;
//		if(newX >= WORLD_WIDTH)
//		{
//			newX = 0;
//			newY++;
//			if(newY >= WORLD_HEIGHT)
//				newY = 0;
//		}
		if(ARandom(2))
		{
			List	*pList;
			int	newSide = ARandom(TOTAL_SIDES), num;
			pList = &unitLib.lUnits[newSide];
			if(pList->count)
			{
				num = ARandom(pList->count);
				gUnit = (grip)(pList->At(num + 1));
				if(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					vPort.MoveCursor(pUnit->tileX, pUnit->tileY);
				}
			}
		}
		else
		{
			int newX, newY;

			do
			{
				newX = ARandom(WORLD_WIDTH);
				newY = ARandom(WORLD_HEIGHT);
			} while((vPort.fog[newY][newX] & FOG_CENTER));
			vPort.MoveCursor(newX, newY);
		}
	}
#endif

	static time_t	tLastTime = 0;
	// yes, static.  That's okay,	this isn't important. -Van

	time_t 			tCurrTime;
	time(&tCurrTime);
	if(tCurrTime != tLastTime)
	{
		if(fShowCurrentUnitPath)
		{
			// time to show another frame count
			char string1[10];
			sprintf(string1, "%d FPS", framesRun);
			MsgBoxAdd(string1, 0, 0);
			MsgBoxDraw();
		}

		currFPS = framesRun;
		framesRun = 0;
		tLastTime = tCurrTime;
	}
	else
	{
		framesRun++;
	}

	// Vas's damned queued AND chained vocals!
	if(pBam->voiceChains && !pBam->voice1.IsPlaying())
	{
		// speech! speech!
		pBam->voice1.Play(pBam->rVoiceChains[0][0], MAX_VOLUME, MkPan(100, 100), NULL, 1);
		pBam->voice1.AddToChain(pBam->rVoiceChains[0][1]);
		pBam->voice1.AddToChain(pBam->rVoiceChains[0][2]);
		pBam->voiceChains--;
		if(pBam->voiceChains)
		{
			memmove(&pBam->rVoiceChains[0][0], &pBam->rVoiceChains[1][0],
				sizeof(pBam->rVoiceChains[0][0]) * pBam->voiceChains * 3);
		}
	}

	// cycle Attract mode as necessary
	if(tShowOffTimer && ATicks() > tShowOffTimer)
	{
		tShowOffTimer = ATicks() + TICKS_PER_SEC * 10;
		// pick something interesting to look at
		switch(ARandom(3))
		{
			case 0:	// combat
			case 1:	// travel
				// jump to random point in unit list
				if(unitLib.lUnits[SIDE1].count || unitLib.lUnits[SIDE2].count)
				{
					// fast fwd both lists to random pos
					count = unitLib.lUnits[SIDE1].count;
					if(count)
					{
						count = ARandom(count);
						unitLib.lUnits[SIDE1].FirstValue();
						for(loop1 = 0; loop1 < count; loop1++)
							unitLib.lUnits[SIDE1].NextValue();
					}

					count = unitLib.lUnits[SIDE2].count;
					if(count)
					{
						count = ARandom(count);
						unitLib.lUnits[SIDE2].FirstValue();
						for(loop1 = 0; loop1 < count; loop1++)
							unitLib.lUnits[SIDE2].NextValue();
					}

					// start searching for an interesting unit
					tileX = -1;
					count = AMax(unitLib.lUnits[SIDE1].count, unitLib.lUnits[SIDE2].count);

					gUnit = (grip)unitLib.lUnits[SIDE1].NextValue();
					gUnit2 = (grip)unitLib.lUnits[SIDE2].NextValue();

					for(loop1 = 0; loop1 < count && tileX == -1; loop1++)
					{
						if(!gUnit)
						{
							gUnit = (grip)unitLib.lUnits[SIDE1].FirstValue();
						}
						if(!gUnit2)
						{
							gUnit2 = (grip)unitLib.lUnits[SIDE2].FirstValue();
						}
						
						if(gUnit)
						{
							pUnit = ADerefAs(Unit, gUnit);
							gUnit = (grip)unitLib.lUnits[SIDE1].NextValue();
							if(!(vPort.fog[pUnit->tileY][pUnit->tileX] & FOG_CENTER))
							{
								// if fighting, or at least hit recently
								if(pUnit->currAction == ATTACK || (pUnit->lastAttackTicks && pUnit->lastAttackTicks >
									ATicks() - TICKS_PER_SEC * 5))
								{
									tileX = pUnit->tileX;
									tileY = pUnit->tileY;

									// stop here - combat is good
									break;
								}
		 						else if(pUnit->currAction == MOVE_TO)
								{
									tileX = pUnit->tileX;
									tileY = pUnit->tileY;
									// remember this one, but keep going (in case we
									// can find combat)
								}
							}
						}
						if(gUnit2)
						{
							pUnit = ADerefAs(Unit, gUnit2);
							gUnit2 = (grip)unitLib.lUnits[SIDE2].NextValue();
							if(!(vPort.fog[pUnit->tileY][pUnit->tileX] & FOG_CENTER))
							{
								// if fighting, or at least hit recently
								if(pUnit->currAction == ATTACK || (pUnit->lastAttackTicks && pUnit->lastAttackTicks >
									ATicks() - TICKS_PER_SEC * 5))
								{
									tileX = pUnit->tileX;
									tileY = pUnit->tileY;

									// stop here - combat is good
									break;
								}
	 							else if(pUnit->currAction == MOVE_TO && tileX == -1)
								{
									tileX = pUnit->tileX;
									tileY = pUnit->tileY;
									// remember this one, but keep going (in case we
									// can find combat)
								}
							}
						}
 					}
					if(tileX != -1)
						break;
				}
				// fall-thru

			case 2:	// portal
				tileX = map.portalCoords[playerSide][0];
				tileY = map.portalCoords[playerSide][1];
				break;				
		}
		pWorldMap = ADerefAs(WorldMap, gWorldMap);
		vPort.MoveView(tileX - VPORT_WIDTH / 2, tileY - VPORT_HEIGHT / 2);
		vPort.MoveCursor(tileX, tileY);
		pWorldMap->MoveCursor(vPort.ViewX, vPort.ViewY);
	}

//	if(!SyncActionsWithRemote())
//	{
//	}

	bool	fNewState;
	fNewState = (bool)(AGetEventMods() & MOD_CTRL);
	if(bGlobal.storyLine == SHOW_OFF)
	{
		// override during show-off mode
		fNewState = FALSE;
	}

	if(fNewState && !vPort.fRepeatCmdMode && !worldEnder)
	{
		vPort.fAutoCommandMode = FALSE;
		if(vPort.currentVerb)
			vPort.SetCurrentVerb(NO_ACTION);
		vPort.SetRepeatCommandMode(TRUE);

		// if a group currently selected
		if(vPort.groupSize)
		{
			vPort.CleanupGroupCursors();
		}

		pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_REPEAT);
	}
	else if(!fNewState && vPort.fRepeatCmdMode)
	{
		vPort.SetRepeatCommandMode(FALSE);
		pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_HAND);
	}

	if(!music1.IsPlaying())
	{
		if(musicNum)
			music1.Play((uint16) musicNum);

		startup_game = FALSE;
	}

	if(tWorldEnds && currTicks >= tWorldEnds)
	{
		// it's the end of the world!

		// if player has won && was researching
		if(worldEnder == playerSide && currResearchSubject)
		{
			// silently complete research
			bGlobal.unitsResearched[currResearchSubject] = TRUE;
			currResearchSubject = NOUNIT;
		}
		else
		{
			tWorldEnds = 0;
			EndScenario(worldEnder);
			return;
		}
	}

	if(fIsPaused)
	{
		// things to do only during PAUSE mode
		if(clock() > PauseFlashTimer)
		{	
			// currently in pause mode - flash the PAUSE icon
			BAM_Button	*pButton = ADerefAs(BAM_Button, gMainButton[MAINBTN_FREEZE]);
			pButton->Draw();
			pButton->SetCel((pButton->scrim.cel == 1)? 3: 1);
			PauseFlashTimer = clock() + 25;
		}
	}
	else
	{
		// things to do only outside of PAUSE mode
		if(tMinesRelease && currTicks >= tMinesRelease)
		{
			int newSide;

			// periodic monster release
			if(mapResNum - (mapResNum % 10) == 9250)
			{
				tMinesRelease = ATicks() + TICKS_PER_SEC * 70 + ARandom(40);
			}
			else if(mapResNum - (mapResNum % 10) == 9230)
			{
				tMinesRelease = ATicks() + TICKS_PER_SEC * 40 + ARandom(40);
			}

			// release a critter
			do
			{
				if(pBam->playerTypes[SIDE2] == PLAYER_COMPUTER)
					newSide = SIDE2;
				else if(pBam->playerTypes[SIDE1] == PLAYER_COMPUTER)
					newSide = SIDE1;
				else break;
				pList = &unitLib.lUnits[newSide];

				unit1 = NOUNIT;
				if(map.rMapNum % 9230 < 10)
				{
					// keep the peasant population down
					pList = &unitLib.lUnits[SIDE0];
					gUnit = (grip)pList->FirstValue();
					count = 0;
					while(gUnit)
					{
						pUnit = ADerefAs(Unit, gUnit);
						if(pUnit->type == PEASANT)
							count++;
						gUnit = (grip)pList->NextValue();					
					}
					if(count < 6)
						unit1 = PEASANT;
					newSide = SIDE0;
				}
				else
				{
					unit1 = NOUNIT;
					switch(ARandom(4))
					{
						case 0:
							// dont let that side become too powerful
							if(pList->count < 10)
								unit1 = DRUID;
							break;
						case 1:
							// dont let that side become too powerful
							if(pList->count < 10)
								unit1 = RANGER;
							break;
						case 2:
							// keep the wolf population down
							pList = &unitLib.lUnits[SIDE3];
							gUnit = (grip)pList->FirstValue();
							count = 0;
							while(gUnit)
							{
								pUnit = ADerefAs(Unit, gUnit);
								if(pUnit->type == WOLF)
									count++;
								gUnit = (grip)pList->NextValue();					
							}
							if(count < 6)
								unit1 = WOLF;
							newSide = SIDE3;
							break;
						case 3:
							pList = &unitLib.lUnits[SIDE3];
							gUnit = (grip)pList->FirstValue();
							count = 0;
							while(gUnit)
							{
								pUnit = ADerefAs(Unit, gUnit);
								if(pUnit->type == BANSHEE)
									count++;
								gUnit = (grip)pList->NextValue();					
							}
							if(count < 3)
								unit1 = BANSHEE;
							newSide = SIDE3;
							break;
					}
				}

				if(unit1)
				{
					// pick a mineshaft to release him at
					MapSpace *mines[20];
					pList = &map.lBuildings[SIDE0];

					count = 0;
					gSpace = (grip)pList->FirstValue();
					while(gSpace)
					{
						pSpace = ADerefAs(MapSpace, gSpace);
						if((pSpace->h.func == FUNC_MINESHAFT || pSpace->h.func == FUNC_PRISON_PIT) &&
							!unitLib.GetUnitGrip(pSpace->h.xPos, pSpace->h.yPos))
							mines[count++] = pSpace;
						gSpace = (grip)pList->NextValue();
					}

					if(!count)
					{
						unit1 = NOUNIT;
					}
					else
					{
						// search for an adjacent opening
						pSpace = mines[ARandom(count)];
						for(loop1 = ARandom(8), loop2 = 0; loop2 < 8; loop1 = (loop1 + 1) % 8, loop2++)
						{
							tileX = pSpace->h.xPos + offsets[loop1][0];
							tileY = pSpace->h.yPos + offsets[loop1][1];
							if(!unitLib.GetUnitGrip(tileX, tileY))
								break;
						}
						if(!unitLib.GetUnitGrip(tileX, tileY))
						{
							TRACK_MEM("Unit");	gAni = unitLib.NewUnit();
							pUnit = ADerefAs(Unit, gAni);
							pUnit->SetType(unit1);
							pUnit->SetSide(newSide);
							vPort.AddAni(pUnit->gSelf);
							pUnit->fEnteringMineShaft = TRUE;	// prevent automatically falling into chasm
							pUnit->SetTilePos(pSpace->h.xPos, pSpace->h.yPos);
							pUnit->RunAnimation(ST_ANIM_GUARD);
							vPort.CheckAni(gAni);
							if(unit1 == PEASANT || unit1 == WOLF)
								pUnit->FindALeader();

							loop1 = ARandom(8);
							loop2 = 0;
							do
							{
								if(loop2 == 8)
									break;

								tileX = pSpace->h.xPos + offsets[loop1][0];
								tileY = pSpace->h.yPos + offsets[loop1][1];
								loop1 = (loop1 + 1) % 8;
								loop2++;
							}
							while(!pUnit->CanIMoveTo(tileX, tileY));
							if(pUnit->CanIMoveTo(tileX, tileY))
								pUnit->SetPrimaryAction(MOVE_TO, tileX, tileY);
							else pUnit->SetPrimaryAction(MOVE_TO, pUnit->tileX, pUnit->tileY);
						}
						unit1 = NOUNIT;
					}
				}
			} while(unit1);				
		}

		if(currResearchSubject && currTicks >= tCurrResearchPhaseEnd)
		{
			// cycle research

			if((currResearchPhase + 1) >= RESEARCH_PHASES)
			{
				// end of research
				BAM_Button	*pButton;

				if(bGlobal.storyLine == TUTORIAL)
				{
					if(currResearchSubject == GARGOYLE)
					{
						pWorld->SetTutorialGoal(TUT_UNIT_RESEARCHED, TRUE);	
					}
				}

				bGlobal.unitsResearched[currResearchSubject] = TRUE;
				currResearchSubject = NOUNIT;

				pButton = ADerefAs(BAM_Button, gSpendXPButton);
				pButton->Select(FALSE);
				pButton->Listen(TRUE);

				// if player is in the unit selection menu
				if(statBoxCurrMode == UNIT)
				{
					// insure any new options are shown
					SetTransformButtons(vPort.targSerial);
				}

				Announce(map.portalCoords[pBam->playerSide][0], map.portalCoords[pBam->playerSide][1],
					SQUIB_WORLD_MSGS, SQUIB_WORLD_RESEARCHCOMPLETED);
			}
			else
			{
				// next phase of research begins
				CelHeader	*pSrcCel, *pDestCel;
				grip			gRes;

				currResearchPhase++;
				tCurrResearchPhaseEnd = currTicks + tCurrResearchPhaseDuration;

				pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));
				CopyCel(pDestCel, 261, 0, RES_ANIM, 98, currResearchPhase + 1, FALSE);

				TRACK_MEM("ResearchAni");	gRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, 98);
				pSrcCel = ADerefAs(Resource, gRes)->GetHeader(currResearchPhase + 1);
			  	rTemp1.Set(261, 0, 261 + pSrcCel->width - 1, pSrcCel->height - 1);
				AUpdateRect(&rTemp1);
			}
		}

		if(currTicks >= tNextManaBonus)
		{
			// update mana for both players, based on meditating units
	
//			ScanForNewMana(SIDE1);
//			ScanForNewMana(SIDE2);
			if(bGlobal.storyLine == SHOW_OFF)
			{
				AdjustManaLevel(SIDE1, MANA_MEDITATE * 10);
				AdjustManaLevel(SIDE2, MANA_MEDITATE * 10);
			}

			AdjustManaLevel(SIDE1, MANA_MEDITATE);	// because of portal
			AdjustManaLevel(SIDE2, MANA_MEDITATE);	// because of portal
			SetManaLevel(SIDE0, 999);
			SetManaLevel(SIDE3, 999);

			tNextManaBonus = ATicks() + TICKS_PER_SEC * 6;		// original
		}
		else if(currTicks >= tNextWinCheck)
		{
			CheckWinLose(vPort.ViewX + VPORT_WIDTH / 2, vPort.ViewY + VPORT_HEIGHT / 2);
			tNextWinCheck = currTicks + TICKS_PER_SEC * 30;
		}
	
		int32 myMana = mana[playerSide], myLastMana = lastMana[playerSide];
	
		// if mana level has changed for us
		if(mana[playerSide] != lastMana[playerSide])
		{
			DrawManaGuage(mana[playerSide]);
			lastMana[playerSide] = mana[playerSide];
		}
	
		if (aiOn)
		{
			for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
				if(pBam->playerTypes[loop1] == PLAYER_COMPUTER)
					ai.Cycle(loop1);
		}
	
//		if(time(&currTime) % 10 == 0)		// periodic integrity check
//		{
//			uint32		loopX, loopY;
//			Unit			*pUnit;
	
//			// check for leftover grips in grip map (unit is no longer there)
//			if(!fMemCheck)
//			for(loopY = 0; loopY < WORLD_HEIGHT; loopY++)
//				for(loopX = 0; loopX < WORLD_WIDTH; loopX++)
//					if(unitLib.gUnitMap[loopX][loopY])
//					{
//						pUnit = ADerefAs(Unit, unitLib.gUnitMap[loopX][loopY]);
//						if(pUnit->type != JUGGERNAUT && (pUnit->tileX != loopX || pUnit->tileY != loopY))
//							pMono->Out("World::Cycle() - error, unit grip (%d) misplaced at X%d Y%d\n",
//								pUnit->gSelf, loopX, loopY);
//					}
//	//		pMemMgr->HeapCheck();
//			fMemCheck = TRUE;
//		}
//		else fMemCheck = FALSE;
	}
	
	// spot-check the itemmgr
	if(ARandom(2) && itemMgr.lItems.count)
	{
		grip	gItem = (grip)itemMgr.lItems.At(ARandom(itemMgr.lItems.count) + 1);
		BAMItem *pItem = ADerefAs(BAMItem, gItem), *pItem2;
		itemType	item1;

		pItem2 = itemMgr.CheckItem(pItem->tileX, pItem->tileY);
		if(pItem2 != pItem)
		{
			// somehow this item's grip entry has been walked-on.
			// restore it.
			tileX = pItem->tileX;
			tileY = pItem->tileY;
			item1 = pItem->type;

			itemMgr.lItems.Delete((void *)pItem->gSelf);
			vPort.CutAni(pItem->gSelf);
			itemMgr.DeleteItem(pItem);

			itemMgr.PlaceItem(tileX, tileY, item1, 10, FALSE);
		}
	}
	
	BAM_Room::Cycle();

	// anti-piracy crash flag test
	if(ARandom2(15000) == 5213)
		if((bGlobal.antiPiracyCrashFlag & 0x00000F00) != 0xD00)
		{
//			APanic("PIRATE!\n");
			// shotgun to zero page!
			for(loop1 = 0; loop1 < 256; loop1++)
				*((uchar *)(ARandom(256))) = (uchar)ARandom(256);

			// and then take a bite out of the stack
			#ifdef	OS_DOS
			// get the processor flags
			#pragma aux INTS_ENABLE6 =		\
				"pop		bp"					\
				"pop		bp"					\
				"pop		bp"					\
				"pop		bp"					\
				"pop		bp"					\
				"pop		bp"					\
				"pop		bp"					\
				"ret";
			INTS_ENABLE6();
			#endif
		}
	#ifdef DEBUG_BIG_NET
		pMono->Out("World::Cycle() done\n");
	#endif
}

void
World::CheckWinLose(int eventX, int eventY)
{
	int		loop1, loop2, count;
	WinningCondition	*pWinCon;
	MapSpace	*pSpace, *pCamp1 = NULL, *pCamp2 = NULL;
	int		bldgCounts[5][TOTALFUNCTIONS + 1], unitCounts[5][UNITMAXTYPES + 1];
	bool		fHasWon[5] = {FALSE, FALSE, FALSE, FALSE, FALSE};
	grip		gBldg, gUnit;
	Unit		*pUnit;
	UnitGroup	*pGroup;
	itemType	item1;
	BAMItem	*pItem;
	List		*pList;

	if(worldEnder)
	{
		// world is already ending - dont bother
		return;
	}

	// count all unit ownership
	memset(unitCounts, NULL, sizeof(unitCounts));

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		pList = &pWorld->unitLib.lUnits[loop1];
		gUnit = (grip)pList->FirstValue();
		while(gUnit)
		{
			pUnit = ADerefAs(Unit, gUnit);
			gUnit = (grip)pList->NextValue();

			switch(pUnit->primaryAction)
			{
				case DIE:
				case DISBAND:
					break;
		
				default:
					unitCounts[loop1][pUnit->type]++;
					break;
			}
		}
	}

/*	for(loop1 = 0, pGrip = &vPort.gAnims[0]; loop1 <= vPort.highestAnim; loop1++, pGrip++)
	{
		gAni = *pGrip;
		if(!gAni)
			continue;

		pAni = ADerefAs(BAM_Ani, gAni);
		if(pAni->selfType == UNIT)
		{
			pUnit = (Unit *)pAni;
			switch(pUnit->primaryAction)
			{
				case DIE:
				case DISBAND:
					break;

				default:
					unitCounts[pUnit->player][pUnit->type]++;
					break;
			}
		}
	}
*/

	// count all structure ownership
	memset(bldgCounts, NULL, sizeof(bldgCounts));

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		pList = &map.lBuildings[loop1];

		gBldg = (grip)pList->FirstValue();
		while(gBldg)
		{
			pSpace = ADerefAs(MapSpace, gBldg);
			switch((funcType)pSpace->h.func)
			{
				case FUNC_ARBORLODGE:
				case FUNC_BARRACKS:
				case FUNC_CRYPT:
				case FUNC_RUNESTONE:
				case FUNC_TEMPLE:
				case FUNC_HOME:
				case FUNC_KEEP:
				case FUNC_AQUEDUCT:
				case FUNC_GATE:
				case FUNC_CAMP:
				case FUNC_BANISH_STONE:
					// only these bldgs matter

					if(pSpace->h.func == FUNC_CAMP)
					{
						if(pSpace->h.owner == SIDE1)
							pCamp1 = pSpace;
						else if(pSpace->h.owner == SIDE2)
							pCamp2 = pSpace;
					}

					bldgCounts[loop1][pSpace->h.func]++;
					break;
			}
			gBldg = (grip)pList->NextValue();
		}
	}

	// count totals for each side
	for(loop1 = 0; loop1 < 5; loop1++)
	{
		for(loop2 = 0;	loop2 < TOTALFUNCTIONS;	loop2++)
			bldgCounts[loop1][TOTALFUNCTIONS] += bldgCounts[loop1][loop2];
		for(loop2 = 0;	loop2 < UNITMAXTYPES;	loop2++)
			unitCounts[loop1][UNITMAXTYPES] += unitCounts[loop1][loop2];
	}

	// in case we're running a new map with no win conditions set up yet,
	// allow only the sides with win conditions to win
	for(loop1 = 0;	loop1 < MAX_WINCONS;	loop1++)
	{
		pWinCon = &winCons[loop1];
		if(pWinCon->targetSpec != NOTARGET)
			fHasWon[pWinCon->ownerSide] = TRUE;
	}

	// kludge for one of Vas's special win conditions
	if(componentsCaptured == 3)
		fHasWon[playerSide] = TRUE;

	// process all win conditions - if any side fails any one of its conditions, its state is FALSE
	for(loop1 = 0;	loop1 < MAX_WINCONS;	loop1++)
	{
		pWinCon = &winCons[loop1];
		if(pWinCon->targetSpec == NOTARGET)
			continue;

		// if owner already doesnt have a chance
		if(!fHasWon[pWinCon->ownerSide])
			continue;

		switch(pWinCon->targetSpec)
		{
			case TERRAIN:
//				pMono->Out("Rule %d: Side(%d) needs %d units from Side(%d) on terrain %s. ", loop1,
//					pWinCon->ownerSide, pWinCon->targetCount, pWinCon->targetSide,
//					pTileLib->terrains[pWinCon->terrainSpec].szName);

				gUnit = (grip)unitLib.lUnits[pWinCon->targetSide].FirstValue();
				count = 0;
				while(gUnit)
				{
					pUnit = ADerefAs(Unit, gUnit);
					if(pUnit->currTerrain == pWinCon->terrainSpec)
						count++;
					gUnit = (grip)unitLib.lUnits[pWinCon->targetSide].NextValue();
				}
//				pMono->Out("%d counted\n", count);
				if(count == pWinCon->targetCount)
					pWinCon->fSatisfied = TRUE;
				else
				{
					pWinCon->fSatisfied = FALSE;
					fHasWon[pWinCon->ownerSide] = FALSE;
				}
				break;

			case BUILDING:		
//				pMono->Out("Rule %d: Side(%d) needs %d %s on Side(%d).  ", loop1, pWinCon->ownerSide,
//					pWinCon->targetCount,
//					(pWinCon->structSpec == TOTALFUNCTIONS)? "STRUCTS":
//					&szFuncNames[pWinCon->structSpec][0], pWinCon->targetSide);
//				pMono->Out("%d counted\n", bldgCounts[pWinCon->targetSide][pWinCon->structSpec]);
				if(bldgCounts[pWinCon->targetSide][pWinCon->structSpec] == pWinCon->targetCount)
					pWinCon->fSatisfied = TRUE;
				else
				{
					pWinCon->fSatisfied = FALSE;
					fHasWon[pWinCon->ownerSide] = FALSE;
				}
				break;

			case UNIT:
				pGroup = unitLib.GetUnitGroup(pWinCon->unitSpec);
//				pMono->Out("Rule %d: Side(%d) needs %d %s on Side(%d). ", loop1, pWinCon->ownerSide, pWinCon->targetCount,
//					pGroup? pGroup->unitName: "UNITS", pWinCon->targetSide);
//				pMono->Out("%d counted\n", unitCounts[pWinCon->targetSide][pWinCon->unitSpec]);
				if(unitCounts[pWinCon->targetSide][pWinCon->unitSpec] == pWinCon->targetCount)
				{
					pWinCon->fSatisfied = TRUE;
				}
				else
				{
					pWinCon->fSatisfied = FALSE;
					fHasWon[pWinCon->ownerSide] = FALSE;
				}
				break;

			case ITEM:
				pWinCon->fSatisfied = FALSE;
				item1 = pWinCon->itemSpec;
//				pMono->Out("Rule %d: Side(%d) needs %d %s on Side(%d) camp. ", loop1, pWinCon->ownerSide,
//					pWinCon->targetCount, (item1 == MAXITEMTYPES)? "ANY": &itemMgr.itemNames[item1][0],
//					pWinCon->targetSide);
				if(pWinCon->targetSide == SIDE1)
					pSpace = pCamp1;
				else if(pWinCon->targetSide == SIDE2)
					pSpace = pCamp2;
				else
				{
					fHasWon[pWinCon->ownerSide] = FALSE;
					break;
				}

				for(loop2 = 0, count = 0; loop2 < pSpace->size; loop2++)
				{
					pItem = itemMgr.CheckItem(pSpace->tiles[loop2][0], pSpace->tiles[loop2][1]);
					if(pItem && pItem->type == item1)
						count++;
				}
//				pMono->Out("%d counted\n", count);
				if(count == pWinCon->targetCount)
					pWinCon->fSatisfied = TRUE;
				else
				{
					fHasWon[pWinCon->ownerSide] = FALSE;
				}
				break;
				
			case NOTARGET:
				break;		
		}

		if(pWinCon->fSatisfied && pWinCon->rSatisfiedAni)
		{
			// launch fireworks to celebrate (or whatever)
			grip	gAni;
			TRACK_MEM("WinAni");	gAni = vPort.NewAni(RES_ANIM, pWinCon->rSatisfiedAni, 1,	PRI_PROJECTILE, TRUE,
				FALSE, 4, pWinCon->aniX, pWinCon->aniY, TRUE);
			vPort.CheckAni(gAni);
		}
	}

	if(!pBam->fUseWinLose)
		return;

	if(fHasWon[SIDE1] || fHasWon[SIDE2])
	{
		// show ending action
		vPort.MoveView(eventX - VPORT_WIDTH / 2, eventY - VPORT_HEIGHT / 2);
		vPort.MoveCursor(eventX, eventY);
		ADerefAs(WorldMap, gWorldMap)->MoveCursor(vPort.ViewX, vPort.ViewY);

		// start countdown to end of map
		tWorldEnds = ATicks() + TICKS_PER_SEC * 4;
		if(fHasWon[SIDE2])
			worldEnder = SIDE2;
		else worldEnder = SIDE1;

		// shut down AI
		aiOn = FALSE;

		pWorld->DeselectAllVerbButtons();
		DrawStatusBox();
	}
}

void
World::PauseRoom(bool fState)
{
	int	loop1;
	BAM_Button	*pButton;

//	DebugOut("World::PauseRoom(%d) - fIsPaused == %d\n", (int)fState, (int)fIsPaused);

	if(fIsPaused == fState)
	{
		// no change - ignore it
		return;
	}

	if(fIsPaused)		// if already on and turning off
	{
		pButton = ADerefAs(BAM_Button, gMainButton[MAINBTN_FREEZE]);
		if(pButton->currState != TRUE)	// if pause button wasn't currently being clicked
			pButton->Select(FALSE);		// make sure it's in the OFF visual state

		pButton = ADerefAs(BAM_Button, gSpendXPButton);

		// if not in the process of researching, restore research button
		if(!currResearchSubject)
		{
			pButton->Listen(TRUE);
			pButton->Select(FALSE);
		}

		//ResumeTicks();
	}
	else	// pause off, and turning on
	{	
		PauseFlashTimer = clock();		// cause first cel flip to occur right away

		// if not currently researching something
		if(!currResearchSubject)
		{
			// disable research button
			pButton = ADerefAs(BAM_Button, gSpendXPButton);
			pButton->Select(DG_DISABLED);
		}

		//PauseTicks();
	}

	fIsPaused = fState;

	if(!fPauseViaNet && !bGlobal.netDisconnect)
	{
		// inform other players of pause condition
		SPacket1.header.ID = fIsPaused? PAUSE_ON: PAUSE_OFF;

		SPacket1.pData = "1234567";	// net code hates empty packets
		SPacket1.header.len = strlen((char *)SPacket1.pData) + 1;

		for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
			if(pBam->playerTypes[loop1] >= 0)
			{
				SPacket1.header.destID = pCommMgr->GetUserID () ? 0 : 1;	// other player //BUGBUG changed by RW
				pCommMgr->SendData(&SPacket1);
			}
	}
	else fPauseViaNet = FALSE;

	BAM_Room::PauseRoom(fState);

	if(statBoxCurrMode == TRANSFORMING)
	{
		vPort.SetCurrentVerb(NO_ACTION);
		SetVerbButtons(vPort.targType, vPort.targSerial);

		// restore view of transforming unit, just to be slick
		vPort.MoveCursor(vPort.CursorX, vPort.CursorY);
	}
	else
	{
		vPort.SetCurrentVerb(NO_ACTION);
		SetVerbButtons(vPort.targType, vPort.targSerial);
	}
}

void
World::Setup(void)
{
	TPalette		*pPal;
	Guy			*pGuy;
	BAM_Button	*pButton;
	BAM_Ani		*pAni;
	WorldMap		*pWorldMap = ADerefAs(WorldMap, gWorldMap);
	tile			*pTile;
	CelHeader	*pCel, *pAniCel;
	uchar			*pCelData, *pAniCelData, *pClut1, *pClut2, *pClut3, *pClut4;
	grip			gAniRes;
	int			loop1, loop2, x, y, aniNum, loopX, loopY, playerCount;
	MapSpace		*pSpace;
	char			*pString;

//	pMono->Suspend();

	currFPS = 0;
	pCurrPool = &actPool1;
	pAltPool = &actPool2;
	fPoolFlip = FALSE;

	AllowMouseDragging(FALSE);

	if(bGlobal.altMusicNum)
		musicNum = bGlobal.altMusicNum;

	pBam->voiceChains = 0;
	tMinesRelease = NULL;

	// turn sound FX (not music) off until setup is complete
	int masterDigiVolume = pSoundMgr->GetMasterDigiVolume();
	pSoundMgr->SetMasterDigiVolume(0);

	#ifdef OS_MAC
		startup_game = TRUE;
	#endif

	#ifndef NDEBUG
//	DebugOut("World::Setup()\n");
	#endif

	memset(pBam->unitsCreated       ,NULL,sizeof(pBam->unitsCreated));
	memset(pBam->unitsLost          ,NULL,sizeof(pBam->unitsLost));
	memset(pBam->enemiesSlain       ,NULL,sizeof(pBam->enemiesSlain));
	memset(pBam->structuresDestroyed,NULL,sizeof(pBam->structuresDestroyed));
	memset(pBam->sitesControlled    ,NULL,sizeof(pBam->sitesControlled));
	
	tWorldEnds = 0;	// game not over yet
	tUnitAttackTimer = 0;
	tBldgAttackTimer = 0;
	worldEnder = NULL;
	mapResNum = atoi(pBam->scenarioName);
	pFontMgr->colors[9] = CI_BLACK;

	if(bGlobal.storyLine == NETGAME)
	{
		// time to pick sides
		pBam->playerSide = ARandom(2)? SIDE1: SIDE2;

		if(pCommMgr->GetUserID() == 0)
			pBam->playerSide = (pBam->playerSide == SIDE1)? SIDE2: SIDE1;

		pMono->Out("UserID()==%d, playerSide==%d\n", pCommMgr->GetUserID(),
			pBam->playerSide);

		pBam->playerTypes[pBam->playerSide] = PLAYER_LOCAL;
		pBam->playerTypes[(pBam->playerSide == SIDE1)? SIDE2: SIDE1] = pCommMgr->GetUserID();

		//playerSide was already set in kev's code and the respective sides
      //had there colors load based on it -so we need to reassign the colors
		//in case the sides have switched.
		pBam->sideColors[pBam->playerSide] = (unitColor_t) ((bGlobal.curBanner/2) + 1);
		pBam->sideColors[(pBam->playerSide==SIDE1)?SIDE2:SIDE1] = (unitColor_t) ((bGlobal.enemyBanner/2) + 1);
	}
	else if(bGlobal.storyLine == SHOW_OFF)
	{
		// auto-song-and-dance mode
		pBam->playerSide = SIDE1;
		pBam->playerTypes[SIDE1] = PLAYER_COMPUTER;
		pBam->playerTypes[SIDE2] = PLAYER_COMPUTER;
		tWorldEnds = ATicks() + SHOW_OFF_TIME;
		worldEnder = SIDE0;
		#ifdef INTERACTIVE_DEMO
		switch(ARandom(2))
		{
			case 0:
				mapResNum = 9110;
				break;

			case 1:
				mapResNum = 9310;
				break;

//			case 2:
//				mapResNum = 9250;
//				break;

		}
		#else
			#ifndef NDEBUG
			if(ARandom(16) == 0)
				mapResNum = 9410;
			else mapResNum = 9110 + ARandom(15) * 20 + ARandom(2) * 5;
			#else
			switch(ARandom(6))
			{
				case 0:
					mapResNum = 9115;
					break;
				case 1:
					mapResNum = 9195;
					break;
				case 2:
					mapResNum = 9235;
					break;
				case 3:
					mapResNum = 9250;
					break;
				case 4:
					mapResNum = 9310;
					break;
				case 5:
					mapResNum = 9355;
					break;
			}
			#endif
		#endif
		}

	if(ATicks() == 0)
	{
		// necessary because all anim entities default their tLastCycle val to 0, which blocks cycling for
		// startup tick otherwise, because anims think they've already cycle'd for this tick
		ASetTicks(1);
	}

	statBoxCurrMode = NOTARGET;
	componentsCaptured = 0;
	fPauseViaNet = FALSE;
	fIsPaused = FALSE;
	tileResNum = mapResNum - mapResNum % 20;
	currResearchSubject = NOUNIT;
	nextSerialNum = 1;
	memset(serialNums, NULL, sizeof(serialNums));
	
//	OS_ShutDownVideo(pGraphMgr->vidMode);
//	printf("test #1\n");
//	int loop9;
//	Gun gun1;
//	gun1.r = 255;
//	gun1.g = 255;
//	gun1.b = 255;
//	for(loop9 = 1; loop9 < 256; loop9++)
//	{
//		ASetPalette(&gun1, loop9, loop9);
//	}
//	printf("test #2\n");

	// usually restored in World::Cycle() but someone in Setup() may need it before then
	pTileLib = (TileLibRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_TILELIB, tileResNum));
	if(!pTileLib)
	{
		char err[80];
		sprintf(err, "World::Setup() - !AGetResData(%d, %d)\n", (int)RES_TILELIB, tileResNum);
		APanic(err);
		return;
	}

	// setup 2 color look-up tables (CLUTs)

//	pMono->Out("World::Setup() - bug always crashes before here.\n");
	pClut1 = ADerefAs(uchar, clut[SIDE0]);
	pClut2 = ADerefAs(uchar, clut[SIDE1]);
	pClut3 = ADerefAs(uchar, clut[SIDE2]);
	pClut4 = ADerefAs(uchar, clut[SIDE3]);

	for(loop1 = 0;	loop1 < CLUT_SIZE; loop1++)
	{
		pClut1[loop1] = (uchar)(CLUT0_START + loop1);		// neutral
		pClut2[loop1] = (uchar)(CLUT1_START + loop1);		// player
		pClut3[loop1] = (uchar)(CLUT2_START + loop1);		// opponent
		pClut4[loop1] = (uchar)(CLUT0_START + loop1);		// wild animals
	}

	// setup the pather
	pMono->Out("  pather.Setup();\n");
	pather.Setup(WORLD_WIDTH, WORLD_HEIGHT, MAX_PATH_SIZE);

//	// load item names
//	for(loop1 = 1; loop1 < (int)ENDOFITEMTYPES; loop1++)
//	{
//		pString = pWorld->squib1.Load(SQUIB_ITEM_NAMES, loop1);
//		strcpy(&itemMgr.itemNames[loop1][0], pString);
//	}


//	pMono->Out("  map.Load();\n");
	sprintf(map.h.LibName, "%d", mapResNum);
	if(!map.Load(mapResNum))
	{
		char err[80];
		sprintf(err, "World::Setup() - !map.Load(%d)\n", mapResNum);
		APanic(err);
		return;
	}

//	pMono->Out("  map.FixupClusters();\n");
	map.FixupClusters();

	if(mapResNum - (mapResNum % 10) == 9250 || mapResNum - (mapResNum % 10) == 9230)
	{
		// periodic monster release
		tMinesRelease = ATicks() + TICKS_PER_SEC * 60 + ARandom(30);
	}

	// starting bonus
	if(bGlobal.storyLine == TUTORIAL)
	{
		AdjustXP(playerSide, 675);
		memset(bGlobal.fTutorialGoals, NULL, sizeof(bGlobal.fTutorialGoals));

		// damage to peasant hut
		pSpace = map.GetStructure(23, 18);
		pSpace->h.currHP -= 30;
	}

	// NOTE: this is done after TileLib is loaded, so that units can pre-calc values based on terrain modifiers
//	pMono->Out("  unitLib.Load();\n");
	unitLib.Load();

	pPal = ADerefAs(TPalette, gPal);
	// round down to nearest 20
	x = (mapResNum / 20) * 20;
	pPal->Load(x);
	// load partial color ranges for SIDE1 and SIDE2 into our palette
	LoadUnitColors(gPal, pBam->sideColors[SIDE1], pBam->sideColors[SIDE2]);

	pGuy = ADerefAs(Guy, gInterfaceScreen);
	// master cel for interface screen - all stuff gets drawn into it
	pCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));
	pCelData = (uchar *)pCel + sizeof(CelHeader);

	TRACK_MEM("gAniRes");	gAniRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, 100);
	pAniCel = (CelHeader *) AGetResData(gAniRes);
	pAniCelData = (uchar *)pAniCel + sizeof(CelHeader);

	CopyCel(pCel, 0, 0, RES_ANIM, 100, 1, FALSE);
	SetFontColors(CI_SKIP, 64, 74);

//	int		squibRes, squibNum;
//	char		*szName;
//	uchar		*pClut;

//	GetAbbrCharName(&squibRes, &squibNum, 0, 0, TRUE);
//	szName = squib1.Load(squibRes, squibNum);
//	sprintf(szString, "%s:%d", szName, bGlobal.missionsDone + 1);
//	pClut = ADerefAs(uchar, clut[playerSide]);
////	SetFontColors(CI_SKIP, *(pClut + 4), *(pClut + 1));
//	ASetString(254, 10, szString, (uchar *)pCel, pCel->width, NULL);

	pGuy->SetRes(RES_CEL, rISCel);
	pGuy->SetPos(0, 0);
	pGuy->SetRect();
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(PRI_INTERFACE);
	// viewport cursor flash
	TRACK_MEM("CursorFlash");	pGuy->SetupUtil(new PalCycler(PCYC_FORWARD, 220, 223, gPal, 2, 0));

//	pMono->Out("  vPort.Setup();\n");
	vPort.Setup(gSelf);
  	vPort.Draw();

//	pMono->Out("  Processing tile config\n");
	ProcessTileConfig();
//	pMono->Out("  Processing scene config\n");

	#define MAX_PRE_DEFOG	10
	int preDefog[MAX_PRE_DEFOG][4], preDefogCount = 0;
	ProcessSceneConfig(preDefog, &preDefogCount);
			
	if(bGlobal.storyLine == SHOW_OFF)
	{
		// make the AI put on a real show
		ai.SetAggression(99);
		ai.SetWits(99);
		ai.SetSpellIQ(99);
		ai.SetTerrainIQ(99);
		ai.SetCaution(50);
	}

	if(map.fLegendary)
		map.ApplyRandomChanges();
	map.ClearFoundationOwnerships();

	if(map.fLegendary)
	{
		GenerateWinLose();
	}

//	pMono->Out("  SetupVerbButtons()\n");
	SetupVerbButtons();
	SetStatusBoxMode(TERRAIN);

//	pMono->Out("  setting up main buttons\n");
	for(loop1 = 0; loop1 < MAINBTN_TOTAL; loop1++)
	{
		x = mainButtonCoords[loop1][0];
		y = mainButtonCoords[loop1][1];
		aniNum = mainButtonAnis[loop1];

		pButton = ADerefAs(BAM_Button, gMainButton[loop1]);
		pButton->Create(x, y, 300, RES_ANIM, aniNum, 1, gSelf);
		if(bGlobal.storyLine == SHOW_OFF &&
			((mainButtons)loop1 != MAINBTN_FREEZE && (mainButtons)loop1 != MAINBTN_QUESTION
			&& (mainButtons)loop1 != MAINBTN_ENCYCLO))
		{
			// dont let the user do nuthin'
			pButton->SetupReplies(NULL);
		}
		else if(mainButtonKeys[loop1])
			pButton->SetupReplies(REPLY_SELECTED_KEY, (void *)mainButtonKeys[loop1]);
		else pButton->SetupReplies(REPLY_ACTIVATED);

		switch((mainButtons)loop1)
		{
//			case MAINBTN_SEND:
//				pButton->fIsToggle = TRUE;
//				break;

			default:
				pButton->fIsToggle = FALSE;		// click-type button
				break;
		}
		pButton->SetOwnerCel(rISCel);		// draws itself into this DCEL, instead of being drawn by Animate() directly
		if((mainButtons)loop1 == MAINBTN_SEND)
		{
			// verify that there is someone to send to
			for(loop2 = 0, playerCount = 0; loop2 < TOTAL_SIDES; loop2++)
			{
				if(pBam->playerTypes[loop2] >= 0)
				{
					playerCount++;
				}
			}

			// if no other players
			if(!playerCount)
			{
				pButton->Listen(FALSE);
			}
			else
			{
				// set button to unselected state - will cause drawing into master cel
				pButton->Select(FALSE);
			}
		}
		else pButton->Select(FALSE);				// set button to unselected state - will cause drawing into master cel
	}

	// GoTo unit/struct/terrain button (contains portrait of dest)
	pButton = ADerefAs(BAM_Button, gPortrait);
	pButton->Create(0, 320, 300, RES_ANIM, 104, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rISCel);
	pButton->Draw();
	pButton->Listen(TRUE);

	pButton = ADerefAs(BAM_Button, gGoSeeButton);
	pButton->Create(282, 244, 300, RES_ANIM, 112, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rISCel);
//	pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_CENTER);
//	pButton->SetTextOffSet(3, 0);
//	pButton->SetCelText(1, "GO\nSEE");
//	pButton->SetColors(1, 93, 90);							// inactive colors
//	pButton->SetCelText(2, "GO\nSEE");
//	pButton->SetColors(2, 93, 90);							// active colors
	pButton->SetCel(3);			// default state == no button yet
	pButton->Draw();
	pButton->Listen(FALSE);	// turn off click response

	pButton = ADerefAs(BAM_Button, gSpendXPButton);
	pButton->Create(261, 0, PRI_INTERFACE + 1, RES_ANIM, 101, 1, gSelf);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->SetOwnerCel(rISCel);
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pString = squib1.Load(SQUIB_WORLD_MSGS, SQUIB_WORLD_RESEARCH);
	pButton->SetCelText(1, pString);
	pButton->SetColors(1, 93, 90);							// inactive colors
	pButton->SetCelText(2, pString);
	pButton->SetColors(2, 93, 90);							// active colors
	pButton->SetButtonRes(DG_DISABLED, RES_ANIM, 101, 3);				// disabled state
	pButton->Listen(TRUE);
	pButton->Select(FALSE);

	// create Note button
//	pButton = ADerefAs(BAM_Button, gNoteSend);
//	pButton->Create(0, 351, 310, RES_ANIM, 120, 1, gSelf);
//	pButton->SetupReplies(REPLY_ACTIVATED);
//	pButton->fIsToggle = FALSE;
//	pButton->SetOwnerCel(rISCel);
//	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
//	pButton->SetTextOffSet(0, -1);
//	pButton->SetCelText(1, "SEND");
//	pButton->SetColors(1, 93, 90);							// inactive colors
//	pButton->SetCelText(2, "SEND");
//	pButton->SetColors(2, 93, 90);							// active colors
//	pButton->Listen(FALSE);
//
//	pButton = ADerefAs(BAM_Button, gNoteExit);
//	pButton->Create(0, 375, 310, RES_ANIM, 120, 1, gSelf);
//	pButton->SetupReplies(REPLY_ACTIVATED);
//	pButton->fIsToggle = FALSE;
//	pButton->SetOwnerCel(rISCel);
//	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
//	pButton->SetTextOffSet(0, -1);
//	pString = pWorld->squib1.Load(SQUIB_WORLD_MSGS, MSG_EXIT);
//	pButton->SetCelText(1, pString);
//	pButton->SetColors(1, 93, 90);							// inactive colors
//	pButton->SetCelText(2, pString);
//	pButton->SetColors(2, 93, 90);							// active colors
//	pButton->Listen(FALSE);

	pButton = ADerefAs(BAM_Button, gNoteButton);
	pButton->Create(0, 351, 300, RES_ANIM, 118, 1, gSelf);
	pButton->SetupReplies(REPLY_ACTIVATED);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(rISCel);
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_TOP);
	pButton->SetTextOffSet(0, -1);
	pButton->SetCelText(1, "NOTE");
	pButton->SetColors(1, 93, 90);							// inactive colors
	pButton->SetCelText(2, "NOTE");
	pButton->SetColors(2, 93, 90);							// active colors
//	pButton->Select(FALSE);
 	pButton->Listen(FALSE);

	if(gItemPortrait)
	{
		pButton = ADerefAs(BAM_Button, gItemPortrait);
		pButton->Create(282, 320, 300, RES_ANIM, 106, 1, gSelf);
		pButton->fIsToggle = TRUE;
		pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
		pButton->SetOwnerCel(rISCel);
		pButton->Select(FALSE);
		pButton->Listen(FALSE);
	}

	pMono->Out("  Loading font\n");
	// default font
	pFontMgr->SetRes(9050);
	// default font  color
	SetFontColors(CI_SKIP, 64, 74);

	SetupEditMode();

	// setup initial mana levels
	SetManaLevel(SIDE1, MANA_SUMMON);
	SetManaLevel(SIDE2, MANA_SUMMON);
	playerSide = pBam->playerSide;		// World->playerSide is what game code follows

	for(loop1 = 0; loop1 < MSGBOX_MAX; loop1++)
	{
		if(MsgBox[loop1])
			ADelete(MsgBox[loop1]);
		MsgBox[loop1] = NULL;
		MsgBoxDraw();
	}

	// force an update
	bGlobal.curXP[playerSide]++;
	AdjustXP(playerSide, -1);

	pWorldMap = ADerefAs(WorldMap, gWorldMap);
	pWorldMap->Setup(gSelf, ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));

	// populate the world
//	pMono->Out("  SetupUnits();\n");
	SetupUnits();

	// remove fog from areas around player's units and structures
	vPort.DefogAll(playerSide);

	for(loop1 = 0; loop1 < preDefogCount; loop1++)
	{
		vPort.Defogger(preDefog[loop1][0], preDefog[loop1][1], preDefog[loop1][2], preDefog[loop1][3]);
	}

	// create items in the world
//	pMono->Out("  SetupItems();\n");
	SetupItems();

	// search for terrain anims and active terrain to launch
	x = WORLD_WIDTH / 2;
	y = WORLD_HEIGHT / 2;
	for(loopY = 0; loopY < WORLD_HEIGHT; loopY++)
		for(loopX = 0; loopX < WORLD_WIDTH; loopX++)
		{
			pTile = map.GetTile(loopX, loopY);
			pSpace = map.GetStructure(loopX, loopY);

/*			if(pTile->aniRes)
			{	
				TRACK_MEM("BAMMapAni");	pAni = new BAM_Ani;
				lBackgroundAnis.Add((void *)pAni->gSelf);		// save grips to background ani's

				pAni->SetContext(gSelf);		// only used for sending REPLY type msgs, if any
				pAni->SetTilePos(loopX, loopY);
				// background Animations dont need to be draw on World Map
				pAni->fWorldMapUpdate = FALSE;
	
				pAni->RunAnim(pTile->aniRes, TRUE, TRUE, pTile->aniDelay);

				if(pSpace->h.owner)
					pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[pSpace->h.owner]);

				vPort.AddAni(pAni->gSelf);	// add it to the ViewPort amination system
				vPort.CheckAni(pAni->gSelf);
			}*/

			switch(pTile->terrain)
			{
				case PORTAL:
					// auto-create a portal animation
					TRACK_MEM("CosmeticAni");	gAniRes = vPort.NewAni(RES_ANIM,
						ANIM_PORTAL, 1, PRI_FLOOR, TRUE, TRUE, 2, loopX, loopY, TRUE);
					lBackgroundAnis.Add((void *)gAniRes);

					if(pSpace)
					{
						pAni = ADerefAs(BAM_Ani, gAniRes);
						pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, clut[pSpace->h.owner]);
					}
					vPort.CheckAni(gAniRes);
					// fall-through intentional

				case CAULDRON:
					if(pSpace && pSpace->h.owner == playerSide)
					{
						x = loopX;
						y = loopY;
					}
					break;

				case FLAME_SPOUT:
					TRACK_MEM("FlameSpout");	vPort.NewTerrainUnit(ACTIVE_FLAME_SPOUT, loopX, loopY);
					break;
			}
		}

	vPort.MoveCursor(x, y);
	vPort.MoveView(vPort.CursorX - VPORT_WIDTH / 2, vPort.CursorY - VPORT_HEIGHT / 2);
	pWorldMap->MoveCursor(vPort.ViewX, vPort.ViewY);

	pWorldMap->Draw();

	// setup ai 
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		if(pBam->playerTypes[loop1] == PLAYER_COMPUTER)
			ai.Setup(loop1);

	// move us to the top of the ContextMgr receiver list
	pContextMgr->lContexts.Move((void*)gSelf, L_FRONT, NULL);

	// why do we do the following here?  Because we want to palette fade here,
	//	but we want the screen complete
	vPort.Cycle();

	pMouse->EnableDragging(FALSE);	// just in case
	pMouse->SetFenceRes(RES_ANIM, ANIM_FENCE, 2, RES_ANIM, ANIM_FENCE, 1);
	pMouse->UpdatePointer();
	BAM_Room::Setup();

	#ifdef OS_DOS
		AAnimate();
	#endif

	pMemMgr->HeapCheck();
	pMono->Out("World::Setup() complete\n", AAvailMem());
	ReportFreeMem();

//	pMono->Out("World::Setup() finished\n");
//	if(gPal)	// should always be
//	{
//		pPal = ADerefAs(TPalette, gPal);
//		pPal->FadeUp();
//	}

	// restore master volume
	pSoundMgr->SetMasterDigiVolume((int16)masterDigiVolume);

	Tutorial	*pTutor;
	if(bGlobal.storyLine == TUTORIAL)
	{
//		pMono->Out("  Tutorial->Setup();\n");
		TRACK_MEM("Tutorial"); pTutor = new Tutorial;
		pTutor->Setup(gPal,TRUE,FALSE);
	}
	else if(bGlobal.storyLine == SHOW_OFF)
	{
		pPal = ADerefAs(TPalette, gPal);
		pPal->FadeUp();
		tShowOffTimer = ATicks() + TICKS_PER_SEC * 10;
	}
	else
	{
		// default to WIN/LOSE conditions screen
		// its going to hide under legend opponent screen
		pMono->Out("  WinLose->Setup();\n");
		WinLose	*pWinLose;
		TRACK_MEM("WinLose"); pWinLose = new WinLose;
		pWinLose->Setup(gPal, (bool)(bGlobal.storyLine != LEGEND));

		if(bGlobal.storyLine == LEGEND)
		{
			pMono->Out("  LegendOpponent->Setup();\n");
			LegendOpponent	*pLegendOpp;
			TRACK_MEM("LegendOpp"); pLegendOpp = new LegendOpponent;
			pLegendOpp->Setup(gPal, TRUE);
		}
		else if(bGlobal.storyLine == NETGAME)
		{
			// since we've just come out of a lengthy load & setup phase,
			// our next UpdateTicks() may place us out of sync with the remote
			// machine.  Sync up here.

			bool	fSynching = TRUE;

			#ifndef NDEBUG
//			DebugOut("World::Setup() initial sync start\n");
			#endif
			// make sure ticks are as up-to-date as possible
			UpdateTicks();
			while(fSynching)
			{
				currFrame = -2;	// use invalid #s so if these are left over
				//	 in the data stream, they'll be discarded and not interpretted
				//  (hey, I've been here all night, alright?) -V

				if(!SyncSend())
				{
					// send failed - keep trying
					continue;
				}
 				currFrame = -1;
				if(!SyncReceive())
				{
					// receive failed - keep trying
					continue;
				}
				fSynching = FALSE;
			}
			currFrame = 0;	// back to reality
			#ifndef NDEBUG
//			DebugOut("World::Setup() initial sync done\n");
			#endif
		}
	}
	AllowMouseDragging(TRUE);
}

int
World::StringToSide(char *string)
{
	switch(string[4])
	{
		case '0':
			return(SIDE0);
		case '1':
			return(SIDE1);
		case '2':
			return(SIDE2);
		case '3':
			return(SIDE3);
		default:
			return(-1);
	}
}

bool
World::ProcessSceneConfig(int	pDefogArray[][4], int *pPreDefogCount)
{
	char			*configLine, *pToken, buffer[80];
	Guy			*pGuy;
	MapInfoRes	*pMapInfo;
	int			temp1, x1, x2, y1, y2;
	uint32		loop1, winLoop1, resNum, resX, resY, resSpeed, resPri, resSide;
	WinningCondition *pWinCon;
	grip			gAni;
	BAM_Ani		*pAni;
	MapSpace		*pBldg;

	pMapInfo = (MapInfoRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_MAP_INFO, mapResNum));
	pGuy = ADerefAs(Guy, gInterfaceScreen);

	configLine = &pMapInfo->configLines[0][0];
	winLoop1 = 0;
	for(loop1 = 0; loop1 < pMapInfo->totalConfigLines; loop1++)
	{
		strcpy(buffer, configLine);
		strupr(buffer);
		pToken = GetFirstToken(buffer);

		if(!strcmp(pToken, "#WIN") && !map.fLegendary)
		{
			pWinCon = &winCons[winLoop1];

			pToken = GetNextToken();
			pWinCon->ownerSide = StringToSide(pToken);
			if(pWinCon->ownerSide == -1)
				continue;

			pToken = GetNextToken();
			if(!strcmp(pToken, "STRUCT"))
			{
				pWinCon->targetSpec = BUILDING;
				pToken = GetNextToken();
				pWinCon->structSpec = map.StringToFunction(pToken);
			}
			else if(!strcmp(pToken, "UNIT"))
			{
				pWinCon->targetSpec = UNIT;
				pToken = GetNextToken();
				pWinCon->unitSpec = unitLib.GetUnitType(pToken);
				if(pWinCon->unitSpec == NOUNIT)
					pWinCon->unitSpec = UNITMAXTYPES;
			}
			else if(!strcmp(pToken, "ITEM"))
			{
				pWinCon->targetSpec = ITEM;
				pToken = GetNextToken();
				pWinCon->itemSpec = itemMgr.GetItemType(pToken);
				if(pWinCon->itemSpec == NO_ITEM)
					pWinCon->itemSpec = ENDOFITEMTYPES;
			}
	  		else if(!strcmp(pToken, "TERRAIN"))
			{
				pWinCon->targetSpec = TERRAIN;
				pToken = GetNextToken();
				pWinCon->terrainSpec = map.GetTerrainType(pToken);
			}

			pToken = GetNextToken();
			pWinCon->targetSide = StringToSide(pToken);

			pToken = GetNextToken();
			pWinCon->targetCount = atoi(pToken);

			pToken = GetNextToken();
			if(pToken)
			{
				// ani to run when condition is satisfied
				pWinCon->rSatisfiedAni = atoi(pToken);
				pWinCon->aniX = atoi(GetNextToken());
				pWinCon->aniY = atoi(GetNextToken());
			}
			else
			{
				pWinCon->rSatisfiedAni = NULL;
				pWinCon->aniX = 0;
				pWinCon->aniY = 0;
			}
			winLoop1++;
		}
		else if(!strcmp(pToken, "#SOUND"))
		{
			resNum = atoi(GetNextToken());
			resX = atoi(GetNextToken());
			resY = atoi(GetNextToken());
			LaunchSound(resNum, resX, resY, TRUE);
		}
		else if(!strcmp(pToken, "#MUSIC") && !bGlobal.altMusicNum)
		{
			musicNum = atoi(GetNextToken());
			music1.Play((uint16) musicNum);
		}
		else if(!strcmp(pToken, "#ANIM"))
		{
			resNum = atoi(GetNextToken());
			resX = atoi(GetNextToken());
			resY = atoi(GetNextToken());
			resSpeed = atoi(GetNextToken());
			resPri = atoi(GetNextToken());
			resSide = atoi(GetNextToken() + 4);
			TRACK_MEM("CosmeticAni");	gAni = vPort.NewAni(RES_ANIM, resNum, 1, resPri, TRUE, TRUE, resSpeed,
				resX, resY, TRUE);
			lBackgroundAnis.Add((void *)gAni);		// save grips to background ani's
			pAni = ADerefAs(BAM_Ani, gAni);
			if(resSide)
				pAni->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, clut[resSide]);
			vPort.CheckAni(gAni);

			if(resNum == 326)	// windmill
			{
				// attach it to underlying bldg, if there is one, for
				// auto-takedown if bldg goes down
				pBldg = map.GetStructure(resX, resY);
				if(pBldg && pBldg->h.maxHP)
				{
					pBldg->gKillAni = gAni;
				}
			}
		}
		else if(!strcmp(pToken, "#FIRE"))
		{
			pToken = GetNextToken();
			strupr(pToken);
			if(!strcmp(pToken, "DEFAULT"))
			{
				if(!map.FindBurnableXY(&resX, &resY))
					resX = WORLD_WIDTH + 1;	// intentionally invalid
			}
			else
			{
				resX = atoi(pToken);
				resY = atoi(GetNextToken());
			}
			if(resX < WORLD_WIDTH && resY < WORLD_HEIGHT)
			{
				TRACK_MEM("FireAni");	pAni = pWorld->vPort.NewTerrainUnit(ACTIVE_FIRE, resX, resY);
				pAni->fFireSpread = TRUE;
			}
		}
		else if(!strcmp(pToken, "#DEFOG"))
		{
			x1 = atoi(GetNextToken());
			y1 = atoi(GetNextToken());
			x2 = atoi(GetNextToken());
			y2 = atoi(GetNextToken());
			pDefogArray[*pPreDefogCount][0] = x1;
			pDefogArray[*pPreDefogCount][1] = y1;
			pDefogArray[*pPreDefogCount][2] = x2;
			pDefogArray[*pPreDefogCount][3] = y2;
			(*pPreDefogCount)++;
		}
		else if(!strcmp(pToken, "#AI"))
		{
			if(!bGlobal.aiOveride)
			{
				temp1 = AMin(99, atoi(GetNextToken()) + bGlobal.missionsDone * 3);
				ai.SetAggression(temp1);
				temp1 = AMin(99, atoi(GetNextToken()) + bGlobal.missionsDone * 3);
				ai.SetWits(temp1);
				temp1 = AMin(99, atoi(GetNextToken()) + bGlobal.missionsDone * 3);
				ai.SetCaution(temp1);
				temp1 = AMin(99, atoi(GetNextToken()) + bGlobal.missionsDone * 3);
				ai.SetTerrainIQ(temp1);
				temp1 = AMin(99, atoi(GetNextToken()) + bGlobal.missionsDone * 3);
				ai.SetItemIQ(temp1);
				temp1 = AMin(99, atoi(GetNextToken()) + bGlobal.missionsDone * 3);
				ai.SetSpellIQ(temp1);
			}
		}
		else if(!strcmp(pToken, "#AIUNITPREFS"))
		{
			unitType	newType;

			// input AI unit transformation preferences
			do
			{
				pToken = GetNextToken();
				if(!pToken)
					continue;
				newType = unitLib.GetUnitType(pToken);
				temp1 = atoi(GetNextToken());
				ai.AddUnitPref(SIDE2, newType, temp1);
			} while(pToken);
		}
		else if(!strcmp(pToken, "#AIARG"))
		{
			int	cheatNum = atoi(GetNextToken());
			if(cheatNum > 0 && cheatNum <= AICHEAT_TOTAL)
			{
				ai.cheatChances[cheatNum - 1] = atoi(GetNextToken());
			}
		}
		else if(!strcmp(pToken, "#RESEARCHED"))
		{
			unitType	newType = unitLib.GetUnitType(GetNextToken());
			if(newType)
				bGlobal.unitsResearched[newType] = TRUE;
		}
		configLine += CONFIG_LINE_MAX_LEN;
	}
	return(TRUE);
}

void
World::GenerateWinLose(void)
{
	int		loop1, winLoop;
	bool		fKeep[TOTAL_SIDES];
	grip		gBldg;
	MapSpace	*pSpace;
	WinningCondition *pWinCon;

	// cancel any existing win conditions
	for(loop1 = 0; loop1 < MAX_WINCONS; loop1++)
	{
		winCons[loop1].targetSpec = NOTARGET;
	}
	memset(fKeep, NULL, sizeof(fKeep));

	// find any keeps, and remember who owns them
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		gBldg = (grip)map.lBuildings[loop1].FirstValue();
		while(gBldg)
		{
			pSpace = ADerefAs(MapSpace, gBldg);
			if(pSpace->h.func == FUNC_KEEP)
				fKeep[pSpace->h.owner] = TRUE;
			gBldg = (grip)map.lBuildings[loop1].NextValue();
		}
	}

	winLoop = 0;
	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		if(loop1 == SIDE1 || loop1 == SIDE2)
		{
			pWinCon = &winCons[winLoop++];
			if(fKeep[loop1])	// then destroy keep only
			{
				pWinCon->ownerSide = (loop1 == SIDE1)? SIDE2: SIDE1;
				pWinCon->targetSpec = BUILDING;
				pWinCon->structSpec = FUNC_KEEP;
				pWinCon->targetSide = loop1;
				pWinCon->targetCount = 0;
			}
			else	// then no keep, destroy all struct && units
			{
				pWinCon->ownerSide = (loop1 == SIDE1)? SIDE2: SIDE1;
				pWinCon->targetSpec = BUILDING;
				pWinCon->structSpec = TOTALFUNCTIONS;
				pWinCon->targetSide = loop1;
				pWinCon->targetCount = 0;

				pWinCon = &winCons[winLoop++];
 				pWinCon->ownerSide = (loop1 == SIDE1)? SIDE2: SIDE1;
				pWinCon->targetSpec = UNIT;
				pWinCon->unitSpec = UNITMAXTYPES;
				pWinCon->targetSide = loop1;
				pWinCon->targetCount = 0;
			}
		}
	}
}

bool
World::ProcessTileConfig(void)
{
	char			*configLine, buffer[80], *str;
	Guy			*pGuy;
	MapInfoRes	*pMapInfo;
	int			loop1, loop2;
	bool 			fForward;
	uchar			first, last;
	uint32		rate;
	PalCycler	*pPalC;

	pMapInfo = (MapInfoRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_MAP_INFO, mapResNum));
	pGuy = ADerefAs(Guy, gInterfaceScreen);

	configLine = &pMapInfo->configLines[0][0];
	for(loop1 = 0; loop1 < pMapInfo->totalConfigLines; loop1++)
	{
		strcpy(buffer, configLine);
		str = GetFirstToken(buffer);

		if(!strcmp(str, "#PALCYCLE"))
		{
			fForward = (!strcmp(GetNextToken(), "FOR"))? TRUE: FALSE;
			first = (uchar)atoi(GetNextToken());
  			last = (uchar)atoi(GetNextToken());
			rate = atoi(GetNextToken());

			TRACK_MEM("PalCycler");	pPalC = new PalCycler((fForward? PCYC_FORWARD: PCYC_REVERSE),
				first, last, gPal, rate, NULL);
			pGuy->SetupUtil(pPalC);
		}
		else if(!strcmp(str, "#SWAPSERIES"))
		{
			for(loop2 = 0; loop2 < MAP_SWAP_SERIES_LENGTH; loop2++)	
				map.swapSeries[map.totalSwapSeries][loop2] = 0;

			for(loop2 = 0; loop2 < 10; loop2++)
			{
				str = GetNextToken();
				if(!str)
					break;
				map.swapSeries[map.totalSwapSeries][loop2] = atoi(str);
			}
			map.totalSwapSeries++;
		}
		configLine += CONFIG_LINE_MAX_LEN;
	}
	return(TRUE);
}

void
World::SetupEditMode(void)
{
	BAM_Box	*pBox;

	if(gBox && gBoxText)
	{
		pBox = ADerefAs(BAM_Box, gBox);
		pBox->Create(38, 351, 282, 48, 2000, gBoxText, 45, gSelf, rISCel);
		pBox->SetupReplies(REPLY_DESELECTED);
		pBox->Listen(FALSE);
		pBox->Hide();
//		pBox->Select(TRUE);
	}
}

// create starter units in the world
void
World::SetupUnits(void)
{
	int		loop1, loop2, loop3, totalRandLines, lineCount, percent;
	funcType	func;
	MapSpace	*pSpace;
	grip		gSpace;
	char		*configLine;
	List		lStartSpots(40, TRUE), lMasterStartSpots(40, TRUE);
	char		*pRandUnits[40];
	MapInfoRes	*pMapInfo;

	pMapInfo = (MapInfoRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_MAP_INFO, mapResNum));

	// some default attitude changes
	switch(mapResNum)
	{
		case 9410:
		case 9355:
			// SIDE0 (peasants) hate player by default on these maps,
			// but player does NOT automatically hate back
			unitLib.SetEnemyFlag(SIDE0, playerSide, TRUE);
			break;

		case 9235:
		case 9350:
			// SIDE0 (peasants) hate computer by default on these maps,
			// and vice versa
			unitLib.SetEnemyFlag(SIDE0, (playerSide == SIDE1)? SIDE2: SIDE1, TRUE);
			unitLib.SetEnemyFlag((playerSide == SIDE1)? SIDE2: SIDE1, SIDE0, TRUE);
			break;
			
		case 9232:
			// SIDE0 (peasants) hate everyone by default on these maps.
			unitLib.SetEnemyFlag(SIDE0, SIDE1, TRUE);
			unitLib.SetEnemyFlag(SIDE0, SIDE2, TRUE);
			break;
	}

	// scan through MapSpace list to get all possible "DEFAULT" starting locations
	//	(used when unit startloc == "DEFAULT" in .UNI file)
	for(loop1 = 0; loop1 < MAX_SPACES; loop1++)
	{
		gSpace = map.gSpaces[loop1];
		if(!gSpace)
			continue;

		pSpace = ADerefAs(MapSpace, gSpace);
		func = (funcType)pSpace->h.func;
		switch(func)
		{
			case FUNC_MISC:
				if(map.GetStructure(pSpace->h.xPos, pSpace->h.yPos) == pSpace)
				{
					if(pSpace->h.hotSpot && pSpace->h.destX && pSpace->h.destY)
						lMasterStartSpots.Add((void *)pSpace);
				}
				break;
			case FUNC_ARBORLODGE:
			case FUNC_BARRACKS:
	  		case FUNC_CAULDRON:
			case FUNC_CRYPT:
			case FUNC_RUNESTONE:
			case FUNC_TEMPLE:
			case FUNC_FLAMESPOUT:
			case FUNC_MINESHAFT:
			case FUNC_PORTAL:
			case FUNC_WHIRLPOOL:
			case FUNC_HOME:
				if(map.GetStructure(pSpace->h.xPos, pSpace->h.yPos) == pSpace)
				{
					lMasterStartSpots.Add((void *)pSpace);
				}
				break;
		}
	}

	configLine = &pMapInfo->configLines[0][0];
	for(loop1 = 0;	loop1 < pMapInfo->totalConfigLines; loop1++)
	{
		if(!memcmp(configLine, "#UNITS", 6))
			break;
		configLine += CONFIG_LINE_MAX_LEN;
	}
	configLine += CONFIG_LINE_MAX_LEN;
	while(*configLine == '#' && *(configLine + 1) == ' ')	// while parsing #UNITS section
	{
		if(bGlobal.storyLine != NETGAME)
		{
			ProcessUnitLine(configLine, &lStartSpots, &lMasterStartSpots);
		}
		// else generate no units - we'll use a different rule

		// get next # line
		configLine += CONFIG_LINE_MAX_LEN;
	}

	if(bGlobal.storyLine == NETGAME)
	{
		ProcessUnitLine("# BASAL_GOLEM 1 DEFAULT 2", &lStartSpots, &lMasterStartSpots);
		ProcessUnitLine("# BASAL_GOLEM 2 DEFAULT 2", &lStartSpots, &lMasterStartSpots);
	}
	else if(bGlobal.storyLine == LEGEND)
	{
		int		level, unitCount, totalTypes;
		unitType	newType;
		char		string[80], string2[80], *pStr;

		// give the AI an ACOLYTE boost, except for Hall of Legends
		if(bGlobal.missionsDone < 15)
			for(loop1 = 0; loop1 < bGlobal.missionsDone / 3; loop1++)
				ProcessUnitLine("# BASAL_GOLEM 2 DEFAULT 2", &lStartSpots,
					&lMasterStartSpots);

		// create units for both sides, based on missionsDone
		for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
		{
			// calc how many total units to create for this side (and if it's even a valid side)
			level = (bGlobal.missionsDone + 5) / 5;
			if(loop1 == SIDE1)
				;
			else if(pBam->playerTypes[loop1] == PLAYER_COMPUTER)
				level++;
			else continue;	// invalid side

			if(bGlobal.missionsDone == 15)
				unitCount = 0;	// no extras for Hall of Immortals
			else
			{
				if((loop1 == SIDE1 && level > 3) || (loop1 == SIDE2 && level > 4))
				{
					// last map, only 2 acolytes each
					unitCount = 2;
				}
				else unitCount = level * 2;
			}

			for(; unitCount > 0; unitCount -= 2)
			{
				// determine type of unit to create
				if(unitCount <= 2 || !map.lBuildings[loop1].count)
					newType = ACOLYTE;
				else
				{
					// jump to random slot in buildings list
					loop3 = map.lBuildings[loop1].count;
					if(loop3)
					{
						loop3 = ARandom(loop3);
						map.lBuildings[loop1].FirstValue();
						for(; loop3 >= 0; loop3--)
							map.lBuildings[loop1].NextValue();
					}

					// cycle through buildings list until suitable one found
					loop3 = 0;
					do
					{
						gSpace = (grip)map.lBuildings[loop1].NextValue();
						if(!gSpace)
							gSpace = (grip)map.lBuildings[loop1].FirstValue();
						pSpace = ADerefAs(MapSpace, gSpace);
						loop3++;
					} while(!pSpace->IsSpecialBuilding() && loop3 <= map.lBuildings[loop1].count);

					// did we fail?					
					if(!pSpace->IsSpecialBuilding())
						newType = ACOLYTE;
					else
					{
						totalTypes = 0;
						while(pSpace->GetRelatedUnitType(totalTypes) != NOUNIT)
							totalTypes++;

						if(totalTypes)
						do		// randomly pick a unit type compatible with the selected bldg
						{										
							loop3 = ARandom(totalTypes);
							newType = pSpace->GetRelatedUnitType(loop3);

//							if(newType == ENCHANTER || newType == GOBLIN || newType == HARPY)
//							{
//								// these types are not allowed except for certain sides on certain maps
//								switch(map.rMapNum)
//								{
//									case 9350:
//									case 9370:
//									case 9390:
//										if(loop1 != SIDE2)
//											newType = NOUNIT;
//										break;
//									case 9355:
//									case 9372:
//									case 9375:
//									case 9395:
//										if(loop1 != SIDE1)
//											newType = NOUNIT;
//										break;
//									default:
//										newType = NOUNIT;
//										break;
//								}
//							}
//							else
							if(newType == DOPPLEGANGER)
							{
								newType = NOUNIT;
							}
						} while(newType == NOUNIT);
					}
				}

				// unit type determined - now place 'em
				strcpy(string2, (char *)((UnitGroup *)(unitLib.GetUnitGroup(newType))->unitNameInternal));

				// replace any SPACEs with UNDERSCOREs
				pStr = string2;
				do
				{
					if(*pStr == ' ')
						*pStr = '_';
				} while(*(pStr++));

				sprintf(string, "# %s %d DEFAULT 2", string2,
					(loop1 == SIDE1)? 1: 2);
				ProcessUnitLine(string, &lStartSpots, &lMasterStartSpots);
			}
		}
	}

	if(map.fLegendary || bGlobal.storyLine == SHOW_OFF)
	{
		// do random units for Legendary mode
		// find RANDUNITS chunk
		configLine = &pMapInfo->configLines[0][0];
		for(loop1 = 0;	loop1 < pMapInfo->totalConfigLines; loop1++)
		{
			if(!memcmp(configLine, "#RANDUNITS", 6))
				break;
			configLine += CONFIG_LINE_MAX_LEN;
		}

		// gather all rand lines first
		configLine += CONFIG_LINE_MAX_LEN;
		totalRandLines = 0;
		while(*configLine == '#' && *(configLine + 1) == ' ')	// while parsing #RANDUNITS section
		{
			pRandUnits[totalRandLines++] = configLine;

			// get next # line
			configLine += CONFIG_LINE_MAX_LEN;
		}

		// decide how many to run
		if(bGlobal.storyLine == SHOW_OFF)
		{
			// max neutrals
			lineCount = totalRandLines;
		}
		else if(bGlobal.missionsDone == 15)
		{
			// no units in last Legendary map
			loop1 = 0;
		}
		else
		{
			loop1 = ARandom(100);
			lineCount = 0;

			percent = 5 + (60 * ((bGlobal.missionsDone * 100) / 15)) / 100;
			if(loop1 <= percent)
				lineCount = 3;
			else
			{
				percent += 10 + (60 * ((bGlobal.missionsDone * 100) / 15)) / 100;
				if(loop1 <= percent)
					lineCount = 2;
				else
				{
					percent += 20 + (60 * ((bGlobal.missionsDone * 100) / 15)) / 100;
					if(loop1 <= percent)
						lineCount = 1;
					else lineCount = 0;
				}
			}
		}
		if(totalRandLines && lineCount)
		{
			pMono->Out("RandUnits: %d of %d entries selected\n", lineCount, totalRandLines);
			for(; lineCount >= 0; lineCount--)
			{
				loop2 = ARandom(totalRandLines);
				for(loop3 = 0; loop3 < totalRandLines; loop3++, loop2 = ++loop2 % totalRandLines)
				{
					if(pRandUnits[loop2])
					{
						ProcessUnitLine(pRandUnits[loop2], &lStartSpots, &lMasterStartSpots);
						pRandUnits[loop2] = NULL;
						break;
					}
				}
			}
		}
	}
	
	lStartSpots.Release();
	lMasterStartSpots.Release();
}

bool
World::ProcessUnitLine(char *configLine, List *plSpots, List *plMaster)
{
	char		buffer[160], *pToken, *pTemp;
	unitType	newUnitType;	
	int		loop1, int1, newSide, startX, startY, x, y, unitLoop, unitCount, startHP;
	grip		gUnit;
	Unit		*pUnit;
	bool		fSleeper = FALSE;
	funcType	bldgType, altBldgType, func;
	MapSpace	*pSpace;

	strcpy(buffer, configLine);
	GetFirstToken(buffer);

	pToken = GetNextToken();
	pTemp = pToken;
	while(*pTemp)
	{
		if((*pTemp) == '_')
			*pTemp = ' ';
		pTemp++;
	}
	newUnitType = unitLib.GetUnitType(pToken);
	if(newUnitType == NOUNIT)
	{
		pMono->Out("World::SetupUnits() - error, invalid unit name \"%s\"\n", pToken);
		return(FALSE);
	}

	pToken = GetNextToken();
	switch(*pToken)
	{
		case '1':
			newSide = SIDE1;
			break;
		case '2':
			newSide = SIDE2;
			break;
		case '3':
			newSide = SIDE3;
			break;
		case '0':
		default:
			newSide = SIDE0;
			break;
	}

	pToken = GetNextToken();

	// pick starting location
	if(memcmp(pToken, "DEFAULT", 7))
	{
		// precide coords given - parse
		startX = atoi(pToken);
		if(*(pToken + 2) != ',')
			startY = atoi(pToken + 2);
		else startY = atoi(pToken + 3);
		x = startX;
		y = startY;	// presumed exact coords, as opposed to coords of general area
		startX = -1;	// dont need to search - precise default provided
	}	
	else if(newUnitType == TORTOISE)
	{
		// search for open water
		do
		{
			x = ARandom(WORLD_WIDTH - 1);
			y = ARandom(WORLD_HEIGHT - 1);
		}
		while(map.GetTerrainNum(x, y) != (terrainType)WATER || unitLib.GetUnitGrip(x, y));

		// no need to search
		startX = -1;
		startY = -1;
	}
	else if(newUnitType == BRIGAND || newUnitType == WOLF || newUnitType == BANSHEE ||
		newUnitType == TROLL || newUnitType == ROC_EGG)
	{
		do
		{
			x = ARandom(WORLD_WIDTH - 1);
			y = ARandom(WORLD_HEIGHT - 1);
			pSpace = map.GetStructure(x, y);
		}
		while(unitLib.GetUnitGrip(x, y) || (pSpace && pSpace->h.currHP > 0));

		// no need to search
		startX = -1;
		startY = -1;
	}
	else
	{
		bldgType = GetBldgType(newUnitType);
		altBldgType = GetBldgType(newUnitType, TRUE);

		// now pick an instance of MapSpace from lStartSpots with this func type
		if(plSpots->count == 0)	// if temp list empty
		{
			// copy master list to temp list
			pSpace = (MapSpace *)plMaster->FirstValue();
			while(pSpace)
			{
				plSpots->Add((void *)pSpace);
				pSpace = (MapSpace *)plMaster->NextValue();
			}
		}

		// jump to random point in list
		pSpace = (MapSpace *)plSpots->FirstValue();
		int1 = ARandom(plSpots->count);
		for(loop1 = 0; loop1 < int1; loop1++)
			pSpace = (MapSpace *)plSpots->NextValue();
		if(!pSpace)
		{
			// if landed at end of list, go back to top so we start with something valid at least
			pSpace = (MapSpace *)plSpots->FirstValue();
		}

		// search for suitable building
		func = (funcType)pSpace->h.func;
		bool	fListRefreshed = FALSE;
		while(pSpace && ((func != bldgType && func != altBldgType) || pSpace->h.owner != newSide))
		{
			pSpace = (MapSpace *)plSpots->NextValue();
			if(pSpace == NULL)	// if end of list encountered
			{
				if(fListRefreshed)	// if list already refreshed once
				{
					// error - no such building in this map
					// default to portal
					if(bldgType == FUNC_PORTAL)
						bldgType = FUNC_CAULDRON;
					else if(bldgType == FUNC_CAULDRON)
					{
						pMono->Out("World::SetupUnits() - unable to place unit\n");
						return(FALSE);
					}
					else
						bldgType = FUNC_PORTAL;
				}
				else
				{
					fListRefreshed = TRUE;
				}

				// empty out leftovers, if any
				plSpots->Release();

				// copy master list to temp list
				pSpace = (MapSpace *)plMaster->FirstValue();
				while(pSpace)
				{
					plSpots->Add((void *)pSpace);
					pSpace = (MapSpace *)plMaster->NextValue();
				}

				// start from the beginning
				pSpace = (MapSpace *)plSpots->FirstValue();	
				func = (funcType)pSpace->h.func;
			}
			func = (funcType)pSpace->h.func;
		}
	
		// if found one
		if(pSpace && ((funcType)pSpace->h.func == bldgType || (funcType)pSpace->h.func == altBldgType))
		{
			startX = pSpace->h.xPos;
			startY = pSpace->h.yPos;
			plSpots->Delete((void *)pSpace);	// remove from temp list
		}
		else
		{
			startX = ARandom(WORLD_WIDTH);
			startY = ARandom(WORLD_HEIGHT);
			pMono->Out("World::SetupUnits() - warning, cannot place unit at DEFAULT, using random coords\n");
		}
	}
	
	pTemp = GetNextToken();
	unitCount = atoi(pTemp);

	startHP = 0;
	pTemp = GetNextToken();
	if(pTemp)
	{
		if(atoi(pTemp))
			startHP = atoi(pTemp);
		else if(!memcmp(pTemp, "SLEEP", 6))
			fSleeper = TRUE;
	}

	// BUGBUG - this is an experiment
	if(pBam->playerTypes[newSide] == PLAYER_COMPUTER)
		unitCount *= bGlobal.aiUnitMultiplier;

	for(unitLoop = 0; unitLoop < unitCount; unitLoop++)
	{
		TRACK_MEM("Unit!");			gUnit = unitLib.NewUnit();		// unitLib will keep list of all grips
		pUnit = ADerefAs(Unit, gUnit);
		pUnit->SetType(newUnitType);
		if(fSleeper)
			pUnit->StandGuard(TRUE);
		if(startHP && startHP < pUnit->maxHP)
			pUnit->currHP = startHP;
		pUnit->SetSide(newSide);
		vPort.AddAni(pUnit->gSelf);
	
		if(startX == -1)	// if exact coords already given
		{
			// validate them
			if(pUnit->currMove > 0 && !pUnit->CanIMoveTo(x,y, TRUE))
			{
				startX = x;	// force search after all
				startY = y;
			}
		}

		loop1 = 16;
		// if default x,y invalid, or not specified, then search outward for valid x,y
		if(startX != -1)		// if need to search
		{
			// find an open spot
			do
			{
				// find semi-random coords within world boundaries
				do
				{
					x = startX + (ARandom(loop1 / 16) + 1) * (ARandom(2)? 1: -1);
					y = startY + (ARandom(loop1 / 16) + 1) * (ARandom(2)? 1: -1);
				} while(x < 0 || x > WORLD_WIDTH || y < 0 || y > WORLD_HEIGHT);
				loop1++;
			} while(!pUnit->CanIMoveTo(x, y, TRUE));
		}
		// else no need to search - exact coords already given
		pUnit->RunAnimation(ST_ANIM_WALK, 0, FALSE, pUnit->currDir);
		pUnit->SetTilePos(x, y);

		// some units move in packs
		if(pUnit->type == WOLF || pUnit->type == PEASANT)	// pack animals
		{
			pUnit->FindALeader();
			if(pUnit->fPackLeader)	// if he became a leader instead
			{
				AddAction(pUnit->serialNum, MOVE_TO, pUnit->tileX, pUnit->tileY);
				vPort.CheckAni(pUnit->gSelf);
			}
			// else he'll be following his leader's instructions
		}
		else	// just stand and guard - pack leaders will start their own wandering
		{
			// if units of other human player
			if(newSide != playerSide && pBam->playerTypes[newSide] >= 0)
			{
				// leave them alone - other side with send copies of their initialization orders
			}
			else
			{
				// give initial orders
				AddAction(pUnit->serialNum, GUARD, pUnit->tileX, pUnit->tileY);
				vPort.CheckAni(pUnit->gSelf);
			}
			// else other side will send orders for this unit immediately
		}
		pMono->Out("Unit(g%d, %s) placed at %d/%d\n", pUnit->gSelf, pUnit->unitName,
			pUnit->tileX, pUnit->tileY);
	}
	return(TRUE);
}

funcType
World::GetBldgType(unitType unit1, bool fAlternate)
{
	funcType bldgType;

	switch(unit1)
	{
		case PALADIN:
			if(fAlternate)
			{
				bldgType = FUNC_BARRACKS;
				break;
			}
			// fall-thru

		case CLERIC:
		case FURY:
			bldgType = FUNC_TEMPLE;
			break;

		case DOPPLEGANGER:
		case DRUID:
		case GRIFFIN:
		case NYMPH:
			bldgType = FUNC_ARBORLODGE;
			break;

		case GARGOYLE:
		case GHOUL:
		case HARPY:
		case WRAITH:
		case ZOMBIE:
			bldgType = FUNC_CRYPT;
			break;

		case ENCHANTER:
		case GNOME:
		case GOLEM:
		case WIZARD:
		case WYRM:
			bldgType = FUNC_RUNESTONE;
			break;

		case RANGER:
			if(fAlternate)
			{
				bldgType = FUNC_ARBORLODGE;
				break;
			}
			// fall-thru

		case GOBLIN:
		case WARRIOR:
			bldgType = FUNC_BARRACKS;
			break;

//		case TORTOISE:
//			bldgType = FUNC_MISC;
//			break;

		case PEASANT:
			bldgType = FUNC_HOME;
			break;

  		case ACOLYTE:
		default:
			bldgType = FUNC_PORTAL;
			break;
	}
	return(bldgType);
}

void
World::SetupVerbButtons(void)
{
	uint32		loop1, x, y, aniNum;
	BAM_Button	*pButton;
	grip			gCel;
	Resource		*pRes;
	CelHeader	*pCel;

	// these are for unit transform modes, not regular mode
	for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
	{
		pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
		x = transformButtonCoords[loop1][0];
		y = transformButtonCoords[loop1][1];
		aniNum = acolyteBldgAnis[loop1];
		pButton->Create(x, y, 300, RES_ANIM, aniNum, 1, gSelf, 0, 0, 9064);
		pButton->SetupReplies(REPLY_ACTIVATED);
		pButton->SetColors(1, 93, 90, CI_SKIP);	//, 151, 154, 151);				// inactive colors
		pButton->SetColors(2, 154, 151, CI_SKIP);	// active colors
		pButton->SetTextJustify(DG_JUST_RIGHT, DG_JUST_BOTTOM);
		pButton->SetTextOffSet(-2, -2);
		pButton->fIsToggle = FALSE;
		pButton->Listen(FALSE);
		pButton->SetOwnerCel(NULL);
	}

	pRes = ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_ANIM, 142));
	pCel = pRes->GetHeader(1);
	TRACK_MEM("AcolSkillBtn");	gCel = ACreateCel(&rAcolyteSkillButton, 2, 2, pCel->width, pCel->height, CI_WHITE);

	pRes = ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_ANIM, 144));
	pCel = pRes->GetHeader(1);
	TRACK_MEM("AcolTransfBtn");	gCel = ACreateCel(&rAcolyteTransformGuage, 0, 0, pCel->width, pCel->height, CI_WHITE);

	// these are for regular mode
	for(loop1 = 0; loop1 < 7; loop1++)
	{
		pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);

		x = verbButtonCoords[loop1][0];
		y = verbButtonCoords[loop1][1];
		aniNum = verbButtonAnis[loop1];

		pButton->Create(x, y, 300, RES_ANIM, aniNum, 1, gSelf);

		pButton->SetColors(1, 93, 90, 154, 151, 154, 151);				// inactive colors
		pButton->SetColors(2, 154, 151, 154, 151, 154, 151);			// active colors

		switch((action)(loop1 + 1))
		{
			case DISBAND:
				pButton->SetTextOffSet(0, 3);
				// fall-through intentional

			case DROP:
				pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
				// fall-through intentional

			case GUARD:
				pButton->fIsToggle = FALSE;
				break;

			default:
				pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_CENTER);
				pButton->SetTextOffSet(3, 0);
				pButton->fIsToggle = TRUE;
				break;
		}

		// draws itself into this DCEL, instead of being drawn by Animate() directly
		pButton->SetOwnerCel(rISCel);

		// set button to unselected state - will cause drawing into master cel
		pButton->Select(DG_DISABLED);
	}
}

void
World::SetTransformingButtons(uint32 targetSerial)
{
	Unit			*pUnit;
	UnitGroup	*pGroup;
	TargetType	targType;
	BAM_Button	*pButton;
	int			currPhase, tempX, tempY, loop1;
	Resource		*pRes;
	CelHeader	*pCel;
	char			*pString;

	pUnit = (Unit *)DerefSerial(targetSerial, &targType);
	if(!pUnit || targType != UNIT)
	{
		pMono->Out("SetTransformButtons(): invalid serial given\n");
		return;
	}

	if(statBoxCurrMode != TRANSFORMING)
		APanic("SetTransformingButtons() - wrong mode!\n");

	pButton = ADerefAs(BAM_Button, gPortrait);
	pButton->Select(FALSE);
	pButton->Listen(FALSE);

	// new unit type portrait
 	if(pUnit->metaPodType)
	 	pGroup = unitLib.GetUnitGroup(pUnit->metaPodType);
	else
	{
		// BUGBUG!  Failsafe condition only, this shouldn't be happening!
		pGroup = unitLib.GetUnitGroup(ACOLYTE);
	}

	pButton = ADerefAs(BAM_Button, gTransformButton[0]);
	pButton->SetButtonRes(FALSE, RES_ANIM, pGroup->portAnim, 1);
	pButton->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, clut[pUnit->player]);
	pButton->Select(FALSE);
	pButton->Listen(FALSE);

	// transformation progress guage
	pRes = ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_CEL, rAcolyteTransformGuage));
	pCel = pRes->GetHeader(1);
	CopyCel(pCel, 0, 0, RES_ANIM, 144, 1, TRUE);

	if(pGroup->transformTime)
		currPhase = ((int)ATicks() - pUnit->tAutoActionTimer) / ((pGroup->transformTime * TICKS_PER_SEC * 10) / 70) + 1;
	else currPhase = 7;
	currPhase = AMin(currPhase, 7);

	for(loop1 = 2; loop1 <= currPhase; loop1++)
	{
		tempY = 8;
		switch(loop1)
		{
			case 2:
				tempX = 23;
				break;
			case 3:
				tempX = 35;
				break;
			case 4:
				tempX = 48;
				break;
			case 5:
				tempX = 61;
				break;
			case 6:
				tempX = 75;
				break;
			case 7:
			default:
				tempX = 93;
				tempY = 3;
				break;
		}
		CopyCel(pCel, tempX, tempY, RES_ANIM, 144, loop1, TRUE);
	}

	pButton = ADerefAs(BAM_Button, gTransformButton[1]);
	pButton->SetButtonRes(FALSE, RES_CEL, rAcolyteTransformGuage, 1);
	pButton->Select(FALSE);
	pButton->Listen(FALSE);

	// abort button
	pButton = ADerefAs(BAM_Button, gTransformButton[2]);
	pButton->SetButtonRes(FALSE, RES_ANIM, 105, 1);
	pButton->SetButtonRes(TRUE, RES_ANIM, 105, 2);
	pButton->SetButtonRes(DG_DISABLED, RES_ANIM, 105, 3);
	pButton->SetPos(162, 356);

	if(!fIsPaused && pBam->playerTypes[playerSide] == PLAYER_LOCAL && pUnit->player == playerSide && currPhase < 7)
	{
		// ABORT allowed by player
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetTextOffSet(0, 1);
		pString = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)MSG_ABORT_TRANSFORM);
		pButton->SetCelText(1, pString);
		pButton->SetColors(1, 93, 90, CI_SKIP);							// inactive colors
		pButton->SetCelText(2, pString);
		pButton->SetColors(2, 154, 151, CI_SKIP);							// active colors
		pButton->font = 9050;
		pButton->Select(FALSE);
		pButton->Listen(TRUE);
	}
	else
	{
		// ABORT not allowed by player
//		pButton->SetCelText(1, "");
//		pButton->SetCelText(2, "");
		pButton->Select(DG_DISABLED);
	}

	// disable remaining transform buttons - dont need them for this mode
	for(loop1 = 3; loop1 < TRANSFORM_TOTAL; loop1++)
	{
		pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
		pButton->Listen(FALSE);
		pButton->SetOwnerCel(NULL);
		pButton->Select(FALSE);
	}
}

void
World::SetTransformButtons(uint32 targetSerial)
{
	Unit			*pUnit;
	UnitGroup	*pGroup;
	unitType		unitType1;
	MapSpace		*pSpace;
	TargetType	targType;
	bool			fCanTransform, fGhosted;
	int			loop1;
	int32			resNum;
	BAM_Button	*pButton;
	char			*pString, string1[40];

	pUnit = (Unit *)DerefSerial(targetSerial, &targType);
	if(!pUnit || targType != UNIT)
	{
		pMono->Out("SetTransformButtons(): invalid serial given\n");
		return;
	}

	if(statBoxCurrMode == UNIT && pUnit->currTerrain == FOUND_GENERIC)
	{
		// player was already looking at the UNIT transform mode.. change it
		// for them.
		vPort.SetCurrentVerb(vPort.currentVerb);
	}
	
	switch(statBoxCurrMode)
	{
		case BUILDING:
			fCanTransform = FALSE;
			if(pUnit->currTerrain != FOUND_GENERIC)
			{
				// reset!
				vPort.SetCurrentVerb(vPort.currentVerb);
				break;
			}
			pSpace = map.GetStructure(pUnit->tileX, pUnit->tileY);

			if(!pSpace)
			{
				vPort.SetCurrentVerb(vPort.currentVerb);
				break;
			}
				
			if(pSpace->h.func != FUNC_GEN_FOUND)
			{
				vPort.SetCurrentVerb(vPort.currentVerb);
				break;
			}
			fCanTransform = pSpace->CheckFoundationFilled();
			break;
	}

	pButton = ADerefAs(BAM_Button, gPortrait);
	pButton->SetButtonRes(FALSE, RES_ANIM, 140, 1);
	pButton->SetButtonRes(TRUE, RES_ANIM, 140, 2);
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pString = pWorld->squib1.Load(SQUIB_WORLD_MSGS, MSG_EXIT_MENU);
	pButton->SetCelText(1, pString);
	pButton->SetColors(1, 93, 90);							// inactive colors
	pButton->SetCelText(2, pString);
	pButton->SetColors(2, 93, 90);							// inactive colors
	pButton->Select(FALSE);

	switch(statBoxCurrMode)
	{
		case UNIT:
			pUnit->FindNearbyTransformer(&pSpace);
			if(!pSpace)
			{
				// umm.. whatever.  Cleanup.
				pUnit->CheckForTransformer();
				pUnit->nearbyTransformer = FUNC_MISC;
			}

			for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
			{
				fGhosted = FALSE;
				if(loop1 == 0 || pUnit->nearbyTransformer)
				{
					fCanTransform = TRUE;
				}
				else
				{
					fCanTransform = FALSE;
				}

				pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
				pButton->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, clut[pUnit->player]);
				pButton->Listen(fCanTransform);
				pButton->SetCelText(1, "");
				pButton->SetCelText(2, "");
			
				if(loop1 == 0 && pUnit->type == ACOLYTE)
				{
					// TURRET
					resNum = 2724;
				}
				else if(pSpace)
				{
					if(pUnit->type == ACOLYTE)
					{
						unitType1 = pSpace->GetRelatedUnitType(loop1 - 1);
					}
					else
					{
						unitType1 = pSpace->GetRelatedUnitType(loop1);
					}

					if(fCanTransform && loop1 > 0 && !bGlobal.unitsResearched[unitType1])
						fGhosted = TRUE;					

					if(unitType1 == NOUNIT)
					{
						resNum = 2724;
						fCanTransform = FALSE;
						fGhosted = FALSE;
					}
					else
					{
						pGroup = unitLib.GetUnitGroup(unitType1);
						resNum = pGroup->portAnim;
						sprintf(string1, "%d", pGroup->transformCost);
						pButton->SetCelText(1, string1);
					}
				}
				else
				{
					resNum = 2724;		// just need a safe val - it wont be displayed
					fCanTransform = FALSE;
				}

				pButton->SetButtonRes(FALSE, RES_ANIM, resNum, 1);
				pButton->SetButtonRes(TRUE, RES_ANIM, resNum, 1);

				if(fGhosted)
					pButton->SetButtonRes(DG_DISABLED, RES_ANIM, resNum, 2);
				else
					pButton->SetButtonRes(DG_DISABLED, RES_ANIM, 104, 1);

				if(fCanTransform && !fGhosted)
					pButton->Select(FALSE);
				else pButton->Select(DG_DISABLED);
			}
			break;

		case BUILDING:
			for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
				pButton->Listen(fCanTransform);
				pButton->SetButtonRes(FALSE, RES_ANIM, acolyteBldgAnis[loop1], 1);
				pButton->SetButtonRes(TRUE, RES_ANIM, 104, 1);
				pButton->SetButtonRes(DG_DISABLED, RES_ANIM, acolyteBldgAnis[loop1], 2);
				pButton->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, clut[pUnit->player]);
				pButton->SetCelText(1, "");
				pButton->SetCelText(2, "");

				if(fCanTransform)
					pButton->Select(FALSE);
				else pButton->Select(DG_DISABLED);
			}
			break;

		case TERRAIN:
			// error, but fake it anyway.
			for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
				pButton->Select(DG_DISABLED);
			}
			break;
	}
}

void
World::SetVerbButtons(TargetType target, uint32 targetSerial)
{
	int			loop1, loop2, blackLength, blackPoint;
	BAM_Button	*pButton;
	Unit			*pUnit;
	UnitGroup	*pGroup;
	char			string1[80], *pString;
	uchar			*pCelData, *pCelData2;
	MapSpace		*pSpace;
	CelHeader	*pCel;

	if(statBoxCurrMode != TERRAIN)
	{
		// transformation mode - dont walk over it
		return;
	}

	pFontMgr->SetRes(9050);

	pUnit = (Unit *)DerefSerial(targetSerial);
	if(pUnit)
		pSpace = (MapSpace *)pUnit;
	else target = NOTARGET;

	switch(target)
	{
		case UNIT:
			pButton = ADerefAs(BAM_Button, gItemPortrait);
			if(pUnit->item)
			{
				if(pUnit->fAutoControl || fIsPaused || pUnit->player != playerSide || worldEnder
					|| pUnit->gGroupCursor)
					pButton->Listen(FALSE);
				else switch(itemMgr.GetItemUsage(pUnit->item))
				{
					case ITEM_AUTOMATIC:
						pButton->Listen(FALSE);
						break;

					case ITEM_MANUAL:
						pButton->Listen(TRUE);
						pButton->fIsToggle = FALSE;
						break;

					case ITEM_TARGET:
						pButton->Listen(TRUE);
						pButton->fIsToggle = TRUE;
						break;
				}
			}
			else
			{
				pButton->Listen(FALSE);
			}

			for(loop1 = 0; loop1 < 7; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);

				if(!pUnit->CheckActionAvailable((action)(loop1 + 1)))
				{
					// if unit is not part of group selection
					if(!pUnit->gGroupCursor)
					{
						pButton->Select(DG_DISABLED);
						continue;
					}
				}

				if(pUnit->fAutoControl || pUnit->player != playerSide || fIsPaused ||
					worldEnder)
				{
					pButton->Select(DG_DISABLED);
					continue;
				}
				
				switch((action)(loop1 + 1))
				{
					case MOVE_TO:
					case ATTACK:
						pButton->fIsToggle = TRUE;
						pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
						pButton->Listen(TRUE);
						break;

					case DROP:
						// if unit is not part of group selection
						if(pUnit->gGroupCursor)
						{
							pButton->Select(DG_DISABLED);
							continue;
						}

						pButton->SetCelText(1, squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_DROP));
						pButton->SetCelText(2, squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_DROP));
						// fall-through intentional

					case DISBAND:	// no text
						// if unit is not part of group selection
						if(pUnit->gGroupCursor)
						{
							pButton->Select(DG_DISABLED);
							continue;
						}

					case GUARD:		// no text
 						pButton->fIsToggle = FALSE;
						pButton->SetupReplies(REPLY_ACTIVATED);
						pButton->Listen(TRUE);
						break;

					case SKILL:
						// if unit is not part of group selection
						if(pUnit->gGroupCursor)
						{
							pButton->Select(DG_DISABLED);
							continue;
						}

						pButton->SetupReplies(REPLY_ACTIVATED);
						if(pUnit->type == ACOLYTE)
						{
							pCel = (CelHeader *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rAcolyteSkillButton));

							CopyCel(pCel, 0, 0, RES_ANIM, 142, 1, FALSE, clut[playerSide]);
							pCelData = AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rAcolyteSkillButton));
							blackLength = ((MANA_ACOLYTE_MAX / 2 - AMin(pUnit->currMana, MANA_ACOLYTE_MAX / 2))
								* 100) / (MANA_ACOLYTE_MAX / 2);
							blackLength = L2R((blackLength * (270 - 205)) + 99) / 100;
							blackPoint = L2R(270) - blackLength;
	  						if(blackLength)
							{
								pCelData2 = pCelData + (16 * L2R(pCel->width) + 5 + blackPoint);
								for(loop2 = 301; loop2 < 312; loop2++)
								{
									memset(pCelData2, CI_BLACK, L2R(blackLength));
									pCelData2 += L2R(pCel->width);
								}
							}
							sprintf(string1, "%d mana", AMin(pUnit->currMana, MANA_ACOLYTE_MAX / 2));
							pFontMgr->SetRes(9060);
							SetFontColors(CI_SKIP, 155, 165, 65, 62, 155, 165);
							ASetString(0, 19, string1, (uchar *)pCel, pCel->width, 0, DG_JUST_CENTER);

							pString = pWorld->squib1.Load(SQUIB_UNIT_SKILL, SKILL_TRANSFER);
							strcpy(string1, pString);
					
							pButton->SetButtonRes(FALSE, RES_CEL, rAcolyteSkillButton, 1);
							pButton->SetButtonRes(TRUE, RES_ANIM, 142, 2);
							pButton->SetButtonRes(DG_DISABLED, RES_ANIM, 142, 3);
							pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_TOP);
						}
						else
						{
//							if(pUnit->fDoppleganger)
//								pString = squib1.Load(SQUIB_UNIT_SKILL, SKILL_IMMITATE);
//							else
							pString = squib1.Load(SQUIB_UNIT_SKILL, (int)pUnit->type + 1);
							sprintf(string1, "%s\n%d mana", pString, pUnit->skillCost);
							pButton->SetButtonRes(FALSE, RES_ANIM, 108, 1);
							pButton->SetButtonRes(TRUE, RES_ANIM, 108, 2);
							pButton->SetButtonRes(DG_DISABLED, RES_ANIM, 108, 3);
							pButton->SetTextJustify(DG_JUST_LEFT, DG_JUST_CENTER);
						}
						pButton->fIsToggle = pUnit->fSkillNeedsTarget;
						pButton->SetCelText(1, string1);
						pButton->SetCelText(2, string1);
						pButton->Listen(TRUE);
						break;

					case TRANSFORM:
						// if unit is not part of group selection
						if(pUnit->gGroupCursor)
						{
							pButton->Select(DG_DISABLED);
							continue;
						}

						pString = squib1.Load(SQUIB_WORLD_MSGS, (int)MSG_TRANSFORM);
						strcpy(string1, pString);

						// what would become of current unit, if he transformed?
//						newType = pUnit->GetTransformationType(pUnit->nearbyTransformer);
//						pGroup = pWorld->unitLib.GetUnitGroup(newType);
//
//						if(pUnit->nearbyTransformer == FUNC_IMM_WELL)
//						{
//							pString = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)MSG_RANDOM);
//							sprintf(string1, "%s\n%d mana", pString, MANA_TRANSFORM);
//						}
//						else if(newType)		// if transformation from current unit type possible at this loc
//						{
//							sprintf(string1, "%s\n%d mana", pGroup->unitName, MANA_TRANSFORM);
//						}
//						else
//						{
//							pSpace = map.GetStructure(pUnit->tileX, pUnit->tileY);
//							if(pSpace && pSpace->fFoundationFilled && pSpace->IsFoundation())
//							{
////								sprintf(string1, "%s\n%d mana", &szFuncNames[pSpace->h.func - 10][0],
////									pSpace->size * MANA_BUILD_SITE);
//								sprintf(string1, "BUILDING\n%d mana", pSpace->size * MANA_BUILD_SITE);
//							}
//							else
//							{
//								pButton->Select(DG_DISABLED);
//								break;
//							}
//						}
						pButton->SetCelText(1, string1);
						pButton->SetCelText(2, string1);
						pButton->fIsToggle = FALSE;
						pButton->SetupReplies(REPLY_ACTIVATED);
						pButton->Listen(TRUE);
						break;

					default:		// in case something is missed
						pButton->Select(DG_DISABLED);
						continue;
				}

				// cause drawing into master cel
				pButton->Select(pButton->currState);
			}
			break;

		case BUILDING:
			// buildings cant have items - put item button to sleep
			pButton = ADerefAs(BAM_Button, gItemPortrait);
			{
				pButton->Listen(FALSE);
			}

			for(loop1 = 0; loop1 < 7; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
				// set button to unselected state - will cause drawing into master cel

				if(fIsPaused || (pSpace->h.owner != playerSide) || worldEnder)
					pButton->Select(DG_DISABLED);
				else
				{
					switch((action)(loop1 + 1))
					{
						case TRANSFORM:
							if(pSpace->h.func == FUNC_PORTAL && !lastUnitSerialDrawn)
							{
								if(unitLib.lUnits[pBam->playerSide].count >= MAX_UNITS_PER_SIDE)
								{
									break;
									// too many units already - be realistic!
								}

								// SUMMON button (avail only on empty portal tile)
								pGroup = unitLib.GetUnitGroup(ACOLYTE);

								sprintf(string1, "%s\n%d mana", pGroup->unitName, MANA_SUMMON);
								pButton->SetCelText(1, string1);
								pButton->SetCelText(2, string1);
								pButton->SetTextOffSet(2, 0);
								pButton->fIsToggle = FALSE;
								pButton->SetupReplies(REPLY_ACTIVATED);
								pButton->Listen(TRUE);
								pButton->Draw();
								continue;
							}
							break;
					}
					pButton->Select(DG_DISABLED);
				}
			}
			break;

		case NOTARGET:
		default:
			// even if an item is here, it cant be activated.  Put item button to sleep
			pButton = ADerefAs(BAM_Button, gItemPortrait);
			{
				pButton->Listen(FALSE);
				pButton->Select(FALSE);
			}

			for(loop1 = 0; loop1 < 7; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
				pButton->Select(DG_DISABLED);
			}
			break;
	}
}

// read *.ITM file for all items to place in world at startup
void
World::SetupItems(void)
{
	List		lSpots(30, TRUE);		// list of startLocs to place items in
	char		*configLine, buffer[80], itemName[20], *pTerrainName;
	MapSpace *pCluster;
	int32		itemCount, tempX, tempY;
	uint32	loop1, loop2, loop3;
	itemType newItem;
	bool		fRandomAppear;
	MapInfoRes	*pMapInfo;
	terrainType	newTerrain;
	tile		*pTile;

	// make list of remaining startLocs for items.  As each is used, it will be deleted from the list
	for(loop1 = 0; loop1 < map.h.totalSpaces; loop1++)
	{
		pCluster = ADerefAs(MapSpace, map.gSpaces[loop1]);
		// make sure cluster is still present at assigned X,Y and not just an old cluster
		// that was created and then discarded
		if(pWorld->map.MapSpaces[pCluster->h.xPos + pCluster->h.yPos * WORLD_WIDTH] != loop1)
		{
//			pMono->Out("World::SetupItems() - warning, cluster %d not assigned to X%d Y%d\n",
//				loop1, pCluster->h.xPos, pCluster->h.yPos);
			continue;
		}
		
		if(pCluster->h.fSpecialItemDrop)
		{		
			lSpots.Add((void *)pCluster);
//			pMono->Out("Item startLoc: cluster #%d at %d,%d\n", loop1, pCluster->h.xPos, pCluster->h.yPos);
		}
	  	else if(pCluster->h.presetItem && !pCluster->h.currHP && !pCluster->IsFoundation())
		{
			// item already lying on the ground
			itemMgr.PlaceItem(pCluster->h.xPos, pCluster->h.yPos, (itemType)pCluster->h.presetItem);
		}
	}
	pMono->Out("SetupItems(): %d startLocs\n", lSpots.count);

	// begin parsing *.ITM file for items to place
	pMapInfo = (MapInfoRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_MAP_INFO, mapResNum));

	configLine = &pMapInfo->configLines[0][0];
	for(loop1 = 0; loop1 < pMapInfo->totalConfigLines; loop1++)
	{
		if(!memcmp(configLine, "#ITEMS", 6))
			break;
		configLine += CONFIG_LINE_MAX_LEN;
	}

	configLine += CONFIG_LINE_MAX_LEN;
	loop1 = 1;
	while(*configLine == '#')
	{
		strcpy(buffer, configLine);
		strupr(buffer);
		if(!memcmp(buffer, "#PLACEITEM", 10))
		{
			GetFirstToken(buffer);
			strcpy(itemName, GetNextToken());
			newItem = itemMgr.GetItemType(itemName);
			itemCount = atoi(GetNextToken());
			pTerrainName = GetNextToken();

			if(atoi(pTerrainName) > 0)
			{
				// if 2 numeric found, assume X/Y coords instead
				tempX = itemCount;
				tempY = atoi(pTerrainName);
				pCluster = pWorld->map.GetStructure(tempX, tempY);
				if(pCluster && (pCluster->h.maxHP || pCluster->IsFoundation())
					&& !pCluster->h.presetItem)
				{
					// stash one inside the structure
					pMono->Out("STASH %s in BLDG %s @(%d,%d)\n", itemMgr.itemNames[newItem],
						szFuncNames[pCluster->h.func], tempX, tempY);
					pCluster->h.presetItem = newItem;
					pCluster->h.fPickRandomItem = FALSE;
				}
				else if(!itemMgr.CheckItem(tempX, tempY))
				{
					itemMgr.PlaceItem(tempX, tempY, newItem);
				}
				break;
			}

			newTerrain = map.GetTerrainType(pTerrainName);
			// place <itemCount> instances of item type <newItem> at random
			// locations of terrain type <newTerrain>
			for(loop2 = 0; loop2 < itemCount; loop2++)
			{
				tempX = ARandom(WORLD_WIDTH);
				tempY = ARandom(WORLD_HEIGHT);
				for(loop3 = 0; loop3 < WORLD_SIZE; loop3++)
				{
					// is terrain at this X,Y acceptable?
					pTile = pWorld->map.GetTile(tempX, tempY);
					if(pTile->terrain == newTerrain || newTerrain == TERRAINMAXTYPES)
					{
						// is a cluster present and acceptable?
						pCluster = pWorld->map.GetStructure(tempX, tempY);
						if(pCluster && (pCluster->h.maxHP || pCluster->IsFoundation())
							&& !pCluster->h.presetItem && pCluster->h.fPickRandomItem)
						{
							// stash one inside the structure
							pMono->Out("STASH %s in BLDG %s @(%d,%d)\n", itemMgr.itemNames[newItem],
								szFuncNames[pCluster->h.func], tempX, tempY);
							pCluster->h.presetItem = newItem;
							pCluster->h.fPickRandomItem = FALSE;
							break;
						}
//						else just forget the item
//						{
//							if(!itemMgr.CheckItem(tempX, tempY))
//							{
//								itemMgr.PlaceItem(tempX, tempY, newItem);
//								break;
//							}
//						}
					}
					if(++tempX >= WORLD_WIDTH)
					{
						tempX = 0;
						if(++tempY >= WORLD_HEIGHT)
							tempY = 0;
					}
				}
				if(loop3 == WORLD_SIZE)
					pMono->Out("CANNOT PLACE %s #%d ON TERRAIN %s\n", itemName, loop2 + 1, pTerrainName);
			}
		}
		else if(!isdigit(buffer[1]))
			break;
		else
		{
			itemCount = atoi(&buffer[1]);
			fRandomAppear = ((toupper(buffer[3])) == 'Y')? TRUE: FALSE;
			itemMgr.fItemCanAppear[loop1] = fRandomAppear;
//			strcpy(&itemMgr.itemNames[loop1][0], &buffer[5]);

			for(loop2 = 0; loop2 < itemCount; loop2++)
			{
				if(lSpots.count == 0)
				{
					pMono->Out("SetupItems(): error - out of startLocs, cannot place \"%s\"\n",
						&itemMgr.itemNames[newItem][0]);
					continue;
				}
				newItem = (itemType)loop1;
				pCluster = (MapSpace *)lSpots.At(ARandom(lSpots.count) + 1);
				itemMgr.PlaceItem(pCluster->h.xPos, pCluster->h.yPos, newItem);
				lSpots.Delete(pCluster);
			}
		}

		configLine += CONFIG_LINE_MAX_LEN;
		loop1++;
	}
	lSpots.Release();

//	itemMgr.PlaceItem(WORLD_WIDTH - 1, WORLD_HEIGHT - 1, STORM_BRACERS);
//	itemMgr.PlaceItem(WORLD_WIDTH - 2, WORLD_HEIGHT - 1, MAP);
//	itemMgr.PlaceItem(WORLD_WIDTH - 3, WORLD_HEIGHT - 1, MIGHT_MANTLE);
//	itemMgr.PlaceItem(WORLD_WIDTH - 4, WORLD_HEIGHT - 1, HEALING_SALVE);
//	itemMgr.PlaceItem(WORLD_WIDTH - 5, WORLD_HEIGHT - 1, BERSERKER_BREW);
//	itemMgr.PlaceItem(WORLD_WIDTH - 6, WORLD_HEIGHT - 1, SACRED_URN);
//	itemMgr.PlaceItem(WORLD_WIDTH - 7, WORLD_HEIGHT - 1, DAEMONS_BANE);
//	itemMgr.PlaceItem(WORLD_WIDTH - 8, WORLD_HEIGHT - 1, FROST_CAPE);
//	itemMgr.PlaceItem(WORLD_WIDTH - 9, WORLD_HEIGHT - 1, BOG_BOOTS);
//	itemMgr.PlaceItem(WORLD_WIDTH - 10, WORLD_HEIGHT - 1, SHRUB_SPRITE);
//	itemMgr.PlaceItem(WORLD_WIDTH - 11, WORLD_HEIGHT - 1, VERDANT_SHIELD);
//	itemMgr.PlaceItem(WORLD_WIDTH - 12, WORLD_HEIGHT - 1, VIRTUE_VEIL);
//	itemMgr.PlaceItem(WORLD_WIDTH - 13, WORLD_HEIGHT - 1, TRAILFINDER);
//	itemMgr.PlaceItem(WORLD_WIDTH - 14, WORLD_HEIGHT - 1, MANA_ORB);
//	itemMgr.PlaceItem(WORLD_WIDTH - 15, WORLD_HEIGHT - 1, DRIFT_DISC);
//	itemMgr.PlaceItem(WORLD_WIDTH - 16, WORLD_HEIGHT - 1, TELEPORT_TOME);
//	itemMgr.PlaceItem(WORLD_WIDTH - 17, WORLD_HEIGHT - 1, WEIRD_WAND);
//	itemMgr.PlaceItem(WORLD_WIDTH - 18, WORLD_HEIGHT - 1, KEY);
//	itemMgr.PlaceItem(WORLD_WIDTH - 19, WORLD_HEIGHT - 1, DOWRY_CHEST);
//	itemMgr.PlaceItem(WORLD_WIDTH - 20, WORLD_HEIGHT - 1, MASON_MIX);
}

void
World::EndScenario(int winningSide)
{
	int		loop1, loop2;
	MapSpace	*pSpace;
	VictoryDefeat *pPop;
	grip		gSpace;

	AllowMouseDragging(FALSE);

	if(winningSide == SIDE3)
	{
		// special case - just exit back to main menu
		bGlobal.roomMgr.NewRoom(BR_MENU);
		return;
	}
	else if(winningSide == SIDE0)
	{
		// special case - attract mode

		if(pBam->fNoIntro)
		{
			// back to main menu
			bGlobal.roomMgr.NewRoom(BR_MENU);
		}
		else
		{
			bGlobal.cinematic = 15;	// INTERPLAY
//			bGlobal.cinematic = 20;	// TACHYON
//			bGlobal.cinematic = 30;	// cartoon
			bGlobal.roomMgr.NewRoom(BR_CINE);
		}
		return;		
	}

	memcpy(pBam->unitsCreated, unitsCreated, sizeof(pBam->unitsCreated));
	memcpy(pBam->unitsLost, unitsLost, sizeof(pBam->unitsLost));	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		for(loop2 = 0; loop2 < TOTAL_SIDES; loop2++)
		{
			pBam->enemiesSlain[loop1] += unitsSlain[loop2][loop1];
			pBam->structuresDestroyed[loop1] += structuresDestroyed[loop2][loop1];
		}
	}

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		gSpace = (grip)map.lBuildings[loop1].FirstValue();
		while(gSpace)
		{
			pSpace = ADerefAs(MapSpace, gSpace);
			switch((funcType)pSpace->h.func)
			{
				case FUNC_ARBORLODGE:
				case FUNC_BARRACKS:
				case FUNC_CRYPT:
				case FUNC_RUNESTONE:
				case FUNC_TEMPLE:
				case FUNC_HOME:
				case FUNC_KEEP:
					// only these bldgs matter
					sitesControlled[loop1]++;
					break;
			}
			gSpace = (grip)map.lBuildings[loop1].NextValue();
		}
	}
	memcpy(pBam->sitesControlled, sitesControlled, sizeof(pBam->sitesControlled));

	pBam->totalFoundations = 0;
	gSpace = (grip)map.lBuildings[SIDE0].FirstValue();
	while(gSpace)
	{
		pSpace = ADerefAs(MapSpace, gSpace);
		if(pSpace->IsFoundation())
			pBam->totalFoundations++;
		gSpace = (grip)map.lBuildings[SIDE0].NextValue();
	}

	if(winningSide == playerSide)
		LaunchSound(SND_TRUMPET, vPort.ViewX + VPORT_WIDTH / 2, vPort.ViewY + VPORT_HEIGHT / 2);
	else
		LaunchSound(SND_DRUMS, vPort.ViewX + VPORT_WIDTH / 2, vPort.ViewY + VPORT_HEIGHT / 2);

	if(bGlobal.storyLine == TUTORIAL)
	{
		// call generic popup with final goal satisfied
		SetTutorialGoal(TUT_BLDG_DESTROYED, TRUE);	
	}
	else
	{
		TRACK_MEM("WINLOSE2");	pPop = new VictoryDefeat;
		pPop->Setup(gPal, (winningSide == playerSide)? TRUE: FALSE);
	}
}

void
World::Cleanup(void)
{
	TPalette	*pPal;
	int		loop1, int1;
	grip		gAni;

	AllowMouseDragging(FALSE);

	// notify any other players that we are leaving (in case they dont already know)
	int1 = (pBam->playerSide == SIDE1)? SIDE2: SIDE1;
	if(pBam->playerTypes[int1] >= 0 && !bGlobal.netDisconnect)		// if real opponent
	{
		SPacket1.header.ID = PLAYER_LEAVING;
		SPacket1.header.destID = pCommMgr->GetUserID()? 0: 1;	// BUGBUG - this is a Russell trick

		// include some dummy info 'cuz NETNOW hates 0-length packet data
		SPacket1.header.len = sizeof(SPacket1.header);
		SPacket1.pData = &SPacket1.header;
		pCommMgr->SendData(&SPacket1);
	}

	unitLib.Cleanup();
	ai.Cleanup();

	gAni = (grip)lBackgroundAnis.FirstValue();
	while(gAni)
	{
		ADelete(gAni);
		gAni = (grip)lBackgroundAnis.NextValue();
	}
	lBackgroundAnis.Release();

	if(gPal)
	{
		// kill the palette
		pPal = ADerefAs(TPalette, gPal);
		pPal->FadeToBlack();
		ADelete(gPal);
		gPal = NULL;
	}

	ADelete(gPortrait);

	ADelete(gSpendXPButton);

	if(gWorldMap)
		ADelete(gWorldMap);

	if(gInterfaceScreen)
		ADelete(gInterfaceScreen);

	if(gItemPortrait)
		ADelete(gItemPortrait);

	if(gGoSeeButton)
		ADelete(gGoSeeButton);

	if(gNoteButton)
		ADelete(gNoteButton);

//	if(gNoteSend)
//		ADelete(gNoteSend);
//	if(gNoteExit)
//		ADelete(gNoteExit);

	if(gNoteBox)
		ADelete(gNoteBox);

	if(gNoteBoxText)
		AFree(gNoteBoxText);

	AFlush(ALoadDebug(__FILE__, __LINE__, RES_CEL, rAcolyteSkillButton));
	AFlush(ALoadDebug(__FILE__, __LINE__, RES_CEL, rAcolyteTransformGuage));

	for(loop1 = 0; loop1 < MAINBTN_TOTAL; loop1++)
		ADelete(gMainButton[loop1]);

	for(loop1 = 0; loop1 < 7; loop1++)
		ADelete(gVerbButton[loop1]);

	for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
		ADelete(gTransformButton[loop1]);

	for(loop1 = 0; loop1 < MSGBOX_MAX; loop1++)
		if(MsgBox[loop1])
			ADelete(MsgBox[loop1]);

	if(gBox)			ADelete(gBox);
	if(gBoxText)	AFree(gBoxText);

	itemMgr.Cleanup();
	vPort.Cleanup();

	// shut down any remaining sounds & music
	for(loop1 = 0; loop1 < MAX_DIGI_SOUNDS; loop1++)
		pBam->sounds[loop1].Stop();
	pBam->voice1.Stop();
	music1.Stop();

	AFlush(ALoad(RES_TILELIB, tileResNum));

	// release whatever font we were using
//	pFontMgr->Unregister();

//	squib1.Cleanup();
	pResMgr->PurgeAllUnlocked();
	ReportFreeMem();

	// dump list of remaining serial nums (there shouldn't be any)
	#ifndef NDEBUG
	for(loop1 = 0; loop1 < MAX_SERIAL_NUMS; loop1++)
	{
		if(serialNums[loop1][0] == NOTARGET)
			continue;
		BamDebug.Out("ser%d] t%d g%d\n", loop1, serialNums[loop1][0],
			serialNums[loop1][1]);
	}
	#endif
}

void ShowUnitPath(Unit *pUnit)
{
	int		loop1, tempX, tempY;
	grip		gAni;
	BAM_Ani	*pAni;

	if(bGlobal.storyLine == NETGAME)
	{
		return;
		// dont want to de-sync the ani list
	}

	for(loop1 = 0; loop1 < MAX_PATHER_MARKERS; loop1++)
	{
		if(pUnit)
		{
			tempX = pUnit->pathArray[loop1 * 2];
			tempY = pUnit->pathArray[loop1 * 2 + 1];
			if(tempX == -1 || loop1 >= MAX_PATH_SIZE || pUnit->currAction != MOVE_TO)
				pUnit = NULL;
		}
									
		gAni = gPathAni[loop1];
		if(gAni)
			pAni = ADerefAs(BAM_Ani, gAni);

		if(!pUnit)
		{
			if(gAni)
			{
				pAni->SetTilePos(-1, -1);
				pWorld->vPort.CutAni(gAni);
				continue;
			}
		}
								
		if(!gAni)
		{
			TRACK_MEM("CosmeticAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_MOVE,
				PRI_FLOOR, TRUE, FALSE, 0, 0, 0, TRUE);
			pWorld->lBackgroundAnis.Add((void *)gAni);
			gPathAni[loop1] = gAni;
			pAni = ADerefAs(BAM_Ani, gAni);
		}
								
		if(pUnit)
		{
			pWorld->vPort.AddAni(gAni);
			pAni->SetTilePos(tempX, tempY);
		}
		else
		{
			pAni->SetTilePos(-1, -1);
			pWorld->vPort.CutAni(gAni);
		}
	}
}

bool
World::HandleMsg(Message* pMsg)
{
	uint32		loop1, mouseX, mouseY;
	int			tempX, tempY;
	Object		*pObject;
	BAM_Button	*pButton;
	BAM_Box		*pBox;
	WorldMap		*pWorldMap;
	Unit			*pUnit;
	grip			gUnit;
	List			*pList;
	MapSpace		*pSpace;
	char			string1[40];
	itemType		item1;
	SaveMenu		*pSave;
	#ifdef INTERACTIVE_DEMO
	BAM_ErrorPopup *pPop;
	#endif

	//	_MARK_("HandleMsg() start");

	// first - handle interception/blocking of msgs (pause mode, etc)
/*	if(fIsPaused)		// in-pause handler
	{
		switch(pMsg->type)
		{
			case MSG_NOTICE:
				if(pMsg->notice.type == N_CONTROL_REPLY && (int)pMsg->notice.param == REPLY_ACTIVATED
					&& pMsg->notice.gSource == gMainButton[MAINBTN_FREEZE])
				{
	  				// toggle PAUSE off
					Pause(FALSE);
					return(TRUE);
				}
				break;

			case MSG_EVENT:
				break;
		}
		
		// only pass msg to the Pause button itself - no one else matters
		pButton = ADerefAs(BAM_Button, gMainButton[MAINBTN_FREEZE]);
		return(pButton->HandleMsg(pMsg));
	}*/

	switch(pMsg->type)
	{
		case MSG_NOTICE:
			switch(pMsg->notice.type)
			{
				case N_CUE:
					for(loop1 = 0; loop1 < MAX_DIGI_SOUNDS; loop1++)
					{
						if(pMsg->notice.param == (void *)&pBam->sounds[loop1])
						{
							break;
						}
					}
					break;

				case N_CONTROL_REPLY:	// a reply from one of our buttons/boxes/etc.  Determine exact meaning of msg.
					switch((uint16)pMsg->notice.param)
					{
						case REPLY_DESELECTED:
							if(pMsg->notice.gSource == gNoteBox)
							{
								char	*pText = ADerefAs(char, gNoteBoxText);
								if(pText[0])
									NoteSend();
								else NoteExit();
								return(TRUE);
							}

							if(pMsg->notice.gSource == gSpendXPButton)
							{
								SpendExperience	*pExp;
								TRACK_MEM("SpendExperience");		pExp = new SpendExperience;
								pExp->Setup(gPal);
								return(TRUE);
							}

//							if(pMsg->notice.gSource == gBox)	// if reply from text box
//							{
//								pMono->Out("Box contains \"%s\"\n", ADerefAs(char, gBoxText));
//								pBox = ADerefAs(BAM_Box, gBox);
//								pBox->Listen(FALSE);
//								MsgBoxDraw();	// re-write msg box where it should be
//								return(TRUE);
//							}
							break;

						case REPLY_DEACTIVATED:	// one of our verb buttons has been de-activated (clicked twice).  Determine who.
							// is it a verb button?
							for(loop1 = 0; loop1 < 6; loop1++)
								if(gVerbButton[loop1] && gVerbButton[loop1] == pMsg->notice.gSource)
								{
									vPort.SetCurrentVerb(NO_ACTION);
									return(TRUE);
								}
							if(gItemPortrait == pMsg->notice.gSource)
								vPort.SetCurrentVerb(NO_ACTION);
							break;
							
						case REPLY_ACTIVATED:	// one of our controls has been activated (selected w/ confirmation).  Determine who.
							// is it a verb button?
							if(gVerbButton[DROP - 1] == pMsg->notice.gSource)	// DROP button
							{
//								vPort.fAutoCommandMode = FALSE;
								vPort.SetCurrentVerb(DROP);
								return(TRUE);
							}

							for(loop1 = 0; loop1 < 6; loop1++)
								if(gVerbButton[loop1] && gVerbButton[loop1] == pMsg->notice.gSource)
								{
									char	string2[40];

//									vPort.fAutoCommandMode = FALSE;

									// first, check if button was turned on, or turned off
									pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
									if(pButton->fIsToggle && pButton->currState == FALSE)
										vPort.SetCurrentVerb(NO_ACTION);
									else
									{
										// cause viewport to toggle verb mode
										vPort.SetCurrentVerb((action)(loop1 + 1));
									}

									strcpy(string2, &pVerbNames[loop1 + 1][0]);
									if(strchr(string2, '\n'))
										*strchr(string2, '\n') = ' ';

									// button has been activated.  If any buttons other than current are active,
									// de-activate them.
									for(loop1 = 0; loop1 < 6; loop1++)	// make sure all verb buttons are de-selected
									{	
										// skip current button, it's supposed to be lit.
										if(gVerbButton[loop1] != pMsg->notice.gSource)
										{
											pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
											if(pButton->currState == TRUE)
												pButton->Select(FALSE);
			
										}
									}
									return(TRUE);
								}
		
							if(gPortrait == pMsg->notice.gSource)	// Portrait/GoTo button
							{
								switch(statBoxCurrMode)
								{
									case TERRAIN:
										vPort.GoToTarget();
										break;
									case BUILDING:
									case UNIT:
										vPort.SetCurrentVerb(NO_ACTION);
										SetStatusBoxMode(TERRAIN);
										break;
								}
								return(TRUE);
							}
		 					if(gItemPortrait == pMsg->notice.gSource)	// Use Item
							{
//								vPort.fAutoCommandMode = FALSE;
								pButton = ADerefAs(BAM_Button, gItemPortrait);
								if(pButton->currState == TRUE)
									vPort.SetCurrentVerb(USE_ITEM);
							}

							switch(statBoxCurrMode)
							{
								case TRANSFORMING:	// unit-to-unit already in progress
									// if abort button hit
									if(pMsg->notice.gSource == gTransformButton[2])
									{
										pUnit = (Unit *)DerefSerial(vPort.targSerial);
										if(pUnit && pUnit->primaryAction == TRANSFORM)	//safety
										{
											pWorld->AddLocalAction(pUnit->serialNum, ABORT, pUnit->tileX, pUnit->tileY);
//											pUnit->fActionEnding = TRUE;
//											pUnit->tNextStateCheck = ATicks();
//											SetStatusBoxMode(TERRAIN);
										}
										return(TRUE);
									}
									break;

								case BUILDING:
									if(vPort.currentVerb != TRANSFORM)
										break;
									for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
									{
										if(pMsg->notice.gSource == gTransformButton[loop1])
										{
											vPort.ProcessVerb(0, 0, (int32)FUNC_ARBORLODGE + loop1);
											SetStatusBoxMode(TERRAIN);
											return(TRUE);
										}
									}
									break;

								case UNIT:
									// transformation to something selected
									if(vPort.currentVerb != TRANSFORM)
										break;

									for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
									{
										if(pMsg->notice.gSource == gTransformButton[loop1])
										{
											pUnit = (Unit *)DerefSerial(vPort.targSerial);
											if(pUnit)
											{
												if(loop1 == 0 && pUnit->type == ACOLYTE)
													vPort.ProcessVerb(0, 0, UNIT_TERRAIN);
												else
												{
													pUnit = (Unit *)DerefSerial(vPort.targSerial);
													pUnit->FindNearbyTransformer(&pSpace);
													if(!pSpace)
														continue;
													if(pUnit->type == ACOLYTE)
													{
														// offset because his 1st option is not even a unit transformation
														loop1--;
													}
													vPort.ProcessVerb(0, 0, (int32)pSpace->GetRelatedUnitType(loop1));
												}
											}
											SetStatusBoxMode(TERRAIN);
											return(TRUE);
										}
									}
									break;
							}

							// is it a main game button?
//							if(pMsg->notice.gSource == gNoteExit)
//							{
//								NoteExit();
//								return(TRUE);
//							}
//							if(pMsg->notice.gSource == gNoteSend)
//							{
//								NoteSend();
//								return(TRUE);
//							}
							if(pMsg->notice.gSource == gNoteButton)		// send note to other player
							{
								Note();
								return(TRUE);
							}
							if(pMsg->notice.gSource == gGoSeeButton)			// Go To origin of msg
							{
								GoSee();
								return(TRUE);
							}
							else if(pMsg->notice.gSource == gMainButton[MAINBTN_FREEZE])
							{
								#ifndef NDEBUG
//								DebugOut("World::HandleMsg() - click on FREEZE, state change\n");
								#endif
								Pause(fIsPaused? FALSE: TRUE);
								return(TRUE);
							}
							else if(pMsg->notice.gSource == gMainButton[MAINBTN_ENCYCLO])
							{
								AAnimate();
								EncyMenu *pEncyMenu;
								TRACK_MEM("Encyclo"); pEncyMenu = new EncyMenu;
								pEncyMenu->Setup(gPal);
								pButton = ADerefAs(BAM_Button, pMsg->notice.gSource);
								pButton->Select(FALSE);
								return(TRUE);
							}
							else if(pMsg->notice.gSource == gMainButton[MAINBTN_WINLOSE])
							{
								AAnimate();
								WinLose	*pWinLose;
								Tutorial	*pTutor;
								if(bGlobal.storyLine == TUTORIAL)
								{
									TRACK_MEM("Tutorial"); pTutor = new Tutorial;
									pTutor->Setup(gPal,FALSE,FALSE);
								}
								else
								{
									TRACK_MEM("WinLose"); pWinLose = new WinLose;
									pWinLose->Setup(gPal);
								}
								pButton = ADerefAs(BAM_Button, pMsg->notice.gSource);
								pButton->Select(FALSE);
								return(TRUE);
							}
							else if(pMsg->notice.gSource == gMainButton[MAINBTN_SETTINGS])
							{
								AAnimate();
								Option(gPal);
								pButton = ADerefAs(BAM_Button, pMsg->notice.gSource);
								pButton->Select(FALSE);
								return(TRUE);
							}
							else if(pMsg->notice.gSource == gMainButton[MAINBTN_SEND])
							{
								// start inter-player message editor
								Note();
							}
							break;	// end of case REPLY_ACTIVATED:

						default:	// some other N_CONTROL_REPLY msg
							break;
					}
					break;	// end of case N_CONTROL_REPLY:

				default:	// some other NOTICE type
					break;
			}
			break;		// end of case NOTICE:

		case MSG_EVENT:
			//MASTER EVENT CHECK -if in question mode, then intercept now
			if(vPort.currentVerb == QUESTION)
			{
				if(pMsg->event.type == E_MOUSE_DOWN)
				{
					if(pMsg->event.value == RIGHT_BTN)
					{
						vPort.SetCurrentVerb(NO_ACTION);
					}
					else
					{
						mouseX = pMsg->event.x;
						mouseY = pMsg->event.y;
						vPort.ProcessVerb(mouseX,mouseY);
					}
				}
				if(pMsg->event.type != E_MOUSE_UP)
					return (TRUE);
				// allow MOUSE_UP to pass through, for correct visual response of ? button
			}

			switch (pMsg->event.type)
			{
				case E_KEY_DOWN:
					if(bGlobal.storyLine == SHOW_OFF)
					{
						switch(pMsg->event.value)
						{
							case K_SLASH:
								break;

							default:
							 	// any user input terminates this mode
								tWorldEnds = ATicks();
								worldEnder = SIDE3;
								return(TRUE);
						}
					}

					if(tWorldEnds && bGlobal.storyLine != SHOW_OFF)
					{
						// world is ending - ignore further input
						return(TRUE);
					}

					#ifdef ENABLE_EDITOR
					if(pBam->fMapEdit)		// if running in editor mode
						if(gBox)					// failsafe
						{
							pBox = ADerefAs(BAM_Box, gBox);	// pass msg to box first.
							if(pBox->HandleMsg(pMsg))			// if claimed,
								return(TRUE);							// done.
						}
					#endif

					// if message editor currently running
					if(!MsgBoxOn)
					{
						// let editor box have a crack at it.
						// If it really is active, it should gobble this.
						pBox = ADerefAs(BAM_Box, gNoteBox);
						if(pBox->HandleMsg(pMsg))
							return(TRUE);
						// else why is MsgBoxOn==TRUE?
					}
					pWorldMap = ADerefAs(WorldMap, gWorldMap);

					// cheat code?
					if(pMsg->event.modifiers & MOD_ALT)
					{
						char	*pStr, char1;
						int codeLen, compOff;

						memmove(pCheatCodeBuffer, pCheatCodeBuffer + 1, CHEAT_MAX_LEN - 2);
						pCheatCodeBuffer[CHEAT_MAX_LEN - 1] = NULL;	// terminator
						char1 = (char)AScanToASCII(pMsg->event);
						char1 = (char)toupper(char1);
						if(char1 == '/')
							char1 = '?';
						pCheatCodeBuffer[CHEAT_MAX_LEN - 2] = char1;

						for(loop1 = 0; loop1 < CHEAT_MAX; loop1++)
						{
							codeLen = strlen(pCheatCodes[loop1]);
							compOff = (CHEAT_MAX_LEN - 1) - codeLen;
							pStr = pCheatCodeBuffer + compOff;
							if(!strcmp(pStr, pCheatCodes[loop1]))
							{
								switch((cheatCodes)loop1)
								{
									case CHEAT_DEFOG:
										if(bGlobal.storyLine == NETGAME)
											break;

										memset(vPort.fog, NULL, sizeof(vPort.fog));
										vPort.lastViewX = 999;
										vPort.Draw();
										pWorldMap->Draw();
										break;

									case CHEAT_MANA:
										if(bGlobal.storyLine == NETGAME)
											break;

										pWorld->AdjustManaLevel(SIDE1, MANA_MAX);
										pWorld->AdjustManaLevel(SIDE2, MANA_MAX);
										break;

									case CHEAT_RESEARCH:
										if(bGlobal.storyLine == NETGAME)
											break;

										#ifdef INTERACTIVE_DEMO
											bGlobal.unitsResearched[GARGOYLE] = TRUE;
											bGlobal.unitsResearched[RANGER] = TRUE;
											bGlobal.unitsResearched[FURY] = TRUE;
											bGlobal.unitsResearched[GNOME] = TRUE;
										#else
										for(loop1 = 0; loop1 < UNITMAXTYPES; loop1++)
											bGlobal.unitsResearched[loop1] = TRUE;
										#endif
										break;

									case CHEAT_ACOLYTE:
										CheatSummon(ACOLYTE);
										break;

									case CHEAT_CLERIC:
										CheatSummon(CLERIC);
										break;

									case CHEAT_DRUID:
										CheatSummon(DRUID);
										break;

									case CHEAT_ENCHANTER:
										CheatSummon(ENCHANTER);
										break;

									case CHEAT_FURY:
										CheatSummon(FURY);
										break;

									case CHEAT_GARGOYLE:
										CheatSummon(GARGOYLE);
										break;

									case CHEAT_GHOUL:
										CheatSummon(GHOUL);
										break;

									case CHEAT_GNOME:
										CheatSummon(GNOME);
										break;

									case CHEAT_GOBLIN:
										CheatSummon(GOBLIN);
										break;

									case CHEAT_GOLEM:
										CheatSummon(GOLEM);
										break;

									case CHEAT_GRIFFIN:
										CheatSummon(GRIFFIN);
										break;

									case CHEAT_GUARDIAN:
										CheatSummon(GUARDIAN);
										break;

									case CHEAT_HARPY:
										CheatSummon(HARPY);
										break;

									case CHEAT_JUGGERNAUT:
										// find an open spot
										for(loop1 = 0; loop1 < 100; loop1++)
										{
											tempX = ARandom2(WORLD_WIDTH - 1) + 1;
											tempY = ARandom2(WORLD_HEIGHT - 1) + 1;
											if(!unitLib.GetUnitGrip(tempX, tempY) && !map.GetStructure(tempX, tempY) &&
												!unitLib.GetUnitGrip(tempX - 1, tempY) && !map.GetStructure(tempX - 1, tempY) &&
												!unitLib.GetUnitGrip(tempX, tempY - 1) && !map.GetStructure(tempX, tempY - 1) &&
												!unitLib.GetUnitGrip(tempX - 1, tempY - 1) && !map.GetStructure(tempX - 1, tempY - 1))
											{
//												AddLocalAction(NULL, SUMMON, vPort.targX,
//													vPort.targY, (int)JUGGERNAUT);
												AddLocalAction(NULL, SUMMON, tempX, tempY,
													(int)JUGGERNAUT);
												loop1 = 100;
												break;
											}
										}
										break;

									case CHEAT_NYMPH:
										CheatSummon(NYMPH);
										break;

									case CHEAT_PALADIN:
										CheatSummon(PALADIN);
										break;

									case CHEAT_RANGER:
										CheatSummon(RANGER);
										break;

									case CHEAT_WARRIOR:
										CheatSummon(WARRIOR);
										break;

									case CHEAT_WIZARD:
										CheatSummon(WIZARD);
										break;

									case CHEAT_WRAITH:
										CheatSummon(WRAITH);
										break;

									case CHEAT_WYRM:
										CheatSummon(WYRM);
										break;

									case CHEAT_ZOMBIE:
										CheatSummon(ZOMBIE);
										break;

									case CHEAT_PYRO:
										if(vPort.targType != UNIT)
											vPort.SetVerbBuzzer();

										pUnit = (Unit *)DerefSerial(vPort.targSerial);
										if(!pUnit || pUnit->fAutoControl ||
											pUnit->player != pBam->playerSide || pUnit->type != ACOLYTE ||
											pUnit->currTerrain != PORTAL)
										{
											vPort.SetVerbBuzzer();
											break;
										}

										if(GetManaLevel(pBam->playerSide) < 100)
										{
											vPort.SetVerbBuzzer(MSG_NO_MANA);
											break;
										}
										AdjustManaLevel(pBam->playerSide, -100);
										pUnit->fKamikaze = TRUE;

										TRACK_MEM("HealAni");	gUnit = pWorld->vPort.NewAni(RES_ANIM,
											ANIM_HEAL_REPAIR, 1, PRI_SKY, TRUE, FALSE, 3, pUnit->tileX,
											pUnit->tileY, FALSE, pUnit->tileXOff, pUnit->tileYOff);
										pWorld->vPort.CheckAni(gUnit);
										break;

									case CHEAT_CYBORG:
										#ifndef NDEBUG
										// toggle AI on our side
										loop1 = (pBam->playerTypes[pBam->playerSide] == PLAYER_LOCAL)?
											PLAYER_COMPUTER: PLAYER_LOCAL;
										pBam->playerTypes[pBam->playerSide] = loop1;
										pMono->Out("Player AI %s\n", (loop1 == PLAYER_COMPUTER)? "ON": "OFF");
										if(loop1 == PLAYER_COMPUTER)
										{
											ai.Setup(pBam->playerSide);
										}
										#endif
										break;

									case CHEAT_BERSERK:
										if(vPort.targType != UNIT)
											vPort.SetVerbBuzzer();

										pUnit = (Unit *)DerefSerial(vPort.targSerial);
										if(!pUnit || pUnit->fAutoControl ||
											pUnit->player != pBam->playerSide ||
											pUnit->item != BERSERKER_BREW)
										{
											vPort.SetVerbBuzzer();
											break;
										}

										if(GetManaLevel(pBam->playerSide) < 80)
										{
											vPort.SetVerbBuzzer(MSG_NO_MANA);
											break;
										}
										AdjustManaLevel(pBam->playerSide, -80);

										pUnit->UseItem();

										map.GetSurroundingCoords(vPort.CursorX, vPort.CursorY,
											1, tempX, tempY, TRUE);
										do
										{
											pUnit = unitLib.GetUnit(tempX, tempY);
											if(!pUnit || pUnit->fAutoControl ||
												pUnit->ImmuneToItem(BERSERKER_BREW))
											{
												continue;
											}
											pUnit->ApplyItemEffects(BERSERKER_BREW);
										}
										while(pWorld->map.GetSurroundingCoords(vPort.CursorX,
											vPort.CursorY, 1, tempX, tempY));
										break;
								}
							}
							else if(!memcmp(pStr, pCheatCodes[loop1], 4))
							{
								#ifndef NDEBUG
								strncpy(string1, pStr + 4, 2);
								string1[5] = NULL;
								item1 = (itemType)atoi(string1);
								if(item1)
								{
									itemMgr.PlaceItem(vPort.CursorX, vPort.CursorY, item1);
								}
								#endif
							}
						}
						// allow key to pass thru, to allow ALT_X and related keys to still function
						break;
					}

					switch(pMsg->event.value)
					{
#ifndef NDEBUG
//						case K_U:	// unit report diagnostic key
//							gUnit = (grip)unitLib.lUnits[playerSide].FirstValue();
//							pMono->Out("Report: Aticks()=%ld\n", (long)ATicks());
//							while(gUnit)
//							{
//								pUnit = ADerefAs(Unit, gUnit);
//								pMono->Out("Unit(%d): type=%s side=%d cycNxt=%ld pAct=%d cAct=%d fInAct=%d fActDone=%d\n",
//									gUnit, pUnit->unitName, pUnit->player, (long)pUnit->tNextStateCheck,
//									pUnit->primaryAction, pUnit->currAction, pUnit->fInAction, pUnit->fActionDone);
//								gUnit = (grip)unitLib.lUnits[playerSide].NextValue();
//							}
//							return(TRUE);
	
						case K_SPACE:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(playerSide, MANA_SUMMON);
								AddLocalAction(NULL, SUMMON, vPort.CursorX, vPort.CursorY, (int)ACOLYTE);
							}
							return(TRUE);

						case K_F:
							TRACK_MEM("FireAni");	pWorld->vPort.NewTerrainUnit(ACTIVE_FIRE, vPort.CursorX, vPort.CursorY);
							break;
							
						case K_1:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(playerSide, MANA_SUMMON);
								AddLocalAction(NULL, SUMMON, vPort.CursorX, vPort.CursorY, GUARDIAN);
							}
							return(TRUE);					

						case K_2:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(playerSide, MANA_SUMMON);
								AddLocalAction(NULL, SUMMON, vPort.CursorX, vPort.CursorY, HARPY);
							}
							return(TRUE);					

						case K_3:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(playerSide, MANA_SUMMON);
								AddLocalAction(NULL, SUMMON, vPort.CursorX, vPort.CursorY, RANGER);
							}
							return(TRUE);

						case K_4:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(SIDE2, MANA_SUMMON);
								Summon(ACOLYTE, vPort.CursorX, vPort.CursorY, SIDE2);
							}
							return(TRUE);					

						case K_5:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(SIDE2, MANA_SUMMON);
								Summon(ENCHANTER, vPort.CursorX, vPort.CursorY, SIDE2);
							}
							return(TRUE);					
						
						case K_6:
							AdjustManaLevel(SIDE0, MANA_SUMMON);
							Summon(GORGON, vPort.CursorX, vPort.CursorY, SIDE3);
							return(TRUE);					

						case K_7:
							if(!unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY))
							{
								AdjustManaLevel(SIDE2, MANA_SUMMON);
								Summon(NYMPH, vPort.CursorX, vPort.CursorY, SIDE2);
							}
							return(TRUE);					
							
						case K_8:	// give mana side 1
							AdjustManaLevel(SIDE1, 40);
							return(TRUE);
								
						case K_9:	// give mana side 2
							AdjustManaLevel(SIDE2, 40);
							return(TRUE);

						case K_0:	// Hand of Death!
							pUnit = unitLib.GetUnit(vPort.CursorX, vPort.CursorY);
							if(pUnit)
								pUnit->Die();
							else
							{
								pSpace = map.GetStructure(vPort.CursorX, vPort.CursorY);
								if(pSpace && pSpace->h.currHP)
									pSpace->Die(NULL);
							}
							break;
	
						case K_H:	// Hand of Life!
							pUnit = unitLib.GetUnit(vPort.CursorX, vPort.CursorY);
							if(pUnit)
								pUnit->HealThyself(NULL, NOUNIT, 100);
							else
							{
								pSpace = map.GetStructure(vPort.CursorX, vPort.CursorY);
								if(pSpace && pSpace->h.currHP)
									pSpace->HealThyself(NULL, NOUNIT, 100);
							}
							break;
#endif

						case K_F2:
							if(bGlobal.storyLine == SHOW_OFF)
								break;

							#ifdef INTERACTIVE_DEMO
							TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
							pPop->SetWindowOffsets(89,92);
							pPop->Setup(NULL,MM_MAIN_SQB,201);
							return(TRUE);
							#endif

							#ifdef OS_MAC
								// on Mac, just set saveNum, so make loop 
								// will call saveMgr.Save() which will do the rest
								//pMono->Out("Not implemented on the Mac, yet");
								saveNum = 1;
							#else
								pMono->Out("\nSave Button was hit");
								if(bGlobal.storyLine == NETGAME)
								{
									//only one net save "slot" 
									netSaveNum = (uint16) NET_SAVEGAME_NUM;
									netSerialNum = ARandom2(99999);
									sprintf(saveMessage,"%d",netSerialNum);
								}
								else
								{
									TRACK_MEM("SaveMenu");	pSave = new SaveMenu;
									pSave->Setup(NULL, SAVE_BUTTON);
								}
							#endif
							return(TRUE);

						case K_F3:
							if(bGlobal.storyLine == SHOW_OFF)
								break;

							#ifdef INTERACTIVE_DEMO
							TRACK_MEM("BAM_ErrorPopup");	pPop = new BAM_ErrorPopup;
							pPop->SetWindowOffsets(89,92);
							pPop->Setup(NULL,MM_MAIN_SQB,201);
							return(TRUE);
							#endif

							#ifdef OS_MAC
								// on Mac, just set saveNum, so make loop 
								// will call saveMgr.Save() which will do the rest
								//pMono->Out("Not implemented on the Mac, yet");
								restoreNum = 1;
							#else
								pMono->Out("\nLoad Button was hit");
								TRACK_MEM("RestoreMenu");	pSave = new SaveMenu;
								pSave->Setup(NULL, LOAD_BUTTON);
							#endif
							return(TRUE);

#ifndef NDEBUG
						case K_F7:
							if(fShowCurrentUnitPath)
							{
								ShowUnitPath((Unit *)NULL);
							}
							else if(vPort.targType == UNIT)
							{
								ShowUnitPath((Unit *)DerefSerial(vPort.targSerial));
							}
							else ShowUnitPath((Unit *)NULL);

							fShowCurrentUnitPath = fShowCurrentUnitPath? FALSE: TRUE;
							break;

						case K_F8:
							// turn ai on
							aiOn = TRUE;
							return(TRUE);

						case K_F9:
							// turn ai off
							aiOn = FALSE;
	
							int	otherSide;
	
							if (playerSide == SIDE1)
							{
								otherSide = SIDE2;
							}
							else
							{
								otherSide = SIDE1;
							}
	
							gUnit = (grip) unitLib.lUnits[otherSide].FirstValue();
							while(gUnit)
							{
								pUnit = ADerefAs(Unit, gUnit);
								AddLocalAction(pUnit->serialNum, GUARD, pUnit->tileX, pUnit->tileY);
								gUnit = (grip) unitLib.lUnits[otherSide].NextValue();
							}
							return(TRUE);

//						case K_T:	// text box editing test
//							if(pMsg->event.modifiers & MOD_ALT && pBam->fMapEdit)
//							{
//								pBox = ADerefAs(BAM_Box, gBox);
//								if(pBox->currState == FALSE)
//								{
//									pBox->Listen(TRUE);
//									pBox->Draw();
//									pBox->Select(TRUE);
//								}
//								return(TRUE);
//							}
//							break;

						case K_C:	// mem check
//							ReportFreeMem();
                     pCommMgr->Silence((bool)!pCommMgr->fSilence);
							BamDebug.Silence((bool)!BamDebug.fSilence);
//							if(pCommMgr)
//								pCommMgr->DumpQueue();

							// unit totals
//							for(loop1 = 0, int1 = 0; loop1 < TOTAL_SIDES; loop1++)
//								int1 += unitLib.lUnits[loop1].count;

//							DebugOut("Sizeof(Unit)=%d * %d units == %d, sizeof(Ani)=%d * %d anis == %d\n",
//								sizeof(Unit), int1, int1 * sizeof(Unit), sizeof(BAM_Ani), vPort.lAnims.count - int1,
//								(vPort.lAnims.count - int1) * sizeof(BAM_Ani));
//							DebugOut("Sizeof(Bam)=%d sizeof(World)=%d (map=%d) sizeof(TileLib)=%d\n",
//								sizeof(BAM_Application), sizeof(World), sizeof(Map), sizeof(Resource) +
//								ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_TILELIB, tileResNum))->size);
//							DebugOut("sizeof(Tigre)=%d\n", sizeof(EventMgr) + sizeof(ContextMgr) + sizeof(ResourceMgr)
//								+ sizeof(SoundMgr) + sizeof(FontMgr) + sizeof(GraphicsMgr) + sizeof(Mono) + sizeof(TCommMgr)
//								+ sizeof(TNetwork));
							break;

						case K_COMMA:
							EndScenario((playerSide == SIDE1)? SIDE2: SIDE1);
							return(TRUE);
	
						case K_PERIOD:
							EndScenario(playerSide);
							return(TRUE);
	
						case K_F12:
							AdjustXP(pBam->playerSide, 100);
							return(TRUE);
#endif	// end of #ifndef RELEASE_VERSION
	
						case K_DOWN:
							vPort.SlideView(vPort.ViewX, vPort.ViewY + 1);		
							return(TRUE);
	
						case K_UP:
							vPort.SlideView(vPort.ViewX, vPort.ViewY - 1);
							return(TRUE);
	
						case K_LEFT:
							vPort.SlideView(vPort.ViewX - 1, vPort.ViewY);
							return(TRUE);
	
						case K_RIGHT:
							vPort.SlideView(vPort.ViewX + 1, vPort.ViewY);
							return(TRUE);
	
						case K_ESC:
							if(vPort.currentVerb == NO_ACTION)
							{
								Option(gPal);
								return(TRUE);
							}
							else
							{
								DeselectAllVerbButtons();
								vPort.SetCurrentVerb(NO_ACTION);
								return(TRUE);
							}
	
						case K_SLASH:
							DeselectAllVerbButtons();
							vPort.SetCurrentVerb(QUESTION);
							return(TRUE);
	
						case K_A:	// ATTACK hotkey
							FakeMouseClick(gVerbButton[(int)ATTACK - 1]);
							return(TRUE);

						case K_G:	// GUARD hotkey
							FakeMouseClick(gVerbButton[(int)GUARD - 1]);
							return(TRUE);

						case K_R:
							FakeMouseClick(gVerbButton[(int)DISBAND - 1]);
							return(TRUE);
	
						case K_S:
							FakeMouseClick(gVerbButton[(int)SKILL - 1]);
							return(TRUE);
	
						case K_D:
							FakeMouseClick(gVerbButton[(int)DROP - 1]);
							return(TRUE);
	
						case K_E:
							FakeMouseClick(gMainButton[(int)MAINBTN_ENCYCLO]);
							return(TRUE);
	
						case K_P:
							FakeMouseClick(gMainButton[(int)MAINBTN_FREEZE]);
							return(TRUE);
	
						case K_V:
							FakeMouseClick(gMainButton[(int)MAINBTN_WINLOSE]);
							return(TRUE);
	
						case K_I:
							FakeMouseClick((int)gItemPortrait);
							return(TRUE);
	
						case K_TAB:
							pList = &unitLib.lUnits[playerSide];
							if(vPort.targType == UNIT)
							{
								// fast-fwd to currently-selected unit
								pUnit = (Unit *)DerefSerial(vPort.targSerial);
								gUnit = (grip)pList->FirstValue();
								while(gUnit && gUnit != pUnit->gSelf)
									gUnit = (grip)pList->NextValue();
								if(!gUnit)
								{
									// shouldnt be possible, but just in case
									break;
								}
							}
							gUnit = (grip)pList->NextValue();
							if(!gUnit)
								gUnit = (grip)pList->FirstValue();									
							if(!gUnit)
								return(TRUE);

							pUnit = ADerefAs(Unit, gUnit);
							vPort.MoveView(pUnit->tileX - VPORT_WIDTH / 2, pUnit->tileY - VPORT_HEIGHT / 2);
							vPort.MoveCursor(pUnit->tileX, pUnit->tileY);
							ADerefAs(WorldMap, gWorldMap)->MoveCursor(vPort.ViewX, vPort.ViewY);
							return(TRUE);

						case K_T:
							FakeMouseClick(gVerbButton[(int)TRANSFORM - 1]);
							break;

						case K_M:
							FakeMouseClick(gVerbButton[(int)MOVE_TO - 1]);
							return(TRUE);

						case K_ENTER:
							FakeMouseClick(gGoSeeButton);
							return(TRUE);

						case K_PAUSE:
							FakeMouseClick(gMainButton[MAINBTN_FREEZE]);
							return(TRUE);
					}
					break;
	
				case E_MOUSE_UP:
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;
					break;

				case E_MOUSE_DOWN:
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;

					if(pMsg->event.value == RIGHT_BTN)
					{
						Guy	*pGuy = ADerefAs(Guy, vPort.gViewPortGuy);
						if(pGuy->scrim.rect.Contains(mouseX, mouseY))
							break;		// let ViewPort::HandleMsg() deal with it

						pGuy = ADerefAs(Guy, gWorldMap);
						if(((WorldMap *)pGuy)->worldRect.Contains(mouseX, mouseY))
							break;		// let ViewPort::HandleMsg() deal with it

						// if RIGHT_BTN pressed inside Verb Button area, then reset all verb buttons.
						// We do this here because otherwise a button will gobble the msg
						// just because it fell within that button's rect.
						DeselectAllVerbButtons();
						vPort.SetCurrentVerb(NO_ACTION);
						return(TRUE);
					}
					break;
			}
	}

	// if msg survives, pass it along to the receivers
	if(BAM_Room::HandleMsg(pMsg))
	{
//		_MARK_("HandleMsg() end - msg claimed by receiver");
		return(TRUE);
	}
	
	// if no receivers wanted it, maybe it's for us after all?
	switch (pMsg->type)
	{
		case MSG_NOTICE:
			switch (pMsg->notice.type) 
			{
				case N_SETUP:
//					APost(N_SETUP, cControls.gSelf);
//					APost(N_SETUP, locationMod.gSelf, (void*)startRoom);
					return TRUE;
			}
			break;

		case MSG_EVENT:
			switch (pMsg->event.type)
			{
				case E_MOUSE_DOWN:
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;
					break;

				case E_MOUSE_UP:
					// the following is done to make sure that the active button (if any) receives the MOUSE_UP
					// event even if mouse has wandered out of the button's rectangle
					if(gCurControl)
					{
						pObject = ADerefAs(Object, gCurControl);
						if(pObject->HandleMsg(pMsg))
							return(TRUE);
					}
					break;
				case E_KEY_DOWN:
//					switch(pMsg->event.value)
//					{
//						case K_ESC:
//							break;
//					}
					break;
			}
	}
	return FALSE;
}

void
World::AllowMouseDragging(bool fNewState, bool fForce)
{
	if (fNewState)
	{
		if(dragSuspendCnt || fForce)
		{
			if(fForce)
				dragSuspendCnt = 0;
			else dragSuspendCnt--;

			if (!dragSuspendCnt)
			{
				pMouse->SetDraggableRect(vPort.rInner.x1, vPort.rInner.y1,
					vPort.rInner.x2, vPort.rInner.y2);
			}
		}
	}
	else
	{
		dragSuspendCnt++;
		if (dragSuspendCnt == 1 || fForce)
		{
			//
			pMouse->SetDraggableRect(-1, -1, -1, -1);
			if(pMouse->fDragMode)
				pMouse->EnableDragging(FALSE);
		}
	}
}

/*void
World::AllowMouseDragging(bool fNewState)
{
	if(fNewState)
	{
		pMouse->SetDraggableRect(vPort.rInner.x1, vPort.rInner.y1,
			vPort.rInner.x2, vPort.rInner.y2);
	}
	else
	{
		pMouse->SetDraggableRect(-1, -1, -1, -1);
		if(pMouse->fDragMode)
			pMouse->EnableDragging(FALSE);
	}
}*/

void
World::FakeMouseClick(grip gButton)
{
	int			oldX, oldY, buttonX, buttonY;
	BAM_Button	*pButton;

	if(vPort.fFenceMode)
		return;

	pButton = ADerefAs(BAM_Button, gButton);
	buttonX = (pButton->scrim.rect.x1 + pButton->scrim.rect.x2) / 2;
	buttonY = (pButton->scrim.rect.y1 + pButton->scrim.rect.y2) / 2;
	oldX = pMouse->GetX();
	oldY = pMouse->GetY();
	pMouse->SetPos(buttonX, buttonY);
	APostEvent(E_MOUSE_DOWN, LEFT_BTN, FALSE);
	APostEvent(E_MOUSE_UP, LEFT_BTN, FALSE);
	pMouse->SetPos(oldX, oldY);
}

void
World::CheatSummon(unitType unit1)
{
	int	tileX, tileY, loop1;

	if(!map.IsTypeAllowed(unit1, playerSide))
		return;

	for(loop1 = 0; loop1 < TOTAL_SIDES; loop1++)
	{
		if(loop1 != SIDE1 && loop1 != SIDE2)
			continue;

		tileX = map.portalCoords[loop1][0];
		tileY = map.portalCoords[loop1][1];

		// if portal blocked by unit
		if(unitLib.GetUnitGrip(tileX, tileY))
			return;
	}
	AdjustManaLevel(SIDE1, MANA_SUMMON);
	AddLocalAction(NULL, SUMMON, map.portalCoords[SIDE1][0],
		map.portalCoords[SIDE1][1], (int)unit1);
	AdjustManaLevel(SIDE2, MANA_SUMMON);
	AddLocalAction(NULL, SUMMON, map.portalCoords[SIDE2][0],
		map.portalCoords[SIDE2][1], (int)unit1);
}

// SUMMON button processor
void
World::Summon(unitType newUnitType, uint32 xPos, uint32 yPos, uint32 newSide)
{
	grip		gAni;
//	Unit		*pUnit;

	if(xPos >= WORLD_WIDTH || yPos >= WORLD_HEIGHT)
		return;

	if(unitLib.GetUnitGrip(xPos, yPos))
		return;

	if(!map.IsTypeAllowed(newUnitType, newSide))
		return;

	if(!SpendMana(newSide, MANA_SUMMON, xPos, yPos))
	{
		return;
	}

	if(unitLib.lUnits[newSide].count >= MAX_UNITS_PER_SIDE)
		return;

	TRACK_MEM("Unit");	gAni = unitLib.NewUnit(newUnitType, newSide, xPos, yPos, NO_ITEM, GUARD);
	vPort.CheckAni(gAni);

	AdjustXP(newSide, unitLib.GetUnitGroup(newUnitType)->expGained);

	if(playerSide == newSide && !tShowOffTimer)
		vPort.MoveCursor(xPos, yPos);	// make cursor target on new unit

	// poof!
	TRACK_MEM("SummonAni");	gAni = vPort.NewAni(RES_ANIM, 358, 1, PRI_SKY, TRUE, FALSE, 4, xPos, yPos, FALSE);
	vPort.CheckAni(gAni);

	LaunchSound(SND_SUMMON, xPos, yPos);

	if(bGlobal.storyLine == TUTORIAL)
	{
		grip	gUnit;
		Unit	*pUnit;
		int	count = 0;

		if(newUnitType == ACOLYTE && !bGlobal.fTutorialGoals[TUT_ACOLYTES_CREATED])
		{
			gUnit = (grip)unitLib.lUnits[playerSide].FirstValue();
			while(gUnit)
			{
				pUnit = ADerefAs(Unit, gUnit);
				if(pUnit->type == ACOLYTE)
					count++;
				gUnit = (grip)unitLib.lUnits[playerSide].NextValue();
			}
			if(count >= 6)
			{
				SetTutorialGoal(TUT_ACOLYTES_CREATED, TRUE);
			}
		}
	}
}

void
World::SetTutorialGoal(tutorialGoals goal, bool fState)
{
	int	loop1;
	grip	gAni;
	Tutorial	*pTutor;

	if(!bGlobal.fTutorialGoals[goal])
	{
		if(fState)
		{
			// find lowest unsatisfied condition
			for(loop1 = 0; loop1 < (int)TUT_TOTAL_GOALS && bGlobal.fTutorialGoals[loop1]; loop1++);

			// if goal being satisfied was current goal OR last goal
			if(loop1 == goal || goal == TUT_BLDG_DESTROYED)
			{
//				pMono->Out("Goal %d satisfied!\n", (int)goal);

				//tutorial will set goal true
				TRACK_MEM("Tutorial");	pTutor = new Tutorial;
				pTutor->Setup(gPal,FALSE,TRUE,goal);
			}
			else
			{
				bGlobal.fTutorialGoals[goal] = TRUE;
			}

	
			switch(goal)
			{
				case TUT_ACOLYTES_CREATED:
					// add item
					itemMgr.PlaceItem(3, 29, DRIFT_DISC, 2, FALSE);
					TRACK_MEM("PoofAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1,
						PRI_WALKER + 2, TRUE, FALSE, 4, 3, 29, FALSE);				// poof!
					pWorld->vPort.CheckAni(gAni);
					break;

				case TUT_UNIT_TRANSFORMED:
					// add item
					itemMgr.PlaceItem(28, 1, MASON_MIX, 2, FALSE);
					TRACK_MEM("PoofAni");	gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_TRANSFORM, 1,
						PRI_WALKER + 2, TRUE, FALSE, 4, 29, 1, FALSE);				// poof!
					pWorld->vPort.CheckAni(gAni);
					break;

//				case TUT_UNIT_HEALED:
//				case TUT_BLDG_REPAIRED:
//					bGlobal.fTutorialGoals[goal] = fState;
//					if(bGlobal.fTutorialGoals[TUT_UNIT_HEALED] && bGlobal.fTutorialGoals[TUT_BLDG_REPAIRED])
//					{
//						pMono->Out("Goals %d && %d satisfied!\n", (int)TUT_UNIT_HEALED, (int)TUT_BLDG_REPAIRED);
//					}
//					break;			
			}
		}
	}
}

// USE ITEM button processor
void
World::UseItem(void)
{
	Unit		*pUnit;
	
	if(vPort.targType != UNIT)
		return;
	pUnit = (Unit *)DerefSerial(vPort.targSerial);
	pUnit->UseItem();
}

// DROP ITEM button processor
void
World::DropItem(void)
{
	Unit		*pUnit;

	// make sure a unit is currently targetted
	if(vPort.targType != UNIT)
		return;

	// make sure unit has an item
	pUnit = (Unit *)DerefSerial(vPort.targSerial);
	if(pUnit->item == NO_ITEM)
		return;

	pUnit->DropItem();
}

void
World::DeselectAllVerbButtons(void)
{
	uint32		loop1;
	BAM_Button	*pButton;

	for(loop1 = 0; loop1 < 7; loop1++)	// make sure all verb buttons are de-selected
	{
		if(gVerbButton[loop1])
		{
			pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
			if(pButton->currState == TRUE && pButton->fIsToggle)
				pButton->Select(FALSE);
		}
	}
	if(gItemPortrait)
	{
		pButton = ADerefAs(BAM_Button, gItemPortrait);
		if(pButton->currState == TRUE && pButton->fIsToggle)
			pButton->Select(FALSE);
	}
}

// grip of whatever button is currently activated, if any
void
World::SetCurrentControl(grip gNewControl)
{
	BAM_Button	*pButton;

	// if another control is already current (between MOUSE_DOWN and MOUSE_UP)
	if(gCurControl && gCurControl != gNewControl)
	{
		pButton = ADerefAs(BAM_Button, gCurControl);

		if(pButton->currState == TRUE)
		{
			// force it down, since it may not be receiving the MOUSE_DOWN
			pButton->Select(FALSE);
		}
	}

	gCurControl = gNewControl;
}

#define ANIM_UNIT_COVER	116
#define ANIM_HP_GUAGE	132
void
World::ClearAttDefMovGuages(void)
{
	CelHeader	*pDestCel;

	// ptr to header of main screen DCEL, which we draw into
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));

	// clean area first
	CopyCel(pDestCel, 40, 335, RES_ANIM, ANIM_UNIT_COVER, 3, FALSE);
}

void
World::ClearHPGuage(void)
{
	CelHeader	*pDestCel, *pSrcCel;
	grip			gRes;

	// ptr to header of main screen DCEL, which we draw into
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));

	TRACK_MEM("CoverRes");	gRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_UNIT_COVER);
	pSrcCel = ADerefAs(Resource, gRes)->GetHeader(2);

	// clean area first
	CopyCel(pDestCel, 40, 319, RES_ANIM, ANIM_UNIT_COVER, 2, FALSE);

  	rTemp1.Set(40, 319, 40 + pSrcCel->width - 1, 319 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);
}

void
World::DrawAttDefMovGuages(int32 attack, int32 maxAttack, int32 defense, int32 maxDefense, int32 movement,
	int32 maxMovement, bool fShowSign)
{
	int	rOldFont = pFontMgr->curFontNum;

	CelHeader	*pDestCel;

	// ptr to header of main screen DCEL, which we draw into
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));

	ClearAttDefMovGuages();
	
	pFontMgr->SetRes(9060);
	SetFontColors(CI_SKIP, 155, 165, 65, 62, 155, 165);

	DrawGuage(squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_ATTACK),
		attack, maxAttack, 133, fShowSign, 44, 338, pDestCel);
	DrawGuage(squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_DEFENSE),
		defense, maxDefense, 134, fShowSign, 44, 354, pDestCel);
	DrawGuage(squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_MOVE),
		movement, maxMovement, 135, fShowSign, 44, 370, pDestCel);

	pFontMgr->SetRes(rOldFont);
}

void
World::DrawGuage(char *szLabel, int curr, int max, uint rAniRes, bool fShowSign,
	int xPos, int yPos, CelHeader *pDestCel)
{
	grip			gRes;
	char			string1[4], *pString;
	int			celNum, celWidth, pixelX, loopX, markerCount1, markerCount2;
	CelHeader	*pSrcCel;

	TRACK_MEM("GuageAni");	gRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, rAniRes);
	pString = string1;
	if(curr == 0)
	{
		*(pString++) = ' ';
	}
	else if(curr > 0)
	{
		if(fShowSign)
			*(pString++) = '+';
		else *(pString++) = ' ';
	}
	else if(curr < 0)
	{
		if(fShowSign)
			*(pString++) = '-';
		else *(pString++) = ' ';
	}
	if(curr < max)
		celNum = 5;		// deficiency
	else celNum = 4;	// increase
	markerCount1 = AMax(abs(curr), abs(max));

	*(pString++) = (char)('0' + (char)abs(curr));
	*(pString) = NULL;
	ASetString(xPos, yPos + 2, string1, (uchar *)pDestCel, pDestCel->width, NULL);

	// draw label
	ASetString(xPos + 12, yPos + 2, szLabel, (uchar *)pDestCel, pDestCel->width, NULL);

	// draw markers
	pSrcCel = ADerefAs(Resource, gRes)->GetHeader(1);
	celWidth = pSrcCel->width;
	pixelX = xPos + 30;
	markerCount2 = AMin(curr, max);
	for(loopX = 0; loopX < markerCount2; loopX++, pixelX += celWidth)
		CopyCel(pDestCel, pixelX, yPos, RES_ANIM, rAniRes, 1, TRUE);
	for(; loopX < markerCount1; loopX++, pixelX += celWidth)
		CopyCel(pDestCel, pixelX, yPos, RES_ANIM, rAniRes, celNum, TRUE);
}

// called when unit enters new terrain
void
World::UpdateAttDefMoveOnly(Unit *pUnit)
{
	grip			gCoverUpAni;
	Resource		*pRes;
	CelHeader	*pSrcCel;

	if(vPort.groupSize)
	{
		// unit is part of a group - no individual stats allowed to show
		return;
	}

	DrawAttDefMovGuages(pUnit->currAttack, pUnit->baseAttack, pUnit->currDefense, pUnit->baseDefense,
		(pUnit->currMove < 0)? 0: pUnit->currMove, pUnit->baseMove, FALSE);

	// determine rectangle to update based on w/h of cover-up cel
	TRACK_MEM("gCoverUp");	gCoverUpAni = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_UNIT_COVER);
	pRes = ADerefAs(Resource, gCoverUpAni);
	pSrcCel = pRes->GetHeader(3);

	rTemp1.Set(40, 335, 40 + pSrcCel->width - 1, 335 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);
}

void
World::UpdateHPGuageOnly(int32 currHP, int32 maxHP, int ownerSide)
{
	if(vPort.groupSize)
	{
		// unit is part of a group - no individual stats allowed to show
		return;
	}

	if(maxHP)
		DrawHPGuage(currHP, maxHP, ownerSide);
	else ClearHPGuage();
}

void
World::DrawHPGuage(int32 currHP, int32 maxHP, int ownerSide)
{
	int32			HPBarLen, HPBlack, loopY;
	grip			gRes;
	CelHeader	*pDestCel, *pSrcCel;
	uchar			*pDestData, *pOffSet;
	char			szHPString[10];
	int			HPGuageWidth;

	pFontMgr->SetRes(9060);

	TRACK_MEM("CoverRes");	gRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_UNIT_COVER);
	pSrcCel = ADerefAs(Resource, gRes)->GetHeader(2);
  	rTemp1.Set(40, 319, 40 + pSrcCel->width - 1, 319 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);

	// ptr to header of main screen DCEL, which we draw into
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));
	pDestData = (uchar *)pDestCel + sizeof(CelHeader);

	// copy entire guage, as if full.  Then chop off what we dont want.
	CopyCel(pDestCel, 43, 321, RES_ANIM, ANIM_HP_GUAGE, 1, FALSE, clut[ownerSide]);

	TRACK_MEM("HPGuage");	gRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_HP_GUAGE);
	pSrcCel = ADerefAs(Resource, gRes)->GetHeader(1);
	HPGuageWidth = L2R(pSrcCel->width);

	pOffSet = pDestData + 322 * L2R(pDestCel->width) + L2R(44);
	if(currHP < 0)
		currHP = 0;	// safety
	if(!maxHP)
		HPBarLen = 0;	// div/0 safety
	else
	{
		HPBarLen = (currHP * 100) / maxHP;		// % of total

		// # of pixels to show	(+99 simulates percentage round-up, so that
		// low-end percentages dont get rounded off to 0 in the display)
		HPBarLen = ((HPBarLen * HPGuageWidth) + 99) / 100;
	}
	// length in pixels of HP bar to erase
	HPBlack = HPGuageWidth - HPBarLen - 1;
	if(HPBlack < 0)
		HPBlack = 0;
	pOffSet += HPBarLen;
	for(loopY = 0; loopY < 11;	loopY++, pOffSet += L2R(pDestCel->width))
		memset(pOffSet, CI_BLACK, HPBlack);

	sprintf(szHPString, "%d HP", currHP);
	SetFontColors(CI_SKIP, 155, 165, 65, 62, 155, 165);
	ASetString(44, 323, szHPString, (uchar *)pDestCel, 116 - 44, NULL, DG_JUST_CENTER);
}

// draw the Ownership banner for current target, over portrait icon
void
World::DrawBanner(uint32 player)
{
	grip		gAni;
	Resource	*pRes;
	CelHeader	*pSrcCel, *pDestCel;
	uint16		celNum;

//	pMono->Out("DrawBanner(%d): gISCel==%d", player, gISCel);

	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));	// ptr to header of main screen DCEL, which we draw into
	
	TRACK_MEM("Banner");	gAni = ALoadDebug(__FILE__, __LINE__, RES_ANIM, 9004);
	pRes = ADerefAs(Resource, gAni);
	switch(player)
	{
		case SIDE0:
		case SIDE3:
			celNum = 1;
			break;
		case SIDE1:
			celNum = 2;
			break;
		case SIDE2:
			celNum = 3;
			break;
		default:
			pMono->Out("World::DrawBanner(%d) - error, invalid side!\n", player);
			return;
	}
	pSrcCel = pRes->GetHeader(celNum);
 	CopyCel(pDestCel, 1, 307, RES_ANIM, 9004, celNum, TRUE, pWorld->clut[player]);
	rTemp1.Set(1, 307, 1 + pSrcCel->width - 1, 307 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);
}

int32
World::GetManaLevel(uint32 side)
{
	if(side > SIDE2)
	{
		pMono->Out("World::GetManaLevel(%d) - error, invalid side!\n", side);
		return(NULL);
	}
	return(mana[side]);
}

bool
World::StartResearch(unitType subject, int researchSecs)
{
	BAM_Button	*pButton;
	ticks_t currTicks;

	if(bGlobal.unitsResearched[subject])
	{
		// doesnt need to be researched
		return(FALSE);
	}

	currTicks = ATicks();
	currResearchSubject = subject;
	currResearchPhase = -1;
	tCurrResearchPhaseDuration = (researchSecs * TICKS_PER_SEC) / RESEARCH_PHASES;
	tCurrResearchPhaseEnd = currTicks;

	// turn off research button
	pButton = ADerefAs(BAM_Button, gSpendXPButton);
	if(pButton->currState != DG_DISABLED);
		pButton->Select(DG_DISABLED);

	return(TRUE);
}

int
World::AdjustXP(uint32 side, int xpAdjust)
{
	if(bGlobal.storyLine == NETGAME && xpAdjust > 0)
	{
		// 2-player mode, all XP bonuses X10
		xpAdjust *= 10;
	}

	bGlobal.curXP[side] += xpAdjust;

	if(bGlobal.curXP[side] < 0)
		bGlobal.curXP[side] = 0;
	else if(bGlobal.curXP[side] > XP_MAX)
		bGlobal.curXP[side] = XP_MAX;

	if(xpAdjust && side == playerSide)
	{
		// update on-screen counter

		char			string1[80];
		CelHeader	*pDestCel, *pSrcCel;
		grip			gRes;

		// ptr to header of main screen DCEL, which we draw into
		pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));

		// erase old text
		CopyCel(pDestCel, 242, 20, RES_ANIM, ANIM_UNIT_COVER, 6, FALSE);
		TRACK_MEM("CoverRes");	gRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_UNIT_COVER);
		pSrcCel = ADerefAs(Resource, gRes)->GetHeader(6);
		rTemp1.Set(242, 20, 242 + pSrcCel->width - 1, 20 + pSrcCel->height - 1);
		AUpdateRect(&rTemp1);

		pFontMgr->SetRes(9050);
		SetFontColors(CI_SKIP, 65, 62, 155, 165);
		sprintf(string1, "%s %d", squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_XP),
			bGlobal.curXP[side]);
		ASetString(244, 23, string1, (uchar *)pDestCel, pDestCel->width, 0, DG_JUST_LEFT);
	}

	return(bGlobal.curXP[side]);
}

int32
World::AdjustManaLevel(uint32 side, int32 manaAdj)
{
	if(side > SIDE2)
	{
		pMono->Out("World::AdjustManaLevel(%d) - error, invalid side!\n", side);
		return(NULL);
	}
	#ifndef NDEBUG
	BamDebug.Out("World::AdjustManaLevel(side %d, %d)\n", side, manaAdj);
	#endif

	lastMana[side] = mana[side];
	mana[side] += manaAdj;
	if(mana[side] > MANA_MAX)
		mana[side] = MANA_MAX;
	else if(mana[side] < 0)
		mana[side] = 0;
	return(mana[side]);
}

void
World::SetManaLevel(uint32 side, int32 newMana)
{
	if(side > SIDE2)
	{
		pMono->Out("World::SetManaLevel(%d, %d) - error, invalid side!\n", side, newMana);
		return;
	}

	lastMana[side] = mana[side];
	mana[side] = newMana;
	if(mana[side] > 999)
		mana[side] = 999;
	else if(mana[side] < 0)
		mana[side] = 0;
}

// set ItemPortrait button
void
World::DrawItemPortrait(itemType item)
{
	CelHeader	*pDestCel;
	BAM_Button	*pButton;

	if(statBoxCurrMode != TERRAIN)
		return;

	pFontMgr->SetRes(9050);
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));	// ptr to header of main screen DCEL, which we draw into

	// set Item button portrait
	pButton = ADerefAs(BAM_Button, gItemPortrait);
	if(item == NO_ITEM)
	{
		pButton->SetButtonRes(FALSE, RES_ANIM, 106, 1);	// default
	}
	else
	{
		pButton->SetButtonRes(FALSE, RES_ANIM, itemMgr.GetPortAnim(item), 2);
	}
	pButton->Select(FALSE);

	// draw item title
	TRACK_MEM("TitleCoverUp");
 	CopyCel(pDestCel, 198, 306, RES_ANIM, 116, 1, FALSE);
	SetFontColors(CI_SKIP, 64, 74);
//	ASetString(318, 306, &itemMgr.itemNames[item][0], (uchar *)pDestCel, pDestCel->width, NULL, DG_JUST_RIGHT);
   if(item)
     ASetString(318, 306, squib1.Load(SQUIB_ITEM_NAMES, (int)item),
     (uchar *)pDestCel, pDestCel->width, NULL, DG_JUST_RIGHT);

	rTemp1.Set(202, 306, pDestCel->width, 317);
	AUpdateRect(&rTemp1);
}

void
World::DrawManaGuage(int32 manaCount)
{
	CelHeader	*pSrcCel, *pDestCel;
	grip			gAni;
	Resource		*pRes;
	char			string1[20];

	pFontMgr->SetRes(9050);
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));	// ptr to header of main screen DCEL, which we draw into

	// do text mana counter
	TRACK_MEM("ManaCoverUp");	gAni = ALoadDebug(__FILE__, __LINE__, RES_ANIM, 116);
	pRes = ADerefAs(Resource, gAni);
	pSrcCel = pRes->GetHeader(1);
	CopyCel(pDestCel, 254, 168, RES_ANIM, 116, 1, FALSE);		// overwrite previous text
	rTemp1.Set(254, 168, 254 + pSrcCel->width - 1, 168 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);

	sprintf(string1, "MANA:%d", manaCount);
	SetFontColors(CI_SKIP, 64, 74, 154, 151);
	ASetString(257, 170, string1, (uchar *)pDestCel, pDestCel->width, NULL);
}

void
World::DrawUnitStatus(uint32 serialNum, bool fMinimum)
{
	Unit			*pUnit;
	UnitGroup	*pUGroup;
	BAM_Button	*pButton;
	CelHeader	*pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel)), *pCel;
	char			string1[80], name[40], *pStr;
	int			int1;
	Resource		*pRes;

	pUnit = (Unit *)DerefSerial(serialNum);
	if(!pUnit)
		return;

	pUGroup = unitLib.GetUnitGroup(pUnit->type);

	TRACK_MEM("StatsCoverUp");	pRes = ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_ANIM, 116));
	pCel = (CelHeader *)pRes->GetHeader(1);
	CopyCel(pDestCel, 1, 306, RES_ANIM, 116, 1, FALSE);	// overwrite any previous text
	rTemp1.Set(1, 306, 3 + pCel->width - 1, 306 + pCel->height - 1);
	AUpdateRect(&rTemp1);

//	pMono->Out("World::DrawUnitStatus(g%d, s%d, %s, %d/%d);\n", pUnit->gSelf, pUnit->serialNum,
//		pUnit->unitName, pUnit->tileX, pUnit->tileY);

	if(pUnit->type == ACOLYTE)
	{
		if(pUnit->currAction == GUARD || pUnit->currAction == TRANSFORM)
		{
			//	still shows ACOLYTE defense and not OBELISK defense
			pUGroup = unitLib.GetUnitGroup(OBELISK);
		}
	}

	// select a portrait
	pButton = ADerefAs(BAM_Button, gPortrait);
	if(pUnit->gGroupCursor)
	{
		// unit is part of a selected group - show group portrait
		pButton->SetButtonRes(FALSE, RES_ANIM, 2618, 1);
	}
	else if(statBoxCurrMode == TERRAIN)
	{
//		if(pUnit->fDoppleganger)
//			int1 = PORTRAIT_DOPPLEGANGER;
//		else
			int1 = pUGroup->portAnim;
		pButton->SetButtonRes(FALSE, RES_ANIM, int1, 1);
	}
	else if(statBoxCurrMode == TRANSFORMING)
	{
		// METAPOD portrait
		pButton->SetButtonRes(FALSE, RES_ANIM, 2616, 1);
	}
	pButton->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[pUnit->player]);
	if(pButton->currState == FALSE)
	{
		pButton->SetState(S_CHANGED, TRUE);
		pButton->Select(FALSE);
	}

//	if(pUnit->fDoppleganger)
//		strcpy(name, "DOPPLEGANGER");
//	else
	if(pUnit->gGroupCursor)
	{
		strcpy(name, squib1.Load(SQUIB_WORLD_MSGS, SQUIB_WORLD_GROUP));
	}
	else if(statBoxCurrMode == TRANSFORMING)
		strcpy(name, squib1.Load(SQUIB_WORLD_MSGS, SQUIB_METAPOD));
	else strcpy(name, pUnit->unitName);
	strcat(name, " ");

	string1[0] = NULL;
	pStr = NULL;
	if(!pUnit->gGroupCursor && pUnit->fAutoControl)
	{
		switch(pUnit->primaryAction)
		{
			case BERSERK:
				pStr = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_PSYCHO);
				break;

			case FOLLOW:
				if(pUnit->tFearLureStonedTimer)
					pStr = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_LOVESICK);
				break;

			case AVOID:
				if(pUnit->tFearLureStonedTimer)
					pStr = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_AFRAID);
				break;

			case GET_STONED:
				pStr = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)SQUIB_WORLD_STONER);
				break;
		}
		if(pStr)
			sprintf(string1, "%s(%s)", name, pStr);	
	}
	if(!string1[0])
		strcpy(string1, name);

	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP, 64, 74, CI_BLACK, CI_BLACK, 154, 151);
	ASetString(43, 307, string1, (uchar *)pDestCel, pDestCel->width, NULL);

	DrawBanner(pUnit->player);

	if(pUnit->gGroupCursor)
	{
		ClearHPGuage();
		ClearAttDefMovGuages();
		DrawItemPortrait(NO_ITEM);
	}
	else if(!fMinimum)
	{
		if(pUnit->maxHP)		// if capable of having HPs
			DrawHPGuage(pUnit->currHP, pUnit->maxHP, pUnit->player);
		else ClearHPGuage();	// else TORTOISE, WEIRD_WARD, etc
		DrawAttDefMovGuages(pUnit->currAttack, pUnit->baseAttack, pUnit->currDefense, pUnit->baseDefense,
			(pUnit->currMove < 0)? 0: pUnit->currMove, pUnit->baseMove, FALSE);
		if(statBoxCurrMode == TRANSFORMING)
			DrawItemPortrait(NO_ITEM);
		else DrawItemPortrait(pUnit->item);
	}
	lastUnitSerialDrawn = serialNum;
}

void
World::SetStatusBoxMode(TargetType newMode)
{
	int			loop1;
	BAM_Button	*pButton;
	CelHeader	*pSrcCel, *pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));
	char			*pString;
	Unit			*pUnit;
	Resource		*pRes;

	if(statBoxCurrMode == newMode)
		return;

	// turn off and unplug all buttons for current mode
	switch(statBoxCurrMode)
	{
		case TERRAIN:
			for(loop1 = 0; loop1 < 7; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
				pButton->Select(DG_DISABLED);
				pButton->SetOwnerCel(NULL);	// prevent redrawing
	  			pButton->Listen(FALSE);
			}
	  		DrawItemPortrait(NO_ITEM);
			break;

		case TRANSFORMING:
			// undo certain tweaks made only by this mode
			pButton = ADerefAs(BAM_Button, gPortrait);
			pButton->Listen(TRUE);
		
			pButton = ADerefAs(BAM_Button, gTransformButton[2]);
//			aniNum = acolyteBldgAnis[loop1];
			pButton->SetButtonRes(FALSE, RES_ANIM, 2724, 1);
			pButton->SetButtonRes(TRUE, RES_ANIM, 2724, 2);
			pButton->font = 9064;
			pButton->SetPos(transformButtonCoords[2][0], transformButtonCoords[2][1]);
			pButton->SetColors(1, 93, 90, CI_BLACK);		//, 151, 154, 151);				// inactive colors
			pButton->SetColors(2, 154, 151, CI_BLACK);	//, 151, 154, 151);			// active colors
			pButton->SetTextJustify(DG_JUST_RIGHT, DG_JUST_BOTTOM);
			pButton->SetTextOffSet(-2, -2);

			// fall through

		case BUILDING:
		case UNIT:
			for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
				pButton->SetOwnerCel(NULL);	// enable redrawing
				pButton->Listen(FALSE);
				pButton->SetCelText(1, "");
				pButton->SetCelText(2, "");
			}
			break;				
	}

	statBoxCurrMode = newMode;
	switch(statBoxCurrMode)
	{
		case TERRAIN:
			for(loop1 = 0; loop1 < 7; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gVerbButton[loop1]);
				pButton->SetOwnerCel(rISCel);	// enable redrawing
			}
			pButton = ADerefAs(BAM_Button, gPortrait);
			pButton->SetButtonRes(FALSE, RES_ANIM, 104, 1);
			pButton->SetButtonRes(TRUE, RES_ANIM, 104, 2);
			pButton->SetCelText(1, "");
			pButton->SetCelText(2, "");

			if(vPort.targType == UNIT)
				DrawUnitStatus(vPort.targSerial, FALSE);

			SetVerbButtons(vPort.targType, vPort.targSerial);

			pButton = ADerefAs(BAM_Button, gPortrait);
			if(pButton->currState == FALSE)
			{
				// force redraw
				pButton->SetState(S_CHANGED, TRUE);
				pButton->Select(FALSE);
			}
			break;

		case TRANSFORMING:	// unit-to-unit in progress mode
			pUnit = (Unit *)DerefSerial(vPort.targSerial);

			for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
				pButton->SetOwnerCel(rISCel);	// enable redrawing
			}
			SetTransformingButtons(vPort.targSerial);
			break;

		case UNIT:		// unit-to-unit selection mode
		case BUILDING:	// foundation-to-bldg selection mode
			pUnit = (Unit *)DerefSerial(vPort.targSerial);

			for(loop1 = 0; loop1 < TRANSFORM_TOTAL; loop1++)
			{
				pButton = ADerefAs(BAM_Button, gTransformButton[loop1]);
				pButton->SetOwnerCel(rISCel);	// enable redrawing
			}

			pButton = ADerefAs(BAM_Button, gItemPortrait);
			pButton->Listen(FALSE);

			// overwrite previous unit banner & name
			CopyCel(pDestCel, 3, 306, RES_ANIM, 116, 1, FALSE);

			DrawBanner(pUnit->player);
			pFontMgr->SetRes(9050);
			SetFontColors(CI_SKIP, 64, 74, CI_BLACK, CI_BLACK, 154, 151);
			pString = squib1.Load(SQUIB_WORLD_MSGS, MSG_TRANSFORM_INTO);
			ASetString(43, 307, pString, (uchar *)pDestCel, pDestCel->width, NULL);

			rTemp1.Set(43, 306, 43 + pDestCel->width - 1, 306 + AFontPoint());
			AUpdateRect(&rTemp1);

			// hide leftovers
			CopyCel(pDestCel, 312, 318, RES_ANIM, 116, 4, FALSE);
			pRes = ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_ANIM, 116));
			pSrcCel = pRes->GetHeader(4);
			rTemp1.Set(312, 318, 312 + pSrcCel->width - 1, 318 + pSrcCel->height - 1);
			AUpdateRect(&rTemp1);

			SetTransformButtons(vPort.targSerial);
			break;
	}
}

// box for target status
void
World::DrawStatusBox(void)
{
	BAM_Button	*pButton;
	Resource		*pRes;
	CelHeader	*pDestCel, *pSrcCel;
	uchar			*pDestData;
	char			*pStr;
	grip			gCoverUpAni;
	grip			gUnit;
	Terrain		*pTerrain;
	terrainType	terrain1;
	MapSpace		*pSpace;
	Unit 			*pUnit;
	int32			mapOffSet, int1;
	BAMItem		*pItem;
	TargetType	oldTargetType;

	// gather information on whatever's underneath the viewport cursor
	mapOffSet = vPort.CursorY * WORLD_WIDTH + vPort.CursorX;
	pDestCel = (CelHeader *) AGetResData(ALoadDebug(__FILE__, __LINE__, RES_CEL, rISCel));	// ptr to header of main screen DCEL, which we draw into
	pDestData = (uchar *)pDestCel + sizeof(CelHeader);

	// overwrite previous target stats
	TRACK_MEM("StatsCoverUp");	gCoverUpAni = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_UNIT_COVER);
	pRes = ADerefAs(Resource, gCoverUpAni);
	pSrcCel = pRes->GetHeader(3);
	rTemp1.Set(40, 335, 40 + pSrcCel->width - 1, 335 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);

	// overwrite previous target name
	pSrcCel = pRes->GetHeader(1);
	CopyCel(pDestCel, 1, 306, RES_ANIM, 116, 1, FALSE);	// overwrite any previous text
	rTemp1.Set(1, 306, 1 + pSrcCel->width - 1, 306 + pSrcCel->height - 1);
	AUpdateRect(&rTemp1);

	//--------- cover up done, now draw new data

	pButton = ADerefAs(BAM_Button, gPortrait);
	pFontMgr->SetRes(9050);
	SetFontColors(CI_SKIP, 64, 74);

	lastUnitSerialDrawn = NULL;	// for units that wander under cursor
		
	// if an un-targeted unit has wandered under viewport cursor..
	gUnit = pWorld->unitLib.GetUnitGrip(vPort.CursorX, vPort.CursorY);
	if(gUnit)
		pUnit = ADerefAs(Unit, gUnit);
	else pUnit = NULL;
	if(pUnit && pUnit->serialNum != vPort.targSerial &&
		!(vPort.fog[vPort.CursorY][vPort.CursorX] & FOG_CENTER))
	{
		pUnit = ADerefAs(Unit, gUnit);
		DrawUnitStatus(pUnit->serialNum);
	}
	else switch(vPort.targType)
	{
		case NOTARGET:
			ClearAttDefMovGuages();
			ClearHPGuage();
			pButton->SetButtonRes(FALSE, RES_ANIM, 104, 1);
			DrawBanner(SIDE0);
			DrawItemPortrait(NO_ITEM);
			pStr = squib1.Load(SQUIB_WORLD_TERRAIN, SQUIB_TERRAIN_UNKNOWN, TRUE);
			if(pStr)
				ASetString(43, 307, pStr, (uchar *)pDestCel, pDestCel->width, NULL);
			#ifndef NDEBUG
			else APanic("World::DrawStatusBox() null string #1\n");
			#endif
			break;

		case UNIT:
			pUnit = (Unit *)DerefSerial(vPort.targSerial);
			DrawUnitStatus(vPort.targSerial);
			if(statBoxCurrMode != TERRAIN)
			{
				oldTargetType = statBoxCurrMode;
				statBoxCurrMode = NOTARGET;
				SetStatusBoxMode(oldTargetType);
			}
			break;

		case TERRAIN:
			terrain1 = (terrainType)map.GetTerrainNum(vPort.targX, vPort.targY);
			pTerrain = &pTileLib->terrains[(int)terrain1];
			// set pic for UNSELECTED state of button to anim X, cel #1				
			int1 = pTerrain->portraitNum;
			pButton->SetButtonRes(FALSE, RES_ANIM, int1, 1);
			pButton->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[SIDE0]);
			pStr = squib1.Load(SQUIB_WORLD_TERRAIN, (int)terrain1, TRUE);
			if(pStr)
				ASetString(43, 307, pStr, (uchar *)pDestCel, pDestCel->width, NULL);
			#ifndef NDEBUG
			else
			{
				char	pError[80];

				sprintf(pError, "DrawStatusBox() null #2: x%d y%d t%d\n", vPort.targX,
					vPort.targY, (int)terrain1);
				APanic(pError);
			}
			#endif

			ClearHPGuage();
  			DrawAttDefMovGuages(-pTerrain->attackMod, 0, pTerrain->defenseMod, 0,
				-pTerrain->moveRate, 0, TRUE);
			DrawBanner(SIDE0);

			// check for an item sitting at target location
			pItem = itemMgr.CheckItem(vPort.targX, vPort.targY);
			if(pItem)
				DrawItemPortrait(pItem->type);
			else DrawItemPortrait(NO_ITEM);
			break;

		case ITEM:		// probably add this code to case TERRAIN: since we still want terrain stats displayed
			break;

		case BUILDING:		// also portals, etc
			pSpace = (MapSpace *)DerefSerial(vPort.targSerial);
			terrain1 = (terrainType)map.GetTerrainNum(vPort.targX, vPort.targY);
			pTerrain = &pTileLib->terrains[(int)terrain1];
			pMono->Out("MapSpace(): serNum=%d side=%d hot=%d ",
				pSpace->serialNum, pSpace->h.owner, pSpace->h.hotSpot? 1: 0);
			pMono->Out("Port=%d Func=%s\n", pTerrain->portraitNum, &szFuncNames[pSpace->h.func][0]);

			// set pic for UNSELECTED state of button to anim X, cel #1
			pButton->SetButtonRes(FALSE, RES_ANIM, pTerrain->portraitNum, 1);
  			if(pSpace->h.owner == NULL)
				pMono->Out("World::DrawStatusBox() - BUILDING g%d h.owner == NULL!\n", pSpace->gSelf);

			pButton->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[pSpace->h.owner]);

			pStr = squib1.Load(SQUIB_WORLD_TERRAIN, (int)terrain1, TRUE);
			if(pStr)
				ASetString(43, 307, pStr, (uchar *)pDestCel, pDestCel->width, NULL);
			#ifndef NDEBUG
			else APanic("World::DrawStatusBox() null string #3\n");
			#endif
			if(pSpace->h.maxHP && !pSpace->IsFoundation())
			{
				ClearAttDefMovGuages();
				DrawHPGuage(pSpace->h.currHP, pSpace->h.maxHP, pSpace->h.owner);
			}
			else
			{
				ClearHPGuage();
				DrawAttDefMovGuages(-pTerrain->attackMod, 0, pTerrain->defenseMod, 0,
					-pTerrain->moveRate, 0, TRUE);
			}
			DrawBanner(pSpace->h.owner);

			// check for an item sitting at target location (portals && startLocs are also "buildings")
			pItem = itemMgr.CheckItem(vPort.targX, vPort.targY);
			if(pItem)
				DrawItemPortrait(pItem->type);
			else DrawItemPortrait(NO_ITEM);
			break;
	}
	pButton->Select(FALSE);		// force redraw

	SetVerbButtons(vPort.targType, vPort.targSerial);

	// editor mode - extra info
	#ifdef ENABLE_EDITOR
	if(pBam->fMapEdit)
	{
		uint oldFontNum = pFontMgr->curFontNum;
		pFontMgr->SetRes(1009);

		// overwrite previous target name
//		pSrcCel = pRes->GetHeader(1);
//		CopyCel(pDestCel, 103, 268, gCoverUpAni, 1, FALSE);	// overwrite any previous text

		pGroup = map.GetTerrainType(vPort.CursorX, vPort.CursorY);
		tileNum = map.GetTileNum(vPort.CursorX, vPort.CursorY);
		pTile = ADerefAs(Tile, pWorld->map.TileLib.gTile[tileNum]);
		sprintf(string1, "X%02d Y%02d Ti%03d", vPort.CursorX, vPort.CursorY, tileNum);
		SetFontColors(CI_SKIP, CI_WHITE, CI_BLACK);
		ASetString(103, 307, string1, (uchar *)pDestCel, pDestCel->width, NULL);
		rTemp1.Set(103, 307, 320, 323);
		AUpdateRect(&rTemp1);

		sprintf(string1, "Ti%03d Sw%03d An%03d Te%03d %s", tileNum, pTile->h.swapTile, pTile->h.animTile,
			pTile->h.GroupNum, pTerrain->szName);
		MsgBoxAdd(string1, vPort.CursorX, vPort.CursorY);
		sprintf(string1, "Mv%d At%d Ra%d Df%d Dm%d Co%03d Bu%c Po%d", pTerrain->moveRate, pTerrain->attackMod,
			pTerrain->attackRangeMod, pTerrain->defenseMod, pTerrain->damageVal, pTerrain->miniColor,
			pTerrain->burnable? 'Y': 'N', pTerrain->portrait);
		MsgBoxAdd(string1, vPort.CursorX, vPort.CursorY);
		MsgBoxDraw();
		pFontMgr->SetRes(oldFontNum);
	}
	#endif
}

int
World::CalcSoundVolAndPan(int soundX, int soundY, int16 *pNewVol, int16 *pNewPan)
{
	int distance, distanceScale, panLeft, panRight, newVol;

	panLeft = 100;
	panRight = 100;
	if(soundX == 999)
	{
		// max volume always
		distance = 0;
	}
	else if(!vPort.InViewPort(soundX, soundY))
	{
		// faded out of existance
		*pNewVol = 0;
		return(0);
	}
	else
	{
		if(soundX < vPort.ViewX)
		{
			distance = vPort.ViewX - soundX;
			panRight = 100 - ((distance * 100) / WORLD_WIDTH) * 7;
			panRight = AMax(panRight, 0);
			panRight = AMin(panRight, 100);
		}
		else if(soundX > vPort.ViewX2)
		{
			distance = soundX - vPort.ViewX2;
			panLeft = 100 - ((distance * 100) / WORLD_WIDTH) * 7;
			panLeft = AMax(panLeft, 0);
			panLeft = AMin(panLeft, 100);
		}
		else
		{
			distance = 0;
		}

		if(soundY < vPort.ViewY)
		{
			distance = AMax(distance, vPort.ViewY - soundY);
		}
		else if(soundY > vPort.ViewY2)
		{
			distance = AMax(distance, soundY - vPort.ViewY2);
		}
	}
//	pMono->Out("CalcVol:: pan==%d/%d\n", panLeft, panRight);
	*pNewPan = (int16)MkPan(panLeft, panRight);

	// distance == what % of total possible distance?
	distanceScale = (distance * 100) / AMax(WORLD_HEIGHT, WORLD_WIDTH);

	// reduce maxvol by that %
	newVol = (int16)(MAX_VOLUME - (MAX_VOLUME * distanceScale) / 15);
	if(newVol < 0)
		newVol = 0;
	*pNewVol = (int16)newVol;
	return(newVol);
}

void
World::UpdateAllSoundVolumes(void)
{
	int16		newVolume, newPan;
	int		loop1, soundX, soundY;
	int32		*pSoundPos;
	TSound	*pSound1;

	pSound1 = &pBam->sounds[0];
	pSoundPos = &soundPos[0][0];

	for(loop1 = 0;	loop1 < MAX_DIGI_SOUNDS; loop1++, pSound1++, pSoundPos += 2)
		if(pSound1->IsPlaying())
		{
			soundX = *pSoundPos;
			if(soundX == 999)	// no volume scaling
				continue;

			soundY = *(pSoundPos + 1);
			CalcSoundVolAndPan(soundX, soundY, &newVolume, &newPan);
//			pMono->Out("UpdateVol:: PanLeft/Right%d/%d panPos%x vol%d\n", panLeft, panRight, panPos, (int)newVolume);
			if(!newVolume && pSound1->loopCount == 1)
			{
				// non-looping and faded beyond hearing - kill the sound for
				// optimization purposes
				pSound1->Stop();
			}
			else
			{
				pSound1->SetVolume(newVolume, newPan);
			}
		}
}

// make a game message announcement
bool
World::Announce(int tileX, int tileY, int squibNum1, int cel1, int squibNum2, int cel2,
	int squibNum3, int cel3)
{
	char		pBuffer[80];
	
	pBam->BuildString(pBuffer, squibNum1, cel1, squibNum2, cel2, squibNum3, cel3);
	MsgBoxAdd(pBuffer, tileX, tileY);
	MsgBoxDraw();

	pBam->LaunchVoice(squibNum1, cel1, squibNum2, cel2, squibNum3, cel3);
	return(TRUE);
}

// find an avail TSound and start play
bool
World::LaunchSound(int soundNum, int xLoc, int yLoc, bool fLoop, int volPercent)
{
	int16		loop1, volume, panPos;
	grip		gTSound;
	TSound	*pTSound;

	BamDebug.Out("LS(%d,%d,%d,%d)", soundNum, xLoc, yLoc, (int)fLoop);
	//	pMono->Out("LaunchSound(%d, %d): ", soundNum, (int)fLoop);

	if(xLoc == 999 || yLoc == 999)	// default to center of vPort
	{
		volume = MAX_VOLUME;
		panPos = (int16)MkPan(100, 100);
	}
	else
	{
		CalcSoundVolAndPan(xLoc, yLoc, &volume, &panPos);
	}

	if(volPercent != 100)
	{
		volume = (volume * volPercent) / 100;
	}

	// if non-looping and beyond hearing range..
	if(!fLoop && !volume)
	{
		BamDebug.Out("LSd ");
		return(TRUE);
	}

	if(pSoundMgr->NumberDigiPlaying() >= MAX_DIGI_SOUNDS)	// if all TSounds in use
	{
//		pMono->Out("recycling\n");

		// find oldest override-able sound

		gTSound = pSoundMgr->OldestDigiPlaying();
		while(gTSound)
		{
			// got a grip, now find that slot.
			for(loop1 = 0, pTSound = &pBam->sounds[0];
				pTSound->gSelf != gTSound && loop1 < MAX_DIGI_SOUNDS;
				loop1++, pTSound++);
			if(loop1 < MAX_DIGI_SOUNDS && pTSound->loopCount > 0)
			{
				// found it, and it's not permanent (so we don't mind killing it)
				break;
			}

			gTSound = pSoundMgr->NextOldestDigiPlaying();
			if(!gTSound)
			{
				pMono->Out("World::LaunchSound() - error, pSoundMgr->OldestDigiPlaying()\nreturned unknown grip.\n");
				return(FALSE);
			}
		}
	}
	else	// find first avail TSound
	{
		for(loop1 = 0; loop1 < MAX_DIGI_SOUNDS && pBam->sounds[loop1].IsPlaying(); loop1++);
		if(loop1 == MAX_DIGI_SOUNDS)
		{
			APanic("World::LaunchSound() - NumberDigiPlayer() suspected wrong\n");
		}
//		else pMono->Out("assigning %d\n", loop1);
	}

	BamDebug.Out("LS1(%d,%d,%d)", soundNum, volume, panPos);
//	panPos = (xLoc < vPort.ViewX)? DIGI_PAN_LEFT: DIGI_PAN_CENTER;
//	panPos = (xLoc > vPort.ViewX2)? DIGI_PAN_RIGHT: DIGI_PAN_CENTER;
	pBam->sounds[loop1].Play((uint16)soundNum, (int16)volume, panPos, NULL, fLoop? 0: 1);
	// remember x/y of sound, in case viewport (listener pos) changes
	soundPos[loop1][0] = xLoc;
	soundPos[loop1][1] = yLoc;
	BamDebug.Out("LSd");
	return(TRUE);
}

void
World::FreeSerialNum(uint32 serialNum)
{
	if(serialNum >= MAX_SERIAL_NUMS)
	{
		pMono->Out("World::FreeSerialNum(%d) - invalid\n");
		return;
	}
	if(!serialNum)
		return;

	if(serialNums[serialNum][0] == NOTARGET)
	{
		pMono->Out("World::FreeSerialNum(%d) - not presently allocated\n");
		return;
	}
	serialNums[serialNum][0] = NOTARGET;
	serialNums[serialNum][1] = NULL;
}

void *
World::DerefSerial(uint32 serialNum, TargetType *targ)
{
	void	*pTarget;
	grip	gTarget;
	TargetType	targType;
	uint32		serial2;

	if(serialNum >= MAX_SERIAL_NUMS)
		return(NULL);
	if(serialNums[serialNum][0] == NOTARGET)
		return(NULL);
	gTarget = serialNums[serialNum][1];
	if(pMemMgr->CheckGrip(gTarget) != GRIP_VALID)
	{
		pMono->Out("World::DerefSerial(%d) - error, grip %d not allocated\n", serialNum, gTarget);
		FreeSerialNum(serialNum);
		return(NULL);
	}
	targType = (TargetType)serialNums[serialNum][0];
	pTarget = ADerefAs(void, gTarget);
	switch(targType)
	{
		case UNIT:
			serial2 = ((Unit *)pTarget)->serialNum;
			if(serial2 != serialNum)
			{
				pMono->Out("World::DerefSerial(%d) - unit contains serial %d!\n", serialNum, serial2);
				return(NULL);
			}
			break;

		case BUILDING:
			serial2 = ((MapSpace *)pTarget)->serialNum;
			if(serial2 != serialNum)
			{
				pMono->Out("World::DerefSerial(%d) - structure contains serial %d!\n", serialNum, serial2);
				return(NULL);
			}
			break;
	}

	if(targ)
		*targ = targType;
	return(pTarget);
}

uint32
World::GetNextSerialNum(TargetType targType, grip gThing)
{
	int	loop1;

	for(loop1 = 0; loop1 < MAX_SERIAL_NUMS; loop1++, nextSerialNum = (nextSerialNum + 1) % MAX_SERIAL_NUMS)
	{
		if(nextSerialNum == 0)
		{
			// skip over 0
			nextSerialNum++;
		}

		if(serialNums[nextSerialNum][0] != NOTARGET)
			continue;
		serialNums[nextSerialNum][0] = targType;
		serialNums[nextSerialNum][1] = gThing;
		return(nextSerialNum++);
	}
	BamDebug.CloseFile();
	APanic("World::GetNextSerialNum() - error, out of serial numbers!\n");
	return(NULL);
}

void *
World::ValidateTarget(uint32 serialNum, TargetType *targ, int targX, int targY)
{
	Unit		*pUnit;
	MapSpace	*pBldg;
	TargetType	targType = NOTARGET;
	void		*pTarget = NULL;

	if(serialNum)
	{
		pTarget = DerefSerial(serialNum, &targType);
		if(!pTarget)
		{
			// entity has been de-allocated
			targType = NOTARGET;
		}
	}
	else targType = TERRAIN;

	switch(targType)
	{
		case UNIT:
			pUnit = (Unit *)pTarget;
			if(pUnit->primaryAction == DIE || pUnit->primaryAction == DISBAND ||
			 pUnit->primaryAction == TRANSFORM_TO_BLDG)
			{
				pTarget = NULL;
			}
			if(unitLib.GetUnitGrip(pUnit->tileX, pUnit->tileY) != pUnit->gSelf)
			{
				if(pUnit->type == TORTOISE)
					;	// exception to the rule
				else pTarget = NULL;
			}
			break;

		case BUILDING:
			pBldg = (MapSpace *)pTarget;
			if(pBldg->h.currHP < 1)
			{
				pTarget = NULL;
			}
			break;

		case TERRAIN:
			if(targX < 0 || targX >= WORLD_WIDTH || targY < 0 || targY >= WORLD_HEIGHT)
			{
				pTarget = NULL;
			}
			else pTarget = (void *)TRUE;
			break;
	}
	if(!pTarget)
		targType = NOTARGET;
	if(targ)
		*targ = targType;
	return(pTarget);
}


bool
World::Save(uint16 state, FILE *pFile)
{
	grip			gAni;
	int			loop1;

	switch(state)
	{
		case DURING_SAVE:
			fwrite(&worldDataStart, 1, (int)&worldDataEnd -
				(int)&worldDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&worldDataStart, 1, (int)&worldDataEnd -
				(int)&worldDataStart, pFile);
			break;

		case AFTER_RESTORE:
			pTileLib = (TileLibRes *)AGetResData(ALoadDebug(__FILE__, __LINE__, RES_TILELIB, tileResNum));
			// fixup all the clut pointers
			
			for(loop1 = 0; loop1 <= vPort.highestAnim; loop1++)
			{
				gAni = vPort.gAnims[loop1];
				if(!gAni)
					continue;

				ADerefAs(BAM_Ani, gAni)->scrim.Save(state);
			}

			ADerefAs(Guy, vPort.gViewPortCursor)->scrim.Save(state);

			if(fPoolFlip)
			{
				pCurrPool = &actPool2;
				pAltPool = &actPool1;
			}
			else
			{
				pCurrPool = &actPool1;
				pAltPool = &actPool2;
			}

			// make sure this is active
			AllowMouseDragging(TRUE, TRUE);

			break;
	}
	
	return FALSE;
}
