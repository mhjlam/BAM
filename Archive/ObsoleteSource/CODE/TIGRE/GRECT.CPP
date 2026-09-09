//
// GRECT.CPP
//
// February 25, 1994
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#include "api.hpp"
#include "apimem.hpp"
#include "graphmgr.hpp"
#include "grect.hpp"
#include "mono.hpp"
#include "scrimage.hpp"


#error	GRECT.CPP is now an obsolete file. Remove it from your makefile. (jc)


//----[GraphRect]----------------------------------------------------

// GraphRect Constructor
//
GraphRect::GraphRect()
{
	Construct();
	SetName("GraphRect");
}


void
GraphRect::Construct()
{
	x1 = y1 = x2 = y2 = 0;
	owner = 0;
	priority = 0;
	hasSkip = FALSE;
	isDrawRect = FALSE;
	area = 0;
}


// GraphRect Constructor - Create a GraphRect from raw data
GraphRect::GraphRect(coord rx1, coord ry1, coord rx2, coord ry2, grip rowner, int32 pri, bool skip, bool draw)
{
	x1 = rx1;
	y1 = ry1;
	x2 = rx2;
	y2 = ry2;
	owner = rowner;
	priority = pri;
	hasSkip = skip;
	isDrawRect = draw;

	if (owner)
	{
		Scrimage* s = ADerefAs(Scrimage, owner);
		resType = s->resType;
		resNum = s->resNum;
		cel = s->cel;
	}
	else
	{
		resType = RES_CEL;	// 0
		resNum = 0;
		cel = 0;
	}
	area = CalcArea();
	SetName("GraphRect");
}


// GraphRect Constructor - Create a GraphRect from another rect
//
GraphRect::GraphRect(Rectangle* r, int32 rPri, bool rSkip, bool rIsDrawRect)
{
	Construct (r, rPri, rSkip, rIsDrawRect);
	SetName("GraphRect");
}

void
GraphRect::Construct(Rectangle* r, int32 rPri, bool rSkip, bool rIsDrawRect)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2;
	y2 = r->y2;
	owner = r->owner;
	priority = rPri;
	hasSkip = rSkip;
	isDrawRect = rIsDrawRect;

	if (owner) 
	{
		Scrimage* s = ADerefAs(Scrimage, owner);
		resType = s->resType;
		resNum = s->resNum;
		cel = s->cel;
	} 
	else
	{
		resType = RES_CEL;	// 0
		resNum = 0;
		cel = 0;
	}
	area = CalcArea();
}


// GraphRect Constructor - Create a GraphRect from another GraphRect
GraphRect::GraphRect(GraphRect* r)
{
	x1 = r->x1;
	y1 = r->y1;
	x2 = r->x2;
	y2 = r->y2;
	owner = r->owner;
	priority = r->priority;
	hasSkip = r->hasSkip;
	isDrawRect = r->isDrawRect;
	resType = r->resType;
	resNum = r->resNum;
	cel = r->cel;
	area = r->area;
	SetName("GraphRect");
}




// Display information about the rectangle.  Debug only.
void
GraphRect::Display(char* msg)
{
	char	buffer[80];
	sprintf(buffer, "%s[%d,%d - %d,%d o:%d p:%d s:%d d:%d]\n",
		msg,
		x1,
		y1,
		x2,
		y2,
		owner,
		priority,
		hasSkip,
		isDrawRect);
	pMono->Out(buffer);
}


// Calculate the area of a rect large enough to contain this rect
// and another rect.
uint
GraphRect::AreaBounds(GraphRect* pRect2)
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
GraphRect::Unite(GraphRect* pRect2)
{
	x1 = AMin(x1, pRect2->x1);
	y1 = AMin(y1, pRect2->y1);
	x2 = AMax(x2, pRect2->x2);
	y2 = AMax(y2, pRect2->y2);
	area = CalcArea();
}





//----[TRectPool]----------------------------------------------------


TRectPool::TRectPool()
{
	Release();
}


