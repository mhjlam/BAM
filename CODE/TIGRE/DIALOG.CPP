//
// DIALOG.CPP
//
// January 26, 1994
// WATCOM: October 4, 1994  BKH
// Brian K. Hughes
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apidlg.hpp"
#include "apievt.hpp"
#include "apifont.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "fontmgr.hpp"
#include "mouse.hpp"
#include "palette.hpp"


// kludgy, but with drawing taking place during the constructor
// and overloaded constructors, this is simplest.
static DrawProc	ppDrawAtConstruction = NULL;


//----[ TControl Class ]----------------------------------------------

// Constructor
//
TControl::TControl(TControlInfo* pCI)
{
	x = pCI->x;
	y = pCI->y;
	SetName("TControl");
	value = pCI->value;
	attr = 0;
	SetAttr(DG_ENABLED, pCI->enabled);
	SetAttr(DG_EXIT, pCI->exit);
	accel = pCI->key;
	resType = pCI->resType;
	resNum = pCI->resNum;
	cel = pCI->cel;
	gNext = gPrev = 0;
}


// Default destructor - just so TControl can be an abstract class
TControl::~TControl()
{
}


// Return TRUE if the TControl's type matches that specified
//
bool
TControl::IsType(ctrl_t t)
{
	return (type == t) ? TRUE : FALSE;
}


// Set ourselves active
//
void
TControl::Hilite(bool on)
{
	SetAttr(DG_ACTIVE, on);
}


// Select the control
//
void
TControl::Select(bool relVerify)
{
	Dialog* 	d;
	TControl*	pC;

	relVerify = relVerify;		// Gets rid of unreferenced warning

	// Unhilite the currently active control
	d = ADerefAs(Dialog, gDialog);
	pC = ADerefAs(TControl, d->gCurTControl);
	pC->Hilite(FALSE);

	// Hilite ourselves
	Hilite(TRUE);
	d->gCurTControl = gSelf;
}


// Handle events
bool
TControl::HandleMsg(Message* pMsg)
{
	pMsg = pMsg;		// Gets rid of unreferenced warning
	return FALSE;
}


// Check to see if an attribute is set
bool
TControl::CheckAttr(uint a)
{
	return (attr & a) ? TRUE : FALSE;
}


// Set an attribute
void
TControl::SetAttr(uint a, bool on)
{
	if (on)
	{
		attr |= a;
	}
	else
	{
		attr &= ~a;
	}
}


// Draw the control.  Does nothing in the base class
//
void
TControl::Draw()
{
}


// Position the control.  Overridden to position relative to window.
//
void
TControl::SetPos(coord cx, coord cy)
{
	Rectangle	r;
	Dialog*		d = ADerefAs(Dialog, gDialog);

	d->GetWindowRect(&r);
	Guy::SetPos(r.x1 + cx, r.y1 + cy);
	SetRect();
	SetState(S_CHANGED, TRUE);
}



//----[ Label Class ]------------------------------------------------

// Constructor
//
Label::Label(TControlInfo* pCI) : TControl(pCI)
{
	SetName("Label");
	type = CTRL_LABEL;
	gCaption = NULL;
	origResNum = resNum;
	origCel = cel;
	SetAttr(DG_ENABLED, FALSE);
	SetAttr(DG_EXIT, FALSE);
}


// Set up the label
void
Label::Setup(TControlInfo* pCI, uint tline, uint bline)
{
	// Set the caption
	TRACK_MEM(pCI->cap);
	if (gCaption)
	{
		AFree(gCaption);
	}
	gCaption = AMalloc(strlen(pCI->cap) + 1);
	strcpy(ADerefAs(char, gCaption), pCI->cap);
	textX = pCI->tx;
	textY = pCI->ty;
	topLine = tline;
	bottomLine = bline;
	just = pCI->just;
}


Label::~Label(void)
{
	AFree(gCaption);
	gCaption = NULL;
}


// Draw the label by creating/cloning a cel and putting the text on it
//
void
Label::Draw()
{
	char*			cap = ADerefAs(char, gCaption);
	grip			gCel;
	uint			newResNum;
	Rectangle	r;

	// First get rid of the previously created dynamic cel, if any
	scrim.FlushDynamicCel();

	// Create or clone a cel, as required
	if (origResNum)
	{
		// We're using an ani or cel from disk
		gCel = ACloneRes(resType, origResNum, &newResNum, RES_CEL, cel);
	}
	else if (topLine || bottomLine)
	{
		// We're to create partial text
		gCel = ACreatePartialText(&newResNum, cap, AFontColor(FNT_FORE_COLOR), 
											ppDraw, 10, 0, just, topLine, bottomLine);
	}
	else
	{
		// We're to create a dynamic cel

		// Measure the text
		pFontMgr->TextSize(r, cap);

		// Widen the rect by a single pixel border
		r.Set(r.x1, r.y1, r.x2 + 2, r.y2 + 2);

		gCel = ACreateCel(&newResNum, 0, 0, r.Wide(), r.High(), CI_SKIP);
	}

	// Put the caption onto the cel
	ASetString(textX, textY, cap, AGetResData(gCel));

	// Register the resource, position ourselves, and set the rect
	SetRes(RES_CEL, newResNum);
	SetPos(x, y);
}



