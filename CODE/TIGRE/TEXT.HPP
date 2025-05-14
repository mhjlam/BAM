//
// TEXT.HPP
//
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
// 02-03-1994
//
//	Various objects which retrieve or display text
//
//----[]-------------------------------------------------------------

#ifndef	text_hpp
#define	text_hpp

#include "types.hpp"
#include "resmgr.hpp"


//---------------------------------------------------------------------
//---------[ Squib ]---------------------------------------------------

// Squib resources begin with a variable-length array of SquibHeads,
// followed by zero-terminated strings.
//
struct RSquibHead
{
	uint	offset;
	uint	id;
};

struct RSquibData
{
	uint			cSquibs;
	RSquibHead	aSquibHeads[1];		// variable-length array
};


class SquibRes : public AutoRes
{
	public:
							SquibRes(void);
		virtual char*	Load(uint resNum, int squibId, bool handErrorBack = FALSE);
		virtual void	Cleanup(void);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char	squibDataStart;
		char	squibDataEnd;
};

enum t_language
{ LANG_ENGLISH = 0, LANG_GERMAN, LANG_FRENCH
};
#define LANG_DEFAULT LANG_ENGLISH
void	SetLanguage(int newLangNum);

//---------------------------------------------------------------------
//---------[ Conversation ]--------------------------------------------

#define		MAX_CON_EXCHANGES	26

struct RExchange
{
	uint16		talkerId;
	uint16		soundId;
	uint16		offset;
};


// this is a structure of a conversation as load from the resource
//
struct RConversation
{
	RExchange	aExchanges[MAX_CON_EXCHANGES];
};


	
#ifdef USING_RESPONDER
	
	Responder class is not in use.


//---------[ Responder ]------------------------------------------------
//
class Responder : public Periodic
{
	public:

		Responder();

		virtual bool	HandleMsg(Message* pMsg);
		void				Setup(uint res, uint squibId);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		Rectangle		rect;

		char				responderDataStart;
		uint				resNum;
		uint				baseSquibId;
		char				responderDataEnd;
};



//---------[ UtilResponder ]-------------------------------------------
//
// Responds to clicks for actor
//
class UtilResponder : public Utility
{
	public:

		UtilResponder();
		UtilResponder(uint res, uint squibId);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);
		
		virtual void	Setup(void* pActor);
		virtual void	Cleanup();
		virtual bool	HandleMsg(Message*);

		Responder	responder;

		char	utilResponderDataStart;
		grip			gScrim;
		char	utilResponderDataEnd;
};

#endif	// USING_RESPONDER

#endif	// text_hpp
