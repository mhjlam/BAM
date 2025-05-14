// VIEWPORT - Class to handle map display for BAM
 
#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "tigre.hpp"
#include "scrimage.hpp"

#include "alldefs.hpp"
#include "tilelib2.hpp"

#define VPORT_WIDTH	14
#define VPORT_HEIGHT 10
#define VPORT_SIZE	(VPORT_WIDTH * VPORT_HEIGHT)
#define VPORT_PRI		1000	// base pri of all viewport entities

#define GANIMS_MAX	500

// offsets to base pri of ani's tileX/Y
#define PRI_FLOOR			0
#define PRI_SWIMMER		1000
#define PRI_WALKER		2000
#define PRI_ITEM			2001
#define PRI_PROJECTILE	3000
#define PRI_FLIER			4000
#define PRI_SKY			5000
#define PRI_VP_CURSOR	6000
#define PRI_INTERFACE	10000

#define	FOG_NULL		0x00
#define	FOG_SOUTH	0x01
#define	FOG_EAST		0x02
#define	FOG_NORTH	0x04
#define	FOG_WEST		0x08
#define  FOG_CENTER	0x10
#define	FOG_FRESH	0x20
#define	FOG_ALL		(FOG_SOUTH|FOG_EAST|FOG_NORTH|FOG_WEST)

#define ANIM_FENCE			9001
// cel1 == vert, cel2 == horiz

#define ANIM_CONFIRM_MOVE  8998
#define ANIM_CONFIRM_ATTACK 8999
#define ANIM_CURSOR			9000
#define ANIM_CURSOR_HAND	1
#define ANIM_CURSOR_MOVE	2
#define ANIM_CURSOR_ATTACK 3
#define ANIM_CURSOR_SKILL	4
#define ANIM_CURSOR_ITEM	5
#define ANIM_CURSOR_QUESTION 6
#define ANIM_CURSOR_PAUSE	7
#define ANIM_CURSOR_ERROR	8
#define ANIM_CURSOR_HAND2	9
#define ANIM_CURSOR_REPEAT 10

#define FIRE_LIFESPAN	(TICKS_PER_SEC*10)
#define FIRE_DAMAGE		1
#define FLAMESPOUT_DAMAGE 5
#define FLAMESPOUT_MAXWAIT	8
#define WEIRDWARD_LIFESPAN (TICKS_PER_SEC*30)
#define FEAR_DURATION	(TICKS_PER_SEC*30)
#define LURE_DURATION	(TICKS_PER_SEC*30)
#define STONED_DURATION (TICKS_PER_SEC*20)
// for group selection
enum fence_t
{ FENCE_TOP = 0, FENCE_BOTTOM, FENCE_LEFT, FENCE_RIGHT
};

class Unit;

enum action
{	NO_ACTION = 0, MOVE_TO, ATTACK, GUARD, DISBAND, SKILL, TRANSFORM, DROP,
		SUMMON, DIE, BERSERK, USE_ITEM, HEAL, FOLLOW_THE_LEADER, REPAIR, FOLLOW,
		TRANSFORM_TO_BLDG, AVOID, SCARE, WAIL, STEAL, ABORT, VENOM, GET_STONED,
		QUESTION, BLOWN_BACK, ADD_COSMETIC_ANI
};

enum animState_t
{
	ST_ANIM_NULL = 0, ST_ANIM_GUARD, ST_ANIM_WALK, ST_ANIM_ATTACK, ST_ANIM_ATTACK2, ST_ANIM_HIT,
	ST_ANIM_DIE, ST_ANIM_OBELISK, ST_ANIM_STONED, ST_ANIM_METAPOD
};

// each instance of AnimInfo describes settings for an animation to run in the owning BAM_Ani
struct AnimInfo
{
	animState_t		state;
	bool				fHFlip, fVFlip, fFwd, fCycle;
	uint32			resNum, speed;
};