//----[ TButton Class ]----------------------------------------------

// Constructor
//
TButton::TButton(TControlInfo* pCI) : Label(pCI)
{
	SetName("Button");
	type = CTRL_BUTTON;
	gCaption = NULL;
	pCI->just = DG_JUST_CENTER;
	origResNum = resNum;
	origCel = cel;
	SetAttr(DG_ENABLED, pCI->enabled);
	SetAttr(DG_EXIT, pCI->exit);
}


// Set up the button
//
void
TButton::Setup(TControlInfo* pCI)
{
	Label::Setup(pCI);
}


TButton::~TButton(void)
{
}


void
TButton::Hilite(bool on)
{
//	grip			gCel;
//	uchar*		pData;
//	int			i;
//	Dialog*		pD;
//	uchar			clr;
//	CelHeader*	ch;

	TControl::Hilite(on);

	if (resNum)
	{
		// We're using a cloned resource - Just display the hilite cel
		cel = on ? HILITE_CEL : LOLITE_CEL;
		resNum = origResNum;
		Draw();
	}
	else
	{
//		gCel = ALoad(RES_CEL, on ? HILITE_CEL : LOLITE_CEL);
//		pData = AGetResData(gCel) + sizeof(CelHeader);
//		pD = ADerefAs(Dialog, gDialog);
//		clr = on ? pD->hiliteClr : (uchar) CI_SKIP;
//		ch = ADerefAs(Resource, gCel)->GetHeader(0);
//
//		memset(pData, clr, ch->width);
//		for (i = 1; i < ch->height; i++)
//		{
//			*(pData + i * ch->width) = clr;
//			*(pData + (i + 1) * ch->width - 1) = clr;
//		}
//		memset(pData + ch->height * ch->width, clr, ch->width);
//		Draw(scrim.x, scrim.y, DG_JUST_LEFT);
	}
}


// Select the button.  Return FALSE if we require a release verify and
// it happened outside our rect.  (i.e., clicked the mouse on the button,
// then moved the mouse off the button and released).
//
void
TButton::Select(bool relVerify)
{
	Message		msg;
	uint			ctr = 0;

	TControl::Select(relVerify);
	if (relVerify)
	{
		// Draw the button down
		cel = SELECT_CEL;
		resNum = origResNum;
		Draw();
		AAnimate();

		// Loop until we get a mouse up
		do
		{
			pEventMgr->WaitForEvent(&msg);
		} while (ctr++ < 10000 && msg.event.type != E_MOUSE_UP);

		// Draw the button up again
		cel = HILITE_CEL;
		resNum = origResNum;
		Draw();
		AAnimate();
		if (scrim.rect.Contains(msg.event.x, msg.event.y))
		{
			if (CheckAttr(DG_EXIT))
			{
				APostNotice(N_QUIT, gDialog);
			}
		}
	}
	else
	{
		// Draw the button down
		cel = SELECT_CEL;
		resNum = origResNum;
		Draw();
		AAnimate();

		if (CheckAttr(DG_EXIT))
		{
			APostNotice(N_QUIT, gDialog);
		}
	}
}


bool
TButton::HandleMsg(Message* pMsg)
{
	if (pMsg->type == MSG_EVENT)
	{
		switch (pMsg->event.type)
		{
			case E_KEY_DOWN:
				if (pMsg->event.modifiers & MOD_ALT &&
		 			pMsg->event.value == accel)
				{
					Select(FALSE);
					return TRUE;
				}
				break;

			case E_MOUSE_DOWN:
				if (scrim.rect.Contains(pMsg->event.x, pMsg->event.y))
				{
					Select(TRUE);
					return TRUE;
				}
				break;
		}
	}
	return FALSE;
}



//----[ TTextBox Class ]---------------------------------------------

// Constructor
//
TTextBox::TTextBox(TControlInfo* pCI) : Label(pCI)
{
	SetName("TextBox");
	type = CTRL_TEXT;
	gCaption = NULL;
	origResNum = resNum;
	origCel = cel;
}


// Destructor - get rid of the origLine allocation
//
TTextBox::~TTextBox()
{
	AFree(origLine);
}


// Set up the text box
//
void
TTextBox::Setup(TControlInfo* pCI)
{
	Label::Setup(pCI);

	maxLength = pCI->len;
	editLine = ADerefAs(char, gCaption);
	TRACK_MEM(editLine);	origLine = AMalloc(strlen(editLine) + 1);
	strcpy(ADerefAs(char, origLine), editLine);

	// Calculate the dimensions required of the new cel.
	cwide = maxLength * (AGetCharWidth('M') + 1);
	chigh = AFontPoint();
}


