//
// CommMgr.cpp
//
// (c) Copyright 1995, Tachyon Studios, Inc.  All rights reserved.
//
// Future enhancements...
//
// - Make acknowledgements optional:
//		Now all messages return an acknowledge packet. Some communications
//		libraries may already implement error correcting making this 
//		redundant. 
//
//	- Make packet size variable:
//		Some communications libraries may be handicapped by a hardcoded
//		packet size which is not appropriate.

#include "eventmgr.hpp"	// for ATicks() decl.	VAN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.hpp"
#include "savemgr.hpp"
#include "memmgr.hpp"

#ifndef NO_TIGRE
#include "apimem.hpp"
#endif

#ifndef TRACK_MEM
#define TRACK_MEM(dummy)
#endif

#include "commmgr.hpp"
#include "comm.hpp"
#include "debug.hpp"
#include "mono.hpp"

#include <assert.h>
#ifndef ASSERT
#define ASSERT assert
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

//#define NDEBUG

char pErrorStrings[][30] =
{"OK", "NULL CLASS", "INIT FAILED", "CONNECTION FAILED", "PACKET FAILED",
	"PACKET NOT AVAILABLE", "UNKNOWN PACKET", "UNEXPECTED PACKET", "SEND FAILED",
	"BAD CHECKSUM", "TIMEOUT", "NOT A MODEM", "TIMEOUT (BUFFER FULL)", "TIMEOUT (NO ACK)",
	"PREMATURE ACK", "UNKNOWN ERROR", "DATA NAK", "BAD PACKET TYPE"};

char pBlockIDStrings[][30] =
{"_XFER_BLOCK_DATA", "_XFER_BLOCK_REQUEST_ID", "_XFER_BLOCK_ID", "_XFER_BLOCK_REQUEST_RESEND",
"_XFER_BLOCK_NAME", "_XFER_BLOCK_VOICE", "_XFER_BLOCK_VOICE_HEADER", "_XFER_BLOCK_VOICE_END",
"BLOCK_HEADER", "BLOCK_DATA", "BLOCK_END", "DATA_ACK", "DATA_NAK", "CONNECT",
"CONNECT_OK", "EMPTY_PACKET", "BAD_PACKET_TYPE"};

TCommMgr					*pCommMgr = NULL;

// BUGBUG - this is for use by DATA_ACK packets and any other packets which have no
// data to send - point here instead, because NetNow seems to hate empty packets - Van
char dummySpace[32];

Debugger ComDebug;

bool
CommMgrSave(uint16 state, FILE *fp)
{
	return	pCommMgr->Save(state, fp);
}

// function pointer to user abort function (RGW)
extern "C" int (*pfnUserAbort) (int init);

pIntFnInt TCommMgr::SetUserAbortFn (pIntFnInt pNew)
{
	pIntFnInt pOld = pfnUserAbort;
	pfnUserAbort = pNew;
	return (pOld);
}

class TNULL : public TComm
{
	public :
		TNULL ();
		virtual ~TNULL () {};

		// Information
		virtual	WORD		GetUserID () { return (-1); }
		virtual	WORD		GetUserList (WORD*) { return (0); }

	private :
		// Initialization/Shutdown
		virtual	ERROR		Init (long optionalArg);
		virtual	ERROR		Connect ();
		virtual	ERROR		Disconnect ();

		// Data Transfer
		virtual	ERROR		SendPacket (sPacket* pPacket, bool fIsResend = FALSE);
		virtual	ERROR		ReceivePacket (sPacket* pPacket);
		virtual	BOOL		IsPacketAvailable ();
		virtual	ERROR		DiscardPacket (sPacketHeader* pPacketHeader = NULL);

} tNull;

TNULL::TNULL ()
{
	SetError (NULL_CLASS);
}

TComm::ERROR
TNULL::Init (long optionalArg)
{
	// get rid of unreferenced warning
	optionalArg = optionalArg;

	SetError (NULL_CLASS);
	return (GetError ());
}

TComm::ERROR
TNULL::Connect ()
{
	SetError (NULL_CLASS);
	return (GetError ());
}

TComm::ERROR
TNULL::Disconnect ()
{
	SetError (NULL_CLASS);
	return (GetError ());
}

// Data Transfer
TComm::ERROR
TNULL::SendPacket (sPacket*, bool)
{
	SetError (NULL_CLASS);
	return (GetError ());
}

TComm::ERROR
TNULL::ReceivePacket (sPacket*)
{
	SetError (NULL_CLASS);
	return (GetError ());
}

TComm::ERROR
TNULL::DiscardPacket (sPacketHeader*)
{
	SetError (NULL_CLASS);
	return (GetError ());
}

BOOL
TNULL::IsPacketAvailable ()
{
	return (0);
}

int TCommMgr::GetError ()
{
	return (pComm->GetError ());
}

int TCommMgr::GetUserID ()
{
	return (pComm->GetUserID ());
}

int TCommMgr::GetUserList (unsigned* pList)
{
	return (pComm->GetUserList (pList));
}

TCommMgr::TCommMgr ()
{
	#ifndef NDEBUG
	ComDebug.OpenFile("COMMMGR.DBG");
	ComDebug.Out("TCommMgr::TCommMgr()\n");
	#else
	ComDebug.Silence(TRUE);
	#endif

	if (!pCommMgr)
	{
		pCommMgr = this;

		// setup for save
		AtSave(CommMgrSave);
	}

	pComm = &tNull;
	bufferLen = tcommDefaultMaxSendSize;
	TRACK_MEM("TCommMgr Buf");	pBuffer = ADerefAs(char, ASysMalloc(bufferLen));
	head = tail = 0;
	dataAvail = 0;
	totalPacketsWaiting = 0;
	#ifndef NDEBUG
	ComDebug.Out("TCommMgr::TCommMgr() done\n");
	#endif
}

