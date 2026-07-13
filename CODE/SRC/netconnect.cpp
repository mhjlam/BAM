// NETCONNECT.CPP
//
// Network connection screen shown when "Network Game" is chosen from the
// main menu.  Player picks Host or Join, enters an IP (Join only), then
// clicks Connect.  On success transitions to BR_NET_CHAR.

#include "netconnect.hpp"

#include "bam.hpp"
#include "bamfuncs.hpp"
#include "api.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "context.hpp"
#include "comm.hpp"
#include "commmgr.hpp"
#include "eventmgr.hpp"
#include "mono.hpp"
#include "mouse.hpp"
#include "graphmgr.hpp"
#include "rect.hpp"
#include "tigre.hpp"
#include "tenetcomm.hpp"

#include <string.h>
#include <stdio.h>
#include "modex.hpp"
#include <SDL3/SDL.h>

// Re-use the main-menu background + layout constants so the screen fits in.
#define NC_BG_ANI       32
#define NC_BG_PRI       100
#define NC_BUTTON_PRI   200
#define NC_BOX_PRI      210

// Horizontal layout: match the main-menu panel column.
#define NC_PANEL_X      90
#define NC_PANEL_W      140
#define NC_BTN_X        (NC_PANEL_X + 20)  // 110

// Vertical positions
#define NC_BTN_HOST_Y   162
#define NC_BTN_JOIN_Y   184
#define NC_IP_LABEL_Y   220
#define NC_IP_BOX_Y     234
#define NC_STATUS_Y     249   // error / "Connecting…" feedback
#define NC_CAP2_Y       259   // decorative cap before action buttons
#define NC_BTN_CONN_Y   262
#define NC_BTN_CNCL_Y   284

// Button animations (alternating A/B like main menu)
#define NC_BTN_A        35
#define NC_BTN_B        36
#define NC_CAP_ANI      34

// -----------------------------------------------------------------------

NetConnect::NetConnect()
    : gback(grip{}), pback(nullptr), pbackCH(nullptr), rNumBack(0),
      gIpText(grip{}), fIsHost(true),
      _connecting(false), _connectDeadlineMs(0), _pEnetPending(nullptr)
{
    msgMask = E_KEY_DOWN | E_MOUSE_DOWN | E_MOUSE_UP;
}

NetConnect::~NetConnect()
{
}

// -----------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------

