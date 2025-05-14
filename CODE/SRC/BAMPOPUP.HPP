//	BAMPOPUP.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Bam Popup Class
//
// This is the basic popup class for SOME of the game's popups.
//

#ifndef bampopup_hpp
#define bampopup_hpp

#include "bamroom.hpp"
#include "bam_dg.hpp"
#include "bamguy.hpp"
#include "rect.hpp"
#include "fade.hpp"
#include "tigre.hpp"
#include "text.hpp"
#include "types.hpp"


class BAM_Popup : public BAM_Room
{
	public:
	BAM_Popup();
	~BAM_Popup();

	virtual bool		HandleMsg(Message* pMsg);
	virtual bool		HandleNotice(Message* pMsg);
	virtual bool		HandleEvent(Message* pMsg);
 	virtual void		Setup(grip gPal,char* pMsg);
	virtual void		Cleanup();
	virtual void		ActivatePopup();
	virtual void		SetAni(int ani);
	virtual void		SetPri(int pri);
	virtual void		SetFade(bool fPos);
	virtual void		SetBasicNet(bool fPos); //overload
	virtual void		SetPause(bool fPos);
	virtual void		SetWindowOffsets(int x, int y);
 	virtual void		SetTextSqb(int sqbRes,int sqbNum,justify just=DG_JUST_CENTER);
	virtual void		SetMode(modalType mode_P);

	modalType	explicitMode;

	BAM_Guy		back;
	grip			gback;
	uint			rNumBack;
	grip			gbackAnim;
	int			prevFont;
	uchar			*pback;
	CelHeader	*pbackAnimCH,*pbackCH;

	FadeTo		fadeTo;
	bool			fadeOn;

	SquibRes		sqbPopup;
	int			textSqbRes,textSqbNum;
	justify		textJust;

	Rectangle	rback;
	Rectangle	oldMouseLimits;
	res_t			oldMouseResType;
	int			oldMouseResNum;
	int			oldMouseCel;
	int			oldMouseHideCnt;
	int			prevRoomNum;

	int			backAni;
	int			winOffX,winOffY;
	int			popupPri;
	bool			pause;
	int			explicitPause;
	int			explicitNet;

};

class BAM_DefaultPopup : public BAM_Popup
{
	public:

	BAM_DefaultPopup(int buttonNum_P);
	~BAM_DefaultPopup();

 	virtual void		Setup(grip gPal,char *pMsg);
	virtual void		SetButtonStartOffsets(int x, int y);
 	virtual void		SetButtonSqb(int buttonNum,int sqbRes,int sqbNum);

	//button stuff
	int			buttonNum;
	BAM_Button	*pBUTTON;
	int			*pSqbRes;	//for button text
	int			*pSqbNum;
	int			buttonStartX,buttonStartY;
};

class BAM_TeleportPopup : public BAM_DefaultPopup
{
	public:

	BAM_TeleportPopup();
	~BAM_TeleportPopup();

 	virtual void		Setup(int sqbRes,int sqbNum);
	virtual bool		HandleNotice(Message* pMsg);

	char			*pTeleText;
	grip			gTeleText;
	BAM_Box		teleBox;

};


class BAM_ErrorPopup : public BAM_DefaultPopup
{
	public:

	BAM_ErrorPopup();

 	virtual void		Setup(grip gPal,int sqbRes,int sqbNum,int nextRoom=NULL);
 	virtual void		Setup(grip gPal,char *pMsg,int nextRoom=NULL);
	virtual bool		HandleNotice(Message* pMsg);
	virtual void		Cycle();

	int	toRoom;	//room to change to after popup
};


class BAM_WorldEnderPopup : public BAM_DefaultPopup
{
	public:

	BAM_WorldEnderPopup();

 	void		Setup();
	bool		HandleNotice(Message* pMsg);
	bool		HandleEvent(Message* pMsg);

	bool	fConnFail;
};


//=====================================================
//class SamplePopup : public BAM_DefaultPopup
//{
//	public:
//
//	//uses only one button
//	SamplePopup();
//
// 	virtual void		Setup(grip gPal);
//	virtual bool		HandleNotice(Message* pMsg);
//
//
//
//};


#endif
