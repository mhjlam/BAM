/*
 * IBMKEYS.H
 * 
 *  Copyright (C)1986-1995 Greenleaf Software Inc. All Rights Reserved.
 *       
 * DESCRIPTION
 *
 *  Define Keys for the 83/84 key and 101/102 key IBM PC keyboards,
 *  structures, constants and function prototypes for keyboard functions.
 *
 * MODIFICATIONS
 *  Dwayne Lederhosen  12-JAN-1989  16:28:14.76
 *    Added constants, structure and function prototypes for UserIn()
 *    function.  Also changed #defines due to removal of getkey()
 *    translate table.
 *
 *  September 17, 1991  3.20 : Released with no changes since 3.10.
 */

#ifndef _IBMKEYS_DOT_H
#define _IBMKEYS_DOT_H

#include "cppcomp.h"
                                                                
#define F1              0x3b00  /* F1 ...........               */
#define F2              0x3c00
#define F3              0x3d00
#define F4              0x3e00
#define F5              0x3f00
#define F6              0x4000
#define F7              0x4100
#define F8              0x4200
#define F9              0x4300
#define F10             0x4400
#define F11             0x8500  /* Enhanced Keyboards Only      */
#define F12             0x8600  /* Enhanced Keyboards Only      */

#define SF1             0x5400  /* Shift-F1 ......              */
#define SF2             0x5500
#define SF3             0x5600
#define SF4             0x5700
#define SF5             0x5800
#define SF6             0x5900
#define SF7             0x5a00
#define SF8             0x5b00
#define SF9             0x5c00
#define SF10            0x5d00
#define SF11            0x8700  /* Enhanced Keyboards Only      */
#define SF12            0x8800  /* Enhanced Keyboards Only      */

#define CF1             0x5e00  /* Control-F1                   */
#define CF2             0x5f00
#define CF3             0x6000
#define CF4             0x6100
#define CF5             0x6200
#define CF6             0x6300
#define CF7             0x6400
#define CF8             0x6500
#define CF9             0x6600
#define CF10            0x6700
#define CF11            0x8900  /* Enhanced Keyboards Only      */
#define CF12            0x8a00  /* Enhanced Keyboards Only      */

#define AF1             0x6800  /* Alt - F1                     */
#define AF2             0x6900
#define AF3             0x6a00
#define AF4             0x6b00
#define AF5             0x6c00
#define AF6             0x6d00
#define AF7             0x6e00
#define AF8             0x6f00
#define AF9             0x7000
#define AF10            0x7100
#define AF11            0x8b00  /* Enhanced Keyboards Only      */
#define AF12            0x8c00  /* Enhanced Keyboards Only      */

#define ALTA            0x1e00  /* Alt - A ....                 */
#define ALTB            0x3000
#define ALTC            0x2e00
#define ALTD            0x2000
#define ALTE            0x1200
#define ALTF            0x2100
#define ALTG            0x2200
#define ALTH            0x2300
#define ALTI            0x1700
#define ALTJ            0x2400
#define ALTK            0x2500
#define ALTL            0x2600
#define ALTM            0x3200
#define ALTN            0x3100
#define ALTO            0x1800
#define ALTP            0x1900
#define ALTQ            0x1000
#define ALTR            0x1300
#define ALTS            0x1f00
#define ALTT            0x1400
#define ALTU            0x1600
#define ALTV            0x2f00
#define ALTW            0x1100
#define ALTX            0x2d00
#define ALTY            0x1500
#define ALTZ            0x2c00

#define ALT1            0x7800  /* Alt - 1 ....                 */
#define ALT2            0x7900
#define ALT3            0x7a00
#define ALT4            0x7b00
#define ALT5            0x7c00
#define ALT6            0x7d00
#define ALT7            0x7e00
#define ALT8            0x7f00
#define ALT9            0x8000
#define ALT0            0x8100

#define ALTEQUAL        0x8300  /* Alt =                        */
#define ALTMINUS        0x8200  /* Alt -                        */

#define CTRLCURLF       0x7300  /* Ctrl <-                      */
#define CTRLCURRT       0x7400  /* Ctrl ->                      */
#define CTRLEND         0x7500  /* Ctrl End                     */
#define CTRLHOME        0x7700  /* Ctrl Home                    */
#define CTRLPGDN        0x7600  /* Ctrl PgDn                    */
#define CTRLPGUP        0x8400  /* Ctrl PgUp                    */

#define CURDN           0x5000  /* down arrow                   */
#define CURLF           0x4b00  /* <-                           */
#define CENTER          0x4c00
#define CURRT           0x4d00  /* ->                           */
#define CURUP           0x4800  /* up arrow                     */
#if !defined( GCPP_WIN32 ) || !( defined( GCPP_WIN32 ) && defined( GCPP_X32 ) )
/*
 * Windows NT header files define this guy, let's make sure we don't clash.
 */
