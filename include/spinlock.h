#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <preempt.h>

/* More of a per-arch thing */
struct SpinlockTag
{
	DWORD flags;
};

typedef struct SpinlockTag Spinlock;

#define SpinlockIrqSave(sLock) 
#define SpinlockIrqRestore(sLock) 
#define SpinLockIrq(sLock) IrqSaveFlags((sLock)->flags)
#define SpinUnlockIrq(sLock) IrqRestoreFlags((sLock)->flags)
#define SpinLock(sLock) \
	PreemptDisable()

#define SpinUnlock(sLock) \
	PreemptEnable()

#endif
