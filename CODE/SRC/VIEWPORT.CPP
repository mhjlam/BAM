//	Copyright 1994, Tachyon, Inc.
//
// Viewport.cpp
//
//				 

#include <string.h>

#include "manager.hpp"
#include "eventmgr.hpp"
#include "periodic.hpp"
#include "context.hpp"
#include "api.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apires.hpp"
#include "fontmgr.hpp"
#include "tigre.hpp"
#include "scrimage.hpp"
#include "mouse.hpp"
#include "mouseint.hpp"
#include "resource.hpp"
#include "debug.hpp"
#include "savebase.hpp"

#include "bam.hpp"
#include "encyclo.hpp"
#include "tiles.hpp"
#include "viewport.hpp"
#include "world.hpp"
#include "worldmap.hpp"

#ifdef NDEBUG
//#undef NDEBUG
#endif

extern void MouseHandler( int draw_mouse );
extern BAM_Application	*pBam;
extern World				*pWorld;
extern bool	fShowCurrentUnitPath;
extern void ShowUnitPath(Unit *pUnit);
extern void	ReportFreeMem(void);
extern Debugger	BamDebug;

bool
BAM_Ani::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&aniDataStart, 1, (int)&aniDataEnd -
				(int)&aniDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&aniDataStart, 1, (int)&aniDataEnd -
				(int)&aniDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
BAM_Ani::PurgeAnimQeue(void)
{
	pendingAnims = 0;
}

void
BAM_Ani::SetRes(res_t type, uint animResNum, uint iCel)
{
	if(animResNum)
	{
		fAniDone = FALSE;
		Guy::SetRes(type, animResNum, iCel);
		SetRect();	// necessary because Guy::SetAnim() calls scrim.SetRect() directly, not via
						// Guy::SetRect()
		tAniStarted = ATicks();
	}
	else
	{
		fAniDone = TRUE;
		scrim.resNum = NULL;
	}
}

// delayed method of launching an anim
void
BAM_Ani::RunAnimDelayed(ticks_t newTicks, uint32 newAni, bool fNewFwd, bool fNewCycle, uint32 newDelay)
{
	tDelayedAnim = newTicks;
	delayedNewAni = newAni;
	fDelayedFwd = fNewFwd;
	fDelayedCycle = fNewCycle;
	delayedNewDelay = newDelay;
}

void
BAM_Ani::RunAnim(uint32 newResNum, bool fNewFwd, bool fNewCycle, uint32 newDelay)
{
	bool		fContinueSameAni;

//	pMono->Out("RunAnim(ani %d, %d ticks)\n", newAni, newDelay);
	if(scrim.resNum != newResNum || fNewFwd != fAniFwd || fNewCycle != fAniCycle || (!fNewCycle))
	{
		SetRes(RES_ANIM, newResNum, 1);
		if(!fNewFwd)
			SetRes(RES_ANIM, newResNum, scrim.numCels);

		if(!CheckState(S_HIDDEN))		// are we on-screen?
			AUpdateRect(&scrim.rect);
		fContinueSameAni = FALSE;
	}
	else
	{
		fContinueSameAni = TRUE;
		if(fAniDone == TRUE)	// if current anim was stopped
			fAniDone = FALSE;		// just continue it
	}
	fAniFwd = fNewFwd;		// cycle cels forward?
	fAniCycle = fNewCycle;	// wrap around?
	animDelay = newDelay;	// ticks per cel

//	if(fContinueSameAni)
//		BAM_Ani::Cycle();

	cycleNext = ATicks() + animDelay;

	if(tDelayedAnim) 						// if one was waiting
		tDelayedAnim = NULL;				// nevermind
}

void
BAM_Ani::SmartCycle(bool fEssentialOnly)
{
	if(fEssentialOnly);

	ticks_t currTicks = ATicks();

	if(tLastCycle == currTicks)	// if already cycled this tick
		return;								// avoid redundant cycling

#ifdef DEBUG_BIG_NET
	pMono->Out("A(g%d):C() ", gSelf);
#endif

	tLastCycle = currTicks;

	Guy::Cycle();

	if(tDelayedAnim && currTicks >= tDelayedAnim)	// time to start delayed anim
	{
		tDelayedAnim = NULL;
		RunAnim(delayedNewAni, fDelayedFwd, fDelayedCycle, delayedNewDelay);
	}

	if(fInMotion)
		CycleMotion();

	if(fAniDone)	// if no current animation running
	{
		// check animation qeue for the next one to start, if any
		AnimInfo	*pInfo;

		// get the first qeue entry
		if(pendingAnims)
		{
			// start it
			pInfo = &animInfo[0];

			RunAnim(pInfo->resNum, pInfo->fFwd, pInfo->fCycle, pInfo->speed);
			if(pInfo->fHFlip)
				Flip(S_FM_HORZ, TRUE);
			else Flip(S_FM_HORZ, FALSE);
			if(pInfo->fVFlip)
				Flip(S_FM_VERT, TRUE);
			else Flip(S_FM_VERT, FALSE);

			currAnimState = pInfo->state;	// ENUM which describes current animation. eg. ST_ANIM_WALK

			// delete the entry from the qeue
			memmove(&animInfo[0], &animInfo[1], sizeof(struct AnimInfo) * 2);
			pendingAnims--;			
		}
		// else nothing to do
	}
	else	// else an animation is in progress
	{
		// an animation is currently running - cycle it
		if(currTicks >= cycleNext)
		{
			// automatic movement handled here.
			if(animDelay == 0)	// no cycling to occur
			{
				fAniDone = TRUE;
			}
			else
			{
				cycleNext = currTicks + animDelay;
				if(fAniFwd)		// if ani runs forward
				{
					if(scrim.cel == scrim.numCels || !animDelay)
					{
						// if ANI is to re-cycle
						if(fAniCycle)
						{
							SetCel(1);	// then start over
						}
						else
						{
							fAniDone = TRUE;
						}
					}
					else SetCel(scrim.cel + 1);	// increment to next cel
				}
				else		// run ani backwards
				{
					if(scrim.cel <= 1)
					{
						if(fAniCycle)			// if ani is to re-start
							SetCel(scrim.numCels);		// go back to last cel
						else
							fAniDone = TRUE;				// end animation
					}
					else SetCel(scrim.cel - 1);		// else decrement to next cel
				}
//				if(selfType == UNIT)
//					if(((Unit *)this)->player == pWorld->playerSide)
//						pMono->Out("BAM_Ani::Cycle() - %d ticks, cel=%d\n", currTicks, scrim.cel);

				if(fAniDone)
				{
					if(!fAniPermanent)
					{						
						fDeleteMe = TRUE;		// we're temporary, and finished.  Delete ourself.

						#ifdef DEBUG_BIG_NET
						pMono->Out("BAM_Ani()::Cycle() fDeleteMe, done\n");
						#endif

						return;
					}
					if(fAniHideWhenDone)
					{
						SetTilePos(-1, -1);
						AUpdateRect(&scrim.rect);
					}		
				}

				if(!CheckState(S_HIDDEN))		// are we on-screen?
					AUpdateRect(&scrim.rect);
			}
		}
	}
#ifdef DEBUG_BIG_NET
	pMono->Out("done, ");
#endif
}

// start this ANI moving
void
BAM_Ani::MoveAnim(int32 newDestX, int32 newDestY, int32 newDestXOff, int32 newDestYOff, int32 newStepX,
	int32 newStepY, int32 newDelay)
{
	int32	pixelCount;

	motionStartX = tileX;
	motionStartY = tileY;
	motionStartXOff = tileXOff;
	motionStartYOff = tileYOff;

	// target X,Y
	// normalize coords within valid limits
	pixelCount = newDestX * TILE_WIDTH + newDestXOff;
	motionDestX = pixelCount / TILE_WIDTH;
	motionDestXOff = pixelCount % TILE_WIDTH;

	pixelCount = newDestY * TILE_HEIGHT + newDestYOff;
	motionDestY = pixelCount / TILE_HEIGHT;
	motionDestYOff = pixelCount % TILE_HEIGHT;

	// step size
	motionStepX = newStepX;
	motionStepY = newStepY * 2;

	motionRate = newDelay;		// ticks per cycle
	tMotionNextStep = ATicks();
	fInMotion = TRUE;

	// set up vars for Bresnham routine
	motionX0 = tileX * TILE_WIDTH + tileXOff;
	motionY0 = tileY * TILE_HEIGHT + tileYOff;
	motionX1 = motionDestX * TILE_WIDTH + motionDestXOff;
	motionY1 = motionDestY * TILE_HEIGHT + motionDestYOff;
	motionYA = motionY0;
	motionXA = motionX0;
	if (motionY0 < motionY1)
	{
		motionYDel = 1;
	} else
	{
		motionYDel = -1;

		motionY0 ^= motionY1;
		motionY1 ^= motionY0; // From Russell to piss YOU off
		motionY0 ^= motionY1;
	}

	if (motionX0 < motionX1)
	{
		motionXDel = 1;
	} else if (motionX0 > motionX1)
	{
		motionXDel = -1;
		motionX0 ^= motionX1;
		motionX1 ^= motionX0;
		motionX0 ^= motionX1;
	}
	motionLoopY = motionY0;
	motionDeltaX = motionX1 - motionX0;
	motionDeltaY = motionY1 - motionY0;
	motionDDA = motionDeltaX;
	motionLoopY = motionY0;
	motionLoopX = motionX0;
}

void
BAM_Ani::MotionTakeNextTileStep(void)
{
	int	xDif;

	int32	newTileX, newTileY, newXOff, newYOff;
	int	stepCountX, stepCountY;

	if (motionX0 == motionX1)
	{
		// vertical line
		stepCountY = 0;
		for(; motionLoopY <= motionY1 && stepCountY < motionStepY; motionLoopY++, motionYA += motionYDel, stepCountY++)
		{
			motionNewX = motionX0;
			motionNewY = motionYA;
		}
		newTileX = motionNewX / TILE_WIDTH;
		newXOff = motionNewX % TILE_WIDTH;
		newTileY = motionNewY / TILE_HEIGHT;
		newYOff = motionNewY % TILE_HEIGHT;

		if(newTileX != tileX || newTileY != tileY)
			SetTilePos(newTileX, newTileY, newXOff, newYOff);
		else
		{
			SetTilePosOffset(newXOff, newYOff);
			pWorld->vPort.RepositionAni(this);
		}	
	}
	else
	{
		stepCountX = 0;
		stepCountY = 0;
		for(; motionLoopX <= motionX1 && motionLoopY <= motionY1 && stepCountX < motionStepX && stepCountY < motionStepY;
			motionLoopX++, motionXA += motionXDel, stepCountX++)
		{
			motionNewX = motionXA;
			motionNewY = motionYA;
	
			motionDDA -= motionDeltaY;
			if (motionDDA <= 0)
			{
				motionDDA += motionDeltaX;
				motionLoopY++;
				motionYA += motionYDel;
				stepCountY++;
	
				while (motionDDA <= 0 && motionLoopY <= motionY1 && stepCountY < motionStepY)
				{
					motionNewX = motionXA;
					motionNewY = motionYA;

					motionDDA += motionDeltaX;
					motionLoopY++;
					motionYA += motionYDel;
					stepCountY++;
				}
			}
		}
	}

//	if(selfType == TERRAIN)
//	{
//		FILE *pFile;
//		pFile = fopen("temp.tmp", "a");
//		fprintf(pFile, "Proj(g%04d): loopX/Y %4d/%4d\n", gSelf, motionLoopX, motionLoopY);
//		fprintf(pFile, "                     %4d/%4d\n", motionX1, motionY1);
//		fclose(pFile);
//	}

	// minor error correction here - dont allow travel past dest X,Y
	// 0/1 pairs are already sorted low/high
	if(motionLoopX > motionX1)
	{
		motionLoopX = motionX1;
		motionXA = motionDestX * TILE_WIDTH + motionDestXOff;
		motionNewX = motionXA;
	}
	if(motionLoopY > motionY1)
	{
		motionLoopY = motionY1;
		motionYA = motionDestY * TILE_HEIGHT + motionDestYOff;
		motionNewY = motionYA;
	}

	// alright, where are we now?
	newTileX = motionNewX / TILE_WIDTH;
	newXOff = motionNewX % TILE_WIDTH;
	newTileY = motionNewY / TILE_HEIGHT;
	newYOff = motionNewY % TILE_HEIGHT;

	// cross a tile boundary?
	if(newTileX != tileX || newTileY != tileY)
		SetTilePos(newTileX, newTileY, newXOff, newYOff);
	else
	{
		if(newXOff != tileXOff || newYOff != tileYOff)
		{
			SetTilePosOffset(newXOff, newYOff);
			pWorld->vPort.RepositionAni(this);
		}
		else
		{
			// no movement occured
		}
	}
	xDif = abs(motionX0 - motionNewX);
//	pMono->Out("xDif==%d\n", xDif);
	return;
}

// check for time to take another step
void
BAM_Ani::CycleMotion(void)
{
	ticks_t	currTicks = ATicks();

	if(!fInMotion)
		return;

	if(currTicks >= tMotionNextStep)
	{
		MotionTakeNextTileStep();
//		pMono->Out("BAM_Ani::CycleMotion() - %d ticks, pos=%d/%d %d/%d\n", currTicks, tileX, tileXOff,
//			tileY, tileYOff);

		// reached dest X,Y yet?
		if(tileX == motionDestX && tileY == motionDestY && tileXOff == motionDestXOff && tileYOff == motionDestYOff)
		{
			fInMotion = FALSE;

			// request immediate rethink
//			fNeedCriticalCycleNow = TRUE;
//			if(selfType == UNIT)
//				((Unit *)this)->tNextStateCheck = currTicks;
//			cycleNext = currTicks;
		}
		else tMotionNextStep = currTicks + motionRate;
	}
}

void
BAM_Ani::Show(void)
{
	if(!scrim.resNum)
		return;
	Guy::Show();
}

void
BAM_Ani::SetCel(uint celNum)	
{
	Guy::SetCel(celNum);
	SetRect();		// Guy::SetCel() goes straight to scrim.SetRect(), so we do this for clipping to viewport
}

void
BAM_Ani::SetLimits(coord x1, coord y1, coord x2, coord y2)
{
	rLimits.Set(x1, y1, x2, y2);
}

void
BAM_Ani::SetRect(coord x1, coord y1, coord x2, coord y2)
{
	if(!scrim.resNum)
		return;

	scrim.SetRect(x1, y1, x2, y2);

	// clip to viewport rect
	scrim.clipRect.Clip(&rLimits);
}

void
BAM_Ani::SetRect(void)
{
	if(!scrim.resNum)
		return;

	Guy::SetRect();

	// clip to viewport rect
	scrim.clipRect.Clip(&rLimits);
}

BAM_Ani::BAM_Ani(void)
{
	// automatic motion vars

	fNeedCriticalCycleNow = FALSE;
	pendingAnims = 0;
	fFireSpread = FALSE;
	priorityLevel = PRI_FLOOR;	// offset to base pri of ani's tileX/Y
	fInMotion = FALSE;

	tDelayedAnim = 0;
	fDelayedFwd = FALSE;	fDelayedCycle = FALSE;
	tAniStarted = 0;	// last time SetAnim() was called

	delayedNewAni = 0;	delayedNewDelay = 0;
	currAnimState = ST_ANIM_NULL;

	fCursorTracking = FALSE;
	fAniHideWhenDone = FALSE;
	fDeleteMe = FALSE;
	selfType = TERRAIN;	// default - will be overridden by subclassed constructors where applicable
	tileX = -1;
	tileY = -1;
	lastTileX = -1;
	lastTileY = -1;
	fWorldMapUpdate = FALSE;
	tileXOff = 0;
	tileYOff = 0;
	aniNum = 0;
	fAniFwd = TRUE;	fAniCycle = TRUE;		fAniPermanent = TRUE;	fAniDone = TRUE;
	animDelay = 0;
	gMasterCel = 0;
	msgMask = NULL;
	cycleNext = 0;
	tLastCycle = 0;

	Rectangle *prTemp = &pWorld->vPort.rInner;
	SetLimits(prTemp->x1, prTemp->y1, prTemp->x2, prTemp->y2);
//	SetLimits(0, 0, TILE_WIDTH * VPORT_WIDTH - 1, TILE_HEIGHT * VPORT_HEIGHT - 1);
}

BAM_Ani::~BAM_Ani()
{
}

void
BAM_Ani::SetTilePosOffset(uint32 newXOff, uint32 newYOff)
{
	tileXOff = newXOff;
	tileYOff = newYOff;
}

void
BAM_Ani::SetTilePos(uint32 newX, uint32 newY, uint32 newXOff, uint32 newYOff)
{
	lastTileX = tileX;
	lastTileY = tileY;
	fWorldMapUpdate = TRUE;
	tileX = newX;
	tileY = newY;
	SetTilePosOffset(newXOff, newYOff);
	pWorld->vPort.CheckAni(gSelf);		// have viewport check our on/off screen status, since we've changed tile position
}
// dont call Guy::SetPos() directly - that will be done by Viewport::MoveView()
																						 
void
BAM_Ani::SetPos(coord L, coord T)
{
	Guy::SetPos(L, T);
	SetRect();	// ^ only calls scrim.SetRect() directly, but ours does extra rect clipping
}

//---------------------------------------------------
// class TerrainUnit
// any kind of terrain which performs a complex function
// (fire, flamespouts, etc)

TerrainUnit::TerrainUnit(void)
{
	tCycleNext = ATicks();
//	Debug.Out("TerrainUnit:: g%d born @ %d ticks\n", gSelf, tCycleNext);
}

TerrainUnit::~TerrainUnit()
{
	// if X,Y valid
	if(tileX >= 0 && tileY >= 0)
		if(pWorld->vPort.activeTerrain[tileX][tileY] == gSelf)
			pWorld->vPort.activeTerrain[tileX][tileY] = NULL;
}

bool
TerrainUnit::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&tUnitDataStart, 1, (int)&tUnitDataEnd -
				(int)&tUnitDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&tUnitDataStart, 1, (int)&tUnitDataEnd -
				(int)&tUnitDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
TerrainUnit::SetTilePos(uint32 newX, uint32 newY, uint32 newXOff, uint32 newYOff)
{
	// if old coords valid
	if(tileX >= 0 && tileY >= 0)
		if(pWorld->vPort.activeTerrain[tileX][tileY] == gSelf)
			pWorld->vPort.activeTerrain[tileX][tileY] = NULL;
	BAM_Ani::SetTilePos(newX, newY, newXOff, newYOff);
	pWorld->vPort.activeTerrain[tileX][tileY] = gSelf;
}

