/*
Copyright (C) Magna Carta Software, Inc. 1988-1994.  All Rights Reserved.
C COMMUNICATIONS TOOLKIT
Z80SIO.H -- C Communications Toolkit header file for Z80SIO/DART USART
*/


#if !defined(Z80SIO_H_INCLUDED)
    #define Z80SIO_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

//  #include <ccttype.h>


    /* INTERRUPT TYPES USED IN ENABLE COMMANDS */
    #define UZ80_ES                         0X1     /* write to WR1 */
    #define UZ80_TX_ALL                     0X2     /* write to WR1 */
    #define UZ80_RX_FIRST_CHARACTER         0X8     /* write to WR1 */
    #define UZ80_RX_ALL_SPECIAL_RECEIVE    0X10     /* write to WR1 */
    #define UZ80_RX_ALL                    0X18     /* write to WR1 */


    /* Z-80 SIO COMMANDS (issued to WR0) */
    #define UZ80_ES_RESET           0X10    /* external/status reset to WR0 */
    #define UZ80_CH_RESET           0X18    /* channel reset to WR0         */
    #define UZ80_TX_RESET           0X28    /* transmit int. pending reset  */
    #define UZ80_ER_RESET           0X30    /* error reset to WR0           */
    #define UZ80_RX_CRC_RESET       0X40    /* reset RX CRC checker         */
    #define UZ80_TX_CRC_RESET       0X80    /* reset TX CRC checker         */
    #define UZ80_TX_EOM_RESET       0XC0    /* reset TX underrun/EOM latch  */


    /* REGISTER NAMES */
    #define     WR0                    0    /* write register 0 of Z80SIO   */
    #define     WR1                    1    /* write register 1 of Z80SIO   */
    #define     WR2                    2    /* write register 2 of Z80SIO   */
    #define     WR3                    3    /* write register 3 of Z80SIO   */
    #define     WR4                    4    /* write register 4 of Z80SIO   */
    #define     WR5                    5    /* write register 5 of Z80SIO   */
    #define     WR6                    6    /* write register 6 of Z80SIO   */
    #define     WR7                    7    /* write register 7 of Z80SIO   */
    #define     RR0             (0X80 | 0)  /* read  register 0 of Z80SIO   */
    #define     RR1             (0X80 | 1)  /* read  register 1 of Z80SIO   */
    #define     RR2             (0X80 | 2)  /* read  register 2 of Z80SIO   */


    /* MASKS FOR SPECIFIC REGISTERS */
    #define     CHANNEL_A_ADDR        -1    /* address of channel A ctl.    */
    #define     CHANNEL_B_ADDR         0    /* address of channel B ctl.    */
    #define     UZ80_RX_MASK           1    /* UART receive character mask  */
    #define     UZ80_TX_MASK           4    /* UART transmit ready mask     */
    /* WRITE REGISTER 3 -- */
    #define     UZ80_RX_ENABLE          0x01    /* enable receive mask WR3      */
    #define     UZ80_SYNC_INHIBIT       0X02    /* inhibit load of RX SYNC      */
    #define     UZ80_RX_CRC_ENABLE      0X08    /* enable RX CRC                */
    #define     UZ80_HUNT_ENTER         0X10    /* enter hunt phase bit         */
    #define     UZ80_AUTO_ENABLE        0X20    /* auto enable transmit */
    #define     UZ80_RX_DATABITS_MASK   0XC0    /* wr3 = 11000000               */
    /* WRITE REGISTER 4 -- */
    #define     UZ80_PARITY_MASK        0X03    /* wr4 = 00000011               */
    #define     UZ80_STOPBITS_MASK      0X0C    /* wr4 = 00001100               */
    #define     UZ80_SYNC_MASK          0X30    /* Z80 reference clock speed WR4*/
    #define     UZ80_CLOCK_MASK         0XC0    /* Z80 reference clock speed WR4*/
    /* WRITE REGISTER 5 -- */
    #define     UZ80_TX_CRC_ENABLE      0X01    /* wr5 = 00000001               */
    #define     UZ80_RTS_MASK           0X02    /* wr5 = 00000010               */
    #define     UZ80_CRC16_MASK         0X04    /* wr5 = 00000010               */
    #define     UZ80_TX_ENABLE          0X08    /* enable transmit mask WR5     */
    #define     UZ80_BREAK_MASK         0X10    /* wr5 = 00010000               */
    #define     UZ80_TX_DATABITS_MASK   0X60    /* wr5 = 01100000               */
    #define     UZ80_DTR_MASK           0X80    /* wr5 = 10000000               */

    /* READ REGISTERS */
    #define     UZ80_RI_MASK            0X10    /* modem status  reg. 01000000  */
    #define     UZ80_CTS_MASK           0X20    /* DCD mask 00100000            */
    #define     UZ80_DCD_MASK           0X08    /* DCD mask 00001000            */

    /* REFERENCE CLOCK SPEEDS */
    #define     X1                      0       /* Z80 x1  reference clock      */
    #define     X16                     0X40    /* Z80 x16 reference clock      */
    #define     X32                     0X80    /* Z80 x32 reference clock      */
    #define     X64                     0XC0    /* Z80 x64 reference clock      */


    short               uz80_get_rr_(COMM_PORT *p, WORD reg);
    short               uz80_get_wr_(COMM_PORT *p, short reg);
    COMM_PORT * EXPORT_ uz80_init(WORD cha_addr, WORD addr, WORD clock_reg_addr,
        WORD ref_clock_speed, long speed, WORD databits, WORD parity, WORD stopbits);
    void                uz80_set_clock(COMM_PORT *p_port, WORD speed);
    void                uz80_set_reg_(COMM_PORT *p_port, short reg, short value);

    #if defined(__cplusplus)
        }
    #endif
#endif
