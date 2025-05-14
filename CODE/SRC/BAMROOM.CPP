//	BAMROOM.CPP
//
//	Copyright 1994, Tachyon, Inc.
//
// Bam Room Class
//
// This is the base class for all game "rooms"
//

#include "bamroom.hpp"

// all room hpps
#include "option.hpp"
#include "story.hpp"
#include "world.hpp"
#include "choose.hpp"
#include "makechar.hpp"
#include "death.hpp"
#include "cine.hpp"
#include "assess.hpp"
#include "hall.hpp"
#include "mainmenu.hpp"
#include "credits.hpp"
#include "netchar.hpp"
#include "netstory.hpp"
#include "introhal.hpp"
#include "nethall.hpp"

//other
#include "alldefs.hpp"
#include "bampopup.hpp"

//engine
#include "api.hpp"
#include "apievt.hpp"
#include "apigraph.hpp"
#include "fontmgr.hpp"
#include "graphmgr.hpp"
#include "mouse.hpp"
#include "objmgr.hpp"
#include "modex.hpp"
#include "savebase.hpp"

#include "debug.hpp"

// TEMPORARY - Check for Dynamic Resources
#ifndef OS_MAC
#include "mem.hpp"
#endif

// NOTE:  For this to work, declare an instance of BAM_RoomMgr
// somewhere and call the appropriate routines.
 
   
//***********************************************************************
//
// BAM_RoomMgr Class
//
//***********************************************************************

BAM_RoomMgr::BAM_RoomMgr(void)
{
	curRoom = NULL;
	gCurRoom = 0;
	curRoomNum = prevRoomNum = newRoom = 0;
	prevRoomMode = newRoomMode = 0;
}

BAM_RoomMgr::~BAM_RoomMgr(void)
{
//	DeleteCurRoom();
}

bool
BAM_RoomMgr::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&roomMgrDataStart, 1, (int)&roomMgrDataEnd -
				(int)&roomMgrDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&roomMgrDataStart, 1, (int)&roomMgrDataEnd -
				(int)&roomMgrDataStart, pFile);
			break;
	}
	return(TRUE);
}

// setup the variable that lets us know that we want to change rooms
void
BAM_RoomMgr::NewRoom(int32 roomNum)
{
	char	string1[80];

	if (newRoom)
	{
		sprintf(string1, "RoomMgr::NewRoom(%d) - new room %d already set!",
			roomNum, newRoom);
		APanic(string1);
	}

	newRoom = roomNum;	
}

// if we have a room in use, delete it
void
BAM_RoomMgr::DeleteCurRoom(void)
{
	if (curRoom)
	{
		curRoom->Cleanup();
		delete curRoom;
		curRoom = NULL;
		gCurRoom = 0;
		curRoomNum = 0;
	}
}

void
BAM_RoomMgr::Cycle(void)
{
	if (gCurRoom)
	{
		curRoom = ADerefAs(BAM_Room, gCurRoom);
	}
}

