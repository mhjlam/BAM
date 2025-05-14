//	SNAP.HPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Snaps a picture of the current game screen and dumps it as a lbm.
//
//		04-19-95
//


#ifndef snap_hpp
#define snap_hpp

#include "tigre.hpp"
#include "fmt_lbm.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	SNAP_FREQ	1

class Snap : public TMovable
{
	public:

	Snap();
	~Snap();

	void	SnapScreen();
	void	SingleScreen();
	void	StartSnap();
	void	StopSnap();
	void	GetLBMFileName(char* fn,int i);
	void	LoadLBMData();

	FormatLBM	aFormatLBM;

	char			fn[14];

	bool			snapOn;
	int			snapFreqCnt;

	int			snapSeq;
	int			singleSeq;
};

#endif