TCommMgr::~TCommMgr ()
{
	#ifndef NDEBUG
	ComDebug.Out("TCommMgr::~TCommMgr()\n");
	#endif

	delete [] pBuffer;
	if(pComm != &tNull)
		delete pComm;

	#ifndef NDEBUG
	ComDebug.Out("TCommMgr::~TCommMgr() done\n");
	#endif
}

int TCommMgr::Init (TComm* pCom, long optionalArg)
{
	#ifndef NDEBUG
	ComDebug.Out("TCommMgr::Init(%x, %ld)\nACK packet size == %d bytes",
		(void *)pCom, optionalArg, sizeof(sPacket) + 4);
	fSilence = FALSE;
	#else
	fSilence = TRUE;
	#endif

	totalPacketsWaiting = 0;
	if (pCom)
	{
		pComm = pCom;
		return (pComm->Init (optionalArg));
	}
	return (pComm->GetError ());
}

int TCommMgr::Connect ()
{
	#ifndef NDEBUG
	ComDebug.Out("TCommMgr::Connect()\n");
	#endif
	return (pComm->Connect ());
}

int TCommMgr::Disconnect ()
{
	#ifndef NDEBUG
	ComDebug.Out("TCommMgr::Disconnect()\n");
	#endif
	return (pComm->Disconnect ());
}

// dynamically turn on/off debug output
void
TCommMgr::Silence(bool fNewMode)
{
	fSilence = fNewMode;
	ComDebug.Silence(fNewMode);
}

// remove any leftover ACK/NAK packets.
// NOTE: only call this when you are SURE all remaining ACK/NAKs are
// useless!
void
TCommMgr::RemoveOldReplies(void)
{
	int topOfQueue;

	// push top of queue forward until first useful packet is found
   sPacket* pClear = (sPacket*) (queue[head]);

	// presumably a DATA_ACK will never be left behind, but just in case,
	// we'll clear those too.

	while((pClear->header.wType == EMPTY_PACKET ||
			pClear->header.wType == DATA_NAK ||
			pClear->header.wType == DATA_ACK) && head != tail)
   {
		IncrHead ();
      pClear = (sPacket*) (queue[head]);
   }
   
   // do the same from the end
	topOfQueue = (tail + tcommmgrNBLOCKS - 1) % tcommmgrNBLOCKS;

	pClear = (sPacket*) (queue[topOfQueue]);
	while((pClear->header.wType == EMPTY_PACKET ||
			pClear->header.wType == DATA_NAK ||
			pClear->header.wType == DATA_ACK) && head != tail)
   {
      DecrTail ();
		topOfQueue = (tail + tcommmgrNBLOCKS - 1) % tcommmgrNBLOCKS;
      pClear = (sPacket*) (queue[topOfQueue]);
   }
}

void TCommMgr::RemoveEmpties ()
{
	int topOfQueue;

	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 40);
	pMono->Out("RE:0");
	pMono->RestoreWindow();
	#endif

	// Remove all of the empty packets from the front of the queue
   sPacket* pClear = (sPacket*) (queue[head]);
   while (pClear->header.wType == EMPTY_PACKET && head != tail)
   {
//		ComDebug.Out("RemoveEmpties(): IncrHead()\n");

		IncrHead ();
      pClear = (sPacket*) (queue[head]);
   }
   
   // Remove all of the empty packets from the end of the queue
	topOfQueue = (tail + tcommmgrNBLOCKS - 1) % tcommmgrNBLOCKS;

	pClear = (sPacket*) (queue[topOfQueue]);
   while (pClear->header.wType == EMPTY_PACKET && head != tail)
   {
//		ComDebug.Out("RemoveEmpties(): DecrTail()\n");

      DecrTail ();
		topOfQueue = (tail + tcommmgrNBLOCKS - 1) % tcommmgrNBLOCKS;
      pClear = (sPacket*) (queue[topOfQueue]);
   }
	#ifndef NDEBUG
	pMono->SaveWindow();
	pMono->Goto(25, 40);
	pMono->Out("    ");
	pMono->RestoreWindow();
	#endif
}

enum	timerTags {TIMER_TOTAL, TIMER_WAIT1, TIMER_WAIT2, TIMER_WAIT_RESEND,
	TIMER_SEND, TIMER_NQ_EMPTIES, TIMER_NQ_RX, TIMER_NQ_ACKNAK, TEMP_TIMER_MAX};
DebugTimer	tempTimers[TEMP_TIMER_MAX];

