#include "api.hpp"
#include "apievt.hpp"
#include "apires.hpp"
#include "Context.hpp"
#include "dialog.hpp"
#include "Eventmgr.hpp"
#include "FontMgr.hpp"
#include "GraphMgr.hpp"
#include "manager.hpp"
#include "mem.hpp"
#include "mem.h"
#include "memmgr.hpp"
#include "mono.hpp"
#include "mouse.hpp"
#include "objmgr.hpp"
#include "Palette.hpp"
#include "periodic.hpp"
#include "resmgr.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include "bam.hpp"
#include	"tilelib2.hpp"
#include "tilelib.hpp"
#include "mapedit.hpp"
#include "smartdg.hpp"

extern grip			gCEditor;
extern CEditor		*pCEditor;
extern TileLibrary *pTileLib;
extern MapBase		*pMap;
extern char 		szFuncNames[][20];

// CEditTile - context for editing current tile specs
CEditTile::CEditTile(uint16 TileNbr, uint16 SpaceNbr, uint16 MapPosNbr, uint16 xPosNbr, uint16 yPosNbr) : currSpace(FUNC_MISC)
{				 
	TileNum = TileNbr;
	MapPos = MapPosNbr;
	xPos = xPosNbr;
	yPos = yPosNbr;

	SpaceNum = SpaceNbr;
	pMono->Out("CEditTile::CEditTile() - %d free grips\n", CountFreeGrips());
	ClearGrips();
}

CEditTile::~CEditTile()
{
}

void
CEditTile::ClearGrips(void)
{
	gDialog = NULL;
	gNewGroup = NULL;
	gGroupNum = NULL;
	gGroupBurn = NULL;
	gNewSpace = NULL;
	gSpaceNum = NULL;
	gSpaceX = NULL;
	gSpaceY = NULL;
	gSpaceFunc = NULL;
	gSpaceHot = NULL;
	gItem = NULL;
	gRandomItem = NULL;
	gSpecialItem = NULL;
	gHotX = NULL;
	gHotY = NULL;
	gDestX = NULL;
	gDestY = NULL;
	gOwner = NULL;
	gMiniColor = NULL;
}

void
CEditTile::SetupLabels(void)
{
	SmartDialog	*pDialog = ADerefAs(SmartDialog, gDialog);

	SetColors(CI_BLACK, CI_WHITE, CI_WHITE);

	pDialog->AddSmartLabel(240, 30, "Type");
	pDialog->AddSmartLabel(10, 45, "Name");
	pDialog->AddSmartLabel(190, 45, "Clr");
	pDialog->AddSmartLabel(245, 45, "Por");
	pDialog->AddSmartLabel(5, 60, "Mov       Def      Att      Rng      Dmg");
	pDialog->AddSmartLabel(225, 75, "Burn");
	SetColors(CI_BLACK, CI_WHITE, CI_WHITE);

	pDialog->AddSmartLabel(10, 75, "Clust        Func");
//	pDialog->AddSmartLabel(80, 88, "X             Y");
	pDialog->AddSmartLabel(15, 101, "Hot        X        Y");
	pDialog->AddSmartLabel(10, 127, " Item          Random       StartLoc");
	pDialog->AddSmartLabel(180, 101, "HP");
	pDialog->AddSmartLabel(10, 153, "DestX           DestY           Owner");
	pDialog->AddSmartLabel(60, 30, "Swap      Anim      Delay");
}

