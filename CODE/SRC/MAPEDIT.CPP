//
// Mapedit.cpp
//
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <ctype.h>
#include <mem.h>
#include <string.h>
#include <io.h>
#include <conio.h>
#include <bios.h>

#include "api.hpp"
#include "apievt.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "Context.hpp"
#include "eventmgr.hpp"
#include "fontmgr.hpp"
#include "graphmgr.hpp"
#include "manager.hpp"
#include "mem.hpp"
#include "memmgr.hpp"
#include "mono.hpp"
#include "mouseint.hpp"
#include "objmgr.hpp"
#include "Palette.hpp"
#include "periodic.hpp"
#include "resmgr.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include "alldefs.hpp"
//#include "bam.hpp"
#include "bamfunc2.hpp"
#include "bam_dg.hpp"
#include "tilelib.hpp"
#include "tilelib2.hpp"
#include "tiles.hpp"
#include "mapedit.hpp"
#include "viewport.hpp"

#define RIGHT_MAP_BORDER        (SCREEN_WIDTH - TILE_WIDTH - 60)

// global pointer for DESTROY_MGR
void *pDestMgr;
// This NULLs the manager pointer before calling (to avoid problems)
#define	DESTROY_MGR_CAREFUL(type, mgr)	if(mgr) {pDestMgr = (mgr); (mgr) = NULL; \
			delete ((type *) pDestMgr); }
// This doesn't NULL.  Dangerous, but required for a few managers
#define	DESTROY_MGR(mgr)	if(mgr) { delete (mgr); (mgr) = NULL; }

// viewport dimensions, in tile spaces
grip			gCEditor;
CEditor		*pCEditor;
TileLibRes  *pTileLib;
MapBase		*pMap;
extern char szFuncNames[][20];
extern bool	fIsTextMode;
bool			fStripClusters;

// dummy vars so BAMFUNC2 will link in.
int storyLine;
int missionsDone;
int chooseSide;
int curPath;
char curName[140];
//BAM_Application*		pBam;

void main(int argc, char *argv[])
{
	Mouse			*pMouse1;
	GraphicsMgr	*pGraphMgr1;
	int			mapResNum, tileResNum;

	printf("\nBAM Map/Scenario/World Editor   Compiled: %s %s\n", __DATE__, __TIME__);
	printf("Vangelis, July '94.  (C)opyright Tachyon, 1994\n");

	fStripClusters = FALSE;

	if(argc < 2)
	{
		printf("Format: MAPEDIT <map#> [-NOCLUST]\n");
		printf("Ex.:		MAPEDIT 9110\n");
		return;
	}

	mapResNum = atoi(argv[1]);
	tileResNum = mapResNum - (mapResNum % 20);
	if(mapResNum % 20 != 10 && mapResNum % 20 != 12 && mapResNum % 20 != 15)
	{
		printf("Vas Error #1: invalid map number given\n");
		return;
	}
	if(argc > 2)
	{
		if(!strcmpi(argv[2], "-NOCLUST"))
		{
			fStripClusters = TRUE;
		}
	}

	/*	sprintf(string1, "%s.TIL", libname);
	if(access(string1, F_OK) != 0)
	{
		printf("Warning: Unable to open tile library \"%s\".  Creating empty one.\n", string1);
		sleep(1);
	}

	sprintf(string1, "%s.SCE", libname);
	if(access(string1, F_OK) != 0)
	{
		printf("Warning: Unable to open map file %s.  Creating empty one.\n", string1);
		sleep(1);
	}*/

	TRACK_MEM("Mono"); new Mono;

	TRACK_MEM("EventMgr"); new EventMgr;

	TRACK_MEM("ContextMgr"); new ContextMgr;
	pContextMgr->msgMask |= E_MOUSE_UP;

	TRACK_MEM("ResourceMgr");	new ResourceMgr(TRUE);

	TRACK_MEM("GraphMgr");	pGraphMgr = new GraphicsMgr(MODEX_320X400);
	pGraphMgr1 = pGraphMgr;
	
	TRACK_MEM("FontMgr");	pFontMgr = new FontMgr;
	pFontMgr->SetRes(1009);

	TRACK_MEM("Mouse");	new MouseInt;
	pMouse->Init(0,0,SCREEN_WIDTH - 1,SCREEN_HEIGHT - 1);
	pMouse->SetRes(RES_ANIM, 9000, 1);
	pMouse->SetPos(20,20);
	pMouse->Show();
	pMouse1 = pMouse;

	//	create our context
	TRACK_MEM("CEditor");	pCEditor = new CEditor;
	gCEditor = pCEditor->gSelf;
	pCEditor->msgMask |= E_MOUSE_UP;	// we also want these
	pCEditor->Activate(TRUE);		// add this context to the mgr

	// move us to the top of the current list	(for prioritizing)
	pContextMgr->lContexts.Move((void*)pCEditor->gSelf, L_FRONT, NULL);

	APostNotice(N_SETUP, pCEditor->gSelf);	// fake a setup msg to the context, to
														// kick it off

	TRACK_MEM("Map");			pMap = new MapBase;	// init map
	pTileLib = NULL;
 	pCEditor->gMap = pMap->gSelf;	

	pCEditor->LoadAll(mapResNum);

	while (!pContextMgr->fQuitting)
	{
		//	let the mem mangler do its thing first (memory packing, etc)
		// at start of each loop
		pMemMgr->Cycle();

		// maintain current global ptrs to these three things
		pMap = ADerefAs(MapBase, pCEditor->gMap);
//		pTileLib = &(pMap->TileLib);
		pCEditor = ADerefAs(CEditor, gCEditor);
//		pMemMgr->HeapCheck();

		// tell the context mangler to run cycle() of every active context
		// (hopefully this will include ours)
		pContextMgr->Cycle();

		// tell Event Mangler to pass the next pending msg (if any) to
		// the first active context, and let it go from there.
		pEventMgr->PublishNext();
		
		//	update the moose scrimage as necessary
		pMouse->UpdatePointer();
		
		//	tell graphics mangler to do its thing - ALL video RAM access takes
		// place here
		pGraphMgr->Animate();
	}

	// delete map & tile library
	ADelete(pCEditor->gMap);	

	DESTROY_MGR_CAREFUL(Mono, pMono);
//	delete pMono;

	// kill the moose
	delete pMouse;

	// kill our context (by grip), and then Context Mangler itself
	ADelete(gCEditor);	// this will add some erase rectangles to the draw
								// list, so we do it here.
	DESTROY_MGR(pContextMgr);
//	delete pContextMgr;

	//	kill Font Mangler
	DESTROY_MGR_CAREFUL(FontMgr, pFontMgr);
//	delete pFontMgr;

	// kill the event mangler
	DESTROY_MGR_CAREFUL(EventMgr, pEventMgr);
//	delete pEventMgr;

	// all actors should be removed from graph mangler, do one more
	// animate to clear lists.
	pGraphMgr->Animate();

	//	kill the resource mangler
	delete pResMgr;

	// delete graphics mangler
	DESTROY_MGR_CAREFUL(GraphicsMgr, pGraphMgr);
//	delete pGraphMgr;

//	pMemMgr->Dump();
}

void SetColors(uint32 foreCol1, uint32 foreCol2, uint32 backCol)
{
	pFontMgr->colors[FNT_FORE_COLOR] = (uchar) foreCol1;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = (uchar) foreCol2;
	pFontMgr->colors[FNT_BACK_COLOR] = (uchar) backCol;
}

//------------------------------------------------------------------
// member functions, etc

