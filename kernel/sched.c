/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <sched.h>
#include <error.h>
#include <i386/process.h>
#include <slab.h>
#include <typedefs.h>
#include <wait.h>
#include <malloc.h>
#include <fs/vfs.h>
#include <task.h>
#include <panic.h>
#include <vmm.h>
#include <module.h>
#include <sys.h>

extern struct ListHead threadList;

volatile int thrNeedSchedule=false;
struct Thread* prevThread,*currThread;

SYMBOL_EXPORT(currThread);

struct Process* current,*prev;
int nrRunning; /* nrRunning is the number of threads running */

SYMBOL_EXPORT(current);

/***********************************************************************
 *
 * FUNCTION: ThrIdleFunc
 *
 * DESCRIPTION: The CPU idles on this function if there is nothing else
 *				to schedule.
 *
 * PARAMETERS: None.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void ThrIdleFunc()
{
	/* Setting to lowest priority means that any thread woken up will preempt this one */
	ThrSetPriority(currThread,PRIORITY_NONE);

	/* TODO: Zero out pages while there's nothing to do? */
	while (1)
	{
		sti();
		hlt();
	}
}

/***********************************************************************
 *
 * FUNCTION:	ThrChooseNewThread
 *
 * DESCRIPTION: Go through all running threads and calculate the best
 *				thread to run. This is the core of the scheduling
 *				algorithm. This will choose the idle thread (idleTask)
 *				if there are no threads running.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		The next thread to run.
 *
 ***********************************************************************/

struct Thread* ThrChooseNewThread()
{
	int c = -1;
	struct Thread* thread=idleTask,*curr;

	ListForEachEntry(curr,&threadList,list)
	{
		if (curr->quantums > c && curr->state == THR_RUNNING)
		{
			c=curr->quantums;
			thread=curr;
		}
	}
		
	/* Recalculate quantums - all threads have no time left to run */
	if (!c)
	{
		ListForEachEntry(curr,&threadList,list)
			curr->quantums=curr->priority + (curr->quantums >> 2);
	}

	return thread;
}

/***********************************************************************
 *
 * FUNCTION:	ThrSchedule
 *
 * DESCRIPTION: Schedules another thread to run if required.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void ThrSchedule()
{
	DWORD flags;
	IrqSaveFlags(flags);

	thrNeedSchedule=false;

	/* See what's happening to the current thread - does it want to be freed? */
	if (currThread && currThread->state == THR_DYING)
		ThrFreeThread(currThread);

	/* Cannot depend on currThread actually being valid, as it may have been freed
 	 * in ThrFreeThread. */ 

	prevThread=currThread;
	prev=current;

	currThread=ThrChooseNewThread();
	current=currThread->parent;

	/* The only threads without a parent process are kernel threads - who
	 * don't need their own address space as they only manipulate kernel
	 * memory, which is present in every address space.
	 */

	if (current)
	{
		if (UNLIKELY(!prevThread))
			/* If there is no previous thread it has probably been freed in
			 * _ThrFreeThread - and so the virtual address space must be
			 * switched as soon as possible.
			 */
			VirtSetCurrent(current->memManager);
		else if (current != prev)
			/* 
			 * If the current and previous threads have the same address space, 
			 * there's no point switching, since the only thing it'll do is
			 * flush the TLBs.
			 */
			VirtSetCurrent(current->memManager);
	}else if (UNLIKELY(!prevThread))
		VirtSetCurrent(&kernelMem);

	/* If we're actually switching, call the architecture-specific code to switch
	 * contexts.
	 */
	if (prevThread != currThread)
		ThrArchSwitch(prevThread, currThread);

	IrqRestoreFlags(flags);
}

SYMBOL_EXPORT(ThrSchedule);

/***********************************************************************
 *
 * FUNCTION:	SysYield
 *
 * DESCRIPTION: Give up the current thread's timeslice and schedule 
 *				immediately.
 *
 * PARAMETERS:	None.
 *
 ***********************************************************************/

int SysYield()
{
	currThread->quantums=0;
	thrNeedSchedule=true;
	return 0;
}

/* TODO: Send a kill signal once signals are implemented. */
void ThrEndWait(struct Thread* thread)
{
	if (thread->parent && thread->parent->state == THR_DYING)
	{
		/* Kill time until process exits. */
		while (1)
			SysYield();
	}
}

SYMBOL_EXPORT(ThrEndWait);

/***********************************************************************
 *
 * FUNCTION:	ThrInit
 *
 * DESCRIPTION: Sets up the architecture specifics, caches and creates
 *				the idle thread.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Usual error codes.
 *
 ***********************************************************************/

int ThrEarlyInit()
{
	/* Set up arch-specific data structures for multitasking here */
	ThrArchInit();

	currThread=idleTask;
	currThread->state=THR_RUNNING;
	currThread->refs=1;

	nrRunning=0; /* The idle task isn't treated as a normal thread. */

	return 0;
}

struct SysCall schedSysCall=SysEntry(SysYield, 0);

extern int ThreadInit();
extern int ProcessInit();

int ThrInit()
{
	ThreadInit();
	ProcessInit();

	/* Register the scheduler system calls. */
	SysRegisterCall(SYS_SCHED_BASE, &schedSysCall);

	return 0;
}
