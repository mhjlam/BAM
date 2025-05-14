//
// TIGRE.HPP
//
// November 12, 1993
// WATCOM: September 23, 1994  BKH
// (C) 1993, Tachyon, Inc.  All rights reserved.
//
//	General definitions.
//
//----[]-------------------------------------------------------------


#ifndef tigre_hpp
#define tigre_hpp

#include <assert.h>		// must include for all files containing NDEBUG
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.hpp"


#define	PRIORITY_MAX	LONG_MAX


//----[ Message Structure ]------------------------------------------------

const evt_t	E_NULL			= 0x0001;
const evt_t	E_MOUSE_UP		= 0x0002;
const evt_t	E_MOUSE_DOWN	= 0x0004;
const evt_t	E_KEY_UP			= 0x0008;
const evt_t	E_KEY_DOWN		= 0x0010;	
const evt_t	E_DIRECTION		= 0x0020;	// not yet implemented
const evt_t	E_DEFINED		= 0x0080;	
const evt_t	E_ALL				= 0xFFFF;

// Notice specific data struct
struct Notice
{
	notice_t	type;
	grip		gDest;
	grip		gSource;
	void*		param;

};

// Event specific data struct
struct Event
{
	evt_t		type;
	int32		value;
	uint32	ticks;
	uint16	modifiers;
	coord		x;
	coord		y;

};

// Message struct (either Event or Notice)
struct Message
{
	msg_t	type;

	union
	{
		Event		event;
		Notice	notice;
	};
};

// Modifiers mask
const uint16	MOD_INSERT			= 0x0080;
const uint16	MOD_CAPS				= 0x0040;
const uint16	MOD_NUM_LOCK		= 0x0020;
const uint16	MOD_SCROLL_LOCK	= 0x0010;
const uint16	MOD_ALT				= 0x0008;
const uint16	MOD_CTRL				= 0x0004;
const uint16	MOD_R_SHIFT			= 0x0002;
const uint16	MOD_L_SHIFT			= 0x0001;
const uint16	MOD_SHIFT			= (MOD_R_SHIFT | MOD_L_SHIFT);


// true for all tigre implementations
const	ticks_t	TICKS_PER_SEC	= 20;

enum ClassID
{ CID_TMOV = 0, CID_OBJECT, CID_TRANDOM, CID_SCRIM, CID_UTILITY, CID_GUY,
	CID_UTILIZER, CID_TLINETO, CID_WALKTO, CID_ANIMATOR, CID_SQUIB, CID_RESPONDER,
	CID_UTILRESPONDER, CID_BAMAPP, CID_WORLD, CID_TOTAL
};

//-----------[ Base Class for Movable things ]--------------------

class TMovable
{
	public:
		virtual	void	Copy(void *source, size_t sizeToCopy);
		virtual	void	Copy(void *source);
		virtual	bool	Save(uint16 state, FILE *pFile = NULL);

		TMovable();

		// make all destructors virtual
		virtual	~TMovable();

		char		tMovableDataStart;

		grip		gSelf;
		ClassID	classID;

		char		tMovableDataEnd;
};



//-----------[ Base Object Class ]--------------------------------

#define	OBJ_NAME_SIZE	16

class	Object : public TMovable 
{
	public:
		Object();

		// make all destructors virtual
		virtual	~Object();

		virtual bool	HandleMsg(Message* pMsg);
		virtual grip	GetScrimage();
		virtual void	SetResCount(res_t oldRType, uint oldResNum, res_t rType, uint resNum);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		void		SetContext(grip gC);
		void		TapMsgs(evt_t mask = E_ALL, int32 pri = PRIORITY_MAX);
		void		ResetMsgPri(int32 pri);
		void		CorkMsgs();
		void		SetName(char* n);
		bool		InMask(Message* pMsg);

		char		objectDataStart;

		evt_t		msgMask;
		grip		gContext;
		char		name[OBJ_NAME_SIZE];
		uint16	choreoId;

		char		objectDataEnd;
};



//----[ Lists ]------------------------------------------------------

struct node
{
	void*		value;	// value of node (grip, value, or pointer)
	uint		index;	// index number of this node
	uint		next;		// index number of next node
	uint		prev;		// index number of previous node
	int32		key;		// key value for node
	char		padding[12];	// force node size to power of two (32)
};



//----[ Scrimages ]--------------------------------------------------

// State bit defines
#define	S_HIDDEN		0x01	// Scrimage is hidden from view
#define	S_NO_CHANGE	0x02	// Scrimage will not update, regardless of changes
#define	S_ADDED		0x04	// Scrimage newly added
#define	S_CHANGED	0x08	// Scrimage newly changed
#define	S_FLIP		0x10	// Scrimage is flipped
#define	S_MIRROR		0x20	// Scrimage is mirrored
#define	S_FM_HORZ	0x40	// Flip or mirror is horizontal
#define	S_FM_VERT	0x80	// Flip or mirror is vertical



//----[ Dialogs ]----------------------------------------------------

#define	DG_MARGIN		5	// Standard dialog margin (pixels)
#define	DG_NO_WINDOW	-1	// Dialog will not have a window
#define	DG_DFT_WINDOW	0	// Dialog will create a default window
#define	DG_NO_BORDER	-1	// Dialog's window will not have a border


#endif

