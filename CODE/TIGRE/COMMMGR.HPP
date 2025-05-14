//
// CommMgr.hpp
//
// (c) Copyright 1995, Tachyon Studios, Inc.  All rights reserved.
//


#ifndef commmgr_hpp
#define commmgr_hpp

#include "comm.hpp"

class TComm;
struct sPacket;
struct sPacketHeader;
typedef int (*pIntFnInt) (int);

extern char pErrorStrings[][30], pBlockIDStrings[][30];

#ifdef	OS_MAC
	const tcommmgrTOTAL_BLOCK_SIZE = 256;
#else
	// default DOS
	const tcommmgrTOTAL_BLOCK_SIZE = _NETNOW_DATA_PACKET;
#endif

const tcommmgrMAX_PACKET_SIZE = tcommmgrTOTAL_BLOCK_SIZE - sizeof (sPacket) - 16;
const tcommmgrNBLOCKS = 16;
const tcommmgrMAX_PACKETS_WAITING = 1;

// No virtual destructor, as this is not expected to be a base class
class TCommMgr : public Manager, public Object
{
	public :

		TCommMgr ();
		~TCommMgr ();

		char*			GetBuffer ()				{ return (pBuffer); }
		int			GetBufferLen ()			{ return (bufferLen); }

		int 			Init (TComm* pComm, long optionalArg = NULL);
		int 			Connect ();
		int 			Disconnect ();
		int 			SendData 	 (int to, int type, int len, void* pData);
		int 			SendData 	 (sPacket* pPacket, bool fWaitForAck = TRUE);
		int 			ReceiveData (int* pFrom, int* pType, int* pLen, void* pData = NULL);
		int 			ReceiveData (sPacket* pPacket);
		int 			DataAvailable (int* pFrom, int* pType, int* pLen);
		int 			DataAvailable (sPacket* pPacket = NULL);
		int 			DiscardData (), DumpQueue(void);
		int			GetError ();
		int			GetUserID ();
		int			GetUserList (unsigned* pList = NULL);

		bool			Save(uint16 state, FILE *fp = NULL);
		void			Silence(bool fNewMode);

		static pIntFnInt	SetUserAbortFn (pIntFnInt pFn);

		TComm*		pComm;			// Pointer to a pComm derived class
											//	which we will use for communicating
		bool			fSilence;
		int			totalPacketsWaiting;
		int			EnQueueData (bool fDebug = FALSE);

	private :

		int			_SendData (sPacket* pPacket, bool fWaitForAck = TRUE);
		int			_ReceiveData (sPacket* pPacket);
		int			ReceiveLargePacket ();
		TComm::ERROR SendAckNak(sPacket *pPacket, bool fOkay);
		sPacket*		DeQueueData ();

		bool			AddToWaitingRoom(sPacket *pPacket), RemoveFromWaitingRoom(sPacket *pPacket);
		TComm::ERROR ResendWaiters(void);
		void			RemoveEmpties(), RemoveOldReplies(void);
		int			QueueIncr (int oldVal) { return ((oldVal + 1) & (tcommmgrNBLOCKS - 1)); }
		int			QueueDecr (int oldVal) { return ((oldVal + tcommmgrNBLOCKS - 1) & (tcommmgrNBLOCKS - 1)); }
		void  		IncrHead () { head = QueueIncr (head);	}
		void			IncrTail () { tail = QueueIncr (tail); }
		void  		DecrHead () { head = QueueDecr (head);	}
		void			DecrTail () { tail = QueueDecr (tail); }

		char*			pBuffer;			// Pointer to a temporary buffer
		int			bufferLen;		// The length of the receive buffer
		int			dataAvail;		// Indicates that a block is available
		int	head;	
		int	tail;	
		char	queue[tcommmgrNBLOCKS][tcommmgrTOTAL_BLOCK_SIZE];

		sPacket	waitingRoom[tcommmgrMAX_PACKETS_WAITING];
		char		waitingRoomData[tcommmgrMAX_PACKETS_WAITING][tcommmgrTOTAL_BLOCK_SIZE];
};

extern	TCommMgr		*pCommMgr;

#endif
