//
// RECT.HPP
//
// November 22, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Rectangle class definition.
//
//		A Rectangle is a portion of the screen described by two points
//		that has a priority and an owner.
//
//----[]-------------------------------------------------------------


#ifndef	rect_hpp
#define	rect_hpp

#include "tigre.hpp"


class Rectangle
{
	public:
		Rectangle();
		Rectangle(coord x1, coord y1, coord x2, coord y2, grip owner = 0);
		Rectangle(Rectangle* r);

		virtual void	Construct();
		virtual void	Construct(Rectangle*	pr);

		virtual void	Display(char* msg);
		virtual void	Set(coord l, coord t, coord r, coord b);
		virtual bool	Contains(coord x, coord y);
		virtual uint	Wide();
		virtual uint	High();

		virtual uint	CalcArea();

		void	  			SetName(char* sz);
		virtual void	Copy(void* source);
		virtual void	Copy(void *source, size_t sizeToCopy);

		virtual void	Clip(Rectangle* pClipRect);
		virtual bool	Intersect(Rectangle* pTargetRect, Rectangle* pIntRect);
		virtual bool	IsEmpty();
				  
		virtual uint	AreaBounds(Rectangle* pRect2);
		virtual void	Unite(Rectangle* pRect2);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char		rectDataStart;
		ClassID	classID;
		coord		x1;
		coord		y1;
		coord		x2;
		coord		y2;
		uint		area;
		grip		owner;
		char		rectDataEnd;
};


#endif