void
TerrainUnit::SmartCycle(bool fEssentialOnly)
{
	if(fEssentialOnly);

	Terrain		*pTerrain;
	ticks_t		currTicks = ATicks();
	Unit			*pUnit;
	MapSpace		*pBldg;
	int			delay, damage;
	uint32		xPos, yPos;
#ifdef DEBUG_BIG_NET
	pMono->Out("TerrainUnit(g%d)::Cycle() @ %d ticks\n", gSelf, currTicks);
#endif

	BAM_Ani::SmartCycle(FALSE);

	if(selfType != TERRAIN)
	{
		#ifdef DEBUG_BIG_NET
		pMono->Out("TerrainUnit()::Cycle() !TERRAIN, done\n");
		#endif
		return;
	}

	switch(activeType)
	{
		case ACTIVE_FLAME_SPOUT:
//			if(scrim.scale != 256) // if special scaling occuring
//			{
//				SetScale(scrim.scale + 8);
//			}

			if(!fAniDone)
				break;

			if(currTicks >= tCycleNext)
			{
				switch((terrainType)pWorld->map.GetTerrainNum(tileX, tileY))
				{
					case CHASM:
					case BRIDGE:
						Hide();
						fDeleteMe = TRUE;
						return;

					case FLAME_SPOUT:
						break;

					default:
						break;
				}

				RunAnim(ANIM_FLAME_SPOUT, TRUE, FALSE, 3);
				pWorld->vPort.CheckAni(gSelf);
				pWorld->LaunchSound(SND_FLAME_SPURT, tileX, tileY);
				delay = FLAMESPOUT_MAXWAIT * TICKS_PER_SEC;
				tCycleNext = ATicks() + ARandom(delay);
				pUnit = pWorld->unitLib.GetUnit(tileX, tileY);
				if(pUnit)
				{
					pUnit->TakeAHit(this, UNIT_FIRE, tileX, tileY, FLAMESPOUT_DAMAGE, FALSE);
				}
			}
			else if(scrim.resNum)
			{
				Hide();
				scrim.resNum = NULL;
			}
			break;

		case ACTIVE_FIRE:
		case ACTIVE_ARCH_FIRE:
			if(currTicks >= tCycleNext)
			{
				unitType	damageType;
				
				if(activeType == ACTIVE_ARCH_FIRE)
				{
					damageType = UNIT_ARCH_FIRE;
					damage = FIRE_DAMAGE * 2;
				}
				else
				{
					damageType = UNIT_FIRE;
					damage = FIRE_DAMAGE;
				}

				// fire burns any unit in its space once per sec
				tCycleNext = currTicks + TICKS_PER_SEC;
				pUnit = pWorld->unitLib.GetUnit(tileX, tileY);
				if(pUnit)
					pUnit->TakeAHit(NULL, damageType, tileX, tileY, damage, FALSE);
				pBldg = pWorld->map.GetStructure(tileX, tileY);
				if(pBldg && pBldg->h.currHP > 0)
					pBldg->TakeAHit(NULL, damageType, tileX, tileY, damage, tileX, tileY);
			}

			if((currTicks - tAniStarted) > FIRE_LIFESPAN)
			{
				if(fFireSpread)
				{
					if(pWorld->map.FindBurnableXY(&xPos, &yPos))
					{
						SetTilePos(xPos, yPos);

						// tweak birth time, so that fire death isnt synchronized
						tAniStarted = currTicks - ARandom(TICKS_PER_SEC);

						// tweak re-cycle time so that not all structure hits are synchronized
						tCycleNext = currTicks + TICKS_PER_SEC + ARandom(TICKS_PER_SEC);
					}
					else fDeleteMe = TRUE;	// nowhere left to burn
				}
				else
				{
					// check terrain @ X,Y
					if((terrainType)pWorld->map.GetTerrainNum(tileX, tileY) != HOTBED)
					{
						if(!pWorld->map.FireCanBurnAt(tileX, tileY))
							fDeleteMe = TRUE;	// force immediate termination
						else
						{
							// after fire ani finishes its next complete animation cycle, it will die.
							fAniCycle = FALSE;
						}
					}
					// else hotbed, but has it been vacated?
					else if(!pWorld->unitLib.GetUnitGrip(tileX, tileY))
					{
						// after fire ani finishes its next complete animation cycle, it will die.
						fAniCycle = FALSE;
					}
		 			else
					{
						// occupied hotbed, continue burning

						// tweak birth time, so that fire death isnt synchronized
						tAniStarted = currTicks - ARandom(TICKS_PER_SEC);

						// tweak re-cycle time so that not all structure hits are synchronized
						tCycleNext = currTicks + TICKS_PER_SEC + ARandom(TICKS_PER_SEC);
					}
				}
				pTerrain = pWorld->map.GetTerrain(tileX, tileY);
				if(pTerrain->burnable)
				{
					//swap to burnt/destroyed tile
					pWorld->vPort.SwapMapTile(tileX, tileY);
					if(pWorld->vPort.targType == TERRAIN &&
						pWorld->vPort.targX == tileX && pWorld->vPort.targY == tileY)
						pWorld->DrawStatusBox();
				}
			}
			break;
	}

	#ifdef DEBUG_BIG_NET
	pMono->Out("TerrainUnit()::Cycle() done\n");
	#endif
}


//---------------------------------------------------
// ViewPort class

void
ViewPort::PurgeAnis(void)
{
	BAM_Ani	*pAni;
	grip		gAni;

//	pMono->Out("PurgeAnis(): %d to purge\n", lScreenAnims.count);
	gAni = (grip)lScreenAnims.FirstValue();
	while(gAni)
	{
		if(pMemMgr->CheckGrip(gAni) == GRIP_VALID)
		{
			pAni = ADerefAs(BAM_Ani, gAni);
			pAni->Hide();
		}
		gAni = (grip)lScreenAnims.NextValue();
	}
	lScreenAnims.Release();
}

void
ViewPort::SwapMapTile(int32 xPos, int32 yPos, int newTileNum)
{
	tile			*pTile;
	WorldMap		*pWMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	int32			x, y;
//	Unit			*pUnit;
	TerrainUnit	*pTUnit;
	itemType		item1;

	// inspect location's current terrain - anything special to remove?
	pTile = pWorld->map.GetTile(xPos, yPos);
	switch(pTile->terrain)
	{
		case FLAME_SPOUT:
			pTUnit = pWorld->vPort.GetTerrainUnit(xPos, yPos);
			if(pTUnit && pTUnit->activeType == ACTIVE_FLAME_SPOUT)
			{
				pTUnit->fAniCycle = FALSE;
				pTUnit->fAniPermanent = FALSE;
				if(pTUnit->fAniDone)
					pTUnit->fDeleteMe = TRUE;
			}
			pWorld->LaunchSound(SND_FIRE_GRATE_OPEN_CLOSE, xPos, yPos);
			break;

		case BRIDGE:
			pWorld->LaunchSound(SND_BRIDGE_OPEN_CLOSE, xPos, yPos);
			pTUnit = pWorld->vPort.GetTerrainUnit(xPos, yPos);
			if(pTUnit && pTUnit->activeType == ACTIVE_FIRE)
			{
				pTUnit->fAniCycle = FALSE;
				pTUnit->fAniPermanent = FALSE;
				if(pTUnit->fAniDone)
					pTUnit->fDeleteMe = TRUE;
			}
			break;
	}
	
	// swap tile to alternate
	if(newTileNum == -1)
	{
		pTile = pWorld->map.GetTile(xPos, yPos);
		newTileNum = pTile->swapTile;
		if(newTileNum == 0)
			newTileNum = -1;
	}

	if(newTileNum != -1)
	{
		// swap to alternate tile (burnt, rubble, etc)
		pWorld->map.SetTile((int16)newTileNum, (uint16)xPos, (uint16)yPos);

		// if tile was in known territory
		if(!(fog[yPos][xPos] & FOG_CENTER))
		{
			pWMap->Draw(xPos, yPos);

			// if in viewport
			if(InViewPort(xPos, yPos))
			{
				x = xPos - pWorld->vPort.ViewX;
				y = yPos - pWorld->vPort.ViewY;
				Draw(x, y, x, y);
			}
		}
	}

	// destroy any item that might be there
	pTile = &pWorld->pTileLib->tiles[newTileNum];
	switch(pTile->terrain)
	{
		case CHASM:
		case WATER:
			item1 = pWorld->itemMgr.RemoveItem(xPos, yPos);
			if(item1)
				pWorld->itemMgr.PlaceItem(xPos, yPos, item1, 10);
			break;

		case FLAME_SPOUT:
			TRACK_MEM("FlameSpout");	NewTerrainUnit(ACTIVE_FLAME_SPOUT, xPos, yPos);
			break;
	}
	
	switch(pTile->terrain)
	{
		case BRIDGE:
			pWorld->LaunchSound(SND_BRIDGE_OPEN_CLOSE, xPos, yPos);
			break;

		case FLAME_SPOUT:
			pWorld->LaunchSound(SND_FIRE_GRATE_OPEN_CLOSE, xPos, yPos);
			break;
	}

	// check for connected corner stone to the North
	if(yPos > 0 && pWorld->map.GetTileNum(xPos, yPos - 1) == 478)
	{
		SwapMapTile(xPos, yPos - 1, 374);
	}
}

// deref all ani's into pointer array, sorting as we go
// Note: yeah this is global, but it doesn't matter between frames
struct UnitToCycle
{
	uint32	thisSerial;
	Unit		*pUnit;
} cycleList[GANIMS_MAX];

void
ViewPort::Cycle(void)	// cycle all viewport ani's (this includes Units, which derive from BAM_Ani)
{
	BAM_Ani	*pAni;
	grip		gAni, *pGrip;
	bool		fResetVPCursor = FALSE;
	Unit		*pUnit;
	ticks_t	currTicks = ATicks();
//	BAM_Button	*pButton;
	int		xDif, yDif, newXOff, newYOff, loop1, loop2;
	struct UnitToCycle *pEntry;

//	UpdateTrailCursor();

	if(fFenceMode)
	{
		// update fence ani's
		int 	fenceX1, fenceX2, fenceY1, fenceY2, swap1;

		fenceX1 = fenceStartX;
		fenceY1 = fenceStartY;
		fenceX2 = pMouse->GetX();
		fenceY2 = pMouse->GetY();

		// clip to viewport inner area
		if(fenceX2 >= rInner.x2)
			fenceX2 = rInner.x2 - 1;
		if(fenceY2 >= rInner.y2)
			fenceY2 = rInner.y2 - 1;

		// account for negative delta
		if(fenceX2 < fenceX1)
		{
			swap1 = fenceX1;
			fenceX1 = fenceX2;
			fenceX2 = swap1;
		}
		if(fenceY2 < fenceY1)
		{
			swap1 = fenceY1;
			fenceY1 = fenceY2;
			fenceY2 = swap1;
		}

		// only called for the Mac
		pMouse->SetFence(fenceX1, fenceY1, fenceX2, fenceY2);

/*		// one size fits all!
		pFence = ADerefAs(Guy, gVPFenceGuys[FENCE_TOP]);
		pFence->SetPos(fenceX1, fenceY1);
		pFence->SetClipRect((coord)fenceX1, (coord)fenceY1, (coord)fenceX2, (coord)fenceY1 + 1);

		pFence = ADerefAs(Guy, gVPFenceGuys[FENCE_BOTTOM]);
		pFence->SetPos(fenceX1, fenceY2);
		pFence->SetClipRect((coord)fenceX1, (coord)fenceY2, (coord)fenceX2, (coord)fenceY2 + 1);

		pFence = ADerefAs(Guy, gVPFenceGuys[FENCE_LEFT]);
		pFence->SetPos(fenceX1, fenceY1);
		pFence->SetClipRect((coord)fenceX1, (coord)fenceY1, (coord)fenceX1 + 1, (coord)fenceY2);

		pFence = ADerefAs(Guy, gVPFenceGuys[FENCE_RIGHT]);
		pFence->SetPos(fenceX2, fenceY1);
		pFence->SetClipRect((coord)fenceX2, (coord)fenceY1 - 1, (coord)fenceX2 + 1, (coord)fenceY2 + 1);
		*/
	}
	else
	{
		// check for auto-scroll
		int mouseX = pMouse->GetX(), mouseY = pMouse->GetY(),
			tempX, tempY;

		tempX = ViewX;
		tempY = ViewY;

		if(mouseX == pMouse->screenLimits.x1)
			tempX--;
		else if(mouseX == pMouse->screenLimits.x2)
			tempX++;

		if(mouseY == pMouse->screenLimits.y1)
			tempY--;
		else if(mouseY == pMouse->screenLimits.y2)
			tempY++;

		if(tempX != ViewX || tempY != ViewY)
			SlideView(tempX, tempY);
	}

	if(tMouseResReset && currTicks >= tMouseResReset)
	{
		// end of ERROR buzzer cursor - restore
		tMouseResReset = NULL;

		// if mouse still needs changing
		if(pMouse->GetResNum() == ANIM_CURSOR && pMouse->GetCel() == ANIM_CURSOR_ERROR)
		{
			if(fRepeatCmdMode)
			{
				// for some reason this mode doesnt visually restore correctly.  Just re-do it here.
				pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_REPEAT);
			}
			else
				pMouse->SetRes(tPrevMouseResType, rPrevMouseRes, prevMouseCel);
		}
	}

	if(viewXDest != 999)	// if sliding requested
	{
		// if sliding not started yet
		if(viewXDest != ViewX || viewYDest != ViewY)
		{
			// start it
			viewXDest = AMax(viewXDest, -1);	viewXDest = AMin(viewXDest, WORLD_WIDTH - VPORT_WIDTH + 1);
			viewYDest = AMax(viewYDest, -2);	viewYDest = AMin(viewYDest, WORLD_HEIGHT - VPORT_HEIGHT + 1);

			if(viewXDest == ViewX && viewYDest == ViewY)
			{
				// cant get any closer, skip movement
				viewXDest = 999;
			}
			else
			{
				xDif = viewXDest - ViewX;	xDif = xDif? xDif /= abs(xDif): 0;
				yDif = viewYDest - ViewY;	yDif = yDif? yDif /= abs(yDif): 0;
				prevSlideX = xDif;
				prevSlideY = yDif;

				newXOff = xDif * TILE_WIDTH;
				newXOff = newXOff? newXOff: viewXOffset;

				newYOff = yDif * TILE_HEIGHT;
				newYOff = newYOff? newYOff: viewYOffset;
	
				MoveView(viewXDest, viewYDest, TRUE);
				SetViewOffset(newXOff, newYOff);
				ADerefAs(WorldMap, pWorld->gWorldMap)->MoveCursor(ViewX, ViewY);
			}
		}
		else	// sliding over
		{
			// check for arrow keys still pressed
			xDif = 0;
			yDif = 0;
			if(pEventMgr->KeyIsDown(K_DOWN))
			{
				yDif++;
			}
			if(pEventMgr->KeyIsDown(K_UP))
			{
				yDif--;
			}
			if(pEventMgr->KeyIsDown(K_RIGHT))
			{
				xDif++;
			}
			if(pEventMgr->KeyIsDown(K_LEFT))
			{
				xDif--;
			}

			if(xDif || yDif)
			{
				// restart sliding
//				if(slideIncr == 1)
//				{
					// increase scroll speed

//					if(bGlobal.storyLine != NETGAME)
//					{
//						skipViewPortCycle = AMin(skipViewPortCycle + 6, 6);
//						// cant do this in net mode - automatic de-sync
//					}

					SlideView(ViewX + xDif, ViewY + yDif, slideIncr + 2);
//				}
/*				else
				{
					// jump a whole tile, if we can	
					if(xDif)
					{
						if(xDif < 0)
						{
							if(ViewX >= 1)
								ViewX += xDif;
						}
						else if(xDif > 0)
						{
							if(ViewX < (WORLD_WIDTH - VPORT_WIDTH))
								ViewX += xDif;
						}
					}
					if(yDif)
					{
						if(yDif < 0)
						{
							if(ViewY >= 0)
								ViewY += yDif;
						}
						else if(yDif > 0)
						{
							if(ViewY < (WORLD_HEIGHT - VPORT_HEIGHT))
								ViewY += yDif;
						}
					}

					SlideView(ViewX + xDif, ViewY + yDif, slideIncr);
				}*/
			}
			else
			{
				// were we fast sliding?
				if(slideIncr > 2)
				{
					// do a slow slide to finish it off (looks better that way)
					SlideView();
				}
				else
				{
					// stop sliding
					viewXDest = 999;
					skipViewPortCycle = 0;
				}
			}
		}
	}

	// if viewport sliding active
	if(viewXOffset || viewYOffset)
	{
		// cycle it

//		pMono->Out("slideIncr == %d\n", slideIncr);
		if(slideIncr > 2)
		{
			SetViewOffset(0, 0);
		}
		else
		{
			xDif = -viewXOffset;
			xDif = xDif? (xDif / abs(xDif)) * (slideIncr * 2): 0;
			if(abs(xDif) > abs(viewXOffset))
			{
				// dont go too far
				xDif = -viewXOffset;
			}

			yDif = -viewYOffset;
			yDif = yDif? (yDif / abs(yDif)) * (slideIncr * 3): 0;
			if(abs(yDif) > abs(viewYOffset))
			{
				// dont go too far
				yDif = -viewYOffset;
			}
			SetViewOffset(viewXOffset + xDif, viewYOffset + yDif);
		}
	}

	MouseHandler(FALSE);

	if(skipViewPortCycle > 0)
	{
		// pMono->Out("Skipping ViewPort::Cycle()\n");
		// allows certain continuous actions (scrolling the viewport, etc)
		// to run more smoothly by momentarily blocking anim & unit cycling
		skipViewPortCycle--;
		return;
	}
//	else pMono->Out("NOT skipping ViewPort::Cycle)()\n");

	int	unitsToCycle = 0;
	int32	thisSerial;
	for(loop1 = 0, pGrip = &gAnims[0]; loop1 <= highestAnim; loop1++, pGrip++)
	{
		if(loop1 % 4 == 0)
			MouseHandler(FALSE);

		gAni = *pGrip;
		if(!gAni)
			continue;
		pAni = ADerefAs(BAM_Ani, gAni);
		if(pAni->fDeleteMe)
		{
			fResetVPCursor = pAni->fCursorTracking;	// was unit being watched?
			DeleteAni(pAni->gSelf);
			if(fResetVPCursor)
			{
				SetCursorTarget(TERRAIN, CursorX, CursorY);	// else examine terrain (covers items)
				MoveCursor(CursorX, CursorY);		// if so, reset the cursor at same loc

				// if target selection was taking place, cancel it.
				if(currentVerb != NO_ACTION && currentVerb != QUESTION)
					SetCurrentVerb(NO_ACTION);
			}
		}
		else if(pAni->selfType == UNIT)
		{
			// add to sorted list for cycling later
			pUnit = (Unit *)pAni;
			thisSerial = pUnit->serialNum;

			for(loop2 = 0; loop2 < unitsToCycle; loop2++)
			{
				if(cycleList[loop2].thisSerial > thisSerial)
				{
					// insert here
					memmove(&cycleList[loop2 + 1], &cycleList[loop2],
						(unitsToCycle - loop2) * sizeof(UnitToCycle));
					break;
				}
			}

			cycleList[loop2].thisSerial = thisSerial;
			cycleList[loop2].pUnit = pUnit;
			unitsToCycle++;
		}
		else pAni->SmartCycle(FALSE);
	}

	MouseHandler(FALSE);

	// cycle the next barrel o' units
	for(loop1 = 0, pEntry = &cycleList[0]; loop1 < unitsToCycle; loop1++, pEntry++)
	{
		if(pEntry->thisSerial > lastSerialCycled)
			break;		
	}

	if(loop1 == unitsToCycle)
	{
		// everyone already cycled - one more time, from the top!
		loop1 = 0;
	}

	if(bGlobal.storyLine != NETGAME)
	{
		// ONLY IN SINGLE-PLAYER MODE!  Otherwise machines will definitely
		// de-synch

 		if(pWorld->framesRun % 3 == 0)
	 	{
			if(pWorld->currFPS < 12)
			{
				// we're getting slow - try to decrease the system load
				unitsPerFrame = AMax(unitsPerFrame - 1, unitsToCycle / 4);
			}
			else if(pWorld->currFPS >= 15)
			{
				// hmm, we're running pretty well.  Increase the load.
				unitsPerFrame = AMin(unitsPerFrame + 1, unitsToCycle);
			}
		}
		#ifndef NDEBUG
		pMono->SaveWindow();
		pMono->Goto(25, 75);
		pMono->Out("%2do%2d", unitsPerFrame, unitsToCycle);
		pMono->RestoreWindow();
		#endif
	}

	for(loop2 = 0, pEntry = &cycleList[loop1]; loop2 < unitsToCycle &&
		loop2 < unitsPerFrame; loop1++, loop2++, pEntry++)
	{
		if(loop2 % 4 == 0)
			MouseHandler(FALSE);

		if(loop1 >= unitsToCycle)
		{
			// top of list - wrap around
			loop1 = 0;
			pEntry = &cycleList[0];
		}
		pEntry->pUnit->SmartCycle(FALSE);
		lastSerialCycled = pEntry->thisSerial;
	}
