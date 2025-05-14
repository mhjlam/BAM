/*
Copyright (C) Magna Carta Software, Inc. 1988-1995.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
CCTIO_.H -- File I/O routines for system independence.
*/

#if !defined(CCTIO__H_INCLUDED)
    #define CCTIO__H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    #include <stddef.h>
    #include <sys\types.h>
    #include <sys\stat.h>

    #if !defined(FHANDLE_DEFINED)
        #define FHANDLE_DEFINED
        #if defined(USE_STREAM_IO)
            typedef FILE * FHANDLE;
        #else
            typedef long FHANDLE;
        #endif
    #endif
    /* FILE I/O FUNCTIONS */
    short       EXPORT_     cct_file_close_(FHANDLE fh);
    short       EXPORT_     cct_file_delete_(char DLL_FAR_ *fname);
    FHANDLE     EXPORT_     cct_file_open_(const char DLL_FAR_ *fspec, int fmode);
    int         EXPORT_     cct_file_read_(FHANDLE fh, const void DLL_FAR_ *buf, size_t count);
    int         EXPORT_     cct_file_readch_(FHANDLE fh);
    long        EXPORT_     cct_file_seek_(FHANDLE fh, long offset, int where);
    short       EXPORT_     cct_file_stat_(const char DLL_FAR_ *path, struct stat DLL_FAR_ *s);
    int         EXPORT_     cct_file_write_(FHANDLE fh, const void DLL_FAR_ *buf, size_t count);

    #if defined(__cplusplus)
        }
    #endif
#endif
