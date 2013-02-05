#include <sched.h>
#include <module.h>
#include <slab.h>
#include <task.h>
#include <print.h>
#include <panic.h>
#include <i386/process.h>
#include <sys.h>
#include <preempt.h>
#include <malloc.h>

/* Used when a thread is exiting */
BYTE exitStack[PAGE_SIZE/4];

Spinlock thrListLock;
struct Cache* threadCache;
extern int nrRunning;
LIST_HEAD(threadList);

/***********************************************************************
 *
 * FUNCTION: ThrCreateThread
 *
 * DESCRIPTION: Create a kernel/user thread.
 *
 * PARAMETERS: parent - the thread's parent.
 *			   entry - the starting address of the thread.
 *			   user - 1 for user, 0 for kernel.
 *			   stackP - stack pointer of the new thread. Passed to
 *						ThrArchCreateThread.
 *
 * RETURNS: pointer to thread information.
 *
 ***********************************************************************/

struct Thread* ThrCreateThread(struct Process* parent,DWORD entry,int user,DWORD stackP, void* argument)
{
	struct Thread* ret;

	ret=(struct Thread*)MemCacheAlloc(threadCache);
	if (!ret)
		return NULL;

	ret->parent = parent;
	ret->refs = 1;
	ret->priority = PRIORITY_DEFAULT;
	ret->state = THR_PAUSED;
	ret->preemptCount = 0;

	if (parent)
		ret->id=parent->cId++;

	ThrArchCreateThread(ret, entry, user, stackP, argument);

	SpinLockIrq(&thrListLock);
	ListAddTail(&ret->list, &threadList);
	SpinUnlockIrq(&thrListLock);

	return ret;
}

SYMBOL_EXPORT(ThrCreateThread);

/***********************************************************************
 *
 * FUNCTION: ThrSetPriority
 *
 * DESCRIPTION: Sets a thread to a given priority for scheduling.
 *
 * PARAMETERS: thread - the thread in question.
 *			   priority - the thread's new priority.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void ThrSetPriority(struct Thread* thread,int priority)
{
	if (priority > PRIORITY_TOP)
		priority=PRIORITY_TOP;
	else if (priority < PRIORITY_NONE)
		priority=PRIORITY_NONE;

	thread->priority=priority;
}

SYMBOL_EXPORT(ThrSetPriority);

/***********************************************************************
 *
 * FUNCTION: _ThrFreeThread
 *
 * DESCRIPTION: Free the current thread and it's stack - stack must be
 *				switched via ArchSwitchStackCall to call this.
 *
 * PARAMETERS: thread - the current thread.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void _ThrFreeThread(struct Thread* thread)
{
	MemFree((void*)thread->currStack);
	currThread=NULL;
	MemCacheFree(threadCache, thread);
	ThrSchedule();
}

/***********************************************************************
 *
 * FUNCTION: ThrFreeThread
 *
 * DESCRIPTION: Free all resources associated with a thread. Freeing the
 *				current thread is ok at the start of ThrSchedule.
 *
 * PARAMETERS: thread - the thread in question.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void ThrFreeThread(struct Thread* thread)
{
	DWORD flags;

	if (thread == idleTask)
		KernelPanic("Trying to free idle thread");

	IrqSaveFlags(flags);

	/*
	 * Free the thread's kernel stack, because that won't be run anymore.
	 * The exception is for the current thread, but that gets freed in
	 * _ThrFreeThread, where it doesn't run anymore.
	 */

	ListRemove(&thread->list);
	--nrRunning;

	thread->currStack &= ~(ARCH_STACK_SIZE-1);
	
	/* FIXME: Free the user stack. */

	if (thread == currThread)
		ArchSwitchStackCall(exitStack+PAGE_SIZE/4, _ThrFreeThread, thread);
	else{
		MemFree((void*)(thread->currStack));
		MemCacheFree(threadCache,thread);
	}
	
	/* When thread is the current thread, doesn't get here as _ThrFreeThread reschedules */
}

SYMBOL_EXPORT(ThrFreeThread);