//	pMono->Out("ViewPort::Cycle() %d units cycled\n", unitsToCycle);

	// occasionally check if a unit has wandered under the viewport cursor
	if(ARandom2(5) == 0 && !(fog[CursorY][CursorX] & FOG_CENTER))
	{
		gAni = pWorld->unitLib.gUnitMap[CursorY][CursorX];
		if(gAni)
		{
			pUnit = ADerefAs(Unit, gAni);
		 	if(!pUnit->fCursorTracking)	// if not (cursor on unit because of tracking)
			{
				if(pWorld->lastUnitSerialDrawn != pUnit->serialNum)
				{
					pWorld->DrawStatusBox();
				}
			}
		}
		else if(pWorld->lastUnitSerialDrawn)	// if wandering unit has left
			pWorld->DrawStatusBox();
	}

	MouseHandler(FALSE);

//	if(fAutoCommandMode)
//	{
//		int32	mouseX = pMouse->GetX(), mouseY = pMouse->GetY();
//		int32	newMouseTileX, newMouseTileY;
//		Guy	*pGuy = ADerefAs(Guy, gViewPortGuy);
//		pUnit = (Unit *)pWorld->DerefSerial(targSerial);
//		action oldVerb = currentVerb;
//
//		// if mouse moved outside of viewport
//		if(!rInner.Contains(mouseX, mouseY))
//		{
//			WorldMap	*pWMap = ADerefAs(WorldMap, pWorld->gWorldMap);
//			if(pWMap->worldRect.Contains(mouseX, mouseY))
//			{
//				// inside world map
//
//				if(pUnit->type == FURY)
//				{
//					if(currentVerb != ATTACK)
//						SetCurrentVerb(ATTACK);
//				}
//				else if(currentVerb != MOVE_TO)
//				{
//					SetCurrentVerb(MOVE_TO);
//				}
//			}
//			else if(currentVerb != NO_ACTION)
//			{
//				SetCurrentVerb(NO_ACTION);
//				// this will get cleared - preserve it, here only.
//				fAutoCommandMode = TRUE;
//			}
//			mouseTileX = -1;
//			mouseTileY = -1;
//		}
//		else
//		{
//			// figure out which viewport tile the mouse cursor is over
//			newMouseTileX = (pMouse->GetX() - pGuy->scrim.x) / TILE_WIDTH + ViewX;
//			newMouseTileY = (pMouse->GetY() - pGuy->scrim.y) / TILE_HEIGHT + ViewY;
//
//			// did it change since last check?
//			if(mouseTileX != newMouseTileX || mouseTileY != newMouseTileY)
//			{
//				mouseTileX = newMouseTileX;
//				mouseTileY = newMouseTileY;
//	
//				if(pUnit->type == FURY)
//				{
//					// bad attitude
//					if(currentVerb != ATTACK)
//						SetCurrentVerb(ATTACK);
//				}
//				else if(fog[mouseTileY][mouseTileX] & FOG_CENTER)
//				{
//					// dont let player see what's there - just let them move there
//					SetCurrentVerb(MOVE_TO);
//				}
//				else
//				{
//					// check for possible enemy
//					pUnit = pWorld->unitLib.GetUnit(mouseTileX, mouseTileY);
//					if(pUnit)
//					{
//						// look at unit who is source of verb
//						Unit	*pSource = (Unit *)pWorld->DerefSerial(targSerial);
//
//						// if target is one of ours
//						if(pUnit->player == pSource->player)
//						{
////							if(pSource->item)										// if we have an item
////							{
////								SetCurrentVerb(USE_ITEM);
////							}
////							else
//								SetCurrentVerb(MOVE_TO);
//						}
//						else SetCurrentVerb(ATTACK);
//					}
//					else
//					{
//						MapSpace	*pBuilding;
//
//						pBuilding = pWorld->map.GetStructure(mouseTileX, mouseTileY);
//						if(pBuilding && pBuilding->h.maxHP > 0)	// if pointing to a structure
//						{
//							if(pWorld->unitLib.CheckEnemyFlag(pWorld->playerSide, pBuilding->h.owner))
//								SetCurrentVerb(ATTACK);
//							else
//								SetCurrentVerb(MOVE_TO);
//						}
//						else SetCurrentVerb(MOVE_TO);
//					}
//				}
//			}
//		}
//
//		if(oldVerb != currentVerb)
//		{
//			pWorld->DeselectAllVerbButtons();
//
//			// if source still valid (failsafe)
//			if(pWorld->DerefSerial(verbSourceSerial))
//			{
//				if(currentVerb)
//				{
//					// re-press current button
//					pButton = ADerefAs(BAM_Button, pWorld->gVerbButton[(int)currentVerb - 1]);
//					pButton->Select(TRUE);
//				}
//			}
//			else
//			{
//				// error, unit gone.  End auto command mode.
////				fAutoCommandMode = FALSE;
//				SetCurrentVerb(NO_ACTION);
//			}
//		}			
//	}
#ifdef DEBUG_BIG_NET
	pMono->Out("ViewPort::Cycle() done\n");
#endif
}

// remove ani from all lists, remove from screen, and delete
void
ViewPort::DeleteAni(grip gAni)
{
	BAM_Ani	*pAni;
	int		loop1;
	grip		*pGrip;

	for(loop1 = 0, pGrip = &gAnims[0]; loop1 <= highestAnim; loop1++, pGrip++)
	{
		if(*pGrip == gAni)
		{
			*pGrip = NULL;
			if(loop1 == highestAnim)
				highestAnim--;

			if(pMemMgr->CheckGrip(gAni) != GRIP_VALID)	// safety net
			{
				CutAni(gAni);	// remove it from lists, in case it's still there
				return;
			}

			pAni = ADerefAs(BAM_Ani, gAni);
			if(pAni->selfType == UNIT)						// was this ani a unit?
				pWorld->unitLib.DeleteUnit(gAni);			// then delete via unitLib
			else
			{
				CutAni(gAni);
		  		ADelete(gAni);								// else just kill it here
			}
			return;
		}
	}
}

void
ViewPort::DumpGAnims(void)
{
	#ifndef NDEBUG
	int	loop1;
	

	BamDebug.Out("ViewPort::DumpGAnims() - highestAnim==%d\n", highestAnim);
	for(loop1 = 0; loop1 <= highestAnim; loop1++)
	{
		BamDebug.Out("%d] g%d\n", loop1, gAnims[loop1]);
	}
	#endif
}

// add an animation entity to the master list (regardless of on/off screen status)
void
ViewPort::AddAni(grip gNewBAMAni)
{
	BAM_Ani	*pAni;
	int		loop1, slotNum = -1;
	grip		*pGrip, *pOpening = NULL;;

	// search for first opening, and also if entry already exists
	for(loop1 = 0, pGrip = &gAnims[0]; loop1 <= highestAnim; loop1++, pGrip++)
	{
		if(*pGrip == gNewBAMAni)
		{
			// nevermind, redundantly added
			return;
		}
		if(*pGrip == NULL && !pOpening)
		{
			// found first opening - remember it for later
			pOpening = pGrip;
			slotNum = loop1;
		}
	}

	if(!pOpening)
	{
		// add to end of array
		pOpening = &gAnims[++highestAnim];
		slotNum = highestAnim;
//		pMono->Out("ViewPort::AddAni(g%d) - highestAnim == %d\n", gNewBAMAni, highestAnim);
		if(highestAnim == GANIMS_MAX)
		{
			ReportFreeMem();
			APanic("ViewPort::AddAni() - gAnims[] capacity exceeded!\n");
		}
	}
	*pOpening = gNewBAMAni;

	#ifndef NDEBUG
	BamDebug.Out("VP::AddAni(g%d) slot %d\n", gNewBAMAni, slotNum);
	#endif

	pAni = ADerefAs(BAM_Ani, gNewBAMAni);
	pAni->Hide();	// default state - display of BAM Ani's is dictated by ViewPort::MoveView() and cycled by ViewPort::Cycle()
}

void
ViewPort::CutAni(grip gBAMAni)
{
	BAM_Ani *pAni;
	int		loop1;
	grip		*pGrip;

	if(pMemMgr->CheckGrip(gBAMAni) == GRIP_VALID)
	{
		pAni = ADerefAs(BAM_Ani, gBAMAni);

		if(!pAni->CheckState(S_HIDDEN))
		{
			pAni->Hide();										// remove from scrimList
		}
	}
	lScreenAnims.Delete((void *)gBAMAni);			// remove from on-screen Ani list

	// remove from master anim list
	for(loop1 = 0, pGrip = &gAnims[0]; loop1 <= highestAnim; loop1++, pGrip++)
	{
		if(*pGrip == gBAMAni)
		{
			*pGrip = NULL;
			if(loop1 == highestAnim)
				highestAnim--;
			break;
		}
	}
}

bool
ViewPort::SetRepeatCommandMode(bool fNewState)
{
	switch(fNewState)
	{
		case TRUE:
			if(!fRepeatCmdMode)
				pWorld->AllowMouseDragging(FALSE, TRUE);
			break;

		case FALSE:
			if(fRepeatCmdMode)
				pWorld->AllowMouseDragging(TRUE, TRUE);
			break;
	}
	fRepeatCmdMode = fNewState;
	return(fNewState);
}

// for RepeatCmd mode
void
ViewPort::SetLastVerb(action act, TargetType targType, uint32 targSerial, int x, int y)
{
	lastVerb = act;
	lastVerbType = targType;
	lastVerbSerial = targSerial;
	lastVerbX = x;
	lastVerbY = y;
}

void
ViewPort::SetVerbBuzzer(int errNum)
{
	// if mouse not currently in buzzer state
	if(!(pMouse->GetResNum() == ANIM_CURSOR && pMouse->GetCel() == ANIM_CURSOR_ERROR))
	{
		// save current state for restore later
		tPrevMouseResType = pMouse->GetResType();
		rPrevMouseRes = pMouse->GetResNum();
		prevMouseCel = pMouse->GetCel();
	}

	// show buzzer ani
	pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_ERROR);
	tMouseResReset = ATicks() + (TICKS_PER_SEC / 2);

	if(errNum)
	{
		pWorld->Announce(targX, targY, SQUIB_WORLD_MSGS, errNum);
	}
	else pWorld->LaunchSound(SND_ERROR);
}

bool
ViewPort::DoObviousVerbProcess(int targetX, int targetY)
{
	Unit	*pSrcUnit, *pTargUnit;
//	int	rAckAni;
	action	action1;
//	grip		gAni;
//	BAM_Ani	*pAni;

	// source unit should still be targeted at this point

	if(targType != UNIT)
		return(FALSE);

	// save for later
	pSrcUnit = (Unit *)pWorld->DerefSerial(targSerial);

	pTargUnit = pWorld->unitLib.GetUnit(targetX, targetY);

	// given the current source and requested dest, is there an obvious command we can
	// choose?
	if(pTargUnit)
	{
		action1 = PickObviousVerb(pSrcUnit, pTargUnit, pTargUnit->tileX, pTargUnit->tileY);
	}
	else
	{
		action1 = PickObviousVerb(pSrcUnit, NULL,	targetX, targetY);
	}

	if(action1)
	{
		SetCurrentVerb(action1);
	
		// select new target
		MoveCursor(targetX, targetY, TRUE);

		ProcessVerb();

/*		// show acknowledgement ani
		switch(action1)
		{
			case ATTACK:
				rAckAni = ANIM_CONFIRM_ATTACK;
				break;

			case MOVE_TO:
			default:
				rAckAni = ANIM_CONFIRM_MOVE;
				break;
		}

		pWorld->AddCosmeticAni(rAckAni, targX, targY, 0, 0);
//		TRACK_MEM("ActionAni");	gAni = NewAni(RES_ANIM, rAckAni, 1,
//			PRI_INTERFACE, TRUE, FALSE, 4, targX, targY, FALSE);
//		pAni = ADerefAs(BAM_Ani, gAni);
//		CheckAni(gAni);
*/

		RestoreSourceAsTarget();
		return(TRUE);
	}
	else if(pSrcUnit->type == FURY)
	{
		// offer an explanation
		SetVerbBuzzer(SQUIB_WORLD_CANNOTATTACK);
	}
	else
	{
		// no explanation
		SetVerbBuzzer();
	}
	return(FALSE);
}

action
ViewPort::PickObviousVerb(Unit *pSrcUnit, Unit *pTargUnit, int targetX, int targetY)
{
	// is source something we can even control?
	if(!pSrcUnit)
		return(NO_ACTION);
	if(pBam->playerTypes[pSrcUnit->player] != PLAYER_LOCAL)
		return(NO_ACTION);
	if(pSrcUnit->fAutoControl)
		return(NO_ACTION);

	if(fog[targetY][targetX] & FOG_CENTER)
	{
		// dont reveal information about whatever's at target location
		return(MOVE_TO);
	}

	// since searching for a target unit by checking rect's would have already
	// been done before we were called, we just let that info be passed in
	if(pTargUnit)
	{
		if(pSrcUnit->DoIHate(pTargUnit))
			return(ATTACK);
		else if(pSrcUnit == pTargUnit && pSrcUnit->type == ACOLYTE && pSrcUnit->currAction == GUARD)
			return(SKILL);
		else if(pSrcUnit->type == FURY && pWorld->unitLib.CheckEnemyFlag(pSrcUnit->player, pTargUnit->player))
			return(ATTACK);
	}
	else if(pSrcUnit->DoIHate(pWorld->map.GetStructure(targetX, targetY)))
	{
		return(ATTACK);
	}

	if(pSrcUnit->type == FURY)
		return(NO_ACTION);

	return(MOVE_TO);
}

bool
ViewPort::SetCurrentVerb(action newVerb)
{
	Unit			*pUnit;
	MapSpace		*pSpace;
	Resource		*pRes;
	CelHeader	*pDestCel;
	CelHeader	*pSrcCel;
	grip			gCursorRes;
	uint			rNumCursor;
	int			srcWidth, destWidth, srcHeight, destHeight;

//	pMono->Out("::SetCurrentVerb(%d)\n", (int)newVerb);

	if(fFenceMode)
		AbortFencing();

	// validate verb source first - is current target someone/thing we can command?
	if(newVerb != NO_ACTION && newVerb != QUESTION)
		switch(targType)
		{
			case UNIT:
				pUnit = (Unit *)pWorld->DerefSerial(targSerial);
				if(pUnit->player != pWorld->playerSide)
				{
					// not allowed to control units on other sides
					SetVerbBuzzer();
					return(FALSE);
				}
				break;

			case BUILDING:
				pSpace = (MapSpace *)pWorld->DerefSerial(targSerial);
				if(pSpace->h.owner != pWorld->playerSide)
				{
					// not allowed to control buildings on other sides
					SetVerbBuzzer();
					return(FALSE);
				}
				break;

			default:
				SetVerbBuzzer();
				return(FALSE);
		}

	if(bGlobal.storyLine == SHOW_OFF && (newVerb != NO_ACTION && newVerb != QUESTION))
		return(FALSE);

	switch(newVerb)
	{
		case NO_ACTION:
			currentVerb = NO_ACTION;
			pWorld->DeselectAllVerbButtons();
			if(pWorld->statBoxCurrMode != TERRAIN)
				pWorld->SetStatusBoxMode(TERRAIN);
//			fAutoCommandMode = FALSE;
			pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_HAND);
			fSnapToUnits = TRUE;
			break;

		case MOVE_TO:
			if(targType != UNIT)
			{
				// only units can be commanded to move
				SetVerbBuzzer();
				return(FALSE);
			}

			// remember which unit is currently targetted.  He'll be given the MOVE_TO command after destination
			// target is selected
			currentVerb = MOVE_TO;
			verbSourceX = targX;
			verbSourceY = targY;
			verbSourceSerial = targSerial;
			verbSourceType = targType;
			pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_MOVE);
			fSnapToUnits = FALSE;
			break;

		case ATTACK:
			if(targType != UNIT)
			{
				// only units can be commanded to attack
				SetVerbBuzzer();
				return(FALSE);
			}

			// remember which unit is currently targetted.  He'll be given the ATTACK command after destination
			// target is selected
			currentVerb = ATTACK;
			verbSourceX = targX;
			verbSourceY = targY;
			verbSourceSerial = targSerial;
			verbSourceType = targType;
			pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_ATTACK);
			break;

		case SUMMON:
			if(targType != BUILDING)
			{
				// only portals can be commanded to summon
				SetVerbBuzzer();
				return(FALSE);
			}

			currentVerb = newVerb;
			verbSourceX = targX;
			verbSourceY = targY;
			pSpace = pWorld->map.GetStructure(targX, targY);
			verbSourceType = BUILDING;
			verbSourceSerial = pSpace->serialNum;
			ProcessVerb();
			break;

		case GUARD:
		case DISBAND:
		case DROP:
			if(targType != UNIT)
			{
				// only units can do these
				SetVerbBuzzer();
				return(FALSE);
			}

			currentVerb = newVerb;
			verbSourceX = targX;
			verbSourceY = targY;
			verbSourceSerial = targSerial;
			verbSourceType = targType;
			ProcessVerb();
			break;

		case TRANSFORM:
			// this button is dual-purpose
			pUnit = (Unit *)pWorld->DerefSerial(targSerial);
			pSpace = (MapSpace *)pUnit;

			if(!pUnit)
			{
				SetVerbBuzzer();
				return(FALSE);
			}

			switch(targType)
			{
				case UNIT:
					currentVerb = TRANSFORM;
					verbSourceX = targX;
					verbSourceY = targY;
					verbSourceSerial = targSerial;
					verbSourceType = targType;
					if(pUnit->type == ACOLYTE && pUnit->currTerrain == FOUND_GENERIC)
					{
						pWorld->SetStatusBoxMode(BUILDING);
					}
					else
					{
						if(pUnit->nearbyTransformer == FUNC_IMM_WELL)
						{
							ProcessVerb();
						}
						else pWorld->SetStatusBoxMode(UNIT);
					}
					return(TRUE);

				case BUILDING:
					if(pSpace->h.func == FUNC_PORTAL)
					{
						// SUMMON unit at portal
						SetCurrentVerb(SUMMON);
						break;
					}
					SetVerbBuzzer();
					return(FALSE);

				default:
					SetVerbBuzzer();
					return(FALSE);
			}
			SetCurrentVerb(NO_ACTION);
			break;

		case SKILL:
			if(targType != UNIT)
			{
				// only units can do this
				SetVerbBuzzer();
				return(FALSE);
			}

			pUnit = (Unit *)pWorld->DerefSerial(targSerial);
			if(!pUnit)	// safety net
			{
				MoveCursor(CursorX, CursorY);		// reset cursor target
				SetCurrentVerb(NO_ACTION);
				SetVerbBuzzer();
				return(FALSE);
			}

			verbSourceX = targX;
			verbSourceY = targY;
			verbSourceSerial = targSerial;
			verbSourceType = targType;
			pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_SKILL);

