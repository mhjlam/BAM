//
// MOUSE.HPP
//
// November 24, 1993
// WATCOM: October 25, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	Abstract Mouse class definition.
//
//----[]-------------------------------------------------------------


#ifndef	mouse_hpp
#define	mouse_hpp


#include "rect.hpp"
#include "manager.hpp"
#include "types.hpp"


// these match definitions in x32.h
const int32		LEFT_BTN	 = 1;
const int32		RIGHT_BTN = 2;

// Signal interrupt mask constants
#define 	MSM_MOVE 				0x01
#define 	MSM_BTN_DOWN_L			0x02
#define 	MSM_BTN_UP_L 			0x04
#define 	MSM_BTN_DOWN_R			0x08
#define 	MSM_BTN_UP_R			0x10
#define 	MSM_BTN_DOWN_M			0x20
#define 	MSM_BTN_UP_M			0x40

// Status bits
#define	MST_NONE 				0x0000
#define	MST_NO_RES				0x0001
#define	MST_NEW_RES				0x0002

#define	BTNS_UP_DOWN			(MSM_BTN_DOWN_L | MSM_BTN_UP_L | \
										 MSM_BTN_DOWN_R | MSM_BTN_UP_R)

#define	INT_MOUSE				0x33

struct StackPtr
{
	int32		pointer;
	int16		selector;
};

extern StackPtr	oldStackPtr;
extern StackPtr	newStackPtr;

extern int32	MIntValue;
extern int 		MIntMask;

//---------[Mouse]-------------------------------------------------

class Mouse : public Object, public Manager 
{
	public:

		uint16		status;

		int32			mickeysLeftX;
		int32			mickeysLeftY;

		coord			lastX;
		coord			lastY;

		uint16		hideCount;

		Rectangle	screenLimits;		// current screen resolution
		Rectangle	mouseLimits;		// box within the screen that the mouse
												// should be restricted to 

		void				SetFence(int X1, int Y1, int X2, int Y2), DrawFence(void),
								EraseFence(void);
		Rectangle	rClickDrag, rCurrentFence;
		bool			fDragMode;
		virtual void	EnableDragging(bool fNewState) = 0;
		virtual void	SetDraggableRect(int x1, int y1, int x2, int y2);
		int				GetFenceStartX(void), GetFenceStartY(void);

							Mouse();
		virtual			~Mouse();

		void				Init(coord x1, coord y1, coord x2, coord y2);
		void				SetLimits(Rectangle *mouseRect);
 		bool				CheckBounds();

		virtual	void	Show();
		virtual	void	Hide();
		
		virtual	void	GetPos(Message* pMsg);
		virtual	void	SetPos(coord setX, coord setY);

		virtual	coord	GetX() = 0;
		virtual	coord	GetY() = 0;
		virtual	void	SetX(coord setX) = 0;
		virtual	void	SetY(coord setY) = 0;
		virtual	int	GetCel() = 0;
		virtual	res_t	GetResType() = 0;
		virtual	uint	GetResNum() = 0;

		virtual	void	MovePointer() = 0;

		virtual	void	UpdatePointer() = 0;

		virtual	int	ForceShow(int16 newHideCount);
		
		virtual	void	SetRes(res_t type, uint num, uint iCel = 1, uint iScale = 256) = 0;
		virtual	void	SetFenceRes(res_t horizType, uint rHorizNum, uint horizCel,
								res_t vertType, uint rVertNum, uint vertCel) = 0;
		virtual	void	HideFence(void) = 0;
		void				SetFence(int X1, int Y1, int X2, int Y2);
		void				DrawFence(void);
		void				EraseFence(void);
		virtual	void	SetPri(int32 pri) = 0;
		virtual	void	SetScale(uint newScale) = 0;

		void				ShutDownMouseDriver(void); 

		virtual	void	Pause(bool pauseIt) = 0;
		virtual	bool	Protect(bool protect, Rectangle *pRect = NULL) = 0;

		virtual	bool	Save(uint16 state, FILE *fp = NULL) = 0;
		void				ClearMickeys();

	protected:

		// x,y should remain protected to help maintain compatibility w/ the mac
		coord				x, y;

		virtual	void	ShowPointer() = 0;
		virtual	void	HidePointer() = 0;
		virtual	void	HideFence() = 0;
};

extern Mouse* pMouse;

#endif
