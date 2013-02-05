#include <unistd.h>

#include <stdio.h> //TEMP

size_t confstr(int name, char* buf, size_t len)
{
	printf("confstr\n");
}

long sysconf(int name)
{
	if (name == 4096)
		return 4096;

	return -1;
}

long pathconf(char *path, int name)
{
	printf("pathconf\n");
}

long fpathconf(int fd, int name)
{
	printf("fpathconf\n");
}