//			if(pUnit->fDoppleganger)
//			{
//				currentVerb = IMMITATE;			// start looking for target
//			}
//			else
			switch(pUnit->type)
			{
				case ENCHANTER:
				case HARPY:
				case GNOME:
				case PALADIN:
				case CLERIC:
				case WYRM:
					currentVerb = SKILL;

					// still need skill target
					break;

				case ACOLYTE:
				case NYMPH:
				case WRAITH:
				case BANSHEE:
					currentVerb = SKILL;

					// dont need a target
					ProcessVerb();
					break;

				default:
					SetCurrentVerb(NO_ACTION);
					SetVerbBuzzer();
					return(FALSE);
			}
			break;

		case USE_ITEM:
			if(targType != UNIT)
			{
				// nope, not allowed
				SetVerbBuzzer();
				return(FALSE);
			}

			pUnit = (Unit *)pWorld->DerefSerial(targSerial);
			if(!pUnit)	// safety net
			{
				MoveCursor(CursorX, CursorY);		// reset cursor target
				SetCurrentVerb(NO_ACTION);
				SetVerbBuzzer();
				return(FALSE);
			}

			currentVerb = USE_ITEM;
			verbSourceX = targX;
			verbSourceY = targY;
			verbSourceSerial = targSerial;
			verbSourceType = targType;

			switch(pWorld->itemMgr.GetItemUsage(pUnit->item))
			{
				case ITEM_TARGET:

					// build an item cursor
					pDestCel = (CelHeader *)AGetResData(gItemCursorCel);
					CopyCel(pDestCel, 0, 0, RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_ITEM, FALSE);

					// copy item image into cursor
					rNumCursor = pWorld->itemMgr.GetPortAnim(pUnit->item);
					gCursorRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, rNumCursor);
					pRes = ADerefAs(Resource, gCursorRes);
					pSrcCel = pRes->GetHeader(1);
					srcWidth = pSrcCel->width;
					destWidth = pDestCel->width;
					srcHeight = pSrcCel->height;
					destHeight = pDestCel->height;
					CopyCel(pDestCel, 2 + destWidth / 2 - srcWidth / 2, (destHeight / 2 - srcHeight / 2) - 2,
						RES_ANIM, rNumCursor, 1, TRUE);					

					// set cursor
					pMouse->SetRes(RES_CEL, rNumItemCursorCel);
					break;

				 default:
					// use item right away
				 	ProcessVerb();
					break;
			}
			break;

		case QUESTION:
			oldCurrentVerb = currentVerb;
			currentVerb = QUESTION;
			verbSourceX = 0;
			verbSourceY = 0;
			verbSourceSerial = NULL;
			verbSourceType = NOTARGET;
			pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_QUESTION);
			fSnapToUnits = FALSE;
			break;

	}
	return(TRUE);
}

// called whenever currentAction != NO_ACTION and the viewport cursor gets moved
// (implying a target for said verb)
bool
ViewPort::ProcessVerb(uint32 mouseX, uint32 mouseY, int32 aux1)
{
	Unit		*pSrcUnit, *pTargUnit;
	MapSpace	*pSpace, *pTargBldg, *pSrcBldg;
	bool		fError;
	int		errNum, tempX, tempY, loop1;
//	grip		gAni;
//	BAM_Ani	*pAni;

//	pMono->Out("ViewPort::ProcessVerb(%d, %d);\n", mouseX, mouseY);

	pSrcUnit = NULL;
	fError = FALSE;

	if(currentVerb != QUESTION)
	{
		switch(verbSourceType)
		{
			case UNIT:
				pSrcUnit = (Unit *)pWorld->DerefSerial(verbSourceSerial);
				if(!pSrcUnit)
				{
					fError = TRUE;
					break;
				}

				if(pSrcUnit->player != pWorld->playerSide || !pSrcUnit->CheckActionAvailable(currentVerb))
				{
					fError = TRUE;
					break;
				}
				break;

			case BUILDING:
				pSrcBldg = (MapSpace *)pWorld->DerefSerial(verbSourceSerial);
				if(!pSrcBldg)
				{
					fError = TRUE;
					break;
				}

				if(pSrcBldg->h.owner != pWorld->playerSide)
				{
					fError = TRUE;
					break;
				}
				break;

			default:
				fError = TRUE;
				break;
		}
		if(fError)
		{
			SetVerbBuzzer();
			return(FALSE);
		}
	}

	pTargUnit = NULL;
	pTargBldg = NULL;
	switch(targType)
	{
		case UNIT:
			pTargUnit = (Unit *)pWorld->DerefSerial(targSerial);
//			SetLastVerb(currentVerb, UNIT, pTargUnit->serialNum, targX, targY);
			break;

		case BUILDING:
			pTargBldg = (MapSpace *)pWorld->DerefSerial(targSerial);
//			SetLastVerb(currentVerb, BUILDING, pTargBldg->serialNum, targX, targY);
			break;

		default:
//			SetLastVerb(currentVerb, TERRAIN, NULL, targX, targY);
			break;
	}

	switch(currentVerb)
	{
		case NO_ACTION:	// shouldnt happen
			break;

		case SUMMON:
//			pSpace = (MapSpace *)pWorld->DerefSerial(verbSourceSerial);
			if(pWorld->unitLib.lUnits[pBam->playerSide].count >= MAX_UNITS_PER_SIDE)
				break;
			pWorld->AddLocalAction(NULL, SUMMON, verbSourceX, verbSourceY, ACOLYTE);
			SetLastVerb(currentVerb, NOTARGET, NULL);
			SetCurrentVerb(NO_ACTION);
			break;

		case QUESTION:
			Encyclopedia	*pE;

			pMouse->SetRes(RES_ANIM, ANIM_CURSOR, ANIM_CURSOR_HAND);

			TRACK_MEM("Encyclo start");  pE = new Encyclopedia;
			pE->Setup(pWorld->gPal,mouseX,mouseY);

			//encyclo setup will call pause on World which resets the currentVerb
			//so reset it now and when encyclo exits
			SetCurrentVerb(oldCurrentVerb);

			break;

		case MOVE_TO:
			if(verbSourceType == UNIT)
			{
				// perform action and reset
				if(groupSize)
				{
					for(loop1 = 0; loop1 < groupSize; loop1++)
						pWorld->AddLocalAction(sGroup[loop1], MOVE_TO, targX, targY);
				}
				else
				{
					pWorld->AddLocalAction(pSrcUnit->serialNum, MOVE_TO, targX, targY);
				}
				SetLastVerb(currentVerb, TERRAIN, NULL, targX, targY);
				SetCurrentVerb(NO_ACTION);

				pWorld->AddCosmeticAni(ANIM_CONFIRM_MOVE, targX, targY, 0, 0);
//				TRACK_MEM("ActionAni");	gAni = NewAni(RES_ANIM, ANIM_CONFIRM_MOVE, 1,
//					PRI_INTERFACE, TRUE, FALSE, 4, targX, targY, FALSE);
//				pAni = ADerefAs(BAM_Ani, gAni);
//				CheckAni(gAni);
			}
			else
			{
				SetVerbBuzzer();
				return(FALSE);
			}
			break;

		case ATTACK:
			if(verbSourceType != UNIT && verbSourceType)
			{
				// only units can attack - send cursor back to source
				SetVerbBuzzer(SQUIB_WORLD_CANNOTATTACK);
				return(FALSE);
			}

			tempX = TILE_HORZ_CENTER;
			tempY = TILE_VERT_CENTER;
			switch(targType)
			{
				case UNIT:
					if(pTargUnit == pSrcUnit)
					{
						// attack himself? doh!
						SetVerbBuzzer(SQUIB_WORLD_CANNOTATTACK);
						return(FALSE);
					}

					SetLastVerb(currentVerb, UNIT, pTargUnit->serialNum, targX, targY);
					tempX = pTargUnit->tileXOff;
					tempY = pTargUnit->tileYOff;
					break;

				case BUILDING:
					if(!pTargBldg->h.currHP)
					{
						SetVerbBuzzer(SQUIB_WORLD_CANNOTATTACK);
						return(FALSE);
					}
					if(pSrcUnit->type == FURY)
					{
						// only units are valid targets
//						SetVerbBuzzer(SQUIB_WORLD_CANNOTATTACK);
						SetVerbBuzzer();
						return(FALSE);
					}
					SetLastVerb(currentVerb, BUILDING, pTargBldg->serialNum, targX, targY);
					break;

				default:
					SetVerbBuzzer(SQUIB_WORLD_CANNOTATTACK);
					return(FALSE);
			}

			// perform action and reset
			if(groupSize)
			{
				for(loop1 = 0; loop1 < groupSize; loop1++)
					pWorld->AddLocalAction(sGroup[loop1], ATTACK, targX, targY);
			}
			else
			{
				pWorld->AddLocalAction(verbSourceSerial, ATTACK, targX, targY);
			}

			// return to source unit
			SetCurrentVerb(NO_ACTION);

			pWorld->AddCosmeticAni(ANIM_CONFIRM_ATTACK, targX, targY, 0, 0);
//			TRACK_MEM("ActionAni");	gAni = NewAni(RES_ANIM, ANIM_CONFIRM_ATTACK, 1,
//				PRI_INTERFACE, TRUE, FALSE, 4, targX, targY, FALSE, tempX, tempY);
//			pAni = ADerefAs(BAM_Ani, gAni);
//			CheckAni(gAni);
			break;

		case TRANSFORM:
			// TRANSFORM unit into new type
			if(pWorld->statBoxCurrMode == UNIT)
			{
				if((unitType)aux1 == UNIT_TERRAIN)
				{
					// TURRET
					pWorld->AddLocalAction(verbSourceSerial, TRANSFORM_TO_BLDG, verbSourceX, verbSourceY);
				}
				else if((unitType)aux1 != NOUNIT)
				{
					pWorld->AddLocalAction(verbSourceSerial, TRANSFORM, verbSourceX, verbSourceY, aux1);
				}
				else
				{
					SetVerbBuzzer();
					return(FALSE);
				}
				SetLastVerb(currentVerb, NOTARGET, NULL);
			}
			else if(pWorld->statBoxCurrMode == BUILDING)
			{
				pSpace = pWorld->map.GetStructure(verbSourceX, verbSourceY);
				if(pSpace && pSpace->CheckFoundationFilled() && (funcType)aux1 != FUNC_MISC)
				{
					pWorld->AddLocalAction(verbSourceSerial, TRANSFORM_TO_BLDG, verbSourceX, verbSourceY, aux1);
				}
				else
				{
					SetVerbBuzzer();
					return(FALSE);
				}
				SetLastVerb(currentVerb, NOTARGET, NULL);
			}
			else if(pSrcUnit->type == ACOLYTE && pSrcUnit->nearbyTransformer == FUNC_IMM_WELL)
			{
				pWorld->AddLocalAction(verbSourceSerial, TRANSFORM, verbSourceX, verbSourceY);	
			}
			else
			{
				SetVerbBuzzer();
				return(FALSE);
			}
			SetCurrentVerb(NO_ACTION);
			break;
			
		case GUARD:
		case DISBAND:
		case DROP:
			// perform action and reset
			if(groupSize)
			{
				for(loop1 = 0; loop1 < groupSize; loop1++)
					pWorld->AddLocalAction(sGroup[loop1], currentVerb, verbSourceX, verbSourceY);
			}
			else
			{
				pWorld->AddLocalAction(verbSourceSerial, currentVerb, verbSourceX, verbSourceY);
			}
			SetLastVerb(currentVerb, NOTARGET, NULL);
			SetCurrentVerb(NO_ACTION);
			break;

		case SKILL:
			// make sure target is valid type
			switch(targType)
			{
				case UNIT:
					if(pSrcUnit->fSkillNeedsTarget && pTargUnit->ImmuneToSkillFrom(pSrcUnit))
					{
						switch(pSrcUnit->type)
						{
							case PALADIN:
							case CLERIC:
								errNum = SQUIB_WORLD_CANNOTHEAL;
								break;

							case GNOME:
								errNum = SQUIB_WORLD_CANNOTREPAIR;
								break;
				
							case ENCHANTER:
								errNum = SQUIB_WORLD_CANNOTCONVERT;
								break;

							case HARPY:
								errNum = SQUIB_WORLD_CANNOTNET;
								break;

							default:
								errNum = 0;
								break;
						}

						SetVerbBuzzer(errNum);
						return(FALSE);
					}
					SetLastVerb(currentVerb, UNIT, pTargUnit->serialNum, targX, targY);
					break;

				case BUILDING:
					errNum = 0;
					switch(pSrcUnit->type)
					{
						case GNOME:
						case WYRM:
							// only these have skills that can affect buildings
							SetLastVerb(currentVerb, BUILDING, pTargBldg->serialNum, targX, targY);
							break;

						case PALADIN:
						case CLERIC:
							errNum = SQUIB_WORLD_CANNOTHEAL;
							break;

						case ENCHANTER:
							errNum = SQUIB_WORLD_CANNOTCONVERT;
							break;

						case HARPY:
							errNum = SQUIB_WORLD_CANNOTNET;
							break;

						default:
							SetVerbBuzzer();
							break;
					}
					if(errNum)
					{
						SetVerbBuzzer(errNum);
					 	return(FALSE);
					}
					break;

				case TERRAIN:
					if(pSrcUnit->type == WYRM)
					{
						SetLastVerb(currentVerb, TERRAIN, NULL, targX, targY);
						break;
					}
					SetVerbBuzzer();
					return(FALSE);
			}

			if(pSrcUnit->fSkillNeedsTarget)
			{
				// show some visual confirmation
				pWorld->AddCosmeticAni(ANIM_CONFIRM_MOVE, targX, targY, 0, 0);
			}

			pWorld->AddLocalAction(verbSourceSerial, currentVerb, targX, targY);
			SetCurrentVerb(NO_ACTION);
			break;

//		case IMMITATE:
//			pWorld->AddLocalAction(verbSourceSerial, currentVerb, targX, targY);
//			SetLastVerb(currentVerb, UNIT, pTargUnit->serialNum, targX, targY);
//			SetCurrentVerb(NO_ACTION);
//			break;

		case USE_ITEM:
			errNum = 0;
			fError = FALSE;
			if(verbSourceType != UNIT)
			{
				fError = TRUE;
			}
			else if(pWorld->itemMgr.GetItemUsage(pSrcUnit->item) == ITEM_TARGET)
			{
				if(!pWorld->itemMgr.CheckItemTarget(pSrcUnit->item, targType, targX, targY))
				{
					fError = TRUE;
				}
				else if(targType == UNIT && pTargUnit && pTargUnit->ImmuneToItem(pSrcUnit->item))
				{
					fError = TRUE;
				}
				else if(targType == BUILDING && pTargBldg)
				{
					// teleport tome has additional restriction
					if(pSrcUnit->item == TELEPORT_TOME && !pSrcUnit->CanIMoveTo(targX, targY, TRUE))
					{
						fError = TRUE;
					}
				}
				// any fog-dependent conditions must be checked here, because
				// in multi-player mode, the other machine is not aware of our
				// fog map.
				if(!fError && pSrcUnit->item == TELEPORT_TOME &&
					(fog[targY][targX] & FOG_CENTER))
				{
					fError = TRUE;
				}
				else switch(pSrcUnit->item)
				{
					case TELEPORT_TOME:
						if(pWorld->GetManaLevel(pSrcUnit->player) < MANA_TELEPORT)
						{
							pWorld->Announce(pSrcUnit->tileX, pSrcUnit->tileY,
								SQUIB_WORLD_MSGS, (int)MSG_NO_MANA);
							return(FALSE);
						}
						break;

					case WEIRD_WAND:
						if(pWorld->GetManaLevel(pSrcUnit->player) < MANA_WEIRD_WARD)
						{
							pWorld->Announce(pSrcUnit->tileX, pSrcUnit->tileY,
								SQUIB_WORLD_MSGS, (int)MSG_NO_MANA);
							return(FALSE);
						}
						break;
				}

				if(fError)
				{
					if(errNum)
						SetVerbBuzzer(errNum);
					else SetVerbBuzzer();
					return(FALSE);
				}

				pWorld->AddCosmeticAni(ANIM_CONFIRM_MOVE, targX, targY, 0, 0);
				SetLastVerb(currentVerb, targType, targSerial, targX, targY);
			}
			else
			{
				if(fError)
				{
					if(errNum)
						SetVerbBuzzer(errNum);
					else SetVerbBuzzer();
					return(FALSE);
				}

				SetLastVerb(currentVerb, NOTARGET, NULL);
			}

			pWorld->AddLocalAction(verbSourceSerial, USE_ITEM, targX, targY);
			SetCurrentVerb(NO_ACTION);
			break;
	}
	return(TRUE);
}

void
ViewPort::StartFencing(void)
{
	int	loop1;
	Guy	*pGuy;

	if(pMouse->rClickDrag.x1 == -1)
	{
		// nevermind - dragging not allowed
		return;
	}

	if(groupSize)
		CleanupGroupCursors();

	fenceStartX = pMouse->GetFenceStartX();
	fenceStartY = pMouse->GetFenceStartY();
//	fenceStartX = pMouse->rCurrentFence.x1;
//	fenceStartY = pMouse->rCurrentFence.y1;

	// hide primary cursor for a moment
//	pGuy = ADerefAs(Guy, gViewPortCursor);
//	pGuy->Hide();

	if(!fFenceMode)
	{
		// fence mode not already running - allocate fence guys
		for(loop1 = 0; loop1 < 4; loop1++)
		{
			TRACK_MEM("FenceGuy");	pGuy = new Guy;
			gVPFenceGuys[loop1] = pGuy->gSelf;
			pGuy->SetRes(RES_ANIM, ANIM_FENCE, loop1 < 2? 2: 1);
//			pGuy->SetRes(RES_CEL, loop1 < 2? rTopCel: rSideCel);
			pGuy->SetContext(gContext);
//			pGuy->SetPos(0, 0);
			pGuy->Setup();
			pGuy->SetPri(VPORT_PRI);
		}

		Context	*pContext = ADerefAs(Context, gContext);
		pContext->lReceivers.Move((void *)gSelf, L_FRONT);
	}

	for(loop1 = 0; loop1 < 4; loop1++)
	{
		pGuy = ADerefAs(Guy, gVPFenceGuys[loop1]);
		pGuy->Hide();
//		pGuy->SetPos(fenceStartX, fenceStartY);
//		pGuy->SetClipRect(fenceStartX, fenceStartY, fenceStartX, fenceStartY);
//		pGuy->Show();
	}

	fFenceMode = TRUE;
	msgMask |= E_MOUSE_UP;
}

