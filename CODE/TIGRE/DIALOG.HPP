//
// DIALOG.HPP
//
// January 26, 1994
// Brian K. Hughes
// WATCOM: September 23, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// TControl class definition:
//
//		A TControl is a base dialog element.  It is an abstract class,
//		that is, it can be subclassed but not instantiated.  All dialog
//		controls are derived from TControl.  Dialog controls include:
//
//		TButton - Selectable (pushable) control
//		TTextBox - Editable text
//		Label - Non-editable text
//		ListBox - Pick list
//		ComboBox - TTextBox and drop-ListBox combined
//		CheckBox - Boolean indicator
//		OptionButton - Radio buttons
//
// Window class definition:
//
//		A Window is a visual plane into which other items may be placed.
//		For dialogs, a Window provides a solid color background that may
//		have special borders and/or corner pieces.  Items placed into a
//		window will be positioned relative to the window's top left, and
//		are moved when the window is moved.
//
//	Dialog class definition:
//
//		The Dialog handles creating the window, displaying the dialog's
//		elements, and handling the user's events.
//
//----[]-------------------------------------------------------------

#ifndef	dialog_hpp
#define	dialog_hpp

#include "context.hpp"
#include "keybrd.hpp"
#include "scrimage.hpp"
#include "tigre.hpp"

typedef	void	(*voidFuncPtr)();

// TControl attribute defines
#define	DG_EXIT		0x01	// Selecting control will exit dialog
#define	DG_ENABLED	0x02	// TControl is functional
#define	DG_ACTIVE	0x04	// TControl is dialog's current control

// Defines for which cel to use for controls that use ani/cel and don't
// override the hilite and/or select functions
#define	LOLITE_CEL	origCel
#define	HILITE_CEL	origCel + 1
#define	SELECT_CEL	origCel + 2


// Structure used to create dialog items
//
struct	TControlInfo
{
	coord		x;			// Position of control, relative to window
	coord		y;
	coord		tx;		// Offset of caption within cel rect
	coord		ty;
	res_t		resType;	// Resource type to use
	uint		resNum;	// Resource number to use
	uint		cel;		// Cel of resource to use
	bool		exit;		// Does the control close the dialog?
	bool		enabled;	// Does the control start out enabled?
	uchar		key;		// Accelerator key, or 0
	int		value;	// Return value
	char*		cap;		// Caption, or NULL
	justify	just;		// Text justification
	coord		w;			// Generic width
	coord		h;			// Generic height
	uint		len;		// Generic length

	TControlInfo()
	{
		memset(this, NULL, sizeof(TControlInfo));
		resType = RES_CEL;
		exit = FALSE;
		enabled = FALSE;
		just = DG_JUST_CENTER;
	}
};


//----[ TControl Class ]----------------------------------------------

class TControl : public Guy
{
	public:

		grip		gDialog;		// Dialog that owns us
		int		value;		// Return value of control
		grip		gNext;		// Tab to next
		grip 		gPrev; 		// Shift-tab to prev
		char		accel;		// Accelerator key
		res_t		resType;		// Resource type used
		uint		resNum;		// Resource number used
		uint		cel;			// Cel number used
		coord		x;				// -Position
		coord		y;				// /

		TControl(TControlInfo* pCI);
		virtual ~TControl() = 0;	// Makes TControl an abstract, must subclass

		virtual	void	Draw();
		virtual	bool	IsType(ctrl_t t);				// return if type 't'
		virtual	void	Hilite(bool on);		 		// hilite or lolite
		virtual	void	Select(bool relVerify);		// select the control
		virtual	bool	HandleMsg(Message* pMsg);
		virtual	bool	CheckAttr(uint a);
		virtual	void	SetAttr(uint a, bool on);
		virtual	void	SetPos(coord cx, coord cy);// Position the control

	protected:

		DrawProc	ppDraw;

		ctrl_t	type;			// control type
		uint		attr;			// attributes of the control
};



//----[ Label Class ]------------------------------------------------

class Label : public TControl
{
	public:

		grip		gCaption;
		uint		fore;
		uint		back;
		uint		maxWidth;
		coord		textX;		// Text offsets
		coord		textY;
		uint		topLine;		// For partial text
		uint		bottomLine;
		justify	just;
		uint		origResNum;	// -Copies of original resNum & cel
		uint		origCel;		// /