#  if !defined( DELETE )
#define DELETE          0x5300  /* Del                          */
#  endif
#endif
#define GCPP_DELETE     0x5300
#define ENDKEY          0x4f00  /* END key                      */
#define HOME            0x4700  /* HOME key                     */
#define INSERT          0x5200  /* Ins                          */
#define PGDN            0x5100  /* PgDn                         */
#define PGUP            0x4900  /* PgUp                         */
#define REVTAB          0x0F00  /* Shift Tab                    */

#define CTRLBREAK       0       /* for when breakchk(ON)        */
#define NOKEY           0xffff

/*
 * The following constants apply to the 101/102 key enhanced
 * keyboard.
 */
#define ALTACCENT       0x2900  /* Alt Accent  (`)              */
#define ALTAPOST        0x2800  /* Alt Apostrophe (')           */
#define ALTAST          0x3700  /* Alt *                        */
#define ALTBACKSLASH    0x2b00  /* Alt Backslash (\)            */
#define ALTBS           0x0e00  /* Alt-Backspace                */
#define ALTCB           0x1b00  /* Alt close bracket            */
#define ALTCOMMA        0x3300  /* Alt Comma                    */
#define ALTCURDN        0xa000  /* Alt Cursor Down              */
#define ALTCURLF        0x9b00  /* Alt Cursor Left              */
#define ALTCURRT        0x9d00  /* Alt Cursor Right             */
#define ALTCURUP        0x9800  /* Alt Cursor Up                */
#define ALTDELETE       0xa300  /* Alt Delete                   */
#define ALTEND          0x9f00  /* Alt End                      */
#define ALTENTER        0xa600  /* Alt Enter (keypad)           */
#define ALTESC          0x0100  /* Alt Escape                   */
#define ALTHOME         0x9700  /* Alt Home                     */
#define ALTINSERT       0xa200  /* Alt Insert                   */
#define ALTKBMINUS      0x4a00  /* Alt Keypad -                 */
#define ALTKBPLUS       0x4e00  /* Alt Keypad +                 */
#define ALTOB           0x1a00  /* Alt open bracket             */
#define ALTPERIOD       0x3400  /* Alt Period                   */
#define ALTPGDN         0xa100  /* Alt Page Down                */
#define ALTPGUP         0x9900  /* Alt Page Up                  */
#define ALTRETURN       0x1c00  /* Alt Return                   */
#define ALTSEMI         0x2700  /* Alt Semicolon                */
#define ALTSLASH        0x3500  /* Alt Slash                    */
#define ALTTAB          0xa500  /* Alt-Tab                      */
#define ALTXSLASH       0xa400  /* Alt Slash (keypad)           */

#define CTRLAST         0x9600  /* Control *                    */
#define CTRLCENTER      0x8f00  /* Control Center               */
#define CTRLCURDN       0x9100  /* Control Down Arrow           */
#define CTRLDELETE      0x9300  /* Control Delete               */
#define CTRLINSERT      0x9200  /* Control Insert               */
#define CTRLKPMINUS     0x8e00  /* Control Keypad -             */
#define CTRLKPPLUS      0x9000  /* Control Keypad +             */
#define CTRLPRTSC       0x7200  /* Ctrl PrtSc                   */
#define CTRLTAB         0x9400  /* Control Tab                  */
#define CTRLUP          0x8d00  /* Control UP-Arrow             */

/* End 101/102 key Enhanced keyboard constants */

/*
 * Returns (masks) from kbshift()
 */
#define ALTKEY          0x0008
#define CAPSKEY         0x0040
#define CTRLKEY         0x0004
#define INSKEY          0x0080
#define LFALTKEY        0x0200  /* Enhanced Keyboard Only       */
#define LFCTRLKEY       0x0100  /* Enhanced Keyboard Only       */
#define LFSHIFTKEY      0x0002
#ifndef NUMLOCK
#define NUMLOCK         0x0020
#endif
#define RTALTKEY        0x0800  /* Enhanced Keyboard Only       */
#define RTCTRLKEY       0x0400  /* Enhanced Keyboard Only       */
#define RTSHIFTKEY      0x0001
#define SCRL_LOCK       0x0010

/*
 *  The SCANB() macro can be used to transform the 16 bit "getkey()" code
 *  into an 8 bit scan code.
 */
#define SCANB(code)     (code>>8)

#ifdef __cplusplus
extern "C" {
#endif

GCPP_TYPE int GCPP_CFUN enhkbd(void);
GCPP_TYPE int GCPP_CFUN gfkbhit(void);
GCPP_TYPE int GCPP_CFUN breakchk(int option);
GCPP_TYPE unsigned int GCPP_CFUN getkey(void);
GCPP_TYPE unsigned GCPP_CFUN kbfetch(void);
GCPP_TYPE unsigned GCPP_CFUN kbshift(void);
#ifdef __cplusplus
}
#endif

#endif /* _IBMKEYS_DOT_H */
