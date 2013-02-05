#include <sys/wait.h>
#include <syscalls.h>
#include <errno.h>

pid_t wait(int* status)
{
	pid_t ret;
	int state;

	ret=SysWaitForProcessFinish(-1,&state);

	if ((int)ret < 0)
	{
		errno=-(int)ret;
		return -1;
	}

	*status=state;
	return ret;
}

pid_t wait3(int* status, int options, struct rusage* rusage)
{
	printf("wait3\n");
}

pid_t wait4(pid_t pid, int* status, int options, struct rusage* rusage)
{
	printf("wait4\n");
}

void _exit(int status)
{
	SysExit(status);
}

int pause(void)
{
	printf("pause\n");
}
pid_t waitpid(pid_t pid, int* status, int options)
{
	pid_t ret;
	int state;

	ret=SysWaitForProcessFinish(pid, &state);

	if ((int)ret < 0)
	{
		errno=-(int)ret;
		return -1;
	}

	*status=state;
	return ret;
}

