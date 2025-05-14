/*
Copyright (C) Magna Carta Software, Inc. 1992-1993.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
CCTBP_.H -- CompuServe B-Protocol suite internal declarations.
*/

#if !defined(CCTBP_H_INCLUDED)
    #define CCTBP_H_INCLUDED

    #define BP_HOST_WR          0X3000
    #define BP_HOST_WS          0X3001
    #define BP_HOST_BS          0X3002
    #define BP_HOST_CM          0X3003
    #define BP_HOST_DQ          0X3004
    #define BP_HOST_TL          0X3005
    #define BP_HOST_Q1          0X3006
    #define BP_HOST_Q2          0X3007
    #define BP_HOST_Q3          0X3008
    #define BP_HOST_Q4          0X3009
    #define BP_HOST_Q5          0X300A
    #define BP_HOST_Q6          0X300B
    #define BP_HOST_Q7          0X300C
    #define BP_HOST_Q8          0X300D
    #define BP_HOST_DR          0X300E
    #define BP_HOST_UR          0X300F
    #define BP_HOST_FI          0X3010

    #define BP_NEG_WR           0X3020
    #define BP_NEG_WS           0X3021
    #define BP_NEG_BS           0X3022
    #define BP_NEG_CM           0X3023
    #define BP_NEG_DQ           0X3024
    #define BP_NEG_TL           0X3025
    #define BP_NEG_Q1           0X3026
    #define BP_NEG_Q2           0X3027
    #define BP_NEG_Q3           0X3028
    #define BP_NEG_Q4           0X3029
    #define BP_NEG_Q5           0X302A
    #define BP_NEG_Q6           0X302B
    #define BP_NEG_Q7           0X302C
    #define BP_NEG_Q8           0X302D
    #define BP_NEG_DR           0X302E
    #define BP_NEG_UR           0X302F
    #define BP_NEG_FI           0X3030

    #define BP_FI_DATA_TYPE     0X3040
    #define BP_FI_COMPRESSION   0X3041
    #define BP_FI_ZONE          0X3042
    #define BP_FI_CDATE         0X3043
    #define BP_FI_CTIME         0X3044
    #define BP_FI_MDATE         0X3045
    #define BP_FI_MTIME         0X3046
    #define BP_FI_TRUE_NAME     0X3047

    #define CCT_USING_B         0X4050
    #define CCT_USING_QB        0X4051
    #define CCT_USING_BPLUS     0X4052
    #define BP_RX_PKT_TYPE      0X3053
    #define BP_TX_PKT_TYPE      0X3054

    long  EXPORT_ b_get(hPORT p, WORD option);
    short EXPORT_ b_set(hPORT p, WORD option, unsigned long val);
#endif
