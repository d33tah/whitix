#include <stdlib.h>
#include <ctype.h>

int atoi(const char* s)
{
	return (int)strtol(s, NULL, 10);
}

long atol(const char* s)
{
	return strtol(s, NULL, 10);
}