int TCommMgr::EnQueueData(bool fDebug)
{
	int				retv = -1;
	TComm::ERROR	error1;
	bool				fPacketAvailable;

//	#ifndef NDEBUG
//	pMono->SaveWindow();
//	pMono->Goto(25, 40);
//	pMono->Out("NQ:0");
//	pMono->RestoreWindow();
//	#endif

	#ifdef	OS_MAC
		// here' a convenient spot to conduct periodic functions
		pComm->Cycle();
	#endif
	tempTimers[TIMER_NQ_ACKNAK].duration = 0;
	
	tempTimers[TIMER_NQ_EMPTIES].Start();
	// Remove all of the empty packets
	RemoveEmpties ();
	tempTimers[TIMER_NQ_EMPTIES].Stop();

	// Make sure we do not overflow the available queue
	if (QueueIncr (tail) == head)
	{
		#ifndef NDEBUG
		ComDebug.Out("\nEnQ::Input queue already full\n");
		#endif

		DumpQueue();
		pComm->SetError(TComm::INPUT_QUEUE_FULL);

//		#ifndef NDEBUG
//		pMono->SaveWindow();
//		pMono->Goto(25, 40);
//		pMono->Out("    ");
//		pMono->RestoreWindow();
//		#endif

		return (retv);
	}

	// If any data is available place it in the queue
	tempTimers[TIMER_NQ_RX].duration = 0;
	fPacketAvailable = pComm->IsPacketAvailable()? TRUE: FALSE;
	if(fPacketAvailable)
	{
		// receive packet
		tempTimers[TIMER_NQ_RX].Start();
		sPacket* pAck = (sPacket*) (queue[tail]);
		pAck->pData = (void*) (pAck + 1);
		retv = tail; // indicate where the data was placed
		IncrTail ();
		error1 = pComm->ReceivePacket (pAck);
		tempTimers[TIMER_NQ_RX].Stop();

		switch(error1)
		{
			case TComm::PACKET_NOT_AVAILABLE:
				ComDebug.Out("EnQ[NO PACKET])..");
				pAck->header.wType = EMPTY_PACKET;
				DecrTail();
				pComm->ClearError();
				break;

			case TComm::BAD_CHECKSUM:
				// make bad packet available again
				pAck->header.wType = EMPTY_PACKET;
				DecrTail();

				#ifndef NDEBUG
				ComDebug.Out("(EnQ[BAD CRC])..");
				#endif
				pComm->ClearError();

				error1 = SendAckNak(pAck, FALSE);
				if(error1 != TComm::ALL_OK)
				{
//					#ifndef NDEBUG
//					pMono->SaveWindow();
//					pMono->Goto(25, 40);
//					pMono->Out("    ");
//					pMono->RestoreWindow();
//					#endif
					return(error1);
				}
				break;

			case TComm::ALL_OK:
			default:
				// send reply if necessary
				switch (pAck->header.wType)
				{
					case BLOCK_HEADER :
					case BLOCK_DATA :
					case BLOCK_END :
					case _XFER_BLOCK_DATA :
						{
							#ifndef NDEBUG
							ComDebug.Out("(EnQ[wSeq%d])..", pAck->header.wSequence);
							#endif
							error1 = SendAckNak(pAck, TRUE);
							if(error1 != TComm::ALL_OK)
							{
								if(error1 != TComm::ALL_OK)
								{
//									#ifndef NDEBUG
//									pMono->SaveWindow();
//									pMono->Goto(25, 40);
//									pMono->Out("    ");
//									pMono->RestoreWindow();
//									#endif
									return(error1);
								}
							}
						}
						break;
			
					case DATA_ACK:
					case DATA_NAK:
						tempTimers[TIMER_NQ_ACKNAK].Start();
						// first - do we have any packets in the waiting room?
						if(totalPacketsWaiting)
						{
							int	loop1;
							sPacket	*pWaiter;

							if(pAck->header.ID)	// if specific ID is known..
							{
								// find that packet
								for(loop1 = 0, pWaiter = waitingRoom; loop1 < totalPacketsWaiting;
									loop1++, pWaiter++)
								{
									if(pWaiter->header.wSequence == pAck->header.ID)
									{
										if(pAck->header.wType == DATA_ACK)
										{
											#ifndef NDEBUG
											ComDebug.Out("(EnQ[ACK4wSeq%d])..", pAck->header.ID);
											#endif
											// he can stop waiting now
											RemoveFromWaitingRoom(pWaiter);
										}
										else
										{
											#ifndef NDEBUG
											ComDebug.Out("(EnQ[NAK4wSeq%d])..", pAck->header.ID);
											#endif
											// re-send requested, issue new packet w/ same data
											RemoveFromWaitingRoom(pWaiter);
											SendData(pWaiter, FALSE);
										}
										pWaiter = NULL;
										loop1 = totalPacketsWaiting;
									}
								}
								if(pWaiter == NULL)
								{
									// found the target - discard packet
									pAck->header.wType = EMPTY_PACKET;
									DecrTail();
									break;
								}
								else
								{
									// else ack/nak may be for packet currently being
									// transmitted - leave reply in queue.
									#ifndef NDEBUG
									ComDebug.Out("(EnQ[%s4wSeq%d])..", (pAck->header.wType == DATA_ACK)?
										"ACK": "NAK", pAck->header.ID);
									#endif
								}
							}
							else
							{
								#ifndef NDEBUG
								ComDebug.Out("(EnQ[%s4wSeq%d])..", (pAck->header.wType == DATA_ACK)?
									"ACK": "NAK", pAck->header.ID);
								#endif

								if(pAck->header.wType == DATA_NAK)
								{
									// since reply's target is unknown, re-send everybody
									// in the waiting room, AND leave reply in queue
									// in case a packet is currently being sent also

 									ResendWaiters();
								}
							}
							break;
						}

						#ifndef NDEBUG
						ComDebug.Out("(EnQ[%s4wSeq%d])..", (pAck->header.wType == DATA_ACK)?
							"ACK": "NAK", pAck->header.ID);
						#endif
						tempTimers[TIMER_NQ_ACKNAK].Stop();
						// leave it in the queue
						break;
					
					case EMPTY_PACKET:
						// ignore it, leave it in the queue
						break;

					case CONNECT:
					case CONNECT_OK:
						// these are used by the pComm instance while searching for
						// other players.  We can ignore them.
						pAck->header.wType = EMPTY_PACKET;

						// point tail back to that slot for re-use
						DecrTail();
						break;

					default:
						#ifndef NDEBUG
						ComDebug.Out("TCommMgr::EnQueueData(): Unknown packet wType%d, sending NAK\n", pAck->header.wType);
						#endif
						SendAckNak(pAck, FALSE);
						break;
				}
				break;
		}
	}
//   else
//   {
//		ComDebug.Out("EnQueueData(): pComm->IsPacketAvailable() == FALSE\n");

//		if (pComm->GetError () == TComm::PACKET_FAILED)
//      {
////			ComDebug.Out("EnQueueData(): PACKET_FAILED\n");
//
//			sPacket* pAck = (sPacket*) (queue[tail]);
//         if (head != tail)
//         {
//           int last = (tail - 1 + tcommmgrNBLOCKS) % tcommmgrNBLOCKS;
//            sPacket* pLast = (sPacket*) (queue[last]);
//            if (pLast->header.wType == BAD_PACKET_TYPE) // We already have a bad packet
//            {
//               return (retv);
//            }
//         }
//         retv = tail; // indicate where the data was placed
////			ComDebug.Out("IncrTail()\n");
//			IncrTail ();
//         pAck->header.wType = BAD_PACKET_TYPE;
//      }
//   }

	if(fDebug)
	{
		pMono->SaveWindow();
		pMono->Goto(23, 1);
		pMono->Out("Emp[%4d] Rx[%4d] fPA[%d] ",
			tempTimers[TIMER_NQ_EMPTIES].duration, tempTimers[TIMER_NQ_RX].duration,
			fPacketAvailable? 1: 0);
		pMono->RestoreWindow();
	}

//	#ifndef NDEBUG
//	pMono->SaveWindow();
//	pMono->Goto(25, 40);
//	pMono->Out("    ");
//	pMono->RestoreWindow();
//	#endif

	return (retv);
}

