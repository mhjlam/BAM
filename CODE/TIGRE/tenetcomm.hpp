#ifndef tenetcomm_hpp
#define tenetcomm_hpp

#include "comm.hpp"
#include <enet/enet.h>
#include <stdint.h>

class TEnetComm : public TComm
{
public:
    static bool     s_is_host;
    static char     s_remote_host[256];
    static uint16_t s_port;           // default 7733
    static uint32_t s_rng_seed;       // exchanged during Connect(); apply via ASeedRandom after

    TEnetComm();
    virtual ~TEnetComm();

    virtual uint16 GetUserList(uint16* pList = nullptr) override;

    // Step-by-step connection for interruptible connect loops:
    ERROR BeginConnect();          // host: no-op; client: initiates peer connect
    bool  PollConnect(int ms);     // one enet_host_service call; true when connected
    void  FinishConnect();         // exchange RNG seed after PollConnect returns true

protected:
    virtual ERROR Init(long optionalArg = 0) override;
    virtual ERROR Connect() override;
    virtual ERROR Disconnect() override;
    virtual ERROR SendPacket(sPacket* pPacket, bool fIsResend = false) override;
    virtual ERROR ReceivePacket(sPacket* pPacket) override;
    virtual bool  IsPacketAvailable() override;
    virtual ERROR DiscardPacket(sPacketHeader* pHeader = nullptr) override;

private:
    ENetHost* _host;
    ENetPeer* _peer;
    bool      _packet_ready;
    bool      _connected;
    uint8_t   _recv_buf[512];   // sized to tcommmgrTOTAL_BLOCK_SIZE
};

#endif
