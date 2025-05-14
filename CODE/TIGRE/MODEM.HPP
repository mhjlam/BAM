#ifndef modem_hpp
#define modem_hpp

#undef WORD

// These are odd because of CCT
#define RATIONAL
#define ASCII_H_INCLUDED
#define WORD sWORD
typedef unsigned short sWORD;
typedef unsigned char   BYTE;
typedef unsigned long   DWORD;
#define C52

#include "comm.h"
#include "ins8250.h"
#include "modem.h"

#undef WORD

#include "comm.hpp"

class TModem : public TComm
{
	public :
					TModem ();
		virtual ~TModem ();

		// Initialization
		virtual	ERROR		Init (long optionalArg);
		virtual	ERROR		Connect ();
		virtual	ERROR		Disconnect ();
					ERROR 	SetPort (int newPort, int irq = 0, int baseAddr = 0);
					ERROR 	SetBaud (int newBaud);
					int		IsModem ();

					// NOTE This call can change the baud!
					ERROR		Dial (const char* pNum);
					ERROR		WaitForCall ();
					int		GetBaud () { return (baud); }

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket);
		virtual	ERROR		ReceivePacket (sPacket* pPacket);
		virtual	BOOL		IsPacketAvailable ();
		virtual	ERROR		DiscardPacket (sPacketHeader* pPacketHeader = NULL);

		// Information
		virtual	WORD		GetUserList (WORD* pList = NULL);

	protected :
					ERROR		DialModem (const char* num);
					void		ClearVars ();
					static 	void	AtExitFn ();
					void		Destruct ();

	private :
		int	baud;
		short	port;			// short is used for increased compatability with CCT
		short	irq;			// short is used for increased compatability with CCT
		hPORT pSerial;		//
		int	avail;		// is a packet available right now
		int	sequence;	// what number packet are we working on
		int	isModem;		// do we recognize this as a modem
		int	isInit;		// has init already been called
		int	connected;	// this is strictly for modems

	static	TModem*	pModem;
};

#endif


