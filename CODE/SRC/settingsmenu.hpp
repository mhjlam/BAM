// SETTINGSMENU.HPP
//
// SettingsMenu – toggles for gameplay settings (health bars, fast collect)

#ifndef settingsmenu_hpp
#define settingsmenu_hpp

#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "tigre.hpp"
#include "game_config.hpp"


class SettingsMenu : public BAM_Room {
public:
	SettingsMenu();
	~SettingsMenu();

	virtual bool HandleMsg(Message* pMsg);
	virtual void Setup();
	virtual void Cleanup();

	BAM_Guy   back;
	grip      gback;
	uint      rNumBack;
	grip      gbackAnim;
	Rectangle rback;

	BAM_Button button[2];   // [0]=Fast Collect  [1]=Done
	GameConfig cfg;
};

#endif // settingsmenu_hpp
