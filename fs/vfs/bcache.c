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

#include <typedefs.h>
#include <malloc.h>
#include <fs/vfs.h>
#include <error.h>
#include <slab.h>
#include <llist.h>
#include <sched.h>
#include <wait.h>
#include <task.h>
#include <module.h>
#include <panic.h>
#include <print.h>
#include <locks.h>
#include <request.h>
#include <devices/sdevice.h>
#include <devices/class.h>

/* Speed this all up. It's quite a dumb cache at the moment. Readahead? */

struct Cache* blockCache;
struct Cache* requestCache;
struct Thread* bufferFlusher=NULL;
DWORD cacheSize=0;
extern struct DevClass storageClass;

static void _BufferWait(struct Buffer* buffer);
static void BlockAddToHashTable(struct Buffer* buffer);
static struct Buffer* BlockFindBuffer(struct StorageDevice* device, DWORD blockNum);

#define CACHE_MAX_SIZE 2048

/* Hashing of block number */
#define HASH_TABLE_ORDER	16 		/* 1 << 16 = 65536 = 64K */
#define HASH_NUM_ENTRIES	((1 << HASH_TABLE_ORDER)/sizeof(struct ListHead))
#define HASH_MASK			(HASH_NUM_ENTRIES-1)
#define HASH_SHIFT			26

/***********************************************************************
 *
 * FUNCTION:	BufferLock
 *
 * DESCRIPTION: Lock a buffer for unique access. Waits for the buffer to
 * 				be unlocked, and then locks it again.
 *
 * PARAMETERS:	buffer - buffer in question.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void BufferLock(struct Buffer* buffer)
{
	BufferGet(buffer);

	while (BitTestAndSet(&buffer->flags, BUFFER_LOCKED))
		_BufferWait(buffer);
}

SYMBOL_EXPORT(BufferLock);

/***********************************************************************
 *
 * FUNCTION:	BufferUnlock
 *
 * DESCRIPTION: Unlock a buffer, allowing others to access.
 *
 * PARAMETERS:	buffer - buffer in question.
 *
 * RETURNS:	Nothing.
 *
 ***********************************************************************/

void BufferUnlock(struct Buffer* buffer)
{
	BitClear(&buffer->flags, BUFFER_LOCKED);
	
	/* WakeUp one? */
	WakeUpAll(&buffer->waitQueue);
	
	BufferRelease(buffer);
}

SYMBOL_EXPORT(BufferUnlock);

static void _BufferWait(struct Buffer* buffer)
{
	BufferGet(buffer);
			
	WAIT_ON(&buffer->waitQueue, !BufferLocked(buffer));

	BufferRelease(buffer);
}

/***********************************************************************
 *
 * FUNCTION:	WaitForBuffer
 *
 * DESCRIPTION: Wait for a buffer to become unlocked.
 *
 * PARAMETERS:	buffer - buffer in question.
 *
 * RETURNS:	Nothing.
 *
 ***********************************************************************/

void WaitForBuffer(struct Buffer* buffer)
{
	if (BufferLocked(buffer))
		_BufferWait(buffer);
}

SYMBOL_EXPORT(WaitForBuffer);

/***********************************************************************
 *
 * FUNCTION:	BlockBufferAlloc
 *
 * DESCRIPTION: Allocate a new buffer.
 *
 * PARAMETERS:	sDevice - storage device that contains block.
 *				blockNum - block number of the buffer.
 *
 * RETURNS:		The new buffer.
 *
 ***********************************************************************/

struct Buffer* BlockBufferAlloc(struct StorageDevice* device,DWORD blockNum)
{
	struct Buffer* buff;

	buff=(struct Buffer*)MemCacheAlloc(blockCache);
	if (!buff)
		return NULL;

	buff->device=device;
	buff->blockNum=blockNum;
	buff->flags = 0;
	buff->refs=1;
	buff->data=(BYTE*)MemAlloc(device->softBlockSize);
	if (!buff->data)
	{
		MemCacheFree(blockCache, buff);
		return NULL;
	}

	INIT_WAITQUEUE_HEAD(&buff->waitQueue);

	BufferLock(buff);

	BlockAddToHashTable(buff);

	return buff;
}

SYMBOL_EXPORT(BlockBufferAlloc);

/***********************************************************************
 *
 * FUNCTION:	BlockBufferRemove
 *
 * DESCRIPTION: Remove a buffer from all lists and free it.
 *
 * PARAMETERS:	buff - buffer in question.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

static void BlockBufferRemove(struct Buffer* buff)
{
	DWORD flags;
	
	MemFree(buff->data);
	
	IrqSaveFlags(flags);
	ListRemove(&buff->list);
	IrqRestoreFlags(flags);
	
	MemCacheFree(blockCache, buff);
}

/***********************************************************************
 *
 * FUNCTION:	BlockWaitForRequest
 *
 * DESCRIPTION: Wait for a request to complete.
 *
 * PARAMETERS:	request - the request to wait for.
 *
 * RETURNS:		The I/O status of the finished request.
 *
 ***********************************************************************/

