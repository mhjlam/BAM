#ifndef comm_hpp
#define comm_hpp

#include	<stddef.h>
#include "tigre.hpp"
#include "manager.hpp"

const int tcommMaxPlayers = 2;
const int tcommDefaultTimeout = 6000;		/* ms */
const int tcommDefaultMaxSendSize  = 10 * 1024;
const int tcommBadUserID = -1;


class TCommMgr;

// Transport-layer packet types (originally from hmixfer.h).
// commmgr.cpp uses _XFER_BLOCK_DATA and _XFER_BLOCK_ID for internal classification.
enum
{
	_XFER_BLOCK_DATA,             // block of raw data          (= 0)
	_XFER_BLOCK_REQUEST_ID,       // request ID direct          (= 1)
	_XFER_BLOCK_ID,               // contains local target data (= 2)
	_XFER_BLOCK_REQUEST_RESEND,   // request resend of block    (= 3)
	_XFER_BLOCK_NAME,             // user name                  (= 4)
	_XFER_BLOCK_VOICE,            // voice data                 (= 5)
	_XFER_BLOCK_VOICE_HEADER,     // voice header               (= 6)
	_XFER_BLOCK_VOICE_END,        // end of voice chunk         (= 7)
};

// Engine/application-level packet types that extend the above (start at 8).
// note: BLOCK_HEADER == 8
enum _XFER_ADDITIONAL
{
	BLOCK_HEADER = 8,
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
	uint16		wSequence;		// low level info
	uint16		wType;			// Engine level info
	uint16		ID;				// User defineable
	uint16		len;				// Size of data to be sent
	uint16		destID; 			// Player to send packet to, -1 for all
	uint16		sendID; 			// Player who sent packet, autoset
	int32		magicNumber;	// 0xACEACE88, used for re-sync'ing on single byte IO streams

	// the following two members MUST come after all others, so that the above
	// elements can be checksum'ed as a single block
	int32		headerChecksum, dataChecksum;		// Engine level info
};

#define PACKET_MAGIC_NUMBER	0xACEACE88
struct sMultiBlockInfo
{
	uint16	nBlocks;
	uint16	nBytes;
	int32	checksum;
};

struct sPacket
{
	sPacketHeader	header;
	uint16				gl_padding;	// provide space for Greenleaf's GetBuffer adding null terminator
	void*				pData;
};

class TComm : public Manager
{
	public :
		enum	ERROR 
		{ 
			ALL_OK = 0,
			NULL_CLASS, 
			INIT_FAILED, 
			CONNECTION_FAILED, 
			PACKET_FAILED, 
			PACKET_NOT_AVAILABLE, 
			UNKNOWN_PACKET, 
			UNEXPECTED_PACKET, 
			SEND_FAILED,
			BAD_CHECKSUM,
			TIMEOUT,
			NOT_A_MODEM,
			TIMEOUT_BUFFER_FULL,
			TIMEOUT_NO_ACK,
			PREMATURE_ACK,
			UNKNOWN_ERROR,
			ERROR_DATA_NAK,
			INPUT_QUEUE_FULL,
			BAD_PACKET_TYPE,
			WRITE_FAILED,
			TOTAL_ERROR_TYPES
		};

					TComm   ();
		virtual	~TComm  ();


		// Data Access
		ERROR		GetError   ();
		ERROR		SetError   (ERROR err);
		void		ClearError ();
		int		GetTimeout ()				{ return (timeout); }
		void		SetTimeout (int t)		{ timeout = t; }

		uint32				Checksum(void *pData, uint16 len);
		uint16				Checksum16(void *pData, uint16 len);

		// Information
					uint16		GetUserID () { return (wConsoleNode); }
		virtual	uint16		GetUserList (uint16* pList = nullptr) = 0;

		// debug stuff
//		FILE					*pSerialDebug;
		bool					fDataRequired;	// must a packet include data?
		int32					totalBytesSent, totalPacketsSent;

	protected :
		// Initialization/Shutdown
		virtual	ERROR		Init (long optionalArg = 0) = 0;
		virtual	ERROR		Connect () = 0;
		virtual	ERROR		Disconnect () = 0;

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket, bool fIsResend = false) = 0;
		virtual	ERROR		ReceivePacket (sPacket* pPacket) = 0;
		virtual	bool		IsPacketAvailable () = 0;
		virtual	ERROR		DiscardPacket (sPacketHeader* pHeader = nullptr) = 0;

		// Data Access
		sPacket* GetLastPacket () { return (&lastPacket); }

	protected :
		bool			packetAvail;
		sPacket		lastPacket;		// last header used for receive
		uint16			wConsoleNode;

	private :
		ERROR		_error;
		int		timeout;			// Number of milliseconds to wait before timeout


		friend	class TCommMgr;
};

extern char pErrorStrings[][30];
extern char pBlockIDStrings[][30];
#endif

