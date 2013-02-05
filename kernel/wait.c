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

#include <module.h>
#include <sched.h>
#include <task.h>
#include <wait.h>

/***********************************************************************
 *
 * FUNCTION: WakeUpCommon
 *
 * DESCRIPTION: Wake up all threads waiting on a waitqueue.
 *
 * PARAMETERS: waitQueue - wait queue in question.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void WakeUpCommon(WaitQueue* waitQueue, int number)
{
	struct WaitQueueEntry* curr, *curr2;

	/* No point if it's empty */
	if (ListEmpty(&waitQueue->list))
		return;

	ListForEachEntrySafe(curr, curr2, &waitQueue->list, next)
	{
		ListRemoveInit(&curr->next);
		ThrResumeThread(curr->thread);

		if (!--number)
			return;
	}
}

SYMBOL_EXPORT(WakeUpCommon);

void _WakeUp(WaitQueue* waitQueue, int number)
{
	DWORD flags;
	IrqSaveFlags(flags);
	WakeUpCommon(waitQueue, number);
	IrqRestoreFlags(flags);
}

SYMBOL_EXPORT(_WakeUp);

void WaitPrepareWait(WaitQueue* waitQueue, struct WaitQueueEntry* waitEntry)
{
	SpinLockIrq(&waitQueue->spinLock);
		
	if (ListEmpty(&waitEntry->next))
		WaitAddToQueue(waitQueue, waitEntry);
	
	SpinUnlockIrq(&waitQueue->spinLock);
}

SYMBOL_EXPORT(WaitPrepareWait);

void ThrEndWait(struct Thread* thread);

void WaitFinishWait(WaitQueue* waitQueue, struct WaitQueueEntry* waitEntry)
{
	if (!ListEmpty(&waitEntry->next))
	{
		SpinLockIrq(&waitQueue->spinLock);
		ListRemoveInit(&waitEntry->next);
		SpinUnlockIrq(&waitQueue->spinLock);
	}
	
	ThrEndWait(currThread);
}

SYMBOL_EXPORT(WaitFinishWait);