static int BlockWaitForRequest(struct Request* request)
{
	/* Sleep on the request wait queue, which wakes up all waiters when EndRequest is called */
	WaitForBuffer(request->buffer);
	
	return request->ioStatus;
}

/***********************************************************************
 *
 * FUNCTION:	BlockSendRequestRaw
 *
 * DESCRIPTION: Actaully send request and wait for it.
 *
 * PARAMETERS:	device - device to send request to
 *				buff - buffer with block number etc.
 *				type - read or write request?
 *
 * RETURNS:		The I/O status of the finished request.
 *
 ***********************************************************************/

int BlockSendRequestRaw(struct StorageDevice* device, struct Request* request)
{
	int err;

	err=StorageDoRequest(device,request);

	if (LIKELY(err == -SIOPENDING))
		err = BlockWaitForRequest(request);
	else if (err < 0)
	{
		/* I/O error */
		KePrint("BlockSendRequest: error (%d) reading %u\n", err, request->sector);
		err=-EIO;
	}

	return err;
}

SYMBOL_EXPORT(BlockSendRequestRaw);

/***********************************************************************
 *
 * FUNCTION:	BlockSendRequest
 *
 * DESCRIPTION: Construct and send the request to BlockSendRequestRaw
 *
 * PARAMETERS:	device - device to send request to
 *				buff - buffer with block number etc.
 *				type - read or write request?
 *
 * RETURNS:		The I/O status of the finished request.
 *
 ***********************************************************************/

int BlockSendRequest(struct StorageDevice* device,struct Buffer* buff, int type)
{
	struct Request* request;
	int err;

	request=StorageBuildRequest(buff, type);

	if (!request)
	{
		KePrint(KERN_ERROR "BlockSendRequest: could not build request for %u\n", buff->blockNum);
		return -EIO;
	}

	/* Buffer should already be locked if we're coming from BlockRead */

	err = BlockSendRequestRaw(device, request);

	MemCacheFree(requestCache, request);

	return err;
}

static inline int BlockBufferRead(struct StorageDevice* device,struct Buffer* buff)
{
	return BlockSendRequest(device,buff, REQUEST_READ);
}

struct Buffer* BlockRead(struct StorageDevice* device, DWORD blockNum)
{
	struct Buffer* buff;

	/* Exit early if buffer is already in memory */
	if ((buff=BlockFindBuffer(device,blockNum)))
		return buff;

	if (!(buff=BlockBufferAlloc(device, blockNum)))
		return NULL;
		
	if (BlockBufferRead(device,buff))
	{
		BlockBufferRemove(buff);
		return NULL;
	}

	return buff;
}

SYMBOL_EXPORT(BlockRead);

void BlockReadAhead(struct StorageDevice* device, DWORD blockNum)
{
	struct Buffer* buff;
	struct Request* request;

	if (BlockFindBuffer(device, blockNum))
		return;

	if (!(buff=BlockBufferAlloc(device, blockNum)))
		return;

	request=StorageBuildRequest(buff, REQUEST_READ);

	StorageDoRequest(device,request);
}

/* TODO: device is redundant here. */

int BlockWrite(struct StorageDevice* device,struct Buffer* buffer)
{
	/* Just signal that the buffer is dirty, and let BufferFlusher write it to disk in the near future as an update */
		
	if (!BitTestAndSet(&buffer->flags, BUFFER_DIRTY))
		device->numDirty++;

	if (bufferFlusher)
		ThrResumeThread(bufferFlusher);
	
	return 0;
}

SYMBOL_EXPORT(BlockWrite);

static void BlockDeviceFreeAll(struct StorageDevice* device)
{
	struct Buffer* curr,*curr2;
	DWORD i;
	
	if (device->numDirty)
	{
		BlockSyncDevice(device);
		WAIT_ON(&device->flushDone, !device->numDirty);
	}
	
	/* If any buffers were dirty - they would have been done by BlockWrite,
	   which also writes them to disk through buffer flusher. So no need
	   to worry about writing to disk here */

	for (i=0; i<HASH_NUM_ENTRIES; i++)
	{
		struct ListHead* head=&device->hashLists[i];

		ListForEachEntrySafe(curr, curr2, head, list)
			BlockBufferRemove(curr);
	}
}

/* Set the default block size of sDevice to blockSize. This doesn't change the hardware sector size */
int BlockSetSize(struct StorageDevice* sDevice, unsigned long blockSize)
{
	if (!blockSize)
		return -EINVAL;

	/* Must be a power of 2 */
	if (blockSize & (blockSize-1))
		return -EINVAL;

	if (blockSize < sDevice->blockSize)
		return -EINVAL; /* Can't have a sector size smaller than the hardware size */

	if (sDevice->softBlockSize == blockSize)
		return 0;
		
	/* Sync all buffers with the old size */
	BlockDeviceFreeAll(sDevice);

	sDevice->softBlockSize = blockSize;
		
	return 0;
}

