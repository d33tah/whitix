#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>
#include <errno.h>

#include <syscalls.h>

#include "pthread_internal.h"

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr)
{
	mutex->list=NULL;
	mutex->_listlock = 0;
	mutex->_lock = 0;
	mutex->owner = -1;

	if (mutexattr)
		mutex->type = *mutexattr;
	else
		mutex->type = PTHREAD_MUTEX_NORMAL;

	return 0;
}

long BitTestAndSet(volatile long* value)
{
	long ret;
	
	asm volatile("xchgl %0, %1"
		: "=r"(ret), "=m"(*value)
		: "0" (1), "m" (*value)
		: "memory");
		
	return ret;
}

int _pthread_lock(pthread_mutex_t* mutex)
{
	struct pthread_queue entry;
	struct pthread_queue* last;
	int threadId;
	
	threadId = pGetCurrentThreadId();
	
	if (mutex->_lock == 0)
		goto acquire;

//	entry.threadId = threadId;
//	entry.next = NULL;

#if 0
	while (BitTestAndSet(&mutex->_listlock) == 1)
		SysYield();

	last = mutex->list;

	if (!last)
	{
		mutex->list=&entry;
	}else{	
		while (last->next)
			last=last->next;

		last->next=&entry;
	}
	
	mutex->_listlock = 0;
#endif

acquire:
//	printf("mutex->_lock = %#X, mutex->owner = %#X (%#X)\n", mutex->_lock, mutex->owner, threadId);

	while (BitTestAndSet(&mutex->_lock) == 1)
	{
//		if (SysSuspendThread(threadId))
//			return -1;
		SysYield();
	}
		
	mutex->owner = threadId;
	mutex->count = 0;
	
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
	switch (mutex->type)
	{
		case PTHREAD_MUTEX_NORMAL:
			_pthread_lock(mutex);
			break;
			
		case PTHREAD_MUTEX_RECURSIVE:
			if (mutex->_lock == 1 && mutex->owner == pGetCurrentThreadId())
			{
				mutex->count++;
				return 0;
			}
			
			_pthread_lock(mutex);
			break;
			
		default:
			printf("%#X: mutex->type = %#X, from %#X\n", mutex, mutex->type, __builtin_return_address(0));			
	}

	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
	switch (mutex->type)
	{
		case PTHREAD_MUTEX_RECURSIVE:
			if (mutex->owner == pGetCurrentThreadId())
			{
				mutex->count++;
				return 0;
			}
		
		case PTHREAD_MUTEX_NORMAL:
			if (BitTestAndSet(&mutex->_lock) == 0)
			{
				mutex->owner = pGetCurrentThreadId();
				mutex->count = 0;
				return 0;
			}
			break;
			
//		default:
//			printf("%#X: mutex->type = %#X, from %#X\n", mutex, mutex->type, __builtin_return_address(0));			
	}
	
	return EBUSY;	
}

int _pthread_unlock(pthread_mutex_t* mutex)
{
	/* TODO */
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	switch (mutex->type)
	{
		case PTHREAD_MUTEX_RECURSIVE:
			if (mutex->owner != pGetCurrentThreadId())
				return EPERM;
			
			if (mutex->count > 0)
			{
				mutex->count--;
				return 0;
			}

			mutex->owner = -1;
			
		case PTHREAD_MUTEX_NORMAL:
//			while (BitTestAndSet(&mutex->_listlock) == 1)
//				SysYield();
//			printf("Unlocking list\n");
			
			mutex->_lock = 0;

#if 0			
			if (mutex->list)
			{
				int threadId=mutex->list->threadId;
				mutex->list=mutex->list->next;
				SysResumeThread(threadId);
			}			
			mutex->_listlock = 0;
#endif
			
			break;
			
		default:
			printf("mutex->type = %#X, from %#X\n", mutex->type, __builtin_return_address(0));
	}

	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
	if (mutex->_lock == 1)
		return EBUSY;
		
	return 0;
}