void
TRectPool::Release()
{
	int	i;

	for (i=0; i<MAX_GRECTS; i++)
	{
		aRects[i].Construct();
	}

	count = 0;
}

int 
TRectPool::Add(Rectangle* r, int32 rPri, bool rSkip, bool rIsDrawRect)
{
	int	i;
	
	i = GetAvailIndex ();

	aRects[i].Construct(r, rPri, rSkip, rIsDrawRect);
	pGraphMgr->Clip(&aRects[i]);

	count++;

	// if overflowing, reduce rects
	if (count == MAX_GRECTS)
	{
		ReduceRects();
	}

	return count;
}


int 
TRectPool::Count()
{
	return count;
}


GraphRect*
TRectPool::FirstValue()
{
	iter = 0;

	return NextValue();
}


GraphRect*
TRectPool::NextValue()
{
	int	i;

	for (i=iter; i<MAX_GRECTS; i++)
	{
		// check if is in used
		if (aRects[i].y2)
		{
			iter = i+1;
			return &aRects[i];
		}
	}

	// if none in use
	iter = MAX_GRECTS;
	return NULL;
}


int
TRectPool::GetAvailIndex ()
{
	int	i;

	for (i=0; i<MAX_GRECTS; i++)
	{
		if (aRects[i].y2 == 0)
		{
			return i;
		}
	}
	assert (0);
	return 0;
}


// Unite all graph rects whose bounding rect's area is smaller than
//	150% of the sum of the areas of the parts
//
int
TRectPool::UniteRects()
{
	int			i;
	int			j;
	int			bounds;
	long			sumArea;

	if (count < 2)
	{
		// takes two to tangle
		return count;
	}

	// nested loop to compare rects
	//
	for (i=0; i<MAX_GRECTS-1; i++)
	{
		// check if rect is in use
		if (aRects[i].y2 == 0)	continue;

		for (j=i+1; j<MAX_GRECTS; j++)
		{
			// check if rect is in use
			if (aRects[j].y2 == 0)	continue;

			// If the area of the bounding rect is < 150% of the sum of the
			// rects' areas, unite the rects. That is, if there is a lot
			// of overlap, unite the rects.

			bounds = aRects[i].AreaBounds(&aRects[j]);
			sumArea = aRects[i].area + aRects[j].area;

			if ( (bounds * 2) <= (sumArea * 3))
			{
				// unite i and j
				aRects[i].Unite(&aRects[j]);

				// empty j
				aRects[j].Construct();
				count--;

				if (count < 2)
				{
					// take 2 to tangle
					return count;
				}

				// break this loop...
				j = MAX_GRECTS+1;

				// ...and repeat i with the new larger rect.
				i--;
			}
		}
	}

	return count;
}


// Before the pool overflows ReduceRects() guarantees a reduction
// in the number of rects.
//
int
TRectPool::ReduceRects()
{
	// try uniting first
	if (UniteRects() == MAX_GRECTS)
	{
		// ok, that didn't work. Lets find the two biggest, merge them.
		
		// give programmers warning that TRectPool should be bigger
		if (pMono) pMono->Out("TRectPool::ReduceRects");

		int 	i;
		int 	iLargeArea1 = 1;
		int 	iLargeArea2 = 0;
		long	largeArea1 = 0;
		long	largeArea2 = 0;

		// find two largest
		//
		for (i=0; i<MAX_GRECTS; i++)
		{
			// check if rect is in use
			if (aRects[i].y2 == 0)	continue;
 			
			if (aRects[i].area > largeArea1)
			{
				iLargeArea2 = iLargeArea1;
				iLargeArea1 = i;

				largeArea1 = aRects[iLargeArea1].area;
				largeArea2 = aRects[iLargeArea2].area;
			}
		}

		// unite two largest
		aRects[iLargeArea1].Unite(&aRects[iLargeArea2]);

		// empty #2
		aRects[iLargeArea2].Construct();
		count--;

		// the two more call to Add() will bring us back here,
		// but UniteRects will be more effective.
	}
	return count;
}


				


