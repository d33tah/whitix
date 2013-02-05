#include <errno.h>
#include <time.h>
#include <syscalls.h>

struct tm localTime;

time_t time(time_t* time)
{
	struct Time retVal;

	SysGetTime(&retVal);

	if (time)
		*time=retVal.seconds;

	return retVal.seconds;
}

struct tm* localtime(const time_t* time)
{
	if (!time)
	{
		errno=EINVAL;
		return NULL;
	}

	localTime.tm_sec=(*time % 60);
	localTime.tm_min=(*time/60) % 60;
	localTime.tm_hour=(*time/(60*60)) % 24;

	return &localTime;
}

char *ctime(const time_t *timep)
{
	printf("ctime\n");
	return NULL;
}
