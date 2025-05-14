#ifndef serial_hpp
#define serial_hpp

// Greenleaf Com++ lib include
#include "compplib.hpp"

//*jc	#include "comm.hpp"

const unsigned	GF_TIME_OUT = 3;

//*************************************************************
//*

class TSerial : public TComm
{
	public :
					TSerial ();
		virtual	~TSerial ();

		// Initialization
		virtual ERROR		Init (long optionalArg);
		virtual ERROR		Connect ();
		virtual ERROR		Disconnect ();
		virtual ERROR	 	SetPort (int newPort);

		// bugbug: need implementation
		void			 		SetBaud (int newBaud);

					// NOTE This call can change the baud!
					int		GetBaud () { return (_baud); }

		// Data Transfer
		virtual ERROR		SendPacket (sPacket* pPacket, bool fIsResend = FALSE);
		virtual ERROR		ReceivePacket (sPacket* pPacket);
		virtual BOOL		IsPacketAvailable ();
		virtual ERROR		DiscardPacket (sPacketHeader* pPacketHeader = NULL);

		// Information
		virtual WORD		GetUserList (WORD* pList = NULL);
		inline void			MonoHeader (sPacketHeaderTiny* pHeader);

	protected :

		static void			AtExitFn ();
		void					_ClearVars ();
		virtual void		_Destruct ();
		int		 			_CheckValid (sPacketHeaderTiny* pHeader);

		bool					_WaitForTXBufferSpace();
		bool					_CheckLineErrors();

		typedef	unsigned char*	GF_PTR;

		// GreenLeaf objects
   	GFSerial			*_pSerial;
    	GFDataFormat 	*_pDF;
    	GFLineStatus	*_pLS;
	   //* we really should use GFComLink		*pLink;
		// but as GreenLeaf failed to give it a virtual destructor, we won't.
		GFI8250			*_pLink;

		long				_baud;			// baud rate
		long				_desiredBaud;	// requested baud rate
		int				_port;			// 1 == COM1, for example
		int				_avail;			// is a packet available right now
		uchar				_sequence;		// what number packet are we working on

		WORD				_enemyId;		// the unique id (wConsoleNode) of connected opponent

};



//*************************************************************
//*	TModem adds to TSerial

class TModem : public TSerial
{
	public:

		TModem( void );
		~TModem( void );

		virtual ERROR	Init (long optionalArg);
		virtual ERROR	Disconnect ();

		ERROR				WaitForCall( void );
		ERROR				Dial( char* szPhoneNumber );
		void				HangUp ();
		ERROR				Write(char* szWriteStr);

		// dial mode
		enum { PULSE = GCPP_PULSE, TONE = GCPP_TONE };
		void			 	SetDial (int toneOrPulse) { _dialMode = toneOrPulse; };

	protected:
		
		long				_Answer( void );

		GFModem			*_pMdm;
		char				_modemInput[81];
		int				_dialMode;
		bool				_fHangupCalled;
};

#endif


