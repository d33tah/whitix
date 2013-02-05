#include <errno.h>
#include <semaphore.h>

/* For printf. REMOVE WHEN ALL IMPLEMENTED */
#include <stdio.h>

#define UNIMPL printf(__func__)

int sem_init(sem_t* sem,int pshared,unsigned int value)
{
	if (pshared == 1)
	{
		errno = ENOSYS;
		return -1;
	}

	sem->value = value;
	sem->lock = 0;
	return 0;
}

int sem_wait(sem_t* sem)
{
	if (sem->value > 0)
	{
		--sem->value;
		return 0;
	}
	
	while (sem->value > 0)
		SysYield();

	return 0;
}

int sem_destroy(sem_t* sem)
{
	return 0;
}

int sem_post(sem_t* sem)
{
	sem->value = 0;
//	UNIMPL;
}

int sem_getvalue(sem_t* sem,int* value)
{
	UNIMPL;
}
