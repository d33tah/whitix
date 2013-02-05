#include <errno.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include <stdio.h> //TEMP

#include <syscalls.h>

void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset)
{
	unsigned long ret=SysMemoryMap((unsigned long)start, length, prot, fd, offset, flags);

//	printf("mmap(%#X, %u, %d), from %#X\n", start, length, prot, __builtin_return_address(0));

	if (!ret)
	{
		errno=EINVAL;
		return (void*)(-1);
	}

	return (void*)ret;
}

int munmap(void* start, size_t length)
{
	int ret = SysMemoryUnmap(start, length);
	
	if (ret)
	{
		errno = EINVAL;
		return -1;
	}
	
	return 0;
}

void* shmat(int shmid, const void* shmaddr, int shmflgs)
{
	void* ret=(void*)SysSharedMemoryAttach(shmid, shmaddr, shmflgs);

	if (!ret)
		return (void*)-1;

	return ret;
}

int shmdt(const void* shmaddr)
{
	printf("shmdt\n");
	return 0;
}

int shmctl(int shmid, int cmd, struct shmid_ds* buf)
{
//	printf("shmctl\n");
	return 0;
}

int shmget(key_t key, size_t size, int shmflg)
{
	int id=SysSharedMemoryGet(key, size, shmflg);

	if (id < 0)
		return -1;

	return id;
}

int posix_memalign(void** memptr, size_t alignment, size_t size)
{
//	printf("posix_memalign(alignment = %#X, size = %#X)\n", alignment, size);

	unsigned char* ptr;

	ptr = memalign(alignment, size); /* Obselete? */

	*memptr = ptr;

	return 0;
}

void swab(const void* from, void* to, ssize_t n)
{
	printf("swab\n");
}

#if 0
void* sbrk(int* increment)
{
	return SysMoreCore((unsigned long)increment);
}
#endif

int getpagesize(void)
{
	return 4096;
}

int __popcountsi2(int x)
{
	printf("__popcountsi2\n");
	exit(0);
}

int madvise(void* start, size_t length, int advice)
{
	printf("madvise\n");
	return 0;
}

int mprotect(const void* addr, size_t len, int prot)
{
	int ret = SysMemoryProtect(addr, len, prot);
	
	if (ret)
	{
		errno = ret;
		return -1;
	}
	
	return 0;
}

int msync(void* start, size_t length, int flags)
{
	printf("msync\n");
	return 0;
}

void* mremap(void* old_address, size_t old_size, size_t new_size, int flags)
{
	printf("mremap\n");
	SysExit(0);
	return NULL;
}