		Label(TControlInfo* pCI);
		virtual ~Label();
		
					void	Setup(TControlInfo* pCI, uint tline = 0, uint bline = 0);
		virtual	void	Draw();
};



//----[ TButton Class ]----------------------------------------------

class TButton : public Label
{
	public:

		TButton(TControlInfo* pCI);
		virtual ~TButton();

					void	Setup(TControlInfo* pCI);

		virtual	void	Hilite(bool on);
		virtual	void	Select(bool relVerify);
		virtual	bool	HandleMsg(Message* pMessage);
};



//----[ TTextBox Class ]---------------------------------------------

class TTextBox : public Label
{
	public:

		TTextBox(TControlInfo* pCI);
		virtual ~TTextBox();

					void	Setup(TControlInfo* pCI);

		uint			cursor;		// char position of cursor
		char*			editLine;	// temporary line for editing
		grip			origLine;	// original string
		uint			maxLength;	// maximum # chars allowed
		uint			cwide;		// Dimensions of cel to use
		uint			chigh;

		virtual	void	Draw();							// redraw the textbox
		virtual	void	Select();						// Select (edit) the textbox
		virtual	bool	HandleMsg(Message* pMsg);	// Handle events
};


//----[ Window Class ]-----------------------------------------------

class Window : public Guy
{
	public:
	
		Window(uint anim, uint cel, int pri);
		Window(Rectangle* r, uint backColor, int pri, uchar borderColor = DG_NO_BORDER);
		Window(Rectangle* r, int pri, DrawProc ppDrawBorder = NULL);

	protected:
		uint		fore;
		uint		back;
};


//----[ Dialog Class ]-----------------------------------------------

class Dialog : public Context
{
	public:

		Dialog();
		Dialog(coord x, coord y, grip gWin = DG_DFT_WINDOW, uchar bclr = 0, uchar border = DG_NO_BORDER);
		~Dialog();

		grip			gCurTControl;	// TControl that is currently hilit
		grip			gFirstTControl;	// First control in dialog
		grip			gLastTControl;	// Last control in dialog
		bool			selfDestruct;	// Dialog will self-destruct when done
		int			retValue;		// Return value of dialog
		int			margin;			// Margin for text placement (dft = DG_MARGIN)
		uchar			hiliteClr;		// Color for control hilite (dft = black)

		virtual void		Initialize(coord x, coord y, grip gWin = DG_DFT_WINDOW, uchar bclr = 0, uchar border = DG_NO_BORDER);
		virtual void		Initialize(DrawProc ppDrawProc);
		virtual void		Activate(modalType dlgMode = M_MODELESS, int32 pri = 1000);
		virtual void		ActivateFrom(char* filename);
		virtual int			Deactivate();

		virtual bool		HandleMsg(Message* pMessage);
		virtual void		SetPos(coord l, coord t);
		virtual void		SetPri(int32 pri);
		virtual void		SetTControlDrawProc(DrawProc ppDraw);
		virtual void		JustifyWindow(justify justX = DG_JUST_CENTER, justify justY = DG_JUST_CENTER);
		virtual void		GetWindowRect(Rectangle *rWin);
		virtual int			EventLoop(voidFuncPtr loopFunction = NULL);
		virtual grip		NextTab();
		virtual grip		PrevTab();

		virtual grip		AddButton(TControlInfo* pCI);
		virtual grip		AddLabel(TControlInfo* pCI, uint tline = 0, uint bline = 0);
		virtual grip		AddTextBox(TControlInfo* pCI);
		virtual void		AddTControl(grip gObj);


	protected:

		uint			winType;			// DG_DFT_WINDOW, DG_NO_WINDOW, or grip
		uchar			winColor;		// Background color of window (if DG_DFT_WINDOW)
		uchar			border;			// Border color of window
		coord			x;					// Position of the dialog
		coord			y;

		grip			gWindow;			// Window or NULL
		bool			hasExit;			// At least one control is a DG_EXIT
		int32			priority;		// Priority of the dialog

		DrawProc		ppDrawWind;
		DrawProc		ppDrawTControl;

};


#endif


