#include "Comm.hpp"

static int NoUserAbort(int)
{
	return 0;
}

extern "C" int (*pfnUserAbort)(int) = NoUserAbort;

TSerial::TSerial()
{
	_baud = 19200;
	_port = 1;
	SetError(INIT_FAILED);
}

TSerial::~TSerial()
{
}

TComm::ERROR TSerial::Init(long) { return SetError(INIT_FAILED); }
TComm::ERROR TSerial::Connect() { return SetError(CONNECTION_FAILED); }
TComm::ERROR TSerial::Disconnect() { return SetError(ALL_OK); }

TComm::ERROR TSerial::SetPort(int newPort)
{
	_port = newPort;
	return SetError(ALL_OK);
}

void TSerial::SetBaud(int newBaud) { _baud = newBaud; }
TComm::ERROR TSerial::SendPacket(sPacket *, bool) { return SetError(SEND_FAILED); }
TComm::ERROR TSerial::ReceivePacket(sPacket *) { return SetError(PACKET_NOT_AVAILABLE); }
BOOL TSerial::IsPacketAvailable() { return FALSE; }
TComm::ERROR TSerial::DiscardPacket(sPacketHeader *) { return SetError(PACKET_NOT_AVAILABLE); }
WORD TSerial::GetUserList(WORD *) { return 0; }

TModem::TModem() { _dialMode = TONE; }
TModem::~TModem() { }
TComm::ERROR TModem::Init(long) { return SetError(INIT_FAILED); }
TComm::ERROR TModem::Disconnect() { return SetError(ALL_OK); }
TComm::ERROR TModem::WaitForCall() { return SetError(CONNECTION_FAILED); }
TComm::ERROR TModem::Dial(char *) { return SetError(CONNECTION_FAILED); }
void TModem::HangUp() { SetError(ALL_OK); }
TComm::ERROR TModem::Write(char *) { return SetError(WRITE_FAILED); }
