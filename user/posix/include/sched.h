#ifndef _POSIX_SCHED_H
#define _POSIX_SCHED_H

#include <time.h>
#include <sys/types.h>

struct sched_param
{
	int sched_priority;
};

#define SCHED_FIFO		0
#define SCHED_RR		1
#define SCHED_OTHER		2

int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);
int sched_get_param(pid_t pid,struct sched_param* scParam);
int sched_yield(void);

#endif
