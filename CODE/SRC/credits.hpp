// CREDITS.HPP
//
//	Copyright 1994,1995, Tachyon, Inc.
//
//
// Display game credits.
//
// 7/21/95
//

#ifndef credits_hpp
#define credits_hpp

#include "alldefs.hpp"
#include "bamroom.hpp"
#include "bamguy.hpp"
#include "bam_dg.hpp"
#include "palette.hpp"
#include "text.hpp"
#include "rect.hpp"

#define BRODIE_MAX_LEN	10

class Credits : public BAM_Room
{
	public:

	Credits(void);
	~Credits();

	bool		HandleMsg(Message* pMsg);
 	void		Setup();
 	void		SetupPage();
	int		WritePage(int headlineNum, int pageSqb, int pad = 35, int textX = 163);
 	void		ConfigButtons();

	BAM_Guy		back;
	uchar			*pback;
	CelHeader	*pbackCH;
	TPalette		pal;

	grip			gback;
	uint			resNumBack;
	Rectangle	rScreen;

	SquibRes		sqbCredits;
	int			sqbRes,sqbNum;

	BAM_Button	doneButton;
	BAM_Button	leftButton;
	BAM_Button	rightButton;

	int			page;

	char			brodieBuf[BRODIE_MAX_LEN];
	bool			fBrodie;
};

#endif
