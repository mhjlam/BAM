//*
//* _8250.HPP
//*
//* This private header file has all of the 8250 UART register names
//* and bit field definitions.	This file is generally only needed to
//* rebuild the library.
//*
//*  The Greenleaf Comm++ Library
//*
//*  Copyright (C) 1991-95 Greenleaf Software Inc.	All Rights Reserved.
//*
//* NOTES
//*
//* This private header file has all of the 8250 UART register names
//* and bit field definitions.	This file is generally only needed to
//* rebuild the library, not for inclusion in an end user module.  Note
//* that assembly language modules can get this stuff from _8250.EQU.
//*
//* MODIFICATIONS
//*
//*
//*

#ifndef __8250_DOT_HPP
#define __8250_DOT_HPP

#define TRANSMIT_HOLDING_REGISTER          0
#define RECEIVE_BUFFER_REGISTER            0
#define DIVISOR_LATCH_LSB                  0
#define DIVISOR_LATCH_MSB                  1
#define INTERRUPT_ENABLE_REGISTER          1
#define   IER_RECEIVE_DATA_INTERRUPT       0x01
#define   IER_TRANSMIT_DATA_INTERRUPT      0x02
#define   IER_LINE_STATUS_INTERRUPT        0x04
#define   IER_MODEM_STATUS_INTERRUPT       0x08
#define INTERRUPT_ID_REGISTER              2
#define   IID_FIFO_ENABLED_MASK            0xc0
#define FIFO_CONTROL_REGISTER              2
#define   FCR_FIFO_ENABLE                  0x01
#define   FCR_RCVR_FIFO_RESET              0x02
#define   FCR_XMIT_FIFO_RESET              0x04
#define   FCR_DMA_MODE_SELECT              0x08
#define   FCR_RCVR_TRIGGER_LSB             0x40
#define   FCR_RCVR_TRIGGER_MSB             0x80
#define LINE_CONTROL_REGISTER              3
#define   LCR_WORD_LENGTH_SELECT_BITS      0x03
#define     LCR_WORD_LENGTH_SELECT_BIT_0   0x01
#define     LCR_WORD_LENGTH_SELECT_BIT_1   0x02
#define       LCR_WORD_LENGTH_5            0x00
#define       LCR_WORD_LENGTH_6            0x01
#define       LCR_WORD_LENGTH_7            0x02
#define       LCR_WORD_LENGTH_8            0x03
#define   LCR_NUMBER_OF_STOP_BITS          0x04
#define   LCR_PARITY_BITS                  0x38
#define     LCR_PARITY_ENABLE              0x08
#define     LCR_EVEN_PARITY_SELECT         0x10
#define     LCR_STICK_PARITY               0x20
#define       LCR_PARITY_N                 0x00
#define       LCR_PARITY_O                 0x08
#define       LCR_PARITY_E                 0x18
#define       LCR_PARITY_S                 0x38
#define       LCR_PARITY_M                 0x28
#define   LCR_SET_BREAK                    0x40
#define   LCR_DIVISOR_LATCH_ACCESS         0x80
#define MODEM_CONTROL_REGISTER             4
#define   MCR_DATA_TERMINAL_READY          0x01
#define   MCR_REQUEST_TO_SEND              0x02
#define   MCR_OUT1                         0x04
#define   MCR_OUT2                         0x08
#define   MCR_LOOPBACK                     0x10
#define LINE_STATUS_REGISTER               5
#define   LSR_DATA_READY                   0x01
#define   LSR_OVERRUN_ERROR                0x02
#define   LSR_PARITY_ERROR                 0x04
#define   LSR_FRAMING_ERROR                0x08
#define   LSR_BREAK_INTERRUPT              0x10
#define   LSR_THRE                         0x20
#define   LSR_TEMT                         0x40
#define   LSR_FIFO_ERROR                   0x80
#define MODEM_STATUS_REGISTER              6
#define   MSR_DELTA_CTS                    0x01
#define   MSR_DELTA_DSR                    0x02
#define   MSR_TRAILING_EDGE_RI             0x04
#define   MSR_DELTA_CD                     0x08
#define   MSR_CTS                          0x10
#define   MSR_DSR                          0x20
#define   MSR_RI                           0x40
#define   MSR_CD                           0x80
#define SCRATCH_REGISTER                   7

#endif

