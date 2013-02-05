#ifndef CLIB_TIME_H
#define CLIB_TIME_H

#include <stdio.h>
#include <stddef.h>

struct tm
{
	int tm_hour;
	int tm_isdst;
	int tm_mday;
	int tm_min;
	int tm_mon;
	int tm_sec;
	int tm_wday;
	int tm_yday;
	int tm_year;
	int tm_gmtoff;
};

typedef unsigned long time_t;

time_t time(time_t* time);
struct tm* localtime(const time_t* timer);

char *ctime(const time_t *timep);

#define CLOCKS_PER_SEC		1000000

#endif
