#include <conio.h>
#include <string.h>
#include <time.h>

#include "types.hpp"
#include "apimem.hpp"		// TRACK_MEM
#include "api.hpp"			// APanic
#include "eventmgr.hpp"		// IsKeyDown
#include "comm.hpp"
#include "mono.hpp"
#include "debug.hpp"

//***********************BUGBUG
#define	BUGPRINT	pMono->Out
//***********************BUGBUG

#define NDEBUG

// not using this junk below anymore
/*
#ifdef USING_NODE_IDS
	//* if using player id's
	#define SEND_ID wConsoleNode 
	#define DEST_ID _enemyId
//	#error not using ids now
#else
	//* else using old style constants
	const int	kPlayerId = 0x12345678;
	const int	kEnemyId =  0x56789abc;
	#define SEND_ID kPlayerId
	#define DEST_ID kEnemyId
#endif
*/

static int  receivedBytes;
static char inputBuffer[(516)+20];
//* bugbug: debugging error check
//static char errorCheckBuffer[] = "check for input buffer overflow";

//* we need this puppy around for the atexit function
static TSerial	*gpTheSerialInstance = NULL;

#pragma off (check_stack)

// function pointer to user abort function (RGW)
extern "C" int (*pfnUserAbort) (int init);

static struct GFModemInfo mi = {
  "Modem Initialization",
  "AT",
  "\r",
  "X1V1E0 S0=0 S7=66",
  "DT",
  "DP",
  "~~~+++~~~ATH0",
  "Z",
  "S%d=%d",
  "S%d?",
  "S0=1",
  "S0=0",
  "A",
  "E1",
  "E0",
  "O",
  "H0",
  "H1",
  "V0",
  "V1",
  {"M0", "M1", "M2"},
  {"X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7"}
};


extern Debugger ComDebug;
//******************************************************
//*

TSerial::TSerial ()
{
	//* the time of construction will function as our unique id.
	//wConsoleNode = clock();
	wConsoleNode = 0;	// negotiation will occur later
	_enemyId = 0;

	_pLink = NULL;
	_pSerial = NULL;
	_pDF = NULL;
	_pLS = NULL;

	_ClearVars ();
	gpTheSerialInstance = this;
	atexit (AtExitFn);
}

TSerial::~TSerial ()
{
	ComDebug.Out("TSerial::~TSerial()\n");

	if (gpTheSerialInstance)
	{
		_Destruct();
	}
}

void TSerial::_Destruct ()
{
	if (gpTheSerialInstance->_pSerial)	delete gpTheSerialInstance->_pSerial;
	if (gpTheSerialInstance->_pLink)		delete gpTheSerialInstance->_pLink;
	
	gpTheSerialInstance = NULL;
	_ClearVars ();
}

void TSerial::AtExitFn ()
{
	if (gpTheSerialInstance)
	{
		gpTheSerialInstance->_Destruct();
	}
}

void TSerial::_ClearVars ()
{
	_baud = 38400;		// User defined
	_port = 0;			// User defined
	_avail = 0;			// Is data available
	_sequence = 1;
}

//*******************************************************************
//*	Set the specified Comm port. The argument passed in is "one-relative",
//*	meaning 1 = COM1, 2 = COM2 etc. The member "_port" is also
//*	one-relative, but Greenleaf actually take a zero-relative arg.
//*
TComm::ERROR TSerial::SetPort (int newPort)
{
	_port = newPort;
	return (GetError ());
}

void
TSerial::SetBaud(int desiredBaud)
{
	_desiredBaud = desiredBaud;
}


TComm::ERROR
TSerial::Init(long)
{
	totalBytesSent = 0;
	totalPacketsSent = 0;
	fDataRequired = FALSE;

	#ifndef NDEBUG
	ComDebug.Out("TSerial::Init()\nPacket overhead == %d bytes, ACK == %d bytes\n",
		sizeof(sPacketHeaderTiny), sizeof(sPacketHeaderTiny));
	#endif

	ClearError();

	//* pass zero relative comm port number
	_pLink = new GFI8250( _port - 1 );
	_pSerial = new GFSerial( _pLink );
	_pDF = _pSerial->GetDataFormat();
	_pLS = _pSerial->GetLineStatus();

	if ( !_pLink || !_pSerial || !_pDF || !_pLS)
	{
		SetError (INIT_FAILED);
	}


	//* should we do this ????
	_pLS->SetHandShake( RTS, CTS );


	// try to set the requested baud rate
	if (_pDF->BaudRate(_desiredBaud) == GCPP_ERROR)
	{
		APanic ("Could not set baud rate");		
	}

	return (GetError ());
}