// Draw the text box
//
void
TTextBox::Draw()
{
	uint			hc = 0;	// cursor offsets
	uint			vc = 0;
	uint			charOffset;
	uint			cursorWidth;
	char			editChar;
	uint			newResNum;
	grip			gCel;
	char*			cap = ADerefAs(char, gCaption);

	// First get rid of the previously created dynamic cel, if any
	scrim.FlushDynamicCel();

	// Create or clone a cel, as required
	if (origResNum)
	{
		// We're using an ani or cel from disk
		gCel = ACloneRes(resType, origResNum, &newResNum, RES_CEL, origCel);
	}
	else
	{
		// We're to create a dynamic cel
		TRACK_MEM("TEXT_BOX::Draw");
		gCel = ACreateCel(&newResNum, 0, 0,
								cwide, (chigh + 1),
								AFontColor(FNT_BACK_COLOR));
	}


	// Put the latest copy of the string into the cel
	ASetString(textX, textY, cap, AGetResData(gCel), cwide, 0, DG_JUST_LEFT);

	// If we're active, also draw the cursor
	if (CheckAttr(DG_ACTIVE))
	{
		// Draw the cursor
		AGetCursorPosn(editLine, cursor, cwide, hc, vc, 0);
		charOffset = chSize + vc * cwide + hc;

		// Set the cursor width.
		// If under an existing char, set to char width.
		// If at the end of string, set to widest possible
		editChar = *(editLine + cursor);
		if (!editChar)
		{
			editChar = 'M';
		}
		cursorWidth = AGetCharWidth(editChar);
//		memset(pResData + charOffset, AFontColor(FNT_FORE_COLOR), cursorWidth);
	}

	// Register the resource, position ourselves, and set the rect
	SetRes(RES_CEL, newResNum);
	SetPos(x, y);
}


// Select the text box - edit the text
//
void
TTextBox::Select()
{
	TControl::Select(FALSE);
	cursor = strlen(editLine);
	Draw();

	// Keep editing until we're no longer active
	while (CheckAttr(DG_ACTIVE))
	{
		AMemCycle();
		pContextMgr->Cycle();
		APublishNext();
		pMouse->UpdatePointer();
		AAnimate();
	}
}


// Handle the events, if editing
//
bool
TTextBox::HandleMsg(Message* pMsg)
{
//	Rectangle 	r(x, y, x + cwide, y + chigh);
	bool			ret = FALSE;
	char			buffer[255];

	if (pMsg->type == MSG_EVENT)
	{
		switch (pMsg->event.type)
		{
			case E_MOUSE_DOWN:
				if (scrim.rect.Contains(pMouse->GetX(), pMouse->GetY()))
				{
					Select();
					ret = TRUE;
				}
				break;

			case E_KEY_DOWN:
				if (CheckAttr(DG_ACTIVE))
				{
					switch (pMsg->event.value)
					{
						case K_ENTER:
							if (CheckAttr(DG_EXIT))
							{
								ret = TRUE;
								APostNotice(N_QUIT, gDialog);
							}
							break;

						case K_ESC:
							strcpy(editLine, ADerefAs(char, origLine));
							break;

						case K_BACK_SPACE:
							if (strlen(editLine))
							{
								*(editLine + strlen(editLine) - 1) = NULL;
								--cursor;
							}
							ret = TRUE;
							break;

						case K_DEL:
							if (cursor < strlen(editLine))
							{
								strcpy(buffer, editLine + cursor + 1);
								strcpy(editLine + cursor, buffer);
							}
							ret = TRUE;
							break;

						case K_LEFT:
							if (cursor &&
							 	strlen(editLine))
							{
								--cursor;
							}
							ret = TRUE;
							break;

						case K_RIGHT:
							if (cursor < strlen(editLine))
							{
								++cursor;
							}
							ret = TRUE;
							break;

						case K_HOME:
							cursor = 0;
							ret = TRUE;
							break;

						case K_END:
							cursor = strlen(editLine);
							ret = TRUE;
							break;

						default:
							if (pMsg->event.modifiers == MOD_CTRL)
							{
								switch (pMsg->event.value)
								{
									case K_C:
										*editLine = NULL;
										ret = TRUE;
								}
							}
							else
							{
								if (strlen(editLine) < maxLength)
								{
									memset(buffer, NULL, 255);
									if (cursor)
									{
										memcpy(buffer, editLine, cursor);
									}
									*(buffer + cursor) = AScanToASCII(pMsg->event);
									if (cursor < strlen(editLine))
									{
										strcat(buffer, editLine + cursor);
									}
									strcpy(editLine, buffer);
									++cursor;
								}
								ret = TRUE;
							}
							break;
					}
					Draw();
				}
		}
	}
	return ret;
}


//----[ Window Class ]-----------------------------------------------

// Constructor - create the window using an anim/cel.
//
Window::Window(uint a, uint c, int pri)
{
	SetRes(RES_ANIM, a, c);
	SetPri(pri);

	Show();
	SetName("Window");
}