class BAM_Ani : public Guy
{
	public:
							BAM_Ani(void);
							~BAM_Ani();
		virtual void	SetTilePos(uint32 newX, uint32 newY, uint32 newXOff = TILE_WIDTH / 2 - 1,
								uint32 newYOff = TILE_HEIGHT / 2 - 1);
		virtual void	SetTilePosOffset(uint32 newXOff, uint32 newYOff);
		virtual void	SmartCycle(bool fCycleEssential);
		virtual void	SetRect(coord x1, coord y1, coord x2, coord y2);
		virtual void	SetRect(void);
		void				SetLimits(coord x1, coord y1, coord x2, coord y2);
		virtual void	SetCel(uint celNum);
		virtual void	SetPos(coord L, coord T);
		virtual void	Show(void);
		virtual void	SetRes(res_t type, uint animResNum, uint iCel);
		virtual void	RunAnim(uint32 newResNum, bool fNewFwd, bool fNewCycle, uint32 newDelay);
		virtual void	RunAnimDelayed(ticks_t newTicks, uint32 newAni, bool fNewFwd, bool fNewCycle,
			uint32 newDelay);
		virtual void	PurgeAnimQeue(void);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char				aniDataStart;

//		uint32			serialNum;
		int32				priorityLevel;
		TargetType		selfType;	// self-identification (UNIT, TERRAIN, ITEM, etc)
		ticks_t	tDelayedAnim;	// if !0 then tick-count to start delayed anim at
		bool		fDelayedFwd, fDelayedCycle;
		uint32	delayedNewAni, delayedNewDelay;

		bool		fWorldMapUpdate;
		bool		fDeleteMe;
		bool		fCursorTracking;	// is viewport cursor locked to us?
		int32		tileX, tileY;				// pos in Tile coords, not pixels
		int32		lastTileX, lastTileY;	// needed for WorldMap to update properly
		int32		tileXOff, tileYOff;		//	pixel offset into Tile
		uint32	aniNum, gMasterCel, msgMask, animDelay;
		bool		fAniFwd, fAniCycle, fAniPermanent, fAniHideWhenDone, fAniDone, fFireSpread;
		ticks_t	cycleNext, tLastCycle, tAniStarted;
		Rectangle	rLimits;		// SetRect will always clip to this
		animState_t	currAnimState;

		// ani automatic motion
		void		MoveAnim(int32 newDestX, int32 newDestY, int32 newDestXOff, int32 newDestYOff,
						int32 newStepX, int32 newStepY, int32 newDelay), CycleMotion(void),
						MotionTakeNextTileStep(void);
		int32		motionDestX, motionDestY, motionDestXOff, motionDestYOff, motionStepX, motionStepY, motionRate;
		int32		motionStartX, motionStartY, motionStartXOff, motionStartYOff;

		// Bresnham algorythm stuff
  		int32		motionX0, motionY0, motionX1, motionY1;
		int32		motionLoopX, motionLoopY, motionXDel, motionYDel, motionYA, motionXA, motionNewX, motionNewY;
		int32		motionDeltaX, motionDeltaY, motionDDA;
		bool		fInMotion, fNeedCriticalCycleNow;
		ticks_t	tMotionNextStep;

		int32		pendingAnims;
		struct AnimInfo animInfo[3];
		char	 	aniDataEnd;
};

class TerrainUnit : public BAM_Ani
{
	public :
								TerrainUnit(void);
								~TerrainUnit();
		virtual void		SmartCycle(bool fEssentialOnly);
		virtual void		SetTilePos(uint32 newX, uint32 newY, uint32 newXOff = TILE_WIDTH / 2 - 1,
									uint32 newYOff = TILE_HEIGHT / 2 - 1);
		virtual bool		Save(uint16 state, FILE *pFile = NULL);

		char					tUnitDataStart;
		activeTerrain_t	activeType;
		ticks_t				tCycleNext;
		char					tUnitDataEnd;
};

class ViewPort : public Periodic
{
	public :
								ViewPort(void);
								~ViewPort();
		virtual bool		HandleMsg(Message *pMsg);
		void					Setup(grip gContext), Cleanup(void);
		void					MoveView(int32 NewViewX, int32 NewViewY, bool fSliding = FALSE),
									SetViewOffset(int xOff, int yOff), SetViewPos(int newX, int newY),
									SlideView(int tileX = 999, int tileY = 999, int newSlideIncr = 2);
		bool					EdgeScroll(uint32 mouseX, uint32 mouseY);
		grip					NewAni(res_t newType, uint32 newAni, uint32 newCel, int32 newPri,
									bool fNewFwd, bool fNewCycle, uint32 newDelay, uint32 newX, uint32 newY,
									bool fNewPerm,	uint32 newXOff = TILE_HORZ_CENTER,
									uint32 newYOff = TILE_VERT_CENTER);
		BAM_Ani				*NewTerrainUnit(activeTerrain_t newType, uint32 newX, uint32 newY);
		TerrainUnit			*GetTerrainUnit(int32 newX, int32 newY);
		bool					CheckAni(grip gAni);
		void					DeleteAni(grip gAni);
		void					SetLastVerb(action act, TargetType targType, uint32 targSerial, int x = 0, int y = 0);
		action				PickObviousVerb(Unit *pSrcUnit, Unit *pTargUnit, int targetX, int targetY);
		bool					SetCurrentVerb(action), ProcessVerb(uint32 mouseX = 0, uint32 mouseY = 0, int32 aux1 = NULL),
									DoObviousVerbProcess(int targetX, int targetY);
		void					MoveCursor(int32 NewCursorX, int32 NewViewY, bool fJustTargeting = FALSE),
									UpdateTrailCursor(void), DrawTrailCursor(void), RestoreSourceAsTarget(void);
		void					SetCursorPos(int32 NewCursorX, int32 NewCursorY,
									int32 newXOff = TILE_WIDTH / 2 - 1, int32 newYOff = TILE_HEIGHT / 2 - 1);
		void					RepositionAni(BAM_Ani *pAni);
		void					ClipCursorPos(void);	// insure that cursor is position within world map boundaries