SYMBOL_EXPORT(BlockSetSize);

int DoBlockWrite(struct StorageDevice* sDevice,struct Buffer* buffer);

int BlockSyncDevice(struct StorageDevice* device)
{
	DWORD i;
	struct Buffer* buff, *buff2;

//	KePrint("BlockSyncDevice(%s), numDirty = %d\n", KeObjGetName(&device->device.object), device->numDirty);

	if (device->syncing || !device->numDirty)
		return 0;
		
	device->syncing = 1;

again:
	for (i=0; i<HASH_NUM_ENTRIES; i++)
	{
		struct ListHead* head = &device->hashLists[i];

		/* Write all the blocks belonging to device to disk */
		ListForEachEntrySafe(buff, buff2, head, list)
		{
			if (BufferDirty(buff))
			{
//				KePrint("%#X: dirty(%u), %u\n", buff, buff->blockNum, device->numDirty);
				
				--device->numDirty;
				BitClear(&buff->flags, BUFFER_DIRTY);
				
				/* For each block, call the respective StorageDevice write function */
				DoBlockWrite(buff->device, buff);
				
//				KePrint("%#X: done dirty(%u), %u\n", buff, buff->blockNum, device->numDirty);
			}
			
			if (!device->numDirty)
				goto out;
		}
	}
	
out:
	if (!device->numDirty)
		WakeUpAll(&device->flushDone);
	else
		goto again;
	
	device->syncing = 0;
	return 0;
}

int BlockSyncAll()
{
	struct StorageDevice* sDevice;
	struct KeObject* object, *object2;

	ListForEachEntrySafe(object, object2, &storageClass.set.head, next)
	{
		sDevice = KeObjectToStorageDev(object);
		BlockSyncDevice(sDevice);
	}

	return 0;
}

int BlockFree(struct Buffer* buffer)
{
	buffer->refs--;

	if (buffer->refs < 0)
		KePrint("BlockFree: freeing free buffer\n");

	return 0;
}

SYMBOL_EXPORT(BlockFree);

int DoBlockWrite(struct StorageDevice* sDevice,struct Buffer* buffer)
{
	BufferLock(buffer); /* We don't want the buffer contents to change mid-write */
	return BlockSendRequest(sDevice, buffer, REQUEST_WRITE);
}

/* BufferFlusher
 * -------------
 *
 * A kernel thread that flushes buffers in memory to disk. The reason that disk writing is in a seperate thread is because
 * writing is a non-critical operation (well, until rebooting), and why should a process be paused writing stuff to disk?
 */

static void BufferFlusher()
{
	ThrSetPriority(currThread, 1);

	while (1)
	{
		ThrSuspendThread(currThread);
		ThrSchedule();
		
		BlockSyncAll();
	}
}

#define BUFFER_HASH_FUNC(blockNum) \
	(blockNum) /* Improve! */

#define BUFFER_HASH(device, blockNum) \
	(&(device)->hashLists[BUFFER_HASH_FUNC(blockNum) & HASH_MASK])

static void BlockAddToHashTable(struct Buffer* buffer)
{
	struct ListHead* head=BUFFER_HASH(buffer->device, buffer->blockNum);

	PreemptDisable();
	ListAddTail(&buffer->list, head);
	PreemptEnable();
}

int BlockCreateHashTable(struct StorageDevice* dev)
{
	DWORD i;

	dev->hashLists=(struct ListHead*)MemAlloc(1 << HASH_TABLE_ORDER);

	for (i=0; i<HASH_NUM_ENTRIES; i++)
		INIT_LIST_HEAD(&dev->hashLists[i]);

	return 0;
}

static struct Buffer* BlockFindBuffer(struct StorageDevice* device, DWORD blockNum)
{
	struct ListHead* head=BUFFER_HASH(device, blockNum);
	struct Buffer* curr;

	PreemptDisable();
	
	ListForEachEntry(curr, head, list)
		if (curr->blockNum == blockNum)
		{
			BufferGet(curr);
			PreemptEnable();
			WaitForBuffer(curr);
			return curr;
		}

	PreemptEnable();
	return NULL;
}

int BlockInit()
{
	blockCache=MemCacheCreate("Blocks",sizeof(struct Buffer),NULL,NULL, 0);
	if (!blockCache)
		return -ENOMEM;

	requestCache=MemCacheCreate("Requests",sizeof(struct Request),NULL,NULL,0);
	if (!requestCache)
		return -ENOMEM;

	/* Start a kernel thread - BufferFlusher - that regularly updates the disk */
	bufferFlusher=ThrCreateKernelThread(BufferFlusher);
	ThrStartThread(bufferFlusher);

	return 0;
}