// we have a special routine for these two, because they require special
// handling
TComm::ERROR
TCommMgr::SendAckNak(sPacket *pPacket, bool fOkay)
{
	#ifndef NDEBUG
	ComDebug.Out("(Send%s4wSeq%d..", fOkay? "ACK": "NAK",
		pPacket->header.wSequence);
	#endif
	sPacket ackPacket;
	TComm::ERROR err1;

	ackPacket.header.wType = fOkay? DATA_ACK: DATA_NAK;
	if(pComm->fDataRequired)
	{
		ackPacket.header.len = 4;
		ackPacket.pData = dummySpace;
	}
	else
	{
		ackPacket.header.len = 0;
		ackPacket.pData = NULL;
	}

	ackPacket.header.destID = pPacket->header.sendID;
	ackPacket.header.ID = pPacket->header.wSequence;

//	// blindly send reply twice, to decrease odds of reply corruption
	pComm->SendPacket (&ackPacket);
	#ifndef NDEBUG
	ComDebug.Out("Sent[wSeq%d]", ackPacket.header.wSequence);
	#endif

	if(pComm->GetError())
	{
		// trouble on our side. Well, wait a second and try yet again
		#ifndef NDEBUG
		ComDebug.Out("..ERR,RESEND..");
		#endif

		sleep(1);
		pComm->SendPacket(&ackPacket);
		#ifndef NDEBUG
		ComDebug.Out("Sent[wSeq%d]..", ackPacket.header.wSequence);
		#endif
	}
	err1 = pComm->GetError();
	#ifndef NDEBUG
	if(err1)
		ComDebug.Out("..ERR%d)", err1);
	else ComDebug.Out("..OK)");
	#endif
	return(err1);
}

sPacket* TCommMgr::DeQueueData ()
{
  sPacket*		pRetv = NULL;

   // Remove all of the empty packets
	RemoveEmpties ();

	if (head != tail)
	{
		pRetv = (sPacket*) (queue[head]);
//		ComDebug.Out("DeQueueData(): IncrHead()\n");
		IncrHead ();
	}
	return (pRetv);
}

bool
TCommMgr::RemoveFromWaitingRoom(sPacket* pPacket)
{
	int		loop1;
	sPacket	*pSlot;

	for(loop1 = 0, pSlot = &waitingRoom[0]; loop1 < totalPacketsWaiting;
		loop1++, pSlot++)
	{
		if(pSlot->header.wSequence == pPacket->header.wSequence)
		{
			ComDebug.Out("REMFROMWAIT[wSeq%d]..", pPacket->header.wSequence);

			// if any packets in the queue after this
			if(totalPacketsWaiting > loop1 + 1)
			{
				// compress the array
				memmove(&waitingRoom[loop1], &waitingRoom[loop1 + 1],
					sizeof(sPacket) * totalPacketsWaiting - (loop1 + 1));
				memmove(waitingRoomData[loop1], waitingRoomData[loop1 + 1],
					sizeof(tcommmgrTOTAL_BLOCK_SIZE) * totalPacketsWaiting - (loop1 + 1));
			}
			totalPacketsWaiting--;
			return(TRUE);
		}
	}

	// packet could not be found
	return(FALSE);
}

bool
TCommMgr::AddToWaitingRoom(sPacket* pPacket)
{		
	if(totalPacketsWaiting == tcommmgrMAX_PACKETS_WAITING)
	{
		// no room left
		return(FALSE);
	}

	if(sizeof(*pPacket) + pPacket->header.len > tcommmgrTOTAL_BLOCK_SIZE)
	{
		// too big, throw it back
		return(FALSE);
	}

	ComDebug.Out("ADD2WAIT[wSeq%d]..", pPacket->header.wSequence);
	memcpy(&waitingRoom[totalPacketsWaiting], pPacket, sizeof(*pPacket));
	if(pPacket->header.len)
	{
		// bring along his baggage
		memcpy(waitingRoomData[totalPacketsWaiting], pPacket->pData, pPacket->header.len);
		waitingRoom[totalPacketsWaiting].pData = waitingRoomData[totalPacketsWaiting];
	}
	else
	{
		// traveling light
		waitingRoom[totalPacketsWaiting].pData = NULL;
	}
	totalPacketsWaiting++;
	return(TRUE);
}

