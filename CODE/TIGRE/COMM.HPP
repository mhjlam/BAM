#ifndef comm_hpp
#define comm_hpp

#include <stddef.h>
#include <time.h>

#include "Tigre.hpp"
#include "Manager.hpp"

// Compatibility types formerly supplied by HMI NetNow.
typedef unsigned int WORD;
typedef unsigned int W32;
typedef int BOOL;

#define _NETNOW_DATA_PACKET 512

enum
{
	_XFER_BLOCK_DATA,
	_XFER_BLOCK_REQUEST_ID,
	_XFER_BLOCK_ID,
	_XFER_BLOCK_REQUEST_RESEND,
	_XFER_BLOCK_NAME,
	_XFER_BLOCK_VOICE,
	_XFER_BLOCK_VOICE_HEADER,
	_XFER_BLOCK_VOICE_END
};

const int tcommMaxPlayers = 2;
const int tcommDefaultTimeout = CLOCKS_PER_SEC * 6;
const int tcommDefaultMaxSendSize = 10 * 1024;
const int tcommBadUserID = -1;

class TCommMgr;

enum _XFER_ADDITIONAL
{
	BLOCK_HEADER = _XFER_BLOCK_VOICE_END + 1,
	BLOCK_DATA,
	BLOCK_END,
	DATA_ACK,
	DATA_NAK,
	CONNECT,
	CONNECT_OK,
	EMPTY_PACKET,
	TOTAL_BLOCK_IDS
};

struct sPacketHeader
{
	WORD wSequence;
	WORD wType;
	WORD ID;
	WORD len;
	WORD destID;
	WORD sendID;
	int32 magicNumber;
	int32 headerChecksum;
	int32 dataChecksum;
};

struct sPacketHeaderTiny
{
	uchar wSequence;
	uchar wType;
	uchar ID;
	uchar len;
	int32 magicNumber;
	uint16 headerChecksum;
	uint16 dataChecksum;
};

#define PACKET_MAGIC_NUMBER 0xACEACE88

struct sMultiBlockInfo
{
	WORD nBlocks;
	WORD nBytes;
	int32 checksum;
};

struct sPacket
{
	sPacketHeader header;
	WORD gl_padding;
	void *pData;
};

class TComm : public Manager
{
	public:
		enum ERROR
		{
			ALL_OK = 0, NULL_CLASS, INIT_FAILED, CONNECTION_FAILED,
			PACKET_FAILED, PACKET_NOT_AVAILABLE, UNKNOWN_PACKET,
			UNEXPECTED_PACKET, SEND_FAILED, BAD_CHECKSUM, TIMEOUT,
			NOT_A_MODEM, TIMEOUT_BUFFER_FULL, TIMEOUT_NO_ACK,
			PREMATURE_ACK, UNKNOWN_ERROR, ERROR_DATA_NAK,
			INPUT_QUEUE_FULL, BAD_PACKET_TYPE, WRITE_FAILED,
			TOTAL_ERROR_TYPES
		};

		TComm();
		virtual ~TComm();

		ERROR GetError();
		ERROR SetError(ERROR err);
		void ClearError();
		int GetTimeout() { return timeout; }
		void SetTimeout(int value) { timeout = value; }

		uint32 Checksum(void *pData, WORD len);
		uint16 Checksum16(void *pData, WORD len);
		WORD GetUserID() { return wConsoleNode; }
		virtual WORD GetUserList(WORD *pList = NULL) = 0;

		bool fDataRequired;
		int32 totalBytesSent;
		int32 totalPacketsSent;

	protected:
		virtual ERROR Init(long optionalArg = 0) = 0;
		virtual ERROR Connect() = 0;
		virtual ERROR Disconnect() = 0;
		virtual ERROR SendPacket(sPacket *pPacket, bool fIsResend = FALSE) = 0;
		virtual ERROR ReceivePacket(sPacket *pPacket) = 0;
		virtual BOOL IsPacketAvailable() = 0;
		virtual ERROR DiscardPacket(sPacketHeader *pHeader = NULL) = 0;
		sPacket *GetLastPacket() { return &lastPacket; }

		BOOL packetAvail;
		sPacket lastPacket;
		WORD wConsoleNode;

	private:
		ERROR _error;
		int timeout;
		friend class TCommMgr;
};

// Network support is deliberately unavailable in the core restoration build.
// Keeping this interface lets the original menus fail gracefully without HMI.
class TNetwork : public TComm
{
	public:
		TNetwork();
		virtual ~TNetwork();
		virtual ERROR Init(long optionalArg = 0);
		virtual ERROR Connect();
		virtual ERROR Disconnect();
		virtual ERROR SendPacket(sPacket *pPacket, bool fIsResend = FALSE);
		virtual ERROR ReceivePacket(sPacket *pPacket);
		virtual BOOL IsPacketAvailable();
		virtual ERROR DiscardPacket(sPacketHeader *pHeader = NULL);
		virtual WORD GetUserList(WORD *pList = NULL);
};

extern char pErrorStrings[][30];
extern char pBlockIDStrings[][30];

#include "Serial.hpp"
#endif
