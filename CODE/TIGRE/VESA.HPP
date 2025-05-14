//
// VESA.HPP
//
// June, 1994
// WATCOM: October 4, 1994  BKH
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
//----[]-------------------------------------------------------------

#ifndef	vesa_hpp
#define	vesa_hpp

#include "types.hpp"

#define	SCREEN_HEIGHT					480
#define	VIDEO_SEGMENT_ADDR			0xA000
#define	SCREEN_WIDTH					640
#define	SIZE_OF_VESA_INFO				256

#define	V_640x480_256					0x0101	// VESA: Screen Mode
#define 	V_INQ								0x4F00	// VESA: Inquire
#define	V_GET_MODE_INFO				0x4F01	// VESA: Get Mode Info
#define	V_SET_MODE						0x4F02	// VESA: Set Mode
#define	V_GET_MODE						0x4F03	// VESA: Get Mode
#define 	V_VID_MEM_SELECT				0x4F05	// VESA: Video Memory Select
#define 	V_GET_WIN_POSN					0x0100	// VESA: Get Window Position
#define 	V_SET_WIN_POSN					0x0000	// VESA: Set Window Position
#define	V_SUCCESS						0x004F	// VESA: Function Successful

#define	V7_640x480_256					0x0067	// Video 7: Screen Mode
#define	V7_INQ							0x6F00	// Video 7: Inquire
#define	V7_GET_MODE						0x6F04	// Video 7: Get Screen Mode
#define	V7_SET_MODE						0x6F05	// Video 7: Set Screen Mode
#define	V7_SUCCESS						0x5637	// Video 7: Function Successful

#define	SET_MODE							0x00;
#define	GET_MODE							0x0F;

struct bankInfo
{
	uint16	bankNumber;
	uint16	yOffset;
	uint16	transitionX;
	uint16	padTo8Bytes;
};

struct VESAInfo
{
	char		signature[4];
	char		version[2];
	char		*vendorData;
	char		*capabilities;
	uint16	*VESAMode;
	uint16	numberOf64kBlocks;
	char		padTo256Bytes[236];
};

struct VESAModeInfo
{
	uint16	modeAttributes;
	uchar		windowAAttributes;
	uchar		windowBAttributes;
	uint16	windowGranularity;
	uint16	windowSize;
	uint16	windowASegment;
	uint16	windowBSegment;
	void		*windowFunctionCall;
	uint16	BytesPerLine;
	uint16	horizontalRes;
	uint16	verticalRes;
	uchar		charCellWidth;
	uchar		charCellHeight;
	uchar		bitPlanes;
	uchar		bitsPerPixel;
	uchar		memoryBanks;
	uchar		memoryModel;
	uchar		kPerMemoryBank;
	uchar		displayPages;
	char		padTo256Bytes[226];
};


#endif