TComm::ERROR	TSerial::Connect ()
{
	sPacket	received;
	char		pConnectData[128];
	unsigned long	hisRandomNumber, myRandomNumber;
	ERROR		error1;
	int		loopsLeft;

	// If we have not initialized, 
	if (!_pSerial)
	{
		#ifndef NDEBUG
		ComDebug.Out("TSerial::Connect() error - !_pSerial\n");
		#endif
		SetError (CONNECTION_FAILED);
		return (GetError ());
	}

	// Flush the receive queue
	//*jc _pSerial->RXFlush ();

	// Call the user abort function to initialize it
	int contact = 0;			// used to indicate a contact has been made
	
	// Create a connection packet
	sPacket packet;

	memset (&packet, 0, sizeof (packet));
	packet.header.wType = CONNECT;

	// pass our current free mem (just need a unique random number)
	myRandomNumber = pMemMgr->AvailMem() % (rand() % 32767);
	sprintf(pConnectData, "Connecting%d", myRandomNumber);
	packet.pData = pConnectData;
	packet.header.len = strlen(pConnectData);

	loopsLeft = 4;
	// Send it out every second 
	while (!(contact && !loopsLeft))
	{
		// if haven't received confirmation yet
		#ifndef NDEBUG
		ComDebug.Out("\nSending hello");
		#endif
		SendPacket (&packet);
		if(contact)
		{
			// count-down til we go on
			loopsLeft--;
		}

		#ifndef NDEBUG
		ComDebug.Out("[wSeq%d dCRC%d dSize%d].. sent.  Waiting.. ", packet.header.wSequence,
			packet.header.dataChecksum, packet.header.len);
		#endif
		sleep (1); // delay (3 * 8000 * (packet.header.len + sizeof (packet.header)) / _baud);

		// Check to see if a packet has been received
		if (IsPacketAvailable ())
		{
			#ifndef NDEBUG
			ComDebug.Out ("receiving.. ");
			#endif
			received.pData = inputBuffer + sizeof (received.header);
			ReceivePacket(&received);
			error1 = GetError();
			if(error1)
			{
				#ifndef NDEBUG
				ComDebug.Out("err%d[%s]\n", error1, pErrorStrings[error1]);
				#endif
			}
			else
			{
				// If a packet has been received make sure it is a CONNECT packet
				// otherwise, ignore it
				#ifndef NDEBUG
				ComDebug.Out("[wSeq%d dCRC%d dSize%d].. ", received.header.wSequence,
					received.header.dataChecksum, received.header.len);
				#endif
				if (received.header.wType == CONNECT)
				{
					if(contact)
					{
						#ifndef NDEBUG
						ComDebug.Out("redundant CONNECT received\n");
						#endif
					}
					else
					{
						#ifndef NDEBUG
						ComDebug.Out("Contact!\n");
						#endif
						contact = 1;

						// determine who is #0 && who is #1
						hisRandomNumber = atoi((char *)received.pData + 10);
						if(hisRandomNumber > myRandomNumber)
						{
							wConsoleNode = 1;
							_enemyId = 0;
							#ifndef NDEBUG
							ComDebug.Out("Configuring as SLAVE\n");
							#endif
						}
						else if(hisRandomNumber < myRandomNumber)
						{
							wConsoleNode = 0;
							_enemyId = 1;
							#ifndef NDEBUG
							ComDebug.Out("Configuring as MASTER\n");
							#endif
					}
						else
						{
							#ifndef NDEBUG
							ComDebug.Out("Error, failed to establish priority.\n");
							#endif
							contact = 0;	// keep trying
						}
					}
				}
				#ifndef NDEBUG
				else ComDebug.Out("not CONNECT packet\n");
				#endif
			}
		}
		#ifndef NDEBUG
		else ComDebug.Out(" no response received\n");
		#endif

		if (pfnUserAbort (1))
		{
			Disconnect();
			break;
		}
	}

	// if we have not made contact, we need to let the program know
	if (!contact)
	{
		SetError(TComm::CONNECTION_FAILED);
		return (GetError ());
	}

	//* get the sender Id of the connect packet
	_enemyId = received.header.sendID;

	SendPacket (&packet);
	delay (3 * 8000 * (packet.header.len + sizeof (packet.header)) / _baud);

	// Otherwise go on about the business of connecting
	packet.header.wType = CONNECT_OK;
	packet.pData = "Have Connection";
	packet.header.len = strlen ((char*)packet.pData);
	SendPacket (&packet);

	// Finally admit that we have connection
	ClearError ();
	ASSERT (GetError() == TComm::ALL_OK);

	#ifndef NDEBUG
	ComDebug.Out ("end connect!\n");
	#endif
	return (GetError ());
}

