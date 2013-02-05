#ifndef I386_SIB_H
#define I386_SIB_H

#include <typedefs.h>

struct SystemInfoBlock
{
	int version;
	
	/* Time */
	unsigned long seconds;
	unsigned long uSeconds;
	unsigned long upTime;
	
	/* Processes */
	unsigned long numProcesses, numRunning;

	/* System information. */
	unsigned long pageSize;
	char osVersion[16];
}PACKED;

/* SIB API */
void SibUpdateTime(unsigned long seconds, unsigned long useconds);

#endif
