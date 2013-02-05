#include <stdlib.h>
#include <stdio.h>

void* alloca(size_t size)
{
	printf("alloca\n");
	return NULL;
}
