//
// Context.HPP
//
// December 09, 1993
// WATCOM: September 26, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------


#ifndef	context_hpp
#define	context_hpp


#include "keylist.hpp"
#include "manager.hpp"
#include "periodic.hpp"
#include "types.hpp"


#define	CT_MSGS	0x01
#define	CT_SERV	0x02
#define	CT_ROST	0x04
#define	CT_ALL	0x07


//---------[ Context ]--------------------------------------------

class Context : public Periodic
{
	public:

		Context(void);
		virtual			~Context(void);

		virtual void	Cycle(void);
		virtual bool	HandleMsg(Message* pMessage);
		virtual void	Activate(bool fOn);

		uint32	key;

		List		lRoster;
		List		lServiceables;
		KeyList	lReceivers;

		void		AddObject(grip gObj, uint16 flags, int32 pri = PRIORITY_MAX/2);
		void		CutObject(grip gObj, uint16 flags);

		void		ResetMsgPri(grip gObj, int32 pri);

		bool			fIsActive;
		modalType	mode;

		grip			gModerator;

	protected:

		void		AddRoster(void);
		void		CutRoster(void);

		void		AddScrimage(grip gActor);
		void		CutScrimage(grip gActor);

		void		AddReceiver(grip gObj, int32 pri);
		void		CutReceiver(grip gObj);

		void		AddServiceable(grip gPeriodic);
		void		CutServiceable(grip gPer);

		void		ClearLists(void);
};




//---------[ Context Manager ]------------------------------------

class ContextMgr : public Manager, public Periodic
{
	public:

		ContextMgr(void);
		virtual			~ContextMgr(void);

		virtual bool	HandleMsg(Message* pMessage);
		virtual void	Cycle(void);
		
		void		Quit(void);

		void		Add(grip gContext);
		void		Cut(grip gContext);

		SysList	lContexts;

		bool		fQuitting;
};

extern ContextMgr* pContextMgr;

typedef	bool	(*HandleMsgProc)(Message* pMsg);
extern HandleMsgProc	ppGlobalMsg;

#endif 

