// This is the stupidest program in the world.
// -Van

#include <stdio.h>
#include <mem.h>
#include <conio.h>
#include <ctype.h>
#include "dpmi.hpp"
#include "install.h"

// pick one of the following
#define INSTALL_DOS
//#define INSTALL_WIN95

void
main(void)
{
	DPMI	dpmi;
	uchar	*pVidMem;
	char	key, fileName[20];
	
	fileName[0] = 0;

	// throw up the screen
	pVidMem = (uchar *) dpmi.RealToProtected(0xB8000000);
	memcpy(pVidMem, LANGSCRN, sizeof(LANGSCRN));

	while(!fileName[0])
	{
		key = (char)toupper(getch());
		switch(key)
		{	
			case 'E':
				#ifdef INSTALL_DOS
				strcpy(fileName, "INSTENGL");
				#else
				strcpy(fileName, "INSTW95E");
				#endif
				break;

			case 'F':
				#ifdef INSTALL_DOS
				strcpy(fileName, "INSTFREN");
				#else
				strcpy(fileName, "INSTW95F");
				#endif
				break;

			case 'D':
				#ifdef INSTALL_DOS
				strcpy(fileName, "INSTDEUT");
				#else
				strcpy(fileName, "INSTW95D");
				#endif
				break;
		}
	}
	system(fileName);
}


