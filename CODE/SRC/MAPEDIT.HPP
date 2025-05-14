#ifndef MAPEDIT_HPP
#define MAPEDIT_HPP

#include "mapsbase.hpp"

#define	MAP_VPORT_WIDTH		(VPORT_WIDTH - 2)
#define	MAP_VPORT_HEIGHT		(VPORT_HEIGHT - 2)	

class CEditor : public Context
{
	public:
		CEditor(void);
		~CEditor();
		virtual void	Setup(void);
		virtual bool	HandleMsg(Message *pMsg);
		virtual void	Cycle(void);
		void				SetCurrentControl(grip gNewControl);
		void				MoveTileList(int move), ListSpaces(int num);
		void				ListTerrains(int num);
		void				MoveCursor(coord xPos, coord yPos), MarkCluster(uint32 SpaceNum);
		void				DrawSideBar(void);
		void				DrawSpotInfo(void);

		void				SaveAll(void), TileLibReport(void);
		void				LoadAll(int newMapResNum), RefreshPics(uint tileRes), ParseNewTileInfo(uint rTileResNum);
		grip				CreateButton(Guy *, int, int, char *, int);
		grip				CreateBox(Guy *pWin, int xCoord, int yCoord, grip gText, int maxLen, uint32 newReplyMask = NULL);
		void				DrawViewPort(void), DrawViewPort(coord xPos, coord yPos), MoveViewPort(coord xPos, coord yPos);
		void				EditTileNum(coord xPos, coord yPos), EditTile(coord CursorX, coord CursorY), EndEditTile(void),
								AcceptEditorChanges(void), EditTileSetupTerrain(void), EditTileSetupCluster(int),
								EditTileSetupTitle(void);
		Guy				*GetWindowUp(void);

		TPalette			palette;

		uint			rTileResNum, rMapResNum, rBackground;
		grip			gBackGuy, gTypesGuy, gSpacesGuy, gSpacesBtnGuy, gNumbersGuy, gUpGuy, gDownGuy, gCurControl,
							gLoadGuy, gSaveGuy, gTerrainGuy, gEditGuy, gOKBtn;
		grip			gEditBox[3], gEditBoxBuffers[3];
		grip			gEditTileBox[25], gEditTileBuffers[25];
		tile			tile1;
		Terrain		terrain1;
		MapSpaceBase space1;

		Rectangle	rViewport;

		coord			ViewX, ViewY, CursorX, CursorY;
		int16			VPortMoving, TileListStart;
		grip			gMapCursor, gClusterMarkers[MAX_CLUSTER_MARKERS];	// guys - box cursor in map viewport, cluster markers too
		grip			gMap;
		uint32		ClusterListPos;
		uint32		TypesListPos;
		bool			ShowNumbers;

		grip			gSaveWindow;
		grip			gSaveWindowCel; // used on subsequent calls to EditTile()
		uint			rNumSaveWindowCel;
};

void	BorderizeCel(grip gCel);

#define CLUSTER_LIST_SIZE	5
#define TYPES_LIST_SIZE	5

class CEditTile : public Context
{
	public:
		CEditTile(uint16 TileNbr, uint16 SpaceNbr, uint16 MapPosNbr, uint16 xPosNbr, uint16 yPosNbr);
		~CEditTile();
		virtual bool	HandleMsg(Message *pMsg);
		void				SetupSpace(void), ShutDown(void), SetupLabels(void);
		void				SetupGroup(bool Init);
		void				SetupRandomItem(void), SetupSpecialItem(void), SetupSpaceFunc(void),
								SetupSpaceHot(void), ClearGrips(void), SetupGroupColor(void),
								SetupGroupBurnable(void);
		virtual void	Setup(bool fSetupTile, bool fSetupGroup, bool fSetupSpace);

		uint32			TileNum, SpaceNum, MapPos, xPos, yPos;
		Tile				currTile;
		TileGroup		currGroup;
		MapSpaceBase	currSpace;
		char				szGroupNumText[5], szSwapTileText[5], szGroupDescr[30],
							szGroupMove[5], szGroupDef[5], szGroupAtt[5], szGroupDmg[5],
							szGroupRng[5], szGroupAnim[5], szGroupAnimDelay[5], szGroupPortrait[5];
		char				szSpaceNum[5], szSpaceX[5], szSpaceY[5], szSpaceFunc[15],
								szSpaceHot[5], szItem[5], szHotX[5], szHotY[5], szDestX[5], szDestY[5], szOwner[3],
								szMiniColor[5], szSpaceHP[5];
		grip				gDialog, gNewGroup, gGroupNum, gGroupBurn,
							 gNewSpace, gSpaceNum, gSpaceX, gSpaceY, gSpaceFunc,
							 gSpaceHot, gItem, gRandomItem, gSpecialItem,
							 gHotX, gHotY, gDestX, gDestY, gOwner, gMiniColor;

		uint32			BoxCount;
		grip				gBoxes[30];
};

// LBM BitMap header
typedef struct
{
	unsigned short w, h;
	short 	x, y;
	char 		nPlanes;
	char 		masking;
	char 		compression;
	char 		pad1;
	unsigned short transparentColor;
	char 		xAspect, yAspect;
	short 	pageW, pageH;
} LBMBMHeader;

long	motr2intl(long l);
int	motr2inti(int n);
bool	ReadLBMLine(FILE *infile, LBMBMHeader *pLBM, uchar *bitmap, uint32& scanLine);

#endif
