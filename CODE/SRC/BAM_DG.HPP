// BAM_DG.HPP	-	BAM Dialog thingies (boxes, buttons, etc)

#ifndef BAM_DG_HPP
#define BAM_DG_HPP

#define BOX_FLASH_INTERVAL	10

#include "api.hpp"
#include "bamguy.hpp"
#include "object.hpp"
#include "palette.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

#include <time.h>

#define	BDG_MAX_RES		3

struct bdg_resinfo
{
	res_t		resType;
	uint		resNum;
	uint		resCel;
};


class BAM_Control : public BAM_Guy
{
	public:
							BAM_Control(void);
							~BAM_Control();
		virtual void	Listen(bool fListen);
		void				SetOwnerCel(uint rNewOwnerCel);
		void				SetupReplies(uint32 newReplyMask, void *data = NULL);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char		bamConDataStart;
		uint32	offSetH;
		uint32	offSetV;
		bool		fDeselectCurControl;
		int		currState;
		int		font,prevFont;

	protected:
		uint32	ReplyMask;
		void		*replyData;
		uint		rOwnerCel;	// master cel we're to Draw() ourselves into,
									// rather than letting the Context roster handle us
		char		bamConDataEnd;
};

class BAM_Box : public BAM_Control
{
	public:
							BAM_Box(void);
							~BAM_Box();
		virtual void	Listen(bool fListen);
		void				Create(uint32 xPos, uint32 yPos, uint32 maxWidth, uint32 maxHeight,
									 int32 newPri, grip gMsgText, uint32 newMaxLen,
									 grip gNewContext,uint rNewOwnerCel,
									 uint32 offSetH_P = 0, uint32 offSetV_P = 0);
		void				SetColors(uchar backColor_P = CI_BLACK, uchar foreColor1_P = CI_WHITE, uchar foreColor2_P=0, uchar foreColor3_P=0,
									 	 uchar foreColor4_P=0 , uchar foreColor5_P=0, uchar foreColor6_P=0,
									 	 uchar cursorColor1_P=CI_WHITE, uchar cursorColor2_P=CI_WHITE);
		void				SetFont(int font_P = 9050);
		void				SetBackCopyColor(int color = -1); //def. copy background
		void				Draw(void);
		virtual bool	HandleMsg(Message *pMsg);
		void				Select(bool fState);
		void				ReturnString(void);
		void				AddChar(char newByte);
		void				DelChar(void);
		virtual void	Cycle(void);
		void				UpdateString(void), SetNextBox(grip gNewNextBox), SetEditBuffer(grip gMsgText);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char		bamBoxDataStart;
		uint32	boxMsgPriority;
		grip		gNextBox;

	protected:
		Rectangle rOwner;
		grip		gCel, gCel2, gBack, gOrigText;
		uint		rNumCel, rNumCel2, rNumBack;
		char		editBuf[80];
		uint32	currLen, maxLen;
		clock_t	boxFlashTimer;
		int		flashToggle;
		int		backCopyColor;
		uchar		backColor,foreColor1,foreColor2,foreColor3,foreColor4;
		uchar		foreColor5,foreColor6,cursorColor1,cursorColor2;
		char		bamBoxDataEnd;
};

class BAM_Button : public BAM_Control
{
	public:
							BAM_Button(void);
							BAM_Button(uint32 xPos, uint32 yPos, uint32 newPri,
								res_t resType, uint32 resNum, uint32 celNum,
								grip gNewContext, uint32 NewReplyMask,
								uint32 offSetH_P = 0, uint32 offSetV_P = 0);
		bool				Create(uint32 xPos, uint32 yPos, uint32 newPri,
									res_t resType, uint32 resNum, uint32 celNum,
									grip gNewContext, uint32 offSetH_P = 0, uint32 offSetV_P = 0, int font_P = 9050);
							~BAM_Button();
		virtual void	Listen(bool fListen);
		virtual void	Cycle(void);		
		virtual bool	HandleMsg(Message *pMsg);
		virtual void	Select(int newState);
		virtual void	Draw(void);

		// specify an anim/cel for each button state (TRUE/ON and FALSE/OFF)
		// does not overload Guy::SetAnim()
		bool				SetButtonRes(int state, res_t newResType, uint32 newResNum, uint32 newCel);

		void				SetCelText(uint32 celNum, char pNewText[]);
		void				SetColors(uint32 celNum, uint32 newColor1, uint32 newColor2, uint32 newBackColor = CI_SKIP);
		void				SetColors(uint32 celNum, uint32 newColor1, uint32 newColor2,
										 uint32 newColor3, uint32 newColor4, uint32 newBackColor = CI_SKIP);
		void				SetColors(uint32 celNum, uint32 newColor1, uint32 newColor2,
										 uint32 newColor3, uint32 newColor4,
										 uint32 newColor5, uint32 newColor6, uint32 newBackColor = CI_SKIP);
		void				SetTextJustify(justify NewHJust, justify NewVJust);
		void				SetTextOffSet(int32 NewHOffSet, int32 NewVOffSet);
		virtual bool	Save(uint16 state, FILE *pFile = NULL);

		char			bamButtonDataStart;
		bool			fIsToggle;
		justify		CelTextHJust, CelTextVJust;
		int32			TextHOffSet, TextVOffSet, celTextLines[3];
		char			CelText[3][40];
		int32			CelTextColors[3][7];
		bdg_resinfo	celAnim[BDG_MAX_RES];	// type/anim/cel combo for each button state
		char			bamButtonDataEnd;
};

// macros for controls (buttons, etc) to send replies to their owning contexts
#define	N_CONTROL_REPLY	N_LAST

#define	REPLY_SELECTED			0x01
#define	REPLY_DESELECTED		0x02
#define	REPLY_ACTIVATED		0x04
#define	REPLY_DEACTIVATED		0x08
#define	REPLY_CANCELLED		0x10
#define  REPLY_SELECTED_KEY 	0x20

#define DG_DISABLED		2
// used in conjunction with FALSE(0) and TRUE(1)
#endif