void
ViewPort::EndFencing(int mouseX, int mouseY)
{
	List			*plUnits;
	Guy			*pGuy;
	Rectangle	rSelect;
	grip			gUnit;
	Unit			*pUnit;

	rSelect.Set(AMin(fenceStartX, mouseX), AMin(fenceStartY, mouseY),
		AMax(fenceStartX, mouseX), AMax(fenceStartY, mouseY));

	pGuy = ADerefAs(Guy, gViewPortCursor);
	pGuy->Show();

	if(groupSize)
		CleanupGroupCursors();

	if(fFenceMode)
	{
		// delete fence animations
		int	loop1;
		for(loop1 = 0; loop1 < 4; loop1++)
		{
			ADelete(gVPFenceGuys[loop1]);
		}

		fFenceMode = FALSE;	
		msgMask &= ~E_MOUSE_UP;
	}

	plUnits = &pWorld->unitLib.lUnits[pBam->playerSide];
	gUnit = (grip)plUnits->FirstValue();

//	pMono->Out("fence@(%d,%d %d,%d)\n", fenceStartX, fenceStartY, mouseX,
//		mouseY);
	
	while(gUnit)
	{
		pUnit = ADerefAs(Unit, gUnit);
//		pMono->Out("Exam %s @%d,%d: ", pUnit->unitName, pUnit->scrim.x,
//			pUnit->scrim.y);

		gUnit = (grip)plUnits->NextValue();
		if(pUnit->CheckState(S_HIDDEN))
		{
//			pMono->Out("offscreen\n");
			continue;
		}

		if((rSelect.x1 < pUnit->scrim.rect.x1 &&
			rSelect.x2 < pUnit->scrim.rect.x1))
		{
//			pMono->Out("unit is right\n");
			continue;
		}

		if((rSelect.x1 > pUnit->scrim.rect.x2 &&
			rSelect.x2 > pUnit->scrim.rect.x2))
		{
//			pMono->Out("unit is left\n");
			continue;
		}

		if((rSelect.y1 < pUnit->scrim.rect.y1 &&
			rSelect.y2 < pUnit->scrim.rect.y1))
		{
//			pMono->Out("unit is below\n");
			continue;
		}

		if((rSelect.y1 > pUnit->scrim.rect.y2 &&
			rSelect.y2 > pUnit->scrim.rect.y2))
		{
//			pMono->Out("unit is above\n");
			continue;
		}

		if(pUnit->fAutoControl)
		{
//			pMono->Out("autocontrol\n");
			continue;
		}

		if(pUnit->type == FURY)
			continue;

//		pMono->Out("unit OK\n");
  		pUnit->GroupSelection(TRUE);
		sGroup[groupSize++] = pUnit->serialNum;

		// if first dude
		if(groupSize == 1)
		{
			// gotta leave the cursor somewhere
//			MoveCursor(pUnit->tileX, pUnit->tileY);
		}
	}

	if(groupSize == 1)
	{
		int32	serial1 = sGroup[0];

		// forget it, return to normal targetting
		RemoveUnitFromGroup(sGroup[0]);

		// if cursor not already there..
		if(CursorX != pUnit->tileX || CursorY != pUnit->tileY)
		{
			pUnit = (Unit *)pWorld->DerefSerial(serial1);
			MoveCursor(pUnit->tileX, pUnit->tileY);
		}
	}
	else if(groupSize > 1)
	{
		int32	serial1 = sGroup[0];

		// move real cursor to the 1st guy, just to hide it
		pUnit = (Unit *)pWorld->DerefSerial(serial1);
		MoveCursor(pUnit->tileX, pUnit->tileY);
	}
	else if(groupSize == 0)
	{
		// didn't catch anyone - gotta put the cursor somewhere though
		MoveCursor(ViewX + mouseX / TILE_WIDTH + 1, ViewY + mouseY / TILE_HEIGHT + 1);
	}
}

void
ViewPort::RemoveUnitFromGroup(uint32 unitSerial)
{
	int	loop1;
	Unit	*pUnit;

	for(loop1 = 0; loop1 < groupSize; loop1++)
	{
		if(sGroup[loop1] != unitSerial)
			continue;
		pUnit = (Unit *)pWorld->DerefSerial(sGroup[loop1]);
		if(pUnit)
			pUnit->GroupSelection(FALSE);
		if(loop1 + 1 < groupSize)
		{
			memmove(&sGroup[loop1], &sGroup[loop1 + 1], sizeof(sGroup[loop1]) *
			(groupSize - (loop1 + 1)));
		}

		groupSize--;
	}

	// if only one guy left, go to normal selection mode
	if(groupSize == 1)
	{
		groupSize = 0;
		pUnit = (Unit *)pWorld->DerefSerial(sGroup[0]);
		if(pUnit)
		{
			pUnit->GroupSelection(FALSE);
			MoveCursor(pUnit->tileX, pUnit->tileY);
		}
	}
	else if(groupSize > 1)
	{
		// move cursor to someone else in group
		pUnit = (Unit *)pWorld->DerefSerial(sGroup[0]);
		if(pUnit)
			MoveCursor(pUnit->tileX, pUnit->tileY);
	}
}

void
ViewPort::CleanupGroupCursors(void)
{
	int	loop1;
	Unit	*pUnit;

	for(loop1 = 0; loop1 < groupSize; loop1++)
	{
		pUnit = (Unit *)pWorld->DerefSerial(sGroup[loop1]);
		if(pUnit)
			pUnit->GroupSelection(FALSE);
	}
	groupSize = 0;
}

void
ViewPort::AbortFencing(void)
{
	grip	gFence;

	if(fFenceMode)
	{
		// delete fence animations
		int	loop1;
		for(loop1 = 0; loop1 < 4; loop1++)
		{
			gFence = gVPFenceGuys[loop1];
			if(!gFence)
				continue;
				gVPFenceGuys[loop1] = NULL;
			if(pMemMgr->CheckGrip(gFence) != GRIP_VALID)
				continue;
			ADelete(gFence);
		}
		fFenceMode = FALSE;
	}

//	pMono->Out("AbortFencing()\n");
	msgMask &= ~E_MOUSE_UP;
}

bool
ViewPort::HandleMsg(Message *pMsg)
{
	uint32	mouseX, mouseY, xPos, yPos;
//	int		loop1;
	bool		fFoundUnit;
	BAM_Ani	*pAni;
	grip		gAni;
	Unit		*pUnit;
	List		*plUnits;

	switch(pMsg->type)
	{
		case MSG_EVENT:
			if(pWorld->tWorldEnds && bGlobal.storyLine != SHOW_OFF)
			{
				// game is about to end - no user input
				break;
			}

			switch (pMsg->event.type)
			{
				case E_KEY_DOWN:
					if(bGlobal.storyLine == SHOW_OFF)
						return(TRUE);

//					switch(pMsg->event.value)
//					{
//				  	}
					break;

				case E_MOUSE_UP:
					if(fFenceMode)
					{
						// doesn't matter whether it's in our domain or not
						EndFencing(pMsg->event.x, pMsg->event.y);
						return(TRUE);
					}

				case E_MOUSE_DOWN:
					// did click occur within the viewport area?		
					mouseX = pMsg->event.x;
					mouseY = pMsg->event.y;

					if(!rInner.Contains(mouseX, mouseY))
					{
						return(FALSE);
					}

	  				// before sending cursor to the tile clicked on, check if click fell within a Unit rect
					// (makes Units easier to select)
					fFoundUnit = FALSE;
		  
					if((pMsg->event.value == LEFT_BTN || pMsg->event.value == RIGHT_BTN) && fSnapToUnits)
					{
						// if trying to target a unit
						if(pMsg->event.value == LEFT_BTN)
						{
							plUnits = NULL;
							switch(currentVerb)
							{
								case SKILL:
									pUnit = (Unit *)pWorld->DerefSerial(verbSourceSerial);
									if(!pUnit)
										break;
									if(pUnit->skillType == SKILL_NONE)
										break;

									if(pUnit->skillType == SKILL_PASSIVE)
										plUnits = &pWorld->unitLib.lUnits[pWorld->playerSide];
									// fall-thru

								case ATTACK:
									// search enemy unit lists first
									if(!plUnits)
										plUnits = &pWorld->unitLib.lUnits[(pWorld->playerSide == SIDE1)? SIDE2: SIDE1];

									gAni = (grip)plUnits->FirstValue();
									while(gAni && !fFoundUnit)
									{
										pUnit = ADerefAs(Unit, gAni);
										gAni = (grip)plUnits->NextValue();

										// if unit is offscreen
										if(pUnit->CheckState(S_HIDDEN))
											continue;

										if(pUnit->scrim.rect.Contains(mouseX, mouseY))
										{
											fFoundUnit = TRUE;
											break;
										}
									}
									break;

								default:
									break;
							}
						}

						if(fFoundUnit)
						{
							// skip the normal search
							pAni = (BAM_Ani *)pUnit;
							gAni = NULL;
						}
						else
						{
							gAni = (grip)lScreenAnims.FirstValue();
						}

						while(gAni)
						{
							if(pMemMgr->CheckGrip(gAni) != GRIP_VALID)
								pMono->Out("ViewPort::HandleMsg() - error, invalid grip %d in lScreenAnims\n", gAni);
							else
							{
								pAni = ADerefAs(BAM_Ani, gAni);
								if(pAni->selfType == UNIT && pAni->scrim.rect.Contains(mouseX, mouseY))
								{
									fFoundUnit = TRUE;

									// auto command mode now disabled
//									pUnit = (Unit *)pAni;
//									// if unit was already targeted)
//									if(pUnit->fCursorTracking && targSerial == pUnit->serialNum && !pWorld->fIsPaused &&
//										pUnit->player == pWorld->playerSide && !pWorld->worldEnder && !pUnit->fAutoControl
//										&& bGlobal.storyLine != SHOW_OFF && currentVerb == NO_ACTION)
//									{
//										// double-clicked unit - go into auto command mode
//										mouseTileX = 0;
//										mouseTileY = 0;
//										fAutoCommandMode = TRUE;
//									}
									break;
								}
							}
							gAni = (grip)lScreenAnims.NextValue();
						}
					}

					if(fFoundUnit)
					{
						xPos = pAni->tileX - (ViewX + 1);
						yPos = pAni->tileY - (ViewY + 1);
					}
					else
					{
						xPos = mouseX / TILE_WIDTH;
						yPos = mouseY / TILE_HEIGHT;
					}

					// if clicked in map title bar
					if(ViewY == -2 && !yPos)
					{
						if(!fAutoCommandMode)
						{
							StartFencing();
						}
						return(TRUE);
					}

					// we have a tile X,Y within the viewport - do something with it
					switch(pMsg->event.value)
					{
						case LEFT_BTN:
							// if a command verb was in effect (waiting for a dest)
							if(currentVerb)
							{
								if(pWorld->statBoxCurrMode != TERRAIN)
								{
									MoveCursor(ViewX + xPos + 1, ViewY + yPos + 1);
								}
								else
								{
									MoveCursor(ViewX + xPos + 1, ViewY + yPos + 1, TRUE);
									// then presume click was meant as verb destination
									ProcessVerb();
									RestoreSourceAsTarget();
								}
							}
							else if(fRepeatCmdMode)
							{
								// move cursor to new subject
								MoveCursor(ViewX + xPos + 1, ViewY + yPos + 1);

								if(!SetCurrentVerb(lastVerb))
								{
									// verb is invalid for this unit/bldg, regardless of pre-defined target
									break;
								}

								// pre-defined verb selected.
								// now auto-select same target that was selected last time
								// if target was a unit (and therefore could have moved since)
								if(lastVerbType == UNIT)
								{
									pUnit = (Unit *)pWorld->DerefSerial(lastVerbSerial);
									if(pUnit)	// still alive?
									{
										// target his current x,y instead of last known
										MoveCursor(pUnit->tileX, pUnit->tileY, TRUE);
									}
									else
									{
										// unit has died or something - just target last known X,Y
										MoveCursor(lastVerbX, lastVerbY, TRUE);
									}										
								}
								else if(lastVerbType == BUILDING || lastVerbType == TERRAIN)
								{
									// bldg or terrain
									MoveCursor(lastVerbX, lastVerbY, TRUE);
								}
								// else last verb had no target to which we should return
								
								// pre-defined target now selected.
								// execute pre-defined command.
								if(ProcessVerb())
									SetRepeatCommandMode(FALSE);	// cause it to re-register
								else
								{
									SetCurrentVerb(NO_ACTION);
									SetVerbBuzzer();
									// command error'ed out.  Dont cause repeat mode to immediately
									// re-register, because we want to leave the error cursor up for a sec
								}
								RestoreSourceAsTarget();
							}
							else if(!fAutoCommandMode)
							{
/*								if(pMsg->event.type == E_MOUSE_UP)
								{
									if(fFenceMode)
									{
										EndFencing();
									}
									return(TRUE);
								}*/

								MoveCursor(ViewX + xPos + 1, ViewY + yPos + 1);
								StartFencing();

								// "click verbal response"
//								if(targType == UNIT)
//								{
//									pUnit = (Unit *)pWorld->DerefSerial(targSerial);
//									switch(pUnit->type)
//									{
//										case BANSHEE:
//											int1 = SND_FEMALE_WAIL;
//											break;
//
//										case DRUID:
//											int1 = SND_DRUID;
//											break;
//
//										case GARGOYLE:
//											int1 = SND_GARGOYLE;
//											break;
//
//										case GHOUL:
//											int1 = SND_GHOUL;
//											break;
//
//										case GNOME:
//											int1 = SND_GNOME;
//											break;
//
//										case GORGON:
//											int1 = SND_GORGON;
//											break;
//
//										case GRIFFIN:
//											int1 = SND_GRIFFIN;
//											break;
//
//										case JUGGERNAUT:
//											int1 = SND_JUGGERNAUT;
//											break;
//
//										case NYMPH:
//											int1 = SND_NYMPH;
//											break;
//
//										case ROC_EGG:
//											int1 = SND_METAPOD;
//											break;
//
//										case ROC:
//											int1 = SND_BIRD_CRY;
//											break;
//
//										case WIZARD:
//											int1 = SND_WIZARD;
//											break;
//
//										case WOLF:
//											int1 = SND_WOLF;
//											break;
//
//										case WRAITH:
//											int1 = SND_WRAITH;
//											break;
//
//										case WYRM:
//											int1 = SND_WYRM;
//											break;
//
//										case ZOMBIE:
//											int1 = SND_ZOMBIE;
//											break;
//
//										case TORTOISE:
//											int1 = SND_TORTOISE;
//											break;
//
//										case HARPY:
//											int1 = SND_HARPY;
//											break;
//
//										case GUARDIAN:
//											int1 = SND_GUARDIAN;
//											break;
//
//										case BRIGAND:
//											int1 = SND_BRIGAND;
//											break;
//
//										case ACOLYTE:
//											if(pUnit->currAction == TRANSFORM)
//												int1 = SND_METAPOD;
//												else int1 = SND_ACOLYTE;
//											break;
//
//										case GOBLIN:
//											int1 = SND_GOBLIN;
//											break;
//
//										case CLERIC:
//											int1 = SND_CLERIC;
//											break;
//
//										case PEASANT:
//											int1 = SND_PEASANT;
//											break;
//
//										case FURY:
//											int1 = SND_FURY;
//											break;
//
//										case PALADIN:
//											int1 = SND_PALADIN;
//											break;
//
//										case RANGER:
//											int1 = SND_RANGER;
//											break;
//
//										case TROLL:
//											int1 = SND_TROLL;
//											break;
//
//										case WARRIOR:
//											int1 = SND_WARRIOR;
//											break;
//
//										default:
//											int1 = 0;
//											break;
//									}
//									if(int1)
//									{
//										pWorld->LaunchSound(int1, CursorX, CursorY);
//									}
//								}
							}
							break;

						case RIGHT_BTN:
							if(pMsg->event.type == E_MOUSE_UP)
								return(FALSE);

							if(pWorld->fIsPaused)
								break;

							if(currentVerb)
							{
								pWorld->DeselectAllVerbButtons();
								SetCurrentVerb(NO_ACTION);
								break;
							}

							if(fFoundUnit)
							{
								pUnit = (Unit *)pAni;
								DoObviousVerbProcess(pUnit->tileX, pUnit->tileY);
							}
							else
							{
								DoObviousVerbProcess(ViewX + xPos + 1, ViewY + yPos + 1);
							}

							// auto-center code disabled
//							else
//							{
//								WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);
//								MoveView(ViewX + xPos + 1 - VPORT_WIDTH / 2, ViewY + yPos + 1 - VPORT_HEIGHT / 2);
//								pWorldMap->MoveCursor(ViewX, ViewY);
//							}
//							break;

						// we should do something with the middle button
					}
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}

int turrets2tiles[16] =
{FOG_NULL,
FOG_EAST, FOG_WEST, FOG_EAST|FOG_WEST,
FOG_NORTH, FOG_SOUTH, FOG_NORTH|FOG_SOUTH,
FOG_EAST|FOG_SOUTH, FOG_WEST|FOG_SOUTH,
FOG_NORTH|FOG_EAST, FOG_WEST|FOG_NORTH,
FOG_NORTH|FOG_EAST|FOG_SOUTH, FOG_WEST|FOG_SOUTH|FOG_EAST,
FOG_NORTH|FOG_WEST|FOG_SOUTH, FOG_NORTH|FOG_EAST|FOG_WEST,
FOG_NORTH|FOG_SOUTH|FOG_EAST|FOG_WEST};

void
ViewPort::Setup(grip gContext)
{
	Guy		*pGuy;
	Context	*pContext;
	int		loopX, loopY, loop1, terrainNum, dir, squibRes, squibNum;
	grip		gCel;
	Resource	*pRes;
	CelHeader	*pCel;
	char		*pString;
	SquibRes	squib1;

	unitsPerFrame = 40;	// just to be safe

	// no last view
	lastViewX = 999;	lastViewY = 999;	lastViewX2 = 999;  lastViewY2 = 999;

	// safe defaults
	fFenceMode = FALSE;
	groupSize = 0;	// no one currently selected

	viewXDest = 999;	// no viewport sliding at present
	tPrevMouseResType = RES_ANIM;
	rPrevMouseRes = ANIM_CURSOR;
	prevMouseCel = ANIM_CURSOR_HAND;
	tMouseResReset = NULL;	// no current state to reset

	lastVerb = NO_ACTION;
	for(loop1 = 0; loop1 < pWorld->pTileLib->totalTiles; loop1++)
	{
		terrainNum = (int)pWorld->pTileLib->tiles[loop1].terrain;
		if(ABetween(terrainNum, TURRET, TURRET15))
		{
			dir = turrets2tiles[terrainNum - TURRET];
			turretTiles[dir] = loop1;
		}
	}

	// fog-of-battle
	if(!pBam->fUseFog)
		memset(fog, NULL, sizeof(fog));
	else for(loopY = 0; loopY < WORLD_HEIGHT; loopY++)
			for(loopX = 0; loopX < WORLD_WIDTH; loopX++)
				fog[loopY][loopX] = FOG_CENTER;
	
	// determine which tile to use as black (completely fogged)
	fogTile = 475;		// semi-reliable default, just in case
	for(loopX = 0;	loopX < pWorld->pTileLib->totalTiles; loopX++)
		if(pWorld->pTileLib->tiles[loopX].terrain == FOG_TERR)
		{
			fogTile = loopX;
			break;
		}

//	SetCurrentVerb(NO_ACTION);		// no verb button is currently active

	// attach ourselves to context
	SetContext(gContext);
	pContext = ADerefAs(Context, gContext);
	pContext->AddObject(gSelf, CT_MSGS|CT_SERV);

	// set up the ViewPort guy itself
	// pGuy->Setup() does the following, although not quite how we want
	pGuy = ADerefAs(Guy, gViewPortGuy);
	pGuy->SetRes(RES_CEL, rNumViewPortCel);
	pGuy->SetContext(gContext);
	pGuy->SetPos(-TILE_WIDTH, -TILE_HEIGHT);
	pGuy->Setup();
	pGuy->SetPri(VPORT_PRI);

	// screen limits of the area of the master cel which we can draw into
	rect.Set(0, 0, VPORT_WIDTH * TILE_WIDTH, VPORT_HEIGHT * TILE_HEIGHT);

	// used for click detection
	rInner.Set(0, 0, (VPORT_WIDTH - 2) * TILE_WIDTH - 1, (VPORT_HEIGHT - 2) * TILE_HEIGHT - 1);

	//set up the ViewPort cursor guy
	pGuy = ADerefAs(Guy, gViewPortCursor);
	pGuy->SetRes(RES_ANIM,9002, 1);
	pGuy->SetContext(gContext);
	pGuy->SetPos(0, 0);
	pGuy->Setup();
	pGuy->SetPri(VPORT_PRI + PRI_VP_CURSOR);

	//set up the ViewPort trailing cursor guy
//	pGuy = ADerefAs(Guy, gVPTrailCursor);
//	pGuy->SetAnim(9002, 1);
//	pGuy->SetContext(gContext);
//	pGuy->SetPos(0, 0);
//	pGuy->Setup();
//	pGuy->SetPri(VPORT_PRI + 299);

	pRes = ADerefAs(Resource, ALoadDebug(__FILE__, __LINE__, RES_ANIM, 116));
	pCel = pRes->GetHeader(5);
	TRACK_MEM("MapTitleCel");	gCel = ACreateCel(&rMapTitleCel, 0, 0, pCel->width, pCel->height, CI_BLACK);
	pCel = (CelHeader *)AGetResData(gCel);
	CopyCel(pCel, 0, 0, RES_ANIM, 116, 5, FALSE);
	squibRes = 10002;	squibNum = 1;	// safety defaults
	if(bGlobal.storyLine == SHOW_OFF || bGlobal.storyLine == NETGAME)
	{
		squibRes = REGION_SQB;
		squibNum = (pWorld->mapResNum - 9110) / 20 + 1;
	}
	else
	{
		GetStoryName(&squibRes, &squibNum, bGlobal.storyLine);
	}
	pString = squib1.Load(squibRes, squibNum);

	// place string in cel, centered
	SetFontColors(CI_SKIP, 65, 62);

	pFontMgr->SetRes(9050);
	ASetString(0, 10, pString, (uchar *)pCel, pCel->width, NULL, DG_JUST_CENTER);
	
	TRACK_MEM("MapTitleGuy");	pGuy = new Guy;
	gMapTitleGuy = pGuy->gSelf;
	pGuy->SetRes(RES_CEL, rMapTitleCel);
	pGuy->SetPos(0, 0);
	pGuy->SetContext(gContext);
	pGuy->Setup();
	pGuy->SetPri(VPORT_PRI + 1);
	pGuy->Hide();
}

ViewPort::ViewPort(void) : lScreenAnims(100, TRUE)
{
	Guy		*pGuy;
	grip		gTemp;
	Resource	*pRes;
	CelHeader	*pCel;

	memset(activeTerrain, ACTIVE_NONE, sizeof(activeTerrain));
	fSnapToUnits = TRUE;
	skipViewPortCycle = 0;
	lScreenAnims.fDuplicates = FALSE;
	highestAnim = -1;
	lastSerialCycled = 0;

	memset(fogAnim, NULL, sizeof(fogAnim));
	fogAnim[FOG_WEST][0] = 308;							fogAnim[FOG_WEST][1] = 1;
	fogAnim[FOG_NORTH][0] = 308;							fogAnim[FOG_NORTH][1] = 2;
	fogAnim[FOG_WEST|FOG_SOUTH][0] = 308;				fogAnim[FOG_WEST|FOG_SOUTH][1] = 3;
	fogAnim[FOG_NORTH|FOG_WEST|FOG_SOUTH][0] = 308;	fogAnim[FOG_NORTH|FOG_WEST|FOG_SOUTH][1] = 4;
	fogAnim[FOG_WEST|FOG_NORTH|FOG_EAST][0] = 308;	fogAnim[FOG_WEST|FOG_NORTH|FOG_EAST][1] = 5;
	fogAnim[FOG_EAST][0] = 309;							fogAnim[FOG_EAST][1] = 1;
	fogAnim[FOG_SOUTH][0] = 309;							fogAnim[FOG_SOUTH][1] = 2;
	fogAnim[FOG_SOUTH|FOG_EAST][0] = 309;				fogAnim[FOG_SOUTH|FOG_EAST][1] = 3;
	fogAnim[FOG_WEST|FOG_NORTH][0] = 309;				fogAnim[FOG_WEST|FOG_NORTH][1] = 4;
	fogAnim[FOG_EAST|FOG_NORTH][0] = 309;				fogAnim[FOG_EAST|FOG_NORTH][1] = 5;
	fogAnim[FOG_NORTH|FOG_EAST|FOG_SOUTH][0] = 309;	fogAnim[FOG_NORTH|FOG_EAST|FOG_SOUTH][1] = 6;
	fogAnim[FOG_WEST|FOG_SOUTH|FOG_EAST][0] = 309;	fogAnim[FOG_WEST|FOG_SOUTH|FOG_EAST][1] = 7;
	
	mouseTileX = -1;	mouseTileY = -1;	// used in auto-command mode only
	fAutoCommandMode = FALSE;

	//	no verb currently in effect, so there cant be a verb source now can there?
	currentVerb = NO_ACTION;
	verbSourceX = 0;
	verbSourceY = 0;
	verbSourceSerial = NULL;
	verbSourceType = NOTARGET;

	// what is currently targetted by cursor?
	targType = NOTARGET;
	targSerial = 0;
	targX = 0;
	targY = 0;

	ViewX = WORLD_WIDTH / 2;		// default center of the world
	ViewY = WORLD_HEIGHT / 2;
	CursorX = 0;
	CursorY = 0;
	CursorXOff = 0;
	CursorYOff = 0;

	SetContext(NULL);

	// determine dimensions of item cursor, and alloc a DCEL of same size
	gTemp = ALoadDebug(__FILE__, __LINE__, RES_ANIM, ANIM_CURSOR);
	pRes = ADerefAs(Resource, gTemp);
	pCel = pRes->GetHeader((uint16)(ANIM_CURSOR_ITEM));
	TRACK_MEM("ItemCursorDCEL");	gItemCursorCel = ACreateCel(&rNumItemCursorCel,
		0, 0, pCel->width, pCel->height, NULL, NULL);

	TRACK_MEM("ViewPortDCEL");		gViewPortCel = ACreateCel(&rNumViewPortCel,
		0, 0, TILE_WIDTH * VPORT_WIDTH, TILE_HEIGHT * VPORT_HEIGHT, CI_BLACK, 1);
		TRACK_MEM("ViewPortGuy");		pGuy = new Guy;
	gViewPortGuy = pGuy->gSelf;

	TRACK_MEM("ViewPortCursor");	pGuy = new Guy;
	gViewPortCursor = pGuy->gSelf;

//	TRACK_MEM("ViewPortTCursor");	pGuy = new Guy;
//	gVPTrailCursor = pGuy->gSelf;
}

bool
ViewPort::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&vpDataStart, 1, (int)&vpDataEnd -
				(int)&vpDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&vpDataStart, 1, (int)&vpDataEnd -
				(int)&vpDataStart, pFile);
			break;
	}
	return(TRUE);
}