// Constructor - create the window using a dynamic buffer filled with color.
//
Window::Window(Rectangle* r, uint b, int pri, uchar border)
{
	CelArrayElement	celArrayEl;
	uint32				theSize;
	uint					i;
	uint					w, h;
	grip					gNewCel;
	grip					gCelData;
	uint					newResNum;

	w = r->x2 - r->x1;
	h = r->y2 - r->y1;

	celArrayEl.celHeader.orgX = 0;
	celArrayEl.celHeader.orgY = 0;
	celArrayEl.celHeader.width = (uint16) w;
	celArrayEl.celHeader.height = (uint16) h;
	celArrayEl.celHeader.priority = (int16) pri;
	celArrayEl.celHeader.offset = 0;

	back = b;
	theSize = w * h + sizeof(CelHeader);
	char buffer[40];
	sprintf(buffer, "Win: %d,%d", r->x1, r->y1);
	TRACK_MEM(buffer);	gCelData = AMalloc(theSize);
	celArrayEl.celData = ADerefAs(uchar, gCelData);
	memset(celArrayEl.celData, back, theSize);

	// If border is not DG_NO_BORDER (default), use it as a border color
	if (border != DG_NO_BORDER)
	{
		memset(celArrayEl.celData, border, w);
		for (i = 1; i < h - 1; i++)
		{
			memset(celArrayEl.celData + w * i, border, 1);
			memset(celArrayEl.celData + (w * (i + 1)) - 1, border, 1);
		}
		memset(celArrayEl.celData + w * i, border, w);
	}

	// Create the cel and register it with the ResourceMgr.
	gNewCel = ACreateEasyCel(&celArrayEl, &newResNum);
	AFree(gCelData);
	SetRes(RES_CEL, newResNum);
	scrim.priority = pri;
	SetPos(r->x1, r->y1);
	Show();
	SetName("Window");
}


// Constructor - create the window using a dynamic buffer filled with color.
//
Window::Window(Rectangle* r, int pri, DrawProc ppDrawCel)
{
	CelArrayElement	celArrayEl;
	uint32				theSize;
	uint					w, h;
	grip					gNewCel;
	grip					gCelData;
	uint					newResNum;

	w = r->x2 - r->x1;
	h = r->y2 - r->y1;

	celArrayEl.celHeader.orgX = 0;
	celArrayEl.celHeader.orgY = 0;
	celArrayEl.celHeader.width = (uint16) w;
	celArrayEl.celHeader.height = (uint16) h;
	celArrayEl.celHeader.priority = (int16) pri;
	celArrayEl.celHeader.offset = 0;

	back = CI_SKIP;
	theSize = w * h + sizeof(CelHeader);
	char buffer[40];
	sprintf(buffer, "Win: %d,%d", r->x1, r->y1);
	TRACK_MEM(buffer);	gCelData = AMalloc(theSize);
	celArrayEl.celData = ADerefAs(uchar, gCelData);

	// Create the cel and register it with the ResourceMgr.
	gNewCel = ACreateEasyCel(&celArrayEl, &newResNum);
	AFree(gCelData);

	ppDrawCel(gNewCel);

	SetRes(RES_CEL, newResNum);
	scrim.priority = pri;
	SetPos(r->x1, r->y1);
	Show();
	SetName("Window");
}



//----[ Dialog Class ]-----------------------------------------------

// Constructor - set the window and reset the gCurTControl.
//
Dialog::Dialog(coord X, coord Y, grip gWin, uchar bclr, uchar winBorder)
{
	ppDrawWind = NULL;
	ppDrawTControl = NULL;
	gWindow = NULL;
	gCurTControl = gFirstTControl = gLastTControl = 0;
	margin = DG_MARGIN;
	hiliteClr = CI_BLACK;

	hasExit = FALSE;
	selfDestruct = FALSE;
	retValue = 0;
	SetName("Dialog");

	Initialize(X, Y, gWin, bclr, winBorder);
}


// Constructor - set the window and reset the gCurTControl.
//
Dialog::Dialog()
{
	ppDrawWind = NULL;
	ppDrawTControl = NULL;
	gWindow = NULL;
	gCurTControl = gFirstTControl = gLastTControl = 0;
	margin = DG_MARGIN;
	hiliteClr = CI_BLACK;

	hasExit = FALSE;
	selfDestruct = FALSE;
	retValue = 0;
	SetName("Dialog");
}


// Destructor - delete all dynamically created controls
//
Dialog::~Dialog()
{
	grip		g;

	// delete dynamically allocated controls, relies on virtual destructor
	while (lReceivers.count)
	{
		g = (grip) lReceivers.At(1);

		Object* pObj = ADerefAs(Object, g);
		delete pObj;

		// object's destructor will cut it from lReceivers,
		// therefore this is not an infinite loop
	}

	// guarantee that dialog is deactivated before being destroyed
	ASSERT (!fIsActive);
}


void
Dialog::Initialize(coord X, coord Y, grip gWin, uchar bclr, uchar winBorder)
{
	winType = gWin;
	border = winBorder;
	winColor = bclr;
	x = X;
	y = Y;
}


//
void
Dialog::Initialize(DrawProc ppDrawProc)
{
	winType = DG_DFT_WINDOW;
	border = 0;
	winColor = CI_BLACK;

	x = 0;
	y = 0;
	ppDrawWind = ppDrawProc;
}


