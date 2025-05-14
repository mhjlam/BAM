#ifndef comm_hpp
#define comm_hpp

#include	<stddef.h>
#include	<time.h>

#include "tigre.hpp"
#include "manager.hpp"

// Includes for HMI NetNow! system
extern "C"
{
#include "hmistd.h"
#include "netnow.h"
};

const tcommMaxPlayers = 2;
const tcommDefaultTimeout = CLOCKS_PER_SEC * 6;
const tcommDefaultMaxSendSize  = 10 * 1024;
const tcommBadUserID = -1;


class TCommMgr;

// note: BLOCK_HEADER == 8
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
	WORD		wSequence;		// low level info
	WORD		wType;			// Engine level info
	WORD		ID;				// User defineable
	WORD		len;				// Size of data to be sent
	WORD		destID; 			// Player to send packet to, -1 for all
	WORD		sendID; 			// Player who sent packet, autoset
	int32		magicNumber;	// 0xACEACE88, used for re-sync'ing on single byte IO streams

	// the following two members MUST come after all others, so that the above
	// elements can be checksum'ed as a single block
	int32		headerChecksum, dataChecksum;		// Engine level info
};

struct sPacketHeaderTiny
{
	uchar		wSequence;		// packet serial #
	uchar		wType;			// engine-level packet type
	uchar		ID;				// engine-level packet type aux info
	uchar		len;				// len of attached data (if any)
	int32		magicNumber;	// data stream sync marker

	uint16		headerChecksum, dataChecksum;	// self-explanatory
};

#define PACKET_MAGIC_NUMBER	0xACEACE88
struct sMultiBlockInfo
{
	WORD	nBlocks;
	WORD	nBytes;
	int32	checksum;
};

struct sPacket
{
	sPacketHeader	header;
	WORD				gl_padding;	// provide space for Greenleaf's GetBuffer adding null terminator
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

		uint32				Checksum(void *pData, WORD len);
		uint16				Checksum16(void *pData, WORD len);

		// Information
					WORD		GetUserID () { return (wConsoleNode); }
		virtual	WORD		GetUserList (WORD* pList = NULL) = 0;

		// debug stuff
//		FILE					*pSerialDebug;
		bool					fDataRequired;	// must a packet include data?
		int32					totalBytesSent, totalPacketsSent;

	protected :
		// Initialization/Shutdown
		virtual	ERROR		Init (long optionalArg = 0) = NULL;
		virtual	ERROR		Connect () = NULL;
		virtual	ERROR		Disconnect () = NULL;

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket, bool fIsResend = FALSE) = NULL;
		virtual	ERROR		ReceivePacket (sPacket* pPacket) = NULL;
		virtual	BOOL		IsPacketAvailable () = NULL;
		virtual	ERROR		DiscardPacket (sPacketHeader* pHeader = NULL) = NULL;

		// Data Access
		sPacket* GetLastPacket () { return (&lastPacket); }

	protected :
		BOOL			packetAvail;
		sPacket		lastPacket;		// last header used for receive
		WORD			wConsoleNode;

	private :
		ERROR		_error;
		int		timeout;			// Number of milliseconds to wait before timeout


		friend	class TCommMgr;
};

class TNetwork : public TComm
{
	public :
					TNetwork ();
		virtual ~TNetwork ();

		// Initialization
		virtual	ERROR		Init (long optionalArg = 0);
		virtual	ERROR		Connect ();
		virtual	ERROR		Disconnect ();

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket, bool fIsResend = FALSE);
		virtual	ERROR		ReceivePacket (sPacket* pPacket);
		virtual	BOOL		IsPacketAvailable ();
		virtual	ERROR		DiscardPacket (sPacketHeader* pPacketHeader = NULL);

		// Information
		virtual	WORD		GetUserList (WORD* pList = NULL);

	private :
		static	void	AtExitFn();
		static	int	init, isKilled;
      static	W32	wNETNodes;              // number of nodes to locate
      static	W32	wNETSocket;             // socket for IPX
};

extern char pErrorStrings[][30];
extern char pBlockIDStrings[][30];
#include "serial.hpp"
#endif

