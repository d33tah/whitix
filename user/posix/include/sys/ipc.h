#ifndef _POSIX_SYS_IPC_H
#define _POSIX_SYS_IPC_H

#define IPC_PRIVATE		0

#define IPC_CREAT		0
#define IPC_RMID		1

#include <sys/time.h>
#include <sys/types.h>

struct ipc_perm
{
	key_t key;
	uid_t uid;
	gid_t gid;
	uid_t cuid;
	gid_t cgid;
	unsigned short mode;
	unsigned short seq;
};

#endif