void
CEditTile::SetupGroup(bool fInit)
{
	SmartBox		*pBox;
	SmartDialog	*pDialog;

	pDialog = ADerefAs(SmartDialog, gDialog);

	if(fInit)
	{	
		sprintf(szGroupNumText, "%d", currTile.h.GroupNum);
		sprintf(szGroupDescr, "%s", currGroup.h.descr);
		sprintf(szGroupMove, "%d", currGroup.h.moveRate);
		sprintf(szGroupDef, "%d", currGroup.h.defenseMod);
		sprintf(szGroupAtt, "%d", currGroup.h.attackMod);
		sprintf(szGroupRng, "%d", currGroup.h.attackRangeMod);
		sprintf(szGroupDmg, "%d", currGroup.h.damageVal);
		sprintf(szGroupPortrait, "%d", currGroup.h.portrait);
	}

	SetColors(CI_WHITE, CI_BLACK, CI_BLACK);

	gGroupNum = pDialog->AddSmartBox(265, 30, szGroupNumText, 3);
	pBox = ADerefAs(SmartBox, gGroupNum);
	pBox->SetupReplies(gSelf, TEXT_CHANGED);
	gBoxes[BoxCount++] = gGroupNum;

	gBoxes[BoxCount++] = pDialog->AddSmartBox(45, 45, szGroupDescr, 22);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(265, 45, szGroupPortrait, 4);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(30, 60, szGroupMove, 3);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(85, 60, szGroupDef, 3);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(140, 60, szGroupAtt, 3);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(195, 60, szGroupRng, 3);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(255, 60, szGroupDmg, 3);
	SetupGroupColor();
	SetupGroupBurnable();
}

void
CEditTile::SetupGroupBurnable(void)
{
	SmartButton	*pButton;
	SmartDialog *pDialog = ADerefAs(SmartDialog, gDialog);
	uint32		x, y, pri;

	if(gGroupBurn)
	{
		// necessary because Dialog->Activate() will have changed X & Y
		pButton = ADerefAs(SmartButton, gGroupBurn);
		x = pButton->scrim.x;
		y = pButton->scrim.y;
		pri = pButton->scrim.priority;	// if not being run from Setup()
		ADelete(gGroupBurn);
	}
	else
	{
		x = 255;
		y = 75;
		pri = 100;	// doesnt really matter in this case
	}

	SetColors(CI_WHITE, CI_BLACK, CI_BLACK);
	gGroupBurn = pDialog->AddSmartButton(x, y, currGroup.h.burnable? "YES": "no", FALSE, gSelf, SELECTED);
	gBoxes[BoxCount++] = gGroupBurn;
	pButton = ADerefAs(SmartButton, gGroupBurn);
	pButton->SetPri(pri);
}

void
CEditTile::SetupGroupColor(void)
{
	SmartBox		*pBox;
	SmartDialog	*pDialog;
//	uint32		x, y, pri;
	//oldFore = pFontMgr->colors[FNT_FORE_COLOR], oldBack = pFontMgr->colors[FNT_BACK_COLOR];

	pDialog = ADerefAs(SmartDialog, gDialog);

	if(currGroup.h.miniColor != CI_BLACK)
	{
		SetColors(currGroup.h.miniColor, CI_BLACK, CI_BLACK);
	}
	else
	{
		SetColors(currGroup.h.miniColor, CI_WHITE, CI_WHITE);
	}

	sprintf(szMiniColor, "%d", currGroup.h.miniColor);

	if(!gMiniColor)
	{
		gMiniColor = pDialog->AddSmartBox(210, 45, szMiniColor, 4);
		gBoxes[BoxCount++] = gMiniColor;
		pBox = ADerefAs(SmartBox, gMiniColor);
		pBox->SetupReplies(gSelf, TEXT_CHANGED);
//		pBox->SetPri(100);
	}
	else
	{
		pBox = ADerefAs(SmartBox, gMiniColor);
		pBox->SetCurrentString(szMiniColor);
	}
//	pFontMgr->colors[FNT_FORE_COLOR] = oldFore;
//	pFontMgr->colors[FNT_BACK_COLOR] = oldBack;
}