void NetConnect::Setup()
{
    BAM_Room::Setup();

    BAM_Button* pButton;

    // Full-screen background cel, same background art as the main menu.
    gback = ACreateCel(&rNumBack, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                       CI_BLACK, NC_BG_PRI);
    pback    = AGetResData(gback);
    pbackCH  = (CelHeader*)pback;
    CopyCel(pbackCH, 0, 0, RES_ANIM, NC_BG_ANI, 1, false);

    BAM_Guy* pGuy = &back;
    pGuy->SetRes(RES_CEL, rNumBack, 1);
    pGuy->SetPos(0, 0);
    pGuy->SetContext(gSelf);
    pGuy->Setup(CT_ROST);
    pGuy->SetPri(NC_BG_PRI);

    pal.Load(NC_BG_ANI);

    // Legalese text (upper right, same as all other menus)
    {
        SquibRes sqb;
        char* pTxt = sqb.Load(7010, 300);
        pFontMgr->SetRes(9060);
        pFontMgr->point++;
        SetFontColors(CI_SKIP, 10, 7, 10, 7, 26, 23);
        ASetString(183, 1, pTxt, (uchar*)pbackCH, 138, 0, DG_JUST_LEFT);
    }

    // Side titles matching other menus
    pFontMgr->SetRes(9052);
    SetFontColors(CI_SKIP, 45, 46, 47, 49, 50, CI_BLACK);
    {
        SquibRes sqb;
        char* pTxt = sqb.Load(7030, 10);  // "Two Player Menu" from double-menu SQB
        ASetString(23,  200, pTxt, (uchar*)pbackCH, 67, 0, DG_JUST_CENTER);
        ASetString(230, 200, pTxt, (uchar*)pbackCH, 67, 0, DG_JUST_CENTER);
    }

    pFontMgr->SetRes(9050);
    SetFontColors(CI_SKIP, 93, 90, 93, 90, 93, 90);

    // Decorative cap before mode buttons
    CopyCel(pbackCH, NC_BTN_X, NC_BTN_HOST_Y - 3, RES_ANIM, NC_CAP_ANI, 1, false);

    // HOST GAME button
    pButton = &hostButton;
    pButton->Create(NC_BTN_X, NC_BTN_HOST_Y, NC_BUTTON_PRI,
                    RES_ANIM, NC_BTN_A, 1, gSelf);
    pButton->SetupReplies(REPLY_DESELECTED);
    pButton->fIsToggle = false;
    pButton->SetOwnerCel(rNumBack);
    pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
    pButton->SetCelText(1, "Host Game");
    pButton->SetColors(1, 93, 90);
    pButton->SetCelText(2, "Host Game");
    pButton->SetColors(2, 155, 142);
    pButton->Select(false);

    // JOIN GAME button
    pButton = &joinButton;
    pButton->Create(NC_BTN_X, NC_BTN_JOIN_Y, NC_BUTTON_PRI,
                    RES_ANIM, NC_BTN_B, 1, gSelf);
    pButton->SetupReplies(REPLY_DESELECTED);
    pButton->fIsToggle = false;
    pButton->SetOwnerCel(rNumBack);
    pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
    pButton->SetCelText(1, "Join Game");
    pButton->SetColors(1, 93, 90);
    pButton->SetCelText(2, "Join Game");
    pButton->SetColors(2, 155, 142);
    pButton->Select(false);

    // IP address text box (created always; shown/hidden by RefreshIpField)
    gIpText = AMalloc(64);
    char* pText = ADerefAs(char, gIpText);
    pText[0] = '\0';
    ipBox.SetColors(CI_SKIP, 93, 90, 93, 90, 93, 90, 155, 142);
    ipBox.SetFont(9050);
    ipBox.Create(NC_BTN_X, NC_IP_BOX_Y, 120, 15, NC_BOX_PRI,
                 gIpText, 63, gSelf, rNumBack, 0, 0);
    ipBox.SetupReplies(REPLY_DESELECTED);

    // Decorative cap before action buttons
    CopyCel(pbackCH, NC_BTN_X, NC_CAP2_Y, RES_ANIM, NC_CAP_ANI, 2, false);

    // CONNECT button
    pButton = &connectButton;
    pButton->Create(NC_BTN_X, NC_BTN_CONN_Y, NC_BUTTON_PRI,
                    RES_ANIM, NC_BTN_A, 1, gSelf);
    pButton->SetupReplies(REPLY_DESELECTED);
    pButton->fIsToggle = false;
    pButton->SetOwnerCel(rNumBack);
    pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
    pButton->SetCelText(1, "Connect");
    pButton->SetColors(1, 93, 90);
    pButton->SetCelText(2, "Connect");
    pButton->SetColors(2, 155, 142);
    pButton->Select(false);

    // CANCEL button
    pButton = &cancelButton;
    pButton->Create(NC_BTN_X, NC_BTN_CNCL_Y, NC_BUTTON_PRI,
                    RES_ANIM, NC_BTN_B, 1, gSelf);
    pButton->SetupReplies(REPLY_DESELECTED);
    pButton->fIsToggle = false;
    pButton->SetOwnerCel(rNumBack);
    pButton->SetTextJustify(DG_JUST_CENTER, DG_JUST_CENTER);
    pButton->SetCelText(1, "Cancel");
    pButton->SetColors(1, 93, 90);
    pButton->SetCelText(2, "Cancel");
    pButton->SetColors(2, 155, 142);
    pButton->Select(false);

    // Apply initial mode highlight and ip field visibility, then fade in.
    UpdateModeHighlight();
    RefreshIpField(false);  // host is the default — no IP needed
    AUpdateRect((Rectangle*)nullptr);
    pal.FadeUp();
}

