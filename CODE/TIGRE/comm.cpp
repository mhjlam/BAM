/*
 * comm.cpp — TComm base class implementation.
 *
 * TNetwork (IPX via HMI NetNow), TSerial and TModem (Greenleaf serial/modem)
 * have been removed as part of the Linux port cleanup.  Only the protocol-
 * agnostic TComm base class lives here; add a TCP/IP subclass when needed.
 */

#include <string.h>
#include "comm.hpp"
#include "commmgr.hpp"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// ---------------------------------------------------------------------------
// TComm base class
// ---------------------------------------------------------------------------

TComm::ERROR TComm::GetError()
{
	return (_error);
}

TComm::ERROR TComm::SetError(ERROR err)
{
	return (_error = err);
}

void TComm::ClearError()
{
	_error = ALL_OK;
}

TComm::TComm()
{
	fDataRequired = false;
	_error        = ALL_OK;
	timeout       = tcommDefaultTimeout;
	wConsoleNode  = tcommBadUserID;
	packetAvail   = 0;
	memset(&lastPacket, 0, sizeof(lastPacket));
}

TComm::~TComm()
{
}

uint32 TComm::Checksum(void *pData, uint16 len)
{
	int32          checksum = 0;
	unsigned char *pTemp    = (unsigned char *)pData;
	for (int i = 0; i < len; i++)
		checksum += pTemp[i] * (i + 1);
	return (checksum);
}

uint16 TComm::Checksum16(void *pData, uint16 len)
{
	return ((uint16)(Checksum(pData, len) % 0xFFFF));
}
