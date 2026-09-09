/*
Copyright (C) Magna Carta Software, Inc. 1990-1993. All Rights Reserved.
C COMMUNICATIONS TOOLKIT
TV.H -- Include file for TELEVIDEO terminal emulation.
*/

#if !defined(TV_H_INCLUDED)
    #define TV_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if !defined(TERMINAL_H_INCLUDED)
        #include <termina_.h>
    #endif

    /* MODE COMMAND FLAG (BIT ENCODED) */
    #define ISPROTECTED     1
    #define INPROTECTMODE   2

    /*
    t->mode is encoded as:
    Bit 0:  1=protect mode set;
    Bit 1:  1=in protect mode;
    */
    #define isprotected(a)          ((a)->mode & ISPROTECTED)   /* character protected */
    #define inprotectmode(a)        ((a)->mode & INPROTECTMODE) /* in protect mode */


    short           tv_con_put_(hTERMINAL t, WORD row, WORD col, WORD ch, WORD att);
    short WPASCAL_  tv_con_putc_(TERMINAL DLL_FAR_ *t, WORD ch, WORD att);
    void            tv_cpr_(hTERMINAL t);
	void  WPASCAL_  tv_cur_att_(TERMINAL DLL_FAR_ *t, WORD cmd);
    short           tv_cur_pos_(hTERMINAL t, short cmd);
    short           tv_dispatch_ctrl_(hTERMINAL t, WORD ch);
    short           tv_editcmd_(hTERMINAL t, short cmd);
    void            tv_htc_(hTERMINAL t);
    void            tv_hts_(hTERMINAL t);
    short           tv_key_click_(hTERMINAL t, short state);
    short           tv_parse_cmd_(hTERMINAL t, const char *n);
    void            tv_reset_mode_(hTERMINAL t, short cmd);
    void            tv_rpm_(hTERMINAL t);
    void            tv_set_mode_(hTERMINAL t, short cmd);
    void            tv_sgr_(hTERMINAL t);
    void            tv_spm_(hTERMINAL t);
    void            tv_tbc_(hTERMINAL t);
    short           tv_update_attbuf_(hTERMINAL t, short row, short col, short att);
    short           tv_update_scr_(hTERMINAL t, WORD col, WORD row);


    /* EXPORTED FUNCTIONS */
    void    EXPORT_ tv925_init(hTERMINAL t);


    #if defined(__cplusplus)
        }
    #endif


#endif
