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

#include <fs/vfs.h>
#include <malloc.h>
#include <module.h>
#include <slab.h>
#include <task.h>
#include <wait.h>
#include <timer.h>
#include <print.h>
#include <user_acc.h>

int PollAddWait(struct PollQueue* pollQueue, WaitQueue* newQueue)
{
	DWORD flags;
	IrqSaveFlags(flags);

	ListRemove(&pollQueue->waitEntries[pollQueue->i].next);

	/* Add the current thread to the wait queue. */
	WaitPrepareWait(newQueue, &pollQueue->waitEntries[pollQueue->i]);

	/* And add the wait queue to the poll wait queues. */
	pollQueue->waitQueues[pollQueue->i++]=newQueue;

	IrqRestoreFlags(flags);

	return 0;
}

SYMBOL_EXPORT(PollAddWait);

int VfsPollFd(int fd, struct PollItem* pollItem, struct PollQueue* waitQueues)
{
	struct File* file;

	file = FileGet(fd);
	
	if (!file || !file->fileOps || !file->fileOps->poll)
		return -EINVAL;
		
	return file->fileOps->poll(file, pollItem, waitQueues);
}

int SysPoll(void* fds, DWORD numFds, int timeout)
{
	struct PollItem* pollArray=(struct PollItem*)fds;
	DWORD i;
	int changed=0;
	DWORD flags;
	struct PollQueue pollQueue;

	/* Check pollArray is okay to access. */
	if (VirtCheckArea(pollArray, numFds*sizeof(struct PollItem), VER_READ | VER_WRITE))
		return -EFAULT;

	/* Preliminary check to ensure that all file descriptors are
	 * okay to access.
	 */
	for (i=0; i<numFds; i++)
	{
		if (!FileGet(pollArray[i].fd))
			return -EBADF;
	}

	/* Set up the waitqueues. */
	pollQueue.numFds=numFds;
	pollQueue.waitQueues=(WaitQueue**)MemAlloc(numFds*sizeof(WaitQueue*));
	ZeroMemory(pollQueue.waitQueues, numFds * sizeof(WaitQueue*));
	pollQueue.waitEntries=(struct WaitQueueEntry*)MemAlloc(sizeof(struct WaitQueueEntry) * numFds);
	
	/* and the waitqueue entries. */
	for (i=0; i<numFds; i++)
	{
		pollQueue.waitEntries[i].thread=currThread;
		INIT_LIST_HEAD(&pollQueue.waitEntries[i].next);
	}

	while (1)
	{
		/* Poll each file descriptor, then go to sleep and
		 * wait until one wakes up. */
		pollQueue.i = 0;

		for (i=0; i<numFds; i++)
		{
			pollArray[i].revents = 0;

			VfsPollFd(pollArray[i].fd, &pollArray[i], &pollQueue);

			pollArray[i].revents &= (pollArray[i].events & POLL_EVENTS_MASK) | POLL_ERR;

			if (pollArray[i].revents > 0)
				changed++;
		}

		if (changed || !timeout)
			break;
						
		/* Just wait until someone wakes us up. */
		if (timeout > 0)
		{
			Sleep(timeout);
		}else{
			ThrSuspendThread(currThread);
			ThrSchedule();
		}
		
		timeout=0;
	}
	
	/* Clean up the poll waitqueues. */
	for (i=0; i<numFds; i++)
		if (pollQueue.waitQueues[i])
			WaitFinishWait(pollQueue.waitQueues[i], &pollQueue.waitEntries[i]);

	IrqSaveFlags(flags);

	MemFree(pollQueue.waitEntries);
	MemFree(pollQueue.waitQueues);
	
	IrqRestoreFlags(flags);

	return changed;
}
