#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#include <sys/types.h>

typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;

#define FSTYPSZ		16

struct statvfs
{
	u_long      f_bsize;             /* preferred file system block size */
	u_long      f_frsize;            /* fundamental filesystem block
		                                    (size if supported) */
	fsblkcnt_t  f_blocks;            /* total # of blocks on file system
		                                     in units of f_frsize */
	fsblkcnt_t  f_bfree;             /* total # of free blocks */
	fsblkcnt_t  f_bavail;            /* # of free blocks avail to
		                                     non-super-user */
	fsfilcnt_t  f_files;             /* total # of file nodes (inodes) */
	fsfilcnt_t  f_ffree;             /* total # of free file nodes */
	fsfilcnt_t  f_favail;            /* # of inodes avail to
		                                     non-super-user*/
	u_long      f_fsid;              /* file system id (dev for now) */
	char        f_basetype[FSTYPSZ]; /* target fs type name,
		                                     null-terminated */
	u_long      f_flag;              /* bit mask of flags */
	u_long      f_namemax;           /* maximum file name length */
	char        f_fstr[32];          /* file system specific string */
	u_long      f_filler[16];        /* reserved for future expansion */
};

int statvfs(const char* path, struct statvfs* buf);
int fstatvfs(int fd, struct statvfs* buf);

#endif
