#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <syscalls.h>
#include <stdlib.h>

#include "pthread_internal.h"

struct pThreadKey
{
	int inUse;
	void (*destructor)(void*);
};

#define PTHREAD_SECOND_LEVEL_SIZE	32
#define PTHREAD_FIRST_LEVEL_SIZE	((PTHREAD_KEYS_MAX + PTHREAD_SECOND_LEVEL_SIZE - 1) \
	/ PTHREAD_SECOND_LEVEL_SIZE)

static struct pThreadKey pKeys[PTHREAD_KEYS_MAX];

static pthread_mutex_t pKeyMutex = PTHREAD_MUTEX_INITIALIZER;

struct pthread_descr
{
	void** specific[PTHREAD_FIRST_LEVEL_SIZE];
};

typedef struct pthread_descr* pthread_descr_t;

struct pthread_descr initial =
{
	{NULL,} 
};

pthread_descr_t thread_self()
{
	if (pGetCurrentThreadId() == 0)
		return &initial;
		
	printf("%#X\n", SysGetCurrentThreadId());
	exit(0);

	return 0;
}

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
	int i;
	
	pthread_mutex_lock(&pKeyMutex);

	for (i=0; i<PTHREAD_KEYS_MAX; i++)
	{
		if (!pKeys[i].inUse)
		{
			pKeys[i].inUse = 1;
			pKeys[i].destructor = destructor;
			pthread_mutex_unlock(&pKeyMutex);
			*key = i;
			return 0;
		}
	}
	
	pthread_mutex_unlock(&pKeyMutex);
	return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
	pthread_mutex_lock(&pKeyMutex);
	
	if (key >= PTHREAD_KEYS_MAX || !pKeys[key].inUse)
	{
		pthread_mutex_unlock(&pKeyMutex);
		return EINVAL;
	}
	
	pKeys[key].inUse = 0;
	pKeys[key].destructor = NULL;
	
	/* TODO: Null out in other threads? */
	
	pthread_mutex_unlock(&pKeyMutex);
	
	return 0;
}

int pthread_setspecific(pthread_key_t key,const void* value)
{
	pthread_descr_t self;
	unsigned int firstLevel, secondLevel;
	
	if (key >= PTHREAD_KEYS_MAX || !pKeys[key].inUse)
		return EINVAL;
		
	self = thread_self();
	
	firstLevel = key / PTHREAD_SECOND_LEVEL_SIZE;
	secondLevel = key % PTHREAD_SECOND_LEVEL_SIZE;
	
	if (!self->specific[firstLevel])
	{
		self->specific[firstLevel] = calloc(PTHREAD_SECOND_LEVEL_SIZE, sizeof(void*));
		
		if (!self->specific[firstLevel])
			return ENOMEM;
	}
	
	self->specific[firstLevel][secondLevel] = (void*)value;
	
	return 0;
}

void* pthread_getspecific(pthread_key_t key)
{
	pthread_descr_t self;
	unsigned int firstLevel, secondLevel;
	
	if (key >= PTHREAD_KEYS_MAX)
		return NULL;

	self = thread_self();

	firstLevel = key / PTHREAD_SECOND_LEVEL_SIZE;
	secondLevel = key % PTHREAD_SECOND_LEVEL_SIZE;
	
	if (!self->specific[firstLevel] || !pKeys[key].inUse)
		return NULL;
		
	return self->specific[firstLevel][secondLevel];
}