TComm::ERROR	TSerial::Disconnect ()
{
	#ifndef NDEBUG
	ComDebug.Out("Disconnecting\n");
	#endif
	return (GetError ());
}

TComm::ERROR
TSerial::SendPacket(sPacket* pPacket, bool fIsResend)
{
	sPacketHeaderTiny	tinyHeader;

	if (_pSerial)
	{
		if(!fIsResend)
		{
			// Assign sequence numbers
			pPacket->header.wSequence = _sequence;
			if(_sequence >= 255)
				_sequence = 1;
			else _sequence++;
		}

		// make the usual modifications to the packet header, even though we
		// dont transmit these bulky packet headers anymore
		pPacket->header.sendID = wConsoleNode;
		pPacket->header.destID = _enemyId;
		pPacket->header.magicNumber = PACKET_MAGIC_NUMBER;
		pPacket->header.dataChecksum = Checksum16(pPacket->pData, pPacket->header.len);
		pPacket->header.headerChecksum = Checksum16(&pPacket->header,
			sizeof(pPacket->header) - (sizeof(pPacket->header.headerChecksum) +
			sizeof(pPacket->header.dataChecksum)));

		tinyHeader.wSequence = (uchar)(pPacket->header.wSequence);
		tinyHeader.wType = (uchar)(pPacket->header.wType);
		tinyHeader.ID = (uchar)(pPacket->header.ID);
		tinyHeader.len = (uchar)(pPacket->header.len);
		tinyHeader.magicNumber = pPacket->header.magicNumber;
		tinyHeader.dataChecksum = (uint16)(pPacket->header.dataChecksum);
		tinyHeader.headerChecksum = Checksum16(&tinyHeader, sizeof(tinyHeader) -
			sizeof(tinyHeader.headerChecksum) - sizeof(tinyHeader.dataChecksum));

//		ComDebug.Out("TSerial::SendPacket() - wType=%d[%s] wSeq=%d magic=%x\n",
//			(int)pPacket->header.wType,
//			(pPacket->header.wType < TOTAL_BLOCK_IDS)? pBlockIDStrings[pPacket->header.wType]: "",
//			(int)pPacket->header.wSequence, pPacket->header.magicNumber);
		ComDebug.Out("SEND[wSeq%d]..", tinyHeader.wSequence);

		//* force synchronicity
		if(!_WaitForTXBufferSpace())
		{
			return(GetError());			
		}

//		_pSerial->PutBuffer ((unsigned char*) &pPacket->header, sizeof (pPacket->header));
		_pSerial->PutBuffer ((unsigned char*) &tinyHeader, sizeof(tinyHeader));
		totalBytesSent += sizeof(tinyHeader);

		if (pPacket->pData && pPacket->header.len)
		{
			_pSerial->PutBuffer ((unsigned char*) pPacket->pData, pPacket->header.len);
			totalBytesSent += pPacket->header.len;
		}
		totalPacketsSent++;
	}
	return (GetError ());
}

