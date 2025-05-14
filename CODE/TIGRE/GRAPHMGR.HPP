//
// GRAPHMGR.HPP
//
// November 22, 1993
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// GraphicsMgr class definition.
//
// 	The GraphicsMgr handles all high-level graphics routines.
// 	It works on a minimum rectangle dual list theory, whereby the
//		minimum intersecting rectangles are calculated to minimize
// 	the drawing area, then stuffed onto either an erase list or a draw
// 	list.  The lists are then reconciled via calls to low level graphics
// 	routines.
//
// 	(See doco below)
//
//----[]-------------------------------------------------------------


#ifndef	graphmgr_hpp
#define	graphmgr_hpp

#include "list.hpp"
#include "rect.hpp"
#include "cliprect.hpp"
#include "manager.hpp"
#include "palette.hpp"
#include "scrimage.hpp"
#include "vgabuf.hpp"
#include "tigre.hpp"


enum
{
	GF_DRAW_BIT_MAP,
	GF_FILL_PLANE,
	GF_INIT_VGA,
	GF_MOVE_RECTANGLE,
	GF_RESET_VGA,
	GF_SET_BUFFER_DIMENSIONS,
	GF_SET_INTENSITY,
	GF_SET_PALETTE
};



//----[]-------------------------------------------------------------

#define	MAX_SCALE_Y	640

class ScaleTable
{
	public:
		ScaleTable(void);

		void				Set(coord top, uint16 topScale, coord bottom, uint16 bottomScale);
		void				Fill(uint16 scale);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char				scaleTableDataStart;
		uint16			aScales[MAX_SCALE_Y];
		char				scaleTableDataEnd;
};



//----[ GList ]------------------------------------------------------

// The GList is a go-between that handles pointer-to-grip and grip-to-pointer
// conversion.  This allows the graphics manager to maintain lists of grips
// while still handling them as pointers (saves a sh*tload of editing)

#define		DRAW_LIST_SIZE 60

class GList : public SysList
{
	public:
								GList(uint16 max = DRAW_LIST_SIZE, bool resize = TRUE);
		virtual	uint16 	Add(void* id, int32 key = 0, uint16 posn = L_END, void* target = 0);
		virtual	node*		Find(void* id);
		virtual	bool 		Move(void* id, uint16 posn, void* pTarget = 0);
		virtual	void		Release();
		virtual	bool		Save(uint16 state, FILE *pFile = NULL);

		char		GListDataStart;
		bool		fResortList;	
		char		GListDataEnd;
};

class ScrimList : public GList
{
	public:
		virtual	Scrimage*	PtrAt(uint index);
};



//----[ TRectPool ]---------------------------------------------------

class TRectPool
{
	public:
						TRectPool();

		int			Add(Rectangle* r);
		int			Add(TClipRect* r);
		TClipRect*	FirstValue();
		TClipRect*	NextValue();
		
		int			UniteRects();
		int			Count();
		void			Release();

	protected:

		enum { MAX_GRECTS = 20 };

		TClipRect		aRects[MAX_GRECTS];
		int			count;
		int			iter;

		int			GetAvailIndex ();
		int			ReduceRects();
};



//----[]-------------------------------------------------------------

class GraphicsMgr : public Manager, public Object
{
	public:

		GraphicsMgr(uint mode = 0);
		virtual ~GraphicsMgr();

		ScrimList 	scrimList;

		ScaleTable	scaleTable;

		coord		maxX;
		coord		maxY;
		uint		vidMode;
		uchar		vidPage;
		uint		vgaDriver;		// enum for which driver we need
		uint		vgaBankNoIncrement;
		uint		videoBufferSeg;
		bool		fMinRectMode;

		TPalette	saveRestorePalette;

		virtual	void	DrawCel(uint16 anim, uint16 cel, coord x, coord y, Rectangle* pClipRect = 0);
		virtual	void	Animate();
		virtual	void	AddScrimage(Scrimage* pScrim);
		virtual	void	ChangeScrimage(Scrimage* pScrim);
		virtual	void	DeleteScrimage(Scrimage* pScrim);
		virtual	Rectangle*	GetRect(res_t resType, uint resNum, uint celNum);

		void		UpdateRect(Rectangle *rect);
		void		UpdateRect(TClipRect *rect);

