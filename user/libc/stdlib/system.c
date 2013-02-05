#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syscalls.h>

/* NOT TESTED */

int system(const char* command)
{
	int err=0;
	char* args;
	int fds[]={0,1,2};
	int i=0;

	if (!command)
		return 1;

	char* progName=(char*)malloc(strlen(command));
	char* cmdStr=NULL;

	/* Copy over command, so it can be edited */

	/* Copy program name into new string */
	while (command[i] != ' ' && command[i])
	{
		progName[i]=command[i];
		i++;
	}

	progName[i]='\0';
	/* Commands after? */
	if (command[i] == ' ')
		cmdStr=&command[i+1];
		
	err=SysCreateProcess(progName,fds,cmdStr);

	if (err < 0)
	{
		errno=-err;
		return -1;
	}
		
	err=SysWaitForProcessFinish(err,NULL);
	if (err < 0)
	{
		errno=-err;
		return -1;
	}
	

	return err;
}

int setenv(const char* name, const char* value, int overwrite)
{
	printf("setenv(%s)\n", name);
	return 0;
}

int unsetenv(const char* name)
{
	printf("unsetenv(%s)\n", name);
	return 0;
}
