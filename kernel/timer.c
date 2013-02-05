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

#include <timer.h>
#include <llist.h>
#include <console.h>
#include <module.h>
#include <error.h>
#include <task.h>
#include <malloc.h>

LIST_HEAD(timerList);
Spinlock timerLock;

/***********************************************************************
 *
 * FUNCTION: TimerAdd
 *
 * DESCRIPTION: Add a timer to the list.
 *
 * PARAMETERS: timer - timer in question.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

void TimerAdd(struct Timer* timer)
{
	SpinLockIrq(&timerLock);
	struct Timer* curr, *curr2;

	if (ListEmpty(&timerList))
		ListAdd(&timer->list, &timerList);
	else{
		ListForEachEntrySafe(curr, curr2, &timerList,list)
		{
			if (curr->expires >= timer->expires)
			{
				if (curr->list.prev != &timerList)
					timer->expires-=ListEntry(curr->list.prev,struct Timer,list)->expires;

				curr->expires -= timer->expires;

				/* Add before the current timer. */
				DoListAdd(&timer->list,curr->list.prev,&curr->list);
				goto out;
			}
		}

		/* Add to end of list */
		if (curr->list.prev != &timerList)
			timer->expires-=ListEntry(curr->list.prev,struct Timer,list)->expires;

		ListAddTail(&timer->list, &timerList);
	}

out:
	SpinUnlockIrq(&timerLock);
}

SYMBOL_EXPORT(TimerAdd);

/***********************************************************************
 *
 * FUNCTION: TimerRemove
 *
 * DESCRIPTION: Remove a timer and update other timers.
 *
 * PARAMETERS: timer - timer in question.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

int TimerRemove(struct Timer* timer)
{
	int err=0;

	SpinLockIrq(&timerLock);

	if (timer->list.next == NULL || timer->list.prev == NULL)
		goto out;

	if (ListEmpty(&timerList))
		err=-EINVAL;
	else{
		struct Timer* curr, *curr2;
				
		ListForEachEntrySafe(curr, curr2, &timerList, list)
		{
			if (curr == timer)
			{
				ListRemove(&curr->list);
				timer->list.prev = timer->list.next = NULL;

				if (curr->expires > 0 && &curr2->list != &timerList /* TODO: Check */)
					curr2->expires += curr->expires;
			
				goto out;
			}
		}
	}

out:
	SpinUnlockIrq(&timerLock);

	return err;
}

SYMBOL_EXPORT(TimerRemove);

void TimerThreadExit(struct Thread* thread)
{
	struct Timer* curr, *curr2;
	
	ListForEachEntrySafe(curr, curr2, &timerList, list)
	{
		if (curr->owner == thread)
			TimerRemove(curr);
	}
}

/***********************************************************************
 *
 * FUNCTION: UpdateTimers
 *
 * DESCRIPTION: Update the list of timers.
 *
 * PARAMETERS: None.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

/* TODO: Does it really need to run in an IRQ? */

void UpdateTimers()
{
	struct Timer* timer;

	if (ListEmpty(&timerList))
		return;
	
	/* Get head of the list */
	timer=ListEntry(timerList.next, struct Timer, list);
	timer->expires-=10; /* The timeslice */

	if (timer->expires <= 0)
	{
		ListRemove(&timer->list);
		timer->list.prev = timer->list.next = NULL;
	
		timer->func(timer->data);
	}
}

void SleepWakeup(void* data)
{
	struct Thread* thread=(struct Thread*)data;
	
	ThrResumeThread(thread);
}

SYMBOL_EXPORT(SleepWakeup);

void Sleep(int milliSeconds)
{
	struct Timer timer;

	timer.func = SleepWakeup;
	timer.expires = milliSeconds;
	timer.data = currThread;
	timer.owner = currThread;
	
	TimerAdd(&timer);
	ThrSuspendThread(currThread);
	ThrSchedule();
	
	TimerRemove(&timer);
}

SYMBOL_EXPORT(Sleep);