		void					SetCursorTarget(TargetType newTargType, uint32 newSerial, coord newX, coord newY);
		void					SetCursorTarget(TargetType newTargType, coord newX, coord newY);
		void					SetCursorTarget(TargetType newTargType, uint32 newSerial, Guy *pAltCursor = NULL);
		void					GoToTarget(void);
		void					Defogger(int32 x1, int32 y1, int32 x2, int32 y2), DefogAll(uint32 side);
		void					DrawCursor(void), SwapMapTile(int32 xPos, int32 yPos, int newTileNum = -1);
		virtual void		Draw(void), Draw(int32 xPos, int32 yPos),
									Draw(int32 x1, int32 y1, int32 x2, int32 y2, bool fUpdateRect = TRUE);
		bool					InViewPort(int32 x, int32 y, TargetType target = TERRAIN),
									SetRepeatCommandMode(bool fNewState);
		void					AddAni(grip gNewBAMAni), DumpGAnims(void);
		void					CutAni(grip gBAMAni);
		void					PurgeAnis(void);
		virtual void		Cycle(void);
		void					UpdateTracking(void);
		void					CleanupGroupCursors(void), RemoveUnitFromGroup(uint32 serialNum);
		void					StartFencing(void), EndFencing(int mouseX, int mouseY), AbortFencing(void);
		void					SetVerbBuzzer(int errNum = NULL);
		virtual bool		Save(uint16 state, FILE *pFile = NULL);

		char			vpDataStart;
		int32			fog[WORLD_HEIGHT][WORLD_WIDTH];	// fog-of-battle		              
		grip			activeTerrain[WORLD_WIDTH][WORLD_HEIGHT];	// flame spouts, fire, etc
		uint32		fogTile;		// solid black tile for this map
		grip			gAnims[GANIMS_MAX];		// list of landscape ANIs (portals, fountains, etc)
		int			highestAnim, lastSerialCycled, unitsPerFrame;
		List			lScreenAnims;	// list of "" which are on-screen right now
		int32			ViewX, ViewY, ViewX2, ViewY2, viewXOffset, viewYOffset, viewXDest, viewYDest,
							prevSlideX, prevSlideY, slideIncr, CursorX, CursorY, CursorXOff, CursorYOff,
							trailCursorX, trailCursorY, lastViewX, lastViewY, lastViewX2, lastViewY2;
		Rectangle	rect, rInner;
		grip			gItemCursorCel;
		uint			rNumItemCursorCel;
		grip			gViewPortCel;		// our DCEL - kept around so we can change it without re-alloc
		uint			rNumViewPortCel, rMapTitleCel;
		grip			gViewPortGuy, gMapTitleGuy;
		grip			gViewPortCursor, gVPTrailCursor, gVPFenceGuys[4];
		int			fogAnim[13][2], turretTiles[16], groupSize;
		uint32		sGroup[VPORT_SIZE];

		// stuff for handling main screen verb buttons
		action			currentVerb,oldCurrentVerb;
		uint32			verbSourceSerial, verbSourceX, verbSourceY;
		TargetType		verbSourceType;

		TargetType		targType;						// type of thing currently in stat box
		uint32			targSerial, targX, targY;	// other info as necessary
		bool				fAutoCommandMode, fRepeatCmdMode, fSnapToUnits, fFireSpread;
		int32				mouseTileX, mouseTileY, skipViewPortCycle;

		// for RepeatCmd mode
		ticks_t			tMouseResReset;
		uint				rPrevMouseRes, prevMouseCel;
		res_t				tPrevMouseResType;
		action			lastVerb;
		TargetType		lastVerbType;
		uint32			lastVerbSerial;
		int				lastVerbX, lastVerbY;

		int fenceStartX, fenceStartY;
		bool	fFenceMode;
		char	vpDataEnd;
};

#endif