// check to see if we want to change rooms
extern Debugger	ResMgrDebug;
void
BAM_RoomMgr::CheckRoomChange(void)
{
	if (!newRoom)
	{
		// no room change
		return;		
	}

	char			mess[100];
	TPalette		*pal;

	// save the old room number
	prevRoomNum = curRoomNum;

	// if we have a room in use, delete it
	DeleteCurRoom();

#ifndef OS_MAC
	// TEMPORARY - Check for Dynamic Resources
	int	i;
	int	count;
	int	dynams = 0;
	Resource*	dgRes;
	node*			dgResNode;
	struct mh *dl;

	// between rooms it is understood that no dynamic resources should be
	// left.  In case there are, flusharoo!

	ResMgrDebug.Out("Resource flush-a-roo!\n");
	count = pResMgr->resList->count;
	for (i = 0; i < count; i++)
	{
		dgResNode = pResMgr->resList->GetPNode(i + 1);
		dgRes = ADerefAs(Resource, (grip) dgResNode->value);
		if (dgRes->IsDynamic())
		{
			if(dgRes->lockCount)
				dgRes->Unlock(TRUE);
			dynams++;
			// back up to get to the memory header
			dl = (struct mh *) ((char *) dgRes - sizeof(struct mh));
			pMono->Out("Unfreed Dynamic Res: %s %p - deallocating", dl->Mstr, (void *)dgRes);
			AFlush((grip)dgResNode->value);
//			printf("\nUnfreed Dynamic Res: %s", dl->Mstr);
  		}
  		else if(!dgRes->lockCount)
  		{
			// ah, who cares.  Flush unlocked static resources too.
  			AFlush((grip)dgResNode->value);
  		}
  	}
  	if (dynams)
  	{
  		sprintf(mess, "%d unfreed dynamic resources, you brodie!", dynams);
  		ResMgrDebug.Out(mess);
  	}

	// cause squib1 to release its current resource from the MIDDLE OF PHUCKIN
	// MEMORY and load a new one somewhere at the front of memory, where we
	// won't care about it. - Van
	pBam->squib1.Load(1100, 100);

	// now that he's unlocked, remove him from the middle of memory
	pResMgr->PurgeAllUnlocked();


	pMono->Out("RAM: %d total avail, %d largest avail\n",
		(int)pMemMgr->AvailMem(), (int)pMemMgr->LargestAlloc());

#endif

	switch(newRoom)
	{
		case BR_WORLD:
//			pMemMgr->Dump(44001, "CheckRoomChange(BR_WORLD)", TRUE);
			TRACK_MEM("World Rm");
			curRoom = new World;
			break;

		case BR_STORY:
			TRACK_MEM("Story Rm");
			curRoom = new Story;
			break;

		case BR_CHOOSE:
			TRACK_MEM("Choose Rm");
			curRoom = new Choose;
			break;

		case BR_CHAR:
			TRACK_MEM("MakeChar Rm");
			curRoom = new MakeChar;
			break;

		case BR_DEATH:
			TRACK_MEM("Death Rm");
			curRoom = new Death;
			break;

		case BR_CINE:
//			pMemMgr->Dump(44002, "CheckRoomChange(BR_CINE)", TRUE);
			TRACK_MEM("Cine Rm");
			curRoom = new Cine;
			break;

		case BR_ASSESS:
			TRACK_MEM("Assess Rm");
			curRoom = new Assess;
			break;

		case BR_HALL:
			TRACK_MEM("Hall Rm");
			curRoom = new Hall;
			break;

		case BR_MENU:
			TRACK_MEM("Menu Rm");
			curRoom = new MainMenu;
			break;

		case BR_CREDITS:
			TRACK_MEM("Credits Rm");
			curRoom = new Credits;
			break;

		case BR_NET_CHAR:
			TRACK_MEM("NetChar Rm");
			curRoom = new NetChar;
			break;

		case BR_NET_STORY:
			TRACK_MEM("NetStory Rm");
			curRoom = new NetStory;
			break;

		case BR_INTRO_HALL:
			TRACK_MEM("IntroHall Rm");
			curRoom = new IntroHall;
			break;

		case BR_NET_HALL:
			TRACK_MEM("NetHall Rm");
			curRoom = new NetHall;
			break;

		default:
			sprintf(mess, "Invalid Room Number: %d", newRoom);
			APanic(mess);
	}

	TRACK_MEM("RmMgr Pal");	pal = new TPalette;
	pal->Load(9100); //default
	//setup curRoom's default pal from mgr's pal
	memcpy(curRoom->bPal.guns,pal->guns,sizeof(pal->guns));
	pal->FadeToBlack();
	delete pal;

	pMouse->Hide();

	FillScreen(CI_BLACK);

	pMouse->Init(0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1);
	pMouse->SetRes(RES_ANIM,POINTER_RES,7); //hourglass cel
	pMouse->ForceShow(0);
	curRoom->bPal.UpdateAll();

	curRoomNum = newRoom;
	gCurRoom = curRoom->gSelf;
	curRoom->Setup();
	newRoom = 0;
	AFlushEvents();
}


//***********************************************************************
//
// BAM_Room Class
//
//***********************************************************************

BAM_Room::BAM_Room(void)
{
	Activate(TRUE);
	gCurControl = NULL;
	pausedCnt = 0;
	basicNet = FALSE;
}

BAM_Room::~BAM_Room(void)
{
}

bool
BAM_Room::Save(uint16 state, FILE *pFile)
{
	switch(state)
	{
		case DURING_SAVE:
			fwrite(&bamRoomDataStart, 1, (int)&bamRoomDataEnd -
				(int)&bamRoomDataStart, pFile);
			break;

		case DURING_RESTORE:
			fread(&bamRoomDataStart, 1, (int)&bamRoomDataEnd -
				(int)&bamRoomDataStart, pFile);
			break;
	}
	return(TRUE);
}

void	
BAM_Room::Setup(void)
{
	bPal.FadeToBlack(0);
	if(pMouse->GetCel() == 7) //still at hourglass so reset
		pMouse->SetRes(RES_ANIM,POINTER_RES,1);
}

void	
BAM_Room::Cleanup(void)
{
}

void
BAM_Room::PauseRoom(bool pauseOn)
{
	// eliminate warning
	pauseOn = pauseOn;
}