TComm::ERROR
TCommMgr::ResendWaiters(void)
{
	// resend all packets in waiting room, preserving their wSeq #s.
	// that way, if an ACK is already en-route to us but just not here yet,
	// we won't invalidate it and cause unnecessary delays

	int				loop1;
	sPacket			*pWaiter;
	TComm::ERROR	err1;

	err1 = TComm::ALL_OK;
	for(loop1 = 0, pWaiter = waitingRoom; loop1 < totalPacketsWaiting && !err1;
		loop1++, pWaiter++)
	{
		err1 = pComm->SendPacket(pWaiter, TRUE);
	}
	ComDebug.Out("Sent..");
	return(err1);
}

int TCommMgr::_SendData (sPacket* pPacket, bool fWaitForAck)
{
	bool				fPacketSent = FALSE, fDone = FALSE, fIsResend = FALSE;
	TComm::ERROR	error1;
	int				ack = 0, retries = 0, enQCount = 0, waitCount = totalPacketsWaiting;
	clock_t 			timeout;

	#ifndef NDEBUG
	ComDebug.Out("_SendData(%s)..", fWaitForAck? "WAIT": "NOWAIT");
	tempTimers[TIMER_TOTAL].Start();
	#endif

	pPacket->header.sendID = pComm->GetUserID ();
	pComm->ClearError ();

	// at this point, any ACK/NAKs already in the queue are useless.  If they
	// were for packets in the waitingRoom, that's already handled in EnQueueData().
	// We're not (yet) in a Send-and-wait mode, so they're aren't for us (though
	// they might have been left in the queue by EnQueue() in case they were
	// for us.)  Anyway, delete them now.
	RemoveOldReplies();

	// if the previous cycle's packet is still awaiting ACK, wait until
	// it clears before we proceed (this assumes that the app insists on
	// packets arriving in a FIFO manner)
	tempTimers[TIMER_WAIT1].duration = 0;
	tempTimers[TIMER_WAIT2].duration = 0;
	tempTimers[TIMER_WAIT_RESEND].duration = 0;

	if(totalPacketsWaiting)
	{
		ComDebug.Out("WaitRoom[%d] ", totalPacketsWaiting);
		tempTimers[TIMER_WAIT1].Start();

		// the ACK is probably en-route, so just chill..
		timeout = clock() + pComm->GetTimeout() / 2;
		while(totalPacketsWaiting && clock() < timeout)
		{
			EnQueueData(TRUE);
			enQCount++;
			if(pComm->GetError())
			  return(pComm->GetError());
		}

		tempTimers[TIMER_WAIT1].Stop();
		if(totalPacketsWaiting)
		{
			ComDebug.Out("[%d] left..", totalPacketsWaiting);

			// if any of them cleanly disappeared in transit, we won't even get
			// a NAK for them, so play it safe and resend everyone still waiting.
			tempTimers[TIMER_WAIT_RESEND].Start();
			ResendWaiters();
			tempTimers[TIMER_WAIT_RESEND].Stop();

			// NOW let's see if we get a response
			tempTimers[TIMER_WAIT2].Start();
			timeout = clock() + pComm->GetTimeout() / 2;
			while(totalPacketsWaiting && clock() < timeout)
			{
				EnQueueData();
			}
			tempTimers[TIMER_WAIT2].Stop();
		}

		if(totalPacketsWaiting)
		{
			ComDebug.Out("[%d] left..", totalPacketsWaiting);
			// well, let's hope the other machine doesn't need them
		}
		else ComDebug.Out("CLEAR..");
	}

	#ifndef NDEBUG
	ComDebug.Out("[");
	#endif
	timeout = clock() + pComm->GetTimeout() / 3;
	retries = 0;
	tempTimers[TIMER_SEND].Start();
	while(!fDone)
	{
		if(clock() >= timeout)
		{
			if(retries < 2)
			{
				// give it one more try..
				retries++;
				timeout = clock() + pComm->GetTimeout() / 3;
				ComDebug.Out("ACK_TIMEOUT1[wSeq%d]..", pPacket->header.wSequence);
				fPacketSent = FALSE;
			}
			else
			{
				// failure
				fDone = TRUE;
				ComDebug.Out("ACK_TIMEOUT2[wSeq%d]..", pPacket->header.wSequence);
				continue;
			}
		}

		// if we havent sent out the packet yet (or need to resend)..
		if(!fPacketSent)
		{
			#ifndef NDEBUG
			ComDebug.Out("Send..");
			#endif

			pComm->ClearError ();
			pComm->SendPacket (pPacket, fIsResend);
			fIsResend = TRUE;	// in case we do have to re-send
			#ifndef NDEBUG
			ComDebug.Out("SENT[wSeq%d]..", pPacket->header.wSequence);
			#endif
			error1 = pComm->GetError();
			if(error1)
			{
				#ifndef NDEBUG
				ComDebug.Out("SErr%d[%s] wSeq[%d]", pErrorStrings[error1], pPacket->header.wSequence);
				#endif
				switch(error1)
				{
					case TComm::CONNECTION_FAILED:
					case TComm::TIMEOUT_BUFFER_FULL:
						// these are very bad
						#ifndef NDEBUG
						ComDebug.Out("]");
						#endif
						return(error1);

					default:
						#ifndef NDEBUG
						ComDebug.Out("..");
						#endif
						break;
				}
			}																					  
			else
			{
				#ifndef NDEBUG
				ComDebug.Out("Sent[wSeq%d]..", pPacket->header.wSequence);
				#endif

				fPacketSent = TRUE;

				// if we're don't want to sit here and wait for ACK/NAK
				if(!fWaitForAck)
				{
					// put this packet in the waiting room, in case a problem
					// occurs later.
					if(AddToWaitingRoom(pPacket))
					{
						// pretend we're done
						ack = 1;
						fDone = TRUE;
						continue;
					}
					else
					{
						// nevermind, cant do it right now (maybe waiting room
						// is full)
						fWaitForAck = FALSE;
					}
				}
			}
		}

		// look for an ACKnowledgement packet
		int last = EnQueueData ();

		// if any critical problems, bail
		error1 = pComm->GetError();
		if(error1)
		{
			#ifndef NDEBUG
			ComDebug.Out("WErr%d[%s]", error1, pErrorStrings[error1]);
			#endif
			switch(error1)
			{
				case TComm::CONNECTION_FAILED:
				case TComm::TIMEOUT_BUFFER_FULL:
					// these are very bad
					#ifndef NDEBUG
					ComDebug.Out("]");
					#endif
					return(error1);

				default:
					#ifndef NDEBUG
					ComDebug.Out("..");
					#endif
					break;
			}
		}

		if (last != -1)
		{
			// peek into the queue and spy on the most recently received
			// packet, watching for an ACK/NAK for us
			sPacket* pAck = (sPacket*) (queue[last]);

			switch (pAck->header.wType)
			{
				case DATA_ACK :
					if (pAck->header.ID == pPacket->header.wSequence)
					{
						ack = TRUE;
						fDone = TRUE;
						tail = last;
						#ifndef NDEBUG
						ComDebug.Out("RxACK");
						#endif
					}
					// the following is now handled in EnQueueData()
//					// else anyone int the waiting room?
//					else if(totalPacketsWaiting)
//					{
//						int		loop1;
//						sPacket	*pWaiter;
//
//						for(loop1 = 0, pWaiter = &waitingRoom[0]; loop1 < totalPacketsWaiting;
//							loop1++, pWaiter++)
//						{
//							if(pAck->header.ID == pWaiter->header.wSequence)
//							{
//								// okay, that packet is free to go now
//								RemoveFromWaitingRoom(pWaiter);
//								loop1 = totalPacketsWaiting;
//								#ifndef NDEBUG
//								ComDebug.Out("ACK4wSeq%d", pAck->header.ID);
//								#endif
//								continue;
//							}
//						}
//
//						// at this point, disregard the ACK no matter what.
//					}
					pAck->header.wType = EMPTY_PACKET;
					break;

//				case BAD_PACKET_TYPE :
//					// this is sent to us when packet header integrity is comprimised.
//					pAck->header.wType = EMPTY_PACKET;
//					tail = last;
//					fPacketSent = FALSE;	// re-send
//					pComm->SetError(TComm::BAD_PACKET_TYPE);
//					ComDebug.Out("BAD..");
//					break;

				case DATA_NAK :
					if(pAck->header.ID == pPacket->header.wSequence)
					{
						// resend the current packet only
						fPacketSent = FALSE;	// re-send
						#ifndef NDEBUG
						ComDebug.Out("RxNAK[wSeq%d]..", pAck->header.ID);
						#endif
					}
					else if(totalPacketsWaiting)
					{
						// check the waiting room
						int	loop1;
						sPacket	*pWaiter;

						for(loop1 = 0, pWaiter = &waitingRoom[0]; loop1 < totalPacketsWaiting;
							loop1++, pWaiter++)
						{
							if(pWaiter->header.wSequence == pAck->header.ID)
							{
								// re-send this guy
								#ifndef NDEBUG
								ComDebug.Out("RxNAK[wSeq%d]..", pAck->header.ID);
								#endif
								SendData(pWaiter, FALSE);
								loop1 = totalPacketsWaiting;
								pWaiter = NULL;
							}
						}
						if(pWaiter == NULL)
						{
							// the packet to resend could not be identified, so resend
							// the current packet AND all packets in the waiting room.
							// It is assumed that all packets have been uniquely serialized,
							// so redundant sends won't hurt anything
							#ifndef NDEBUG
							ComDebug.Out("RxNAK[wSeq?]..");
							#endif

							for(loop1 = 0; loop1 < totalPacketsWaiting; loop1++)
								SendData(&waitingRoom[loop1], FALSE);

							fPacketSent = FALSE;	// trigger re-send of current packet
						}
					}
					pAck->header.wType = EMPTY_PACKET;
					tail = last;
					break;

				case _XFER_BLOCK_DATA:
					// received a data packet while waiting for ACK/NAK in the meantime.
					// Note it and go on.
					#ifndef NDEBUG
					ComDebug.Out("(RxData[wSeq%d])..", pAck->header.wSequence);
					#endif
					break;

				default:
					break;
			}
		}
	}
	tempTimers[TIMER_SEND].Stop();

	if (!ack)
	{
		if(GetError())
			return(GetError());

//		ComDebug.Out("  No ACK - SEND_FAILED\n");
		#ifndef NDEBUG
		ComDebug.Out("ACK_TIMEOUT");
		#endif
		pComm->SetError (TComm::TIMEOUT_NO_ACK);
	}
	else if(!fWaitForAck)
	{
		// store packet in "unACKed qeue" for later removal,
		// and go on with life for now
	}

	#ifndef NDEBUG
	ComDebug.Out("]\n");
	tempTimers[TIMER_TOTAL].Stop();
	pMono->SaveWindow();
	pMono->Goto(24, 1);
	pMono->Out("Tot[%5d] WC[%d] W1[%4d] W2[%4d] WR[%4d] Tx[%4d] NqC[%4d] ",
		tempTimers[TIMER_TOTAL].duration, waitCount, tempTimers[TIMER_WAIT1].duration,
		tempTimers[TIMER_WAIT2].duration, tempTimers[TIMER_WAIT_RESEND].duration,
		tempTimers[TIMER_SEND].duration, enQCount);
	pMono->RestoreWindow();
	#endif
	return (GetError());
}

