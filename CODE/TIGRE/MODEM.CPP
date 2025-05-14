#include <dos.h>
#include <conio.h>
#include <string.h>

#include "types.hpp"
#include "modem.hpp"
#include "mono.hpp"

static int  receivedBytes;
static char inputBuffer[516];

const cPlayerID = 0x12345678;
const cEnemyID = 0x56789abc;
static const char* cModemPickUpPhone = "S0=1";

// function pointer to user abort function (RGW)
extern "C" int (*pfnUserAbort) (int init);

TModem* TModem::pModem;


TModem::TModem ()
{
	ClearVars ();
	pModem = this;
	atexit (AtExitFn);
}

TModem::~TModem ()
{
	if (pModem)
	{
		Destruct ();
	}
}

void TModem::Destruct ()
{
	pMono->Out ("Dininitialzing all ports\n");
	// This will be done by the atexit function, but for re-entrancy...
	Disconnect ();
	deinit_all_ports ();
	ClearVars ();
}

void TModem::AtExitFn ()
{
	if (pModem)
	{
		pModem->Destruct ();
	}
}

void TModem::ClearVars ()
{
	baud = 38400;		// User defined
	port = 0;			// User defined
	irq = 0;				// User defined or dependent on the port
	pSerial = NULL;	// Not initialized yet
	avail = 0;			// Is data available
	sequence = 0;
	isModem = 0;
	wConsoleNode = 1;	// BUGBUG this should be set in the connect code
	isInit = 0;
	connected = 0;
	pModem = NULL;
}

TComm::ERROR TModem::SetPort (int newPort, int irq, int baseAddr)
{
  static const short ports[] = { COM1, COM2, COM3, COM4 };
  static const short irqs[]  = { 4, 3, 4, 3 };

	if (--newPort & ~3)
	{
		SetError (INIT_FAILED);
	}
	else
	{
		port = baseAddr ? baseAddr : ports[newPort];
		irq  = irq ? irq : irqs[newPort];
	}
	return (GetError ());
}

TComm::ERROR TModem::SetBaud (int newBaud)
{
	static int const bauds [] =
	{
		300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
	};

	SetError (INIT_FAILED);
	for (int i=0;i<sizeof (bauds)/sizeof (bauds[0]);i++)
	{
		if (bauds[i] == newBaud)
		{
			ClearError ();
			baud = newBaud;
			break;
		}
	}
	if (!GetError () && pSerial)
	{
		set_speed (pSerial, baud);
	}
	return (GetError ());
}

int TModem::IsModem ()
{
	if (!isModem && pSerial)
	{
		isModem = modem_type (pSerial);
		isModem = isModem == EOF ? 0 : isModem;
	}
	return (isModem);
}

TComm::ERROR TModem::Init (long)
{
	SetError (INIT_FAILED);
	if (!port)
	{
		return (GetError ());
	}
	if (isInit)
	{
		ClearError ();
		return (GetError ());
	}

	static const char* const uartType[] =
		{
			"Unknown",
			"INS8250/INS8250B",
			"INS8250A/NS16450", 
			"NS16550", 
			"NS16450A"
		};

	// Find out what kind of UART is on this machine
	int uart = isauart (port);
	pMono->Out ("UART = %s\n", uartType[uart+1]);

	// If the UART is a 16550A, set threshold to be 14 bytes
	if (uart == 3)
	{
		set_16550_threshold (port, 14);
	}

	pSerial = u8250_init (port, baud, DATABITS8, PARITY_NONE, STOPBITS1);
	if (!pSerial)
	{
		return (GetError ());
	}

	// Use interrupt-driven receive and transmit 
	// Install interrupt processing routines for receive and transmit 
   install_ipr(pSerial, RECEIVE, NULL, 4096);
   install_ipr(pSerial, TRANSMIT, NULL, 4096);

	isr_strategy = PASS_TO_PROTECTED;
	// Install the interrupt handler (null implies default handler) 
	install_isr(pSerial, irq, (PIHANDLER) NULL);

	isModem = IsModem ();
	if (isModem)
	{
		pMono->Out ("Modem found %d\n", isModem);
		modem_set (pSerial, MODEM_CMD_ECHO, (long)"E0");
	}

	ClearError ();
	isInit = 1;
	return (GetError ());
}

