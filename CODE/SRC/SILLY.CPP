// SILLY.CPP
// Accepts a path (ended with \) and writes to a file, for return to the Autoplay'er
// so it can insert info into the Win95 registery

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
main(int argc, char *argv[])
{
	FILE 	*pFile;

	if(argc != 2)
	{
		exit(0);
	}

	pFile = fopen("C:\\BAM1TXT.TXT", "wb");
	if(!pFile)
		exit(0);

	fwrite(argv[1], strlen(argv[1]) + 1, 1, pFile);
	fclose(pFile);
}
