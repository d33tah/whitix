#include <stdio.h>
#include <sync.h>

/* Will be used in SMP applications. For single-threaded applications, the current implementation will suffice. */
int _CompareAndSwap(long int* pointer, long int oldVal, long int newVal)
{
	char ret;
	long int readVal;

	asm volatile("lock; cmpxchgl %3, %1; sete %0"
				: "=q" (ret), "=m" (*pointer), "=a" (readVal)
				: "r" (newVal), "m" (*pointer), "a" (oldVal)
				: "memory");

	return ret;
}

void MutexCreate(struct Mutex* mutex)
{
	mutex->_lock=0;
}

int MutexLock(struct Mutex* mutex)
{
	if (mutex->_lock == 1)
	{
		printf("MutexLock: todo\n");
		while (1);
		return 1;
	}

	mutex->_lock=1;
	return 0;
}

int MutexTryLock(struct Mutex* mutex)
{
	return mutex->_lock;
}

int MutexUnlock(struct Mutex* mutex)
{
	mutex->_lock=0;
	return 0;
}

int MutexDestroy(struct Mutex* mutex)
{
	/* Null function for the moment. */
	return 0;
}