void
ViewPort::Cleanup(void)
{
	int		loop1;

	DumpGAnims();

	ADelete(gMapTitleGuy);

	if(fFenceMode)
	{
		for(loop1 = 0; loop1 < 4; loop1++)
		{
			ADelete(gVPFenceGuys[loop1]);
		}
		fFenceMode = FALSE;
	}

	if(gViewPortGuy)
		ADelete(gViewPortGuy);
	if(gViewPortCursor)
		ADelete(gViewPortCursor);
	if(gItemCursorCel)
		ADelete(gItemCursorCel);

	for(loop1 = 0; loop1 <= highestAnim; loop1++)
	{
		if(gAnims[loop1])
			DeleteAni(gAnims[loop1]);
	}

	lScreenAnims.Release();

	if(gContext)
	{
		Context*	pContext;
		pContext = ADerefAs(Context, gContext);
		pContext->CutObject(gSelf, CT_MSGS|CT_SERV);
	}
}

ViewPort::~ViewPort()
{
}

bool
ViewPort::EdgeScroll(uint32 mouseX, uint32 mouseY)
{
	int32	scrollX = 0, scrollY = 0, pixWidth, pixHeight;
	
	pixWidth = VPORT_WIDTH * TILE_WIDTH;
	pixHeight = VPORT_HEIGHT * TILE_HEIGHT;
	
	// determine which of 9 areas was clicked in
	if(mouseX < pixWidth / 3)
		scrollX = -1;
	else if(mouseX > pixWidth - (pixWidth / 3))
		scrollX = 1;
	else scrollX = 0;

	if(mouseY < pixHeight / 3)
		scrollY = -1;
	else if(mouseY > pixHeight - (pixHeight / 3))
		scrollY = 1;
	else scrollY = 0;

//	if(mouseX < rInner.x1 && ViewX > 0)
//		scrollX = -1;
//	else if(mouseX > rInner.x2 && (ViewX2 < WORLD_WIDTH - 1))
//		scrollX = 1;
//
//	if(mouseY < rInner.y1 && ViewY > 0)
//		scrollY = -1;
//	else if(mouseY > rInner.y2 && (ViewY2 < WORLD_HEIGHT - 1))
//		scrollY = 1;

	if(!scrollX && !scrollY)	// safety check
		return(FALSE);		// no scrolling took place
	
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	MoveView(ViewX + scrollX, ViewY + scrollY);
	pWorldMap->MoveCursor(ViewX, ViewY);
	return(TRUE);
}

void
ViewPort::MoveView(int newX, int newY, bool fSliding)
{
	if(fFenceMode)
	{
		// NO viewport movement allowed during group selection!
		return;
	}

	// if sliding not currently on, and we're in the map title row,
	// and we're leaving that row

//	Debug.Out("ViewPort;:MoveView(%d, %d, %d)\n", newX, newY, (int)fSliding);

	if(viewXDest == 999 && ViewY <= -2 && newY > -2)
	{
		// turn off the map title
		Guy	*pGuy = ADerefAs(Guy, gMapTitleGuy);
		pGuy->Hide();
	}

	SetViewPos(newX, newY);

	if(!fSliding)
	{
		// cancel any viewport sliding that may be occuring
		viewXDest = 999;
		skipViewPortCycle = 0;
//		viewXDest = ViewX;
//		viewYDest = ViewY;
		if(viewXOffset || viewYOffset)
			SetViewOffset(0, 0);
	}
}

void
ViewPort::SetViewOffset(int xOff, int yOff)
{
	Guy		*pGuy;
	grip		gAni;
	BAM_Ani	*pAni;

	if(viewXOffset != xOff || viewYOffset != yOff)
	{
		// if map title bar is showing
		pGuy = ADerefAs(Guy, gMapTitleGuy);
		if(!pGuy->CheckState(S_HIDDEN))
		{
			// adjust vert pos
			if(ViewY == -2)
				pGuy->SetPos(0, yOff);
			else
			{
				pGuy->SetPos(0, yOff - TILE_HEIGHT);
				if(yOff == 0)
					pGuy->Hide();
			}
		}
		
		viewXOffset = xOff;
		viewYOffset = yOff;

		// set visual offset to default
		pGuy = ADerefAs(Guy, gViewPortGuy);
		pGuy->SetPos(-TILE_WIDTH + xOff, -TILE_HEIGHT + yOff);
		pGuy->SetState(S_CHANGED, FALSE);
		AUpdateRect(&rInner);

		gAni = (grip)lScreenAnims.FirstValue();
		while(gAni)
		{
			pAni = ADerefAs(BAM_Ani, gAni);
			RepositionAni(pAni);
			gAni = (grip)lScreenAnims.NextValue();	
		}
	}
	DrawCursor();		// movement of viewport allows cursor to lag behind, so redraw cursor as necessary
}

void
ViewPort::SetViewPos(int newX, int newY)
{
	grip	*pGrip;
	Guy	*pGuy;
	int	loop1;
//	BAM_Ani	*pAni;

	// normalize within limits
	newX = AMax(newX, -1);	newX = AMin(newX, WORLD_WIDTH - VPORT_WIDTH + 1);
	newY = AMax(newY, -2);	newY = AMin(newY, WORLD_HEIGHT - VPORT_HEIGHT + 1);

	// if vert tile pos is changing
	if(ViewY != newY)
	{
		pGuy = ADerefAs(Guy, gMapTitleGuy);
		if(newY < -1)
		{
			if(pGuy->CheckState(S_HIDDEN))
				pGuy->Show();
			pGuy->SetPos(0, viewYOffset);
		}
		else if(newY > -1 && ViewY <= -1)
		{
			// leaving title bar area for sure
			if(!pGuy->CheckState(S_HIDDEN))
				pGuy->Hide();
		}
	}

	ViewX = newX;
	ViewY = newY;
	ViewX2 = ViewX + VPORT_WIDTH - 1;
	ViewY2 = ViewY + VPORT_HEIGHT - 1;

	Draw();
	DrawCursor();		// movement of viewport allows cursor to lag behind, so redraw cursor as necessary

	PurgeAnis();	// delete from screen all ANIs already displayed

//	if(fShowCurrentUnitPath)
//		pMono->Out("ViewPort::SetViewPos(newX, newY);\n");
	// scan list of Ani's to see which ones are now on-screen
	bool	fReport;
	for(loop1 = 0, pGrip = &gAnims[0]; loop1 <= highestAnim; loop1++, pGrip++)
	{
		if(*pGrip)
		{
			fReport = CheckAni(*pGrip);
//			if(fShowCurrentUnitPath && fReport)
//			{
//				pAni = ADerefAs(BAM_Ani, *pGrip);
//				pMono->Out("ani g%d@%d/%d visible\n", *pGrip, pAni->tileX, pAni->tileY);
//			}
		}
	}

//	skipViewPortCycle = FALSE;
	pWorld->UpdateAllSoundVolumes();
}

void
ViewPort::SlideView(int tileX, int tileY, int newSlideIncr)
{	
	int	newX, newY;

	newX = AMax(tileX, -1);	newX = AMin(newX, WORLD_WIDTH - VPORT_WIDTH + 1);
	newY = AMax(tileY, -2);	newY = AMin(newY, WORLD_HEIGHT - VPORT_HEIGHT + 1);
	if(newX == ViewX && newY == ViewY)
	{
		// nevermind, we're at the edge of the world already
		return;
	}

	if(bGlobal.storyLine != NETGAME)
	{
		skipViewPortCycle = AMin(skipViewPortCycle + 6, 6);
		// cant do this in net mode - automatic de-sync
	}

	if(tileX == 999)
	{
		// default same dir as last slide
		viewXDest = ViewX + prevSlideX;
		viewYDest = ViewY + prevSlideY;
	}
	else
	{
		viewXDest = tileX;
		viewYDest = tileY;
	}

	slideIncr = AMin(newSlideIncr, 12);
	// sliding will occur in ViewPort::Cycle()
}

void
ViewPort::RepositionAni(BAM_Ani *pAni)
{
	int32		newPri;
	pAni->SetPos((pAni->tileX - (ViewX + 1)) * TILE_WIDTH + viewXOffset + pAni->tileXOff,
		(pAni->tileY - (ViewY + 1)) * TILE_HEIGHT + viewYOffset + pAni->tileYOff);

	// set priority according to tile Y
	newPri = VPORT_PRI + pAni->priorityLevel + ((pAni->tileY - (ViewY + 1)) * TILE_HEIGHT + pAni->tileYOff) * 2;
	pAni->SetPri(newPri);		
}

bool
ViewPort::CheckAni(grip gAni)
{
	BAM_Ani	*pAni;
	bool		fAniHidden = FALSE;
	int		tileX, tileY;
	Unit		*pUnit;
	
	// is this Ani on the map where the ViewPort is looking?

	pAni =  ADerefAs(BAM_Ani, gAni);
	pUnit = (Unit *)pAni;

	if(!pAni->scrim.resNum)
	{
		// ani is not even set up yet
		return(FALSE);
	}

	tileX = pAni->tileX;
	tileY = pAni->tileY;

	if(!InViewPort(tileX, tileY, TERRAIN_ANI))
	{
		// outside of viewport area - forget it
		fAniHidden = TRUE;
	}
	else
	{
		if(pAni->selfType == UNIT && pUnit->type == JUGGERNAUT)
		{
			if(fog[tileY][tileX] & FOG_CENTER && fog[tileY - 1][tileX] & FOG_CENTER &&
				fog[tileY][tileX - 1] & FOG_CENTER && fog[tileY - 1][tileX - 1] & FOG_CENTER)
			{
				// totally hidden
				fAniHidden = TRUE;
			}
			// else partially hidden - let him show
		}
		else if(fog[tileY][tileX] & FOG_CENTER)
		{
			// tile is fogged - but check for exceptions
			switch(pAni->scrim.resNum)
			{
				case ANIM_CURSOR:
				case ANIM_CONFIRM_MOVE:
				case ANIM_CONFIRM_ATTACK:
					// exceptions to the rule
					break;

				default:
					// nope, not important enough
					fAniHidden = TRUE;
					break;
			}
		}		
	}

	if(fAniHidden)
	{
		// ani should not be showing

		if(!pAni->CheckState(S_HIDDEN))	// if showing
		{
			// remove from screen list, and un-show (these two should be synonymous)
			lScreenAnims.Delete((void *)pAni->gSelf);
			pAni->Hide();
		}
		return(FALSE);
	}
	else
	{
		// ani should be showing
		if(pAni->CheckState(S_HIDDEN))	// if not already on screen
		{
			lScreenAnims.Add((void *)pAni->gSelf);
			RepositionAni(pAni);
			pAni->Show();
		}
		else
		{
			RepositionAni(pAni);
		}
		return(TRUE);
	}
}

