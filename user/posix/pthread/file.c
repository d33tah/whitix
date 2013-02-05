#include <stdio.h>
#include <pthread.h>

void flockfile(FILE* file)
{
	file->_lock=1;
}

void funlockfile(FILE* file)
{
	file->_lock=0;
}
