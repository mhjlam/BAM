// OPTION4.CPP
//
// OptionsMenu  – routing submenu opened from OptionMenu
// SpeedSlider  – discrete 3-position slider
// GameSpeedMenu – game-speed submenu
// (Health Bars toggle lives in OptionsMenu, not SettingsMenu, so it can be changed mid-game)

#include "option4.hpp"

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
#include "game_config.hpp"
#include "tigre.hpp"
#include "eventmgr.hpp"
#include "option2.hpp"
#include "graphmgr.hpp"

#include <string.h>

#define OPTION4_WOFF_X  89
#define OPTION4_WOFF_Y  92

// Rendering priority layers (must sit between OptionMenu and NoiseMenu):
//   OptionMenu    back=20000  btns=20001
//   OptionsMenu   back=20003  btns=20004
//   NoiseMenu     back=20010  slide=20011  hdl=20012
//   GameSpeedMenu back=20015  slide=20016  hdl=20017
#define OPTS_BACK_PRI   20003
#define OPTS_BTN_PRI    20004
#define SPD_BACK_PRI    20015
#define SPD_SLIDE_PRI   20016
#define SPD_HDL_PRI     20017

// Speed label x-positions within the background cel (y=79, same row as OFF/MAX
// in NoiseMenu).  Chosen so labels don't overlap and FASTEST fits within the
// ~121px-wide window interior (cel x 0..120 ≈ screen 89..209).
// Slider label positions (cel coords).  NORMAL and FASTEST sit above the
// track (y=50), FAST sits below (y=74) so the three labels don't overlap.
// Slider track is at cel y=63 (screen 155 - woff 92).
static const int kLabelX[3] = { 24, 58, 75 };   // NORMAL / FAST / FASTEST
static const int kLabelY[3] = { 34, 84, 34 };
static const char* kLabelText[3] = { "NORMAL", "FAST", "FASTEST" };

static GameSpeed level_to_speed(int16 level)
{
	if (level == 1) return GameSpeed::Fast;
	if (level == 2) return GameSpeed::Fastest;
	return GameSpeed::Normal;
}

static int16 speed_to_level(GameSpeed spd)
{
	if (spd == GameSpeed::Fast)    return 1;
	if (spd == GameSpeed::Fastest) return 2;
	return 0;
}


// =================================================================
//  OptionsMenu
// =================================================================

OptionsMenu::OptionsMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
}

OptionsMenu::~OptionsMenu()
{
}

