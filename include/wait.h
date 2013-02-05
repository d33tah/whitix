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

#ifndef WAIT_H
#define WAIT_H

#include <llist.h>
#include <spinlock.h>
#include <sched.h>
#include <typedefs.h>

/* Uncomment this if you'd like to investigate strange wait-queue behaviour. */

struct WaitQueueTag
{
	struct ListHead list;
	Spinlock spinLock;
};

typedef struct WaitQueueTag WaitQueue;

#define DECLARE_WAITQUEUE_HEAD(waitQueue) \
	WaitQueue waitQueue={LIST_HEAD_INIT((waitQueue).list),{0}}

#define INIT_WAITQUEUE_HEAD(waitQueue) \
	do { INIT_LIST_HEAD(&((waitQueue)->list)); (waitQueue)->spinLock.flags=0; } while (0)

struct WaitQueueEntry
{
	struct Thread* thread;
	struct ListHead next;

#ifdef WAIT_DEBUG
	unsigned int magic;
#endif
};

#define WAIT_DEFINE(name) \
	struct WaitQueueEntry name = { \
		.thread = currThread, \
		.next = LIST_HEAD_INIT((name).next) \
	}

#define PERFORM_WAIT_ON(waitQueue, condition) \
do{ \
	WAIT_DEFINE(waitEntry); \
	ThrGetThread(currThread); \
	while (1) { \
		WaitPrepareWait(waitQueue, &waitEntry); \
		if ((condition)) break; \
		ThrSuspendThread(currThread); \
		ThrSchedule(); /* Will return here when the thread is scheduled again */ \
	} \
	WaitFinishWait(waitQueue, &waitEntry); \
	ThrReleaseThread(currThread); \
}while(0)

#define WAIT_ON(waitQueue, condition) \
do { \
	if (!(condition)) \
		PERFORM_WAIT_ON(waitQueue, condition); \
}while(0)

#define SLEEP_ON(waitQueue) \
do { \
	WAIT_DEFINE(waitEntry); \
	ThrGetThread(currThread); \
	WaitPrepareWait(waitQueue, &waitEntry); \
	ThrSuspendThread(currThread); \
	ThrSchedule(); \
	WaitFinishWait(waitQueue, &waitEntry); \
	ThrReleaseThread(currThread); \
}while(0)

void WakeUpCommon(WaitQueue* waitQueue, int number);
void _WakeUp(WaitQueue* waitQueue, int number);

#define WakeUp(waitQueue) _WakeUp(waitQueue, 1)
#define WakeUpAll(waitQueue) _WakeUp(waitQueue, 0)

/* Internal wait functions. */
void WaitPrepareWait(WaitQueue* waitQueue, struct WaitQueueEntry* waitEntry);
void WaitFinishWait(WaitQueue* waitQueue, struct WaitQueueEntry* waitEntry);

/* Inline functions. */

static inline void WaitAddToQueue(WaitQueue* waitQueue, struct WaitQueueEntry* waitEntry)
{
	ListAddTail(&waitEntry->next, &waitQueue->list);
}

#endif