// Show the dialog on the screen.
//
void
Dialog::Activate(modalType m, int32 pri)
{
	coord			x1, y1, x2, y2;
	uint			i;
	Window*		pW;
	Scrimage*	pS;
	Rectangle*	pR;
	TControl*		pC;

	// if already active, do nothing
	if (fIsActive) return;

	SetPri(pri);
	mode = m;
	retValue = 0;
	Context::Activate(TRUE);

	switch (winType)
	{
		case DG_NO_WINDOW:
			gWindow = NULL;
			break;

		case DG_DFT_WINDOW:
			// Calculate how large a rectangle we need
			x1 = x;
			x2 = x;
			y1 = y;
			y2 = y;
			for (i = 1; i <= lRoster.count; i++)
			{
				pS = ADerefAs(Scrimage, (grip) lRoster.At(i));
				x2 = AMax(x2, pS->rect.x2);
				y2 = AMax(y2, pS->rect.y2);
			}
			// Add margins all the way around
			x2 += margin * 2;
			y2 += margin * 2;
				   
			// Create a window
			TRACK_MEM("Rect");	pR = new Rectangle(x1, y1, x2, y2, gSelf);
			char buffer[40];
			sprintf(buffer, "DWin: %d,%d", pR->x1, pR->y1);
			TRACK_MEM(buffer);

			if (ppDrawWind)
			{
				pW = new Window(pR, priority, ppDrawWind);
			}
			else
			{
				pW = new Window(pR, winColor, priority, border);
			}

			pW->SetContext(gSelf);
			pW->Setup();
			gWindow = pW->gSelf;
			delete pR;
			break;

		default:
			gWindow = winType;
			break;
	}

	// Now set all the priorities of the objects & draw them
	for (i = 1; i <= lRoster.count; i++)
	{
		pS = ADerefAs(Scrimage, (grip) lRoster.At(i));
		if (pS->gOwner != gWindow)
		{
			// This scrimage belongs to a control
			pC = ADerefAs(TControl, pS->gOwner);
			pC->SetPri(priority + 1);
			pC->Draw();
		}
	}

	if (gWindow)
	{
		pW = ADerefAs(Window, gWindow);
		pW->SetPri(priority);
	}

	// Hilite the current control
	pC = ADerefAs(TControl, gCurTControl);
	pC->Hilite(TRUE);
}


