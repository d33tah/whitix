#ifndef STAT_H
#define STAT_H

#include <sys/time.h>
#include <sys/types.h>

/* Temp values for now. FIXME */
#define S_ISREG(m) (m & 0x01)
#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISFIFO(m) (0)
#define S_ISSOCK(fd) 0
#define S_ISLNK(fd) 0
#define S_ISCHR(fd) 0
#define S_ISBLK(fd) 0

#define S_IFBLK		0

#define S_IFDIR		0x02

/* CHANGE */
#define S_IFSOCK	0x20
#define S_IFIFO		0x40

#define S_IXOTH		0x10
#define S_IXUSR		0x20
#define S_IXGRP		0x40

#define S_IFMT		0xF

#define S_IWRITE	0x100

#define F_WRLCK 0
#define F_SETLK 0
#define F_UNLCK	0

struct flock
{
	int l_type,l_whence,l_start,l_len;
};

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
};

/* Defines of stat fields for backwards compat. */
#define st_mtim	st_mtime

int open(const char* path,int oflag,...);
int stat(const char* file_name,struct stat* buf);
int lstat(const char* file_name,struct stat* buf);
int fstat(int fd,struct stat* buf);
int chmod(const char* path, mode_t mode);
int mkdir(const char* path, mode_t mode);
mode_t umask(mode_t cmask);

/* MOVE */
#define F_SETFL 1
#define F_GETFL 0

#endif