void
ViewPort::UpdateTrailCursor(void)
{
	Guy	*pGuy, *pVPGuy;
	int32	newX, newY, mouseX, mouseY;

	pGuy = ADerefAs(Guy, gVPTrailCursor);		// trailing-cursor in viewport
	pVPGuy = ADerefAs(Guy, gViewPortGuy);		// to get viewport rect

	mouseX = pMouse->GetX();
	mouseY = pMouse->GetY();
	if(!pVPGuy->scrim.rect.Contains(mouseX, mouseY))
	{
		pGuy->Hide();
		return;
	}
	newX = mouseX / TILE_WIDTH;
	newY = mouseY / TILE_HEIGHT;
	if(trailCursorX != newX || trailCursorY != newY)
	{
		trailCursorX = newX;
		trailCursorY = newY;
		DrawTrailCursor();
	}
}

void
ViewPort::DrawTrailCursor(void)
{
	Guy	*pGuy;
	int32	newX, newY;
	Unit	*pUnit;

	pGuy = ADerefAs(Guy, gVPTrailCursor);
	pUnit = pWorld->unitLib.GetUnit(ViewX + 1 + trailCursorX, ViewY + 1 + trailCursorY);
	if(pUnit)
	{
		newX = (pUnit->tileX - (ViewX + 1)) * TILE_WIDTH + pUnit->tileXOff;
		newY = (pUnit->tileY - (ViewY + 1)) * TILE_HEIGHT + pUnit->tileYOff;
	}
	else
	{
		newX = trailCursorX * TILE_WIDTH + TILE_WIDTH / 2 - 1;
		newY = trailCursorY * TILE_HEIGHT + TILE_HEIGHT / 2 - 1;
	}
	pGuy->SetPos(newX, newY);
	if(pGuy->CheckState(S_HIDDEN))
	{
		pGuy->Show();
	}
}

void
ViewPort::DrawCursor(void)
{
	Guy	*pGuy;

	pGuy = ADerefAs(Guy, gViewPortCursor);

	if(CursorX >= ViewX && CursorX < ViewX + VPORT_WIDTH && CursorY >= ViewY && CursorY < ViewY + VPORT_HEIGHT)
	{
		pGuy->SetPos((CursorX - (ViewX + 1)) * TILE_WIDTH + CursorXOff + viewXOffset,
			(CursorY - (ViewY + 1)) * TILE_HEIGHT + CursorYOff + viewYOffset);

		if(pGuy->CheckState(S_HIDDEN))
		{
			pGuy->Show();
		}
	}
	else if(!pGuy->CheckState(S_HIDDEN))
	{
		pGuy->Hide();
	}
}

void
ViewPort::SetCursorPos(int32 NewCursorX, int32 NewCursorY, int32 newXOff, int32 newYOff)
{
	WorldMap		*pWM;		 

	CursorX = NewCursorX;
	CursorY = NewCursorY;
	CursorXOff = newXOff;
	CursorYOff = newYOff;
	ClipCursorPos();
	DrawCursor();

	pWM = ADerefAs(WorldMap, pWorld->gWorldMap);
	pWM->MoveShadowCursor(CursorX,CursorY);
}

void
ViewPort::ClipCursorPos(void)	// insure that cursor is position within world map boundaries
{
	if(CursorX >= WORLD_WIDTH)
		CursorX = WORLD_WIDTH - 1;
	if(CursorX < 0)
		CursorX = 0;
	if(CursorY >= WORLD_HEIGHT)
		CursorY = WORLD_HEIGHT - 1;
	if(CursorY < 0)
		CursorY = 0;
}

TerrainUnit *
ViewPort::GetTerrainUnit(int32 newX, int32 newY)
{
	grip 	g;
	TerrainUnit	*pTUnit = NULL;

	g = activeTerrain[newX][newY];
	if(g)
		if(pMemMgr->CheckGrip(g) == GRIP_VALID)
			pTUnit = ADerefAs(TerrainUnit, g);
	
	return(pTUnit);
}

BAM_Ani *
ViewPort::NewTerrainUnit(activeTerrain_t newType, uint32 newX, uint32 newY)
{
	TerrainUnit	*pTUnit;
	uint32		newAni, newCel, newDelay;
	bool			fNewFwd, fNewCycle, fNewPerm, fStartUpscale = FALSE;
	ticks_t		tNewCycleTime;

	switch(newType)
	{
		case ACTIVE_FLAME_SPOUT:
			switch((terrainType)pWorld->map.GetTerrainNum(newX, newY))
			{
				case FLAME_SPOUT:
					tNewCycleTime = ATicks() + ARandom(TICKS_PER_SEC * FLAMESPOUT_MAXWAIT);
					fNewPerm = TRUE;
					break;

				case BRIDGE:
				case CHASM:
					fStartUpscale = TRUE;
				default:					
					tNewCycleTime = ATicks() + TICKS_PER_SEC;
					fNewPerm = FALSE;
					break;
			}

			newAni = ANIM_FLAME_SPOUT;
			newCel = 1;
			newDelay = 3;
			fNewFwd = TRUE;
			fNewCycle = FALSE;
			break;

		case ACTIVE_FIRE:
			// if HERNE'S WOOD map
			if(!pWorld->map.FireCanBurnAt(newX, newY))
			{
				if(pWorld->map.rMapNum % 9170 < 10)
				{
					pWorld->Announce(newX, newY, SQUIB_WORLD_MSGS, SQUIB_WORLD_NOSMOKING);
				}
				return(NULL);
			}

			newAni = ANIM_FIRE;
			newCel = 1;
			newDelay = 3;
			fNewFwd = TRUE;
			fNewCycle = TRUE;
			fNewPerm = FALSE;
			tNewCycleTime = ATicks() + ARandom(TICKS_PER_SEC);
			break;
		
		case ACTIVE_ARCH_FIRE:
			if(!pWorld->map.FireCanBurnAt(newX, newY))
				return(NULL);

			newAni = ANIM_ARCH_FIRE;
			newCel = 1;
			newDelay = 3;
			fNewFwd = TRUE;
			fNewCycle = TRUE;
			fNewPerm = FALSE;
			tNewCycleTime = ATicks() + TICKS_PER_SEC + ARandom(TICKS_PER_SEC);
			break;

		default:
			pMono->Out("ViewPort::NewTerrainUnit(%d) - invalid type\n", (int32)newType);
			return(NULL);
	}

	pTUnit = GetTerrainUnit(newX, newY);

	// if no existing terrain unit at this X,Y
	if(!pTUnit)
	{
		TRACK_MEM("TerrainUnit");	pTUnit = new TerrainUnit;
	}
	else
	{
		// recycle the TerrainUnit that's already here
		pTUnit->tAniStarted = ATicks();	//refresh birth time
	}

	pTUnit->priorityLevel = PRI_WALKER + 2;
	pTUnit->tCycleNext = tNewCycleTime;
	pTUnit->activeType = newType;
	pTUnit->SetContext(pWorld->gSelf);
	AddAni(pTUnit->gSelf);
	pTUnit->fAniPermanent = fNewPerm;
	pTUnit->RunAnim(newAni, fNewFwd, fNewCycle, newDelay);
	pTUnit->SetTilePos(newX, newY);
	if(fStartUpscale)
		pTUnit->SetScale(128);
	return(pTUnit);
}

// add an animation (non-item, non-unit) to the system
grip
ViewPort::NewAni(res_t newType, uint32 newAni, uint32 newCel, int32 newPri, bool fNewFwd, bool fNewCycle,
	uint32 newDelay, uint32 newX, uint32 newY, bool fNewPerm, uint32 newXOff, uint32 newYOff)
{
	BAM_Ani	*pAni;
	grip		gAni;

	pAni = new BAM_Ani;		// TRACK_MEM should be called before calling NewAni

	pAni->priorityLevel = newPri;

	gAni = pAni->gSelf;
	if(newCel == 999)
	{
		pAni->SetRes(newType,newAni, 1);
		newCel = pAni->scrim.numCels;
	}

	pAni->SetRes(newType,newAni, newCel);
	pAni->animDelay = newDelay;
	pAni->fAniFwd = fNewFwd;
	pAni->fAniCycle = fNewCycle;
	pAni->fAniPermanent = fNewPerm;
	if(newAni)
	{
		pAni->SetTilePos(newX, newY, newXOff, newYOff);
		pAni->fWorldMapUpdate = FALSE;
		pAni->SetRect();
	}
	pAni->SetContext(pWorld->gSelf);
	AddAni(gAni);

//	FILE	*fOutfile = fopen("allocs.grp", "a");
//	fprintf(fOutfile, "Ani g%d\n", (int)gAni);
//	fclose(fOutfile);

	return(gAni);
}

void
ViewPort::RestoreSourceAsTarget(void)
{
	targType = verbSourceType;
	targSerial = verbSourceSerial;
	targX = verbSourceX;
	targY = verbSourceY;
}

void
ViewPort::MoveCursor(int32 NewCursorX, int32 NewCursorY, bool fJustTargeting)
{
	Unit		*pUnit;
	MapSpace	*pSpace;

	// if really meaning to move the cursor, and not just select a target (which is always
	// followed by returning the cursor to the source, rather than lingering on the target)
	if(!fJustTargeting)
	{
		// notify previous target that they are no longer being viewed (and thus can
		// stop updating the status box when their stats change)

		pUnit = (Unit *)pWorld->DerefSerial(targSerial);
		pSpace = (MapSpace *)pUnit;
		if(targType == UNIT)	// if already tracking a unit
		{
			if(pUnit)
			{
				// stop tracking it		
				pUnit->SetTracking(FALSE);
			}
		}
		else if(targType == BUILDING)
		{
			if(pSpace)
			{
				pSpace->SetTracking(FALSE);
			}
		}
	}

	// if we've moved the cursor anywhere while in UNIT TRANSFORM mode, must return to normal mode.
	if(targType == UNIT && pWorld->statBoxCurrMode != TERRAIN)
	{
		pWorld->SetStatusBoxMode(TERRAIN);
		SetCurrentVerb(NO_ACTION);
	}

	if(fog[NewCursorY][NewCursorX] & FOG_CENTER)
	{
		// fog at new X,Y - can't give away any secrets!
		if(fJustTargeting)
		{
			targType = TERRAIN;
			targSerial = NULL;
			targX = NewCursorX;
			targY = NewCursorY;
		}
		else
		{
			SetCursorPos(NewCursorX, NewCursorY);

			// Override targetting - dont allow the user to peek under the fog.
			// kludgy - one SetCursorTarget() should handle all cases
			SetCursorTarget(NOTARGET, CursorX, CursorY);
		}
	}
	else
	{
		pUnit = pWorld->unitLib.GetUnit(NewCursorX, NewCursorY);		// is there a unit at this place?
		if(pUnit)				// if unit under cursor
		{
			if(fJustTargeting)
			{
				// set target info manually here, dont worry about updating the screen and moving the
				// cursor - we just want targetting info
				targType = UNIT;
				targSerial = pUnit->serialNum;
				targX = NewCursorX;
				targY = NewCursorY;
			}
			else
			{
				// unit will cause cursor tracking && status box update, because it will
				// will know when its own stats change.
				pUnit->SetTracking(TRUE);		
				SetCursorPos(NewCursorX, NewCursorY);
				SetCursorTarget(UNIT, pUnit->serialNum);	// lock cursor to unit #X
				UpdateTracking();
				if(pUnit->primaryAction == TRANSFORM)
				{
					pWorld->SetStatusBoxMode(TRANSFORMING);
				}
			}
		}
		else
		{
			pSpace = pWorld->map.GetStructure(NewCursorX, NewCursorY);

			// is it actually a building or owned space, or just a map marker?
			if(pSpace && ((pSpace->h.maxHP && !pSpace->IsFoundation()) || pSpace->h.owner == SIDE1 ||
				pSpace->h.owner == SIDE2))
			{
				// ERECT buildings (not foundations), and portals

				if(fJustTargeting)
				{
					targType = BUILDING;
					targSerial = pSpace->serialNum;
					targX = NewCursorX;
					targY = NewCursorY;
				}
				else
				{
					pSpace->SetTracking(TRUE);		// cause auto-update of HP guage
					SetCursorPos(NewCursorX, NewCursorY);
					SetCursorTarget(BUILDING, pSpace->serialNum, CursorX, CursorY);
				}
			}
			else
			{
				if(fJustTargeting)
				{
					targType = TERRAIN;
					targSerial = NULL;
					targX = NewCursorX;
					targY = NewCursorY;
				}
				else
				{
					SetCursorPos(NewCursorX, NewCursorY);
					SetCursorTarget(TERRAIN, CursorX, CursorY);	// else examine terrain (covers items)
				}
			}
		}
	}

	if(!fJustTargeting)
	{
		pWorld->DrawStatusBox();

		if(fAutoCommandMode || currentVerb)
			SetCurrentVerb(NO_ACTION);
	}
}

// move viewport to center on target (which the cursor should have already been tracking on)
void
ViewPort::GoToTarget(void)
{
	WorldMap	*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);

	if(targType != NOTARGET)
	{
		MoveView(targX - VPORT_WIDTH / 2, targY - VPORT_HEIGHT / 2);
		pWorldMap->MoveCursor(ViewX, ViewY);
	}
}

void
ViewPort::SetCursorTarget(TargetType newTargType, uint32 newSerial, coord newX, coord newY)
{
	MapSpace	*pSpace;
	uint32	cursorType;
	Guy		*pGuy;
	
	targType = newTargType;
	targX = newX;
	targY = newY;
	targSerial = newSerial;
	pSpace = (MapSpace *)pWorld->DerefSerial(newSerial);

	switch(pSpace->h.owner)
	{
		case SIDE0:
			cursorType = 1;
			break;
		case SIDE1:
			cursorType = 2;
			break;
		case SIDE2:
			cursorType = 3;
			break;
	}
	pGuy = ADerefAs(Guy, gViewPortCursor);
	pGuy->SetRes(RES_ANIM,9002, cursorType);
	pGuy->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[pSpace->h.owner]);
	ADerefAs(WorldMap, pWorld->gWorldMap)->shadowCursor.SetRes(RES_ANIM,
		ANIM_SHADOW_CURSOR, SHADOW_POINT);
}

void
ViewPort::SetCursorTarget(TargetType newTargType, coord newX, coord newY)
{
	uint32	cursorType;
	Guy		*pGuy;

	pGuy = ADerefAs(Guy, gViewPortCursor);
	targType = newTargType;
	targX = newX;
	targY = newY;

	pGuy->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[SIDE0]);

	if(newTargType == NOTARGET)
	{
		targSerial = NULL;
		pGuy->SetRes(RES_ANIM,9002, 1);		// neutral
		return;
	}

	targSerial = pWorld->map.MapTiles[newY * WORLD_WIDTH + newX];
/*	switch(SIDE0)
	{
		case SIDE0:
			cursorType = 1;
			break;
		case SIDE1:
			cursorType = 2;
			break;
		case SIDE2:
			cursorType = 3;
			break;
	}*/
	cursorType = 1;

	pGuy->SetRes(RES_ANIM,9002, cursorType);
	ADerefAs(WorldMap, pWorld->gWorldMap)->shadowCursor.SetRes(RES_ANIM,
		ANIM_SHADOW_CURSOR, SHADOW_POINT);
}

void
ViewPort::SetCursorTarget(TargetType newTargType, uint32 newSerial, Guy *pAltCursor)
{
	Unit		*pUnit;
	Guy		*pGuy;
	uint32	cursorType;

	if(pAltCursor)
		pGuy = pAltCursor;
	else
	{
		pGuy = ADerefAs(Guy, gViewPortCursor);

		targType = newTargType;
		targSerial = newSerial;
	}

	if(newTargType == NOTARGET)
	{
		pGuy->SetRes(RES_ANIM,9002, 1);		// neutral
		return;
	}

	pUnit = (Unit *)pWorld->DerefSerial(newSerial);
	switch(pUnit->player)
	{
		case SIDE0:
		case SIDE3:
			cursorType = 4;
			break;
		case SIDE1:
			cursorType = 5;
			break;
		case SIDE2:
			cursorType = 6;
			break;
	}
	pGuy->SetRes(RES_ANIM,9002, cursorType);
	pGuy->SetClut(CLUT_TEAM_COLORS, CLUT_SIZE, pWorld->clut[pUnit->player]);
	if(!pAltCursor)
	{
		if(pUnit->skillRange == 3)
		{
			ADerefAs(WorldMap, pWorld->gWorldMap)->shadowCursor.SetRes(RES_ANIM,
				ANIM_SHADOW_CURSOR, SHADOW_BOX);
		}
		else
		{
			ADerefAs(WorldMap, pWorld->gWorldMap)->shadowCursor.SetRes(RES_ANIM,
				ANIM_SHADOW_CURSOR, SHADOW_POINT);
		}
		if(fShowCurrentUnitPath)
			ShowUnitPath(pUnit);
	}
}

void
ViewPort::UpdateTracking(void)
{
	Unit			*pUnit;
	TargetType	targ1;

	pUnit = (Unit *)pWorld->DerefSerial(targSerial, &targ1);
	if(!pUnit || targ1 != UNIT)
	{
		// how did this happen?  Well, try to manage.
		MoveCursor(targX, targY);
		return;
	}

	// removed verify below because target selection no longer causes actual tracking to change
	if(pUnit->fCursorTracking)	// verify that this is the unit we're tracking (just to be safe)
	{
		targX = pUnit->tileX;
		targY = pUnit->tileY;
		if(fog[targY][targX] == FOG_CENTER)
		{
			// unit has moved under fog - break link
			pUnit->fCursorTracking = FALSE;
			MoveCursor(targX, targY);
		}
		else if(pUnit->type == TORTOISE)
			SetCursorPos(targX, targY, pUnit->tileXOff, pUnit->tileYOff + 4);
		else SetCursorPos(targX, targY, pUnit->tileXOff, pUnit->tileYOff);
	}
	else
	{
		// error
		MoveCursor(targX, targY);
//		targX = 0;
//		targY = 0;
	}
}

// defog all terrain around unit/structures/portals for player X
void
ViewPort::DefogAll(uint32 side)
{
	// skip units - they defog themselves via SetTilePos()

	MapSpace	*pSpace;
	uint32	loopX, loopY;

	for(loopY = 0; loopY <= WORLD_HEIGHT; loopY++)
		for(loopX = 0; loopX <= WORLD_WIDTH; loopX++)
		{
			pSpace = pWorld->map.GetStructure(loopX, loopY);
			if(pSpace)
			{
				if(pSpace->h.owner == side)
				{
					Defogger(loopX - 1, loopY - 1, loopX + 1, loopY + 1);
				}
			}

		}
}

