#include <unistd.h>
#include <syscalls.h>
#include <sys/resource.h>
#include <sys/utsname.h>

pid_t getpid(void)
{
	return SysGetCurrentProcessId();
}

pid_t tcgetpgrp(int fd)
{
	printf("tcgetpgrp(%d)\n", fd);
	return 0;
}

int tcsetpgrp(int fd, pid_t pgrp)
{
	printf("tcsetpgrp\n");
	return 0;
}

int nice(int inc)
{
	printf("nice\n");
	return 0;
}

int execv(const char* path, char *const argv[])
{
	printf("execv\n");
	return 0;
}

int execve(const char *filename, char *const argv[], char *const envp[])
{
	printf("execve\n");
	return 0;
}

int uname(struct utsname* buf)
{
	/* TODO: Read /System/Config. */
	strcpy(buf->sysname, "Whitix");
	strcpy(buf->nodename, "");
	strcpy(buf->release, "");
	strcpy(buf->version, "0.2");
	strcpy(buf->machine, "i386");
	return 0;
}

int getrusage(int who, struct rusage *usage)
{
//	printf("getrusage\n");
	return 0;
}

int getrlimit(int resource, struct rlimit* limit)
{
#if 0
	switch (resource)
	{
		case 5:
			limit->rlim_cur=0;
			limit->rlim_max=0;
			break;
			
		case 9:
			limit->rlim_cur=limit->rlim_max=0xC0000000;
			break;
			
		default:
#endif
			printf("getrlimit(%d)\n", resource);
//	}
	
	return 0;
}

int setrlimit(int resource, const struct rlimit* limit)
{
	printf("setrlimit\n");
	return 0;
}

int execvp(const char *file, char *const argv[])
{
#if 0
	char buf[2048];
	int ret;
	
	ret = SysCreateProcess(file, NULL, argv);
	
	if (ret != -ENOENT)
		goto error;
		
	strcpy(buf, "/Applications/");
	strcat(buf, file);
	
	ret = SysCreateProcess(file, NULL, argv);
	
	if (ret)
			goto error;
		
error:
	errno = -ret;
	return -1;
#endif
	printf("execvp\n");
	exit(0);
	
	
	return 0;
}

int getdtablesize()
{
//	printf("getdtablesize\n");
	/* TODO: Read process control block to find out fds allocated. */
	return 1024;
}

int getpriority(int which, int who)
{
	printf("getpriority\n");
	return 0;
}

int setpriority(int which, int who, int prio)
{
	printf("setpriority\n");
	return 0;
}
