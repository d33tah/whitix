#include <stdio.h>

int fscanf(FILE* stream, const char* format, ...)
{
	printf("fscanf(%s)\n", format);
	return 0;
}

int scanf(const char* format, ...)
{
	printf("scanf(%s)\n", format);
	return 0;
}
