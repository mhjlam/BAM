#ifndef netconnect_hpp
#define netconnect_hpp

#include "alldefs.hpp"
#include "bam.hpp"
#include "bamguy.hpp"
#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "api.hpp"
#include "palette.hpp"
#include "text.hpp"

class TEnetComm;

class NetConnect : public BAM_Room
{
public:
    NetConnect();
    ~NetConnect();

    void    Setup();
    void    Cleanup();
    bool    HandleMsg(Message* pMsg);
    void    Cycle();

private:
    void    UpdateModeHighlight();
    void    RefreshIpField(bool show);
    void    ShowStatus(const char* msg);
    void    DoConnect();
    void    AbortConnect(const char* msg);
    void    FinishConnect();

    TPalette    pal;

    BAM_Guy     back;
    grip        gback;
    uchar*      pback;
    CelHeader*  pbackCH;
    uint        rNumBack;

    BAM_Button  hostButton;
    BAM_Button  joinButton;
    BAM_Button  connectButton;
    BAM_Button  cancelButton;

    BAM_Box     ipBox;
    grip        gIpText;

    bool        fIsHost;

    // Non-blocking connection state
    bool        _connecting;
    uint64_t    _connectDeadlineMs;
    TEnetComm*  _pEnetPending;
};

#endif
