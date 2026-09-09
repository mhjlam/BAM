//
// KEYBRD.HPP
//
// November 22, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Keyboard definitions.
//
//----[]-------------------------------------------------------------


#ifndef keybrd_hpp
#define keybrd_hpp


// key code (as recieved from port 0x60)

// arrow keys
const unsigned char K_UP				= 72;
const unsigned char K_DOWN				= 80;
const unsigned char K_RIGHT			= 77;
const unsigned char K_LEFT				= 75;

// modifier keys
const unsigned char K_RIGHT_SHIFT	= 54;
const unsigned char K_LEFT_SHIFT		= 42;
const unsigned char K_ALT				= 56;
const unsigned char K_CTRL				= 29;
const unsigned char K_CAPS				= 58;

// other keys
const unsigned char K_TAB				= 15;
const unsigned char K_ENTER			= 28;
const unsigned char K_RETURN			= K_ENTER;

const unsigned char K_DEL	 			= 83;
const unsigned char K_BREAK 			= 69;

const unsigned char K_INSERT 			= 82;
const unsigned char K_BACK_SPACE		= 14;
const unsigned char K_SPACE 			= 57;
const unsigned char K_PAGE_UP			= 73;
const unsigned char K_PAGE_DOWN		= 81;

const unsigned char K_HOME 			= 71;
const unsigned char K_END	 			= 79;
const unsigned char K_ESC	 			= 1;

const unsigned char K_LEFT_BRACKET	= 26;
const unsigned char K_RIGHT_BRACKET	= 27;
const unsigned char K_SEMICOLON		= 39;
const unsigned char K_QUOTE 			= 40;
const unsigned char K_GRAVE 			= 41;
const unsigned char K_COMMA 			= 51;
const unsigned char K_PERIOD 			= 52;
const unsigned char K_SLASH 			= 53;
const unsigned char K_BACKSLASH		= 43;

const unsigned char K_PRINT_SCREEN	= 55;

// alpha keys
const unsigned char K_A				= 30;
const unsigned char K_B				= 48;
const unsigned char K_C				= 46;
const unsigned char K_D				= 32;
const unsigned char K_E				= 18;
const unsigned char K_F				= 33;
const unsigned char K_G				= 34;
const unsigned char K_H				= 35;
const unsigned char K_I				= 23;
const unsigned char K_J				= 36;
const unsigned char K_K				= 37;
const unsigned char K_L				= 38;
const unsigned char K_M				= 50;
const unsigned char K_N				= 49;
const unsigned char K_O				= 24;
const unsigned char K_P				= 25;
const unsigned char K_Q				= 16;
const unsigned char K_R				= 19;
const unsigned char K_S				= 31;
const unsigned char K_T				= 20;
const unsigned char K_U				= 22;
const unsigned char K_V				= 47;
const unsigned char K_W				= 17;
const unsigned char K_X				= 45;
const unsigned char K_Y				= 21;
const unsigned char K_Z				= 44;

// numeric keys
const unsigned char K_1 			= 2;
const unsigned char K_2 			= 3;
const unsigned char K_3 			= 4;
const unsigned char K_4 			= 5;
const unsigned char K_5 			= 6;
const unsigned char K_6 			= 7;
const unsigned char K_7 			= 8;
const unsigned char K_8 			= 9;
const unsigned char K_9 			= 10;
const unsigned char K_0 			= 11;

const unsigned char K_PAD_1		= 79;
const unsigned char K_PAD_2		= 80;
const unsigned char K_PAD_3		= 81;
const unsigned char K_PAD_4		= 75;
const unsigned char K_PAD_5		= 76;
const unsigned char K_PAD_6		= 77;
const unsigned char K_PAD_7		= 71;
const unsigned char K_PAD_8		= 72;
const unsigned char K_PAD_9		= 73;
const unsigned char K_PAD_0		= 82;

const	unsigned	char K_PLUS			= 74;
const	unsigned	char K_MINUS		= 78;

// function keys
const unsigned char K_F1 			= 59;
const unsigned char K_F2 			= 60;
const unsigned char K_F3 			= 61;
const unsigned char K_F4 			= 62;
const unsigned char K_F5 			= 63;
const unsigned char K_F6 			= 64;
const unsigned char K_F7 			= 65;
const unsigned char K_F8 			= 66;
const unsigned char K_F9 			= 67;
const unsigned char K_F10 			= 68;
const unsigned char K_F11 			= 87;
const unsigned char K_F12 			= 88;

// special keys
// NOTE: These are not real scan codes.
//       Since these keys require multiple keys,
//       these defines are merely for our convenience.

const unsigned char K_PAUSE		= 127;

#endif
