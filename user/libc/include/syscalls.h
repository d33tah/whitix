#ifndef SYSCALLS_H
#define SYSCALLS_H

/* FIXME: CHANGE ALL THIS SOON */

/* Some useful structures for syscalls */

struct DirEntry
{
	unsigned long vNodeNum;
	unsigned long offset;
	unsigned short length;
	unsigned short type;
	char name[1];
};

struct Time
{
	unsigned long seconds,useconds;
};

struct Stat
{
	unsigned long long size;
	unsigned long vNum;
	unsigned long mode;
	unsigned long aTime,cTime,mTime;
};

#define _SYS_FILE_READ			1
#define _SYS_FILE_WRITE			2
#define _SYS_FILE_CREATE_OPEN	4
#define _SYS_FILE_FORCE_OPEN	8
#define _SYS_FILE_FORCE_CREATE	16
#define _SYS_FILE_DIRECTORY		32

#define _SYS_STAT_FILE			1
#define _SYS_STAT_DIR			2
#define _SYS_STAT_READ			4
#define _SYS_STAT_WRITE			8
#define _SYS_STAT_BLOCK			16
#define _SYS_STAT_CHAR			32

/* Constant for SysMemoryMap */
#define _SYS_MMAP_PRIVATE	0x00000000
#define _SYS_MMAP_SHARED	0x00000001
#define _SYS_MMAP_FIXED		0x00000002
#define _SYS_MMAP_GROWDOWN	0x00000004

/* Defines for architecture-specific memory functions. */
#define PAGE_SIZE	4096 /* 4K */
#define PAGE_ALIGN(addr) ((addr) & ~(PAGE_SIZE-1))
#define PAGE_OFFSET(addr) ((addr) & (PAGE_SIZE-1))
#define PAGE_ALIGN_UP(addr) (PAGE_ALIGN((addr)+PAGE_SIZE-1))

#define SYSCALL(SysCallIndex,ReturnType,FunctionName,ArgBytes,Arguments) \
	ReturnType FunctionName Arguments

#include "sysdefs.h"

int SysGetTime(void* time);

#endif
