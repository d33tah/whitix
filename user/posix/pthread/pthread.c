#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>
#include <syscalls.h>

/* For printf. REMOVE WHEN ALL IMPLEMENTED */
#include <stdio.h>

#include "pthread_internal.h"

#define UNIMPL printf("%s\n", __func__); exit(0)

struct pthread_queue first = {0, NULL};
struct pthread_queue* threadList = &first;
struct pthread_queue* joinList = NULL;
pthread_mutex_t joinMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;

struct pthreadArgs
{
	void* (*startRoutine)(void*);
	pthread_cond_t* signal;
	void* args;
};

static void pthread_start(void* arg)
{
	struct pthreadArgs* pArgs=(struct pthreadArgs*)arg;
	void* (*start)(void*) = pArgs->startRoutine;
	void* args = pArgs->args;
	
	struct pthread_queue entry;
	struct pthread_queue* head;

	DlTlsCreateContext();
	
	entry.threadId = pGetCurrentThreadId();
	entry.next = NULL;
	
	pthread_mutex_lock(&listMutex);
	
	head = threadList;
	
	while (head->next)
		head = head->next;
		
	head->next = &entry;
	
	pthread_mutex_unlock(&listMutex);

	pthread_cond_signal(pArgs->signal);

	start(args);
	
	/* Remove ourselves from the thread list. */
	pthread_mutex_lock(&listMutex);
	
	head = threadList;
	
	while (head->next && head->next != &entry)
		head = head->next;
	
	if (head->next)
		head->next = head->next->next;
		
	pthread_mutex_unlock(&listMutex);

	/* Wake up and remove everything in the join list. */
	pthread_mutex_lock(&joinMutex);
	
	head = joinList;
	joinList = NULL;
	
	while (head)
	{
		SysResumeThread(head->threadId);
		
		head = head->next;
	}
	
	pthread_mutex_unlock(&joinMutex);
	
	/* Actually exit the thread. We used to be able to return onto a frame
	 * set up by the kernel, but that no longer happens. */
	SysExitThread(-1);
}

int pthread_create(pthread_t* thread,const pthread_attr_t* attr,void* (*startRoutine)(void*),void* args)
{
	pthread_cond_t signal;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	
	struct pthreadArgs pArgs;
	
	pArgs.startRoutine = startRoutine;
	pArgs.args = args;
	pArgs.signal = &signal;
	
	pthread_cond_init(&signal, 0);
	
	*thread = SysCreateThread(pthread_start, 0, &pArgs);
	
	/* Wait for the new thread to tell it's started up okay. */
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&signal, &mutex);
	pthread_mutex_unlock(&mutex);
	
	return 0;
}

static int pCheckThreadList(pthread_t thread)
{
	struct pthread_queue* head = threadList;
	int ret = 0;
	
	pthread_mutex_lock(&listMutex);
	
	while (head)
	{
		if (head->threadId == thread)
		{
			ret = 1;
			break;
		}
		
		head = head->next;			
	}
	
	pthread_mutex_unlock(&listMutex);
	
	return ret;
}

int pthread_join(pthread_t thread, void** valuePtr)
{
	if (valuePtr)
		printf("valuePtr != NULL\n");
		
	struct pthread_queue entry;
	struct pthread_queue* head;

	/* Check if it is already on the thread list. */
	if (!pCheckThreadList(thread))
		return 0;	
	
	while (1)
	{
		/* Add to the join list. */
		pthread_mutex_lock(&joinMutex);
	
		head = joinList;

		entry.threadId = pGetCurrentThreadId();
		entry.next = NULL;

		printf("waiting for %d\n", thread);

		if (!head)
		{
			joinList = &entry;
		}else{
			while (head->next)
				head = head->next;
			
			head->next = &entry;	
		}
	
		pthread_mutex_unlock(&joinMutex);
		
		SysSuspendThread(pGetCurrentThreadId());
	
		if (!pCheckThreadList(thread))
			break;
	}
	
	pthread_mutex_lock(&joinMutex);
	pthread_mutex_unlock(&joinMutex);
		
	return 0;
}

int pthread_detach(pthread_t thread)
{
	UNIMPL;
}

int pthread_getattr_np(pthread_t thread,pthread_attr_t* attr)
{
	UNIMPL;
}

int pthread_sigmask(int how,const sigset_t* set,sigset_t* oset)
{
	UNIMPL;
}

int pthread_attr_setstacksize(pthread_attr_t* attr,size_t stackSize)
{
	UNIMPL;
}

int pthread_kill(pthread_t thread,int sig)
{
	UNIMPL;
}

int pthread_mutexattr_init(pthread_mutexattr_t* attr)
{
	*attr = PTHREAD_MUTEX_NORMAL;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t* attr)
{
	return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr,int* type)
{
	*type = *attr;
}

int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type)
{
	*attr = type;
}

void pthread_exit(void* retval)
{
	UNIMPL;
	while (1);
}

int pthread_cancel(pthread_t thread)
{
	UNIMPL;
	while (1);
}

pthread_t pthread_self(void)
{
	return pGetCurrentThreadId();
}

int pthread_setschedparam(pthread_t target_thread, int policy, const struct sched_param *param)
{
	printf("pthread_setschedparam\n");
	return 0;
}

int pthread_getschedparam(pthread_t target_thread, int *policy, struct sched_param *param)
{
	*policy = SCHED_OTHER;
	return 0;
}

void _pthread_cleanup_push(struct _pthread_cleanup_buffer* buffer, void (*routine)(void*), void* arg)
{
//	printf("_pthread_cleanup_push\n");
}

void _pthread_cleanup_pop(struct _pthread_cleanup_buffer* buffer, int execute)
{
	if (execute == 1)
		printf("_pthread_cleanup_pop: TODO\n");
//	printf("_pthread_cleanup_pop\n");
}
