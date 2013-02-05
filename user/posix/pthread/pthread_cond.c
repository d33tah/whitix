#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>
#include <syscalls.h>

/* For printf. REMOVE WHEN ALL IMPLEMENTED */
#include <stdio.h>

#include "pthread_internal.h"

int pthread_cond_init(pthread_cond_t* cond, pthread_condattr_t* cond_attr)
{
	/* cond_attr is ignored */
	
	pthread_mutex_init(&cond->mutex, NULL);
	
	cond->list = NULL;
	cond->signal = PTHREAD_COND_INITIAL;
	
	return 0;
}

int pthread_cond_signal(pthread_cond_t* cond)
{
	struct pthread_queue* queue;
	
	pthread_mutex_lock(&cond->mutex);
	queue = cond->list;
	
	if (cond->list)
		cond->list = cond->list->next;
		
	pthread_mutex_unlock(&cond->mutex);	

	/* Wake up the first in the queue. */
	if (queue)
	{
		cond->signal = PTHREAD_COND_SIGNAL;
		SysResumeThread(queue->threadId);
	}

	return 0;
}

int pthread_cond_broadcast(pthread_cond_t* cond)
{
	struct pthread_queue* queue;
	
	pthread_mutex_lock(&cond->mutex);
	
	queue = cond->list;
	cond->list = NULL;

	pthread_mutex_unlock(&cond->mutex);

	if (queue)
		cond->signal = PTHREAD_COND_SIGNAL;

	while (queue)
	{
		int threadId = queue->threadId;
		SysResumeThread(threadId);
		queue = queue->next;
	}

	return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
	struct pthread_queue entry;
	struct pthread_queue* last=cond->list;
	int threadId;

	threadId = pGetCurrentThreadId();

	if (mutex->owner != threadId)
		return EINVAL;

	if (cond->signal == PTHREAD_COND_SIGNAL)
	{
		cond->signal = PTHREAD_COND_INITIAL;
		return 0;
	}

	entry.threadId=threadId;
	entry.next=NULL;

	pthread_mutex_lock(&cond->mutex);

	if (!last)
	{
		cond->list=&entry;
	}else{
		while (last->next)
			last=last->next;

		last->next=&entry;
	}
	
	pthread_mutex_unlock(&cond->mutex);

	pthread_mutex_unlock(mutex);

	while (cond->signal != PTHREAD_COND_SIGNAL)
		if (SysSuspendThread(threadId))
			return -1;

	pthread_mutex_lock(mutex);
			
	cond->signal = PTHREAD_COND_INITIAL;

	return 0;
}

int pthread_cond_destroy(pthread_cond_t* cond)
{
	if (cond->list)
	{
		printf("pthread_cond_destroy: there are still waiters on this condition variable.\n");
		return EBUSY;
	}
	
	pthread_mutex_destroy(&cond->mutex);

	return 0;
}

struct timespec;

int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* abstime)
{
	printf("pthread_cond_timedwait\n");
	SysShutdown(0);
	while (1);
	return 0;
}

