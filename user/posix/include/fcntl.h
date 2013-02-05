#ifndef _POSIX_FCNTL_H
#define _POSIX_FCNTL_H

#include <sys/stat.h>
#include <syscalls.h>

/* File flags. FIXME */

#define O_SYNC 0
#define O_CREAT _SYS_FILE_CREATE_OPEN
#define O_EXCL (1 << 31)
#define O_RDONLY _SYS_FILE_READ
#define O_RDWR (_SYS_FILE_WRITE | _SYS_FILE_READ)
#define O_WRONLY _SYS_FILE_WRITE
#define O_TRUNC 128
#define O_NONBLOCK	0x100
#define O_APPEND	0x200

#define F_SETFD 1 
#define F_CLOEXEC 0
#define F_DUPFD		2

#define FD_CLOEXEC 1

#define FASYNC 0x1

int fcntl(int fd, int cmd, ...);

#define POSIX_FADV_NORMAL		0x01
#define POSIX_FADV_SEQUENTIAL	0x02
#define POSIX_FADV_RANDOM		0x03

#endif
