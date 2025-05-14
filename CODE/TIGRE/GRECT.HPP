//
// GRECT.HPP
//
// February 25, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// GraphRect class definition.
//
//		A GraphRect is a rectangle specifically designed for use in the
//		GraphicsMgr's Animate function.  It contains information about priority,
//		the owner, and whether there is skip or not.
//
//----[]-------------------------------------------------------------


#ifndef	grect_hpp
#define	grect_hpp

#error	GRECT.HPP is now an obsolete file. Remove it from your includes. (jc)

#include "rect.hpp"


class GraphRect : public Rectangle
{
	public:
					GraphRect();
		void		Construct();

					GraphRect(Rectangle* r, int32 rPri, bool rSkip, bool rIsDrawRect);
		void		Construct(Rectangle* r, int32 rPri, bool rSkip, bool rIsDrawRect);

					GraphRect(coord rx1, coord ry1, coord rx2, coord ry2, grip rowner, int32 rPri, bool rSkip, bool rIsDrawRect);
					GraphRect(GraphRect* r);

		int32		priority;
		bool		hasSkip;
		bool		isDrawRect;
		res_t		resType;
		uint		resNum;
		uint		cel;
		uint		area;

		virtual	void	Display(char* msg);
		uint				AreaBounds(GraphRect* pRect2);
		void				Unite(GraphRect* pRect2);
};


class TRectPool
{
	public:
						TRectPool();

		int			Add(Rectangle* r, int32 rPri, bool rSkip, bool rIsDrawRect);
		GraphRect*	FirstValue();
		GraphRect*	NextValue();
		
		int			UniteRects();
		int			Count();
		void			Release();

	protected:

		enum { MAX_GRECTS = 20 };

		GraphRect	aRects[MAX_GRECTS];
		int			count;
		int			iter;

		int			GetAvailIndex ();
		int			ReduceRects();
};


#endif