/***********************************************************************
 *
 * FUNCTION: _ThrStartThread
 *
 * DESCRIPTION: Gets a thread running again - adding it to be scheduled
 *			  etc.
 *
 * PARAMETERS: thread - the thread in question.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

static void _ThrStartThread(struct Thread* thread)
{
	thread->state=THR_RUNNING;
	++nrRunning;
}

/***********************************************************************
 *
 * FUNCTION: ThrStartThread
 *
 * DESCRIPTION: Serializes access to _ThrStartThread
 *
 * PARAMETERS: thread - the thread in question.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void ThrStartThread(struct Thread* thread)
{
	if (LIKELY(thread->state != THR_RUNNING))
	{
		SpinLockIrq(&thrListLock);
		_ThrStartThread(thread);
		SpinUnlockIrq(&thrListLock);
	}
}

SYMBOL_EXPORT(ThrStartThread);

/***********************************************************************
 *
 * FUNCTION: ThrDoExitThread
 *
 * DESCRIPTION: Exit the current thread in the current process.
 *
 * PARAMETERS: None.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

int ThrDoExitThread()
{
	currThread->state=THR_DYING;
	
	ThrReleaseThread(currThread); /* A waitqueue might still have a reference to this thread, so might not be freed */
	ThrArchDestroyThread(currThread);
	ThrSchedule();

	return 0;
}

SYMBOL_EXPORT(ThrDoExitThread);

/***********************************************************************
 *
 * FUNCTION:	ThrSuspendThread
 *
 * DESCRIPTION: Suspend a thread, by changing its state and reducing the
 *				number of threads running.
 *
 * PARAMETERS:	thread - the thread in question.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void ThrSuspendThread(struct Thread* thread)
{
	if (thread == idleTask)
		KernelPanic("Idle task being suspended - driver sleeping in interrupt?");

	if (LIKELY(thread->state == THR_RUNNING))
	{
		SpinLockIrq(&thrListLock);
		--nrRunning;
		
		thread->state=THR_PAUSED;
		SpinUnlockIrq(&thrListLock);
	}
}

SYMBOL_EXPORT(ThrSuspendThread);

/***********************************************************************
 *
 * FUNCTION:	ThrResumeThread
 *
 * DESCRIPTION: Resume a paused thread, and signal scheduling if it's of
 *				a higher priority.
 *
 * PARAMETERS:	thread - the thread in question.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void ThrResumeThread(struct Thread* thread)
{
	if (LIKELY(thread->state != THR_RUNNING))
	{
		ThrStartThread(thread);
			
		if (thread->quantums >= currThread->quantums)
			thrNeedSchedule=true;
	}
}

SYMBOL_EXPORT(ThrResumeThread);

/***********************************************************************
 *
 * FUNCTION:	SysCreateThread
 *
 * DESCRIPTION: Creates a new thread in the current process with the
 *				instruction pointer set to 'address'.
 *
 * PARAMETERS:	address - the starting address of the thread.
 *
 * RETURNS:		Usual error codes.
 *
 ***********************************************************************/

int SysCreateThread(DWORD address,DWORD stackP, void* argument)
{
	struct Thread* thread=ThrCreateThread(current, address, true, stackP, argument);

	if (!thread)
		return -ENOMEM;

	ThrStartThread(thread);

	if (thread->quantums > currThread->quantums)
		ThrSchedule();

	return thread->id;
}

int SysGetCurrentThreadId()
{
	return currThread->id;
}

struct Thread* ThrGetThreadById(struct Process* process, DWORD threadId)
{
	struct Thread* curr;

	ListForEachEntry(curr, &threadList, list)
	{
		if (!curr->parent)
			continue;

		if (curr->id == threadId && curr->parent == process)
			return curr;
	}

	return NULL;
}

int SysSuspendThread(DWORD threadId)
{
	struct Thread* thread;

	thread=ThrGetThreadById(current, threadId);

	if (!thread)
		return -EINVAL;

	ThrGetThread(thread);
	ThrSuspendThread(thread);
	ThrSchedule();

	ThrReleaseThread(thread);

	return 0;
}

int SysResumeThread(DWORD threadId)
{
	struct Thread* thread;

	thread=ThrGetThreadById(current, threadId);

	if (!thread)
		return -EINVAL;

	ThrResumeThread(thread);

	return 0;
}

int SysExitThread(int threadId)
{
	if (threadId == -1) /* Exit current thread. */
		ThrDoExitThread();
	else
		return -ENOTIMPL;

	return 0;
}

struct SysCall thrSysCallTable[]=
{
	SysEntry(SysCreateThread, 12),
	SysEntry(SysGetCurrentThreadId, 0),
	SysEntry(SysSuspendThread, 4),
	SysEntry(SysResumeThread, 4),
	SysEntry(SysExitThread, 4),
	SysEntryEnd()
};

int ThreadInit()
{
	threadCache=MemCacheCreate("Threads", sizeof(struct Thread), NULL, NULL, 0);
	SysRegisterRange(SYS_THREAD_BASE, thrSysCallTable);
	return 0;	
}
