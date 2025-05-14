#include <stdio.h>
#include <conio.h>
#include <stdarg.h>
#include <ctype.h>
#include <mem.h>
#include "dpmi.hpp"
#include "last1e.h"
#include "last1f.h"
#include "last1d.h"
#include "last2e.h"

// note: TIGRE\DPMI.OBJ must be present

const screen_size = 4000;

char screenBkp[screen_size];

void main(int argc, char *argv[])
{
	DPMI dpmi;
	char *pScreen = (char *)dpmi.RealToProtected(0xB8000000), *pSrc;

	memcpy(screenBkp, pScreen, screen_size);

	if(argc < 2)
		pSrc = (char *)MSG_ENGL;
	else switch(toupper(*argv[1]))
	{
		case '2':
			pSrc = (char *)MSG_DEMO_ENGL;
			break;

		case 'D':
			pSrc = (char *)MSG_GERM;
			break;

		case 'F':
			pSrc = (char *)MSG_FREN;
			break;

		case 'E':
		default:
			pSrc = (char *)MSG_ENGL;
			break;
	}

	memcpy(pScreen, pSrc, screen_size);
	while(!kbhit());
	getch();
	memcpy(pScreen, screenBkp, screen_size);
//	memset(pScreen, 0, screen_size);
}