int TCommMgr::SendData (int to, int type, int len, void* pData)
{
	sPacket packet;

	// Simply repackage and send on
	// Any fields not filled in should be filled in by engine code
	packet.header.ID = type;
	packet.header.len = len;
	packet.header.destID = to;
	packet.pData = pData;
	return (SendData (&packet));
}

int TCommMgr::SendData (sPacket* pPacket, bool fWaitForAck)
{
	pComm->ClearError (); // BUGBUG is this valid?
	// Can we send this as a single block?
	pPacket->header.wType = _XFER_BLOCK_DATA;
	if (pPacket->header.len < tcommmgrMAX_PACKET_SIZE)
	{
		return (_SendData (pPacket, fWaitForAck));
	}

	// Now we have to break this up and send each part
	sMultiBlockInfo mbi;
	sPacket packet;
	
	memset (&mbi, 0, sizeof (mbi));
	mbi.nBlocks = pPacket->header.len / tcommmgrMAX_PACKET_SIZE +
			(pPacket->header.len % tcommmgrMAX_PACKET_SIZE ? 1 : 0);
	mbi.nBytes = pPacket->header.len;

	int bytes_to_send = mbi.nBytes;

	// Setup the send header
	memset (&packet, 0, sizeof (packet));
	packet.header.wType = BLOCK_HEADER;
	packet.header.destID = pPacket->header.destID;
	packet.header.len = sizeof (mbi);
	packet.pData = &mbi;

	// Send and verify the header
   if (_SendData (&packet))
	{
		return (pComm->GetError ());
	}

// BUGBUG There is no mechanism for verifying the receiver is waiting for this block
//	if (!pComm->ReceivePacket (&packet) || packet.header.wType != BLOCK_HEADER_ACK)
//	{
//		SetError (SEND_FAILED);
//		return (GetError ());
//	}

	// Send each of the blocks we wish sent

	packet.header.wType = BLOCK_DATA;
	packet.header.destID = pPacket->header.destID;
	for (int i=0;i<mbi.nBlocks;i++)
	{
		packet.header.ID = i;
		packet.header.len = min (tcommmgrMAX_PACKET_SIZE, bytes_to_send);
		bytes_to_send -= packet.header.len;
		packet.pData = (char*)pPacket->pData + tcommmgrMAX_PACKET_SIZE * i;
      if (_SendData (&packet))
		{
			return (pComm->GetError ());
		}
	}

	return (pComm->GetError ());
}

