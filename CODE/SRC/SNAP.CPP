//	SNAP.CPP
//
//	Copyright 1995, Tachyon, Inc.
//
// Snaps a picture of the current game screen and dumps it as a lbm.
//
//		04-19-95
//


#include "snap.hpp"

#include "apigraph.hpp"
#include "graphmgr.hpp"
#include "mouse.hpp"

#define	SCREEN_WIDTH	320
#define	SCREEN_HEIGHT	400


Snap::Snap()
{
	snapSeq = 0;
	singleSeq = 0;
	snapFreqCnt = SNAP_FREQ;
	snapOn = FALSE;
}


Snap::~Snap()
{
}


void
Snap::SnapScreen()
{
	snapFreqCnt++;

	if(snapOn && snapFreqCnt >= SNAP_FREQ)
	{
		snapFreqCnt = 0;
		//pMouse->Hide();

		sprintf(fn,"sn%d.lbm",snapSeq);
		snapSeq++;

		//let get palette info straight into AFormatLBM
 		AGetPalette((Gun*)aFormatLBM.lbmPal,0,255);

		pMouse->Pause(TRUE);
		ARBlit(pGraphMgr->vgaDriver, aFormatLBM.sData,
				0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, pGraphMgr->videoBufferSeg);
		pMouse->Pause(FALSE);

		aFormatLBM.Write(fn);

		//pMouse->Show();
	}
}


void
Snap::StartSnap()
{
 	snapOn = TRUE;
}


void
Snap::StopSnap()
{
 	snapOn = FALSE;
}


void
Snap::SingleScreen()
{
	//pMouse->Hide();

	sprintf(fn,"snap%d.lbm",singleSeq);
	singleSeq++;

	//let get palette info straight into AFormatLBM
 	AGetPalette((Gun*)aFormatLBM.lbmPal,0,255);

	pMouse->Pause(TRUE);
	ARBlit(pGraphMgr->vgaDriver, aFormatLBM.sData,
			0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, pGraphMgr->videoBufferSeg);
	pMouse->Pause(FALSE);

	aFormatLBM.Write(fn);

	//pMouse->Show();
}