// Read the dialog directives from a .DLG file and then activate it.
//
// Syntax of a .DLG file is:
//
//		KEYWORD: value [value [...]]
//
// Keywords:
//
//		POSN:			x y
//		WINDOW:		w h color pri [borderColor]
//		BUTTON1:		x y caption val [key [exit]]
//		BUTTON2:		ani cel x y caption val [key [exit]]
//		LABEL1:		x y caption [just]
//		LABEL2:		ani cel x y caption [just]
//		TEXTBOX1:	x y caption len
//		TEXTBOX2:	ani cel x y caption len
//
void
Dialog::ActivateFrom(char* filename)
{
	#define		NUM_KW	8

	FILE*			fDlg;
	char			buffer[80];
	char*			p;
	char*			p2;
	char			keyword[15];
	Window*		pW;
	Rectangle	r;

	char			arg[80];
	coord			dx = 0, dy = 0;
	coord			cx, cy;
	coord			tx, ty;
	uint			ani, cel;
	coord			width, height;
	uchar			color1;
	uchar			color2;
	uint			pri;
	int			val;
	char			key;
	bool			exit;
	justify		just;
	uint			mlen;
	char			caption[80];

	int			i;
	char*			kwTable[NUM_KW] =
	{
		"POSN",
		"WINDOW",
		"BUTTON1",
		"BUTTON2",
		"LABEL1",
		"LABEL2",
		"TEXTBOX1",
		"TEXTBOX2",
	};
	enum
	{
		KW_POSN,
		KW_WINDOW,
		KW_BUTTON1,
		KW_BUTTON2,
		KW_LABEL1,
		KW_LABEL2,
		KW_TEXTBOX1,
		KW_TEXTBOX2
	};

	fDlg = fopen(filename, "rt");
	if (!fDlg)
	{
		sprintf(buffer, "Can't open dialog file: %s", filename);
		APanic(buffer);
	}

	strcpy(buffer, "init");
	while (strlen(buffer))
	{
		memset(buffer, NULL, 80);
		fgets(buffer, 255, fDlg);
		p = strchr(buffer, '\n');
		if (p)
		{
			*p = NULL;
		}
		// Ignore comments and blanks
		if (*buffer != '/' && strlen(buffer))
		{
			// Parse arguments
			memset(keyword, NULL, 15);
			p = strchr(buffer, ':');
			memcpy(keyword, buffer, p - buffer);
			strupr(keyword);
			for (i = 0; i < NUM_KW; i++)
			{
				if (!strcmp(keyword, kwTable[i]))
				{
					break;
				}
			}

			if (i == NUM_KW)
			{
				sprintf(buffer, "Unknown keyword: %s", keyword);
				APanic(buffer);
			}

			while (*++p == ' ');

			switch (i)
			{
				case KW_POSN:
					sscanf(p, "%s", arg);
					dx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					dy = atoi(arg);
					break;

				case KW_WINDOW:
					sscanf(p, "%s", arg);
					width = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					height = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					color1 = (uchar) atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					pri = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					color2 = (uchar) atoi(arg);

					r.Set(0, 0, width, height);
					TRACK_MEM("DlgWinow");	pW = new Window(&r, color1, pri, color2);
					Initialize(dx, dy, pW->gSelf, color1, color2);
					pW->SetContext(gSelf);
					pW->Setup();
					break;

				case KW_BUTTON1:
					sscanf(p, "%s", arg);
					cx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cy = atoi(arg);
					p += strlen(arg) + 1;

					if (*p == '\"')
					{
						// Quoted string, search for other quote
						p2 = strchr(p + 1, '\"');
						memset(caption, NULL, 80);
						strncpy(caption, p + 1, p2 - p - 1);
						p = p2 + 2;
					}
					else
					{
						// Single word, no quotes
						sscanf(p, "%s", caption);
						p += strlen(caption) + 1;
					}

					sscanf(p, "%s", arg);
					val = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					key = (char) atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					exit = (!strcmp(arg, "TRUE")) ? TRUE : FALSE;

//					AddButton(cx, cy, caption, val, key, exit);
					break;

				case KW_BUTTON2:
					sscanf(p, "%s", arg);
					ani = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cel = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cy = atoi(arg);
					p += strlen(arg) + 1;

					if (*p == '\"')
					{
						// Quoted string, search for other quote
						p2 = strchr(p + 1, '\"');
						memset(caption, NULL, 80);
						strncpy(caption, p + 1, p2 - p - 1);
						p = p2 + 2;
					}
					else
					{
						// Single word, no quotes
						sscanf(p, "%s", caption);
						p += strlen(caption) + 1;
					}

					sscanf(p, "%s", arg);
					val = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					key = (char) atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					exit = (!strcmp(arg, "TRUE")) ? TRUE : FALSE;

//					AddButton(ani, cel, cx, cy, caption, val, key, exit);
					break;

				case KW_LABEL1:
					sscanf(p, "%s", arg);
					cx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cy = atoi(arg);
					p += strlen(arg) + 1;

					if (*p == '\"')
					{
						// Quoted string, search for other quote
						p2 = strchr(p + 1, '\"');
						memset(caption, NULL, 80);
						strncpy(caption, p + 1, p2 - p - 1);
						p = p2 + 2;
					}
					else
					{
						// Single word, no quotes
						sscanf(p, "%s", caption);
						p += strlen(caption) + 1;
					}

					sscanf(p, "%s", arg);
					if (!strcmp(arg, "CENTER"))
					{
						just = DG_JUST_CENTER;
					}
					else if (!strcmp(arg, "RIGHT"))
					{
						just = DG_JUST_RIGHT;
					}
					else
					{
						just = DG_JUST_LEFT;
					}

//					AddLabel(cx, cy, caption, just);
					break;

				case KW_LABEL2:
					sscanf(p, "%s", arg);
					ani = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cel = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cy = atoi(arg);
					p += strlen(arg) + 1;

					if (*p == '\"')
					{
						// Quoted string, search for other quote
						p2 = strchr(p + 1, '\"');
						memset(caption, NULL, 80);
						strncpy(caption, p + 1, p2 - p - 1);
						p = p2 + 2;
					}
					else
					{
						// Single word, no quotes
						sscanf(p, "%s", caption);
						p += strlen(caption) + 1;
					}

					sscanf(p, "%s", arg);
					if (!strcmp(arg, "CENTER"))
					{
						just = DG_JUST_CENTER;
					}
					else if (!strcmp(arg, "RIGHT"))
					{
						just = DG_JUST_RIGHT;
					}
					else
					{
						just = DG_JUST_LEFT;
					}

//					AddLabel(ani, cel, cx, cy, caption, just);
					break;

				case KW_TEXTBOX1:
					sscanf(p, "%s", arg);
					cx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cy = atoi(arg);
					p += strlen(arg) + 1;

					if (*p == '\"')
					{
						// Quoted string, search for other quote
						p2 = strchr(p + 1, '\"');
						memset(caption, NULL, 80);
						strncpy(caption, p + 1, p2 - p - 1);
						p = p2 + 2;
					}
					else
					{
						// Single word, no quotes
						sscanf(p, "%s", caption);
						p += strlen(caption) + 1;
					}

					sscanf(p, "%s", arg);
					mlen = atoi(arg);
					p += strlen(caption) + 1;
					sscanf(p, "%s", arg);
					tx = atoi(arg);
					p += strlen(caption) + 1;
					sscanf(p, "%s", arg);
					ty = atoi(arg);

//					AddTextBox(cx, cy, caption, mlen, tx, ty);
					break;

				case KW_TEXTBOX2:
					sscanf(p, "%s", arg);
					ani = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cel = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cx = atoi(arg);
					p += strlen(arg) + 1;
					sscanf(p, "%s", arg);
					cy = atoi(arg);
					p += strlen(arg) + 1;

					if (*p == '\"')
					{
						// Quoted string, search for other quote
						p2 = strchr(p + 1, '\"');
						memset(caption, NULL, 80);
						strncpy(caption, p + 1, p2 - p - 1);
						p = p2 + 2;
					}
					else
					{
						// Single word, no quotes
						sscanf(p, "%s", caption);
						p += strlen(caption) + 1;
					}

					sscanf(p, "%s", arg);
					mlen = atoi(arg);
					p += strlen(caption) + 1;
					sscanf(p, "%s", arg);
					tx = atoi(arg);
					p += strlen(caption) + 1;
					sscanf(p, "%s", arg);
					ty = atoi(arg);

//					AddTextBox(ani, cel, cx, cy, caption, mlen, tx, ty);
					break;
			}
		}
	}
	fclose(fDlg);
	Activate(M_MODAL);
}


