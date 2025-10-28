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
#include "vdata.h"
#include "vchat.h"
};

const tcommMaxPlayers = 8;
const tcommDefaultTimeout = CLOCKS_PER_SEC * 4;
const tcommDefaultMaxSendSize  = 10 * 1024;
const tcommBadUserID = -1;


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
	BAD_PACKET_TYPE
};

struct sPacketHeader
{
	WORD		wSequence;		// low level info
	WORD		wType;			// Engine level info
	WORD		ID;				// User defineable
	WORD		len;				// Size of data to be sent
	WORD		destID; 			// Player to send packet to, -1 for all
	WORD		sendID; 			// Player who sent packet, autoset
	WORD		checksum;		// Engine level info
};

struct sMultiBlockInfo
{
	WORD	nBlocks;
	WORD	nBytes;
	WORD	checksum;
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
			ALL_OK, 
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
			UNKNOWN_ERROR
		};

					TComm   ();
		virtual 	~TComm  () {}


		// Data Access
		ERROR		GetError   ();
		ERROR		SetError   (ERROR err);
		void		ClearError ();
		int		GetTimeout ()				{ return (timeout); }
		void		SetTimeout (int t)		{ timeout = t; }


		// Information
					WORD		GetUserID () { return (wConsoleNode); }
		virtual	WORD		GetUserList (WORD* pList = NULL) = 0;

	protected :
		// Initialization/Shutdown
		virtual	ERROR		Init (long optionalArg = 0) = NULL;
		virtual	ERROR		Connect () = NULL;
		virtual	ERROR		Disconnect () = NULL;

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket) = NULL;
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
					TNetwork () { init = 0; };
		virtual ~TNetwork ();

		// Initialization
		virtual	ERROR		Init (long optionalArg = 0);
		virtual	ERROR		Connect ();
		virtual	ERROR		Disconnect ();

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket);
		virtual	ERROR		ReceivePacket (sPacket* pPacket);
		virtual	BOOL		IsPacketAvailable ();
		virtual	ERROR		DiscardPacket (sPacketHeader* pPacketHeader = NULL);

		// Information
		virtual	WORD		GetUserList (WORD* pList = NULL);

	private :
		static	void		AtExitFn ();

	private :
					int	init;
		static	int	isKilled;
};

// Support function
WORD Checksum (void *pData, WORD len);

#include "serial.hpp"
#endif