int TCommMgr::DataAvailable (sPacket* pPacket)
{
	// Start by queueing up all the currently available data 
	// BUGBUG this may cause overflowing of the receive queue
//	ComDebug.Out("DataAvailable(): EnQueueData()..\n");
	while (EnQueueData () != -1 && !GetError())
	{
		;
	}

	if(GetError())
		return(0);
	
//	ComDebug.Out("DataAvailable(): EnQueueData().. done.  dataAvail==%d tail=%d head=%d\n",
//		(int)dataAvail, (int)tail, (int)head);

	if (!dataAvail && (tail != head))
	{
		sPacket* pLast = (sPacket*) (queue[head]);
		switch (pLast->header.wType)
		{
			case _XFER_BLOCK_DATA :
				dataAvail = 1;
				pComm->ClearError ();
//				ComDebug.Out("XFER_BLOCK_DATA\n");
				break;

			case BLOCK_HEADER :
			// BUGBUG need to implement Large packet receipt
				dataAvail = 1;
				ReceiveLargePacket ();
//				ComDebug.Out("BLOCK_HEADER\n");
				break;

			case _XFER_BLOCK_ID :
//				ComDebug.Out("XFER_BLOCK_ID\n");
				DiscardData ();
				break;

			case DATA_ACK:
			case DATA_NAK:
//				ComDebug.Out("TCommMgr::DataAvail() - ignoring extraneous ACK/NAKfor %d\n",
//					pLast->header.ID);
				DiscardData();
				break;

			default :
//				ComDebug.Out("UNKNOWN_PACKET\n");

//				// dont set an error, because this is not necessarily a stop condition
//				pComm->SetError (TComm::UNKNOWN_PACKET);
				#ifndef NDEBUG
				ComDebug.Out("CommMgr::DataAvail(): unknown packet type %d\n", pLast->header.wType);
				#endif
				DiscardData ();
				break;
		}
	}

// Copy the data to the packet header passed in
	if (dataAvail && pPacket)
	{
		memcpy (pPacket, (queue[head]), sizeof (pPacket));
	}

	return (dataAvail);
}

int TCommMgr::ReceiveData (sPacket* pPacket)
{
	#ifndef NDEBUG
//	ComDebug.Out("ReceiveData() packets==%d", ((tail - head) + tcommmgrNBLOCKS)
//		% tcommmgrNBLOCKS);
	#endif
	if (!dataAvail && !DataAvailable ())
	{
		return (1);
	}

	if(_ReceiveData (pPacket))
	{
		#ifndef NDEBUG
//		ComDebug.Out(", recvd wSeq%d\n", pPacket->header.wSequence);
		#endif
	}
	else 
	{
		#ifndef NDEBUG
//		ComDebug.Out(", _ReceiveData() error!\n");
		#endif
	}

	return (0);
}

int TCommMgr::DiscardData ()
{
	dataAvail = 0;
	DeQueueData ();
	return (TComm::ALL_OK);
}

int
TCommMgr::DumpQueue(void)
{
	int loop1, loop2;
	sPacket	*pPacket;

	for(loop1 = 0, loop2 = head; loop1 < tcommmgrNBLOCKS; loop1++, loop2 = ++loop2 % tcommmgrNBLOCKS)
	{
		pPacket = (sPacket *)(queue[loop2]);
		#ifndef NDEBUG
		ComDebug.Out("queue[%d]: wType=%d(%s) wSeq%d\n", loop2, pPacket->header.wType,
			pBlockIDStrings[pPacket->header.wType], pPacket->header.wSequence);
		#endif
	}
	return(1);
}

