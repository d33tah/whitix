#ifndef _POSIX_TIME_H
#define _POSIX_TIME_H

#include <time.h> /* Include the c header for time_t */
#include <sys/types.h>

struct timeval
{
	int tv_sec;
	int tv_usec;
};

int gettimeofday(struct timeval* tp,void* tzp);

struct timespec
{
	time_t tv_sec;
	long tv_nsec;
};

/* itimers */
#define ITIMER_REAL		0
#define ITIMER_VIRTUAL	1
#define ITIMER_PROF		2

struct itimerval
{
	struct timeval it_interval;
	struct timeval it_value;
};

int getitimer(int which, struct itimerval* value);
int setitimer(int which, const struct itimerval* value, struct itimerval* ovalue);

int utimes(const char* filename, const struct timeval times[2]);
clock_t clock(void);

double difftime(time_t time1, time_t time2);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
struct tm *gmtime(const time_t *timep);

#endif
