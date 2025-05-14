/*
 * _DWDISP.H
 *
 * DESCRIPTION
 *
 * Defines and prototypes for DataWindows 3.0 and above.
 * This allows the library to compile for DOS without the need
 * for the DataWindows header files.
 *
 *
 * The Greenleaf Comm++ Library
 *
 * Copyright (C) 1989-95 Greenleaf Software Inc.  All Rights Reserved.
 *
 * MODIFICATIONS
 *  August 19, 1994 - Modified to support Watcom C++ 10.0.
 */

#ifndef _DWDISP_H
#define _DWDISP_H

#include "cppcomp.h"

#define UsePhysicalAttributes   physattr
#define bldattr(fg,bg)          (int)(((fg)|((bg)<<4)) & 0xff)
#define MAXPUSH                 8
#define MAXATTVALUE             255
#define MAXPHYATTVALUE          16
#define CURRENT                 -1
#define vrputc(x1,x2,x3)        vratputc(x1,CURRENT,CURRENT,x2,x3)
#define DW_YES                  1
#define DWSUCCESS               0
#define DW_NONE                 (HWND)0


#ifndef _HWNDDEFINED
#define _HWNDDEFINED	1
typedef int		HWND;
#endif

/* CURENT & CURSTACK define the entry in the window state structure
 *  that is responsible for keeping lcurpush() & lcurpop() information.
 */
typedef struct {
	int crow;
	int ccol;

} CURENT;


typedef struct {
	int currententries;		/* Number of entries in stack	*/
	CURENT xentry[MAXPUSH]; 	/* The individual entries	*/

} CURSTACK;


/* LOGPHYSATTRIBUTE defines the structure for the logical->physical attribute
 *  Table.  Each location in this table will represent a physical attribute
 *  it will be indexed with a logical attribute.  At setup time the correct
 *  logical/physical attribute table will be installed that corresponds
 *  with the currently connected display.
 */
typedef struct {
	unsigned physicalattribute[MAXATTVALUE+1];

} LOGPHYSATTRIBUTE;


typedef struct {
	unsigned physicalattribute[MAXPHYATTVALUE + 1];

} STDLOGPHYSATTRIBUTE;


/* The ZOOMSTAT structure is used for temporary storage of information
 *  describing the dimensions of the window before it is zoomed.
 */
typedef struct {
	int zlrn;		/* Number of rows in logical window	*/
	int zlcn;		/* Number of columns in logical window	*/
	int zprn;		/* Number of rows in physical window	*/
	int zpcn;		/* Number of columns in physical window	*/
	int zlr1;		/* Upper row of logical window		*/
	int zlc1;		/* Upper column of logical window	*/
	int zpr1;		/* Upper row on physical screen		*/
	int zpc1;		/* Left column on physical screen	*/

} ZOOMSTAT;


/* Each window as it is created will have associated with it a
 *  structure that is responsible for storing the state information
 *  for the window.  The window will be referenced throughout the
 *  system by a window handle.  This handle is the index into an array
 *  of pointers to window state information variables.
 */
typedef struct {
	unsigned toppriority:	1;
	unsigned needsrefresh:	1;
	unsigned hasrealcursor: 1;
	unsigned getsrealcursor:1;
	unsigned dynframe:1;
	unsigned dyntitle:1;	/* top title loaded from disk is dynamic    */
	unsigned dynrtitle:1;	/* right title loaded from disk is dynamic  */
	unsigned dynbtitle:1;	/* bottom title loaded from disk is dynamic */
	unsigned dynltitle:1;	/* left title loaded from disk is dynamic   */
	unsigned WSTpad:7;	/* word align				    */

} WINSTATBITS;


typedef struct {
	unsigned visible:	1;
	unsigned hasframe:	1;
	unsigned hasshadow:	6;
	unsigned cursdrag:	1;
	unsigned curswrap:	1;
	unsigned autoscroll:	1;
	unsigned wordwrap:	1;
    unsigned frameoffscr:   1;
    unsigned physattr:  1;
    unsigned WMODpad:   2;

} WINMODEBITS;