// -----------------------------------------------------------------------
// Cleanup
// -----------------------------------------------------------------------

void NetConnect::Cleanup()
{
    pal.FadeToBlack();
    if (gIpText)
    {
        AFree(gIpText);
        gIpText = grip{};
    }
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

// Highlight the active mode button (cel-1 uses bright colors) and dim the other.
void NetConnect::UpdateModeHighlight()
{
    if (fIsHost) {
        hostButton.SetColors(1, 155, 142);
        joinButton.SetColors(1, 93, 90);
    } else {
        hostButton.SetColors(1, 93, 90);
        joinButton.SetColors(1, 155, 142);
    }
    hostButton.SetState(S_CHANGED, true);
    hostButton.Select(false);
    joinButton.SetState(S_CHANGED, true);
    joinButton.Select(false);
}

// Show or hide the "Host IP Address" label and text-entry box.
void NetConnect::RefreshIpField(bool show)
{
    // Restore original background pixels for the label+box area.
    CopyCel(pbackCH, NC_BTN_X, NC_IP_LABEL_Y - 2,
            RES_ANIM, NC_BG_ANI, 1,
            NC_BTN_X, NC_IP_LABEL_Y - 2,
            NC_BTN_X + NC_PANEL_W - 20, NC_IP_BOX_Y + 16,
            false);

    if (show) {
        pFontMgr->SetRes(9050);
        SetFontColors(CI_SKIP, 93, 90, 93, 90, 93, 90);
        ASetString(NC_BTN_X, NC_IP_LABEL_Y, "Host IP Address:",
                   (uchar*)pbackCH, NC_PANEL_W, 0);
        ipBox.currState = false;
        ipBox.Draw();
        ipBox.Listen(true);
    } else {
        ipBox.currState = false;
        ipBox.Listen(false);
    }

    Rectangle upd;
    upd.Set(0, NC_IP_LABEL_Y - 2, SCREEN_WIDTH - 1, NC_IP_BOX_Y + 17);
    AUpdateRect(&upd);
}

// Write a one-line status message (error / "Connecting…") into the bg cel.
void NetConnect::ShowStatus(const char* msg)
{
    Rectangle r;
    r.Set(NC_BTN_X, NC_STATUS_Y, NC_BTN_X + NC_PANEL_W - 20, NC_STATUS_Y + 12);
    {
        uchar* pixels = (uchar*)pbackCH + sizeof(CelHeader);
        for (int fy = r.y1; fy <= r.y2; fy++)
            memset(pixels + fy * pbackCH->width + r.x1, 0, r.x2 - r.x1 + 1);
    }

    pFontMgr->SetRes(9050);
    SetFontColors(CI_SKIP, 93, 90, 93, 90, 93, 90);
    ASetString(NC_BTN_X, NC_STATUS_Y, msg,
               (uchar*)pbackCH, NC_PANEL_W, 0, DG_JUST_CENTER);

    Rectangle upd;
    upd.Set(0, NC_STATUS_Y - 1, SCREEN_WIDTH - 1, NC_STATUS_Y + 13);
    AUpdateRect(&upd);
}

// -----------------------------------------------------------------------
// DoConnect — starts a non-blocking connect; Cycle() polls each frame
// -----------------------------------------------------------------------

void NetConnect::DoConnect()
{
    if (!fIsHost)
    {
        char* pText = ADerefAs(char, gIpText);
        if (!pText || pText[0] == '\0')
        {
            ShowStatus("Enter a host IP address.");
            return;
        }
        strncpy(TEnetComm::s_remote_host, pText, 255);
        TEnetComm::s_remote_host[255] = '\0';
    }
    TEnetComm::s_is_host = fIsHost;
    TEnetComm::s_port    = 7733;

    pBam->fNetworkTest   = true;

    TEnetComm* pEnet = new TEnetComm();
    if (pCommMgr->Init(pEnet, 0) != TComm::ALL_OK)
    {
        pBam->fNetworkTest = false;
        ShowStatus("Network init failed.");
        return;
    }
    pComm = pEnet;

    if (pEnet->BeginConnect() != TComm::ALL_OK)
    {
        pCommMgr->Disconnect();
        pBam->fNetworkTest = false;
        ShowStatus("Connection failed.");
        return;
    }

    _pEnetPending      = pEnet;
    _connectDeadlineMs = SDL_GetTicks() + 30000;
    _connecting        = true;

    pMouse->SetRes(RES_ANIM, POINTER_RES, 7);
    ShowStatus(fIsHost ? "Waiting for opponent... (Cancel to abort)"
                       : "Connecting to host...");
}

// -----------------------------------------------------------------------
// AbortConnect — cancel an in-progress non-blocking connect
// -----------------------------------------------------------------------

void NetConnect::AbortConnect(const char* msg)
{
    _connecting    = false;
    _pEnetPending  = nullptr;

    pCommMgr->Disconnect();
    bGlobal.storyLine  = STORY_NONE;
    pBam->fNetworkTest = false;

    pMouse->SetRes(RES_ANIM, POINTER_RES, 1);
    ShowStatus(msg);
}

// -----------------------------------------------------------------------
// FinishConnect — called once PollConnect returns true
// -----------------------------------------------------------------------

void NetConnect::FinishConnect()
{
    _connecting   = false;
    _pEnetPending = nullptr;

    pMouse->SetRes(RES_ANIM, POINTER_RES, 1);

    bGlobal.storyLine = NETGAME;

    TEnetComm* pEnet = static_cast<TEnetComm*>(pComm);
    pEnet->FinishConnect();

    ASeedRandom(TEnetComm::s_rng_seed);
    ASeedRandom2(TEnetComm::s_rng_seed);

    int    me = pCommMgr->GetUserID();
    uint16 others[2];
    pCommMgr->GetUserList(others);
    if (me < (int)others[0])
    {
        pBam->playerSide         = SIDE1;
        pBam->playerTypes[SIDE1] = PLAYER_LOCAL;
        pBam->playerTypes[SIDE2] = me;
    }
    else
    {
        pBam->playerSide         = SIDE2;
        pBam->playerTypes[SIDE2] = PLAYER_LOCAL;
        pBam->playerTypes[SIDE1] = me;
    }

    bGlobal.roomMgr.NewRoom(BR_NET_CHAR);
}

// -----------------------------------------------------------------------
// HandleMsg
// -----------------------------------------------------------------------

bool NetConnect::HandleMsg(Message* pMsg)
{
    if (pMsg->type == MSG_NOTICE && pMsg->notice.type == N_CONTROL_REPLY &&
        (uint16)(uintptr_t)pMsg->notice.param == REPLY_DESELECTED)
    {
        if (pMsg->notice.gSource == hostButton.gSelf)
        {
            fIsHost = true;
            UpdateModeHighlight();
            RefreshIpField(false);
            return true;
        }
        if (pMsg->notice.gSource == joinButton.gSelf)
        {
            fIsHost = false;
            UpdateModeHighlight();
            RefreshIpField(true);
            return true;
        }
        if (pMsg->notice.gSource == connectButton.gSelf)
        {
            DoConnect();
            return true;
        }
        if (pMsg->notice.gSource == cancelButton.gSelf)
        {
            if (_connecting)
                AbortConnect("Cancelled.");
            else
                bGlobal.roomMgr.NewRoom(BR_MENU);
            return true;
        }
        if (pMsg->notice.gSource == ipBox.gSelf)
        {
            return true;
        }
    }
    return BAM_Room::HandleMsg(pMsg);
}

// -----------------------------------------------------------------------
// Cycle
// -----------------------------------------------------------------------

void NetConnect::Cycle()
{
    if (_connecting)
    {
        if (_pEnetPending->PollConnect(5))
        {
            FinishConnect();   // transitions to BR_NET_CHAR
            return;
        }
        if (SDL_GetTicks() >= _connectDeadlineMs)
        {
            AbortConnect("Connection timed out.");
            return;
        }
    }

    BAM_Room::Cycle();
}