bool
CEditor::HandleMsg(Message *pMsg)
{
	coord		mouseX, mouseY, xPos, yPos;
	Guy		*pGuy;
	int		loop1, tileNum, terrainNum, spaceNum;
	BAM_Box	*pBox;
	MapSpaceBase *pSpace;
	char		szString[20];
	//	tile		*pTile;

//	pMono->Out("Free grips==%d\n", CountFreeGrips());
	switch(pMsg->type)
	{
		case MSG_NOTICE:
			switch(pMsg->notice.type)
			{
				case N_SETUP:
					Setup();
					return(TRUE);
//				case N_TBUTTON:
//					if((int)(pMsg->notice.param) == SELECTED)
//					{
//						if(gClusterDg)		// if Cluster List dialog is active
//						{
//							if(pMsg->notice.gSource == gClusterNext)
//								ListSpaces(ClusterListPos + CLUSTER_LIST_SIZE);
//							if(pMsg->notice.gSource == gClusterPrev)
//								ListSpaces(ClusterListPos - CLUSTER_LIST_SIZE);
//							if(pMsg->notice.gSource == gClusterOK)
//								pMouse->SetLimits(&pMouse->screenLimits);
//						}
//						else if(gTypesDg)	// if Types list dialog is active
//						{
//							if(pMsg->notice.gSource == gTypesNext)
//								ListTerrains(TypesListPos + TYPES_LIST_SIZE);
//							if(pMsg->notice.gSource == gTypesPrev)
//								ListTerrains(TypesListPos - TYPES_LIST_SIZE);
//							if(pMsg->notice.gSource == gTypesOK)
//								pMouse->SetLimits(&pMouse->screenLimits);
//						}
//					}
//					return(TRUE);
				case N_CONTROL_REPLY:		// news from one of our textboxes
					if((int)(pMsg->notice.param) == REPLY_DESELECTED)
					{
						if(pMsg->notice.gSource == gEditBox[0])
						{
							// update text in TerrainNum box (#1) to reflect Tile selected in box #0
							tileNum = atoi(ADerefAs(char, gEditBoxBuffers[0]));
							if((int)(pTileLib->tiles[tileNum].terrain) < 1)
								szString[0] = NULL;
							else sprintf(szString, "%d", (int)(pTileLib->tiles[tileNum].terrain));
							strcpy(ADerefAs(char, gEditBoxBuffers[1]), szString);
							pBox = ADerefAs(BAM_Box, gEditBox[1]);
							pBox->SetEditBuffer(gEditBoxBuffers[1]);
							pBox->Draw();
						}
						else if(pMsg->notice.gSource == gEditBox[2])
						{
							// quick-edit of tile #s finished				
							tileNum = atoi(ADerefAs(char, gEditBoxBuffers[0]));
							terrainNum = atoi(ADerefAs(char, gEditBoxBuffers[1]));
							spaceNum = atoi(ADerefAs(char, gEditBoxBuffers[2]));
							pMap->SetTile(tileNum, ViewX + CursorX, ViewY + CursorY);
							pTileLib->tiles[tileNum].terrain = (terrainType)terrainNum;
							pMap->MapSpaces[(ViewY + CursorY) * WORLD_WIDTH + ViewX + CursorX] = (int16)spaceNum;

							for(loop1 = 0; loop1 < 3; loop1++)
							{
								ADelete(gEditBox[loop1]);
								gEditBox[loop1] = NULL;
							}
							DrawViewPort(CursorX, CursorY);
							pGuy = ADerefAs(Guy, gMapCursor);
							pGuy->Show();
							MoveCursor(CursorX + 1, CursorY);
							pMouse->Show();
						}
						else if(pMsg->notice.gSource == gEditTileBox[3])	// terrain type
						{
							// terrain type of current tile was changed in Tile Editor.
							// editor window must be updated.
							terrainNum = atoi(ADerefAs(char, gEditTileBuffers[3]));
							if(terrainNum >= pTileLib->totalTerrains)
							{
								terrainNum = pTileLib->totalTerrains;
								if(terrainNum == TERRAINMAXTYPES)
									terrainNum = TERRAINMAXTYPES - 1;
								pTileLib->totalTerrains = terrainNum + 1;
								memset(&pTileLib->terrains[terrainNum], NULL, sizeof(struct Terrain));
							}
							tile1.terrain = (terrainType)terrainNum;
							sprintf(ADerefAs(char, gEditTileBuffers[3]), "%d", (int)tile1.terrain);
							pBox = ADerefAs(BAM_Box, gEditTileBox[3]);
							pBox->SetEditBuffer(gEditTileBuffers[3]);
							pBox->Draw();
							EditTileSetupTitle();
							EditTileSetupTerrain();
						}
						else if(pMsg->notice.gSource == gEditTileBox[13])	// cluster #
						{
							spaceNum = atoi(ADerefAs(char, gEditTileBuffers[13]));
							if(spaceNum >= pMap->h.totalSpaces)
							{
								// set new space num
								spaceNum = pMap->AddSpace(FUNC_MISC);
								pSpace = ADerefAs(MapSpaceBase, pMap->gSpaces[spaceNum]);

								// default values
								memset(&pSpace->h, NULL, sizeof(pSpace->h));
								pSpace->h.owner = (uint32)SIDE0;
								EditTileSetupCluster(spaceNum);
							}
							sprintf(ADerefAs(char, gEditTileBuffers[13]), "%d", spaceNum);
							pBox = ADerefAs(BAM_Box, gEditTileBox[13]);
							pBox->SetEditBuffer(gEditTileBuffers[13]);
							pBox->Draw();
							EditTileSetupTitle();
							EditTileSetupCluster(spaceNum);
						}
					}
					return(TRUE);
			}
			break;
		case MSG_EVENT:

			// let context children have a crack at it first.
			// (maybe a BAM_Box is running, etc)
			if(Context::HandleMsg(pMsg))
				return(TRUE);

			switch(pMsg->event.type)
			{
				case E_MOUSE_UP:
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;
		/*			pGuy = ADerefAs(Guy, gBackGuy);
					if(pGuy->scrim.rect.Contains(mouseX, mouseY))
					{
						xPos = ((mouseX - pGuy->scrim.x) - 1) / (TILE_WIDTH + 1);
						yPos = ((mouseY - pGuy->scrim.y) - 1) / (TILE_HEIGHT + 1);
						pMap->MoveCursor(xPos, yPos);
						return(TRUE);
					}*/
	//				else pMono->Out("Mouse released @%d/%d\n", mouseX, mouseY);
					if(VPortMoving)
					{
						VPortMoving = FALSE;	// halt vport auto-scrolling
						return(TRUE);
					}
					break;			
				case E_MOUSE_DOWN:
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;

					// if any of the following Guy's are showing, block viewport clicking.
					pGuy = ADerefAs(Guy, gTypesGuy);
					if(!pGuy->CheckState(S_HIDDEN))
					{
						return(FALSE);
					}
					pGuy = ADerefAs(Guy, gSpacesGuy);
					if(!pGuy->CheckState(S_HIDDEN))
					{
						return(FALSE);
					}
					pGuy = ADerefAs(Guy, gEditGuy);
					if(!pGuy->CheckState(S_HIDDEN))
					{
						return(FALSE);
					}

					if(rViewport.Contains(mouseX, mouseY))
					{
						xPos = (mouseX - 1) / (TILE_WIDTH + 1);
						yPos = (mouseY - 1) / (TILE_HEIGHT + 1);
						if(xPos == CursorX && yPos == CursorY)
						{
							EditTileNum(xPos, yPos);
						}
						else MoveCursor(xPos, yPos);
						return(TRUE);
					}
//					else pMono->Out("Mouse clicked @%d/%d\n", mouseX, mouseY);

					if(mouseX == 0)	// scroll vport left until MOUSE_UP
					{
						VPortMoving = K_LEFT;
						return(TRUE);
					}
					if(mouseX == SCREEN_WIDTH - 1)	// scroll vport right until MOUSE_UP
					{
						VPortMoving = K_RIGHT;
						return(TRUE);
					}
					if(mouseY == 0)	// scroll vport up until MOUSE_UP
					{
						VPortMoving = K_UP;
						return(TRUE);
					}
					if(mouseY == SCREEN_HEIGHT - 1)	// scroll vport down until MOUSE_UP
					{
						VPortMoving = K_DOWN;
						return(TRUE);
					}
					if(mouseY > (MAP_VPORT_HEIGHT + 1) * (TILE_HEIGHT + 1))
					{
						EditTile(CursorX, CursorY);
						return(TRUE);
					}
					break;
				case E_KEY_DOWN:
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;
					switch (pMsg->event.value)
					{
						case K_RETURN:
							pGuy = ADerefAs(Guy, gEditGuy);
							if(pGuy->CheckState(S_HIDDEN))
							{
								EditTileNum(CursorX, CursorY);
								return(TRUE);					
							}
							// fall-through intentional

						case K_O:
							AcceptEditorChanges();
							EndEditTile();
							DrawSpotInfo();
							return(TRUE);
							
						case K_T:
							pGuy = ADerefAs(Guy, gTypesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								pGuy->Hide();
								return(TRUE);
							}

							pGuy = ADerefAs(Guy, gSpacesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								pGuy->Hide();
							}

							ListTerrains(TypesListPos);
							return(TRUE);

						case K_C:
							pGuy = ADerefAs(Guy, gSpacesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								pGuy->Hide();
								return(TRUE);
							}

							pGuy = ADerefAs(Guy, gTypesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								pGuy->Hide();
							}

							ListSpaces(ClusterListPos);
							return(TRUE);

						case K_U:
							pGuy = GetWindowUp();
							if(pGuy && pGuy->gSelf == gEditGuy)
								return(TRUE);

							pGuy = ADerefAs(Guy, gTypesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								ListTerrains(TypesListPos - 5);
								return(TRUE);
							}

							pGuy = ADerefAs(Guy, gSpacesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								ListSpaces(ClusterListPos - 5);
								return(TRUE);
							}

							MoveTileList(-5);
							return(TRUE);

						case K_D:
							pGuy = GetWindowUp();
							if(pGuy && pGuy->gSelf == gEditGuy)
								return(TRUE);

							pGuy = ADerefAs(Guy, gTypesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								ListTerrains(TypesListPos + 5);
								return(TRUE);
							}

							pGuy = ADerefAs(Guy, gSpacesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								ListSpaces(ClusterListPos + 5);
								return(TRUE);
							}
							MoveTileList(5);
							return(TRUE);

						case K_E:
							EditTile(CursorX, CursorY);
							return(TRUE);

						case K_DEL:
							pMap->SetTile(-1, (uint16) (ViewX + CursorX), (uint16) (ViewY + CursorY));		
							DrawViewPort(CursorX, CursorY);
							DrawSpotInfo();
							return(TRUE);

//						case K_1:
//						case K_2:
//						case K_3:
//						case K_4:
//						case K_5:
//							TileNum = (int16) (TileListStart + (pMsg->event.value - K_1));
//							pMap->SetTile(TileNum, (uint16) (ViewX + CursorX), (uint16) (ViewY + CursorY));			
//							DrawViewPort(CursorX, CursorY);
//							DrawSpotInfo();
//							return(TRUE);

						case K_N:
						 	ShowNumbers = ShowNumbers? FALSE: TRUE;
							DrawViewPort();
							return(TRUE);

						case K_RIGHT:
							if(GetWindowUp())
								return(TRUE);

							if(pMsg->event.modifiers & MOD_ALT)
						 		MoveCursor(CursorX + MAP_VPORT_WIDTH / 2, CursorY);
							else
								MoveCursor(CursorX + 1, CursorY);
							return(TRUE);

						case K_LEFT:
							if(GetWindowUp())
								return(TRUE);

							if(pMsg->event.modifiers & MOD_ALT)
						 		MoveCursor(CursorX - MAP_VPORT_WIDTH / 2, CursorY);
							else
								MoveCursor(CursorX - 1, CursorY);
							return(TRUE);

						case K_UP:
							if(GetWindowUp())
								return(TRUE);

							if(pMsg->event.modifiers & MOD_CTRL)
								MoveTileList(-5);
							else
							{
								if(pMsg->event.modifiers & MOD_ALT)
							 		MoveCursor(CursorX, CursorY - MAP_VPORT_HEIGHT / 2);
								else
									MoveCursor(CursorX, CursorY - 1);
							}
							return(TRUE);

						case K_DOWN:
							if(GetWindowUp())
								return(TRUE);

							if(pMsg->event.modifiers & MOD_CTRL)
								MoveTileList(5);
							else
							{
								if(pMsg->event.modifiers & MOD_ALT)
									MoveCursor(CursorX, CursorY + MAP_VPORT_HEIGHT / 2);
								else
									MoveCursor(CursorX, CursorY + 1);
							}
							return(TRUE);

						case K_L:
							// flush old copy from memory, in case of changes.
							if(AIsLoaded(RES_TILELIB, rMapResNum))
							{
								AFlush(ALoad(RES_TILELIB, rMapResNum));
							}
							ADelete(gMap);
							TRACK_MEM("Map");			pMap = new MapBase;	// init map
							gMap = pMap->gSelf;
							pTileLib = NULL;

							LoadAll(rMapResNum);
							DrawViewPort();
							DrawSideBar();
							DrawSpotInfo();
							return(TRUE);

						case K_S:
							SaveAll();
							return(TRUE);

						case K_ESC:
							pGuy = ADerefAs(Guy, gTypesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								pGuy->Hide();
								return(TRUE);
							}

							pGuy = ADerefAs(Guy, gSpacesGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								pGuy->Hide();
								return(TRUE);
							}

							pGuy = ADerefAs(Guy, gEditGuy);
							if(!pGuy->CheckState(S_HIDDEN))
							{
								EndEditTile();
								return(TRUE);
							}

							pContextMgr->Quit();
							return(TRUE);
					}
					break;
			}
			break;

	}
	return Context::HandleMsg(pMsg);
}

void
CEditor::SetCurrentControl(grip gNewControl)
{
	BAM_Button	*pButton;

	if(gCurControl && gCurControl != gNewControl)	// if another control is already current (between MOUSE_DOWN and MOUSE_UP)
	{
		pButton = ADerefAs(BAM_Button, gCurControl);
		pButton->Select(FALSE);
	}

	gCurControl = gNewControl;
}

void
CEditor::LoadAll(int newMapResNum)
{
	Guy			*pGuy;
	grip			gCel, gTileLib;
	uint			resNum;
	CelHeader	*pCel;

	sprintf(pMap->h.LibName, "%d", newMapResNum);
	rMapResNum = newMapResNum;
	rTileResNum = newMapResNum - (newMapResNum % 20);

	TRACK_MEM("Loading");	pGuy = new Guy;
	TRACK_MEM("LoadingCel");	gCel = ACreateCel(&resNum, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, CI_BLACK, 1);

	pCel = (CelHeader *)AGetResData(gCel);
	SetFontColors(CI_BLACK, CI_WHITE, CI_BLACK);
	ASetString(100, 200, "Loading map.  Please wait...", (uchar *)pCel, pCel->width, NULL);

	pGuy->SetRes(RES_CEL, resNum);
	pGuy->SetPos(0, 0);
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(1000);
	AAnimate();

	if(!pMap->Load(pMap->h.LibName, fStripClusters))
	{
		pMap->AddSpace(FUNC_MISC);	// dummy #0
	}

	ASetString(100, 200, "Loading tile library.  Please wait...", (uchar *)pCel, pCel->width, NULL);
	pGuy->SetState(S_CHANGED, TRUE);
	AAnimate();
	if(!AExists(RES_TILELIB, rTileResNum))
	{
		bool		fBool1;
		uint16	wordData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};		// dummy

		// create new resource
		TRACK_MEM("TileLibResMem");	pTileLib = (TileLibRes *)mem_malloc(sizeof(TileLibRes));
		memset(pTileLib, NULL, sizeof(TileLibRes));

		fBool1 = CreateGeneric(RES_TILELIB, rTileResNum, sizeof(TileLibRes), pTileLib, wordData, FALSE);
		mem_free(pTileLib);
	}
	gTileLib = ALoad(RES_TILELIB, rTileResNum);
	RefreshPics(rTileResNum);
	ParseNewTileInfo(rTileResNum);

	// pTileLib MUST be deref'ed again at this point - realloc() may have occured in RefreshPics()
	pTileLib = (TileLibRes *)AGetResData(gTileLib);
	palette.Load(rTileResNum);
	palette.UpdateAll();

	ADelete(pGuy->gSelf);
}

