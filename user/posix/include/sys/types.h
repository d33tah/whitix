#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stddef.h> /* For size_t and ssize_t */

typedef unsigned int dev_t;
typedef unsigned int ino_t;
typedef unsigned int mode_t;
typedef unsigned int nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#ifndef DEFINED_OFFT
#define DEFINED_OFFT
typedef unsigned int off_t;
#endif
typedef unsigned int blksize_t;
typedef unsigned int blkcnt_t;
typedef unsigned long clock_t;
typedef volatile unsigned int sig_atomic_t;
typedef unsigned int sigset_t;
typedef unsigned int key_t;

/* Pointer types */
typedef unsigned int uintptr_t;
typedef int	intptr_t;

typedef unsigned short ushort;
typedef unsigned long u_long;

typedef unsigned long pid_t;

typedef char* caddr_t;

#endif
