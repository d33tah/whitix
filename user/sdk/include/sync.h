#ifndef _SYNC_H
#define _SYNC_H

struct Mutex
{
	int _lock;
};

void MutexCreate(struct Mutex* mutex);
int MutexLock(struct Mutex* mutex);
int MutexTryLock(struct Mutex* mutex);
int MutexUnlock(struct Mutex* mutex);
int MutexDestroy(struct Mutex* mutex);

#endif
