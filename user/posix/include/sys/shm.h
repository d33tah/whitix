#ifndef _SYS_SHM_H
#define _SYS_SHM_H

#include <sys/ipc.h>
#include <sys/types.h>

#include <stddef.h>

typedef unsigned int shmatt_t;

struct shmid_ds
{
	struct ipc_perm		shm_perm;
	size_t				shm_segsz;
	time_t				shm_atime;
	time_t				shm_dtime;
	time_t				shm_ctime;
	pid_t				shm_cpid;
	pid_t				shm_lpid;
	shmatt_t			shm_nattch;
};

#define SHM_RDONLY		0x01

int shmget(key_t key, size_t size, int shmflg);
void* shmat(int shmid, const void* shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds* buf);

#endif
