#include <unistd.h>

uid_t getuid(void)
{
	printf("getuid\n");
	return 0;
}

uid_t geteuid(void)
{
	printf("geteuid\n");
	return 0;
}

gid_t getgid(void)
{
	printf("getgid\n");
	return 0;
}

gid_t getegid(void)
{
	printf("getegid\n");
	return 0;
}

pid_t getsid(pid_t pid)
{
	printf("getsid\n");
	return 0;
}

pid_t setsid(void)
{
	printf("setsid\n");
	return 0;
}

char* getlogin(void)
{
	printf("getlogin\n");
	return NULL;
}

pid_t getppid(void)
{
	printf("getppid\n");
	return 0;
}

pid_t getpgrp(void)
{
	printf("getpgrp\n");
	return 0;
}

pid_t getpgid(pid_t pid)
{
	printf("getpgid\n");
	return 0;
}

int setuid(uid_t uid)
{
	printf("setuid\n");
	return 0;
}

int seteuid(uid_t euid)
{
	printf("seteuid\n");
	return 0;
}

int setgid(gid_t gid)
{
	printf("setgid\n");
	return 0;
}

int setegid(gid_t egid)
{
	printf("setegid\n");
	return 0;
}

int setpgid(pid_t pid, pid_t pgid)
{
	printf("setpgid\n");
	return 0;
}

int setreuid(uid_t ruid, uid_t euid)
{
	printf("setreuid\n");
	return 0;
}

int setregid(gid_t rgid, gid_t egid)
{
	printf("setregid\n");
	return 0;
}

int setpgrp(void)
{
	printf("setpgrp\n");
	return 0;
}

int getgroups(int size, gid_t list[])
{
	printf("getgroups\n");
	return 0;
}

int setgroups(size_t size, const gid_t* list)
{
	printf("setgroups\n");
	return 0;
}

struct group* getgrnam(const char* name)
{
	printf("getgrnam(%s)\n", name);
	return NULL;
}

struct group* getgrgid(gid_t gid)
{
	printf("getgrgid(%u)\n", gid);
	return NULL;
}