typedef struct {
	int	priority;	/* Priority of window			*/
	WINSTATBITS wstatus;	/* Status bits for this window		*/
	WINMODEBITS wmode;	/* Bits defining current modes for window */
	int	 pacol, 	/* Absolute column/row position on	*/
		 parow; 	/* physical output device.		*/
	int	 col,		/* Logical Cursor column/row position	*/
		 row;		/* relative to logical window		*/
	int	 pc1,		/* Leftmost column/row of the physical	*/
		 pr1;		/* window relative to screen		*/
	int	 pcn,		/* The number of columns/rows in the	*/
		 prn;		/* physical window			*/
	int	 lc1,		/* The leftmost column/row of the physical */
		 lr1;		/* window relative to the logical window*/
	int	 lcn,		/* The maximum number of columns/rows	*/
		 lrn;		/* in the logical window		*/
	unsigned int pspmcalc,	/* Pspma Calculation save area		*/
		 llevchar,	/* Storage bucket for low-level character */
		 llevattr;	/* Storage bucket for low-level attribute */
	int frameattr;		/* Attribute for frame			*/
	int titleattr;		/* Attribute for top title		*/
	int rtitleattr; 	/* Attribute for right title		*/
	int btitleattr; 	/* Attribute for bottom title		*/
	int ltitleattr; 	/* Attribute for left title		*/
	int titlepos;		/* Start position of top title		*/
	int rtitlepos;		/* Start position of right title	*/
	int btitlepos;		/* Start position of bottom title	*/
	int ltitlepos;		/* Start position of left title		*/
	int shadowchar; 	/* Character used for shadowing		*/
	int shadowattr; 	/* Attribute used for shadowing		*/
	int clearchar;		/* Character used for clearing		*/
	int clearattr;		/* Attribute used for clearing		*/
	int currattr;		/* Current attribute			*/
	HWND myhandle;		/* Handle for this window		*/
	ZOOMSTAT  *wzoom;	/* Points To structure to hold zoomed stuff */
	CURSTACK *custack;	/* Points to Cursor stack		*/
  unsigned int *rowtble; /* Points to table of row    */
	char *logscreen;	/* Points to logical screen buffer	*/
	char *frame;		/* Points to frame variable		*/
	char *title;		/* Points to top title			*/
	char *rtitle;		/* Points to right title		*/
	char *btitle;		/* Points to bottom title		*/
	char *ltitle;		/* Points to left title			*/

} WINSTATE;

#ifdef __cplusplus
extern "C" {
#endif

int  GCPP_FUN physattr( int );
HWND GCPP_FUN vcreat(int ,int ,int, int );
int  GCPP_FUN curdrag(HWND,int);
int  GCPP_FUN visible(HWND,int,int);
int  GCPP_FUN vdelete(HWND,HWND);
int  GCPP_FUN vclear(HWND);
int  GCPP_FUN pcurset(HWND,int,int);
int  GCPP_FUN setattr(HWND,int);
int  GCPP_FUN vclrattr(HWND,int);
int  GCPP_FUN vratputc(HWND,int,int,int,char);
int  GCPP_FUN vscrolup(HWND,int);
int  GCPP_FUN vscroldn(HWND,int);
char GCPP_FUN vgetc(HWND,int,int,int *);
int  GCPP_FUN leeol(HWND);
int  GCPP_FUN leeow(HWND);
WINSTATE * GCPP_CFUN _vgwsta(HWND);
void GCPP_FUN _vrefwnd(WINSTATE *);
void GCPP_CFUN pcuroff(void);
void GCPP_CFUN pcuron(void);
int GCPP_FUN curwrap(HWND,int);
int GCPP_FUN autoscrl(HWND,int);

#ifdef __cplusplus
}
#endif
#endif
