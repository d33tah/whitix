#ifndef _POSIX_SEMAPHORE_H
#define _POSIX_SEMAPHORE_H

struct sem_s
{
	int value;
	unsigned int lock;
};

typedef struct sem_s sem_t;

#endif