// Show the dialog on the screen.
//
int
Dialog::Deactivate()
{
	Window*	pWin;
	int		ret;

	// if already de-active, do nothing
	if (!fIsActive) return 0;

	if (gCurTControl)
	{
		retValue = ret = ADerefAs(TControl, gCurTControl)->value;
	}
	else
	{
		retValue = ret = -1;
	}

	if (gWindow)
	{
		pWin = ADerefAs(Window, gWindow);
		pWin->Hide();
		if (winType == DG_DFT_WINDOW)
		{
			// Dynamic window, must deallocate
			delete pWin;
		}
		gWindow = 0;
		winType = 0;
	}
	Context::Activate(FALSE);

	if (selfDestruct)
	{
		delete this;
		return ret;
	}

	return retValue;
}


// Handle messages.
//
bool
Dialog::HandleMsg(Message* pMsg)
{
	bool	claimed = FALSE;

	// First let all the dialog's controls handle the message
	if (pMsg->type == MSG_EVENT &&
			(pMsg->event.type == E_KEY_DOWN || pMsg->event.type == E_MOUSE_DOWN)
		)
	{
		claimed = Context::HandleMsg(pMsg);
	}

	// If not claimed, handle it ourselves
	if (!claimed)
	{
		switch (pMsg->type)
		{
			case MSG_NOTICE:
				if (pMsg->notice.type == N_QUIT)
				{
					Deactivate();
				}
				break;

			case MSG_EVENT:
				switch (pMsg->event.type)
				{
					case E_KEY_DOWN:
						switch (pMsg->event.value)
						{
							case K_RETURN:
								// Enter key simply exits the dialog (like ESC) if there
								// are no DG_EXIT controls.
								if (!hasExit)
								{
									claimed = TRUE;
									Deactivate();
								}
								else
								{
									// otherwise select current control, DG_EXIT
									// this will exit the dialog
									if (gCurTControl)
									{
										TControl*	pCtrl = ADerefAs(TControl, gCurTControl);
										pCtrl->Select(FALSE);
									}
								}
								break;
						
							case K_ESC:
								// Escape key always exits a dialog with a -1 return.
								claimed = TRUE;
								Deactivate();
								retValue = -1;
								break;

							case K_TAB:
								if (pMsg->event.modifiers & MOD_SHIFT)
								{
									PrevTab();
								}
								else
								{
									NextTab();
								}
								claimed = TRUE;
								break;
						}
						break;

					case E_MOUSE_DOWN:
						if (!hasExit)
						{
							claimed = TRUE;
							Deactivate();
						}
						break;
				}
		}
	}


	// unless the Context is modal pass event on to other contexts
	
	if (mode == M_MODAL && claimed)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// Position the dialog on the screen.
void
Dialog::SetPos(coord l, coord t)
{
	uint		i;

	x = l;
	y = t;

	if (gWindow)
	{
		Window*	w = ADerefAs(Window, gWindow);
		w->SetPos(l, t);
		w->scrim.SetRect();
		w->SetState(S_CHANGED, TRUE);
	}

	for (i = 1; i <= lRoster.count; i++)
	{
		Scrimage* s = ADerefAs(Scrimage, (grip) lRoster.At(i));
		if (s->gOwner != gWindow)
		{
			TControl* pC = ADerefAs(TControl, s->gOwner);
			pC->SetPos(pC->x, pC->y);
		}
	}
}


// Set the priority of the dialog.
void
Dialog::SetPri(int32 pri)
{
	priority = pri;
}


// Set draw procedure of an owned control
void
Dialog::SetTControlDrawProc(DrawProc ppDrawProc)
{
	ppDrawAtConstruction = ppDrawProc;
}


// Justify the Window
//
void
Dialog::JustifyWindow(justify justX, justify justY)
{
	coord	rX, rY;

	if (gWindow)
	{
		Rectangle *pWindowR = &ADerefAs(Window, gWindow)->scrim.rect; 

		switch (justX)
		{
			case DG_JUST_LEFT:
 				rX = 0;
				break;

			default:
			case DG_JUST_CENTER:
 				rX = (AMaxX() - pWindowR->Wide()) / 2;
				break;

			case DG_JUST_RIGHT:
 				rX = AMaxX() - pWindowR->Wide();
		}

		switch (justY)
		{
			case DG_JUST_TOP:
 				rY = 0;
				break;

			default:
			case DG_JUST_CENTER:
 				rY = (AMaxY() - pWindowR->High()) / 2;
				break;

			case DG_JUST_BOTTOM:
 				rY = AMaxY() - pWindowR->High();
		}

		SetPos(rX, rY);
	}
}


// Fill in the rect passed with the window's dimensions
//
void
Dialog::GetWindowRect(Rectangle* rWin)
{
	Window*	pWin;

	if (gWindow)
	{
		pWin = ADerefAs(Window, gWindow);
		rWin->x1 = pWin->scrim.rect.x1;
		rWin->y1 = pWin->scrim.rect.y1;
		rWin->x2 = pWin->scrim.rect.x2;
		rWin->y2 = pWin->scrim.rect.y2;
//		rWin->Copy((void*) &((ADerefAs(Window, gWindow))->scrim.rect));
	}
	else
	{
		// no window
		rWin->Set(0,0,0,0);
	}
}


//	Run a modal dialog until it is closed.  Returns retValue.
//
int
Dialog::EventLoop(voidFuncPtr loopFunction)
{
	// Loop until something deactivates the dialog
	while (fIsActive)
	{
		if (loopFunction)
		{	
			// user defined main loop
			loopFunction();
		}
		else 
		{
			// default main loop
			AMemCycle();
			pContextMgr->Cycle();
			APublishNext();
			pMouse->UpdatePointer();
			AAnimate();
		}
	}

	return retValue;
}


// Advance gCurTControl to next in tab order
//
grip
Dialog::NextTab()
{
	TControl*	pC;
	
	pC = ADerefAs(TControl, gCurTControl);
	pC->Hilite(FALSE);

	do
	{
		gCurTControl = pC->gNext;
		pC = ADerefAs(TControl, gCurTControl);
	} while (!pC->CheckAttr(DG_ENABLED));

	pC->Hilite(TRUE);
	return gCurTControl;
}


// Retreat gCurTControl to previous in tab order
//
grip
Dialog::PrevTab()
{
	TControl*	pC;
	
	pC = ADerefAs(TControl, gCurTControl);
	pC->Hilite(FALSE);

	do
	{
		gCurTControl = pC->gPrev;
		pC = ADerefAs(TControl, gCurTControl);
	} while (!pC->CheckAttr(DG_ENABLED));

	pC->Hilite(TRUE);
	return gCurTControl;
}


// Add a button to the dialog.
//
grip
Dialog::AddButton(TControlInfo* pCI)
{
	TRACK_MEM(pCI->cap);	TButton* b = new TButton(pCI);
	b->Setup(pCI);
	AddTControl(b->gSelf);
	return b->gSelf;
}


// Add a label to the dialog.
//
grip
Dialog::AddLabel(TControlInfo* pCI, uint tline, uint bline)
{
	TRACK_MEM(pCI->cap);	Label* l = new Label(pCI);
	l->Setup(pCI, tline, bline);
	AddTControl(l->gSelf);
	return l->gSelf;
}


// Add a textbox to the dialog
//
grip
Dialog::AddTextBox(TControlInfo* pCI)
{
	TRACK_MEM(pCI->cap);	TTextBox* t = new TTextBox(pCI);
	t->Setup(pCI);
	AddTControl(t->gSelf);
	return t->gSelf;
}


// Add the object to the Context's list.  Overridden to add the priority
//	to the object.
//
void
Dialog::AddTControl(grip gObj)
{
	TControl*	pFirst;
	TControl*	pLast;
	TControl* pCon = ADerefAs(TControl, gObj);

	// Set up the double link
	gCurTControl = pCon->gSelf;
	pCon->gDialog = gSelf;

	// Mark if we have at least one exit control (affects how ENTER key works)
	if (pCon->CheckAttr(DG_EXIT))
	{
		hasExit = TRUE;
	}

	// If there's a window, use its priority, else default to 1000.
	if (gWindow)
	{
		pCon->SetPri(ADerefAs(Window, gWindow)->scrim.priority + 1);
	}
	else
	{
		pCon->SetPri(1000);
	}
	pCon->Draw();

	// Set up the tab order, unless this is the first control.  Since the
	// first item in the Dialog's roster may or may not be a window, we
	// keep track of the first control added.  Makes things much easier.
	if (gFirstTControl)
	{
		pFirst = ADerefAs(TControl, gFirstTControl);
		pLast = ADerefAs(TControl, gLastTControl);
		pCon->gPrev = gLastTControl;
		pCon->gNext = gFirstTControl;
		pLast->gNext = gObj;
		pFirst->gPrev = gObj;
	}
	else
	{
		gFirstTControl = gObj;
	}

	pCon->SetContext(gSelf);

	Context::AddObject(gObj, CT_MSGS | CT_ROST);
	gLastTControl = gObj;
}

