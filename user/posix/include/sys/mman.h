#ifndef MMAN_H
#define MMAN_H

#include <sys/shm.h>
#include <sys/types.h>
#include <syscalls.h>

#define PROT_NONE	0x0
#define PROT_READ	0x1
#define PROT_WRITE	0x2
#define PROT_EXEC	0x1

#define MAP_SHARED	0x1
#define MAP_PRIVATE	0x0
#define MAP_FIXED	0x2
#define MAP_ANON	0x0
#define MAP_FAILED	-1

#define MS_ASYNC		0x1
#define MS_SYNC			0x2
#define MS_INVALIDATE	0x4

void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void* start, size_t length);

#define MADV_NORMAL		0x01
#define MADV_DONTNEED	0x02

int madvise(void* start, size_t length, int advice);

#endif
