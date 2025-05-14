//
// RESINFO.CPP
//
// May 6, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
//	Create a resource file.
//
//----[]-------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.hpp"
#include "tigre.hpp"
#include "writeres.hpp"


//----[Function Prototypes]------------------------------------------

void	ShowUsage();


//----[Function Main]------------------------------------------------

int
main(int argc, char* argv[])
{
	int				ret;
	FILE*				fRes;
	ResHeader		resHdr;
	Configuration	config;
	uint16			resType;
	char*				p;
	char*				endString;

	if (argc < 2)
	{
		ShowUsage();
		return TRUE;
	}

	// Load resource config file
	config.startLine = 2;
	config.endLine = (uint16) (RES_LAST + config.startLine);
	config.numParams = CFG_PARMS_MAX;
	if (!config.Load("res_cfg.hpp"))
	{
		printf("Can't open RES_CFG.HPP\n");
		return FALSE;
	}

	// Open the resource file
	ret = TRUE;
	fRes = fopen(argv[1], "rb");
	if (fRes)
	{
		// Find the file extension and use it to locate the config record.
		// move to the end of the string
		p = endString = argv[1] + strlen(argv[1]) - 1;
		// back up to the period
		while((*p != '.') && (p >= argv[1]))
		{
			p--;
		}

		if (p < argv[1] || 
			 p == endString)
		{
			// no extension was found
			printf("No file extension given.\n");
			return FALSE;
		}

		// move past the period
		p++;
		resType = config.FindSubject(p, 2);

		// Now see if the config says this type is headerless.  If so,
		// we're outta here.
		if (config.HasParam(resType, "H"))
		{
			printf("Resource is headerless.\n");
			return FALSE;
		}
		else
		{
			fread(&resHdr, rhSize, 1, fRes);
			printf("Compression:   ");
			if (resHdr.compType & CMP_TRLE)
			{
				printf("TRLE");
			}
			if (resHdr.compType & CMP_T12)
			{
				if (resHdr.compType != CMP_T12)
				{
					printf(", ");
				}
				printf("T12");
			}
			if (!resHdr.compType)
			{
				printf("None");
			}
			printf("\n");

			printf("Uncomp. Size:  %d (uncompressed)\n", resHdr.uSize);
			printf("Comp. Size:    %d\n", resHdr.size);
			printf("Resource type: ");
			switch (resHdr.type)
			{
				case RES_CEL:
					printf("Cel\n");
					break;

				case RES_ANIM:
					printf("Animation\n");
					break;

				case RES_PIC:
					printf("Picture\n");
					break;

				case RES_MIDI:
					printf("MIDI sound\n");
					break;

				case RES_DAC:
					printf("Digital sample\n");
					break;

				case RES_PAL:
					printf("Palette\n");
					break;

				case RES_DATA:
					printf("Raw data\n");
					break;

				case RES_FONT:
					printf("Font\n");
					break;

				case RES_SQUIB:
					printf("Squibble\n");
					break;

				case RES_CONV:
					printf("Conversation\n");
					break;

				case RES_CHOREO:
					printf("Choreograph\n");
					break;

				case RES_BNK:
					printf("Sound patch bank\n");
					break;

				default:
					printf("Unknown!\n");
					break;
			}
		}

		printf("# Headers:     %d\n", resHdr.numHeaders);
		printf("Header data:   %d (0x%x)  %d (0x%x)  %d (0x%x)  %d (0x%x)\n",
			resHdr.data[0], resHdr.data[0], resHdr.data[1], resHdr.data[1],
			resHdr.data[2], resHdr.data[2], resHdr.data[3], resHdr.data[3]);

		if (resHdr.endword == 0xfefe)
		{
			printf("Header check word OK\n");
		}
		else
		{
			printf("Header check word invalid!\n");
		}

		printf("\n");
	}
	else
	{
		printf("Error opening file: %s\n", argv[1]);
		ret = FALSE;
	}
	return ret;
}


void
ShowUsage()
{
	printf("Usage:   RESINFO filename\n");
	printf("Example: RESINFO 1500.ani\n");
}