TComm::ERROR
TSerial::ReceivePacket (sPacket* pPacket)
{
	int	checksum1;
	sPacketHeaderTiny *pHeader = (sPacketHeaderTiny *)inputBuffer;

	ClearError();
	if (_avail || IsPacketAvailable ())
	{
//		memcpy (&pPacket->header, inputBuffer, sizeof (pPacket->header));
		pPacket->header.wSequence = pHeader->wSequence;
		pPacket->header.wType = pHeader->wType;
		pPacket->header.ID = pHeader->ID;
		pPacket->header.len = pHeader->len;
		pPacket->header.magicNumber = pHeader->magicNumber;
		pPacket->header.headerChecksum = pHeader->headerChecksum;
		pPacket->header.dataChecksum = pHeader->dataChecksum;

		// assume packet header integrity, otherwise we wouldnt have a packet at all

		memcpy (pPacket->pData, inputBuffer + sizeof(*pHeader), pPacket->header.len);
		DiscardPacket (NULL);

		checksum1 = Checksum16(pPacket->pData, pPacket->header.len);
		if(checksum1 != pPacket->header.dataChecksum)
		{
			// checksum failure!
			SetError(BAD_CHECKSUM);
			#ifndef NDEBUG
			ComDebug.Out("TSer::RcvPkt() bad dCRC%d [wSeq%d dCRC%d size%d mag:%08x]\n",
				checksum1, pPacket->header.wSequence, pPacket->header.dataChecksum,
				pPacket->header.len, pPacket->header.magicNumber);
			#endif
		}
//		ComDebug.Out("TSerial::ReceivePacket() - wType=%d[%s] wSeq=%d magic=%x\n",
//			(int)pPacket->header.wType,
//			(pPacket->header.wType < TOTAL_BLOCK_IDS)? pBlockIDStrings[pPacket->header.wType]: "",
//			(int)pPacket->header.wSequence, pPacket->header.magicNumber);
	}
	return (GetError ());
}

inline int TSerial::_CheckValid (sPacketHeaderTiny* pHeader)
{
	bool	fValid;

	// check easy stuff first
	fValid = ((pHeader->magicNumber == PACKET_MAGIC_NUMBER) &&
		(pHeader->len < 512) && (pHeader->wType < TOTAL_ERROR_TYPES))?
		TRUE: FALSE;

	if(fValid)
	{
		// do a checksum on the header
//		fValid = (Checksum16(pHeader, sizeof(*pHeader) - (sizeof(pHeader->dataChecksum)
//			+ sizeof(pHeader->headerChecksum))) == pHeader->headerChecksum)? TRUE: FALSE;
		fValid = (Checksum16(pHeader, sizeof(*pHeader) - (sizeof(pHeader->dataChecksum)
			+ sizeof(pHeader->headerChecksum))) == pHeader->headerChecksum)? TRUE: FALSE;
	}
	return((int)fValid);
}

bool	TSerial::_CheckLineErrors()
{
	if (_pLS)
	{
		int	flag = 0;

		_pLS->Status();
		if (_pLS->BufferOverflow())	flag |= 1;
		if (_pLS->FramingError())		flag |= 2;
		if (_pLS->HardwareOverrun())	flag |= 4;
		if (_pLS->ParityError())		flag |= 8;

		if(flag)
		{
			char	msg[40];
			sprintf (msg, "\nGL LineStatus Error %d\n", flag);

			#ifndef NDEBUG
			ComDebug.Out(msg);
			#endif
			//* bugbug: when debugging only!
//			APanic (msg);

			return TRUE;
		}
	}
	return FALSE;
}



//******************************************************
//*
//*	_WaitForTXBufferSpace verifies that the transmit buffer
//*	has room to take the buffer that we want to send.
//*
bool	TSerial::_WaitForTXBufferSpace()
{
	clock_t		startTime;

	startTime = clock();

	//* loop for up to 10 seconds waiting for TX space
	while (clock() < (startTime + (CLOCKS_PER_SEC * 10)))
	{
		if (_pSerial->TXCount() == 0)
		{
			return TRUE;
		}
	}

	SetError(TIMEOUT_BUFFER_FULL);
	return FALSE;
}