void
CEditor::ParseNewTileInfo(uint rTileResNum)
{
	FILE			*report;
	int			loop1, groupNum, tileNum, terrainNum;
	char			buffer[256], *str, buffer2[256];
	Terrain		*pTerrain;
	tile			*pTile;
	uint32		tilTime, txtTime;

	sprintf(buffer, "%d.TLB", rTileResNum);
	tilTime = AFileLastModTime(buffer);

	sprintf(buffer, "%d.TXT", rTileResNum);
	txtTime = AFileLastModTime(buffer);

	pMono->Out("Parsing new tile info from %s\n", buffer);

	if(txtTime <= tilTime)
	{
		pMono->Out("Tile info file %s is not newer than .TLB - ignoring\n", buffer);
		return;
	}

	report = fopen(buffer, "r");
	if(!report)
		return;

	fgets(buffer, 255, report);
	fgets(buffer, 255, report);
	fgets(buffer, 255, report);

	fgets(buffer, 255, report);
	loop1 = 0;
	while(buffer[0] == '#')
	{
		str = strtok(buffer, " \r\n\t");

		if(!memcmp(str, "#CHOP", 5))
		{
			// discard any further terrain defs.
			pTileLib->totalTerrains = AMax(1, loop1);
			break;
		}

		groupNum = atoi(str + 1);
		pTerrain = &pTileLib->terrains[groupNum];
		if(groupNum >= pTileLib->totalTerrains)
			pTileLib->totalTerrains = groupNum + 1;

		str = strtok(NULL, " \r\n\t");

		// convert underscores to spaces
		strcpy(buffer2, str);
		str = buffer2;
		while(*str)
		{
			if(*str == '_')
				*str = ' ';
			str++;
		}

		strncpy(pTerrain->szName, buffer2, sizeof(pTerrain->szName) - 1);
		str = strtok(NULL, " \r\n\t");	pTerrain->moveRate = atoi(str + 3);
		str = strtok(NULL, " \r\n\t");	pTerrain->attackMod = atoi(str + 3);
		str = strtok(NULL, " \r\n\t");	pTerrain->attackRangeMod = atoi(str + 3);
		str = strtok(NULL, " \r\n\t");	pTerrain->defenseMod = atoi(str + 3);
		str = strtok(NULL, " \r\n\t");	pTerrain->damageVal = atoi(str + 3);
		str = strtok(NULL, " \r\n\t");	pTerrain->burnable = (toupper(*(str + 5)) == 'Y')? TRUE: FALSE;
		str = strtok(NULL, " \r\n\t");	pTerrain->color = atoi(str + 3);
		str = strtok(NULL, " \r\n\t");	pTerrain->portraitNum = atoi(str + 4);

		fgets(buffer, 255, report);
		loop1++;
	}

	// search for end of terrain list
	while(buffer[0] == '#')
		fgets(buffer, 255, report);
		
	// now search for beginning of tiles list
	while(buffer[0] != '#')
		fgets(buffer, 255, report);

	loop1 = 0;
	while(buffer[0] == '#')
	{
		str = strtok(buffer, " \r\n\t");
		tileNum = atoi(str + 1);
		if(tileNum < pTileLib->totalTiles)
		{
			pTile = &pTileLib->tiles[tileNum];

			str = strtok(NULL, " \r\n\t");
			terrainNum = atoi(str + 4);
			if(terrainNum >= pTileLib->totalTerrains)
				terrainNum = 0;	// terrain num invalid
			pTile->terrain = (terrainType)terrainNum;
			
			str = strtok(NULL, " \r\n\t");			pTile->swapTile = atoi(str + 8);
			str = strtok(NULL, " \r\n\t");			pTile->aniRes = atoi(str + 8);
			str = strtok(NULL, " \r\n\t");			pTile->aniDelay = atoi(str + 9);
		}
		fgets(buffer, 255, report);
		loop1++;
	}

	fclose(report);
}

// extract fresh bitmaps from *.LBMs into a pre-existing TileLibRes
void
CEditor::RefreshPics(uint tileRes)
{
	uint32		ChunkSize, ScanLine;
	int			loop1, loop2, pageNum, tileNum, pixelX, pixelY, pixelOffset, newSize, oldSize;
	char			szBitMapFile[80], szBuffer[10];
	uchar			*pBitMap;
	FILE			*pFile;
	tile			*pTile;
	bool			fReading, fPalFound;
	LBMBMHeader *pLBM;
	uchar			*pPal;
	grip			gLib;

	gLib = ALoad(RES_TILELIB, tileRes);
	pTileLib = (TileLibRes *)AGetResData(gLib);

	TRACK_MEM("tempPal");	pPal = new uchar[768];
	TRACK_MEM("LBMHeader"); pLBM = new LBMBMHeader;
	TRACK_MEM("BitMap"); pBitMap = new uchar[1024 * 1024];	// reasonable max

	fReading = TRUE;
	pageNum = 0;
	tileNum = 0;
	pMono->Out("Reading fresh tile images..\n");
	while(fReading)
	{
		sprintf(szBitMapFile, "%d.LBM", tileRes + pageNum + 1);
		pFile = fopen(szBitMapFile, "rb");
		if(!pFile)
		{
			pMono->Out("Warning: Unable to open %s.  End of tile bitmap refresh.\n", szBitMapFile);
			fReading = FALSE;
			continue;
		}
		fPalFound = FALSE;
		pMono->Out("Processing %s..", szBitMapFile);
		// read & discard LBM header
		fread(szBuffer, 1, 4, pFile);
		if(memcmp(szBuffer, "FORM", 4) && memcmp(szBuffer, "LIST", 4) && memcmp(szBuffer, "CAT ", 4))
			pMono->Out("Error: LBM header in \"%s\" invalid.\n", szBitMapFile);
		else
		{
			fread(&ChunkSize, 1, 4, pFile);		// file size - ignore
			fread(szBuffer, 1, 4, pFile);			// sub-type

			// search LBM for BODY chunk
			do
			{
				fread(szBuffer, 1, 4, pFile);
				fread(&ChunkSize, 1, 4, pFile);
				ChunkSize = motr2intl(ChunkSize);
				if(ChunkSize & 1L)
					++ChunkSize;
				if(!memcmp(szBuffer, "BMHD", 4))
				{
					pMono->Out("Reading LBM bitmap header..\n");
					fread(pLBM, 1, sizeof(LBMBMHeader), pFile);
					pLBM->h = (unsigned short) motr2inti(pLBM->h);
					pLBM->w = (unsigned short) motr2inti(pLBM->w);
					pMono->Out("  w=%d, h=%d, x=%d, y=%d, nPlanes=%d, compression=%d\n",
					 pLBM->w, pLBM->h, pLBM->x, pLBM->y, (int)(pLBM->nPlanes),
					 (int)(pLBM->compression));
				}
				else if(!memcmp(szBuffer, "CMAP", 4))
				{
					pMono->Out("Reading LBM palette\n");
					fPalFound = TRUE;
					fread(pPal, 1, 256*3, pFile);

/*					FILE *pPalFile = fopen("palette.dmp", "at");
					fprintf(pPalFile, "-------------------\n");
					for(loop1 = 0; loop1 < 256; loop1++)
						fprintf(pPalFile, "%3d %3d %3d\n", pPal->guns[loop1].r, pPal->guns[loop1].g, pPal->guns[loop1].b);
					pPal->GetColors((Gun *)h.palette, 0, 255);
					fprintf(pPalFile, "\nnew palette\n");
					for(loop1 = 0; loop1 < 256; loop1++)
						fprintf(pPalFile, "%3d %3d %3d\n", h.palette[loop1 * 3], h.palette[loop1 * 3 + 1], h.palette[loop1 * 3 + 2]);						
					fclose(pPalFile);*/
				}
				else if(!memcmp(szBuffer, "BODY", 4))
				{
//					pMono->Out("Reading 400 raster lines\n");
					ScanLine = 0;
					for(loop1 = 0; loop1 < 400; loop1++)
					{
						ReadLBMLine(pFile, pLBM, pBitMap, ScanLine);
					}
				} else fseek(pFile, ChunkSize, SEEK_CUR);
			} while(memcmp(szBuffer, "BODY", 4) && !ferror(pFile));
		}
		if(!fPalFound)
			pMono->Out("Warning: no CMAP chunk found/\n");

//		pMono->Out("Loading images for existing tiles..\n");
		// load in tiles and their corresponding images
		for(loop1 = 0; tileNum < pTileLib->totalTiles && loop1 < 150; loop1++)
		{
			pTile = &pTileLib->tiles[tileNum++];
			// extract new image from bitmap
			pixelX = (loop1 % 15) * (TILE_WIDTH + 1) + 1;
			pixelY = (loop1 / 15) * (TILE_HEIGHT + 1) + 1;
			pixelOffset = pixelY * pLBM->w + pixelX;
			for(loop2 = 0;	loop2 < TILE_HEIGHT;	loop2++)
				memcpy(pTile->pBitMap + loop2 * TILE_WIDTH, pBitMap + pixelOffset + loop2 * pLBM->w, TILE_WIDTH);
		}
		pMono->Out("%d images loaded for existing tiles.\n", loop1);
		
		// if entire page not used
		if(loop1 < 150)
		{
			// create new Tile for each remaining image in source bitmap.
			// EOF is when an image using color 254 in top-left pixel is found.
			pMono->Out("Creating new tiles for remaining images..\n");
			do
			{
				// extract new image from bitmap
				pixelX = (loop1 % 15) * (TILE_WIDTH + 1) + 1;
				pixelY = (loop1 / 15) * (TILE_HEIGHT + 1) + 1;
				pixelOffset = pixelY * pLBM->w + pixelX;
				if(*(pBitMap + pixelOffset) != CI_SKIP)
				{
					oldSize = pMemMgr->SizeOfGrip(gLib);					
					newSize = oldSize + sizeof(tile);
					TRACK_MEM("Realloc");	gLib = ARealloc(gLib, newSize);
					pMemMgr->HeapCheck();
					pTileLib = (TileLibRes *)AGetResData(gLib);
					pTile = &pTileLib->tiles[tileNum++];

					// init new tile to default values
					memset(pTile, NULL, sizeof(struct tile));

					for(loop2 = 0;	loop2 < TILE_HEIGHT;	loop2++)
						memcpy(pTile->pBitMap + loop2 * TILE_WIDTH, pBitMap + pixelOffset + loop2 * pLBM->w, TILE_WIDTH);
					pMemMgr->HeapCheck();
				}				
				loop1++;
			}
			while(*(pBitMap + pixelOffset) != CI_SKIP && tileNum < MAX_TILES && loop1 < 150);		
		}
		fclose(pFile);
		pageNum++;
	}
	pMono->Out("%d total images loaded\n", tileNum);
	pTileLib->totalTiles = tileNum;
	pMemMgr->HeapCheck();
	delete pLBM;
	delete pBitMap;
	delete pPal;

}

// create a dialog to browse the Cluster list
void
CEditor::ListSpaces(int num)
{
	MapSpaceBase *pSpace;
	int			loop1;
	char			szString1[80], szString2[40];
	grip			gSpace;
	Guy			*pGuy;
	CelHeader	*pCel;
	uchar			*pCelData;
	
	if(num < 0)
	{
		if(num <= 1 - CLUSTER_LIST_SIZE)
		{
			// wrap around to bottom end
			num = pMap->h.totalSpaces - CLUSTER_LIST_SIZE;
			num = AMax(num, 0);
		}
		else num = 0;
	}
	if(num >= pMap->h.totalSpaces - CLUSTER_LIST_SIZE)
	{
		if(num >= pMap->h.totalSpaces)
			num = 0;
		else
			num = pMap->h.totalSpaces - CLUSTER_LIST_SIZE;
	}

	ClusterListPos = num;
	
	pGuy = ADerefAs(Guy, gSpacesGuy);
	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, pGuy->scrim.resNum));
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	for(loop1 = 0;	loop1 < pCel->height; loop1++)
	{
		memset(pCelData + loop1 * pCel->width, CI_WHITE, pCel->width);
	}

	SetFontColors(CI_WHITE, CI_BLACK, CI_WHITE);
	ASetString(50, 2, "Cluster List", (uchar *)pCel, pCel->width, NULL);

	for(loop1 = 0; loop1 < CLUSTER_LIST_SIZE; loop1++)
	{
		gSpace = pMap->gSpaces[ClusterListPos + loop1];	
		if(gSpace)
		{
			pSpace = ADerefAs(MapSpaceBase, gSpace);
			if(pSpace->h.fPickRandomItem)
				strcpy(szString2, "R");
			else if(pSpace->h.presetItem == 0)
				strcpy(szString2, "none");
			else
				itoa(pSpace->h.presetItem, szString2, 10);
			sprintf(szString1, "%02d] HP%02d @%d/%d Hot:%c Func:%02d%s\nItem:%-10s SpecLoc:%c Dest %02d/%02d",
				ClusterListPos + loop1, pSpace->h.maxHP, pSpace->h.xPos, pSpace->h.yPos,
				pSpace->h.hotSpot? 'Y': 'n', pSpace->h.func, &szFuncNames[pSpace->h.func][0], szString2,
				pSpace->h.fSpecialItemDrop? 'Y': 'n', pSpace->h.destX, pSpace->h.destY);
			ASetString(2, loop1 * pFontMgr->point * 3 + 15, szString1, (uchar *)pCel, pCel->width, NULL);
		}
	}
	pGuy->Show();
	pGuy->SetState(S_CHANGED, TRUE);
}

