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

/* FIXME: Split file into separate headers. */

#ifndef VFS_H
#define VFS_H

/* VFS includes. */

#include <llist.h>
#include <typedefs.h>
#include <types.h>
#include <time.h>
#include <wait.h>
#include <locks.h>

/* Include all virtual filesystem headers, so we don't repeat ourselves in source files. */
#include <fs/super.h>
#include <fs/bcache.h>
#include <fs/exports.h>
#include <fs/filesystems.h>
#include <fs/file_table.h>

/* vNode flags */

#define VFS_ATTR_FILE 		1
#define VFS_ATTR_DIR 		2
#define VFS_ATTR_READ 		4
#define VFS_ATTR_WRITE 		8
#define VFS_ATTR_BLOCK 		16
#define VFS_ATTR_CHAR 		32
#define VFS_ATTR_SOCKET		64
#define VFS_ATTR_SOFTLINK	128

#define VFS_ATTR_RW 		(VFS_ATTR_READ | VFS_ATTR_WRITE)

/* Flags for SysOpen and NameToVNode */

#define FILE_READ			1
#define FILE_WRITE			2
#define FILE_CREATE_OPEN	4
#define FILE_FORCE_OPEN		8
#define FILE_FORCE_CREATE	16
#define FILE_DIRECTORY		32 /* Only open the file if it is a directory */

/* General defines */
#define MAX_FILES 10 /* Max files per task */
#define PATH_MAX 2048

struct VNode;
struct File;
struct FileSystem;
struct VfsSuperBlock;
struct PollItem;
struct PollQueue;

int VfsInit();
int VfsMountRoot(struct StorageDevice* storageDev);
int VNodeInitCache();

/* VNode functions (vnode.c) */
int NameToVNode(struct VNode** retVal, char* path, int flags, struct VNode* dir);
struct VNode* DirNameVNode(char* path,char** retPath,int* retLen, struct VNode* dir);

/* VNode cache functions (vcache.c) */
struct VNode* VNodeGet(struct VfsSuperBlock* superBlock, vid_t id);
struct VNode* VNodeGetWithData(struct VfsSuperBlock* superBlock, vid_t id, void* data);
struct VNode* VNodeAlloc(struct VfsSuperBlock* superBlock, DWORD id);
struct VNode* VNodeGetEmpty();
void VNodeRelease(struct VNode* vNode);
void VNodeLock(struct VNode* vNode);
void VNodeUnlock(struct VNode* vNode);
void VNodeWrite(struct VNode* vNode);
int VNodeRead(struct VNode* vNode);
void VNodeWaitOn(struct VNode* vNode);

int VfsChangeDir(char* dirName);

/* vNode time functions */
void VfsFileAccessed(struct VNode* vNode);
void VfsFileModified(struct VNode* vNode,int create);

/* File functions (file.cpp) */
int FileGenericRead(struct File* file,BYTE* buffer,size_t size);
int FileGenericWrite(struct File* file,BYTE* buffer,size_t size);
int VfsGetFreeFd(struct Process* process);
struct File* FileGet(int fd);
int DoOpenFile(struct File* file,char* path,int flags,int perms);
int DoReadFile(struct File* file,BYTE* buffer,size_t size);
int DoWriteFile(struct File* file,BYTE* buffer,size_t size);
int DoCloseFile(struct File* file);
int DoSeekFile(struct File* file,int distance,int whence);

/* For demand loading */
int FileGenericReadPage(addr_t pageAddr,struct VNode* vNode,int offset);

char* GetName(char* path);

/* Used for the SysGetDirEntries system call */

struct DirEntry
{
	DWORD vNodeNum; /* Need this? */
	DWORD offset;
	WORD length;
	WORD type;
	char name[1];
};

/* VNODE 

Represents an on-disk directory entry in memory 

*/

/* VNode flags (bit positions) */
#define VNODE_DIRTY		0x00
#define VNODE_LOCKED	0x01

#define VNodeLocked(vNode) (BitTest(&vNode->flags, VNODE_LOCKED))

struct VNode
{
	struct VNode* mountNode;
	DWORD flags,refs,mode;
	unsigned long long size;
	vid_t id;
	struct VfsSuperBlock* superBlock;
	struct VNodeOps* vNodeOps;
	struct FileOps* fileOps;
	struct ListHead next;
	DevId devId;
	WaitQueue waitQueue;
	void* extraInfo;
	struct Time aTime,cTime,mTime;

	/* Virtual memory */
	struct ListHead sharedList;
};

static inline void SetVNodeDirty(struct VNode* vNode)
{
	BitSet(&vNode->flags, VNODE_DIRTY);
	if (vNode->superBlock && vNode->superBlock->sbOps->dirtyVNode)
		vNode->superBlock->sbOps->dirtyVNode(vNode);
}

/* FILE

File object. Used when reading/writing to a file (VNode doesn't need those
things really)

*/

#define FILE_NONBLOCK	0x1

struct File
{
	struct VNode* vNode;
	pos_t position;
	int flags, refs;
	struct FileOps* fileOps;
	void* filePriv;
};

struct FileOps
{
	int (*open)(struct File* file);
	int (*close)(struct File* file);
	int (*read)(struct File* file,BYTE* buffer,DWORD size);
	int (*write)(struct File* file,BYTE* buffer,DWORD size);
	int (*seek)(struct File* file,int pos,int whence);
	int (*readDir)(struct File* file,void* dirEntries);
	int (*ioctl)(struct File* file,unsigned long code,char* data);
	int (*poll)(struct File* file, struct PollItem* item, struct PollQueue* pollQueue);
	int (*mMap)(struct VNode* vNode, DWORD address, DWORD offset);
};

int FillDir(void* dirEntry,char* name,int nameLen,DWORD vNodeNum); 

/* Flags to pass to block map */
#define VFS_MAP_CREATE		0x1

struct VNodeOps
{
	int (*create)(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
	int (*remove)(struct VNode* dir,char* name,int nameLen);
	int (*lookup)(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
	int (*mkDir)(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
	int (*rmDir)(struct VNode* dir,char* name,int nameLength);
	int (*permission)(struct VNode* vNode,int access);
	int (*followLink)(struct VNode** retVal, struct VNode* vNode);
	int (*blockMap)(struct VNode* vNode, DWORD offset, int flags);
	int (*truncate)(struct VNode* vNode, DWORD size);
};

struct Stat
{
	unsigned long long size;
	unsigned long vNum,mode;
	unsigned long aTime,cTime,mTime;
}PACKED;

/* poll.c */
#define POLL_IN		1
#define POLL_OUT	2
#define POLL_ERR	4

#define POLL_EVENTS_MASK	0x03

struct PollQueue
{
	int i;
	int numFds;
	WaitQueue** waitQueues;
	struct WaitQueueEntry* waitEntries;
};

struct PollItem
{
	int fd;
	short events;
	short revents;
};

int PollAddWait(struct PollQueue* pollQueue, WaitQueue* waitQueue);

#endif
