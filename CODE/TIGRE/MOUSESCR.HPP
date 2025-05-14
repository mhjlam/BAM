//
// MOUSESCR.HPP
//
//	Scrimage based Mouse
//
// November 24, 1993
// WATCOM: October 25, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//	Mouse class definition.
//
//----[]-------------------------------------------------------------


#ifndef	mousescr_hpp
#define	mousescr_hpp


#include "rect.hpp"
#include "scrimage.hpp"
#include "types.hpp"
#include "mouse.hpp"

//---------[MouseScr]-------------------------------------------------

class MouseScr : public Mouse
{
	public:

							MouseScr();
		virtual			~MouseScr();

		virtual	coord	GetX();
		virtual	coord	GetY();
		virtual	int	GetCel();
		virtual	res_t	GetResType();
		virtual	uint	GetResNum();

		void				SetX(coord setX);
		void				SetY(coord setY);

		void				MovePointer(void);

		// this procedure is used right before GraphicsMgr::Animate()
		void				UpdatePointer();

		virtual void	SetRes(res_t type, uint num, uint iCel = 1, uint iScale = 256);
		virtual void	SetPri(int32 pri);
		virtual void	SetScale(uint newScale);

		void				ShutDownMouseDriver(void); 
		void				Pause(bool pauseIt);
		bool				Save(uint16 state, FILE *fp = NULL);
		virtual	bool	Protect(bool protect, Rectangle *pRect = NULL);

	protected:

		Guy				guy;

		virtual	void	ShowPointer();
		virtual	void	HidePointer();
};


#endif

