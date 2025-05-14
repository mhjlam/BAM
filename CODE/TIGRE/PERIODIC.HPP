//
// PERIODIC.HPP
//
// December 09, 1993
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#ifndef	periodic_hpp
#define	periodic_hpp

#include "tigre.hpp"
#include "types.hpp"
#include "clock.hpp"



//---------[Periodic]--------------------------------------------

class Periodic : public Object
{
	public:

		Periodic();
		virtual			~Periodic();
			 	
		virtual void	Cycle();
		virtual void 	Service();
		virtual void	SetClock(grip gNewClock);

		virtual void	Set_Script(void* pScript, grip gSuccessor = 0);
		virtual void	ScriptAborting();

		void				SetInterval(ticks_t interval);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char		periodicDataStart;
		grip		gScript;
		grip		gClock;

		uint32	interval;
		uint32	timer;

	protected:

	//private:
		
		char		periodicDataEnd;
};



//---------[Script]--------------------------------------------

class Script : public Periodic
{
	public:

		Script();
		~Script();

		virtual void	Setup(grip gContext, Periodic *pOwner = NULL, grip toCue = 0);
		virtual void	Cleanup();
		virtual void	Abort();

		virtual bool	HandleMsg(Message* pMsg);

		virtual void	CountDown(ticks_t ticks);

		virtual void	Service();

		uint16	state;
		ticks_t	scriptTimer;

		grip		gOwner;
		grip		gSuccessor;

		notice_t	completionNoticeType;

	protected:

		virtual void	Cue();
		
		bool		fIsActive;
};


#endif 

