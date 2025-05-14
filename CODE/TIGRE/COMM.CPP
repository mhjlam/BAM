#include <stdarg.h>
#include "comm.hpp"
#include "commmgr.hpp"
#include <string.h>
#include "mono.hpp"

#include <assert.h>
#ifndef ASSERT
#define ASSERT assert
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

//#define NDEBUG
int TNetwork::isKilled, TNetwork::init;
W32     TNetwork::wNETSocket;
W32     TNetwork::wNETNodes;

extern "C"
{
BOOL	hmiVCHATInit();
}


//****************************************************
//* These were inline

TComm::ERROR	TComm::GetError   ()
{
	return (_error); 
}

TComm::ERROR	TComm::SetError(ERROR err)
{ 
	return (_error = err); 
}

void				TComm::ClearError ()
{
	_error = ALL_OK; 
}
//****************************************************



TComm::TComm ()
{
	fDataRequired = FALSE;
	_error   = ALL_OK;
	timeout = tcommDefaultTimeout;
	wConsoleNode = tcommBadUserID;
	packetAvail = 0;
	memset (&lastPacket, 0, sizeof (lastPacket));

//	pSerialDebug = fopen("serial.dbg", "w");
}

virtual
TComm::~TComm()
{
//	fclose(pSerialDebug);
}

TNetwork::TNetwork ()
{
        init = 0;
        isKilled = 0;
        wNETSocket = 0x4545;
        wNETNodes = 2;
}

TNetwork::~TNetwork ()
{
	AtExitFn ();
}

void TNetwork::AtExitFn ()
{
	// if driver was initialized && not yet shut down
	if (init && !isKilled)
	{
		hmiNETNOWUnInitSystem ();
		isKilled = 1;
	}
}

TComm::ERROR
TNetwork::Init (long)
{
	#ifndef NDEBUG
	pMono->Out("TNetwork::Init().. ");
	#endif
	totalBytesSent = 0;
	totalPacketsSent = 0;
	if (!init)
	{
//		#ifndef NDEBUG
//		pMono->Out("hmiVCHATInit().. ");
//		#endif
//		hmiVCHATInit ();
		#ifndef NDEBUG
		pMono->Out("hmiNETNOWInitSystem(%d).. ", tcommMaxPlayers);
		#endif
   	if  (hmiNETNOWInitSystem (tcommMaxPlayers) != _NETNOW_NO_ERROR)
		{
			#ifndef NDEBUG
			pMono->Out("error!\n");
			#endif
			SetError (INIT_FAILED);
		}
		init = 1;
		atexit (AtExitFn);
		fDataRequired = TRUE;	// compensate for NetNOW bug - Van
	}

	#ifndef NDEBUG
	pMono->Out("done\n");
	#endif
	return (GetError ());
}

TComm::ERROR
TNetwork::Connect ()
{
	#ifndef NDEBUG
	pMono->Out("TNetwork::Connect()\n");
	#endif

	if (!hmiNETNOWFindNodes (2))
	{
		SetError (CONNECTION_FAILED);
	}
	wConsoleNode = hmiNETNOWGetConsoleNode();
	#ifndef NDEBUG
	pMono->Out("TNetwork::Connect() done\n");
	#endif
	return (GetError ());
}

BOOL
TNetwork::IsPacketAvailable ()
{
	if (!packetAvail)
	{
		packetAvail = hmiNETNOWGetHeader ((PSTR) &lastPacket, sizeof (lastPacket.header), (PSTR*) &lastPacket.pData);
		if (packetAvail && lastPacket.header.wType == _XFER_BLOCK_ID)
		{
			packetAvail = 0;
			DiscardPacket ();
			return (IsPacketAvailable ());
		}
	}
	ClearError ();
	return (packetAvail);
}

TComm::ERROR
TNetwork::SendPacket (sPacket* pPacket, bool fIsResend)
{

	if(fIsResend);

	//	pMono->Out ("%d packet sent\n", pPacket->header.wType);
	// We don't have any errors yet
	ClearError ();

	// Make one attempt to send the data without checking the time
	totalBytesSent += sizeof(pPacket->header);
	totalPacketsSent++;
	if (hmiNETNOWSendData ((PSTR)pPacket, sizeof (pPacket->header),
		(PSTR) pPacket->pData, pPacket->header.len, pPacket->header.destID))
	{
		return (GetError ());
	}

// Since our first attempt failed, we need to be sure we do not timeout

	// Get the current number of clock ticks
	int start = clock();

	// Determine the number of clock ticks to timeout
	clock_t	end = start + GetTimeout ();

	// BUGBUG there may be a problem with wraparound at 12:00

	// Loop until the message is sent, or we timeout
	while ((totalBytesSent += pPacket->header.len && ++totalPacketsSent) &&
		!hmiNETNOWSendData ((PSTR)pPacket, sizeof (pPacket->header),
		(PSTR) pPacket->pData, pPacket->header.len, pPacket->header.destID))
	{
		clock_t	ticks = clock();
		if (ticks > end)
		{
			SetError (SEND_FAILED);
			break;
		}
	}
	return (GetError ());
}

TComm::ERROR
TNetwork::ReceivePacket (sPacket* pPacket)
{
	ASSERT (pPacket);

	if (!packetAvail)
	{
		return (SetError (PACKET_NOT_AVAILABLE));
	}
	 
	ClearError ();

	memcpy (&pPacket->header, &lastPacket.header, sizeof (lastPacket.header));
	memcpy (pPacket->pData, lastPacket.pData, pPacket->header.len);
	DiscardPacket (&pPacket->header);
//	pMono->Out ("Packet %d rec\n", pPacket->header.wType);

	return (GetError ());
}

TComm::ERROR
TNetwork::Disconnect ()
{
	return (GetError ());
}

WORD
TNetwork::GetUserList (WORD* pList)
{
	int users = hmiNETNOWGetActiveNodes ();
	int me = GetUserID ();

	if (pList)
	{
		for (int i=0;i<users;i++)
		{
			if (i != me)
			{
				*pList = i;
				pList++;
			}
		}
	}

	return (users-1);
}

TComm::ERROR TNetwork::DiscardPacket (sPacketHeader*)
{
	packetAvail = 0;

	hmiNETNOWPostListen ();
	return (GetError ());
}

uint32
TComm::Checksum(void *pData, WORD len)
{
	int32		checksum = 0;
	unsigned char*	pTemp = (unsigned char*) pData;

	for (int i=0;i<len;i++)
	{
		checksum += pTemp[i] * (i + 1);
	}
	return (checksum);
}

uint16
TComm::Checksum16(void *pData, WORD len)
{
	return((uint16)(Checksum(pData, len) % 0xFFFF));
}
