#include <stdlib.h>
#include <stdio.h>
#include <syscalls.h>

void abort()
{
	printf("Abnormal program termination from %#X\n", __builtin_return_address(0));
	
	SysExit(1);
}

struct AtExitFn
{
	void (*function)();
};

#define NUM_ATEXIT		512

struct AtExitFn exitFns[NUM_ATEXIT];
int numExits=0;

void exit(int returnCode)
{
	int i;

	for (i=0; i<numExits; i++)
		exitFns[i].function();

	SysExit(returnCode);
}

int atexit(void (*function)())
{
	if (numExits == NUM_ATEXIT)
		return -1;
		
	exitFns[numExits++].function = function;
	
	return 0;
}