int
TCommMgr::_ReceiveData (sPacket* pPacket)
{
	int retv = 0;

	#ifndef NDEBUG
	ComDebug.Out("_RxData()..");
	#endif
	if (dataAvail)
	{
		sPacket* pCommPacket = DeQueueData ();

		memcpy (&pPacket->header, &pCommPacket->header, sizeof (pPacket->header));
		if(pPacket->header.len)
			memcpy (pPacket->pData, pCommPacket->pData, pPacket->header.len);

		ComDebug.Out("Rx[wSeq%d]\n", pPacket->header.wSequence);
		int totalPackets, loop1, currPacket;

		if(tail < head)
			totalPackets = (tail + tcommmgrNBLOCKS) - head;
		else totalPackets = tail - head;

//		ComDebug.Out("%d] TCommMgr::_ReceiveData() : packets=%d head=%d tail=%d\n", 
//			(int)ATicks(), totalPackets, head, tail);
		for(loop1 = 0, currPacket = head; loop1 < totalPackets; loop1++)
		{
//			ComDebug.Out("  %d] ID=%d\n", currPacket, ((sPacket *)queue[currPacket])->header.ID);
			currPacket = (currPacket + 1) % tcommmgrNBLOCKS;
		}

	// Let the system know we are done with its buffer
		dataAvail = 0;
		retv = 1;
	}
	else ComDebug.Out("NO DATA\n");
	return (retv);
}


// BUGBUG this is not finished
int
TCommMgr::ReceiveLargePacket ()
{
	int	received[100];	// BUGBUG allows up to 100 blocks
	sPacket	block;
	sMultiBlockInfo mbi;
	int	nReceived = 0;
	int	i;

	// Acknowledge recipt of the last header
	block.pData = &mbi;
	_ReceiveData (&block);

	pComm->ClearError ();
	memset (received, 0, sizeof (received));

	clock_t timeout = clock() + pComm->timeout;
	
// This is the error we expect
	pComm->SetError (TComm::TIMEOUT);

	while (nReceived < mbi.nBlocks && timeout >= clock())
	{
		while (EnQueueData () != -1 && !GetError())
		{
			;
		}

		if(GetError())
			return(GetError());

		while (head != tail && nReceived < mbi.nBlocks)
		{
		  sPacket* pPacket = (sPacket*) (queue[head]);
		  sPacket  packet;

			if (pPacket->header.wType == BLOCK_DATA)
			{
				// Copy data pointed to by pBuffer to scratch area
				packet.pData = GetBuffer () + pPacket->header.ID * tcommmgrMAX_PACKET_SIZE;
				dataAvail = 1;
				_ReceiveData (&packet);
				received[packet.header.ID] = 1;
				nReceived++;
				timeout = clock() + pComm->timeout; // BUGBUG this stretches the idea of timeout a bit
			}
			else
			{
				pComm->SetError (TComm::UNEXPECTED_PACKET);
				return (pComm->GetError ());
			}
		}
	
		// Make sure we have recevied all blocks
		for (i=0;i<mbi.nBlocks;i++)
		{
			if (!received[i])
			{
				break;
			}
		}
		if (i == mbi.nBlocks)
		{
			pComm->ClearError ();
			break;
		}
	}

	head = (head - 1 + tcommmgrNBLOCKS) % tcommmgrNBLOCKS;
	sPacket* pCommPacket = (sPacket*) queue[head];

// BUGBUG This is bad form, we are writing back into the data structures
// of another class
	memcpy (pCommPacket, &block.header, sizeof (block.header));

	pCommPacket->header.dataChecksum = mbi.checksum;
	pCommPacket->header.len = mbi.nBytes;
	pCommPacket->pData = GetBuffer ();

	dataAvail = 1;

	return(GetError ());
}

bool
TCommMgr::Save(uint16 state, FILE *fp)
{
	// get rid of unreferenced warning
	fp = fp;

	bool		error = FALSE;
//	grip		gMgr = 0;
//	TCommMgr	*pTempMgr;

	switch(state)
	{
		case BEFORE_SAVE:
			break;

		case DURING_SAVE:
//			_disable();				// stop messages
//			// write out mgr
//			if (fwrite(this, 1, sizeof(TCommMgr), fp) != sizeof(TCommMgr))
//			{
//				// some sort of error
//				error = TRUE;
//				break;
//			}
//			_enable();				// start messages
			break;

		case AFTER_SAVE:
			break;

		case BEFORE_RESTORE:
			break;

		case DURING_RESTORE:
//			_disable();				// stop messages
//			// read old mgr into temp
//			TRACK_MEM("TmpCommMgr");	gMgr = AMalloc(sizeof(TCommMgr));
//			pTempMgr = ADerefAs(TCommMgr, gMgr);
//
//			// run in a while loop so that we can break on an error
//			// and clean up properly
//			while(TRUE)
//			{
//				// read the graph manager
//				if (fread(pTempMgr, 1, sizeof(SoundMgr), fp) != sizeof(SoundMgr))
//				{
//					// some sort of error
//					error = TRUE;
//					break;
//				}
//				break;
//			}
//
//			// if we allocated memory, free it
//			if (gMgr)
//			{
//				AFree(gMgr);
//			}
//
//			_enable();				// start messages

			break;

		case AFTER_RESTORE:
			break;
	}
	return error;
}

#if 0
int TCommMgr::ReceiveData (int* pType, int* pLen, void* pBuffer)
{
	sPacket packet;
	packet.pData = pBuffer;
	ReceiveData (&packet);
	*pType = packet.header.wType;
	*pLen = packet.header.len;
}
#endif