void
CEditTile::SetupSpace(void)
{
	SmartBox		*pBox;
	SmartDialog	*pDialog;

	pDialog = ADerefAs(SmartDialog, gDialog);

	SetColors(CI_WHITE, CI_BLACK, CI_BLACK);

	sprintf(szSpaceNum, "%d", SpaceNum);
	gSpaceNum = pDialog->AddSmartBox(45, 75, szSpaceNum, 3);
	gBoxes[BoxCount++] = gSpaceNum;
	pBox = ADerefAs(SmartBox, gSpaceNum);
	pBox->SetupReplies(gSelf, TEXT_CHANGED);

	if(!SpaceNum)
	  return;

	SetupSpaceFunc();
	SetupSpaceHot();
	SetupRandomItem();
	SetupSpecialItem();

	sprintf(szSpaceHP, "%d", currSpace.h.maxHP);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(195, 101, szSpaceHP, 5);

/*	sprintf(szSpaceX, "%d", currSpace.h.xPos);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(90, 88, szSpaceX, 3);

	sprintf(szSpaceY, "%d", currSpace.h.yPos);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(140, 88, szSpaceY, 3);
*/		
	sprintf(szHotX, "%d", currSpace.h.hotX);
	gHotX = pDialog->AddSmartBox(90, 101, szHotX, 3);
	gBoxes[BoxCount++] = gHotX;
	sprintf(szHotY, "%d", currSpace.h.hotY);
	gHotY = pDialog->AddSmartBox(140, 101, szHotY, 3);
	gBoxes[BoxCount++] = gHotY;

	sprintf(szItem, "%d", currSpace.h.presetItem);
	gItem = pDialog->AddSmartBox(45, 127, szItem, 3);
	gBoxes[BoxCount++] = gItem;

	sprintf(szDestX, "%d", currSpace.h.destX);
	gDestX = pDialog->AddSmartBox(45, 153, szDestX, 4);
	gBoxes[BoxCount++] = gDestX;

	sprintf(szDestY, "%d", currSpace.h.destY);
	gDestY = pDialog->AddSmartBox(140, 153, szDestY, 4);
	gBoxes[BoxCount++] = gDestY;

	sprintf(szOwner, "%d", currSpace.h.owner);
	gOwner = pDialog->AddSmartBox(235, 153, szOwner, 2);
	gBoxes[BoxCount++] = gOwner;

	SetColors(CI_WHITE, CI_BLACK, CI_BLACK);
}

void
CEditTile::SetupSpaceHot(void)
{
	SmartButton	*pButton;
	SmartDialog *pDialog = ADerefAs(SmartDialog, gDialog);
	uint32		x, y, pri;

	if(gSpaceHot)
	{
		// necessary because Dialog->Activate() will have changed X & Y
		pButton = ADerefAs(SmartButton, gSpaceHot);
		x = pButton->scrim.x;
		y = pButton->scrim.y;
		pri = pButton->scrim.priority;	// if not being run from Setup()
		ADelete(gSpaceHot);
	}
	else
	{
		x = 45;
		y = 101;
		pri = 100;	// doesnt really matter in this case
	}

	gSpaceHot = pDialog->AddSmartButton(x, y, currSpace.h.hotSpot? "YES": "no", FALSE, gSelf, SELECTED);
	gBoxes[BoxCount++] = gSpaceHot;
	pButton = ADerefAs(SmartButton, gSpaceHot);
	pButton->SetPri(pri);
}

void
CEditTile::SetupSpaceFunc(void)
{
	SmartButton	*pButton;
	SmartDialog *pDialog = ADerefAs(SmartDialog, gDialog);
	uint32		x, y, pri;

	if(gSpaceFunc)
	{
		// necessary because Dialog->Activate() will have changed X & Y
		pButton = ADerefAs(SmartButton, gSpaceFunc);
		x = pButton->scrim.x;
		y = pButton->scrim.y;
		pri = pButton->scrim.priority;	// if not being run from Setup()
		ADelete(gSpaceFunc);
	}
	else
	{
		x = 115;
		y = 75;
		pri = 100;	// doesnt really matter in this case
	}

	sprintf(szSpaceFunc, "%d:%s", currSpace.h.func, &szFuncNames[currSpace.h.func][0]);
	gSpaceFunc = pDialog->AddSmartButton(x, y, szSpaceFunc, FALSE, gSelf, SELECTED);
	pButton = ADerefAs(SmartButton, gSpaceFunc);
	pButton->SetPri(pri);
	gBoxes[BoxCount++] = gSpaceFunc;
}

