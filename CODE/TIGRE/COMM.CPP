#include <string.h>

#include "Comm.hpp"

TComm::TComm()
{
	fDataRequired = FALSE;
	totalBytesSent = 0;
	totalPacketsSent = 0;
	_error = ALL_OK;
	timeout = tcommDefaultTimeout;
	wConsoleNode = tcommBadUserID;
	packetAvail = FALSE;
	memset(&lastPacket, 0, sizeof(lastPacket));
}

TComm::~TComm()
{
}

TComm::ERROR TComm::GetError()
{
	return _error;
}

TComm::ERROR TComm::SetError(ERROR err)
{
	return _error = err;
}

void TComm::ClearError()
{
	_error = ALL_OK;
}

uint32 TComm::Checksum(void *pData, WORD len)
{
	int32 checksum = 0;
	unsigned char *bytes = (unsigned char *)pData;
	for (WORD i = 0; i < len; ++i)
		checksum += bytes[i] * (i + 1);
	return checksum;
}

uint16 TComm::Checksum16(void *pData, WORD len)
{
	return (uint16)(Checksum(pData, len) % 0xffff);
}

TNetwork::TNetwork()
{
	SetError(INIT_FAILED);
}

TNetwork::~TNetwork()
{
}

TComm::ERROR TNetwork::Init(long)
{
	return SetError(INIT_FAILED);
}

TComm::ERROR TNetwork::Connect()
{
	return SetError(CONNECTION_FAILED);
}

TComm::ERROR TNetwork::Disconnect()
{
	return SetError(ALL_OK);
}

TComm::ERROR TNetwork::SendPacket(sPacket *, bool)
{
	return SetError(SEND_FAILED);
}

TComm::ERROR TNetwork::ReceivePacket(sPacket *)
{
	return SetError(PACKET_NOT_AVAILABLE);
}

BOOL TNetwork::IsPacketAvailable()
{
	return FALSE;
}

TComm::ERROR TNetwork::DiscardPacket(sPacketHeader *)
{
	return SetError(PACKET_NOT_AVAILABLE);
}

WORD TNetwork::GetUserList(WORD *)
{
	return 0;
}