// create a dialog to browse the Types list
void
CEditor::ListTerrains(int num)
{
	Terrain	*pTerrain;
	int		loop1;
	char		szString1[80];
	Guy		*pGuy;
	CelHeader	*pCel;
	uchar		*pCelData;

	pGuy = ADerefAs(Guy, gTypesGuy);

	if(num < 0)
	{
		if(num <= 1 - TYPES_LIST_SIZE)
		{
			// wrap around to bottom end
			num = pTileLib->totalTerrains - TYPES_LIST_SIZE;
			num = AMax(num, 0);
		}
		else num = 0;
	}
	if(num >= pTileLib->totalTerrains - TYPES_LIST_SIZE)
	{
		if(num >= pTileLib->totalTerrains)
			num = 0;
		else
			num = pTileLib->totalTerrains - TYPES_LIST_SIZE;
	}

	TypesListPos = num;

	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, pGuy->scrim.resNum));			
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	for(loop1 = 0;	loop1 < pCel->height; loop1++)
	{
		memset(pCelData + loop1 * pCel->width, CI_WHITE, pCel->width);
	}

	SetFontColors(CI_WHITE, CI_BLACK, CI_WHITE);
	ASetString(50, 2, "Terrain List", (uchar *)pCel, pCel->width, NULL);

	for(loop1 = 0; loop1 < TYPES_LIST_SIZE; loop1++)
	{
		if(TypesListPos + loop1 >= pTileLib->totalTerrains)
			continue;

		pTerrain = &pTileLib->terrains[TypesListPos + loop1];
		sprintf(szString1, "%02d]  Mov%02d  Att%02d  Rng%02d  Def%02d  Dmg%02d  Clr%02d\nBurn:%c  Name:%s",
			TypesListPos + loop1, pTerrain->moveRate, pTerrain->attackMod, pTerrain->attackRangeMod, pTerrain->defenseMod,
			pTerrain->damageVal, pTerrain->color, pTerrain->burnable? 'Y': 'n', pTerrain->szName);
		ASetString(2, loop1 * pFontMgr->point * 3 + 15, szString1, (uchar *)pCel, pCel->width, NULL);
	}

	pGuy->Show();
	pGuy->SetState(S_CHANGED, TRUE);
}

void
CEditor::SaveAll(void)
{
	Guy			*pGuy;
	grip			gCel;
	CelHeader	*pCel;
	uint			resNum;
	bool			fBool1;
	uint16		wordData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	TRACK_MEM("SavingGuy");	pGuy = new Guy;
	TRACK_MEM("SavingCel");	gCel = ACreateCel(&resNum, 0, 0,	200, pFontMgr->point * 2, CI_BLACK, 1);
	BorderizeCel(gCel);
	pCel = (CelHeader *)AGetResData(gCel);
	SetFontColors(CI_BLACK, CI_WHITE, CI_BLACK);
	ASetString(0, pFontMgr->point / 2, "Saving map, please wait...", (uchar *)pCel, pCel->width, 0, DG_JUST_CENTER);
	pGuy->SetRes(RES_CEL, resNum);
	pGuy->SetPos(0, 200);
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(1000);
	AAnimate();

	pMap->Save();

	memset(pCel + sizeof(CelHeader), pCel->height * pCel->width, CI_BLACK);
	BorderizeCel(gCel);

	ASetString(0, pFontMgr->point / 2, "Saving tile library, please wait...", (uchar *)pCel, pCel->width, 0, DG_JUST_CENTER);
	pGuy->SetState(S_CHANGED, TRUE);

	// save new tile library resource
	fBool1 = CreateGeneric(RES_TILELIB, rTileResNum,
		sizeof(struct TileLibRes) + (pTileLib->totalTiles - 1) * sizeof(tile),
		pTileLib, wordData, FALSE);

	// flush and reload resource - if save process is defective, this will hopefully show
	// the damage right away.
	AFlush(ALoad(RES_TILELIB, rTileResNum));
	pTileLib = (TileLibRes *)AGetResData(ALoad(RES_TILELIB, rTileResNum));

	TileLibReport();

	ADelete(pGuy->gSelf);
}

void
CEditor::TileLibReport(void)
{
	FILE		*report;
	uint32	loop1;
	char		buffer[256], buffer2[256], *str;
	Terrain	*pTerrain;
	tile		*pTile;

	sprintf(buffer, "%d.TXT", rTileResNum);
	pMono->Out("Writing tile lib report %s\n", buffer);

	report = fopen(buffer, "w");
	fprintf(report, "// %d tile library content report\n", rTileResNum);
	fprintf(report, "// -----------------\n// Terrain Types\n");

	for(loop1 = 0; loop1 < pTileLib->totalTerrains; loop1++)
	{

		pTerrain = &pTileLib->terrains[loop1];

		// convert descr to uppercase, and ' ' to '_'
		strcpy(buffer, pTerrain->szName);

		str = buffer;
		while(*str)
		{
			if(*str == ' ')
				*str = '_';
			else *str = (char)(toupper(*str));
			str++;
		}

		fprintf(report, "#%03d %-26s Mov%02d Att%02d Rng%02d Def%02d Dmg%02d Burn:%c Clr%03d Port%04d\n",
			loop1, buffer, pTerrain->moveRate, pTerrain->attackMod,
			pTerrain->attackRangeMod, pTerrain->defenseMod, 
		 pTerrain->damageVal, pTerrain->burnable? 'Y': 'n', pTerrain->color, pTerrain->portraitNum);
	}
	fprintf(report, "\n-----------------\nTiles\n");

	for(loop1 = 0; loop1 < pTileLib->totalTiles; loop1++)
	{
		pTile = &pTileLib->tiles[loop1];

		pTerrain = &pTileLib->terrains[(int)pTile->terrain];

		// convert descr to uppercase, and ' ' to '_'
		strcpy(buffer, pTerrain->szName);
		str = buffer;
		while(*str)
		{
			if(*str == ' ')
				*str = '_';
			else *str = (char)(toupper(*str));
			str++;
		}

		sprintf(buffer2, "#%04d type%03d:%-26s swapTile%03d animTile%03d animDelay%02d\n",
			loop1, (int)pTile->terrain, buffer, pTile->swapTile,
			pTile->aniRes, pTile->aniDelay);
		fprintf(report, buffer2);
	}
	fprintf(report, "-----\nEnd of file\n");
	fclose(report);
}

// info area below viewport - contains info for selected map spot
void
CEditor::DrawSpotInfo(void)
{
	char			string1[40], string2[40], string3[40];
	uchar			*pCelData;
	CelHeader	*pCel;
	int			mapPos, tileNum, terrainNum, spaceNum, yCoord, loop1;
	tile			*pTile;
	Terrain		*pTerrain;
	MapSpaceBase *pSpace;
	Rectangle	rUpdate;

	SetFontColors(CI_BLACK, CI_WHITE, CI_BLACK);
	// figure out what tile we're standing on
	mapPos = (ViewY + CursorY) * WORLD_WIDTH + ViewX + CursorX;
	tileNum = pMap->MapTiles[mapPos];
	spaceNum = pMap->MapSpaces[mapPos];

	if(tileNum >= pTileLib->totalTiles)
	{
		pTile = NULL;
	}
	else
	{
		pTile = &pTileLib->tiles[tileNum];
		terrainNum = (int)pTile->terrain;
		if(terrainNum < 0 || terrainNum >= pTileLib->totalTerrains)
		{
			pTerrain = NULL;
		}
		else pTerrain = &pTileLib->terrains[terrainNum];
	}

	if(spaceNum == 0 || spaceNum >= pMap->h.totalSpaces)
		pSpace = NULL;
	else
		pSpace = ADerefAs(MapSpaceBase, pMap->gSpaces[spaceNum]);
		
	// get address of cel's data area
	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, rBackground));		// get address of Resource->CelHeader
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	yCoord = MAP_VPORT_HEIGHT * (TILE_HEIGHT + 1) + 5;

	pCelData += (yCoord + pFontMgr->point * 3 - 1) * pCel->width;
	for(loop1 = pFontMgr->point * 3; loop1 > 0; loop1--)
	{
		memset(pCelData, CI_BLACK, pCel->width);
		pCelData -= pCel->width;
	}

	if(!pTile)
	{
		ASetString(0, yCoord, "Invalid tile", (uchar *)pCel, pCel->width, NULL);
	}
	else if(!pTerrain)
	{
		ASetString(0, yCoord, "Invalid terrain", (uchar *)pCel, pCel->width, NULL);
	}
	else
	{
		sprintf(string1, "Tile%03d  Swap%03d  Burn:%c  Type%02d:%s", tileNum, pTile->swapTile,
			pTerrain->burnable? 'Y': 'n', terrainNum, pTerrain->szName);
		ASetString(0, yCoord, string1, (uchar*)pCel, pCel->width, NULL);
		sprintf(string1, "Mv%02d  Att%02d  Rng%02d  Def%02d  Dmg%02d",
			pTerrain->moveRate, pTerrain->attackMod, pTerrain->attackRangeMod, pTerrain->defenseMod, pTerrain->damageVal);
		ASetString(0, yCoord + pFontMgr->point, string1, (uchar*)pCel, pCel->width, NULL);
	}

	if(pSpace)
	{
		if(pSpace->h.hotSpot)
			sprintf(string2, "%03d:%03d", pSpace->h.hotX, pSpace->h.hotY);
		else strcpy(string2, "N");

		if(pSpace->h.fPickRandomItem)
			strcpy(string3, "R");
		else sprintf(string3, "%02d", pSpace->h.presetItem);

		sprintf(string1, "Clust%02d  HP%02d  Hot:%s  Item:%s  %s", spaceNum, pSpace->h.maxHP, string2, string3,
			&szFuncNames[pSpace->h.func][0]);
		ASetString(0, yCoord + pFontMgr->point * 2, string1, (uchar*)pCel, pCel->width, NULL);
	}
	rUpdate.Set(0, yCoord, pCel->width, pCel->height);
	AUpdateRect(&rUpdate);
}

// info space right of viewport - current map x/y loc, etc
void
CEditor::DrawSideBar(void)
{
	char			string1[40];
	uchar			*pCelData;
	CelHeader	*pCel;
	tile			*pTile;
	static int16 LastTileStart = 9999;
	int			coordX, coordY, loop1;
	Rectangle	rUpdate;
 	RCopyPixels	rcp;

	SetFontColors(CI_BLACK, CI_WHITE, CI_BLACK);
	// create right info sidebar

	// get address of cel's data area
	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, rBackground));
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	sprintf(string1, "x%02d y%02d", ViewX + CursorX, ViewY + CursorY);
	coordX = RIGHT_MAP_BORDER;
	ASetString(coordX, 0, string1, (uchar*)pCel, pCel->width, NULL);
	rUpdate.Set(coordX, 0, pCel->width, pFontMgr->point - 1);
	AUpdateRect(&rUpdate);

	if(LastTileStart != TileListStart)
	{
		// refresh tile list

		coordY = 10;
		for(loop1 = 0; loop1 < 5; loop1++)
		{
			sprintf(string1, "%3d", TileListStart + loop1);
			ASetString(coordX + TILE_WIDTH + 1, coordY + 5, string1, (uchar *)pCel, pCel->width, NULL);
			pTile = &pTileLib->tiles[TileListStart + loop1];

			rUpdate.Set(coordX, coordY, coordX + TILE_WIDTH - 1, coordY + TILE_HEIGHT - 1);
			rcp._vpRectFillArea = &rUpdate;		// rect describing data to copy
			rcp._vbufWidth = pCel->width;		// dest cel width
			rcp._celX = 0;							// x offset into src data
			rcp._celY = 0;							// y offset into src data
			rcp._celWidth = TILE_WIDTH;		// src cel width
			rcp._pCelData = pTile->pBitMap;								// ptr to src data
			rcp._pBufData = (uchar *)pCel + sizeof(CelHeader);		// ptr to dest data
			rcp._pDst = rcp._pBufData + ((rUpdate.y1 * pCel->width) + rUpdate.x1);
			rcp._scale = 256;
			rcp._pScrim = NULL;

			// determine copy pixels transfer mode.
			rcp._flags = 0;
			rcp._flags |= CP_NOSKIP;
		
			ACopyPixels(&rcp);
			rUpdate.Set(coordX, coordY, pCel->width, coordY + TILE_HEIGHT - 1);
			AUpdateRect(&rUpdate);
			coordY += TILE_HEIGHT + 1;
		}
	}
}