void
CEditTile::SetupRandomItem(void)
{
	char			szString1[10];
	SmartButton	*pButton;
	SmartDialog *pDialog = ADerefAs(SmartDialog, gDialog);
	uint32		x, y, pri;

	if(gRandomItem)
	{
		// necessary because Dialog->Activate() will have changed X & Y
		pButton = ADerefAs(SmartButton, gRandomItem);
		x = pButton->scrim.x;
		y = pButton->scrim.y;
		pri = pButton->scrim.priority;	// if not being run from Setup()
		ADelete(gRandomItem);
	}
	else
	{
		x = 140;
		y = 127;
		pri = 100;	// doesnt really matter in this case
	}

	sprintf(szString1, "%s", currSpace.h.fPickRandomItem? "YES": "no");
	gRandomItem = pDialog->AddSmartButton(x, y, szString1, FALSE, gSelf, SELECTED);
	pButton = ADerefAs(SmartButton, gRandomItem);
	pButton->SetPri(pri);	// wont matter if run from Setup() cuz Dialog->Activate() will override it
	gBoxes[BoxCount++] = gRandomItem;
}

void
CEditTile::SetupSpecialItem(void)
{
	char			szString1[10];
	SmartButton	*pButton;
	SmartDialog *pDialog = ADerefAs(SmartDialog, gDialog);
	uint32		x, y, pri;

	if(gSpecialItem)
	{
		// necessary because Dialog->Activate() will have changed X & Y
		pButton = ADerefAs(SmartButton, gSpecialItem);
		x = pButton->scrim.x;
		y = pButton->scrim.y;
		pri = pButton->scrim.priority;	// if not being run from Setup()
		ADelete(gSpecialItem);
	}
	else
	{
		x = 230;
		y = 127;
		pri = 100;	// doesnt really matter in this case
	}

	sprintf(szString1, "%s", currSpace.h.fSpecialItemDrop? "YES": "no");
	gSpecialItem = pDialog->AddSmartButton(x, y, szString1, FALSE, gSelf, SELECTED);
	pButton = ADerefAs(SmartButton, gSpecialItem);
	pButton->SetPri(pri);	// wont matter if run from Setup() cuz Dialog->Activate() will override it
	gBoxes[BoxCount++] = gSpecialItem;
}