inline void
TSerial::MonoHeader (sPacketHeaderTiny* pHeader)
{
	pHeader = pHeader;
//	#ifndef NDEBUG
//	pMono->Out("wSequence:%d wType:%d(<%d) ID:%d len:%d(<512)\n", (int)(pHeader->wSequence),
//		(int)(pHeader->wType), (int)BAD_PACKET_TYPE, (int)(pHeader->ID), (int)(pHeader->len));
//	pMono->Out("destID:%d(%d) sendID:%d(%d) dataCheck:%ld magic:%08x(%x)\n", (int)(pHeader->destID),
//		(int)wConsoleNode, (int)(pHeader->sendID), (int)_enemyId, (long)(pHeader->dataChecksum),
//		(int)(pHeader->magicNumber), (int)PACKET_MAGIC_NUMBER);
//	#endif
}

BOOL
TSerial::IsPacketAvailable()
{
	int	bytes;
	bool	fResynched = FALSE;

	if (!_avail && _pSerial)
	{
		// Make sure we have enough for a header first
//		if (receivedBytes < sizeof (sPacketHeader))
		if (receivedBytes < sizeof (sPacketHeaderTiny))
		{
			bytes = _pSerial->RXCount();

			//* get the next amount to read which is not any more than
			//* needed to fill the header
//			bytes = (receivedBytes + bytes > sizeof (sPacketHeader)) ? sizeof (sPacketHeader) - receivedBytes : bytes;
			bytes = (receivedBytes + bytes > sizeof (sPacketHeaderTiny)) ? sizeof (sPacketHeaderTiny) - receivedBytes : bytes;

			//* read
			bytes = _pSerial->GetBuffer ((GF_PTR)inputBuffer + receivedBytes, (unsigned) bytes, GF_TIME_OUT);

			//* error check
			if (bytes + receivedBytes > sizeof (inputBuffer))
			{
				#ifndef NDEBUG
				ComDebug.Out ("Too Many bytes %d\n", bytes + receivedBytes);
				#endif
			}

			//* update rec'd count, ignore negative return values
			receivedBytes += bytes > 0 ? bytes : 0;
		}

		// Find out if we have a full packet
//		if (receivedBytes >= sizeof (sPacketHeader))
		if (receivedBytes >= sizeof (sPacketHeaderTiny))
		{
//			sPacketHeader* pHeader = (sPacketHeader*) inputBuffer;
			sPacketHeaderTiny* pHeader = (sPacketHeaderTiny*) inputBuffer;
			// BUGBUG Check to see if the packet is already corrupt
			if (!_CheckValid (pHeader))
			{
				#ifndef NDEBUG
				ComDebug.Out("TSerial::IsPacketAvail() - invalid header, magic:%08x\n",
					pHeader->magicNumber);

				//* In a perfect world, a valid header should always be found
				// at inputBuffer[0] if enough bytes have already been read.
				// So the data stream has been damaged

				ComDebug.Out("Input:bad header(wSeq%d magic%x)..",
					pHeader->wSequence, (int)pHeader->magicNumber);
				#endif
//				MonoHeader (pHeader);

				// At this point, we dont know if the lost packet was an ACK/NAK
				// or a data packet.  If the former, then it will have been sent
				// twice so let's hope the other copy makes it through okay.
				// In case we lost a data packet, we should respond with a generic
				// DATA_NAK to cause a resend.
//				char		pDummySpace[4];
				sPacket nak;
				nak.header.wType = DATA_NAK;
//				nak.header.len = sizeof(pDummySpace);
				nak.header.len = 0;
//				nak.pData = pDummySpace;
				nak.pData = NULL;
				nak.header.sendID = 0;	// like it matters
				nak.header.destID = 1;	// like it matters
				nak.header.ID = 0;	// don't know wSeq of lost packet
				SendPacket(&nak);
				#ifndef NDEBUG
				ComDebug.Out("DATA_NAKwSeq%d sent\n", nak.header.wSequence);
				#endif
			
				// if the port shows any errors, display them
				_CheckLineErrors();
				
				//* read up to capacity of inputbuffer
				bytes = sizeof (inputBuffer) - receivedBytes;
				bytes = _pSerial->GetBuffer ((GF_PTR) inputBuffer + receivedBytes, (unsigned) bytes, 1);
//				GF_TIME_OUT (3 secs) may be too long
//				bytes = _pSerial->GetBuffer ((GF_PTR) inputBuffer + receivedBytes, (unsigned) bytes, GF_TIME_OUT);

				//* update rec'd count
				receivedBytes += (bytes > 0)? bytes : 0;

				//* advance thru inputbuffer, trying to find the next valid packet
				// header in the stream
				fResynched = FALSE;
				for(int i = 1; i < receivedBytes - sizeof (*pHeader); i++)
				{
//					if (_CheckValid((sPacketHeader*)(inputBuffer + i)))
					if (_CheckValid((sPacketHeaderTiny*)(inputBuffer + i)))
					{
						#ifndef NDEBUG
						ComDebug.Out("ReSynch@ %d of %d bytes, ", i, receivedBytes);
						#endif
						// appears to have re-synch, move new header data and any
						// trailing data to front of input buffer, discarding any
						// data in the stream ahead of it as corrupt
						memmove(inputBuffer, inputBuffer + i, receivedBytes - i);
						receivedBytes -= i;
						fResynched = TRUE;
						break;
					}
				}
				
				if(fResynched)
				{
//					MonoHeader((sPacketHeader*) inputBuffer);
//					MonoHeader((sPacketHeaderTiny*) inputBuffer);
				}
				else
				{
					#ifndef NDEBUG
					ComDebug.Out("TSerial::IsPacketAvail() - cannot resynch in %d bytes\n",
						receivedBytes);
					#endif
					return(FALSE);
				}

//				ComDebug.Out("How can we know if _avail is accurate?");
//				return (_avail);	//* Why was he returning?! -Van
			}

			#ifndef NDEBUG
			ComDebug.Out("IsPktAvail() [wSeq%d dCRC%d size%d magic:%08x] ",
				pHeader->wSequence, pHeader->dataChecksum, pHeader->len,
				pHeader->magicNumber);
			#endif

			// We should have a valid header at this point.  Now to retrieve
			// rest of packet.

			//* get the number of bytes we still need to read
			bytes = pHeader->len + sizeof (*pHeader) - receivedBytes;

			//* read
			bytes = _pSerial->GetBuffer ((GF_PTR) inputBuffer + receivedBytes, (unsigned) bytes, GF_TIME_OUT);

			//* update rec'd count
			receivedBytes += bytes > 0 ? bytes : 0;

			//
//			_avail = receivedBytes >= (pHeader->len + sizeof (sPacketHeader));
			_avail = receivedBytes >= (pHeader->len + sizeof (sPacketHeaderTiny));

			//	if ((3 * (receivedBytes - sizeof (*pHeader)) / 4 - pHeader->len) > 0)
			//	ComDebug.Out ("<Type %d %3d of %3d\n", pHeader->wType, receivedBytes - sizeof (*pHeader), pHeader->len);
			
			if (_avail)
			{
				#ifndef NDEBUG
				ComDebug.Out("OK\n");
				#endif
//				memcpy (&lastPacket, pHeader, sizeof (*pHeader));

				// translate compressed header to normal
				lastPacket.header.wSequence = (uchar)(pHeader->wSequence);
				lastPacket.header.wType = (uchar)(pHeader->wType);
				lastPacket.header.ID = (uchar)(pHeader->ID);
				lastPacket.header.len = (uchar)(pHeader->len);
				lastPacket.header.magicNumber = pHeader->magicNumber;

				lastPacket.pData = pHeader + 1;
			}
			else
			{
				#ifndef NDEBUG
				ComDebug.Out("inc\n");
				#endif
			}
		}
	}


//#ifndef NDEBUG
//	//* bugbug: debugging error check
//	if (strcmp (errorCheckBuffer, "check for input buffer overflow"))
//	{
//		APanic("IsPacketAvail()::input buffer overflowed!");
//	}
//#endif

	return (_avail);
}