//	if(!gListSpaces)
//	{
//		TRACK_MEM("Cluster Button");	gCel = ACreateCel(&rNumCel, 0, 0, 40, 15, CI_BLACK, 1);
//		pCel = (CelHeader *)pResMgr->GetResData(gCel);
//		pCelData = (uchar *)pCel + sizeof(CelHeader);
//		pFontMgr->SetString(0, 2, "Clust", (uchar *)pCel, pCel->width, 1, DG_JUST_CENTER);
//		BorderizeCel(gCel);
//		TRACK_MEM("Cluster Button");	pGuy = new Guy;
//		gListSpaces = pGuy->gSelf;
//		pGuy->SetRes(RES_CEL, rNumCel);
//		pGuy->SetPos(RIGHT_MAP_BORDER + 15, 226);
//		pGuy->SetRect();
//		pGuy->SetContext(gContext);
//		pGuy->Setup();
//		pGuy->SetPri(300);
//	}

//	if(!gListTypes)
//	{
//		TRACK_MEM("Type Button");	gCel = ACreateCel(&rNumCel, 0, 0, 40, 15, CI_BLACK, 1);
//		pCel = (CelHeader *)pResMgr->GetResData(gCel);
//		pCelData = (uchar *)pCel + sizeof(CelHeader);
//		pFontMgr->SetString(0, 2, "Type", (uchar *)pCel, pCel->width, 1, DG_JUST_CENTER);
//		BorderizeCel(gCel);
//		TRACK_MEM("Cluster Button");	pGuy = new Guy;
//		gListTypes = pGuy->gSelf;
//		pGuy->SetRes(RES_CEL, rNumCel);
//		pGuy->SetPos(RIGHT_MAP_BORDER, 282);
//		pGuy->SetRect();
//		pGuy->SetContext(gContext);
//		pGuy->Setup();
//		pGuy->SetPri(300);
//	}

//	if(!gLoad)
//	{
//		// create Load/Save buttons
//		TRACK_MEM("LoadCel");		gCel = ACreateCel(&rNumCel, 0, 0, 35, 15, CI_BLACK, 1);
//		pCel = (CelHeader *)pResMgr->GetResData(gCel);
//		pCelData = (uchar *)pCel + sizeof(CelHeader);
//		pFontMgr->SetString(2, 2, "Load", (uchar *)pCel, pCel->width, NULL);
//		BorderizeCel(gCel);
//		TRACK_MEM("Load");	pGuy = new Guy;
//		gLoad = pGuy->gSelf;
//		pGuy->SetRes(RES_CEL, rNumCel);
//		pGuy->SetPos(RIGHT_MAP_BORDER, 245);
//		pGuy->SetRect();
//	 	pGuy->SetContext(pCEditor->gContext);
//	 	pGuy->Setup();
//	 	pGuy->SetPri(300);
//
//		TRACK_MEM("SaveCel");		gCel = ACreateCel(&rNumCel, 0, 0, 35, 15, CI_BLACK, 1);
//		pCel = (CelHeader *)pResMgr->GetResData(gCel);
//		pCelData = (uchar *)pCel + sizeof(CelHeader);
//		pFontMgr->SetString(2, 2, "Save", (uchar *)pCel, pCel->width, NULL);
//		BorderizeCel(gCel);
//		TRACK_MEM("Save");	pGuy = new Guy;
//		gSave = pGuy->gSelf;
//		pGuy->SetRes(RES_CEL, rNumCel);
//		pGuy->SetPos(RIGHT_MAP_BORDER, 265);
//		pGuy->SetRect();
//	 	pGuy->SetContext(pCEditor->gContext);
// 		pGuy->Setup();
//	 	pGuy->SetPri(300);
//	}
//

// move the viewport within the world map
void
CEditor::MoveViewPort(coord xPos, coord yPos)
{
	ViewX = xPos;
	ViewY = yPos;

	if(ViewX > WORLD_WIDTH - MAP_VPORT_WIDTH)
		ViewX = 0;

	if(ViewX < 0)
		ViewX = WORLD_WIDTH - MAP_VPORT_WIDTH;

	if(ViewY > WORLD_HEIGHT - MAP_VPORT_HEIGHT)
		ViewY = 0;

	if(ViewY < 0)
		ViewY = WORLD_HEIGHT - MAP_VPORT_HEIGHT;

	DrawViewPort();
}

// move the highlight cursor within the map's viewport
void
CEditor::MoveCursor(coord xPos, coord yPos)
{
	Guy	*pGuy;
	bool	MoveVPort = FALSE;
	uint32	MapPos, SpaceNum;

	CursorX = xPos;
	CursorY = yPos;

	if(CursorX >= MAP_VPORT_WIDTH)
	{
		ViewX += (CursorX - MAP_VPORT_WIDTH) + 1;
		CursorX = MAP_VPORT_WIDTH - 1;
		MoveVPort = TRUE;
	}

	if(CursorX < 0)
	{
		ViewX += CursorX;
		CursorX = 0;
		MoveVPort = TRUE;
	}
	if(CursorY >= MAP_VPORT_HEIGHT)
	{
		ViewY += (CursorY - MAP_VPORT_HEIGHT) + 1;
		CursorY = MAP_VPORT_HEIGHT - 1;
		MoveVPort = TRUE;
	}
	if(CursorY < 0)
	{
		ViewY += CursorY;
		CursorY = 0;
		MoveVPort = TRUE;
	}

	if(MoveVPort)		// if cursor exceed viewport bounds, move viewport
	{
		// if viewport exceeding world bounds, vport will wrap so wrap cursor
		if(ViewX < 0)
			CursorX = MAP_VPORT_WIDTH - 1;
		else if(ViewX > WORLD_WIDTH - MAP_VPORT_WIDTH)
			CursorX = 0;
		if(ViewY < 0)
			CursorY = MAP_VPORT_HEIGHT - 1;
		else if(ViewY > WORLD_HEIGHT - MAP_VPORT_HEIGHT)
			CursorY = 0;
		MoveViewPort(ViewX, ViewY);
	}

	pGuy = ADerefAs(Guy, gMapCursor);
	pGuy->SetPos(CursorX * (TILE_WIDTH + 1), CursorY * (TILE_HEIGHT + 1));

	MapPos = (ViewY + CursorY) * WORLD_WIDTH + ViewX + CursorX;
	if(pMap->MapSpaces[MapPos])
		SpaceNum = pMap->MapSpaces[MapPos];
	else SpaceNum = NULL;
	MarkCluster(SpaceNum);	// position cluster markers over all tiles belonging to cluster of tile currently under cursor
	DrawSideBar();
	DrawSpotInfo();
}

void
CEditor::MarkCluster(uint32 SpaceNum)	// place a cluster marker guy over every tile belonging to given cluster (space) nbr
{
	uint32	loopX, loopY, Space1, MapPos, MarkerNum, PixX = 0, PixY = 0;
	Guy		*pGuy;
		
	MapPos = ViewY * WORLD_WIDTH + ViewX;
	MarkerNum = 0;

	if(SpaceNum)
		for(loopY = 0 ; loopY < MAP_VPORT_HEIGHT; loopY++)
		{
			for(loopX = 0; loopX < MAP_VPORT_WIDTH; loopX++)
			{
				Space1 = pMap->MapSpaces[MapPos + loopX];
				if(SpaceNum == Space1 && MarkerNum < MAX_CLUSTER_MARKERS)
				{
					pMap->ViewToPix(loopX, loopY, PixX, PixY);
					pGuy = ADerefAs(Guy, gClusterMarkers[MarkerNum]);
					pGuy->SetPos(PixX, PixY);
					pGuy->Show();
					MarkerNum++;
				}	
			}
			MapPos += WORLD_WIDTH;
		}

	// hide remaining markers - dont need em right now
	for(; MarkerNum < MAX_CLUSTER_MARKERS; MarkerNum++)
	{
		pGuy = ADerefAs(Guy, gClusterMarkers[MarkerNum]);
		pGuy->Hide();
	}
}

//--------------------------------------------------------------
void
CEditor::EditTileNum(coord xPos, coord yPos)
{
	char			*szBuf;
	int			tileNum, spaceNum, mapPos, terrainNum;
	coord		 	xCoord, yCoord;
	BAM_Box		*pBox;
//	tile			*pTile;
	Guy			*pGuy;

	mapPos = (ViewY + yPos) * WORLD_WIDTH + ViewX + xPos;
	xCoord = xPos * (TILE_WIDTH + 1) + 1;
	yCoord = yPos * (TILE_HEIGHT + 1) + 1;
	tileNum = pMap->MapTiles[mapPos];
	terrainNum = (int)pTileLib->tiles[tileNum].terrain;
	spaceNum = pMap->MapSpaces[mapPos];

	TRACK_MEM("Box3");	pBox = new BAM_Box;
	gEditBox[2] = pBox->gSelf;
	szBuf = ADerefAs(char, gEditBoxBuffers[2]);
	if(spaceNum < 1)
		*szBuf = NULL;
	else sprintf(szBuf, "%d", spaceNum);
	pBox->SetColors(CI_BLACK, CI_WHITE, CI_BLACK);
	pBox->SetBackCopyColor(CI_BLACK);
	pBox->SetFont(1009);
	pBox->Create(xCoord, yCoord + 26, 30, 12, 300, gEditBoxBuffers[2], 5, gSelf, rBackground);
	pBox->SetupReplies(REPLY_DESELECTED);	// tell us when THIS finished

	TRACK_MEM("Box2");	pBox = new BAM_Box;
	gEditBox[1] = pBox->gSelf;
	szBuf = ADerefAs(char, gEditBoxBuffers[1]);
	if(terrainNum < 1)
		*szBuf = NULL;
	else sprintf(szBuf, "%d", terrainNum);
	pBox->SetColors(CI_BLACK, CI_WHITE, CI_BLACK);
	pBox->SetBackCopyColor(CI_BLACK);
	pBox->SetFont(1009);
	pBox->Create(xCoord, yCoord + 13, 30, 12, 300, gEditBoxBuffers[1], 5, gSelf, rBackground);
	pBox->SetNextBox(gEditBox[2]);

	TRACK_MEM("Box1");	pBox = new BAM_Box;
	gEditBox[0] = pBox->gSelf;
	szBuf = ADerefAs(char, gEditBoxBuffers[0]);
	if(tileNum < 1)
		*szBuf = NULL;
	else sprintf(szBuf, "%d", tileNum);
	pBox->SetColors(CI_BLACK, CI_WHITE, CI_BLACK);
	pBox->SetBackCopyColor(CI_BLACK);
	pBox->SetFont(1009);
	pBox->Create(xCoord, yCoord, 35, 12, 300, gEditBoxBuffers[0], 5, gSelf, rBackground);
	pBox->SetupReplies(REPLY_DESELECTED);	// tell us when THIS finished
	pBox->SetNextBox(gEditBox[1]);
	pBox->Select(TRUE);
	pMouse->Hide();

	pGuy = ADerefAs(Guy, gMapCursor);
	pGuy->Hide();
}