		grip		FindTopGuy(coord x, coord y);
		bool		IntersectRect(TClipRect* pR1, TClipRect* pR2, Rectangle* pR3);
		void		Clip(Rectangle* r1, Rectangle* pR2 = NULL);
		void		Clip(TClipRect* r1, TClipRect* pR2 = NULL);

		bool		Save(uint16 state, FILE *pFile = NULL);

	private:

		uint16		DetectVGA();
		bool			SetVGA(uint16 mode);

		// these objects are used as locals in Animate(), and FindTopGuy().
		// but there's no need to call the constructors every call
		TClipRect		scrimRect;
		Rectangle	grInt;

		TRectPool	drawPool;

		VGABuffer	vbuf;

};

extern GraphicsMgr*	pGraphMgr;


#endif

/*


----[About GraphMgr]-------------------------------------------------

The graphics manager provides two sets of functionality:  First, it
provides access to low level assembly routines to perform basic graphic
functions such as draw line and fill rect.  Second, it provides the
animation logic.

Animation takes place in cycles.  During a cycle, the graphics manager
identifies all Scrimages that have changed (or require changes to their
images), describes these graphic areas and the images of any other
affected Scrimages as draw rectangles and erase rectangles, and posts
the appropriate changes to the screen.  In order to avoid keeping
underbits, erase rectangles are used into which the background bitmap
will be copied.

There are two lists that are used to manage the rectangles.  The draw
list (called pDrawList) contains all the original rectagles generated
by adding, changing, or deleting Scrimages, as well as smaller
rectangles generated by minimizing.  When the minimizing calculations
are completed on this list, the resultant rectangles are placed into
the final draw list.

In general, minimizing takes place in the following 6 steps:

	1. Sort the list by priority (highest -> lowest).
	2. For each rectangle on the list, calculate intersections with
		all higher priority rectangles.
	3. If there is an intersection, split the lower priority rectangle
		into the intersection and remaining rectangles.
	4. If there is an intersection and the covering rectangle does not
		contain skip, eliminate the covered rectangle.
	5. Sort the final list by priority (lowest -> highest).
	6. Expand and blit all rectangles.

Before the process can be applied to the draw list, it must contain all
affected rectangles.  This is accomplished by finding all Scrimages
whose rectangles intersect rectangles on the draw list and adding their
rectangles to the list.  When the minimizing process is performed on
the list, any rectangles left on the draw list are discarded.

Because of the splitting of rectangles, the complexity increases at a
non-linear rate for each object that rectangle A or B overlap. 
Theoretically, there is a break even point beyond which the number of
rectangles to update becomes great enough that simply copying the
entire screen is more efficient.  That break even point will depend on
the final speed of the engine in a real-time situation.



Minimizing Rules
----------------

To ensure that only the minimum number of pixels are updated to the
screen, some simple algorithms can be used to determine which portions
of rectangles need to be updated.  The algorithms apply equally to draw
and erase rectangles.


Terminology:

When describing the minimizing process, the following terminology will
be used:

	rect .......... short for rectangle
	drawRect ...... a rect on the draw list
	eraseRect ..... a rect on the draw list that will contain background
	scrimRect ..... a rect belonging to a non-updating Scrimage
	intRect ....... a rect representing the intersection of two rects
	underRect ..... in the intersection of two rects, the rect with the
							lowest priority
	overRect ...... in the intersection of two rects, the rect with the
							highest priority
	covered rect .. the intRect portion of the underRect
	covering rect . the intRect portion of the overRect
	split rect .... a portion of the underRect after the coveredRect has
							been determined (the underRect is made up of one or
							more splitRects and the coveredRect)
	original rect . a rect generated directly by an add, change, or delete
	derived rect .. a rect generated by the minimize process (will be a
							splitRect or intRect)
	opaque ........ a rect does not contain skip

In the following rule descriptions, rects of higher priority will be
shown with a double border, while those of lower priority will be shown
with a single border.  Also, original draw and erase rects will be
identified by upper case letters, while derived rects will be identifed
by lower case letters.



Rule 1:	The rects do not intersect.

Actions: None.

				ÚÄÄÄÄÄÄÄÄÄÄ¿
				³          ³
				³          ³      ÉÍÍÍÍÍÍÍÍÍ»
				³    A     ³      º         º
				³          ³      º         º
				³          ³      º    B    º
				ÀÄÄÄÄÄÄÄÄÄÄÙ      º         º
                  				º         º
                  				ÈÍÍÍÍÍÍÍÍÍ¼



Rule 2:	The underRect is completely covered by the overRect.

Action:	1. If rect B is opaque, rect A is eliminated.
			1. Else, no action.

				ÉÍÍÍÍÍÍÍÍÍÍÑÍÍÍ»
				º          ³   º
				º          ³   º
				º    A  i  ³ B º
				º          ³   º
				º          ³   º
				ÇÄÄÄÄÄÄÄÄÄÄÙ   º
				º              º
				ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼



Rule 3:	The underRect and overRect intersect.

Action:	1. Rect A is split into splitRects c and d, and intRect i.
			2. Rect A is replaced by rects c and d.
			3. If covering rect i contains skip, covered rect i is added
				to draw list.
			3. Else, covered rect i is eliminated.

				ÚÄÄÄÄÄÄÄÄÄÄ¿
				³A   |  c  ³
				³    ÉÍÍÍÍÍØÍÍÍ»
				³    º     ³   º
				³ d  º  i  ³   º
				³    º     ³ B º
				ÀÄÄÄÄ×ÄÄÄÄÄÙ   º
     			     º         º
     			     ÈÍÍÍÍÍÍÍÍÍ¼



Priority Resolution
-------------------

	The priority of an object may be floating or fixed.  Priorities
are expressed as integers between 0 and 512, exclusive.  A floating
priority indicates that the object's priority will be the same as
its y coordinate.

	Rectangles are drawn in the order they appear on the list, so that
the first rectangle on the list will always have the appearance of
lowest priority.



----[Examples]-------------------------------------------------------

Example 1: A not updating, B updating, C updating w/skip

                  ÚÄÄÄÄÄÄÄÄÄÄ¿
                  ³     i    ³
            ÚÄÄÄÄÄÅÄÄÄÄ¿  B  ³
            ³     ³  e ³     ³
            ³   ÚÄÅÄÄÄÄÅÄÄÄ¿-³
            ³ _ ³ ³ f  ³ g ³h³
            ³ A ³ ÀÄÄÄÄÅÄÄÄÅÄÙ
            ³   ³  d   ³   ³
            ³   ³      ³ Cs³
            ÀÄÄÄÅÄÄÄÄÄÄÙ   ³
                ÀÄÄÄÄÄÄÄÄÄÄÙ

		1. Move C to final draw list
		2. Split B into (f+g), h, and i
		3. Covering rect (f+g) contains skip, so move (f+g), h, and i
			to final draw list


Example 2: A not updating, B updating w/skip, C updating

                  ÚÄÄÄÄÄÄÄÄÄÄ¿
                  ³     i    ³
            ÚÄÄÄÄÄÅÄÄÄÄ¿  Bs ³
            ³     ³  e ³     ³
            ³   ÚÄÅÄÄÄÄÅÄÄÄ¿-³
            ³ _ ³ ³ f  ³ g ³h³
            ³ A ³ ÀÄÄÄÄÅÄÄÄÅÄÙ
            ³   ³  d   ³   ³
            ³   ³      ³ C ³
            ÀÄÄÄÅÄÄÄÄÄÄÙ   ³
                ÀÄÄÄÄÄÄÄÄÄÄÙ

		1. Move C to final draw list
		2. Split B into (f+g), h, and i
		3. Covering rect (f+g) is opaque, so move h and i to final draw list


Example 3: A updating, B updating w/skip, C updating

                  ÚÄÄÄÄÄÄÄÄÄÄ¿
                  ³     i    ³
            ÚÄÄÄÄÄÅÄÄÄÄ¿  Bs ³
            ³   |j³  e ³     ³
            ³   ÚÄÅÄÄÄÄÅÄÄÄ¿-³
            ³   ³ ³ f  ³ g ³h³
            ³ A ³ ÀÄÄÄÄÅÄÄÄÅÄÙ
            ³ l ³k| d  ³   ³
            ³   ³ |    ³ C ³
            ÀÄÄÄÅÄ+ÄÄÄÄÙ   ³
                ÀÄÄÄÄÄÄÄÄÄÄÙ

		1. Move C to final draw list
		2. Split B into (f+g), h, and i
		3. Covering rect (f+g) is opaque, so move h to final draw list,
			leave i on draw list
		4. Split A into (d+f+k), (j+e), and l
		5. Covering rect (d+f+k) is opaque, so move l to final draw list,
			leave (j+e) on draw list
		6. Split (j+e) into j and e
		7. Covering rect e contains skip, so move j and e to final draw list


----[]-------------------------------------------------------------


*/