//TComm::ERROR	TSerial::DiscardPacket (sPacketHeaderTiny *)
TComm::ERROR	TSerial::DiscardPacket (sPacketHeader*)
{
//	sPacketHeader* pHeader = (sPacketHeader*) inputBuffer;
	sPacketHeaderTiny* pHeader = (sPacketHeaderTiny *) inputBuffer;
	
	// Adjust data available for data being thrown away
//	int packetSize = pHeader->len + sizeof (sPacketHeader);
	int packetSize = pHeader->len + sizeof (*pHeader);
	receivedBytes -= packetSize;

	// Make sure we are not throwing away more than we have
   receivedBytes = receivedBytes < 0 ? 0 : receivedBytes;

	// Move the data down in the buffer
	memmove (inputBuffer, inputBuffer + packetSize, receivedBytes);


	// Check to see if data is available, we need to do this often anyway!
	_avail = 0; // Must clear to indicate that we don't want what we have (like everyone)
	IsPacketAvailable ();
	return (GetError ());
}


//*
//* return id of all OTHER connections
//*
WORD	TSerial::GetUserList (WORD* pList)
{
	if (pList)
	{
		*pList = _enemyId;
	}
	return (1);
}


//*************************************************************
//*	TModem adds to TSerial

//******************************************************
//*

