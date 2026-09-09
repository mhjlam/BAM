#ifndef serial_hpp
#define serial_hpp

class TSerial : public TComm
{
	public:
		TSerial();
		virtual ~TSerial();
		virtual ERROR Init(long optionalArg = 0);
		virtual ERROR Connect();
		virtual ERROR Disconnect();
		virtual ERROR SetPort(int newPort);
		void SetBaud(int newBaud);
		int GetBaud() { return (int)_baud; }
		virtual ERROR SendPacket(sPacket *pPacket, bool fIsResend = FALSE);
		virtual ERROR ReceivePacket(sPacket *pPacket);
		virtual BOOL IsPacketAvailable();
		virtual ERROR DiscardPacket(sPacketHeader *pHeader = NULL);
		virtual WORD GetUserList(WORD *pList = NULL);

	protected:
		long _baud;
		int _port;
};

class TModem : public TSerial
{
	public:
		enum { PULSE = 0, TONE = 1 };
		TModem();
		virtual ~TModem();
		virtual ERROR Init(long optionalArg = 0);
		virtual ERROR Disconnect();
		ERROR WaitForCall();
		ERROR Dial(char *phoneNumber);
		void HangUp();
		ERROR Write(char *text);
		void SetDial(int mode) { _dialMode = mode; }

	protected:
		int _dialMode;
};

#endif
