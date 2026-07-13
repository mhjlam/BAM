// SETTINGSMENU.CPP
//
// SettingsMenu – toggles for gameplay settings (fast collect)
// Note: health bars toggle has been moved to the in-game OptionsMenu (option4.cpp)

#include "settingsmenu.hpp"
#include "mainmenu.hpp"

#include "apifont.hpp"
#include "apires.hpp"
#include "apigraph.hpp"
#include "context.hpp"
#include "scrimage.hpp"
#include "api.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamfuncs.hpp"
#include "bam_dg.hpp"
#include "rect.hpp"
#include "tigre.hpp"
#include "eventmgr.hpp"

#include <string.h>

// Same window offset as OptionsMenu/GameSpeedMenu – reuses the same popup frame
#define SETTINGS_WOFF_X  89
#define SETTINGS_WOFF_Y  92

// Priority layers: above all existing option menus
#define SETTINGS_BACK_PRI   20050
#define SETTINGS_BTN_PRI    20051


SettingsMenu::SettingsMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
}

SettingsMenu::~SettingsMenu()
{
}

void
SettingsMenu::Setup()
{
	if (bGlobal.storyLine == NETGAME)
		return;

	BAM_Guy    *pGuy;
	BAM_Button *pButton;
	uchar      *pback;
	CelHeader  *pbackAnimCH, *pbackCH;
	int         x, buttonY;

	mode = M_MODAL;

	gbackAnim = ALoad(RES_ANIM, 40);
	pbackAnimCH = (CelHeader*)AGetResData(gbackAnim);
	rback.Set(SETTINGS_WOFF_X, SETTINGS_WOFF_Y,
	          SETTINGS_WOFF_X - 1 + pbackAnimCH->width,
	          SETTINGS_WOFF_Y - 1 + pbackAnimCH->height);

	gback = ACreateCel(&rNumBack, 0, 0,
	                   pbackAnimCH->width, pbackAnimCH->height,
	                   CI_BLACK, SETTINGS_BACK_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	CopyCel(pbackCH, 0, 0, RES_ANIM, 40, 1, false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(SETTINGS_WOFF_X, SETTINGS_WOFF_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SETTINGS_BACK_PRI);

	// --- Layout: 2 buttons split 1+1 ---
	// Group 1: Fast Collect
	// Group 2: Done
	const int buttonNum = 2;
	int topMargin = SETTINGS_WOFF_Y + (210 - (18 + 17 * buttonNum)) / 2;
	int capPosY[4];
	capPosY[0] = topMargin;
	capPosY[1] = capPosY[0] + 3 + 17 * 1;
	capPosY[2] = capPosY[1] + 3 + 6;
	capPosY[3] = capPosY[2] + 3 + 17 * 1;

	CopyCel(pbackCH, 112 - SETTINGS_WOFF_X, capPosY[0] - SETTINGS_WOFF_Y, RES_ANIM, 40, 2, false);
	CopyCel(pbackCH, 112 - SETTINGS_WOFF_X, capPosY[1] - SETTINGS_WOFF_Y, RES_ANIM, 40, 3, false);
	CopyCel(pbackCH, 112 - SETTINGS_WOFF_X, capPosY[2] - SETTINGS_WOFF_Y, RES_ANIM, 40, 2, false);
	CopyCel(pbackCH, 112 - SETTINGS_WOFF_X, capPosY[3] - SETTINGS_WOFF_Y, RES_ANIM, 40, 3, false);

	static const char* labels[2] = { "FAST COLLECT", "DONE" };

	cfg = LoadGameConfig();

	for (x = 0; x < buttonNum; x++) {
		if (x == buttonNum - 1)
			buttonY = topMargin + 15 + x * 17;
		else
			buttonY = topMargin + 3 + x * 17;

		pButton = &button[x];
		if (x % 2 == 0)
			pButton->Create(112, buttonY, SETTINGS_BTN_PRI, RES_ANIM, 42, 1,
			                gSelf, SETTINGS_WOFF_X, SETTINGS_WOFF_Y);
		else
			pButton->Create(112, buttonY, SETTINGS_BTN_PRI, RES_ANIM, 44, 1,
			                gSelf, SETTINGS_WOFF_X, SETTINGS_WOFF_Y);

		pButton->SetOwnerCel(rNumBack);
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetCelText(1, (char*)labels[x]);
		pButton->SetColors(1, 93, 90);
		pButton->SetCelText(2, (char*)labels[x]);
		pButton->SetColors(2, 155, 142);

		if (x == 0) {
			// Fast Collect toggle
			pButton->fIsToggle = true;
			pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
			pButton->Select(cfg.fast_collect);
		} else {
			// Done button
			pButton->fIsToggle = false;
			pButton->SetupReplies(REPLY_DESELECTED);
			pButton->Select(false);
		}
	}

	Activate(true);
	pContextMgr->lContexts.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);
	gCurControl = grip{};
}

bool
SettingsMenu::HandleMsg(Message* pMsg)
{
	if (BAM_Room::HandleMsg(pMsg))
		return true;

	switch (pMsg->type) {
		case MSG_NOTICE:
			if (pMsg->notice.type == N_CONTROL_REPLY) {
				uint16 reply = (uint16)(uintptr_t)pMsg->notice.param;

				if (pMsg->notice.gSource == button[0].gSelf) {
					cfg.fast_collect = (reply == REPLY_ACTIVATED);
					bGlobal.fFastCollect = cfg.fast_collect;
					SaveGameConfig(cfg);
					return true;
				}
				if (pMsg->notice.gSource == button[1].gSelf) {
					Cleanup();
					return true;
				}
			}
			break;

		case MSG_EVENT:
			switch (pMsg->event.type) {
				case E_MOUSE_UP:
					if (gCurControl) {
						Object *pObject = ADerefAs(Object, gCurControl);
						if (pObject->HandleMsg(pMsg))
							return true;
					}
					break;

				case E_KEY_DOWN:
					switch (pMsg->event.value) {
						case K_ESC:
						case K_D:
							Cleanup();
							return true;
					}
					break;
			}
			break;
	}
	return true;
}

void
SettingsMenu::Cleanup()
{
	MainMenu_ResetIdleTimer();
	Activate(false);
	delete this;
}