TModem::TModem()
{
	_dialMode = TONE;
	_pLink = NULL;
	_pSerial = NULL;
	_pMdm = NULL;
	_fHangupCalled = TRUE;
}

TComm::ERROR	TModem::Init( long )
{
	fDataRequired = FALSE;

	// pass zero-relative port number
	_pLink = new GFI8250( _port - 1 );
	_pSerial = new GFSerial( _pLink );
	_pMdm = new GFModem();
	_pDF = _pSerial->GetDataFormat();
	_pLS = _pSerial->GetLineStatus();

	if ( !_pLink || !_pSerial || !_pDF || !_pLS)
	{
		SetError (INIT_FAILED);
		return GetError();
	}

	_pMdm->WaitForOk( 800, "OK" );
	_pMdm->Install( _pSerial, &mi );
	_pMdm->SetVerbalResponse();
	//_pMdm->EchoOn();
	//*jjc	_pMdm->SetAbortKey( ALTX );
	//*jjc	cout << "Setup complete\n" << flush;


	//* should we do this ????
	_pLS->SetHandShake( RTS, CTS );

	return ALL_OK;
}

TModem::~TModem( void )
{
	HangUp();

	if(_pMdm)
		delete( _pMdm );
	_pMdm = NULL;

}

TComm::ERROR	TModem::Disconnect()
{
	HangUp();
	#ifndef NDEBUG
	ComDebug.Out("\n");
	#endif
	return TSerial::Disconnect();
}

void	TModem::HangUp()
{
	#ifndef NDEBUG
	ComDebug.Out("Modem hangup\n");
	#endif
	if(!_fHangupCalled)
   	_pMdm->HangUp();
}


TComm::ERROR	TModem::WaitForCall()
{
	#ifndef NDEBUG
	ComDebug.Out("Waiting for call\n");
	#endif
	pfnUserAbort (0); // We are reinitializing user abort
	ClearError();
	for(;;) 
	{
		if( _pMdm->GetModemInput( _modemInput, 81, 1000 ) == GCPP_USER_ABORT )
		{
			_baud = 0;
			break;
		}

		#ifndef NDEBUG
		ComDebug.Out("%s\n",_modemInput);
		#endif
		if (pfnUserAbort (1))
		{
			_baud = 0;
			break;
		}

		// if ringing attempt to answer
		if( strcmp( _modemInput, "RING" ) == 0 ) 
		{
			#ifndef NDEBUG
			ComDebug.Out("The phone is ringing lets answer it now.\n");
			#endif
			_baud = _Answer();
			break;
		}
	}

	if (!_baud)
	{
		#ifndef NDEBUG
		ComDebug.Out("Connection Failed.\n");
		#endif
		Disconnect(); //in case we've already answered the phone before fail
		SetError (CONNECTION_FAILED);
	}

	#ifndef NDEBUG
	ComDebug.Out("We've connected!\n");
	#endif
	return GetError();
}

