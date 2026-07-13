// BAM Tile Library/Map structure stuff - for use with TILELIB, MAPEDIT, and BAM
#ifndef TILELIB2_HPP
#define TILELIB2_HPP

#define	TILE_WIDTH			20
#define	TILE_HEIGHT			38
#define	TILE_HORZ_LEFT		0
#define	TILE_HORZ_RIGHT		(TILE_WIDTH - 1)
#define	TILE_HORZ_CENTER	(TILE_WIDTH / 2 - 1)
#define	TILE_VERT_TOP		0
#define	TILE_VERT_BOTTOM	(TILE_HEIGHT - 1)
#define	TILE_VERT_CENTER	(TILE_HEIGHT / 2 - 1)
#define	TILE_SIZE 			(TILE_WIDTH*TILE_HEIGHT)
#define	MAX_TILE_GROUPS	900
#define	MAX_TILES			1500

//----------------------
// Tile Groups contain information common to all tiles assigned to that group
// example: all road tiles have the same movement rate.
struct TileGroupHeader
{
	char		descr[40];		// just some name
	uint16	moveRate;		// 0 for obstacles, x>0 for open terrain
	uint16	dummy2;			//	for buildings
	uint16	dummy1;
	int16		attackMod;		// +/- to attacking
	int16		attackRangeMod;		// +/- to attack range
	int16		defenseMod;		// +/- to defending
	int16		damageVal;		// damage to units passing through
	char		watcomPad7[2];
	uint32	miniColor;		// fill color to use in mini-map
	uint32	burnable;		// can be burnt by fire?
	uint32	portrait;		// ANI # for portrait display
	uchar		pad[244];
};

class TileGroup : public TMovable
{
public:
	struct TileGroupHeader h;

	TileGroup(void);
	~TileGroup(void);
	bool Write(FILE *output);
};

// Tiles themselves still have some individual info
struct TileHeader
{
	uint16	GroupNum;
	uchar		BitMap[TILE_WIDTH * TILE_HEIGHT];
	uint16	swapTile;		// swap with tile X when destroyed
	uint32	animTile, animDelay;		// # of ANI to run on top of tile.  animDelay == # of ticks per frame
	uchar		pad[248];
};

class Tile : public TMovable
{
public:
	struct TileHeader h;

	Tile(void);
	~Tile(void);
	bool Write(FILE *output);
	bool Read(FILE *input);
	bool LoadImage(uchar *SrcBitMap, uint16 SrcBitMapWidth, uint16 cel_x, uint16 cel_y);
};
//----------------------

// header for library itself
struct TileLibraryHeader	
{
	char Descr[40], LibName[40];
	uint16 totalTiles, totalTileGroups;
	uchar palette[768];
	uchar pad[256];
};

class TileLibrary : public TMovable
{
public:
	struct TileLibraryHeader h;
	grip gTileGroup[MAX_TILE_GROUPS], gTile[MAX_TILES], gPal;
	
	TileLibrary(void);
	~TileLibrary(void);
	void Init(void);
	bool	LoadLibrary(char LibName[]);		// load an existing library
	void	RefreshPics(void);					// load/reload tile images from LBM
	bool	SaveLibrary(char fileName[]);		// save tile library with new name
	bool	SaveLibrary(void);						// save tile library 
	uint16	AddGroup(void);
	void		Report(void), ParseNewTileInfo(void);

	uint32	fogTile;	// tile # for all-black fog
};

#endif