TComm::ERROR TModem::Dial (const char* pNum)
{
	if (!pSerial)
	{
		return (GetError ());
	}

	if (!IsModem ())
	{
		SetError (NOT_A_MODEM);
		return (GetError ());
	}

	pfnUserAbort (0); // We are reinitializing user abort
	modem_init (pSerial, (short)isModem);	// BUGBUG check for out of memory!

	int mDial = modem_dial (pSerial, pNum);
	while (mDial > 0 && (mDial < 8 || mDial > 20))
	{
		pMono->Out ("Modem dial returned %d\n", mDial);
      mDial = modem_wait_dial_response (pSerial);
		baud = get_speed (pSerial);
		if (pfnUserAbort (1))
		{
			break;
		}
	}
	pMono->Out ("Modem dial returned %d\n", mDial);
	if (mDial > 0)
	{
		connected = 1;
	}
	else
	{
		SetError (CONNECTION_FAILED);
	}

	return (GetError ());
}

TComm::ERROR TModem::WaitForCall ()
{
	if (!pSerial)
	{
		return (GetError ());
	}

	if (!isModem)
	{
		isModem = IsModem ();
		if (!isModem)
		{
			SetError (NOT_A_MODEM);
			return (GetError ());
		}
	}

	pfnUserAbort (0); // We are reinitializing user abort
	modem_init (pSerial, (short)isModem);	// BUGBUG check for out of memory!

	set_dtr (pSerial, HIGH);
	modem_cmd (pSerial, (char*) cModemPickUpPhone);

   int mDial = modem_wait_dial_response (pSerial);
	while (mDial > 0 && (mDial < 8 || mDial > 20))
	{
		pMono->Out ("Modem dial returned %d\n", mDial);
      mDial = modem_wait_dial_response (pSerial);
		baud = get_speed (pSerial);
		if (pfnUserAbort (1))
		{
			break;
		}
	}
	pMono->Out ("Modem dial returned %d\n", mDial);
	if (mDial > 0)
	{
		connected = 1;
	}
	else
	{
		SetError (CONNECTION_FAILED);
	}
	baud = get_speed (pSerial);

	return (GetError ());
}

TComm::ERROR	TModem::Connect ()
{
	// If we have not initialized, 
	if (!pSerial)
	{
		return (GetError ());
	}


	// Flush the receive queue
	c_rxflush (pSerial, 0);

	// Assume the connection fails unless told otherwise
	SetError (CONNECTION_FAILED);
//	if (isModem && !connected)
//	{
//		return (GetError ());
//	}

	// Call the user abort function to initialize it
	int userAborted = 0;
	int contact = 0;			// used to indicate a contact has been made
	
	// Create a connection packet
	sPacket packet;

	memset (&packet, 0, sizeof (packet));
	packet.header.wType = CONNECT;
	packet.pData = "Connecting";
	packet.header.len = strlen ((char*)packet.pData);

	pfnUserAbort (0); // We are reinitializing user abort

	// Send it out every second 
	while (!contact && !userAborted)
	{
		SendPacket (&packet);
		sleep (1); // delay (3 * 8000 * (packet.header.len + sizeof (packet.header)) / baud);

		// Check to see if a packet has been received
		if (IsPacketAvailable ())
		{
			sPacket received;
			received.pData = inputBuffer + sizeof (received.header);
			ReceivePacket (&received);
//			pMono->Out ("Received packet\n");

			// If a packet has been received make sure it is a CONNECT packet
			// otherwise, ignore it
			if (received.header.wType == CONNECT)
			{
				contact = 1;
				break;
			}
		}
		userAborted = pfnUserAbort (1); // BUGBUG this should be a call to the user supplied function
	}

	// if we have not made contact, we need to let the program know
	if (!contact)
	{
		return (GetError ());
	}

	SendPacket (&packet);
	delay (3 * 8000 * (packet.header.len + sizeof (packet.header)) / baud);

	// Otherwise go on about the business of connecting
	packet.header.wType = CONNECT_OK;
	packet.pData = "Have Connection";
	packet.header.len = strlen ((char*)packet.pData);
	SendPacket (&packet);
#if 0
	pfnUserAbort (0); // We are reinitializing user abort
	while (!IsPacketAvailable ())
	{
		if (pfnUserAbort (1))
		{
			return (GetError ());
		}
	}

	// Check to see if a packet has been received
	if (IsPacketAvailable ())
	{
		sPacket received;
		received.pData = inputBuffer + sizeof (received.header);
		ReceivePacket (&received);

		// If a packet has been received make sure it is a CONNECT packet
		// otherwise, ignore it
		if (received.header.wType == CONNECT_OK)
		{
			// Finally admit that we have connection
			ClearError ();
		}
	}
#else
	// Finally admit that we have connection
	ClearError ();
#endif
	return (GetError ());
}

TComm::ERROR	TModem::Disconnect ()
{
	if (isModem)
	{
		modem_hangup (pSerial);
	}
	return (GetError ());
}

