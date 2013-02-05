#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <sys/time.h>

#define RUSAGE_SELF	0x01
#define RLIMIT_CORE	0xDEADBEEF

struct rlimit
{
	unsigned long rlim_cur, rlim_max;
};

int getrlimit(int resource, struct rlimit* lim);

struct rusage
{
	struct timeval ru_utime;
	struct timeval ru_stime;
	
	long   ru_maxrss;        /* maximum resident set size */
	long   ru_ixrss;         /* integral shared memory size */
	long   ru_idrss;         /* integral unshared data size */
	long   ru_isrss;         /* integral unshared stack size */
	long   ru_minflt;        /* page reclaims */
	long   ru_majflt;        /* page faults */
	long   ru_nswap;         /* swaps */
	long   ru_inblock;       /* block input operations */
	long   ru_oublock;       /* block output operations */
	long   ru_msgsnd;        /* messages sent */
	long   ru_msgrcv;        /* messages received */
	long   ru_nsignals;      /* signals received */
	long   ru_nvcsw;         /* voluntary context switches */
	long   ru_nivcsw;        /* involuntary context switches */
};

int getrusage(int processes,struct rusage* rusage);

#endif
