// OPTION2.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
//
// A popup screen to handle all game options.
//
// 11/16/94
//

#ifndef option2_hpp
#define option2_hpp

#include "bamroom.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "tigre.hpp"
#include "text.hpp"
#include "option.hpp"

enum {
	MIDI=0,
	DIGI
};

class NoiseSlider : public Slider
{
	public:
		bool		fOff; //for updating hilite text
		bool		fMax;

		//functions are overloaded to handle text hilighting of OFF and MAX
		virtual void	Setup(grip gContext, res_t theType, uint theNum, uint theCel, coord theX, coord theY,
								   int thePri, int limitA, int limitB, int activeWidth,
									int theDir, int16 level_P,int16 maxLevel_P);
		virtual void	Cycle(void); // have slider real time adjust sound
		virtual void	EndCheck(); 
};

// For Sound or Music
class NoiseMenu : public BAM_Room
{
	public:

	NoiseMenu(void);
	~NoiseMenu();

	virtual bool		HandleMsg(Message* pMsg);
 	virtual void		Setup(option_t	sndOrMus_P);
	//virtual void		Cycle();
	virtual void		Cleanup();
	virtual void		SetSound(int16 soundType,int16 level);

	option_t			sndOrMus;

	SquibRes			sqbOption;
	char 				*pTxt,*pOffTxt,*pMaxTxt;

	BAM_Guy			back;
	grip				gback;
	uint				rNumBack;
	grip				gbackAnim;
	BAM_Guy			slideBack;
	Rectangle		rback;
	NoiseSlider		slider;
	Rectangle		rOff; // for the text 'OFF' to be easily updated
	Rectangle		rMax;

	BAM_Button		button[2];

	//int				level;
	int16				maxLevel;
	int16				saveLevel;
};

#endif // option2_hpp