//	redraw a single space in the viewport
void
CEditor::DrawViewPort(coord xPos, coord yPos)
{
	CelHeader	*pCel;
	uchar		*pCelData, *pDest;
	uint16	MapPos;
	int16		TileNum, SpaceNum;
	int		xCoord, yCoord, loop1;
	char		string1[10];
	tile		*pTile;
 	RCopyPixels	rcp;
 	Rectangle	rArea;

	xCoord = xPos * (TILE_WIDTH + 1) + 1;
	yCoord = yPos * (TILE_HEIGHT + 1) + 1;

	MapPos = (uint16) ((ViewY + yPos) * WORLD_WIDTH + ViewX + xPos);
	TileNum = pMap->MapTiles[MapPos];
	SpaceNum = pMap->MapSpaces[MapPos];
	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, rBackground));		// get address of Resource->CelHeader
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	pDest = pCelData + yCoord * pCel->width + xCoord;

	rArea.Set(xCoord, yCoord, xCoord + TILE_WIDTH - 1, yCoord + TILE_HEIGHT - 1);

	if(TileNum < 0 || TileNum >= pTileLib->totalTiles)
	{
		pTile = NULL;
		for(loop1 = 0;	loop1 < TILE_HEIGHT; loop1++)
			memset(pDest + loop1 * pCel->width, CI_WHITE, TILE_WIDTH);
	}
	else
	{
		pTile = &pTileLib->tiles[TileNum];

		rcp._vpRectFillArea = &rArea;		// rect describing data to copy
		rcp._vbufWidth = pCel->width;		// dest cel width
		rcp._celX = 0;							// x offset into src data
		rcp._celY = 0;							// y offset into src data
		rcp._celWidth = TILE_WIDTH;		// src cel width
		rcp._pCelData = pTile->pBitMap;								// ptr to src data
		rcp._pBufData = (uchar *)pCel + sizeof(CelHeader);
		rcp._pDst = rcp._pBufData + ((rArea.y1 * pCel->width) + rArea.x1);
		rcp._scale = 256;
		rcp._pScrim = NULL;

		// determine copy pixels transfer mode.
		rcp._flags = CP_NOSKIP;

		// fill buffer with platform optimized routine
		ACopyPixels(&rcp);

		if(pCEditor->ShowNumbers)
		{
			SetFontColors(CI_SKIP, CI_WHITE, CI_BLACK);

			sprintf(string1, "%d", TileNum);
			ASetString(xPos * (TILE_WIDTH + 1) + 1, yPos * (TILE_HEIGHT + 1) + 2, string1, (uchar*)pCel, pCel->width, NULL);

			if(pTile)
			{
				sprintf(string1, "%d", (int)pTile->terrain);
				ASetString(xPos * (TILE_WIDTH + 1) + 1, yPos * (TILE_HEIGHT + 1) + 2 + pFontMgr->point, string1, (uchar*)pCel, pCel->width, NULL);

				sprintf(string1, "%d", SpaceNum);
				ASetString(xPos * (TILE_WIDTH + 1) + 1, yPos * (TILE_HEIGHT + 1) + 2 + pFontMgr->point * 2, string1, (uchar*)pCel, pCel->width, NULL);
			}
		}
	}
	AUpdateRect(&rArea);
}

void
BorderizeCel(grip gCel)
{
	uint16		loopY;
	uchar			*pCelData;
	CelHeader	*pCel;

	// get address of cel's data area
	pCel = (CelHeader *)pResMgr->GetResData(gCel);		// get address of Resource->CelHeader
	pCelData = (uchar *)pCel + sizeof(CelHeader);

	// draw the border inside of cel
	memset(pCelData, pFontMgr->colors[FNT_FORE_COLOR], pCel->width);
	memset(pCelData + pCel->width * (pCel->height - 1), pFontMgr->colors[FNT_FORE_COLOR], pCel->width);
	for(loopY = 0; loopY < pCel->height; loopY++)
	{
		*(pCelData + pCel->width * loopY) = pFontMgr->colors[FNT_FORE_COLOR];
		*(pCelData + pCel->width * (loopY + 1) - 1) = pFontMgr->colors[FNT_FORE_COLOR];
	}
}

/*void
Map::SetTile(int16 TileNum, uint16 xPos, uint16 yPos)
{
	uint32	MapOffSet;

	MapOffSet = yPos * WORLD_WIDTH + xPos;
	MapTiles[MapOffSet] = TileNum;
	if(TileNum == -1)		// if no tile
		MapSpaces[MapOffSet] = 0;	// then no cluster info either
}*/

void
CEditor::DrawViewPort(void)
{
	uchar		*pDest, *pTempDest, *pCelData;
	uint16	loopX, loopY, xStart, MapPos;
	int16		TileNum, SpaceNum;
	coord		xDif, yDif;
	char		string1[10];												
	tile		*pTile;
	CelHeader *pCel;
 	RCopyPixels	rcp;
	static coord LastX = 9999, LastY = 9999;
	Rectangle	rArea;
	int		xCoord, yCoord, loop1;

	SetFontColors(CI_SKIP, CI_WHITE, CI_BLACK);

	// get address of cel's data area
	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, rBackground));		// get address of Resource->CelHeader
	pCelData = (uchar *)pCel + sizeof(CelHeader);

	// figure out what part of viewport cel can just be moved within the cel
	xDif = ViewX - LastX;
	yDif = ViewY - LastY;
	xStart = 0;
	LastX = ViewX;
	LastY = ViewY;

	pDest = pCelData + pCel->width + 1;	// origin of bitmap (inside of border)
	pDest += xStart * (TILE_WIDTH + 1);		// + horz offset

	for(loopY = 0; loopY < MAP_VPORT_HEIGHT; loopY++)
	{
		pTempDest = pDest;
		yCoord = loopY * (TILE_HEIGHT + 1) + 1;
		xCoord = 1;
		for(loopX = xStart; loopX < MAP_VPORT_WIDTH; loopX++)
		{
			MapPos = (uint16) ((ViewY + loopY) * WORLD_WIDTH + ViewX + loopX);
			TileNum = pMap->MapTiles[MapPos];
			SpaceNum = pMap->MapSpaces[MapPos];
			if(TileNum >= pTileLib->totalTiles)
			{
				for(loop1 = 0;	loop1 < TILE_HEIGHT; loop1++)
					memset(pTempDest + loop1 * pCel->width, CI_WHITE, TILE_WIDTH);
			}
			else
			{
				pTile = &pTileLib->tiles[TileNum];

	 			rArea.Set(xCoord, yCoord, xCoord + TILE_WIDTH - 1, yCoord + TILE_HEIGHT - 1);
				rcp._vpRectFillArea = &rArea;		// rect describing data to copy
				rcp._vbufWidth = pCel->width;		// dest cel width
				rcp._celX = 0;							// x offset into src data
				rcp._celY = 0;							// y offset into src data
				rcp._celWidth = TILE_WIDTH;		// src cel width
				rcp._pCelData = pTile->pBitMap;								// ptr to src data
				rcp._pBufData = (uchar *)pCel + sizeof(CelHeader);		// ptr to dest data
				rcp._pDst = rcp._pBufData + ((rArea.y1 * pCel->width) + rArea.x1);
				rcp._scale = 256;
				rcp._pScrim = NULL;
			
				// determine copy pixels transfer mode.
				rcp._flags = CP_NOSKIP;

				// fill buffer with platform optimized routine
				ACopyPixels(&rcp);

				if(ShowNumbers)
				{
					sprintf(string1, "%d", TileNum);
					ASetString(loopX * (TILE_WIDTH + 1) + 1, loopY * (TILE_HEIGHT + 1) + 2, string1, (uchar*)pCel, pCel->width, NULL);

					if(pTile)	
					{
						sprintf(string1, "%d", (int)pTile->terrain);
						ASetString(loopX * (TILE_WIDTH + 1) + 1, loopY * (TILE_HEIGHT + 1) + 2 + pFontMgr->point, string1, (uchar*)pCel, pCel->width, NULL);

						sprintf(string1, "%d", SpaceNum);
						ASetString(loopX * (TILE_WIDTH + 1) + 1, loopY * (TILE_HEIGHT + 1) + 2 + pFontMgr->point * 2, string1, (uchar*)pCel, pCel->width, NULL);
					}
				}
			}
			pTempDest += TILE_WIDTH + 1;
			xCoord += TILE_WIDTH + 1;
		}
		pDest += pCel->width * (TILE_HEIGHT + 1);
	}
	rArea.Set(0, 0, MAP_VPORT_WIDTH * (TILE_WIDTH + 1) + 1, MAP_VPORT_HEIGHT * (TILE_HEIGHT + 1) + 1);
	AUpdateRect(&rArea);
}

void
CEditor::Cycle(void)
{
	Context::Cycle();
	if(VPortMoving)
	{
		switch(VPortMoving)
		{
			case K_UP:
				MoveCursor(CursorX, CursorY - 1);
				break;
			case K_DOWN:
				MoveCursor(CursorX, CursorY + 1);
				break;
			case K_LEFT:
				MoveCursor(CursorX - 1, CursorY);
				break;
			case K_RIGHT:
				MoveCursor(CursorX + 1, CursorY);
				break;
		}
	}
}

void
CEditor::MoveTileList(int move)
{
	TileListStart += move;
	if(TileListStart < 0)
		TileListStart = (int16) (pTileLib->totalTiles - 5);
	if(TileListStart > pTileLib->totalTiles - 5)
		TileListStart = 0;
	DrawSideBar();
}

grip
CEditor::CreateBox(Guy *pWin, int xCoord, int yCoord, grip gText, int maxLen, uint32 newReplyMask)
{
	BAM_Box	*pBox;

	TRACK_MEM("CreateBox");		pBox = new BAM_Box;
	pBox->SetColors(CI_BLACK, CI_WHITE, CI_BLACK);
	pBox->SetBackCopyColor(CI_BLACK);
	pBox->SetFont(1009);
	pBox->Create(xCoord, yCoord, 8 * maxLen, pFontMgr->point + 2, pWin->scrim.priority + 10,
		gText, maxLen, pWin->gContext, pWin->scrim.resNum);
	pBox->SetupReplies(newReplyMask);
	pBox->SetNextBox(NULL);
	return(pBox->gSelf);
}

grip
CEditor::CreateButton(Guy *pWin, int xPos, int yPos, char *szName, int keyEquiv)
{
	grip			gCel;
	BAM_Button	*pButton;
	uint			rTemp;
	CelHeader	*pCel;

	TRACK_MEM(szName);	gCel = ACreateCel(&rTemp, 0, 0, strlen(szName) * 9, pFontMgr->point + 4, CI_BLACK, 1);
	pCel = (CelHeader *)AGetResData(gCel);
	ASetString(0, 2, szName, (uchar *)pCel, pCel->width, 1, DG_JUST_CENTER);
	BorderizeCel(gCel);
	TRACK_MEM(szName);	pButton = new BAM_Button;
	pButton->fDeselectCurControl = FALSE;
	pButton->Create(xPos, yPos, 300, RES_CEL, rTemp, 1, pWin->gContext);
	pButton->SetupReplies(REPLY_ACTIVATED | REPLY_SELECTED_KEY, (void *)keyEquiv);
	pButton->fIsToggle = FALSE;
	pButton->SetOwnerCel(pWin->scrim.resNum);
	pButton->Select(FALSE);
	return(pButton->gSelf);
}

Guy *
CEditor::GetWindowUp(void)
{
	Guy	*pGuy;

	pGuy = ADerefAs(Guy, gTypesGuy);
	if(!pGuy->CheckState(S_HIDDEN))
		return(pGuy);

	pGuy = ADerefAs(Guy, gSpacesGuy);
	if(!pGuy->CheckState(S_HIDDEN))
		return(pGuy);

	pGuy = ADerefAs(Guy, gEditGuy);
	if(!pGuy->CheckState(S_HIDDEN))
		return(pGuy);

	return(NULL);
}

void
CEditor::EndEditTile(void)
{
	int	loop1;
	Guy	*pGuy;
	BAM_Button	*pButton;

	pGuy = ADerefAs(Guy, gEditGuy);
	if(pGuy->CheckState(S_HIDDEN))
	{
		// editor window not up - nothing to close
		return;
	}
	
	for(loop1 = 0;	loop1 < 25; loop1++)
	{
		if(gEditTileBox[loop1])
		{
			ADelete(gEditTileBox[loop1]);
			gEditTileBox[loop1] = NULL;
		}
	}
	pGuy->Hide();

	pButton = ADerefAs(BAM_Button, gOKBtn);
	pButton->Listen(FALSE);
	return;
}

