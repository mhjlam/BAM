//	BAMGUY.hpp
//
//	Copyright 1994, Tachyon, Inc.
//
// Bam Guy Class
//
// This is the base class for all game "rooms"
//

#include "api.hpp"
#include "apimem.hpp"
#include "bamguy.hpp"
#include "context.hpp"
#include "mono.hpp"
#include "mouse.hpp"

void
BAM_Guy::Setup(void)
{
	Guy::Setup();
}

// Set object's Message mask, subscribe to Message stream
void
BAM_Guy::Setup(uint16 msgFlags)
{
	Context*	pContext;

	// error check
	if (!gContext)	APanic ("No context assigned to Object");

	fIsActive = TRUE;

	pContext = ADerefAs(Context, gContext);
	pContext->AddObject(gSelf, msgFlags);
}



//-----------------------------------------------------------------
//	Slider
//-----------------------------------------------------------------

void
Slider::Setup(grip gContext, res_t theType, uint theNum, uint theCel, 
					coord theX, coord theY,
				  int thePri, int limitA, int limitB, int activeWidth, int theDir,
				  int16 level_P, int16 maxLevel_P)
{
	int	tempWidth;
	int	curPosn;
	int	percentage;

	maxLevel = maxLevel_P;
	level = level_P;

	msgMask =  E_MOUSE_DOWN | E_MOUSE_UP;
	fIsMouseDown = FALSE;

	SetContext(gContext);
	SetRes(theType, theNum, theCel);
	SetPri(thePri);
	BAM_Guy::Setup(CT_ROST | CT_MSGS); //No service yet
	orientation = theDir;

	if(orientation == HORIZONTAL)
	{
		slideRect.Set(limitA,theY,limitB,theY);
		containRect.Set(limitA-5,theY-activeWidth,limitB+5,theY+activeWidth);
	}
	else
	{
		slideRect.Set(theX,limitA,theX,limitB);
		containRect.Set(theX-activeWidth,limitA-5,theX+activeWidth,limitB+5);
	}

	topOrLeft = limitA;
	rightOrBottom = limitB;

	if (maxLevel != 0)
	{
		tempWidth = rightOrBottom - topOrLeft;
		percentage = level * 100 / maxLevel;
		curPosn = percentage * tempWidth / 100;
	}
	else
	{
		curPosn = 0;
	}
	if (orientation == HORIZONTAL)
		BAM_Guy::SetPos(curPosn + topOrLeft, theY);
	else
		BAM_Guy::SetPos(theX, rightOrBottom - curPosn); // where bott=0 and top=max
}


void
Slider::Cycle(void)
{
	coord theX = pMouse->GetX();
	coord theY = pMouse->GetY();

	SetPos(theX,theY);
}

void
Slider::SetPos(coord theX, coord theY)
{
	int	tempWidth;
	int	curPosn;
	int	percentage;

	if (orientation == HORIZONTAL)
	{
		// if close to either end of the slider then snap to end
		if (theX > rightOrBottom - 3)
			scrim.x = rightOrBottom;
		else if (theX < topOrLeft + 3)
			scrim.x = topOrLeft;
		else
			scrim.x = theX;
	}
	else
	{
		if (theY < topOrLeft + 3)
			scrim.y = topOrLeft;
		else if (theY > rightOrBottom -3)
			scrim.y = rightOrBottom;
		else
			scrim.y = theY;
	}
	scrim.SetRect();
	SetState(S_CHANGED, TRUE);

	if (maxLevel == 0)
	{
		pMono->Out("\nNO MAX LEVEL SET FOR THIS SLIDER");
		level = 0;
	}
	else
	{
		tempWidth = rightOrBottom - topOrLeft;
		if (orientation == HORIZONTAL)
			curPosn = scrim.x - topOrLeft;
		else
			curPosn = rightOrBottom - scrim.y;
		percentage = curPosn * 100 / tempWidth;
		level = (int16)(percentage * maxLevel / 100);
	}
}

bool
Slider::HandleMsg(Message* pMsg)
{

 	if (containRect.Contains(pMsg->event.x, pMsg->event.y))
 	{
		switch (pMsg->type)
		{
			case MSG_EVENT:
				switch (pMsg->event.type)
				{
					case E_MOUSE_DOWN:

						if (!fIsMouseDown)
						{
							fIsMouseDown = TRUE;

							saveMouseRect.Copy(&(pMouse->mouseLimits));

							// Lock boundaries on mouse
							pMouse->SetLimits(&(slideRect)); //calls bounds check also

							// Turn on service to make guy draggable
							if (gContext)
								(ADerefAs(Context, gContext))->AddObject(gSelf, CT_SERV);
						}

						return TRUE;
						break;

					case E_MOUSE_UP:
						fIsMouseDown = FALSE;

						// Turn off service to make guy undraggable
						if (gContext)
							(ADerefAs(Context, gContext))->CutObject(gSelf, CT_SERV);

						// Unlock boundaries on mouse
						pMouse->SetLimits(&saveMouseRect);

						SetPos(pMsg->event.x, pMsg->event.y);

						return TRUE;
						break;
				}
				break;
		}
	}

	return FALSE;
}
