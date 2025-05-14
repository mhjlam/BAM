//	BAMROOM.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
// Bam Room Class
//
// This is the base class for all game "rooms"
//


#ifndef bamroom_hpp
#define bamroom_hpp

#include "commmgr.hpp"
#include "context.hpp"
#include "palette.hpp"
#include "rect.hpp"
#include "tigre.hpp"

// bam room enums
enum 
{
	BR_DUMMY,
	BR_STORY,
	BR_WORLD,
	BR_CHAR,
	BR_ASSESS,
	BR_HALL,
	BR_CHOOSE,
	BR_DEATH,
	BR_CINE,
	BR_MENU,
	BR_NET_CHAR,
	BR_NET_STORY,
	BR_CREDITS,
	BR_INTRO_HALL,
	BR_NET_HALL
};

class BAM_Room : public Context
{
	public:
					BAM_Room(void);
		virtual ~BAM_Room(void);

		// the following 4 functions should be overloaded in each room
		virtual void	Setup(void);
		virtual void	Cleanup(void);
		virtual bool	HandleMsg(Message* pMsg);
		virtual void	Cycle(void);
		void				Pause(bool pauseOn, bool force=FALSE);
		virtual void	PauseRoom(bool pauseOn);
		void				Option(grip gPal);
		bool				SetBasicNet(bool net);
		bool				NetHandleMsg(struct sPacket *sPacket1, int toRoom = NULL);

		// stuff to allow a room to have buttons
		virtual void	SetCurrentControl(grip gNewControl);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char		bamRoomDataStart;
		grip		gCurControl;
		uint32		pausedCnt;
		TPalette		bPal;
		bool			basicNet;
		struct sPacket		bsPacket;
		char		bamRoomDataEnd;
};


extern bool menuFeaturesOn;

// This class is used for the older popups (before creation of the
// BAM_Popup class) which need functionality common to them all.
// Currently this is only used for enabling/disabling of the MAC menu bar.
class BAM_RM_Popup : public BAM_Room
{
	public:
		BAM_RM_Popup();
		~BAM_RM_Popup();
};


class BAM_RoomMgr 
{
	public:
		BAM_RoomMgr(void);
		~BAM_RoomMgr(void);

		void	NewRoom(int32 roomNum);
		void	DeleteCurRoom(void);
		void	CheckRoomChange(void);
		void	Cycle(void);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char		roomMgrDataStart;
		BAM_Room	*curRoom;
		grip		gCurRoom;
		int32 	curRoomNum;

		//Some rooms can be entered in various 'modes' which effect their startup.
		int32		newRoom;
		int32		newRoomMode;

		int32		prevRoomNum;
		int32		prevRoomMode;
		char		roomMgrDataEnd;
};

#endif