void
CEditor::AcceptEditorChanges(void)
{
	int		tileNum, spaceNum, mapPos;
	tile		*pTile;
	Terrain	*pTerrain;
	MapSpaceBase *pSpace;

	mapPos = (ViewY + CursorY) * WORLD_WIDTH + ViewX + CursorX;

	tileNum = pMap->MapTiles[mapPos];
	pTile = &pTileLib->tiles[tileNum];

	pTerrain = &pTileLib->terrains[(int)tile1.terrain];

	spaceNum = atoi(ADerefAs(char, gEditTileBuffers[13]));
	if(spaceNum < 0 || spaceNum >= pMap->h.totalSpaces)
		spaceNum = 0;

	// process new tile info
	tile1.swapTile = atoi(ADerefAs(char, gEditTileBuffers[0]));
	tile1.aniRes = atoi(ADerefAs(char, gEditTileBuffers[1]));
	tile1.aniDelay = atoi(ADerefAs(char, gEditTileBuffers[2]));
	tile1.terrain = (terrainType)atoi(ADerefAs(char, gEditTileBuffers[3]));

	strncpy(terrain1.szName, ADerefAs(char, gEditTileBuffers[4]), sizeof(terrain1.szName));
	terrain1.moveRate = atoi(ADerefAs(char, gEditTileBuffers[5]));
	terrain1.defenseMod = atoi(ADerefAs(char, gEditTileBuffers[6]));
	terrain1.attackMod = atoi(ADerefAs(char, gEditTileBuffers[7]));
	terrain1.attackRangeMod = atoi(ADerefAs(char, gEditTileBuffers[8]));
	terrain1.damageVal = atoi(ADerefAs(char, gEditTileBuffers[9]));
	terrain1.portraitNum = atoi(ADerefAs(char, gEditTileBuffers[10]));
	terrain1.color = atoi(ADerefAs(char, gEditTileBuffers[11]));
	terrain1.burnable = atoi(ADerefAs(char, gEditTileBuffers[12]));

	space1.h.func = atoi(ADerefAs(char, gEditTileBuffers[14]));
	space1.h.hotSpot = (int8)atoi(ADerefAs(char, gEditTileBuffers[15]));
	space1.h.fPickRandomItem = (int8)atoi(ADerefAs(char, gEditTileBuffers[16]));
	space1.h.presetItem = atoi(ADerefAs(char, gEditTileBuffers[17]));
	space1.h.destX = atoi(ADerefAs(char, gEditTileBuffers[18]));
	space1.h.destY = atoi(ADerefAs(char, gEditTileBuffers[19]));
	space1.h.owner = atoi(ADerefAs(char, gEditTileBuffers[20]));
	space1.h.maxHP = atoi(ADerefAs(char, gEditTileBuffers[21]));
	space1.h.fSpecialItemDrop = (int8)atoi(ADerefAs(char, gEditTileBuffers[22]));

	memcpy(pTile, &tile1, sizeof(tile));
	memcpy(pTerrain, &terrain1, sizeof(Terrain));

	pMap->MapSpaces[mapPos] = (int16)spaceNum;
	if(spaceNum)
	{
		pSpace = ADerefAs(MapSpaceBase, pMap->gSpaces[spaceNum]);
		memcpy(&(pSpace->h), &(space1.h), sizeof(space1.h));
	}
}

void
CEditor::EditTileSetupTitle(void)
{
	CelHeader	*pCel;
	uchar			*pCelData;
	Guy			*pGuy;
	char			szString1[80];
	Rectangle	rUpdate;

	pGuy = ADerefAs(Guy, gEditGuy);
	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, pGuy->scrim.resNum));			
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	SetFontColors(CI_WHITE, CI_BLACK, CI_WHITE);

	sprintf(szString1, "Tile Editor - %d Tiles, %d Types, %d Clusters ", pTileLib->totalTiles,
		pTileLib->totalTerrains, pMap->h.totalSpaces);
	ASetString(30, 5, szString1, (uchar *)pCel, pCel->width, NULL);
	rUpdate.Set(0, 0, pCel->width, 5 + pFontMgr->point);
	AUpdateRect(&rUpdate);
}

void
CEditor::EditTileSetupTerrain(void)
{
	BAM_Box	*pBox;
	int		loop1;

	// load new information
	memcpy(&terrain1, &pTileLib->terrains[(int)tile1.terrain], sizeof(Terrain));

	// place new information in box buffers
	sprintf(ADerefAs(char, gEditTileBuffers[4]), "%s", terrain1.szName);
	sprintf(ADerefAs(char, gEditTileBuffers[11]), "%d", (int)terrain1.color);
	sprintf(ADerefAs(char, gEditTileBuffers[5]), "%d", (int)terrain1.moveRate);
	sprintf(ADerefAs(char, gEditTileBuffers[6]), "%d", (int)terrain1.defenseMod);
	sprintf(ADerefAs(char, gEditTileBuffers[7]), "%d", (int)terrain1.attackMod);
	sprintf(ADerefAs(char, gEditTileBuffers[8]), "%d", (int)terrain1.attackRangeMod);
	sprintf(ADerefAs(char, gEditTileBuffers[9]), "%d", (int)terrain1.damageVal);
	sprintf(ADerefAs(char, gEditTileBuffers[10]), "%d", (int)terrain1.portraitNum);
	sprintf(ADerefAs(char, gEditTileBuffers[12]), "%d", (int)terrain1.burnable);

	// refresh boxes
	for(loop1 = 4; loop1 < 13; loop1++)
	{
		pBox = ADerefAs(BAM_Box, gEditTileBox[loop1]);
		pBox->SetEditBuffer(gEditTileBuffers[loop1]);
		pBox->Draw();
	}
}

void
CEditor::EditTileSetupCluster(int num)
{
	BAM_Box	*pBox;
	int		loop1;
	MapSpaceBase	*pSpace;
	grip		grip1;

	grip1 = pMap->gSpaces[num];
	pSpace = ADerefAs(MapSpaceBase, grip1);
	memcpy(&space1.h, &pSpace->h, sizeof(space1.h));

	sprintf(ADerefAs(char, gEditTileBuffers[13]), "%d", num);
	sprintf(ADerefAs(char, gEditTileBuffers[14]), "%d", (int)space1.h.func);
	sprintf(ADerefAs(char, gEditTileBuffers[15]), "%d", (int)space1.h.hotSpot);
	sprintf(ADerefAs(char, gEditTileBuffers[16]), "%d", (int)space1.h.fPickRandomItem);
	sprintf(ADerefAs(char, gEditTileBuffers[17]), "%d", (int)space1.h.presetItem);
	sprintf(ADerefAs(char, gEditTileBuffers[18]), "%d", (int)space1.h.destX);
	sprintf(ADerefAs(char, gEditTileBuffers[19]), "%d", (int)space1.h.destY);
	sprintf(ADerefAs(char, gEditTileBuffers[20]), "%d", (int)space1.h.owner);
	sprintf(ADerefAs(char, gEditTileBuffers[21]), "%d", (int)space1.h.maxHP);
	sprintf(ADerefAs(char, gEditTileBuffers[22]), "%d", (int)space1.h.fSpecialItemDrop);

	// refresh boxes
	for(loop1 = 13; loop1 < 23; loop1++)
	{
		pBox = ADerefAs(BAM_Box, gEditTileBox[loop1]);
		pBox->SetEditBuffer(gEditTileBuffers[loop1]);
		pBox->Draw();
	}
}

void
CEditor::EditTile(coord CursorX, coord CursorY)
{
	int			mapPos, tileNum, spaceNum, loop1;
	Guy			*pGuy;
	CelHeader	*pCel;
	uchar			*pCelData;
	char			szString1[80];
	grip			grip1;
	MapSpaceBase	*pSpace;
	BAM_Button	*pButton;

	pGuy = ADerefAs(Guy, gEditGuy);

	pCel = (CelHeader *)AGetResData(ALoad(RES_CEL, pGuy->scrim.resNum));			
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	for(loop1 = 0;	loop1 < pCel->height; loop1++)
	{
		memset(pCelData + loop1 * pCel->width, CI_WHITE, pCel->width);
	}

	mapPos = (ViewY + CursorY) * WORLD_WIDTH + ViewX + CursorX;
	tileNum = pMap->MapTiles[mapPos];
	if(tileNum < 0 || tileNum >= pTileLib->totalTiles)
		return;

	// working copy of tile to hack on	
	memcpy(&tile1, &pTileLib->tiles[tileNum], sizeof(tile));
	memcpy(&terrain1, &pTileLib->terrains[(int)tile1.terrain], sizeof(Terrain));

	spaceNum = pMap->MapSpaces[mapPos];
	grip1 = pMap->gSpaces[spaceNum];
	pSpace = ADerefAs(MapSpaceBase, grip1);

	memcpy(&space1.h, &pSpace->h, sizeof(space1.h));

	SetFontColors(CI_WHITE, CI_BLACK, CI_WHITE);

	EditTileSetupTitle();
	sprintf(szString1, "Tile %d", tileNum);
	ASetString(10, 30, szString1, (uchar *)pCel, pCel->width, NULL);
	
	ASetString(60, 30,  "Swap      Anim      Delay", (uchar *)pCel, pCel->width, NULL);
	ASetString(240, 30, "Type", (uchar *)pCel, pCel->width, NULL);
	sprintf(ADerefAs(char, gEditTileBuffers[0]), "%d", (int)tile1.swapTile);
	sprintf(ADerefAs(char, gEditTileBuffers[1]), "%d", (int)tile1.aniRes);
	sprintf(ADerefAs(char, gEditTileBuffers[2]), "%d", (int)tile1.aniDelay);
	sprintf(ADerefAs(char, gEditTileBuffers[3]), "%d", (int)tile1.terrain);

	gEditTileBox[0] = CreateBox(pGuy, 85, 30, gEditTileBuffers[0], 5);
	gEditTileBox[1] = CreateBox(pGuy, 145, 30, gEditTileBuffers[1], 4);
	gEditTileBox[2] = CreateBox(pGuy, 210, 30, gEditTileBuffers[2], 4);
	gEditTileBox[3] = CreateBox(pGuy, 265, 30, gEditTileBuffers[3], 4, REPLY_DESELECTED);

	SetFontColors(CI_WHITE, CI_BLACK, CI_WHITE);
	ASetString(10,  45, "Name", (uchar *)pCel, pCel->width, NULL);
	ASetString(245, 45, "Por", (uchar *)pCel, pCel->width, NULL);
	ASetString(5,   60, "Mov       Def      Att      Rng      Dmg", (uchar *)pCel, pCel->width, NULL);
	ASetString(225, 75, "Burn", (uchar *)pCel, pCel->width, NULL);
	ASetString(225, 90, "Clr", (uchar *)pCel, pCel->width, NULL);

	gEditTileBox[4] = CreateBox(pGuy, 45, 45, gEditTileBuffers[4], 22);
	gEditTileBox[11] = CreateBox(pGuy, 255, 90, gEditTileBuffers[11], 4);
	gEditTileBox[5] = CreateBox(pGuy, 30, 60, gEditTileBuffers[5], 3);
	gEditTileBox[6] = CreateBox(pGuy, 85, 60, gEditTileBuffers[6], 3);
	gEditTileBox[7] = CreateBox(pGuy, 140, 60, gEditTileBuffers[7], 3);
	gEditTileBox[8] = CreateBox(pGuy, 195, 60, gEditTileBuffers[8], 3);
	gEditTileBox[9] = CreateBox(pGuy, 255, 60, gEditTileBuffers[9], 3);
	gEditTileBox[10] = CreateBox(pGuy, 265, 45, gEditTileBuffers[10], 5);
	gEditTileBox[12] = CreateBox(pGuy, 255, 75, gEditTileBuffers[12], 3);
	EditTileSetupTerrain();

	SetFontColors(CI_WHITE, CI_BLACK, CI_WHITE);
	ASetString(10, 75,	"Clust        Func", 			(uchar *)pCel, pCel->width, NULL);
//	ASetString(80, 88,	"X             Y", 				(uchar *)pCel, pCel->width, NULL);
	ASetString(15, 101,	"Hot        X        Y", 		(uchar *)pCel, pCel->width, NULL);
	ASetString(10, 127,	" Item          Random       StartLoc", (uchar *)pCel, pCel->width, NULL);
	ASetString(180, 101,	"HP", 								(uchar *)pCel, pCel->width, NULL);
	ASetString(10, 153,	"DestX           DestY           Owner", (uchar *)pCel, pCel->width, NULL);

	gEditTileBox[13] = CreateBox(pGuy, 45, 75, gEditTileBuffers[13], 4, REPLY_DESELECTED);
	gEditTileBox[14] = CreateBox(pGuy, 115, 75, gEditTileBuffers[14], 3);
	gEditTileBox[15] = CreateBox(pGuy, 45, 101, gEditTileBuffers[15], 3);
	gEditTileBox[21] = CreateBox(pGuy, 195, 101, gEditTileBuffers[21], 5);
	gEditTileBox[17] = CreateBox(pGuy, 45, 127, gEditTileBuffers[17], 3);
	gEditTileBox[16] = CreateBox(pGuy, 140, 127, gEditTileBuffers[16], 3);
	gEditTileBox[22] = CreateBox(pGuy, 230, 127, gEditTileBuffers[22], 3);
	gEditTileBox[18] = CreateBox(pGuy, 45, 153, gEditTileBuffers[18], 4);
	gEditTileBox[19] = CreateBox(pGuy, 140, 153, gEditTileBuffers[19], 4);
	gEditTileBox[20] = CreateBox(pGuy, 235, 153, gEditTileBuffers[20], 3);
	EditTileSetupCluster(spaceNum);

	pGuy->Show();

	pButton = ADerefAs(BAM_Button, gOKBtn);
	pButton->Listen(TRUE);
	pButton->SetState(S_CHANGED, TRUE);
	pButton->Draw();
}

