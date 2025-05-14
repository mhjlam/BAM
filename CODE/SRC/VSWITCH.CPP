// VSWITCH.CPP
// Replacement for defective SWITCH.EXE used in BAM Installer
//	for replacing the STFS drive letter in RES_CFG.HPP at install-time.
// Begun 12/09/95 (at 6-ph*ckin-am!)
// Copyright 1995 (c) Tachyon Studios Inc

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <mem.h>

#define MAX_LINES	100
#define MAX_LINE_LEN	256

char buffer[MAX_LINES][MAX_LINE_LEN];

void
main(int argc, char *argv[])
{
	FILE	*pFile;
	int	lineCount, loop1, length;
	char	driveLetter;

	if(argc != 2)
	{
		printf("Quit being nosey.\n");
		exit(1);
	}

	driveLetter = toupper(*argv[1]);

	memset(buffer, NULL, sizeof(buffer));
	pFile = fopen("res_cfg.hpp", "r");
	if(!pFile)
	{
		printf("VSWITCH: can't read from .\\res_cfg.hpp!\n");
		exit(1);
	}
	lineCount = 0;
	while(fgets(buffer[lineCount++], MAX_LINE_LEN - 1, pFile));
	fclose(pFile);

	// replace all instances of drive letters with specified replacement letter
	for(loop1 = 0; loop1 < lineCount; loop1++)
	{
		for(length = strlen(buffer[loop1]) - 1; length > 0; length--)
			if(buffer[loop1][length] == ':' && isalpha(buffer[loop1][length - 1]) &&
			(buffer[loop1][length - 2] == ' ' || buffer[loop1][length - 2] == ';' || buffer[loop1][length - 2] == '\t'))
				buffer[loop1][length - 1] = driveLetter;
	}

	pFile = fopen("res_cfg.hpp", "w");
	if(!pFile)
	{
		printf("VSWITCH: Error, can't write to .\\res_cfg.hpp!\n");
		exit(1);
	}

	for(loop1 = 0; loop1 < lineCount; loop1++)
		fprintf(pFile, "%s", buffer[loop1]);
	fclose(pFile);
}
