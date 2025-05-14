//
// RECT.CPP
//
// November 22, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "mono.hpp"
#include "rect.hpp"
#include "savebase.hpp"

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)


// Default Constructor - Create a rect with 0's
Rectangle::Rectangle()
{
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
	owner = 0;
	SetName("Rect");
}


// Constructor - Create a rect given the four coords, and an owner
Rectangle::Rectangle(coord rx1, coord ry1, coord rx2, coord ry2, grip rOwner)
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
Rectangle::Rectangle(Rectangle* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2;
	y2 = r->y2;
	owner = r->owner;
	SetName("Rect");
}


// Display information about the rectangle.  Debug only.
void
Rectangle::Display(char* msg)
{
	char	sz[100];

	sprintf(sz, "%s[%d,%d - %d,%d o:%d]\n",
		msg,
		x1,
		y1,
		x2,
		y2,
		owner);

	if (pMono)
	{
		pMono->Out(sz);
	}
}


// Set new coordinates for the rectangle.
void
Rectangle::Set(coord l, coord t, coord r, coord b)
{
	x1 = l;
	y1 = t;
	x2 = r;
	y2 = b;
}


// Determine if a point is contained within a rect.
bool
Rectangle::Contains(coord x, coord y)
{
	if (	(x >= x1) && 
			(y >= y1) &&
			(x <= x2) &&
			(y <= y2))
	{
		return TRUE;
	}
	return FALSE;
}


// Return the width of the rectangle.
uint
Rectangle::Wide()
{
	return x2 - x1 + 1;
}


// Return the height of the rectangle.
uint
Rectangle::High()
{
	return y2 - y1 + 1;
}


// Calculate the area of this rect
uint
Rectangle::CalcArea()
{
	return Wide() * High();
}


// stub for similarity to Object
void
Rectangle::SetName(char* sz)
{
	sz = sz;	// Gets rid of unreferenced warning
}


// copy from the source rect to this.
void
Rectangle::Copy(void* source)
{
	Copy(source, sizeof(*this) - sizeof(void **));	
	//TMovable::Copy(source, sizeof(*this));	
}


// Copy from the source pointer into this.
// Size cannot be determined with this, so it
// must be passed.
void
Rectangle::Copy(void *source, size_t sizeToCopy)
{
	memcpy(this, source, sizeToCopy);
	//TMovable::Copy(source, sizeToCopy);
}


// Clip this rectangle to be within the passed clipRect
//
void
Rectangle::Clip(Rectangle* clipRect)
{
	x1 = AMax(x1, clipRect->x1);
	y1 = AMax(y1, clipRect->y1);
	x2 = AMax(AMin(x2, clipRect->x2), x1);
	y2 = AMax(AMin(y2, clipRect->y2), y1);
}


// Find the intersection with another rect and fill in the intersect rect
//	if requested (not NULL)
//
bool
Rectangle::Intersect(Rectangle* pTargetRect, Rectangle* pIntRect)
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


// Return TRUE if the rectangle's coords are 0,0,0,0
//
bool
Rectangle::IsEmpty()
{
	return ((x1 | y1 | x2 | y2) ? FALSE : TRUE);
}


// Calculate the area of a rect large enough to contain this rect
// and another rect.
uint
Rectangle::AreaBounds(Rectangle* pRect2)
{
	coord	t, l, b, r;

	l = AMin(x1, pRect2->x1);
	t = AMin(y1, pRect2->y1);
	r = AMax(x2, pRect2->x2);
	b = AMax(y2, pRect2->y2);

	return ((b - t + 1) * (r - l + 1));
}


// Enlarge to bound second rect
void
Rectangle::Unite(Rectangle* pRect2)
{
	x1 = AMin(x1, pRect2->x1);
	y1 = AMin(y1, pRect2->y1);
	x2 = AMax(x2, pRect2->x2);
	y2 = AMax(y2, pRect2->y2);
	area = CalcArea();
}


void
Rectangle::Construct()
{
	x1 = y1 = x2 = y2 = 0;
	owner = 0;
}


void
Rectangle::Construct(Rectangle* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2;
	y2 = r->y2;
	owner = r->owner;
	area = CalcArea();
}


bool
Rectangle::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&rectDataStart, 1, (int)&rectDataEnd -
				(int)&rectDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&rectDataStart, 1, (int)&rectDataEnd -
				(int)&rectDataStart, pFile);
			break;
	}
	return(TRUE);
}

