#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>
#include <syscalls.h>

/* For printf. REMOVE WHEN ALL IMPLEMENTED */
#include <stdio.h>

#define UNIMPL printf(__func__)

int pthread_attr_init(pthread_attr_t* attr)
{
	attr->detachstate=PTHREAD_CREATE_JOINABLE;
	attr->scope = PTHREAD_SCOPE_SYSTEM;

	return 0;
}

int pthread_attr_destroy(pthread_attr_t* attr)
{
	return 0;
}

int pthread_attr_setscope(pthread_attr_t* attr, int scope)
{
	switch (scope)
	{
		case PTHREAD_SCOPE_SYSTEM:
			attr->scope = scope;
			return 0;

		case PTHREAD_SCOPE_PROCESS:
			return ENOTIMPL; /* Find out if we can support this? */

		default:
			return EINVAL;
	}
}

int pthread_attr_getstack(const pthread_attr_t* attr,void** stackAddr,size_t* stackSize)
{
	if (pthread_self() == 0)
	{
		*stackSize = 2*1024*1024;
		*(unsigned long**)stackAddr = (unsigned long*)(0xC0000000-*stackSize);
	}else
		SysExit(0);
		
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate)
{
	/* TODO: Check. */
	attr->detachstate = detachstate;
	return 0;
}

int pthread_attr_getdetachstate( const pthread_attr_t* attr, int* detachstate)
{
	*detachstate = attr->detachstate;
	return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t* attr, struct sched_param *param)
{
	UNIMPL;
	return 0;
}

int pthread_attr_setschedparam(const pthread_attr_t* attr, struct sched_param *param)
{
	UNIMPL;
	return 0;
}