void
CEditTile::Setup(bool fSetupTile, bool fSetupGroup, bool fSetupSpace)
{
	uint32		loop1;
	grip			g;
	SmartBox		*pBox;
	SmartDialog	*pDialog;
	Window		*pWin;
	char			szString1[80];
	uchar			oldFore = pFontMgr->colors[FNT_FORE_COLOR], oldBack = pFontMgr->colors[FNT_BACK_COLOR];

	BoxCount = 0;
//	PROF_Reset();

	// grab fresh data from lists as necessary (like if GroupNum or SpaceNum are changed)
	if(fSetupTile)
		currTile.Copy(ADerefAs(Tile, pTileLib->gTile[TileNum]), sizeof(currTile));
	if(fSetupGroup)
		currGroup.Copy(ADerefAs(TileGroup, pTileLib->gTileGroup[currTile.h.GroupNum]), sizeof(currGroup));
	if(fSetupSpace && SpaceNum > 0)
		currSpace.Copy(ADerefAs(MapSpaceBase, pMap->gSpaces[SpaceNum]), sizeof(currSpace));


	// if dialog already exists, delete it first (group # must have changed)
	if(gDialog)
	{
		pDialog = ADerefAs(SmartDialog, gDialog);
		pDialog->SetupReplies(NULL, NULL);
		pDialog->Deactivate();
		ADelete(gDialog);
		ClearGrips();
	}

	if(pCEditor->gSaveWindow)
	{
		pWin = ADerefAs(Window, pCEditor->gSaveWindow);
		pWin->Show();
		TRACK_MEM("TileDialog"); pDialog = new SmartDialog(0, 0, pWin->gSelf, CI_WHITE, 2);
	}
	else
	{
		TRACK_MEM("TileDialog"); pDialog = new SmartDialog(0, 0, DG_DFT_WINDOW, CI_WHITE, 2);
	}
	gDialog = pDialog->gSelf;

	SetColors(CI_BLACK, CI_WHITE, CI_WHITE);

	sprintf(szString1, "Tile Editor - %d Types, %d Clusters", pTileLib->h.totalTileGroups, pMap->h.totalSpaces);
	g = pDialog->AddSmartLabel(30, 5, szString1);
	sprintf(szString1, "Tile %d", TileNum);
	g = pDialog->AddSmartLabel(10, 30, szString1);
  
	if(!pCEditor->gSaveWindow)
		SetupLabels();
	SetupGroup(fSetupGroup);
	SetupSpace();
	
	SetColors(CI_WHITE, CI_BLACK, CI_BLACK);

	if(fSetupTile)
	{
		sprintf(szSwapTileText, "%d", currTile.h.swapTile);
		sprintf(szGroupAnim, "%d", currTile.h.animTile);
		sprintf(szGroupAnimDelay, "%d", currTile.h.animDelay);
	}
	gBoxes[BoxCount++] = pDialog->AddSmartBox(85, 30, szSwapTileText, 4);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(145, 30, szGroupAnim, 4);
	gBoxes[BoxCount++] = pDialog->AddSmartBox(210, 30, szGroupAnimDelay, 3);

	gNewGroup = pDialog->AddSmartButton(10, 200, "NEW TYPE", FALSE, gSelf, SELECTED);

	gNewSpace = pDialog->AddSmartButton(100, 200, "NEW CLUST", FALSE, gSelf, SELECTED);

	pDialog->AddSmartButton(190, 200, "OK", TRUE, gSelf, SELECTED);

	pDialog->SetupReplies(gSelf, DESELECTED);	// notify this context when dialog is Deactivate()'ed
	pDialog->SetPos(5, 5);
	pDialog->Activate(M_MODAL, 500);	// cause default window to be defined, so Controls can base their pri on it
	pWin = ADerefAs(Window, pDialog->gWin);
	if(!pCEditor->gSaveWindowCel)	// if window hasnt been saved yet
	{
		Window		*pNewWin;
		pWin = ADerefAs(Window, pDialog->gWin);
		CelHeader	*pCel;

		// copy Dialog::window cel for re-use later, to avoid re-creation overhead
		TRACK_MEM("temp Cel");	pCEditor->gSaveWindowCel = ACreateCel(&pCEditor->rNumSaveWindowCel,
				0, 0, pWin->scrim.rect.Wide(), pWin->scrim.rect.High(), 1, CI_WHITE);
		pCel = (CelHeader *)pResMgr->GetResData(pCEditor->gSaveWindowCel);
		CopyCel(pCel, 0, 0, pWin->scrim.resType, pWin->scrim.resNum,
			pWin->scrim.cel, TRUE);	// copy Window::cel to temp for re-use later

		// we will use this window instead, next time
		TRACK_MEM("temp Window");	pNewWin = new Window(&pWin->scrim.rect, CI_WHITE, pWin->scrim.priority);
		pCEditor->gSaveWindow = pNewWin->gSelf;
		pNewWin->SetContext(pCEditor->gSelf);
		pNewWin->Setup();
		pNewWin->SetRes(RES_CEL, pCEditor->rNumSaveWindowCel);
		pNewWin->SetPos(10, 10);
		pNewWin->Hide();	// dont need it right now though
	}

	// boost pri of all boxes in case of label/box overlap
	for(loop1 = 0; loop1 < BoxCount; loop1++)
	{
		pBox = ADerefAs(SmartBox, gBoxes[loop1]);
		pBox->SetPri(pBox->scrim.priority + 1);
	}
	pGraphMgr->scrimList.fResortList = TRUE;
	BoxCount = 0;

	pWin = ADerefAs(Window, pDialog->gWin);
	pMouse->SetLimits(&pWin->scrim.rect);

	// make him #1 in the msg stream
	pContextMgr->lContexts.Move((void *)pDialog->gSelf, L_FRONT, NULL);

	msgMask |= E_MOUSE_UP;	// we also want these
	SetColors(CI_WHITE, CI_BLACK, CI_BLACK);
}

