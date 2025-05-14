//
// SCRIMAGE.HPP
//
// November 22, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Scrimage class definition.
//
//		The Scrimage (Screen Image) is any animating screen object.
//
//----[]-------------------------------------------------------------


#ifndef	scrimage_hpp
#define	scrimage_hpp


#include "tigre.hpp"
#include "keylist.hpp"
#include "rect.hpp"
#include "cliprect.hpp"
#include "periodic.hpp"
#include "writeres.hpp"			// for CelHeader


const	uint LAST_CEL = -1;


//---------[ Scrimage ]--------------------------------------------

class Scrimage : public Object
{
	public:
		Scrimage();
		~Scrimage();

		virtual	void	SetRect(coord x1, coord y1, coord x2, coord y2);
		virtual	void	SetRect();
		virtual	void	SetLastRect(coord x1, coord y1, coord x2, coord y2);
		virtual	void	SetClipRect(coord x1, coord y1, coord x2, coord y2);

		virtual	bool	SetState(uint16 flag, bool newState);
		virtual	bool	CheckState(uint16 flag);

		CelHeader		GetCelHeader();
		grip				GetResGrip();
		void				FlushDynamicCel();

		virtual void	SetClut(uint cIdx = 0, uint cSize = 0, grip clutGrip = NULL);
		virtual bool	Save(uint16 state, FILE	*pFile = NULL);

		char			scrimageDataStart;		
		grip			gOwner;

		coord			x;
		coord			y;
		coord			orgX;
		coord			orgY;
		res_t			resType;		// Type of resource attached to us
		uint			resNum;		// Number of resource attached to us
		uint			cel;			// current cel # within resource
		uint			numCels;
		int			scale;		// % scale factor
		Rectangle	rect;
		uint			state;
		int32			priority;
		bool			hasSkip;

		coord			lastX;
		coord			lastY;
		TClipRect		lastRect;
		TClipRect		clipRect;

		grip				gClut;
		uchar*			clut;
		uint				clutIdx;
		uint				clutSize;

		char			scrimageDataEnd;

};



//---------[ Utility ]--------------------------------------------

#define	NO_SUCCESSOR	0

//typedef uint16	util_t;
enum util_t
{
	UT_NONE = 0,
	UT_LINE_TO,
	UT_ANIMATOR,
	UT_RESPONDER,
	UT_ROUTE_TO,
	UT_LAST
};


class Utility : public Periodic
{
	public:
		Utility();
		virtual	~Utility();

		virtual void	Service();
		virtual void	Setup(void* pOwner);
		virtual void	Cleanup();
		virtual bool	Save(uint16 state, FILE	*pFile = NULL);


		char		utilityDataStart;
		util_t	type;

		grip		gOwner;
		grip		gSuccessor;
		char		utilityDataEnd;
};



//---------[ Utilizer ]--------------------------------------------

class Utilizer : public Periodic
{
	public:

		Utilizer(void);
		~Utilizer();

		KeyList		lUtils;

		virtual void	Service();

		virtual void	SetupUtil(Utility* pUtil);
		virtual void	CleanupUtil(grip gUtil);
		virtual void	CleanupUtils(util_t utilType);
		virtual void	CleanupAllUtils();

		virtual void	ScriptAborting();

		virtual void	AbortUtil(grip gUtil);
		virtual void	AbortUtils(util_t utilType);
		virtual void	AbortAllUtils();
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char	utilizerDataStart;
		char	utilizerDataEnd;
};



//---------[ Guy ]----------------------------------------------------

class Guy : public Utilizer
{
	public:
		virtual	void	Hide();
		virtual	void	Show();

		Guy();
		~Guy();

		virtual grip	GetScrimage();

		virtual bool	SetState(uint16 flag, bool newState);
		virtual bool	CheckState(uint16 flag);

		virtual void	Setup();
		virtual void	Cleanup();

		virtual void	SetPos(coord l, coord t);
		virtual void	OffsetPos(coord xOff, coord yOff);
		virtual void	SetRes(res_t type, uint num, uint cel = 1);
		virtual void	SetCel(uint iCel);
		virtual void	SetPri(int32 pri);
		virtual void	SetScale(uint newScale);
		virtual void	SetRect();
		virtual void	SetClipRect(coord x1 = 0, coord y1 = 0, coord x2 = 0, coord y2 = 0);
		virtual void	Flip(uint16 orient, bool doflip = TRUE);
		virtual void	Mirror(uint16 orient, bool domirror = TRUE);
		virtual void	SetClut(uint cIdx = 0, uint cSize = 0, grip clutGrip = NULL);

		virtual void	ChooseWalker(coord xChange, coord yChange);
		virtual void	ChooseStander();
		virtual void	FaceTo(coord x, coord y);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		Scrimage			scrim;

		char				guyDataStart;
		coord				xMoveMax;
		coord				yMoveMax;

		uint				baseAnim;

		bool				fAutoScale;
		bool				fIsActive;
		char				guyDataEnd;

	protected:
};



//---------[ TLineTo : Utility : Periodic ]------------------------------

// Moves actor in a straight line
//
class TLineTo : public Utility
{
	public:
		TLineTo();
		TLineTo(coord x, coord y, grip gSucessor = NULL, ticks_t interval = 1);

		virtual	~TLineTo();

		virtual void	Service();
		virtual void	Setup(void* pActor);

		// used by WalkTo
		virtual void	AdvanceCel(Guy* pGuy);
		virtual void	ChooseAnim(Guy* pGuy, coord xChange, coord yChange);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

	protected:

		char		tLineToDataStart;
		coord		xGoal;
		coord		yGoal;

		coord		xStart;
		coord		yStart;

		int32		fractionSum;
		int32		fraction;

		coord		xDir;
		coord		yDir;

		bool		fMajorIsHorz;
		char		tLineToDataEnd;
};



//---------[ WalkTo : TLineTo : Utility : Periodic ]----------------------

// Moves actor in a straight line
//
class WalkTo : public TLineTo
{
	public:
		WalkTo();
		WalkTo(coord x, coord y, grip gSucessor = NULL, ticks_t interval = 1);

		virtual void	AdvanceCel(Guy* pGuy);
		virtual void	ChooseAnim(Guy* pGuy, coord xChange, coord yChange);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char	walkToDataStart;
		char	walkToDataEnd;
	protected:
};



//---------[ Animator : Utility : Periodic ]------------------------------

//typedef	uint16	animator_t;
enum animator_t
{
	AN_NONE = 0,
	AN_TO_END,
	AN_TO_START,
	AN_FORWARD,
	AN_BACKWARD,
	AN_OSCILLATE,
	AN_RANDOM,
	AN_RANDOM_RANGE,
	AN_LAST
};



// Changes actor's cel
//
class Animator : public Utility
{
	public:
		Animator();
		Animator(animator_t spinType, grip gSucessor = NULL, ticks_t interval = 1);

		virtual	~Animator();

		virtual void	Service();
		virtual void	Setup(void* pActor);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char	animatorDataStart;
		char	animatorDataEnd;
	protected:

		animator_t	anType;
		bool			goingForward;
};




#endif

