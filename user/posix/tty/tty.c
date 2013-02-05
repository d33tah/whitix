#include <unistd.h>

#include <stdio.h> //TEMP

int isatty(int fd)
{
	/* Need to search through /System/Devices for file corresponding to the current file's data. See SysStat. */

	/* For now, consider stdout, stdin and stderr to be ttys. */
	if (fd > 2)
		return 0;

	return 1;
}

char* ttyname(int fd)
{
	printf("ttyname\n");
	return NULL;
}

char* ctermid(char* s)
{
	printf("ctermid\n");
	return NULL;
}
