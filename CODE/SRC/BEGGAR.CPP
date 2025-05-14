#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>

FILE	*fptr;
char	fortune[256], asterisk[2] = "*";

int	FileCountLines(FILE *pFile);
char	*FileGetLine(FILE *pFile, int lineNum);

int main(void)
{
	int	desired, total;

	srand(time(NULL) % RAND_MAX);

	if((fptr=fopen("C:\\AUTOEXEC.BAT", "r")) == NULL)
	{
		printf("Error Opening File");
		return(1);
	}

	total = FileCountLines(fptr);

	desired = rand() % total;

	strcpy(fortune, FileGetLine(fptr, desired));

	printf("%d of %d] %s", desired + 1, total, fortune);
	fclose(fptr);
	return(0);
}

char *FileGetLine(FILE *pFile, int lineNum)
{
	if(!pFile)
		return(0);
	rewind(pFile);

	static char	string[1024];

	strcpy(string, "EOF reached\n");

	while(fgets(string, sizeof(string), pFile))
	{
		if(!lineNum--)
			break;
	}
	return(string);
}

int FileCountLines(FILE *pFile)
{
	if(!pFile)
		return(0);
	rewind(pFile);

	int	totalLines = 0;
	char	string[1024];

	while(fgets(string, sizeof(string), pFile))
		totalLines++;

	return(totalLines);
}
