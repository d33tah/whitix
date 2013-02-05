/******************************************************************************
 *
 * Name: bcache.h - defines, structures and function prototypes for buffers.
 *
 ******************************************************************************/

/* 
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

#ifndef VFS_BCACHE_H
#define VFS_BCACHE_H

#include <request.h>
#include <wait.h>
#include <locks.h>

struct Request;

/* Buffer flags. Bit index into buffer->flags. */
#define BUFFER_DIRTY	0x0
#define BUFFER_LOCKED	0x1
#define BUFFER_JOURNAL	0x2

/* Defines for dealing with flags. */
#define BufferLocked(buffer) (BitTest(&(buffer)->flags, BUFFER_LOCKED))
#define BufferDirty(buffer) (BitTest(&(buffer)->flags, BUFFER_DIRTY))
#define BufferJournal(buffer) (BitTest(&(buffer)->flags, BUFFER_JOURNAL))

/* Reference counting. Blocks are only freed if there are no references
 * to them.
 */
#define BufferGet(buffer) ((buffer)->refs++)
#define BufferRelease(buffer) ((buffer)->refs--)

/* Buffer:
 * Represents a block read in from disk. data points to a malloc'ed area of
 * memory that is freed when the buffer is freed.
 */

struct Buffer
{
	struct StorageDevice* device;
	struct ListHead list;
	DWORD blockNum;
	unsigned long flags;
	int refs;
	BYTE* data;
	WaitQueue waitQueue;
	void* privData;
};

/* buffer.c */

int BlockInit();
struct Buffer* BlockBufferAlloc(struct StorageDevice* device, DWORD blockNum);
struct Buffer* BlockRead(struct StorageDevice* device,DWORD blockNum);
int BlockSendRequestRaw(struct StorageDevice* device,struct Request* request);
int BlockWrite(struct StorageDevice* device,struct Buffer* buffer);
int BlockSyncAll();
int BlockSyncDevice(struct StorageDevice* device);
int BlockFree(struct Buffer* buffer);
int BlockSetSize(struct StorageDevice* device, unsigned long blockSize);
int BlockCreateHashTable(struct StorageDevice* dev);

/* Buffer locking */
void BufferLock(struct Buffer* buffer);
void WaitForBuffer(struct Buffer* buffer);
void BufferUnlock(struct Buffer* buffer);

#endif