Rectangle	rDefoggerRect;
void
ViewPort::Defogger(int32 x1, int32 y1, int32 x2, int32 y2)
{
	int32			loopX, loopY, vpX, vpY, *pFogMap, *pFogMapLine;
	WorldMap		*pWorldMap = ADerefAs(WorldMap, pWorld->gWorldMap);
	grip			gUnit;
	BAMItem		*pItem;

	x1 = AMax(x1, 0);	x1 = AMin(WORLD_WIDTH - 1, x1);
	x2 = AMax(x2, 0);	x2 = AMin(WORLD_WIDTH - 1, x2);
	y1 = AMax(y1, 0);	y1 = AMin(WORLD_HEIGHT - 1, y1);
	y2 = AMax(y2, 0);	y2 = AMin(WORLD_HEIGHT - 1, y2);

	// adjust fog flags in world (independent of viewport)
	for(loopY = y1, pFogMapLine = &fog[y1][x1]; loopY <= y2; loopY++, pFogMapLine += WORLD_WIDTH)
		for(loopX = x1, pFogMap = pFogMapLine; loopX <= x2; loopX++, pFogMap++)
		{
			if(*pFogMap & FOG_CENTER)		// if tile was fogged
			{
				*pFogMap &= ~FOG_CENTER;
				*pFogMap |= FOG_FRESH;

				if(InViewPort(loopX, loopY))	// if on-screen
				{
					gUnit = pWorld->unitLib.GetUnitGrip(loopX, loopY);	
					if(gUnit)	// if there's a unit here
						CheckAni(gUnit);

					pItem = pWorld->itemMgr.CheckItem(loopX, loopY);
					if(pItem)	// if there's an item here
						CheckAni(pItem->gSelf);

					// any background animations now uncovered? (fountains, portals, etc)
					grip		gAni;
					BAM_Ani	*pAni;
					gAni = (grip)pWorld->lBackgroundAnis.FirstValue();
					while(gAni)
					{
						pAni = ADerefAs(BAM_Ani, gAni);
						if(InViewPort(pAni->tileX, pAni->tileY))
							CheckAni(gAni);
						gAni = (grip)pWorld->lBackgroundAnis.NextValue();
					}
				}

				// was a targetted tile de-fogged?
				if(CursorX == loopX && CursorY == loopY)
				{
					MoveCursor(CursorX, CursorY);
				}

				pWorldMap->Draw(loopX, loopY);
			}
		}

	// expand rectangle for fog re-scan
	x1--;	x2++;	y1--;	y2++;
	x1 = AMax(x1, 0);	x1 = AMin(WORLD_WIDTH - 1, x1);
	x2 = AMax(x2, 0);	x2 = AMin(WORLD_WIDTH - 1, x2);
	y1 = AMax(y1, 0);	y1 = AMin(WORLD_HEIGHT - 1, y1);
	y2 = AMax(y2, 0);	y2 = AMin(WORLD_HEIGHT - 1, y2);

	// scan all explored terrain (freshly-revealed or not) in this area for fog
	uint32 	fogVal, fogCount = 0;
	bool		fExistingFogAnim = FALSE;
	pFogMapLine = &fog[y1][x1];

	for(loopY = y1, vpY = y1 - (ViewY + 1) + 1; loopY <= y2; loopY++, vpY++, pFogMapLine += WORLD_WIDTH)
	{
		for(loopX = x1, vpX = x1 - (ViewX + 1) + 1, pFogMap = pFogMapLine; loopX <= x2; loopX++, vpX++, pFogMap++)
		{
//			fogVal = fog[loopY][loopX];
			fogVal = *pFogMap;
			if(!(fogVal & FOG_CENTER))		// if space is revealed (freshly or otherwise)
			{
				fogVal &= ~FOG_FRESH;			// turn off FRESH flag, if on
				fExistingFogAnim = ((fogVal & FOG_ALL)? TRUE: FALSE);
				if(fog[loopY][loopX - 1] & FOG_CENTER && loopX > 0)	// if fog to the west
					fogVal |= FOG_WEST;
				else fogVal &= ~FOG_WEST;
				if(fog[loopY][loopX + 1] & FOG_CENTER && loopX < WORLD_WIDTH - 1)	// if fog to the east
					fogVal |= FOG_EAST;
				else fogVal &= ~FOG_EAST;
				if(fog[loopY - 1][loopX] & FOG_CENTER && loopY > 0)	// if fog to the north
					fogVal |= FOG_NORTH;
				else fogVal &= ~FOG_NORTH;
				if(fog[loopY + 1][loopX] & FOG_CENTER && loopY < WORLD_HEIGHT - 1)	// if fog to the south
					fogVal |= FOG_SOUTH;
				else fogVal &= ~FOG_SOUTH;
	
//				if(fog[loopY][loopX] != fogVal && InViewPort(loopX, loopY))
				if(*pFogMap != fogVal && InViewPort(loopX, loopY))
				{
//					fog[loopY][loopX] = fogVal;
					*pFogMap = fogVal;
					Draw(vpX, vpY);
					rDefoggerRect.Set((vpX - 1) * TILE_WIDTH, (vpY - 1) * TILE_HEIGHT, vpX * TILE_WIDTH - 1,
						vpY * TILE_HEIGHT - 1);
					AUpdateRect(&rDefoggerRect);
				}
//				else fog[loopY][loopX] = fogVal;
				else *pFogMap = fogVal;
			}
		}
	}
}

bool
ViewPort::InViewPort(int32 x, int32 y, TargetType target)
{
	int	x1, x2, y1, y2;
	bool	fInside;

	x1 = ViewX;		x2 = ViewX2;	y1 = ViewY;		y2 = ViewY2;

	switch(target)
	{
		case UNIT:
		case TERRAIN_ANI:
			// expand viewport search boundaries by 1
			// some animations overlap into other tiles
			x1--;
			x2++;
			y1--;
			y2++;
			break;
	}
	x1 = AMax(x1, 0);
	x2 = AMin(x2, WORLD_WIDTH - 1);
	y1 = AMax(y1, 0);
	y2 = AMin(y2, WORLD_HEIGHT - 1);

	fInside = (x >= x1 && x <= x2 && y >= y1 && y <= y2)? TRUE : FALSE;
	return(fInside);
}

void
ViewPort::Draw(void)
{
	Rectangle	rMapCurrent(ViewX, ViewY, ViewX2, ViewY2),
					rMapLast(lastViewX, lastViewY, lastViewX2, lastViewY2),
					rMapOverlap;

//	pMono->Out("Vx%d Vy%d vx%d vy%d\n", ViewX, ViewY, ViewX2, ViewY2);

	if(lastViewX == 999)
	{
		Draw(0, 0, VPORT_WIDTH - 1, VPORT_HEIGHT - 1);
	}
	else
	{
		// clip last area drawn to current area to draw - any overlap?

		int	xDelta, yDelta;
		xDelta = ViewX - lastViewX;
		yDelta = ViewY - lastViewY;

		if(abs(xDelta) > 5 || abs(yDelta) > 5)
		{
			// rebuild from scratch
			Draw(0, 0, VPORT_WIDTH - 1, VPORT_HEIGHT - 1);
		}
		else
		{
			int			xSource, ySource, width, height, xDest, yDest;
			Rectangle	rVPRebuild;
			CelHeader	*pCel;
			uchar			*pCelDest, *pCelSrc;

			if(xDelta > 0)
			{
				// viewport is moving to the right, so move data to the left
				xSource = xDelta;
			}
			else
			{
				xSource = 0;
			}
			width = (VPORT_WIDTH - abs(xDelta)) * TILE_WIDTH;
			
			if(yDelta > 0)
			{
				// viewport is moving down, so move data up
				ySource = yDelta;
			}
			else
			{
				ySource = 0;
			}
			height = (VPORT_HEIGHT - abs(yDelta)) * TILE_HEIGHT;

			xDest = (xSource - xDelta) * TILE_WIDTH;
			yDest = (ySource - yDelta) * TILE_HEIGHT;
			pCel = (CelHeader *) AGetResData(gViewPortCel);
			pCelSrc = (uchar *)pCel + sizeof(CelHeader);
			pCelDest = (uchar *)pCel + sizeof(CelHeader);

			// preserve overlapped area first
			int	loopY;
			if(yDelta > 0 || (yDelta == 0 && xDelta > 0))
			{
				// forward copy
				pCelSrc += pCel->width * (ySource * TILE_HEIGHT)
					+ xSource * TILE_WIDTH;
				pCelDest += pCel->width * yDest + xDest;
//				pMono->Out("pCelSrc %x  pCelDest %x\n", (void *)pCelSrc, (void *)pCelDest);
				for(loopY = 0; loopY < height; loopY++, pCelSrc += pCel->width,
					pCelDest += pCel->width)
					memmove((void *)pCelDest, (void *)pCelSrc, width);
//				pMono->Out("pCelSrc %x  pCelDest %x\n", (void *)pCelSrc, (void *)pCelDest);
			}
			else
			{
				// reverse copy
				pCelSrc += pCel->width * (ySource * TILE_HEIGHT + height - 1)
					+ xSource * TILE_WIDTH;
				pCelDest += pCel->width * (yDest + height - 1) + xDest;
//				pMono->Out("pCelSrc %x  pCelDest %x\n", (void *)pCelSrc, (void *)pCelDest);
				for(loopY = height - 1; loopY >= 0; loopY--, pCelSrc -= pCel->width,
					pCelDest -= pCel->width)
					memmove((void *)pCelDest, (void *)pCelSrc, width);
//				pMono->Out("pCelSrc %x  pCelDest %x\n", (void *)pCelSrc, (void *)pCelDest);
			}
			AUpdateRect(&rInner);
//			pMono->Out("xSrc%d ySrc%d xDst%d yDst%d w%dof%d h%dod%d\n", xSource, ySource,
//				xDest, yDest, width, VPORT_WIDTH * TILE_WIDTH, height, VPORT_HEIGHT * TILE_HEIGHT);

			rVPRebuild.x1 = 0;
			rVPRebuild.x2 = VPORT_WIDTH - 1;
			if(ViewY > lastViewY)
			{
				// going down
				rVPRebuild.y1 = VPORT_HEIGHT - (ViewY - lastViewY);
				rVPRebuild.y2 = VPORT_HEIGHT - 1;
				Draw(rVPRebuild.x1, rVPRebuild.y1, rVPRebuild.x2, rVPRebuild.y2);
			}
			else if(ViewY < lastViewY)
			{
				// going up
				rVPRebuild.y1 = 0;
				rVPRebuild.y2 = lastViewY - ViewY - 1;
				Draw(rVPRebuild.x1, rVPRebuild.y1, rVPRebuild.x2, rVPRebuild.y2);
			}

			rVPRebuild.y1 = 0;
			rVPRebuild.y2 = VPORT_HEIGHT - 1;
			if(ViewX > lastViewX)
			{
				// going right
				rVPRebuild.x1 = VPORT_WIDTH - (ViewX - lastViewX);
				rVPRebuild.x2 = VPORT_WIDTH - 1;
				Draw(rVPRebuild.x1, rVPRebuild.y1, rVPRebuild.x2, rVPRebuild.y2);
			}
			else if(ViewX < lastViewX)
			{
				// going left
				rVPRebuild.x1 = 0;
				rVPRebuild.x2 = lastViewX - ViewX - 1;
				Draw(rVPRebuild.x1, rVPRebuild.y1, rVPRebuild.x2, rVPRebuild.y2);
			}

			// gettin' tricky: update vidmem directly instead of allowing Animate()
			// to do it.  This way we can bypass the extra generation caused by
			// VGABUF.  -V
//			ABlit(pGraphMgr->vgaDriver, buf,	x, y, width, height,
//				pGraphMgr->videoBufferSeg);

			// Now find any rects that we walked on, and have GraphMgr update
			// them normally.

		}
	}

	lastViewX = ViewX;
	lastViewY = ViewY;
	lastViewX2 = ViewX2;
	lastViewY2 = ViewY2;

	MouseHandler(FALSE);
}

void
ViewPort::Draw(int32 x1, int32 y1, int32 x2, int32 y2, bool fUpdateRect)
{
	CelHeader	*pCel;
	uint			tileNum;
	int			bldgOwner, int1;
	int32			*pFogMap;
	int16			*pMapTiles, *pStructures;
	int			mapPos, mapX, mapY, fogType, fogCelNum;
	grip			gFogRes, gClut,
						*pgMapSpaces = &pWorld->map.gSpaces[0], gClut1, gClut2;
 	Rectangle	rArea;
	int			loopX, loopY, loopY2, xOffSet, yOffSet;
	MapSpace		*pSpace;
	tile			*pTileArray = &pWorld->pTileLib->tiles[0];
	uchar			*pCelData;

	gClut1 = pWorld->clut[SIDE1];
	gClut2 = pWorld->clut[SIDE2];

	// clip coords to viewport dimensions.
	x1 = AMax(x1, 0);		x1 = AMin(VPORT_WIDTH - 1, x1);
	if(ViewX + x1 < 0)	// remember viewport extends outside of visible area.
		x1 -= ViewX + x1;	// be careful of drawing off the map entirely
	x2 = AMax(x2, 0);		x2 = AMin(VPORT_WIDTH - 1, x2);
	if(ViewX + x2 >= WORLD_WIDTH)
		x2 -= (ViewX + x2) - WORLD_WIDTH + 1;

	y1 = AMax(y1, 0);		y1 = AMin(VPORT_HEIGHT - 1, y1);
	if(ViewY + y1 < 0)
		y1 -= ViewY + y1;
	y2 = AMax(y2, 0);		y2 = AMin(VPORT_HEIGHT - 1, y2);
	if(ViewY + y2 >= WORLD_HEIGHT)
		y2 -= (ViewY + y2) - WORLD_HEIGHT + 1;

	pCel = (CelHeader *) AGetResData(gViewPortCel);	// dest cel header
	mapPos = ViewX + (ViewY + y1) * WORLD_WIDTH;

	yOffSet = y1 * TILE_HEIGHT;
	for(loopY = y1, mapY = ViewY + y1; loopY <= y2;
		loopY++, yOffSet += TILE_HEIGHT, mapY++, mapPos += WORLD_WIDTH)
  	{
		xOffSet = x1 * TILE_WIDTH;	// pixel offset
		mapX = ViewX + x1;

		pMapTiles = &pWorld->map.MapTiles[mapPos + x1];
		pStructures = &pWorld->map.MapSpaces[mapPos + x1];
		pFogMap = &fog[mapY][mapX];
		for(loopX = x1; loopX <= x2; loopX++, xOffSet += TILE_WIDTH, mapX++,
			pFogMap++, pMapTiles++, pStructures++)
		{
			fogType = *pFogMap;

			// if totally fogged
			if(fogType & FOG_CENTER)
			{
				tileNum = fogTile;	// show black tile
				pCelData = (uchar *)pCel + sizeof(CelHeader) + pCel->width * yOffSet + xOffSet;
				int1 =  pCel->width;
				for(loopY2 = 0; loopY2 < TILE_HEIGHT; loopY2++, pCelData += int1)
					memset(pCelData, CI_BLACK, TILE_WIDTH);
			}
			else
			{
				tileNum = *pMapTiles;

				// check for structure at X,Y and if found, set color shift accordingly
				int1 = *pStructures;

				if(int1)
				{
					pSpace = ADerefAs(MapSpace, pWorld->map.gSpaces[int1]);
					bldgOwner = pSpace->h.owner;
					switch(bldgOwner)
					{
						case SIDE1:
							gClut = gClut1;
							break;

						case SIDE2:
							gClut = gClut2;
							break;

						default:
							gClut = NULL;
							break;
					}
				}
				else
				{
					gClut = NULL;
				}

				CopyBitMap(pCel, xOffSet, yOffSet, (pTileArray + tileNum)->pBitMap,
					TILE_WIDTH, TILE_HEIGHT, FALSE, gClut);

				//------------ fog
				if(fogType & FOG_ALL)		// if any partial fog
				{
					TRACK_MEM("FogRes");		gFogRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, fogAnim[fogType][0]);
					fogCelNum = fogAnim[fogType][1];
					CopyCel(pCel, xOffSet + TILE_HORZ_CENTER, yOffSet + TILE_VERT_CENTER,
						RES_ANIM, fogAnim[fogType][0], fogCelNum, TRUE, NULL, TRUE);
				}		
			}
		}
	}

	if(fUpdateRect)
	{
		// tell graphmgr to update rect of viewport we just drew in
		rArea.Set((x1 - 1) * TILE_WIDTH, (y1 - 1) * TILE_HEIGHT, (x2 * TILE_WIDTH) - 1, (y2 * TILE_HEIGHT) - 1);
		rArea.Clip(&rInner);
		AUpdateRect(&rArea);
	}
	// else assume caller will handle vidmem update
}			 

// draw tile under viewportX,Y
void
ViewPort::Draw(int32 xPos, int32 yPos)
{
	CelHeader	*pCel;
	int			tileNum, mapPos, xOffSet, yOffSet, mapX, mapY, fogType, fogCelNum;
	MapSpace		*pSpace;
	tile			*pTile;
	grip			gFogRes, gClut;

	xOffSet = xPos * TILE_WIDTH;
	yOffSet = yPos * TILE_HEIGHT;
 	Rectangle	rArea(xOffSet, yOffSet, xOffSet + TILE_WIDTH - 1, yOffSet + (TILE_HEIGHT - 1));

	pCel = (CelHeader *) AGetResData(gViewPortCel);
	mapX = ViewX + xPos;
	mapY = ViewY + yPos;
	mapPos = mapX + mapY * WORLD_WIDTH;
	if(mapX < 0 || mapX >= WORLD_WIDTH || mapY < 0 || mapY >= WORLD_HEIGHT)
		fogType = FOG_CENTER;
	else
	fogType = fog[mapY][mapX];
	
	if(fogType & FOG_CENTER)
		tileNum = fogTile;
	else
		tileNum = pWorld->map.MapTiles[mapPos];
	if(tileNum < 0 || tileNum >= pWorld->pTileLib->totalTiles)
		return;
	pTile = &pWorld->pTileLib->tiles[tileNum];

	pSpace = pWorld->map.GetStructure(mapX, mapY);
	if(pSpace && (pSpace->h.owner == SIDE1 || pSpace->h.owner == SIDE2))
	{
		gClut = pWorld->clut[pSpace->h.owner];
	}
	else gClut = NULL;

	CopyBitMap(pCel, xOffSet, yOffSet, pTile->pBitMap, TILE_WIDTH, TILE_HEIGHT, FALSE, gClut);

	/*	sprintf(string1, "%d", tileNum);
	SetFontColors(CI_SKIP, CI_WHITE, CI_BLACK);

	pFontMgr->SetString(xPos * TILE_WIDTH, yPos * TILE_HEIGHT, string1, (uchar *)pCel, pCel->width, NULL);
	*/

	// draw fog, if any
	if(fogType & FOG_ALL)		// if any partial fog
	{
		TRACK_MEM("FogRes");		gFogRes = ALoadDebug(__FILE__, __LINE__, RES_ANIM, fogAnim[fogType][0]);
		fogCelNum = fogAnim[fogType][1];
		CopyCel(pCel, xOffSet + TILE_HORZ_CENTER, yOffSet + TILE_VERT_CENTER,
			RES_ANIM, fogAnim[fogType][0], fogCelNum, TRUE, NULL, TRUE);
	}			
}
