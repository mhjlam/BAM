//	worldmap.hpp
//
//	Copyright 1994, Tachyon, Inc.
//
//
//		9-19-94:
//


#ifndef	WorldMap_hpp
#define	WorldMap_hpp

#include "api.hpp"
#include "context.hpp"
#include "maps.hpp"
#include "object.hpp"
#include "rect.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"
#include "viewport.hpp"
#include "writeres.hpp"			// for CelHeader


#define UNIT_WIDTH	2
#define UNIT_HEIGHT	3

enum shadowCel_t
{
	SHADOW_POINT=5,
	SHADOW_BOX=7
};

class WorldCursor : public Guy
{
	public:
		bool			fIsMouseDown;
		uint32		tilePosX;
		uint32		tilePosY;
		Rectangle	saveMouseRect;

		virtual void	Setup(grip gContext);
		virtual void	Cycle(void);
		virtual bool	HandleMsg(Message* pMsg);
		virtual void	FindStart();
		virtual void	GetTilePos(uint32 newX, uint32 newY, uint32 *pTileX, uint32 *pTileY, bool fCheckBounds = TRUE);
		virtual void	SetPos(coord newX, coord newY, bool syncVP=FALSE);
		virtual void	SetTile(uint32 tileX, uint32 tileY, bool syncVP=FALSE);
		virtual void	CheckBounds(uint32 *tileX, uint32 *tile);
};


class WorldMap : public Object
{
	public:
		WorldMap();
		~WorldMap();

		grip				gVPCel;

		WorldCursor		worldCursor;

		Guy				shadowCursor; //this cursor shadows the world cursor
		uint32			shadCurPosX;
		uint32			shadCurPosY;
		virtual void	SetShadowAnim(shadowCel_t cel);
		virtual void	MoveShadowCursor(uint32 newTileX, uint32 newTileY);

		Rectangle		worldRect;
		Rectangle		worldMouseRect;
		//uint16  			wArray[WORLD_WIDTH][WORLD_HEIGHT];

		virtual void	Setup(grip gContext, grip gMasterCel);
		virtual void	MoveCursor(uint32 newTileX, uint32 newTileY);
		virtual void	Draw(void);
		virtual void	Draw(uint32 xPos, uint32 yPos, bool fUpdateRect=TRUE, bool fGroupie=FALSE);

		//virtual void	GetMapColors();
		//virtual void	GetUnits();
};


#endif
