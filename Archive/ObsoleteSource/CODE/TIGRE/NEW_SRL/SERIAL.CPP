#include <dos.h>
#include <conio.h>
#include <string.h>
#include <time.h>

#include "types.hpp"
#include "apimem.hpp"		// TRACK_MEM
#include "api.hpp"			// APanic
#include "eventmgr.hpp"		// IsKeyDown
#include "comm.hpp"
#include "mono.hpp"

//***********************BUGBUG
#define	BUGPRINT	pMono->Out
//***********************BUGBUG


#ifdef USING_NODE_IDS
	//* if using player id's
	#define SEND_ID wConsoleNode 
	#define DEST_ID _enemyId
	#error not using ids now
#else
	//* else using old style constants
	const int	kPlayerId = 0x12345678;
	const int	kEnemyId =  0x56789abc;
	#define SEND_ID kPlayerId
	#define DEST_ID kEnemyId
#endif


static int  receivedBytes;
static char inputBuffer[(516)+20];
//* bugbug: debugging error check
static char errorCheckBuffer[] = "check for input buffer overflow";

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


//******************************************************
//*

TSerial::TSerial ()
{
	//* the time of construction will function as our unique id.
	wConsoleNode = clock();
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
	_sequence = 0;
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

void	TSerial::SetBaud (int desiredBaud)
{
	_desiredBaud = desiredBaud;
}


TComm::ERROR TSerial::Init (long)
{
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
	sPacket received;

	// If we have not initialized, 
	if (!_pSerial)
	{
		SetError (CONNECTION_FAILED);
		return (GetError ());
	}

	// Flush the receive queue
	//*jc _pSerial->RXFlush ();

	// Assume the connection fails unless told otherwise
	SetError (CONNECTION_FAILED);

	// Call the user abort function to initialize it
	int userAborted = 0;
	int contact = 0;			// used to indicate a contact has been made
	
	// Create a connection packet
	sPacket packet;

	memset (&packet, 0, sizeof (packet));
	packet.header.wType = CONNECT;
	packet.pData = "Connecting";
	packet.header.len = strlen ((char*)packet.pData);

	// Send it out every second 
	while (!contact && !userAborted)
	{
		SendPacket (&packet);
		sleep (1); // delay (3 * 8000 * (packet.header.len + sizeof (packet.header)) / _baud);

		// Check to see if a packet has been received
		if (IsPacketAvailable ())
		{
			received.pData = inputBuffer + sizeof (received.header);
			ReceivePacket (&received);
//			pMono->Out ("Received packet\n");

			// If a packet has been received make sure it is a CONNECT packet
			// otherwise, ignore it
			if (received.header.wType == CONNECT)
			{
				pMono->Out ("Contact!\n");
				contact = 1;
				break;
			}
		}

		if (pEventMgr->KeyIsDown(K_ESC))
		{
			userAborted = 1;
		}
	}

	// if we have not made contact, we need to let the program know
	if (!contact)
	{
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

	pMono->Out ("end connect!\n");
	return (GetError ());
}


TComm::ERROR	TSerial::Disconnect ()
{
	return (GetError ());
}


TComm::ERROR TSerial::SendPacket (sPacket* pPacket)
{
	if (_pSerial)
	{
		// Assign sequence numbers
		pPacket->header.wSequence = _sequence++;
		pPacket->header.sendID = SEND_ID;
		pPacket->header.destID = DEST_ID;

//		pMono->Out (">Type %d Sending %d bytes (%d %d)\n", pPacket->header.wType,
//			sizeof (pPacket->header) + pPacket->header.len,
//			((long*) pPacket->pData)[0], ((long*) pPacket->pData)[1]);

		_pSerial->PutBuffer ((unsigned char*) &pPacket->header, sizeof (pPacket->header));
		//* force synchronicity
		_WaitForTXBufferSpace();

		if (pPacket->pData && pPacket->header.len)
		{
			_pSerial->PutBuffer ((unsigned char*) pPacket->pData, pPacket->header.len);
			//* force synchronicity
			_WaitForTXBufferSpace();
		}
	}
	return (GetError ());
}

TComm::ERROR	TSerial::ReceivePacket (sPacket* pPacket)
{
	if (_avail || IsPacketAvailable ())
	{
		memcpy (&pPacket->header, inputBuffer, sizeof (pPacket->header));
		memcpy (pPacket->pData, inputBuffer + sizeof (pPacket->header), pPacket->header.len);
		DiscardPacket (NULL);
	}
	return (GetError ());
}

inline int TSerial::_CheckValid (sPacketHeader* pHeader)
{
/*	return (	pHeader->len < 512 &&
				pHeader->wType < BAD_PACKET_TYPE &&
				pHeader->sendID == SEND_ID &&
				pHeader->destID == DEST_ID
			);
*/
	int i = (pHeader->len < 512 &&
				pHeader->wType < BAD_PACKET_TYPE &&
				pHeader->sendID == SEND_ID &&
				pHeader->destID == DEST_ID
			);

	if (i)
		return 1;
	else 
		return 0;
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
		{
			char	msg[40];
			sprintf (msg, "\nGL LineStatus Error! %d", flag);

			pMono->Out(msg);

			//* bugbug: when debugging only!
			APanic (msg);

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

	SetError(SEND_FAILED);
	return FALSE;
}

inline void MonoHeader (sPacketHeader* pHeader)
{
	pMono->Out ("wSequence %d\n", pHeader->wSequence);
	pMono->Out ("wType %d\n", pHeader->wType);
	pMono->Out ("ID %d\n", pHeader->ID);
	pMono->Out ("len %d\n", pHeader->len);
	pMono->Out ("destID %x\n", pHeader->destID);
	pMono->Out ("sendID %x\n", pHeader->sendID);
	pMono->Out ("checksum %d\n", pHeader->checksum);
}

BOOL	TSerial::IsPacketAvailable ()
{
	int	bytes;

	if (!_avail && _pSerial)
	{
		// Make sure we have enough for a header first
		if (receivedBytes < sizeof (sPacketHeader))
		{
			bytes = _pSerial->RXCount();

			//* get the next amount to read which is not any more than
			//* needed to fill the header
			bytes = (receivedBytes + bytes > sizeof (sPacketHeader)) ? sizeof (sPacketHeader) - receivedBytes : bytes;


			//* read
			bytes = _pSerial->GetBuffer ((GF_PTR)inputBuffer + receivedBytes, (unsigned) bytes, GF_TIME_OUT);

			//* error check
			if (bytes + receivedBytes > sizeof (inputBuffer))
			{
				pMono->Out ("Too Many bytes %d\n", bytes + receivedBytes);
			}

			//* update rec'd count, ignor negative return values
			receivedBytes += bytes > 0 ? bytes : 0;

		}

		// Find out if we have a full packet
		if (receivedBytes >= sizeof (sPacketHeader))
		{
			sPacketHeader* pHeader = (sPacketHeader*) inputBuffer;
			// BUGBUG Check to see if the packet is already corrupt
			if (!_CheckValid (pHeader))
			{

				_CheckLineErrors();
				
				//* debug msg
				pMono->Out ("Bad Header received\n");
				MonoHeader (pHeader);

				//* read up to capacity of inputbuffer
				bytes = sizeof (inputBuffer) - receivedBytes;
				bytes = _pSerial->GetBuffer ((GF_PTR) inputBuffer + receivedBytes, (unsigned) bytes, GF_TIME_OUT);

				//* update rec'd count
				receivedBytes += bytes > 0 ? bytes : 0;

				//* advance thru inputbuffer, trying to re-align with data
				for (int i=0;i<receivedBytes - sizeof (*pHeader);i++)
				{
					pMono->Out (".");
					if (_CheckValid ((sPacketHeader*) (inputBuffer + i)))
					{
						//* appears to have re-synch
						pMono->Out ("- RE-SYNCHED? -\n");
						memmove (inputBuffer, inputBuffer+i, receivedBytes - i);
						break;
					}
				}
				
				//* re-calc number of recd bytes
				receivedBytes = (i != receivedBytes - sizeof (*pHeader)) ? receivedBytes - i : 0;
				if (receivedBytes)
				{
					MonoHeader ((sPacketHeader*) inputBuffer);
				}

				pMono->Out ("How can we know if _avail is accurate?");
				return (_avail);
			}

			//* get the number of bytes we still need to read
			bytes = pHeader->len + sizeof (*pHeader) - receivedBytes;

			//* read
			bytes = _pSerial->GetBuffer ((GF_PTR) inputBuffer + receivedBytes, (unsigned) bytes, GF_TIME_OUT);

			//* update rec'd count
			receivedBytes += bytes > 0 ? bytes : 0;

			//
			_avail = receivedBytes >= (pHeader->len + sizeof (sPacketHeader));

			//	if ((3 * (receivedBytes - sizeof (*pHeader)) / 4 - pHeader->len) > 0)
			//	pMono->Out ("<Type %d %3d of %3d\n", pHeader->wType, receivedBytes - sizeof (*pHeader), pHeader->len);
			
			if (_avail)
			{
				memcpy (&lastPacket, pHeader, sizeof (*pHeader));
				lastPacket.pData = pHeader + 1;
			}
		}
	}


	//* bugbug: debugging error check
	if (strcmp (errorCheckBuffer, "check for input buffer overflow"))
	{
		APanic("Ah Hah!, input buffer overflow");
	}

	return (_avail);
}

TComm::ERROR	TSerial::DiscardPacket (sPacketHeader*)
{
	sPacketHeader* pHeader = (sPacketHeader*) inputBuffer;
	

	
	// Adjust data available for data being thrown away
	int packetSize = pHeader->len + sizeof (sPacketHeader);
	receivedBytes -= packetSize;

	// Make sure we are not throwing away more than we have
   receivedBytes = receivedBytes < 0 ? 0 : receivedBytes;

	// Move the data down in the buffer
	memmove (inputBuffer, inputBuffer + packetSize, receivedBytes);


	// Check to see if data is available, we need to do this often anyway!
	_avail = 0; // Must clear to indicate that we don't want what we have (like everyone)
	_avail = IsPacketAvailable ();
	return (GetError ());
}


//*
//* return id of all OTHER connections
//*
WORD	TSerial::GetUserList (WORD* pList)
{
	if (!pList)
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
	_pMdm = NULL;
}

TComm::ERROR	TModem::Init( long )
{
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
	_pMdm->EchoOn();
	//*jjc	_pMdm->SetAbortKey( ALTX );
	//*jjc	cout << "Setup complete\n" << flush;


	//* should we do this ????
	_pLS->SetHandShake( RTS, CTS );


	return ALL_OK;
}

TModem::~TModem( void )
{
	HangUp();
	delete( _pMdm );
	_pMdm = NULL;
}

TComm::ERROR	TModem::Disconnect()
{
	HangUp();
	return TSerial::Disconnect();
}

void	TModem::HangUp()
{
   _pMdm->HangUp();
}


TComm::ERROR	TModem::WaitForCall()
{
	for(;;) 
	{
		if( _pMdm->GetModemInput( _modemInput, 81, 1000 ) == GCPP_USER_ABORT )
		{
			SetError (CONNECTION_FAILED);
		}

		// check for user cancel
		if (pEventMgr->KeyIsDown(K_ESC))
		{
			_baud = 0;
			break;
		}

		// if ringing attempt to answer
		if( strcmp( _modemInput, "RING" ) == 0 ) 
		{
			_baud = _Answer();
			break;
		}
	}

	if (!_baud)
	{
		SetError (CONNECTION_FAILED);
	}

	return GetError();
}

TComm::ERROR	TModem::Dial(char* szPhoneNumber)
{
  	long msecs;

	// bugbug: Tigre require a touch tone phone!
	_pMdm->DialMode( _dialMode );

	_pMdm->Dial( szPhoneNumber );

	// assume we will time out first
	SetError(CONNECTION_FAILED);

	// loop until connect, or until abort key hit
	//	give it 50 seconds to connect
	msecs = 50 * 1000L;
	while( msecs > 0 ) 
	{
		// msecs counts down as we go
   	msecs = _pMdm->GetModemInput( _modemInput, 81, msecs );

		// check for user cancel
		if (pEventMgr->KeyIsDown(K_ESC))
		{
			SetError(CONNECTION_FAILED);
   		return GetError();
		}

		// check for successfull connection
   	if (strstr( _modemInput, "CONNECT" ) != 0)
		{
			ClearError();
   		return GetError();
   	}
	}

	return GetError();
}

long TModem::_Answer()
{
	long msecs = 30000L;

	//*jjc	cout << "Answering phone\n";
	_pMdm->AnswerPhone();
	while( msecs > 0 ) {
		msecs = _pMdm->GetModemInput( _modemInput, 81, msecs );
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
	}
	if( _baud > 0) {
		//*jjc	cout << "\nBaud rate detected as " << _baud << "\n";
		_pDF->BaudRate( _baud );
	}
	else {
		//*jjc	cout << "\nFailed to Connect\n";
		return( 0 );
	}
	return( _baud );
}