void
BAM_Room::Option(grip gPal)
{
	OptionMenu	*pOption;

	TRACK_MEM("Options");	pOption = new OptionMenu;
	//call to pause() now in Option()
	pOption->Setup(gPal);

}

void	
BAM_Room::Pause(bool pauseOn, bool fForce)
{
	if (pauseOn)
	{
		if(fForce)
		{
			pausedCnt = 1;
		}
		else pausedCnt++;
		if (pausedCnt == 1)
		{
			// pause for the first time
			PauseRoom(TRUE);
		}
	}
	else
	{
		if(fForce)
		{
			pausedCnt = 1;
		}

		if (pausedCnt)
		{
			pausedCnt--;
			if (!pausedCnt)
			{
				// ready to unpause
				PauseRoom(FALSE);
			}
		}
	}
}

bool	
BAM_Room::HandleMsg(Message *pMsg)
{
	switch (pMsg->type)
	{
		// check for NOTICE messages
		case MSG_NOTICE:
			switch (pMsg->notice.type) 
			{
				case N_SETUP:
					Setup();
					return TRUE;

				case N_QUIT:
					Cleanup();
					return TRUE;
			}
			break;
	}

	// context didn't want the message, pass on to receivers
	return Context::HandleMsg(pMsg);
}

void	
BAM_Room::Cycle(void)
{

	// call the superclasses method last
	Context::Cycle();

	if(basicNet && !bGlobal.netDisconnect)
	{
		bsPacket.pData = NULL;
		NetHandleMsg(&bsPacket);
	}
}

// grip of whatever button is currently activated, if any
void
BAM_Room::SetCurrentControl(grip gNewControl)
{
	BAM_Button	*pButton;

	if(gCurControl && gCurControl != gNewControl)	// if another control is already current (between MOUSE_DOWN and MOUSE_UP)
	{
		pButton = ADerefAs(BAM_Button, gCurControl);
		pButton->Select(FALSE);
	}

	gCurControl = gNewControl;
}


bool
BAM_Room::SetBasicNet(bool net)
{
	basicNet = net;
	return (basicNet);
}


//For use by rooms that need basic net capabilities
//which can include popups, etc.
//A room already having net code could call this function first
//and if the packet wasn't handled here then it could then handle it.
//Var. toRoom specifies a room to change to when an error popup destructs
bool
BAM_Room::NetHandleMsg(struct sPacket *psPacket, int toRoom)
{
	bool	ret = FALSE;
	BAM_ErrorPopup			*pPop;
	int	error;
	char	recvBuf[257];

	if(pCommMgr->DataAvailable())
	{
		if(!psPacket->pData)
		{
			//setup tmp receive buffer 
			psPacket->pData = recvBuf;
		}
		error = pCommMgr->ReceiveData(psPacket);

		if(error)
		{
			pMono->Out("  Error, no packet received.  Error == %d\n", error);
			APanic("NetChar: no packet received.");
		}
		else
		{
			//decode packet by type placed in header id
			switch(psPacket->header.ID)
			{
				case BMSG_DISCONNECT:
					//Shutdown basic net code if running
					pMono->Out("BAM_Room: Received BMSG_DISCONNECT.\n");
					bGlobal.netDisconnect = TRUE;
					basicNet = FALSE;

					bGlobal.storyLine = STORY_NONE;
					TRACK_MEM("Error Popup"); pPop = new BAM_ErrorPopup;
					pPop->Setup(NULL,POPUP_SQB,2,toRoom); //'opponent has disconnected'
					ret = TRUE;
					break;

				case BMSG_INIT:
					//if other player is trying to init a new room and we're here
					//then we need to post a popup telling user that other player
					//is waiting on them

					pMono->Out("BAM_Room: Received BMSG_INIT.\n");
					TRACK_MEM("Error Popup"); pPop = new BAM_ErrorPopup;
					pPop->Setup(NULL,POPUP_SQB,1,toRoom); //'opponent is waiting'
					ret = TRUE;
					break;

				default:
					break;
			}
		}
	}
	return(ret);	//did we handle the message?
}


//***********************************************************************
//
// BAM_RM_Popup Class
//
//***********************************************************************

bool menuFeaturesOn;

BAM_RM_Popup::BAM_RM_Popup()
{
	menuFeaturesOn = FALSE;
	if(pWorld)
		pWorld->AllowMouseDragging(FALSE);
}

BAM_RM_Popup::~BAM_RM_Popup()
{
	menuFeaturesOn = TRUE;	
	if(pWorld && bGlobal.storyLine != SHOW_OFF)
		pWorld->AllowMouseDragging(TRUE);
}
