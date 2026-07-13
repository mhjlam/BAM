//	BAMGUY.hpp
//
//	Copyright 1994, Tachyon, Inc.
//
// Bam Guy Class
//
// This is the base class for all game "rooms"
//

#ifndef bamguy_hpp
#define bamguy_hpp

#include "tigre.hpp"
#include "scrimage.hpp"
#include	"soundmgr.hpp"

class BAM_Guy : public Guy
{
	public:
		virtual void	Setup(void);
		virtual void	Setup(uint16 msgFlags);
};


enum {
	HORIZONTAL=0,
	VERTICAL
};

// Pure class (not skewed to any specific purpose)
class Slider : public BAM_Guy
{
	public:
		bool			fIsMouseDown;
		Rectangle	saveMouseRect;
		Rectangle	slideRect; 	// is one pixel wide -used for mouse limits when slider is selected
		Rectangle	containRect;// covers entire slide area -used for detecting mouse clicks

		int		orientation;
		int		topOrLeft;
		int		rightOrBottom;
		int16		level;			
		int16		maxLevel;

		virtual void	Setup(grip gContext, res_t theType, uint theNum,
									uint theCel, coord theX, coord theY,
								   int thePri, int limitA, int limitB, int activeWidth, int theDir, int16 level_P,int16 maxLevel_P);
		virtual bool	HandleMsg(Message* pMsg);
		virtual void	Cycle(void);
		virtual void	SetPos(coord theX, coord theY);
};

#endif