/*
//----[About Dialogs]------------------------------------------------

TIGRE dialogs are formed on the same principles as the Windows interface.
A dialog consists of an instance of Dialog (or subclass), which derives
from Context, one or more TControls, and a Window.  The window serves as
the backdrop for the placement of controls and controls are positioned
relative to the window's position.


TControls:
--------

TControls exist in one of three states:

   Inactive (lolit) - The control is not the dialog's current control
	   and will only react to tab, accelerator, and mouse events.

	Active (hilit) - The control is the dialog's current control and
	   will react to all events.  A control will become active if tabbed
		or shift-tabbed to.  Pressing ENTER will usually select an active
		control (see below).

	Selected - The control has been selected either with the ENTER key
	   when active, a mouse click, or via the control's accelerator key
		(see below).  Exit controls will close the dialog when selected
		(see below).

Note that some controls, when selected, behave exactly as they do when
merely activated.  A text box, for example, will display a cursor and allow
editing when made active or when selected.  When the ENTER key is pressed,
the text box may become inactive or may close the dialog (see below).


In addition to the above, there are three attribute bits that further govern
the way in which a control operates:

	DG_EXIT - The control is an "exit" control and, when selected, will
	   close the dialog.  If this bit is not set, the control's behavior
		may still vary when selected, depending on the control type.

	DG_ENABLED - The control may become active and may be selected.  If
	   this bit is not set, the control will not be included in the tab
		order and its accelerator key (if any) will be non-functional.

	DG_ACTIVE - The control is the dialog's current control.  Pressing
	   the ENTER key will usually select the control, unless the control's
		active behavior for the ENTER key is identical to its selected
		behavior (see text box discussion above).


Accelerators:
------------

Each control may have an accelerator key associated with it.  Pressing the
ALT key and the accelerator key will select the control, regardless of tab
order.  If the control has a caption (such as labels and buttons), the
letter used as the accelerator may be displayed in an alternate color by
surrounding it with carets, as in:

	^A^ccept

It is important to note the functional difference between TIGRE and Windows
interfaces, with respect to accelerator keys.  In Windows, the accelerator
key is created by the control's caption, as in:

	Employee &Names

which uses Alt-N as the accelerator automatically.  TControls that do not
have captions cannot have accelerator keys.  In addition, controls that
cannot be made active, such as labels, cannot use accelerator keys that
they are allowed to create, so creating one in its caption causes Windows
to activate the next control in the tab order.  Thus, although labels may
be used to create accelerators for captionless controls, this process is
tab order dependent; that is, the label with the accelerator must immediately
precede the captionless control in the tab order.

In TIGRE, there is no connection whatsoever between a control's caption and
its accelerator key.  Using carets to display a letter in an alternate color
is for visual feedback to the user only.  Since this is the case, however,
labels may be easily used as visual indicators of a captionless control's
accelerator.

For example, consider a dialog with a pick list of employee names.  The pick
list has an accelerator key of Alt-N, but no caption with which to indicate
the key to the user.  A label placed near the pick list with the caption:

   Employee ^N^ames:

provides the proper visual cues without creating unusable accelerators or
requiring a specific tab order.



Notes on Buttons:
----------------

The default behavior for buttons is to select when clicked on, when the
accelerator key is pressed, or when active and the ENTER key is pressed.
Button selection must be release-verified; that is, if clicked on with the
mouse, the user must release the mouse on the button for the selection to
be completed.  If the user moves the mouse off the button and releases,
the button simply reverts to its active state and no selection takes place.
Buttons are not release-verified for key events, so pressing the accelerator
key or ENTER will select the button immediately.

Buttons that use dynamically created cels will be surrounded by a black ring
when active, and surrounded by a ring of skip color when inactive.  Buttons
that use an ANI file must have the cels arranged in a specific order:

	State          Cel to use
	-----          ----------
	Inactive       any
	Active         inactive + 1
	Selected       inactive + 2


Hilite:

The Hilite method may be overridden to provide special visual feedback when
in-/activated.  The Hilite method takes a single boolean parameter that is
TRUE if the button should be made active, FALSE if it should be made inactive.


Select:

The Select method may be overridden to provide special visual feedback when
selected.  The Select method takes a single boolean parameter that is TRUE
if the selected should be release-verified, FALSE if the selection should be
immediate.  The Select method should return a boolean that is TRUE if the
selection was valid (immediate or verified), or FALSE if the selection was
invalid (not verified).


*/
