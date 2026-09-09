/*
Copyright (C) Magna Carta Software, Inc. 1991-1995.  All Rights Reserved.
NASI.H -- CCT Support for the Network Communications Services Interface (NASI).
*/


#if !defined(NASI_H_INCLUDED)
    #define NASI_H_INCLUDED

    #if !defined(COMM_H_INCLUDED)
        #include <comm.h>
    #endif

    short EXPORT_ is_nasi(void);
    short EXPORT_ nasi_findfirst_service(char DLL_FAR_ *circuit, char DLL_FAR_ *server, char DLL_FAR_ *gen, char DLL_FAR_ *spec, WORD DLL_FAR_ *protocol, WORD DLL_FAR_ *status);
    short EXPORT_ nasi_findnext_service(char circuit, char DLL_FAR_ *server, char DLL_FAR_ *gen, char DLL_FAR_ *spec, WORD DLL_FAR_ *protocol, WORD DLL_FAR_ *status);
    hPORT EXPORT_ nasi_init(const char DLL_FAR_ *sname, long speed, short databits, short parity, short stopbits);
    short         nasi_set_autowrite(COMM_PORT *p, short state, WORD count);
    #if (defined(CCTW) || defined(MCE))
        short EXPORT_ nasi_pm_deinit(void);
    #else
        #define nasi_pm_deinit()
    #endif

    extern short nasi_errno;
#endif
