#include <errno.h>

__thread int realErrno=0;

int* errnoLocation()
{
	return &realErrno;
}