// draw border around inside edge of cel
void
CEditor::Setup(void)
{
	uchar			*pCelData;
	Guy			*pGuy;
	CelHeader	*pCel;
	grip			gCel;
	uint			rTemp;
	int			loop1;
	char			*pChar;
	
	fIsTextMode = FALSE;	// for WRITERES
	for(loop1 = 0;	loop1 < 3; loop1++)
	{
		gEditBox[loop1] = NULL;
		TRACK_MEM("EditBoxBuf");	gEditBoxBuffers[loop1] = AMalloc(80);
		pChar = ADerefAs(char, gEditBoxBuffers[loop1]);
	}
	for(loop1 = 0;	loop1 < 25; loop1++)
	{
		gEditTileBox[loop1] = NULL;
		TRACK_MEM("EditTileBuf");	gEditTileBuffers[loop1] = AMalloc(80);
		pChar = ADerefAs(char, gEditTileBuffers[loop1]);
		*pChar = NULL;
	}

	rViewport.Set(0, 0, MAP_VPORT_WIDTH * (TILE_WIDTH + 1) + 1, MAP_VPORT_HEIGHT * (TILE_HEIGHT + 1) + 1);
	TRACK_MEM("BGroundCel");	gCel = ACreateCel(&rBackground, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
		pFontMgr->colors[FNT_BACK_COLOR], 1);
	// get address of cel's data area
	pCel = (CelHeader *)AGetResData(gCel);
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	TRACK_MEM("BGroundGuy");	pGuy = new Guy;
	gBackGuy = pGuy->gSelf;
	pGuy->SetRes(RES_CEL, rBackground);
	pGuy->SetPos(0, 0);
	pGuy->SetRect();
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(100);

	// create instruction/help line
	ASetString(0, 365, "Map:[ALT] UP/DN/L/R  Tiles:U/D  Terrain:T", (uchar *)pCel, pCel->width, NULL);
	ASetString(0, 365 + pFontMgr->point, "Edit:ENTER/DEL  Save:S  Exit:ESC", (uchar *)pCel,
		pCel->width, NULL);

	// create cel && Guy for Terrain List
	TRACK_MEM("TerrainListCel");	gCel = ACreateCel(&rTemp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2 - 25, CI_WHITE, 1);
	BorderizeCel(gCel);
	TRACK_MEM("TerrainListGuy");	pGuy = new Guy;
	gTypesGuy = pGuy->gSelf;
	pGuy->SetRes(RES_CEL, rTemp);
	pGuy->SetPos(0, 0);
	pGuy->SetRect();
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(400);
	pGuy->Hide();

	// create cel && Guy for Cluster List
	TRACK_MEM("ClustListCel");	gCel = ACreateCel(&rTemp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2 - 25, CI_WHITE, 1);
	BorderizeCel(gCel);
	TRACK_MEM("ClustListGuy");	pGuy = new Guy;
	gSpacesGuy = pGuy->gSelf;
	pGuy->SetRes(RES_CEL, rTemp);
	pGuy->SetPos(0, 0);
	pGuy->SetRect();
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(400);
	pGuy->Hide();

	// create cel && Guy for Tile Editor
	TRACK_MEM("EditorCel");	gCel = ACreateCel(&rTemp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2, CI_WHITE, 1);
	BorderizeCel(gCel);
	TRACK_MEM("EditGuy");	pGuy = new Guy;
	gEditGuy = pGuy->gSelf;
	pGuy->SetRes(RES_CEL, rTemp);
	pGuy->SetPos(0, 0);
	pGuy->SetRect();
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(400);
	pGuy->Hide();

	// create cel for map cursor box
	TRACK_MEM("MapCursorCel");	gCel = ACreateCel(&rTemp, 0, 0, TILE_WIDTH + 2, TILE_HEIGHT + 2, CI_SKIP, 1);
	BorderizeCel(gCel);

	TRACK_MEM("MapCursor");		pGuy = new Guy;
	gMapCursor = pGuy->gSelf;
	pGuy->SetRes(RES_CEL, rTemp);
	pGuy->SetPos(0, 0);
	pGuy->SetRect();
	pGuy->SetContext(gSelf);
	pGuy->Setup();
	pGuy->SetPri(300);

	pGuy = ADerefAs(Guy, gEditGuy);
	gOKBtn = CreateButton(pGuy, 190, 175, "OK", K_O);

	pGuy = ADerefAs(Guy, gBackGuy);
	gUpGuy = CreateButton(pGuy, RIGHT_MAP_BORDER, 210, "UP", K_U);
	gDownGuy = CreateButton(pGuy, RIGHT_MAP_BORDER + 22, 210, "DOWN", K_D);
	gTerrainGuy = CreateButton(pGuy, RIGHT_MAP_BORDER, 226, "TERR", K_T);
	gNumbersGuy = CreateButton(pGuy, RIGHT_MAP_BORDER, 242, "#", K_N);
	gSpacesBtnGuy = CreateButton(pGuy, RIGHT_MAP_BORDER + 15, 242, "CLUST", K_C);
	gLoadGuy = CreateButton(pGuy, RIGHT_MAP_BORDER, 258, "LOAD", K_L);
	gSaveGuy = CreateButton(pGuy, RIGHT_MAP_BORDER + 32, 258, "SAVE", K_S);

	MoveCursor(0, 0);
	DrawViewPort();
	DrawSideBar();
	DrawSpotInfo();
}

CEditor::CEditor(void)
{
	uint32		loop1;
	grip			gCel;
	uint			rNumCel;
	uchar			*pCelData;
	CelHeader	*pCel;
	Guy			*pGuy;

	gCurControl = NULL;
	ClusterListPos = 0;
	gSpacesGuy = NULL;
	gTypesGuy = NULL;	TypesListPos = 0;
	gSaveWindowCel = NULL;	// used on subsequent calls to EditTile()
	gSaveWindow = NULL;

	// create a single cel for use with all cluster markers
	TRACK_MEM("BottomBar"); gCel = ACreateCel(&rNumCel, 0, 0, TILE_WIDTH, TILE_HEIGHT, CI_SKIP, 1);

	// get address of cel's data area
	pCel = (CelHeader *)pResMgr->GetResData(gCel);		// get address of Resource->CelHeader
	pCelData = (uchar *)pCel + sizeof(CelHeader);
	memset(pCelData + TILE_WIDTH * pFontMgr->point, CI_WHITE, TILE_WIDTH);
	memset(pCelData + TILE_WIDTH * pFontMgr->point * 2, CI_WHITE, TILE_WIDTH);

	for(loop1 = 0; loop1 < MAX_CLUSTER_MARKERS; loop1++)
	{
		TRACK_MEM("ClusterMarker");		pGuy = new Guy;

		gClusterMarkers[loop1] = pGuy->gSelf;
		pGuy->SetRes(RES_CEL, rNumCel);
		pGuy->SetRect();
		pGuy->SetContext(gSelf);
		pGuy->Setup();
		pGuy->SetPri(300);
		pGuy->Hide();
	}
	
	gBackGuy = NULL;
	gMapCursor	= NULL;
	ViewX = 0;
	ViewY = 0;
	VPortMoving = FALSE;
	TileListStart = 0;
	ShowNumbers = FALSE;
}

CEditor::~CEditor()
{
	uint32	loop1;
	char		*szString;

	pMemMgr->HeapCheck();

	for(loop1 = 0; loop1 < MAX_CLUSTER_MARKERS; loop1++)
		ADelete(gClusterMarkers[loop1]);

	for(loop1 = 0;	loop1 < 3; loop1++)
	{
		szString = ADerefAs(char, gEditBoxBuffers[loop1]);
		AFree(gEditBoxBuffers[loop1]);
	}
	for(loop1 = 0;	loop1 < 25; loop1++)
	{
		AFree(gEditTileBuffers[loop1]);
	}

	ADelete(gOKBtn);
	ADelete(gUpGuy);
	ADelete(gDownGuy);
	ADelete(gNumbersGuy);
	ADelete(gTerrainGuy);
	ADelete(gTypesGuy);
	ADelete(gSpacesGuy);
	ADelete(gSpacesBtnGuy);
	ADelete(gLoadGuy);
	ADelete(gSaveGuy);
	ADelete(gBackGuy);
	ADelete(gEditGuy);

	ADelete(gMapCursor);
}

uint16
CountFreeGrips(void)
{
	uint32 NumFree = 0;

/*	for(loop1 = FIRST_STANDARD_GRIP; loop1 < STANDARD_GRIP_MAX; loop1++)
		if(pMemMgr->gripDataArray[loop1].ptr == NULL)
			NumFree++;*/
	return((uint16)NumFree);
}

long
motr2intl(long l)
{
	return(((l & 0xff000000L) >> 24) +
	       ((l & 0x00ff0000L) >> 8) +
	       ((l & 0x0000ff00L) << 8) +
	       ((l & 0x000000ffL) << 24));
}

int
motr2inti(int n)
{
	return(((n & 0xff00) >> 8) | ((n & 0x00ff) << 8));
}

bool
ReadLBMLine(FILE *infile, LBMBMHeader *pLBM, uchar *bitmap, uint32& scanLine)
{
	uchar		charRead;	//register short charRead;
	uint16	repCount;
	uint16	bytesToRead = pLBM->w;;
	uint16	scanChar = 0;

	do
	{	// Read a byte from the file - error if EOF
		charRead = (uchar) fgetc(infile);
		if (charRead == EOF)
		{
			return FALSE;
		}

		// See if we have a repeat tag
		if ((charRead & 0x80) && pLBM->compression)
		{
			repCount = (uint16) (((~charRead) & 0xff) + 2);
//			repCount = charRead - 0x80;
			charRead = (uchar) fgetc(infile);
//			if (charRead == EOF)
//			{
//				return FALSE;
//			}

			while (repCount--)
			{
				*(bitmap + scanLine + scanChar) = charRead;
				++scanChar;
			}
		}
		else
		{	repCount = (uint16) ((charRead & 0xFF) + 1);

			while(repCount--)
			{
				charRead = (uchar) fgetc(infile);
				*(bitmap + scanLine + scanChar) = charRead;
				++scanChar;
			}
		}
	} while (scanChar < bytesToRead);

	scanLine += pLBM->w;
//	pMono->Out("pLBM->h = %d\n", pLBM->h);
	return TRUE;
}