void
OptionsMenu::Setup()
{
	BAM_Guy    *pGuy;
	BAM_Button *pButton;
	uchar      *pback;
	CelHeader  *pbackAnimCH, *pbackCH;
	int         x, buttonY;

	if (bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		mode = M_MODELESS;
	else
		mode = M_MODAL;

	gbackAnim = ALoad(RES_ANIM, 40);
	pbackAnimCH = (CelHeader*)AGetResData(gbackAnim);
	rback.Set(OPTION4_WOFF_X, OPTION4_WOFF_Y,
	          OPTION4_WOFF_X - 1 + pbackAnimCH->width,
	          OPTION4_WOFF_Y - 1 + pbackAnimCH->height);

	gback = ACreateCel(&rNumBack, 0, 0,
	                   pbackAnimCH->width, pbackAnimCH->height,
	                   CI_BLACK, OPTS_BACK_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	CopyCel(pbackCH, 0, 0, RES_ANIM, 40, 1, false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(OPTION4_WOFF_X, OPTION4_WOFF_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(OPTS_BACK_PRI);

	// --- Layout: 5 buttons split 4+1 ---
	// Group 1: Sounds, Music, Game Speed, Health Bars
	// Group 2: Done
	const int buttonNum = 5;
	int topMargin = OPTION4_WOFF_Y + (210 - (18 + 17 * buttonNum)) / 2;
	int capPosY[4];
	capPosY[0] = topMargin;
	capPosY[1] = capPosY[0] + 3 + 17 * 4;
	capPosY[2] = capPosY[1] + 3 + 6;
	capPosY[3] = capPosY[2] + 3 + 17 * 1;

	CopyCel(pbackCH, 112 - OPTION4_WOFF_X, capPosY[0] - OPTION4_WOFF_Y, RES_ANIM, 40, 2, false);
	CopyCel(pbackCH, 112 - OPTION4_WOFF_X, capPosY[1] - OPTION4_WOFF_Y, RES_ANIM, 40, 3, false);
	CopyCel(pbackCH, 112 - OPTION4_WOFF_X, capPosY[2] - OPTION4_WOFF_Y, RES_ANIM, 40, 2, false);
	CopyCel(pbackCH, 112 - OPTION4_WOFF_X, capPosY[3] - OPTION4_WOFF_Y, RES_ANIM, 40, 3, false);

	static const char* labels[5] = { "SOUNDS", "MUSIC", "GAME SPEED", "HEALTH BARS", "DONE" };

	cfg = LoadGameConfig();

	for (x = 0; x < buttonNum; x++) {
		if (x == buttonNum - 1)
			buttonY = topMargin + 15 + x * 17;
		else
			buttonY = topMargin + 3 + x * 17;

		pButton = &button[x];
		if (x % 2 == 0)
			pButton->Create(112, buttonY, OPTS_BTN_PRI, RES_ANIM, 42, 1,
			                gSelf, OPTION4_WOFF_X, OPTION4_WOFF_Y);
		else
			pButton->Create(112, buttonY, OPTS_BTN_PRI, RES_ANIM, 44, 1,
			                gSelf, OPTION4_WOFF_X, OPTION4_WOFF_Y);

		pButton->SetOwnerCel(rNumBack);
		pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
		pButton->SetCelText(1, (char*)labels[x]);
		pButton->SetColors(1, 93, 90);
		pButton->SetCelText(2, (char*)labels[x]);
		pButton->SetColors(2, 155, 142);

		if (x == 3) {
			// Health Bars: toggle button reflecting current setting
			pButton->fIsToggle = true;
			pButton->SetupReplies(REPLY_ACTIVATED | REPLY_DEACTIVATED);
			pButton->Select(cfg.health_bars);
		} else {
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
OptionsMenu::HandleMsg(Message* pMsg)
{
	char mess[100];

	if (BAM_Room::HandleMsg(pMsg))
		return true;

	switch (pMsg->type) {
		case MSG_NOTICE:
			if (pMsg->notice.type == N_CONTROL_REPLY) {
				uint16 reply = (uint16)(uintptr_t)pMsg->notice.param;

				// Health Bars toggle sends ACTIVATED/DEACTIVATED, handle before DESELECTED check
				if (pMsg->notice.gSource == button[3].gSelf) {
					cfg.health_bars = (reply == REPLY_ACTIVATED);
					SetHealthBarsEnabled(cfg.health_bars);
					SaveGameConfig(cfg);
					return true;
				}

				if (reply != REPLY_DESELECTED) {
					snprintf(mess, sizeof(mess),
					         "OptionsMenu: unrecognized reply: %d", (int)(uintptr_t)pMsg->notice.param);
					APanic(mess);
				}
				if (pMsg->notice.gSource == button[0].gSelf) {
					NoiseMenu *pNoise = new NoiseMenu;
					pNoise->Setup(SOUND_BUTTON);
					return true;
				}
				if (pMsg->notice.gSource == button[1].gSelf) {
					NoiseMenu *pNoise = new NoiseMenu;
					pNoise->Setup(MUSIC_BUTTON);
					return true;
				}
				if (pMsg->notice.gSource == button[2].gSelf) {
					if (bGlobal.storyLine != NETGAME) {
						GameSpeedMenu *pSpeed = new GameSpeedMenu;
						pSpeed->Setup();
					}
					return true;
				}
				if (pMsg->notice.gSource == button[4].gSelf) {
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
						case K_Q:
						case K_C:
							Cleanup();
							return true;
					}
					break;
			}
			break;
	}
	return true;   // no pass-through
}

void
OptionsMenu::Cleanup()
{
	Activate(false);
	delete this;
}


// =================================================================
//  SpeedSlider
// =================================================================

void
SpeedSlider::Setup(grip gCtx, res_t theType, uint theNum, uint theCel,
                   coord theX, coord theY, int thePri,
                   int limitA, int limitB, int activeWidth,
                   int theDir, int16 level_P)
{
	lastLevel = -1;
	level     = level_P;
	maxLevel  = 2;

	msgMask      = E_MOUSE_DOWN | E_MOUSE_UP;
	fIsMouseDown = false;

	SetContext(gCtx);
	SetRes(theType, theNum, theCel);
	SetPri(thePri);
	BAM_Guy::Setup(CT_ROST | CT_MSGS);
	orientation = theDir;

	topOrLeft     = limitA;
	rightOrBottom = limitB;

	if (orientation == HORIZONTAL) {
		slideRect.Set(limitA, theY, limitB, theY);
		containRect.Set(limitA - 5, theY - activeWidth,
		                limitB + 5, theY + activeWidth);
	}

	// Set initial handle position at the correct snap point
	int mid = (topOrLeft + rightOrBottom) / 2;
	coord initX;
	if      (level == 1) initX = (coord)mid;
	else if (level == 2) initX = (coord)rightOrBottom;
	else                  initX = (coord)topOrLeft;
	BAM_Guy::SetPos(initX, theY);

	UpdateLabel();
}

void
SpeedSlider::SetPos(coord theX, coord theY)
{
	// Snap to one of three positions:
	//   topOrLeft (Normal) / midpoint (Fast) / rightOrBottom (Fastest)
	int mid = (topOrLeft + rightOrBottom) / 2;
	int b1  = (topOrLeft + mid) / 2;
	int b2  = (mid + rightOrBottom) / 2;

	if      (theX <= b1) { scrim.x = (coord)topOrLeft;      level = 0; }
	else if (theX <= b2) { scrim.x = (coord)mid;             level = 1; }
	else                  { scrim.x = (coord)rightOrBottom;   level = 2; }

	scrim.SetRect();
	SetState(S_CHANGED, true);
}

void
SpeedSlider::Cycle()
{
	Slider::Cycle();
	ASetGameSpeed(level_to_speed(level));
	UpdateLabel();
}

void
SpeedSlider::UpdateLabel()
{
	if (level == lastLevel)
		return;

	GameSpeedMenu *pMenu = ADerefAs(GameSpeedMenu, gContext);
	CelHeader     *pCel  = (CelHeader*)AGetResData(pMenu->gback);

	uchar saveF, saveF1, saveB;
	saveF  = AFontColor(FNT_FORE_COLOR);
	saveF1 = AFontColor(FNT_FORE_COLOR + 1);
	saveB  = AFontColor(FNT_BACK_COLOR);

	// Redraw all three labels: active one bright, others dim.
	// Using CI_SKIP background so character pixels overwrite previous draw.
	pFontMgr->colors[FNT_BACK_COLOR] = CI_SKIP;
	for (int i = 0; i < 3; i++) {
		if (i == level) {
			pFontMgr->colors[FNT_FORE_COLOR]     = 155;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = 142;
		} else {
			pFontMgr->colors[FNT_FORE_COLOR]     = 93;
			pFontMgr->colors[FNT_FORE_COLOR + 1] = 90;
		}
		ASetString(kLabelX[i], kLabelY[i], (char*)kLabelText[i],
		           (uchar*)pCel, pCel->width - kLabelX[i], 0);
	}

	pFontMgr->colors[FNT_FORE_COLOR]     = saveF;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = saveF1;
	pFontMgr->colors[FNT_BACK_COLOR]     = saveB;

	AUpdateRect(&pMenu->rSpeedLabel);
	lastLevel = level;
}


// =================================================================
//  GameSpeedMenu
// =================================================================

GameSpeedMenu::GameSpeedMenu()
{
	msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
	gback = grip{};
}

GameSpeedMenu::~GameSpeedMenu()
{
}

void
GameSpeedMenu::Setup()
{
	BAM_Guy    *pGuy;
	BAM_Button *pButton;
	uchar      *pback;
	CelHeader  *pbackAnimCH, *pbackCH;

	if (bGlobal.storyLine == NETGAME && !bGlobal.netDisconnect)
		mode = M_MODELESS;
	else
		mode = M_MODAL;

	gbackAnim   = ALoad(RES_ANIM, 40);
	pbackAnimCH = (CelHeader*)AGetResData(gbackAnim);
	rback.Set(OPTION4_WOFF_X, OPTION4_WOFF_Y,
	          OPTION4_WOFF_X - 1 + pbackAnimCH->width,
	          OPTION4_WOFF_Y - 1 + pbackAnimCH->height);

	gback = ACreateCel(&rNumBack, 0, 0,
	                   pbackAnimCH->width, pbackAnimCH->height,
	                   CI_BLACK, SPD_BACK_PRI);
	pback = AGetResData(gback);
	pbackCH = (CelHeader*)pback;

	CopyCel(pbackCH, 0, 0, RES_ANIM, 40, 1, false);

	pGuy = &back;
	pGuy->SetRes(RES_CEL, rNumBack);
	pGuy->SetPos(OPTION4_WOFF_X, OPTION4_WOFF_Y);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SPD_BACK_PRI);

	// Slider track background (cel 4 from the frame animation)
	pGuy = &slideBack;
	pGuy->SetRes(RES_ANIM, 40, 4);
	pGuy->SetPos(112, 140);
	pGuy->SetContext(gSelf);
	pGuy->Setup(CT_ROST);
	pGuy->SetPri(SPD_SLIDE_PRI);

	// --- Draw speed labels at initial state ---
	uchar saveF, saveF1, saveB;
	saveF  = AFontColor(FNT_FORE_COLOR);
	saveF1 = AFontColor(FNT_FORE_COLOR + 1);
	saveB  = AFontColor(FNT_BACK_COLOR);

	// All labels start dim; UpdateLabel() will brighten the current one
	pFontMgr->colors[FNT_FORE_COLOR]     = 93;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = 90;
	pFontMgr->colors[FNT_BACK_COLOR]     = CI_SKIP;
	for (int i = 0; i < 3; i++) {
		ASetString(kLabelX[i], kLabelY[i], (char*)kLabelText[i],
		           (uchar*)pbackCH, pbackCH->width - kLabelX[i], 0);
	}

	// Speed label invalidation rect (covers above and below rows, screen coords)
	rSpeedLabel.Set(OPTION4_WOFF_X,      OPTION4_WOFF_Y + 40,
	                rback.x2,            OPTION4_WOFF_Y + 84);

	// --- Draw title ---
	pFontMgr->colors[FNT_FORE_COLOR]     = 93;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = 90;
	ASetString(20, 98, (char*)"GAME SPEED",
	           (uchar*)pbackCH, pbackCH->width - 40, 0, DG_JUST_CENTER);

	pFontMgr->colors[FNT_FORE_COLOR]     = saveF;
	pFontMgr->colors[FNT_FORE_COLOR + 1] = saveF1;
	pFontMgr->colors[FNT_BACK_COLOR]     = saveB;

	// --- Set up slider ---
	saveSpeed = LoadGameConfig().game_speed;
	slider.Setup(gSelf, RES_ANIM, 40, 5,
	             120, 155, SPD_HDL_PRI,
	             120, 198, 12, HORIZONTAL,
	             speed_to_level(saveSpeed));

	// --- Button caps ---
	CopyCel(pbackCH, 112 - OPTION4_WOFF_X, 219 - OPTION4_WOFF_Y, RES_ANIM, 40, 2, false);
	CopyCel(pbackCH, 112 - OPTION4_WOFF_X, 256 - OPTION4_WOFF_Y, RES_ANIM, 40, 3, false);

	// Done button
	pButton = &button[0];
	pButton->Create(112, 222, SPD_SLIDE_PRI, RES_ANIM, 42, 1,
	                gSelf, OPTION4_WOFF_X, OPTION4_WOFF_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetCelText(1, (char*)"DONE");
	pButton->SetColors(1, 93, 90);
	pButton->SetCelText(2, (char*)"DONE");
	pButton->SetColors(2, 155, 142);
	pButton->Draw();

	// Cancel button
	pButton = &button[1];
	pButton->Create(112, 239, SPD_SLIDE_PRI, RES_ANIM, 44, 1,
	                gSelf, OPTION4_WOFF_X, OPTION4_WOFF_Y);
	pButton->SetupReplies(REPLY_DESELECTED);
	pButton->fIsToggle = false;
	pButton->SetOwnerCel(rNumBack);
	pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
	pButton->SetCelText(1, (char*)"CANCEL");
	pButton->SetColors(1, 93, 90);
	pButton->SetCelText(2, (char*)"CANCEL");
	pButton->SetColors(2, 155, 142);
	pButton->Draw();

	Activate(true);
	pContextMgr->lContexts.Move(grip_to_ptr(gSelf), L_FRONT, nullptr);
	gCurControl = grip{};
}

bool
GameSpeedMenu::HandleMsg(Message* pMsg)
{
	char mess[100];

	if (BAM_Room::HandleMsg(pMsg))
		return true;

	switch (pMsg->type) {
		case MSG_NOTICE:
			if (pMsg->notice.type == N_CONTROL_REPLY) {
				if ((uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED) {
					if (pMsg->notice.gSource == button[0].gSelf) {
						// Done: apply and save
						SetSpeed(level_to_speed(slider.level));
						Cleanup();
						return true;
					}
					if (pMsg->notice.gSource == button[1].gSelf) {
						// Cancel: revert
						ASetGameSpeed(saveSpeed);
						Cleanup();
						return true;
					}
				} else {
					snprintf(mess, sizeof(mess),
					         "GameSpeedMenu: unrecognized reply: %d", (int)(uintptr_t)pMsg->notice.param);
					APanic(mess);
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
						case K_A:
							FakeMouseClick(button[0].gSelf);
							return true;
						case K_ESC:
						case K_C:
							FakeMouseClick(button[1].gSelf);
							return true;
					}
					break;
			}
			break;
	}
	return true;   // no pass-through
}

void
GameSpeedMenu::SetSpeed(GameSpeed spd)
{
	ASetGameSpeed(spd);
	GameConfig cfg = LoadGameConfig();
	cfg.game_speed = spd;
	SaveGameConfig(cfg);
}

void
GameSpeedMenu::Cleanup()
{
	Activate(false);
	delete this;
}
