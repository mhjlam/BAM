//	INTRO.HPP
//
//	Copyright 1994, Tachyon, Inc.
//
// Play the intro cinematic
//

#include "intro.hpp"
#include "bam.hpp"
#include "mouse.hpp"

Intro::Intro(void)
{
	msgMask = E_KEY_DOWN | E_MOUSE_UP;
}

Intro::~Intro(void)
{
	
}

void	
Intro::Setup(void)
{
	pMouse->Hide();
	flic.Setup(30, gSelf, 10);
	// display the first frame
	flic.Cycle();
	tMusic.Play(7100);
}

void	
Intro::Cleanup(void)
{
	tMusic.Stop();
	pMouse->Show();
}

bool	
Intro::HandleMsg(Message *pMsg)
{
	switch (pMsg->type)
	{
//		// check for NOTICE messages
//		case NOTICE:
//			switch (pMsg->notice.value) 
//			{
//				case N_SETUP:
//					Setup();
//					return TRUE;
//					break;
//
//				case N_QUIT:
//					Cleanup();
//					return TRUE;
//					break;
//			}
//			break;
		case MSG_EVENT:
				switch (pMsg->event.type)
				{
					case E_MOUSE_UP:
					case E_KEY_DOWN:
						bGlobal.roomMgr.NewRoom(BR_STORY);
						return TRUE;
				}
			break;
	}

	// context didn't want the message, pass on to receivers
	return BAM_Room::HandleMsg(pMsg);
}

void	
Intro::Cycle(void)
{
	flic.Cycle();

	if (flic.fFlicDone)
	{
		bGlobal.roomMgr.NewRoom(BR_STORY);
	}

	// call the superclasses method last
	BAM_Room::Cycle();
}

