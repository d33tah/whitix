#ifndef UTIME_H
#define UTIME_H

#include <sys/types.h>

struct utimbuf
{
	time_t actime,modtime;
};

static inline int utime(const char* filename,struct utimbuf* buf)
{
	printf("utime\n");
	return 0;
}

#endif