bool
CEditTile::HandleMsg(Message *pMsg)
{
	coord			mouseX, mouseY;
	uint16		GroupNbr, SpaceNbr;
	SmartDialog	*pDialog;
	Tile		*pTile;
	TileGroup	*pGroup;
	MapSpaceBase *pSpace; 
	bool		claimed;

	pMono->Out("HandleMsg(): gDialog==%d\n", gDialog);
	claimed = Context::HandleMsg(pMsg);
						  
	if(!claimed)
	{
		if(gDialog)
			pDialog = ADerefAs(SmartDialog, gDialog);
		switch(pMsg->type)
		{
			case MSG_NOTICE:
				switch(pMsg->notice.type)
				{
					case N_SETUP:
						pMono->Out("  case N_SETUP\n");
						Setup(TRUE, TRUE, TRUE);
						pMono->Out("    break;\n");
						return(TRUE);
					case N_TBUTTON:
						if((int)(pMsg->notice.param) == SELECTED)
						{
							pMono->Out("  case N_TBUTTON:Selected\n");
							if(pMsg->notice.gSource == gSpecialItem)
							{
								currSpace.h.fSpecialItemDrop = currSpace.h.fSpecialItemDrop? FALSE: TRUE;
								SetupSpecialItem();
							}
			  				else if(pMsg->notice.gSource == gGroupBurn)
							{
								currGroup.h.burnable = currGroup.h.burnable? FALSE: TRUE;
								SetupGroupBurnable();
							}
							else if(pMsg->notice.gSource == gRandomItem)
							{
								currSpace.h.fPickRandomItem = currSpace.h.fPickRandomItem? FALSE: TRUE;
								SetupRandomItem();
							}
							else if(pMsg->notice.gSource == gSpaceFunc)
							{
								currSpace.h.func = (currSpace.h.func + 1) % TOTALFUNCTIONS;
								SetupSpaceFunc();
							}		
							else if(pMsg->notice.gSource == gSpaceHot)
							{
								currSpace.h.hotSpot = currSpace.h.hotSpot? FALSE: TRUE;
								SetupSpaceHot();
							}
							else if(pMsg->notice.gSource == gNewSpace)
							{
								// set new space num
								SpaceNum = pMap->AddSpace(FUNC_MISC);
								pSpace = ADerefAs(MapSpaceBase, pMap->gSpaces[SpaceNum]);
								currSpace.Copy(pSpace, sizeof(currSpace));

								currSpace.h.xPos = xPos;
								currSpace.h.yPos = yPos;
								currSpace.h.hotX = xPos;
								currSpace.h.hotY = yPos;
								Setup(FALSE, FALSE, FALSE);
							}
							else if(pMsg->notice.gSource == gNewGroup)
							{
								// set new group num
								currTile.h.GroupNum = pTileLib->AddGroup();
								pGroup = ADerefAs(TileGroup, pTileLib->gTileGroup[currTile.h.GroupNum]);
								currGroup.Copy(pGroup, sizeof(currGroup));
								Setup(FALSE, TRUE, FALSE);
							}
						}
						return(TRUE);
					case N_TBOX:		// news from one of our textboxes
						if((int)(pMsg->notice.param) == TEXT_CHANGED)
						{
							pMono->Out("  case N_TBOX:TEXT_CHANGED\n");
							if(pMsg->notice.gSource == gMiniColor)
							{
								currGroup.h.miniColor = atoi(szMiniColor);
								SetupGroupColor();								
							}
							else if(pMsg->notice.gSource == gGroupNum)
							{
								GroupNbr = (uint16)atoi(szGroupNumText);
								if(GroupNbr >= pTileLib->h.totalTileGroups)
									currTile.h.GroupNum = (uint16) (pTileLib->h.totalTileGroups - 1);
								else currTile.h.GroupNum = GroupNbr;
								pGroup = ADerefAs(TileGroup, pTileLib->gTileGroup[currTile.h.GroupNum]);
								currGroup.Copy(pGroup, sizeof(currGroup));
								Setup(FALSE, TRUE, FALSE);
							}
							else if(pMsg->notice.gSource == gSpaceNum)
							{
								// space # changed
								SpaceNbr = (uint16)atoi(szSpaceNum);
								if(SpaceNbr >= pMap->h.totalSpaces)
									SpaceNum = pMap->h.totalSpaces - 1;
								else SpaceNum = SpaceNbr;

								// if Space# valid, get info for that space# into temp
								if(SpaceNum > 0)
								{
									pSpace = ADerefAs(MapSpaceBase, pMap->gSpaces[SpaceNum]);
									currSpace.Copy(pSpace, sizeof(currSpace));
								}
								Setup(FALSE, FALSE, TRUE);
							}
						}
						pMono->Out("    break;\n");
						return(TRUE);

					case N_DIALOG:	// news from a SmartDialog
						pMono->Out("case N_DIALOG: ");
						if((int)pMsg->notice.param == DESELECTED)
						{
							pMono->Out("notice.param == DESELECTED; ");
							if(pMsg->notice.gSource == gDialog && gDialog)
							{
								pMono->Out("gSource==gDialog; ");
								pMap->CheckSpaces();
					
								// copy new data from SmartBox strings back into temp Tile
								currTile.h.GroupNum = (uint16)atoi(szGroupNumText);
								currTile.h.swapTile = (uint16)atoi(szSwapTileText);
								currTile.h.animTile = (uint32)atoi(szGroupAnim);
								currTile.h.animDelay = (uint32)atoi(szGroupAnimDelay);

								pTile = ADerefAs(Tile, pTileLib->gTile[TileNum]);

								// and then copy from temp into perm
								pTile->Copy(&currTile, sizeof(currTile));
				
								// do same with temp Group, and temp Space
								memcpy(currGroup.h.descr, szGroupDescr, 24);
								currGroup.h.moveRate = (uint16)atoi(szGroupMove);
								currGroup.h.defenseMod = (int16)atoi(szGroupDef);
								currGroup.h.attackMod = (int16)atoi(szGroupAtt);
								currGroup.h.portrait = (uint32)atoi(szGroupPortrait);
								currGroup.h.attackRangeMod = (int16)atoi(szGroupRng);
								currGroup.h.damageVal = (int16)atoi(szGroupDmg);
								currGroup.h.miniColor = (uint32)atoi(szMiniColor);
								pGroup = ADerefAs(TileGroup, pTileLib->gTileGroup[currTile.h.GroupNum]);
								pGroup->Copy(&currGroup, sizeof(currGroup));
	
								SpaceNum = atoi(szSpaceNum);
								currSpace.h.func = atoi(szSpaceFunc);
								currSpace.h.xPos = atoi(szSpaceX);
								currSpace.h.yPos = atoi(szSpaceY);
								currSpace.h.presetItem = atoi(szItem);
								currSpace.h.hotX = atoi(szHotX);
								currSpace.h.hotY = atoi(szHotY);
								currSpace.h.destX = atoi(szDestX);
								currSpace.h.destY = atoi(szDestY);
								currSpace.h.owner = atoi(szOwner);
								currSpace.h.maxHP = atoi(szSpaceHP);

								if(!pMap->CheckSpaces)
									pMono->Out("Change occured!\n");

								pSpace = ADerefAs(MapSpaceBase, pMap->gSpaces[SpaceNum]);
								pSpace->Copy(&currSpace, sizeof(currSpace));
								// set MapSpace[] array to reflect current SpaceNum
								pMap->MapSpaces[MapPos] = (int16) SpaceNum;
								ShutDown();
							}
						}
						pMono->Out("\nbreak;\n\n");
						return(TRUE);
				}
				break;
			case MSG_EVENT:
				switch(pMsg->event.type)
				{
					case E_MOUSE_UP:
						mouseX = (coord)pMsg->event.x;
						mouseY = (coord)pMsg->event.y;
						break;			
					case E_MOUSE_DOWN:
						mouseX = pMsg->event.x;
						mouseY = (coord)pMsg->event.y;
						break;
					case E_KEY_DOWN:
						mouseX = (coord)pMsg->event.x;
						mouseY = (coord)pMsg->event.y;
						switch (pMsg->event.value)
						{
							case K_ESC:
								ShutDown();
								return(TRUE);
							case K_RETURN:
								break;
						}			
						break;
				}
				break;
		}
	}
	return(claimed);
}

void
CEditTile::ShutDown(void)
{
	SmartDialog	*pDialog;

	pMouse->SetLimits(&pMouse->screenLimits);

	if(gDialog)
	{
		pDialog = ADerefAs(SmartDialog, gDialog);
					
		// if dialog still active, de-activate it
		if(pDialog->fIsActive)
			pDialog->Deactivate();
		ADelete(gDialog);
		gDialog = NULL;
		//	pMono->Out("CEditTile - Dialog deactivated.\n");				
	}
	// notify main context that we're through
	APostNotice((notice_t)N_DIALOG, gCEditor, (void *)DESELECTED, gSelf);
}

