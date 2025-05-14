//
// TClipRect.CPP
//
// September 20, 1995
// (c) Copyright 1995, Tachyon, Inc.  All rights reserved.
//
// Clip Rectangle class definition.
//
//		A Rectangle is a portion of the screen described by two points
//		that has a priority and an owner.  TClipRect's differ from 
//    the other rects in Tigre in that they are exclusive
//    (upper right inside, lower right outside).
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "mono.hpp"
#include "cliprect.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


// Default Constructor - Create a rect with 0's
TClipRect::TClipRect()
{
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
	owner = 0;
	SetName("Rect");
}


// Constructor - Create a rect given the four coords, and an owner
TClipRect::TClipRect(coord rx1, coord ry1, coord rx2, coord ry2, grip rOwner)
{
	x1 = rx1;
	y1 = ry1;
	x2 = rx2;
	y2 = ry2;
	owner = rOwner;
	SetName("Rect");
}


// Constructor - Create a rectangle with the same information
// as the rectangle passed
TClipRect::TClipRect(Rectangle* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2 + 1;
	y2 = r->y2 + 1;
	owner = r->owner;
	SetName("Rect");
}

// Constructor - Create a rectangle with the same information
// as the rectangle passed
TClipRect::TClipRect(TClipRect* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2;
	y2 = r->y2;
	owner = r->owner;
	SetName("Rect");
}


// Set new coordinates for the rectangle.
void
TClipRect::Set(coord l, coord t, coord r, coord b)
{
	x1 = l;
	y1 = t;
	x2 = r;
	y2 = b;
}


// Determine if a point is contained within a rect.
bool
TClipRect::Contains(coord x, coord y)
{
	if (	(x >= x1) && 
			(y >= y1) &&
			(x < x2)  &&
			(y < y2))
	{
		return TRUE;
	}
	return FALSE;
}


// Return the width of the rectangle.
uint
TClipRect::Wide()
{
	return x2 - x1;
}


// Return the height of the rectangle.
uint
TClipRect::High()
{
	return y2 - y1;
}


// copy from the source rect to this.
void
TClipRect::Copy(void* source)
{
	Copy(source, sizeof(*this));	
	//TMovable::Copy(source, sizeof(*this));	
}


// Copy from the source pointer into this.
// Size cannot be determined with this, so it
// must be passed.
void
TClipRect::Copy(void *source, size_t sizeToCopy)
{
	memcpy(this, source, sizeToCopy);
	//TMovable::Copy(source, sizeToCopy);
}


// Clip this rectangle to be within the passed TClipRect
//
void
TClipRect::Clip(Rectangle* pClipRect)
{
	x1 = AMax(x1, pClipRect->x1);
	y1 = AMax(y1, pClipRect->y1);
	x2 = AMax(AMin(x2, pClipRect->x2 + 1), x1);
	y2 = AMax(AMin(y2, pClipRect->y2 + 1), y1);
}

// Clip this rectangle to be within the passed TClipRect
//
void
TClipRect::Clip(TClipRect* pClipRect)
{
	x1 = AMax(x1, pClipRect->x1);
	y1 = AMax(y1, pClipRect->y1);
	x2 = AMax(AMin(x2, pClipRect->x2), x1);
	y2 = AMax(AMin(y2, pClipRect->y2), y1);
}


// Find the intersection with another rect and fill in the intersect rect
//	if requested (not NULL)
//
bool
TClipRect::Intersect(Rectangle* pTargetRect, Rectangle* pIntRect)
{
	coord l;
	coord t;
	coord r;
	coord b;
	bool	ret = FALSE;

	if (IsEmpty() == FALSE)
	{
		// we have a rect to check
		l = AMax(x1, pTargetRect->x1);
		t = AMin(x2 - 1, pTargetRect->x2);
		r = AMax(y1, pTargetRect->y1);
		b = AMin(y2 - 1, pTargetRect->y2);

		if (r >= l && b >= t)
		{
			if (pIntRect)
			{
				pIntRect->x1 = l;
				pIntRect->y1 = t;
				pIntRect->x2 = r;
				pIntRect->y2 = b;
			}
			ret = TRUE;
		}
	}

	return ret;
}

// Find the intersection with another rect and fill in the intersect rect
//	if requested (not NULL)
//
bool
TClipRect::Intersect(TClipRect* pTargetRect, TClipRect* pIntRect)
{
	coord l = AMax(x1, pTargetRect->x1);
	coord t = AMin(x2, pTargetRect->x2);
	coord r = AMax(y1, pTargetRect->y1);
	coord b = AMin(y2, pTargetRect->y2);
	bool	ret = FALSE;

	if (r >= l && b >= t)
	{
		if (pIntRect)
		{
			pIntRect->x1 = l;
			pIntRect->y1 = t;
			pIntRect->x2 = r;
			pIntRect->y2 = b;
		}
		ret = TRUE;
	}
	return ret;
}


// Return TRUE if the rectangle's is empty or complete clipped.
//
bool
TClipRect::IsEmpty()
{
	return ((x1 == x2 || y1 == y2) ? TRUE : FALSE);
}


// Calculate the area of a rect large enough to contain this rect
// and another rect.
uint
TClipRect::AreaBounds(Rectangle* pRect2)
{
	coord	t, l, b, r;

	l = AMin(x1, pRect2->x1);
	t = AMin(y1, pRect2->y1);
	r = AMax(x2, pRect2->x2 + 1);
	b = AMax(y2, pRect2->y2 + 1);

	return ((b - t) * (r - l));
}


// Calculate the area of a rect large enough to contain this rect
// and another rect.
uint
TClipRect::AreaBounds(TClipRect* pRect2)
{
	coord	t, l, b, r;

	l = AMin(x1, pRect2->x1);
	t = AMin(y1, pRect2->y1);
	r = AMax(x2, pRect2->x2);
	b = AMax(y2, pRect2->y2);

	return ((b - t) * (r - l));
}


// Enlarge to bound second rect
void
TClipRect::Unite(Rectangle* pRect2)
{
	x1 = AMin(x1, pRect2->x1);
	y1 = AMin(y1, pRect2->y1);
	x2 = AMax(x2, pRect2->x2 + 1);
	y2 = AMax(y2, pRect2->y2 + 1);
	area = CalcArea();
}


// Enlarge to bound second rect
void
TClipRect::Unite(TClipRect* pRect2)
{
	x1 = AMin(x1, pRect2->x1);
	y1 = AMin(y1, pRect2->y1);
	x2 = AMax(x2, pRect2->x2);
	y2 = AMax(y2, pRect2->y2);
	area = CalcArea();
}


void
TClipRect::Construct()
{
	x1 = y1 = x2 = y2 = 0;
	owner = 0;
}


void
TClipRect::Construct(Rectangle* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2 + 1;
	y2 = r->y2 + 1;
	owner = r->owner;
	area = CalcArea();
}


void
TClipRect::Construct(TClipRect* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2;
	y2 = r->y2;
	owner = r->owner;
	area = CalcArea();
}


