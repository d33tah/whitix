#ifndef _POSIX_SEM_H
#define _POSIX_SEM_H

#define IPC_CREAT	0x01
#define IPC_EXCL	0x02
#define IPC_RMID	0x04
#define IPC_NOWAIT	0x08

#define GETVAL	0
#define SETALL 1
#define SETPID 2
#define GETPID 3
#define GETALL 4

#define SEM_UNDO	0x01

struct sembuf
{
	int sem_num,sem_op,sem_flg;
};


#endif
