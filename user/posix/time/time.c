#include <syscalls.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

int gettimeofday(struct timeval* tp,void* tzp)
{
	return SysGetTime(tp);
}

int getloadavg(double loadavg[], int nelem)
{
	printf("getloadavg\n");
	return 0;
}

clock_t clock(void)
{
	printf("clock\n");
	return 0;
}

time_t mktime(struct tm* tm)
{
	printf("mktime\n");
	return 0;
}

int utimes(const char* filename, const struct timeval times[2])
{
	printf("utimes\n");
	return 0;
}

clock_t times(struct tm* buf)
{
	printf("times\n");
	return 0;
}

double difftime(time_t time1, time_t time2)
{
	return (time1-time2);
}

size_t strftime(char* s, size_t max, const char* format, const struct tm* tm)
{
	printf("strftime\n");
	return 0;
}

struct tm* gmtime(const time_t* timep)
{
	printf("gmtime\n");
	return NULL;
}

int usleep(unsigned int usec)
{
	printf("usleep\n");
	return 0;
}

struct tm* localtime_r(const time_t* timep, struct tm* result)
{
	printf("localtime_r\n");
	return NULL;
}

time_t timegm(struct tm *tm)
{
	printf("timegm\n");
	return 0;
}

time_t timelocal(struct tm *tm)
{
	printf("timelocal\n");
	return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem)
{
	printf("nanosleep(%#X)\n", req->tv_sec);
	return 0;
}

char* asctime(const struct tm* tm)
{
	return NULL;
}

unsigned int sleep(unsigned int seconds)
{
	printf("sleep(%u)\n", seconds);
	return 0;
}

void sched_yield()
{
	SysYield();
}

typedef unsigned int cpu_set_t;

int sched_setaffinity(pid_t pid, unsigned int cpusetsize, cpu_set_t *mask)
{
	printf("sched_setaffinity\n");
	return 0;
}

int sched_get_priority_max(int policy)
{
	return 32;
}

int sched_get_priority_min(int policy)
{
	return 0;
}

int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue)
{
	printf("setitimer\n");
	return 0;
}
