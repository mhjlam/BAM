// OPTION4.HPP
//
// OptionsMenu  – routing submenu (Sounds / Music / Game Speed / Health Bars / Done)
// SpeedSlider  – discrete 3-position slider (Normal / Fast / Fastest)
// GameSpeedMenu – game-speed submenu, same shape as NoiseMenu

#ifndef option4_hpp
#define option4_hpp

#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "tigre.hpp"
#include "option.hpp"
#include "game_config.hpp"


// ---- OptionsMenu -----------------------------------------------

class OptionsMenu : public BAM_Room {
public:
	OptionsMenu();
	~OptionsMenu();

	virtual bool HandleMsg(Message* pMsg);
	virtual void Setup();
	virtual void Cleanup();

	BAM_Guy   back;
	grip      gback;
	uint      rNumBack;
	grip      gbackAnim;
	Rectangle rback;

	BAM_Button button[5];   // [0]=Sounds  [1]=Music  [2]=Game Speed  [3]=Health Bars  [4]=Done
	GameConfig cfg;
};


// ---- SpeedSlider -----------------------------------------------

class SpeedSlider : public Slider {
public:
	int16 lastLevel;   // level on last UpdateLabel(); -1 = uninitialised

	// Simpler setup than NoiseSlider: no maxLevel_P (always 2)
	void Setup(grip gCtx, res_t theType, uint theNum, uint theCel,
	           coord theX, coord theY, int thePri,
	           int limitA, int limitB, int activeWidth,
	           int theDir, int16 level_P);

	virtual void SetPos(coord theX, coord theY);
	virtual void Cycle();
	void         UpdateLabel();
};


// ---- GameSpeedMenu ---------------------------------------------

class GameSpeedMenu : public BAM_Room {
public:
	GameSpeedMenu();
	~GameSpeedMenu();

	virtual bool HandleMsg(Message* pMsg);
	virtual void Setup();
	virtual void Cleanup();
	void         SetSpeed(GameSpeed spd);

	BAM_Guy     back;
	grip        gback;
	uint        rNumBack;
	grip        gbackAnim;
	BAM_Guy     slideBack;
	Rectangle   rback;
	SpeedSlider slider;
	Rectangle   rSpeedLabel;   // screen rect covering all three speed labels

	BAM_Button  button[2];     // [0]=Done  [1]=Cancel
	GameSpeed   saveSpeed;
};

#endif // option4_hpp
