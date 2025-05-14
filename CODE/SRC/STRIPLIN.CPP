#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

void main(int argc, char *argv[])
{
	FILE *infile = fopen(argv[1], "r"), *outfile;
	char	string1[255];

	if(!infile)
	{
		printf("Error: can't open \"%s\"\n", argv[1]);
		return;
	}
	fgets(string1, 250, infile);	// swallow first line of GREP output ("File ...")
	outfile = fopen("striplin.tmp", "w");
	fgets(string1, 250, infile);
	while(!feof(infile))
	{
		fputs(string1, outfile);
		fgets(string1, 250, infile);
	}
	fclose(infile);
	fclose(outfile);
	sprintf(string1, "copy striplin.tmp %s", argv[1]);
	system(string1);
	system("del striplin.tmp");
}
