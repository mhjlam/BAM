#include "tenetcomm.hpp"
#include "commmgr.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Static configuration — set before Init() is called
// ---------------------------------------------------------------------------
bool     TEnetComm::s_is_host          = false;
char     TEnetComm::s_remote_host[256] = {};
uint16_t TEnetComm::s_port             = 7733;
uint32_t TEnetComm::s_rng_seed         = 42;

TEnetComm::TEnetComm()
    : _host(nullptr), _peer(nullptr), _packet_ready(false), _connected(false)
{
    memset(_recv_buf, 0, sizeof(_recv_buf));
}

TEnetComm::~TEnetComm()
{
}

// ---------------------------------------------------------------------------
// Init: initialise ENet and create the host object
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::Init(long /*optionalArg*/)
{
    if (enet_initialize() != 0)
        return SetError(INIT_FAILED);

    ENetHost* host;
    if (s_is_host) {
        ENetAddress addr;
        addr.host = ENET_HOST_ANY;
        addr.port = s_port;
        host = enet_host_create(&addr, 1, 2, 0, 0);
    } else {
        host = enet_host_create(nullptr, 1, 2, 0, 0);
    }

    if (!host) {
        enet_deinitialize();
        return SetError(INIT_FAILED);
    }

    _host = host;
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// BeginConnect: for client, kick off enet_host_connect; for host, no-op
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::BeginConnect()
{
    _connected = false;
    if (!s_is_host) {
        ENetAddress addr;
        enet_address_set_host(&addr, s_remote_host);
        addr.port = s_port;
        _peer = enet_host_connect(_host, &addr, 2, 0);
        if (!_peer)
            return SetError(CONNECTION_FAILED);
    }
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// PollConnect: one enet_host_service call; returns true when peer connected
// ---------------------------------------------------------------------------
bool TEnetComm::PollConnect(int timeout_ms)
{
    if (_connected) return true;
    if (!_host)     return false;

    ENetEvent ev;
    int ret = enet_host_service(_host, &ev, timeout_ms);
    if (ret > 0) {
        if (ev.type == ENET_EVENT_TYPE_CONNECT) {
            if (s_is_host) _peer = ev.peer;
            _connected = true;
        } else if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(ev.packet);
        }
    }
    return _connected;
}

// ---------------------------------------------------------------------------
// FinishConnect: exchange RNG seed after PollConnect returns true
// ---------------------------------------------------------------------------
void TEnetComm::FinishConnect()
{
    if (s_is_host) {
        wConsoleNode = 0;
        uint32_t seed = ((uint32_t)rand() & 0x7FFFFFFF) | 1u;
        ENetPacket* sp = enet_packet_create(&seed, sizeof(seed),
                                            ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(_peer, 0, sp);
        enet_host_flush(_host);
        s_rng_seed = seed;
    } else {
        wConsoleNode = 1;
        uint32_t seed = 42;
        ENetEvent ev;
        for (int i = 0; i < 50; i++) {
            int ret = enet_host_service(_host, &ev, 100);
            if (ret > 0 && ev.type == ENET_EVENT_TYPE_RECEIVE) {
                if (ev.packet->dataLength >= sizeof(seed))
                    memcpy(&seed, ev.packet->data, sizeof(seed));
                enet_packet_destroy(ev.packet);
                break;
            }
        }
        s_rng_seed = seed;
    }
}

// ---------------------------------------------------------------------------
// Connect: blocking wrapper used by CLI (-NET) mode
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::Connect()
{
    ERROR e = BeginConnect();
    if (e != ALL_OK) return e;

    bool ok = false;
    for (int i = 0; i < 300; i++) {
        if (PollConnect(100)) { ok = true; break; }
    }
    if (!ok) return SetError(CONNECTION_FAILED);

    FinishConnect();
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// Disconnect: graceful shutdown
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::Disconnect()
{
    if (_peer) {
        enet_peer_disconnect(_peer, 0);
        ENetEvent ev;
        for (int i = 0; i < 30; i++) {
            if (enet_host_service(_host, &ev, 100) <= 0) break;
            if (ev.type == ENET_EVENT_TYPE_DISCONNECT) break;
            if (ev.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(ev.packet);
        }
        _peer = nullptr;
    }
    if (_host) {
        enet_host_destroy(_host);
        _host = nullptr;
    }
    enet_deinitialize();
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// SendPacket: serialise sPacket header + payload into a reliable ENet packet
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::SendPacket(sPacket* pkt, bool /*fIsResend*/)
{
    if (!_peer)
        return SetError(SEND_FAILED);

    uint8_t buf[512];
    size_t hdr_sz  = sizeof(sPacketHeader);
    size_t data_sz = pkt->header.len;

    if (hdr_sz + data_sz > sizeof(buf))
        return SetError(SEND_FAILED);

    memcpy(buf,           &pkt->header, hdr_sz);
    if (data_sz && pkt->pData)
        memcpy(buf + hdr_sz, pkt->pData,  data_sz);

    ENetPacket* ep = enet_packet_create(buf, hdr_sz + data_sz,
                                        ENET_PACKET_FLAG_RELIABLE);
    if (enet_peer_send(_peer, 0, ep) < 0)
        return SetError(SEND_FAILED);

    enet_host_flush(_host);
    totalBytesSent  += (int32)(hdr_sz + data_sz);
    totalPacketsSent++;
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// IsPacketAvailable: non-blocking poll for incoming data
// ---------------------------------------------------------------------------
bool TEnetComm::IsPacketAvailable()
{
    if (_packet_ready) return true;
    if (!_host)        return false;

    ENetEvent ev;
    if (enet_host_service(_host, &ev, 0) > 0) {
        if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
            size_t sz = ev.packet->dataLength;
            if (sz > sizeof(_recv_buf)) sz = sizeof(_recv_buf);
            memcpy(_recv_buf, ev.packet->data, sz);
            enet_packet_destroy(ev.packet);
            _packet_ready = true;
        } else if (ev.type == ENET_EVENT_TYPE_CONNECT) {
            if (s_is_host) _peer = ev.peer;
            _connected = true;
        } else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
            _peer = nullptr;
        }
    }
    return _packet_ready;
}

// ---------------------------------------------------------------------------
// ReceivePacket: copy buffered packet into caller's sPacket
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::ReceivePacket(sPacket* pkt)
{
    memcpy(&lastPacket.header, _recv_buf, sizeof(sPacketHeader));
    lastPacket.pData = _recv_buf + sizeof(sPacketHeader);
    if (pkt) {
        pkt->header = lastPacket.header;
        // Copy data into the caller's pre-allocated buffer (set by EnQueueData to the
        // queue slot's data area). Do NOT replace pData with a pointer to _recv_buf,
        // which would become stale when the next packet overwrites _recv_buf.
        if (pkt->pData && pkt->header.len > 0)
            memcpy(pkt->pData, lastPacket.pData, pkt->header.len);
    }
    _packet_ready = false;
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// DiscardPacket: drop the buffered packet
// ---------------------------------------------------------------------------
TComm::ERROR TEnetComm::DiscardPacket(sPacketHeader* /*pHeader*/)
{
    _packet_ready = false;
    return ClearError(), ALL_OK;
}

// ---------------------------------------------------------------------------
// GetUserList: return the two node IDs (local, remote)
// ---------------------------------------------------------------------------
uint16 TEnetComm::GetUserList(uint16* pList)
{
    if (pList) {
        pList[0] = wConsoleNode ? 0 : 1;  // other player's node ID
    }
    return 1;  // one remote player
}