TComm::ERROR	TModem::Dial(char* szPhoneNumber)
{
  	long msecs;

	// bugbug: Tigre require a touch tone phone!
	_pMdm->DialMode( _dialMode );

	#ifndef NDEBUG
	ComDebug.Out("Dial out\n");
	#endif
	_pMdm->Dial( szPhoneNumber );
	_fHangupCalled = FALSE;

	// assume we will time out first
	SetError(CONNECTION_FAILED);
	pfnUserAbort (0); // We are reinitializing user abort

	// loop until connect, or until abort key hit
	//	give it 50 seconds to connect
	msecs = 50 * 1000L;
	while( msecs > 0 ) 
	{
		// msecs counts down as we go
		// don't come back until we get input or timeout
   	if(_pMdm->GetModemInput( _modemInput, 81, 1000 ))
		{
			#ifndef NDEBUG
			ComDebug.Out("%s\n",_modemInput);
			#endif
		}
		msecs -= 1000L;

		// check for successfull connection
   	if (strstr( _modemInput, "CONNECT" ) != 0)
		{
			#ifndef NDEBUG
			ComDebug.Out("Received CONNECT from modem\n");
			#endif
			ClearError();
   		return GetError();
   	}

		if (pfnUserAbort (1))
		{
			#ifndef NDEBUG
			ComDebug.Out("User aborted dial out.\n");
			#endif
			Disconnect();
   		return GetError();
		}

		// check for error -not sure modem will even return this -but just in case
   	if (strstr( _modemInput, "NO DIALTONE" ) != 0)
		{
			#ifndef NDEBUG
			ComDebug.Out("Received NO DIALTONE from modem\n");
   		#endif
			return GetError();
   	}

	}

	#ifndef NDEBUG
	ComDebug.Out("Timed out waiting for CONNECT from modem\n");
	#endif
	Disconnect();
	return GetError();
}

long TModem::_Answer()
{
	long msecs = 30000L;

	//*jjc	cout << "Answering phone\n";
	#ifndef NDEBUG
	ComDebug.Out("Answering phone\n");
	#endif
	_pMdm->AnswerPhone();
	_fHangupCalled = FALSE;
	while( msecs > 0 )
	{
		if(_pMdm->GetModemInput( _modemInput, 81, 1000))
		{
			#ifndef NDEBUG
			ComDebug.Out("%s\n",_modemInput);
			#endif
		}
		msecs -= 1000L;

		if( strstr( _modemInput, "CONNECT 9600" ) != 0 )
			 _baud = 9600L;
		if( strstr( _modemInput, "CONNECT 4800" ) != 0 )
			 _baud = 4800L;
		if( strstr( _modemInput, "CONNECT 2400" ) != 0 )
			 _baud = 2400L;
		if( strstr( _modemInput, "CONNECT 1200" ) != 0 )
			 _baud = 1200L;
		if( strstr( _modemInput, "NO CARRIER" ) != 0 )
			msecs = _baud = 0;

		if (pfnUserAbort (1))
			msecs = _baud = 0;
	}

	if( _baud > 0)
	{
		#ifndef NDEBUG
		ComDebug.Out("Baud rate detected as %d\n",_baud);
		#endif
		_pDF->BaudRate( _baud );
	}
	else
	{
		#ifndef NDEBUG
		ComDebug.Out("Failed to Connect\n");
		#endif
		return( 0 );
	}
	return( _baud );
}


TComm::ERROR	TModem::Write(char* szWriteStr)
{
  	long msecs;

	// assume we will time out first
	SetError(WRITE_FAILED);
	*_modemInput = '\0';

	_pMdm->Write(szWriteStr);

	//	give it 2 seconds to connect
	msecs = 2 * 1000L;
	while( msecs > 0 ) 
	{
		// msecs counts down as we go
   	msecs = _pMdm->GetModemInput( _modemInput, 81, msecs );

		//if(msecs)
		#ifndef NDEBUG
		ComDebug.Out("%s\n",_modemInput);
		#endif

		// check for successfull connection
   	if (strstr( _modemInput, "OK" ) != 0)
		{
			#ifndef NDEBUG
			ComDebug.Out("TModem::Write() -modem returned OK\n");
			#endif
			ClearError();
   		return GetError();
   	}
	}

	return GetError();
}


