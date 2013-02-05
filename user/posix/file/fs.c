#include <sys/file.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <syscalls.h>

int statvfs(const char* path, struct statvfs* buf)
{
	printf("statvfs(%s)\n", path);
	return 0;
}

int fstatvfs(int fd, struct statvfs* buf)
{
	printf("fstatvfs(%d)\n", fd);
	return 0;
}
