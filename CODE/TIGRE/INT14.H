/*
Copyright (C) Magna Carta Software, Inc. 1992-1994. All Rights Reserved.
C COMMUNICATIONS TOOLKIT
INT14DEF.H -- C Communications Toolkit header file for NETWORK INT14h interface.
*/

#if !defined(INT14_H_INCLUDED)
    #define INT14_H_INCLUDED

    hPORT EXPORT_ int14_init(WORD itype, WORD portnum, long speed, short databits, short parity, short stopbits);
    BYTE  EXPORT_ is_digiboard(BYTE channel);
    BYTE  EXPORT_ is_fossil(void);
    BYTE  EXPORT_ is_netdev(void);
    BYTE  EXPORT_ is_netport(void);

    #define is_FOSSIL() is_fossil

    extern short int14_errno;
#endif