TComm::ERROR TModem::SendPacket (sPacket* pPacket)
{
	if (pSerial)
	{
		// Assign sequence numbers
		pPacket->header.wSequence = (sequence++) % 256;
		pPacket->header.sendID = cPlayerID;
		pPacket->header.destID = cEnemyID;

//		pMono->Out (">Type %d Sending %d bytes (%d %d)\n", pPacket->header.wType,
//			sizeof (pPacket->header) + pPacket->header.len,
//			((long*) pPacket->pData)[0], ((long*) pPacket->pData)[1]);
		c_putn (pSerial, sizeof (pPacket->header), (char*) &pPacket->header);
		c_putn (pSerial, pPacket->header.len, (char*) pPacket->pData);
	}
	return (GetError ());
}

TComm::ERROR	TModem::ReceivePacket (sPacket* pPacket)
{
	if (avail || IsPacketAvailable ())
	{
		memcpy (&pPacket->header, inputBuffer, sizeof (pPacket->header));
		memcpy (pPacket->pData, inputBuffer + sizeof (pPacket->header), pPacket->header.len);
		DiscardPacket (NULL);
	}
	return (GetError ());
}

inline int CheckValid (sPacketHeader* pHeader)
{
	return (	pHeader->len < 512 &&
				pHeader->wType < BAD_PACKET_TYPE &&
				pHeader->sendID == cPlayerID &&
				pHeader->destID == cEnemyID
			);
}

inline void MonoHeader (sPacketHeader* pHeader)
{
	pMono->Out ("wSequence %d\n", pHeader->wSequence);
	pMono->Out ("wType %d\n", pHeader->wType);
	pMono->Out ("ID %d\n", pHeader->ID);
	pMono->Out ("len %d\n", pHeader->len);
	pMono->Out ("destID %x\n", pHeader->destID);
	pMono->Out ("sendID %x\n", pHeader->sendID);
	pMono->Out ("checksum %ld\n", pHeader->checksum);
}

BOOL	TModem::IsPacketAvailable ()
{
	if (!avail && pSerial)
	{
		// Make sure we have enough for a header first
		if (receivedBytes < sizeof (sPacketHeader))
		{
			int bytes = c_rxcount (pSerial);
			bytes = (receivedBytes + bytes > sizeof (sPacketHeader)) ? sizeof (sPacketHeader) - receivedBytes : bytes;
			bytes = c_gets (pSerial, inputBuffer + receivedBytes, bytes, -1);
			if (bytes + receivedBytes > sizeof (inputBuffer))
			{
				pMono->Out ("Too Many bytes %d\n", bytes + receivedBytes);
			}
			receivedBytes += bytes > 0 ? bytes : 0;
		}

		// Find out if we have a full packet
		if (receivedBytes >= sizeof (sPacketHeader))
		{
			sPacketHeader* pHeader = (sPacketHeader*) inputBuffer;
			// BUGBUG Check to see if the packet is already corrupt
			if (!CheckValid (pHeader))
			{
				pMono->Out ("Bad Header received\n");
				MonoHeader (pHeader);
				int bytes = sizeof (inputBuffer) - receivedBytes;
				bytes = c_gets (pSerial, inputBuffer + receivedBytes, bytes, -1);
				receivedBytes += bytes > 0 ? bytes : 0;
				for (int i=0;i<receivedBytes - sizeof (*pHeader);i++)
				{
					if (CheckValid ((sPacketHeader*) (inputBuffer + i)))
					{
						memmove (inputBuffer, inputBuffer+i, receivedBytes - i);
						break;
					}
				}
				receivedBytes = (i != receivedBytes - sizeof (*pHeader)) ? receivedBytes - i : 0;
				if (receivedBytes)
				{
					MonoHeader ((sPacketHeader*) inputBuffer);
				}
				return (avail);
			}
			int bytes = pHeader->len + sizeof (*pHeader) - receivedBytes;
			bytes = c_gets (pSerial, inputBuffer + receivedBytes, bytes, -1);
			receivedBytes += bytes > 0 ? bytes : 0;
			avail = receivedBytes >= (pHeader->len + sizeof (sPacketHeader));
//			if ((3 * (receivedBytes - sizeof (*pHeader)) / 4 - pHeader->len) > 0)
//			pMono->Out ("<Type %d %3d of %3d\n", pHeader->wType, receivedBytes - sizeof (*pHeader), pHeader->len);
			
			if (avail)
			{
				memcpy (&lastPacket, pHeader, sizeof (*pHeader));
				lastPacket.pData = pHeader + 1;
			}
		}
	}
	return (avail);
}

TComm::ERROR	TModem::DiscardPacket (sPacketHeader*)
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
	avail = 0; // Must clear to indicate that we don't want what we have (like everyone)
	avail = IsPacketAvailable ();
	return (GetError ());
}

WORD	TModem::GetUserList (WORD*)
{
	if (!pSerial)
	{
		return (0);
	}
	return (1);// BUGBUG what to do here?
}


