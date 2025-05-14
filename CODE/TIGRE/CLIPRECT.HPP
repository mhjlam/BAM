//
// CLIPRECT.HPP
//
// September 20, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
// Clip Rectangle class definition.
//
//		A Rectangle is a portion of the screen described by two points
//		that has a priority and an owner.  ClipRect's differ from 
//    the other rects in Tigre in that they are exclusive
//    (upper right inside, lower right outside).
//
//----[]-------------------------------------------------------------


#ifndef	cliprect_hpp
#define	cliprect_hpp

#include "tigre.hpp"
#include "rect.hpp"


class TClipRect : public Rectangle 
{
	public:
		TClipRect();
		TClipRect(coord x1, coord y1, coord x2, coord y2, grip owner = 0);
		TClipRect(Rectangle* r);
		TClipRect(TClipRect* r);

		virtual	void		Construct();
		virtual	void		Construct(Rectangle*	pr);
		virtual	void		Construct(TClipRect*	pr);

		virtual	void		Set(coord l, coord t, coord r, coord b);
		virtual	bool		Contains(coord x, coord y);
		virtual	uint		Wide();
		virtual	uint		High();

		virtual	void		Copy(void* source);
		virtual	void		Copy(void *source, size_t sizeToCopy);

		virtual	void		Clip(Rectangle* pClipRect);
		virtual	void		Clip(TClipRect* pClipRect);
		virtual	bool		Intersect(Rectangle* pTargetRect, Rectangle* pIntRect);
		virtual	bool		Intersect(TClipRect* pTargetRect, TClipRect* pIntRect);
		virtual	bool		IsEmpty();

		virtual	uint		AreaBounds(Rectangle* pRect2);
		virtual	uint		AreaBounds(TClipRect* pRect2);
		virtual	void		Unite(Rectangle* pRect2);
		virtual	void		Unite(TClipRect* pRect2);
};


#endif


