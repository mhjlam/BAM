// MAKESERL
// Tool to make encrypted signature files for BAM pre-release distribution.
// Begun by: Vangelis 11/30/95
// Copyright (C) 1995 Tachyon Studios Inc.

#define KEY_MAXLINES			5
#define KEY_MAXLINELENGTH	128
#define CAMO_LENGTH			32

#include "stdlib.h"
#include "stdio.h"
#include "mem.h"
#include "ctype.h"
#include "time.h"
#include <sys\utime.h>

char	camoflage[KEY_MAXLINES][CAMO_LENGTH] =
{"SoundBlaster (16, PRO, AWE32)", "Adlib", "General MIDI", "Gravis Ultrasound", "Pro Audio Spectrum 16"};
char	key[KEY_MAXLINES][KEY_MAXLINELENGTH], *pEncKey;
unsigned long lineChecks[KEY_MAXLINES], lineCheck1, lineCheck2;

void	encrypt(void), decrypt(void);

void
main(int argc, char *argv[])
{
	FILE	*pFile;

	memset(key, NULL, sizeof(key));
	memset(lineChecks, NULL, sizeof(lineChecks));

	printf("MAKESERL compiled %s %s\n", __DATE__, __TIME__);
	printf("to encrypt: C> MAKESERL\n");
	printf("to decrypt: C> MAKESERL D\n");

	pFile = fopen("MAKESERL.EXE", "r");
	if(!pFile)
	{
		printf("ERROR: Unable to open MAKESERL.EXE for read operation (extraction of encryption key)\n");
		exit(1);
	}
	if(argc > 1 && toupper(*argv[1]) == 'D')
		decrypt();
	else encrypt();

	exit(0);
}

void
encrypt(void)
{
	int	loop1, loop2;
	FILE	*pFile;
	time_t		keyTime;
	struct tm	tm_keyTime;
	struct utimbuf utime1;

	printf("Enter %d lines of unique descriptive for this unique key\n", KEY_MAXLINES);
	for(loop1 = 0; loop1 < KEY_MAXLINES; loop1++)
	{
		printf("%d] ", loop1 + 1);
		gets(key[loop1]);
		if(!key[loop1][0])
			break;
	}
	if(!key[0][0])
	{
		printf("No data entered, aborting.\n");
		exit(0);
	}

	// encrypt source data, using .EXE data stream for encryption key (evil, huh?)
	printf("Encrypting your text..\n");

	for(loop1 = 0; loop1 < KEY_MAXLINES; loop1++)
	{
		pEncKey = camoflage[loop1];
		for(loop2 = 0, lineCheck1 = 0; loop2 < KEY_MAXLINELENGTH; loop2++, pEncKey++)
		{
			key[loop1][loop2] = (char)((int)key[loop1][loop2] ^ ((int)pEncKey ^ ((loop2 * loop2 + loop1 * 13 + loop2) % 256)));
			lineCheck1 = lineCheck1 + key[loop1][loop2];
			if(!(*pEncKey))
				pEncKey = &camoflage[loop1][0];
		}
		lineChecks[loop1] = lineCheck1 ^ 0xF61E22C9;
	}

	printf("Generating key file..\n");
	pFile = fopen("HMICARDS.386", "wb");
	for(loop1 = 0; loop1 < KEY_MAXLINES; loop1++)
	{
		fwrite(camoflage[loop1], CAMO_LENGTH, 1, pFile);
		fwrite(key[loop1], KEY_MAXLINELENGTH, 1, pFile);
		fwrite(&lineChecks[loop1], sizeof(lineChecks[loop1]), 1, pFile);
	}

	// generate some filler just to make file size less suspicious
	for(loop1 = 0; loop1 < 24000; loop1++)
		fputc(rand() % 256, pFile);

	fclose(pFile);
	tm_keyTime.tm_sec = 0;
	tm_keyTime.tm_min = 0;
	tm_keyTime.tm_hour = 3;
	tm_keyTime.tm_mday = 6;
	tm_keyTime.tm_mon = 1;
	tm_keyTime.tm_year = 95;
	tm_keyTime.tm_wday = 0;
	tm_keyTime.tm_yday = 0;
	tm_keyTime.tm_isdst = 0;
	keyTime = mktime(&tm_keyTime);
	utime1.actime = keyTime;
	utime1.modtime = keyTime;
	if(utime("HMICARDS.386", &utime1))
	{
		printf("utime() error %d\n", errno);
	}
	printf("Done\n");	
}

void
decrypt(void)
{
	int	loop1, loop2;
	FILE	*pFile;

	printf("Reading key file..\n");
	pFile = fopen("HMICARDS.386", "rb");
	if(!pFile)
	{
		printf("Error: HMICARDS.386 not present\n");
		return;
	}

	for(loop1 = 0; loop1 < KEY_MAXLINES; loop1++)
	{
		fread(camoflage[loop1], CAMO_LENGTH, 1, pFile);
		fread(key[loop1], KEY_MAXLINELENGTH, 1, pFile);
		fread(&lineChecks[loop1], sizeof(lineChecks[loop1]), 1, pFile);
	}
	fclose(pFile);

	// encrypt source data, using .EXE data stream for encryption key (evil, huh?)
	printf("Decrypting text..\n");

	for(loop1 = 0; loop1 < KEY_MAXLINES; loop1++)
	{
		pEncKey = camoflage[loop1];
		for(loop2 = 0, lineCheck1 = 0; loop2 < KEY_MAXLINELENGTH; loop2++, pEncKey++)
		{
			lineCheck1 = lineCheck1 + key[loop1][loop2];
			key[loop1][loop2] = (char)((int)key[loop1][loop2] ^ ((int)pEncKey ^ ((loop2 * loop2 + loop1 * 13 + loop2) % 256)));
			if(!(*pEncKey))
				pEncKey = camoflage[loop1];
		}
		lineCheck2 = lineChecks[loop1];
		lineCheck2 ^= 0xF61E22C9;
		if(lineCheck2 != lineCheck1)
			printf("\nLine %d checksum error\n", loop1);
	}

	printf("Displaying decrypted contents of signature file:\n");
	for(loop1 = 0; loop1 < KEY_MAXLINES; loop1++)
	{
		printf("%d] %s\n", loop1 + 1, key[loop1]);
	}
}
