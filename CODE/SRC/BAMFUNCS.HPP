// BAMFUNCS.CPP
//
//	Copyright 1994,1995 Tachyon, Inc.
//
// Global functions for BAM
//
//	09-07-94
//

#ifndef BAMFUNCS_HPP
#define BAMFUNCS_HPP

#include "types.hpp"
#include "writeres.hpp"
#include "rect.hpp"
#include "bamfunc2.hpp"

//number of path variations for each start point
#define  VARPATHS 4
#define	MAXPATHS VARPATHS*MAXREGIONS

enum unitColor_t
{
	RED = 1,
	BLUE,
	PURPLE,
	GOLD,
	GREEN
};

void	CopyBitMap(CelHeader *pDest, int32 destX, int32 destY, uchar *pSrcBuf, int srcWidth, int srcHeight,
			bool fHasSkip, grip gNewClut = NULL);
uchar GetPixel(int celXOff, int celYOff, res_t srcType,	uint srcNum, uint srcCel);
void	LoadUnitColors(grip gcurPal, unitColor_t color1, unitColor_t color2);
char	*GetFirstToken(char *str);
char	*GetNextToken(void);

void	GetUnitColors(unitColor_t* color1, unitColor_t* color2, int storyLine_P=0);
int	GetCine(int storyLine, int missionsDone, int chooseSide);
char* GetScenario(char* sceneOut, int storyLine, int missionsDone, int chooseSide=1);
void	GetStoryName(int* sqbRes, int* sqbNum, int storyOrMap);
void	GetCharName(int* sqbRes, int* sqbNum, int storyLine_P=0, int chooseSide_P=0,bool specific=FALSE);
void	GetAbbrCharName(int* sqbRes, int* sqbNum, int storyLine_P=0, int chooseSide_P=0,bool specific=FALSE);
int	GetStoryLine(char* mapName);
void	SetDefaults(char* map);
int	GetPortraitAnim(int chooseSide_P=0,bool specific=FALSE);
void	GetLevelName(int* sqbRes, int* sqbNum, int storyLine_P,int missionsDone_P,int chooseSide_P=0);
void  GetRank(int* sqbRes, int* sqbNum, int scoreOrRank);
int*  GetLegendPath(int* legendPath, int pathIndex);
int	GetCharId();
void	AlignLawfulChaotic(int pct);
void	AlignGoodEvil(int pct);
void	GetAlignment(int *sqbRes, int *sqbNum);
void	FakeMouseClick(grip gButton);
void	ClearGlobals();
int	GetMissionsDone(char* map);
bool	IsStoryDone(int storyNum,int storyDone);
int 	GetStoryDone();
void	WriteStoryDone(int storyDone);


#define	random(num)	(rand() % num)

#endif

