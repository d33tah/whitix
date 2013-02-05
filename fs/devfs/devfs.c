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

/* devfs.c
 *
 * Implements the fully dynamic device filesystem and redirects I/O requests to the appropriate device. A fully dynamic 
 * device file-system. At startup, each device registers itself with the device file-system, and the fs keeps track of 
 * read, write and ioctl functions for each device. But this is hidden from the user, because it is mounted at DEVICES_PATH 
*/

#include <console.h>
#include <llist.h>
#include <typedefs.h>
#include <fs/vfs.h>
#include <fs/super.h>
#include <fs/devfs.h>
#include <malloc.h>
#include <module.h>
#include <error.h>
#include <fs/kfs.h>
#include <print.h>
#include <panic.h>

/* Root directory of the device filesystem. */
struct KeFsDir* rootDir;
struct KeFsEntry rootEntry;

struct SuperBlockOps devFsSbOps;
struct VNodeOps devFsVOps;
struct FileOps devFsFileOps;

int DevFsBlockRead(struct File* file,BYTE* buffer,DWORD size);
int DevFsBlockWrite(struct File* file,BYTE* buffer,DWORD size);

/* Generic operations for block devices */
struct FileOps devFsBlockOps={
	.read = DevFsBlockRead,
	.write = DevFsBlockWrite,
};

int DevFsBlockRead(struct File* file, BYTE* buffer, DWORD size)
{
	DWORD readSize,readOffset,copyOffset=0;
	struct Buffer* buff;
	struct StorageDevice* sDevice;

	/* Get it from ops */
	sDevice=(struct StorageDevice*)DeviceGetOps(file->vNode);

	while (size > 0)
	{
		readOffset=(file->position % sDevice->softBlockSize);
		readSize=MIN(sDevice->softBlockSize-readOffset,size);

		buff=BlockRead(sDevice,file->position/sDevice->softBlockSize);

		/* Might be possible, if file->position > number of sectors */
		if (!buff)
			return copyOffset;

		memcpy(buffer+copyOffset,buff->data+readOffset,readSize);
		size-=readSize;
		copyOffset+=readSize;
		file->position+=readSize;
		
		BufferRelease(buff);
	}

	return copyOffset;
}

int DevFsBlockWrite(struct File* file,BYTE* buffer,DWORD size)
{
	DWORD writeSize,writeOffset,copyOffset=0;
	struct Buffer* buff;
	struct StorageDevice* sDevice;

	/* Get it from ops */
	sDevice=(struct StorageDevice*)DeviceGetOps(file->vNode);

	while (size > 0)
	{
		writeOffset=(file->position % sDevice->softBlockSize);
		writeSize=MIN(sDevice->softBlockSize-writeOffset,size);

		buff=BlockRead(sDevice,file->position/sDevice->softBlockSize);

		/* Might be possible, if file->position > number of sectors */
		if (!buff)
			return copyOffset;

		memcpy((char*)(buff->data+writeOffset),(char*)(buffer+copyOffset),writeSize);

		BlockWrite(sDevice,buff);

		size-=writeSize;
		copyOffset+=writeSize;
		file->position+=writeSize;
	}

	return copyOffset;
}

int DevFsReadVNode(struct VNode* vNode)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(vNode->id);
	struct KeDevice* device;
	struct StorageDevice* sDevice;
	
	vNode->vNodeOps=&devFsVOps;
	vNode->mode=VFS_ATTR_READ | VFS_ATTR_WRITE;

	if (entry->type & VFS_ATTR_FILE)
	{
		vNode->mode |= VFS_ATTR_FILE;
		device=(struct KeDevice*)entry->file;
		vNode->devId = device->devId;
		
		if (device->type == DEVICE_BLOCK)
		{
			vNode->fileOps=&devFsBlockOps;
			vNode->mode |= VFS_ATTR_BLOCK;	
		
			/* Get the total size of the block device. */
			sDevice=(struct StorageDevice*)DeviceGetOps(vNode);
			vNode->size=sDevice->blockSize*sDevice->totalSectors;
		}else if (device->type == DEVICE_CHAR)
		{
			vNode->mode |= VFS_ATTR_CHAR;
			/* Just redirect to the usual char devices */
			vNode->fileOps=device->fileOps;
			vNode->size=0;
		}else
			KernelPanic("DevFs file is neither a block nor a character device");
	}else if (entry->type & VFS_ATTR_DIR)
	{
		vNode->fileOps = &devFsFileOps;
		vNode->mode |= VFS_ATTR_DIR;
		vNode->size = KeFsDirSize(&entry->dir);
	}

	return 0;
}

int DevFsAddDevice(struct KeDevice* device, const char* name)
{
	struct KeFsDir* dir;
	struct KeFsEntry* entry;
	
	dir = KeObjGetParentDevDir(&device->object);
	
	if (!dir)
		dir = rootDir;
	
	entry = KeFsAddEntry(dir, (char*)name, VFS_ATTR_RW);
	
	if (!entry)
		return -EFAULT;
		
	entry->file = device;
	entry->type |= device->type;
	
	return 0;
}

SYMBOL_EXPORT(DevFsAddDevice);

int DevFsAddDir(struct KeSet* set, char* name)
{
	struct KeFsDir* dir;
	
	dir = KeObjGetParentDevDir(&set->object);
	
	if (!dir)
		dir = rootDir;
	
	set->devDir = KeFsAddDir(dir, name);
	
	if (!set->devDir)
		return -ENOENT;
	
	return 0;
}

SYMBOL_EXPORT(DevFsAddDir);

int DevFsReadDir(struct File* file, void* dirEntries)
{
	return KeFsReadDir(file, FillDir, dirEntries);
}

int DevFsLookup(struct VNode** retVal,struct VNode* dir, char* name, int nameLength)
{
	struct KeFsEntry* entry;
	
	entry=KeFsLookup(dir, name, nameLength);
	
	if (!entry)
		return -ENOENT;
	
	*retVal=VNodeGet(dir->superBlock, (DWORD)entry);
	
	return 0;
}

struct SuperBlockOps devFsSbOps=
{
	.readVNode = DevFsReadVNode,
};

struct VNodeOps devFsVOps=
{
	.lookup=DevFsLookup,
};

/* Only for devfs directories */
struct FileOps devFsFileOps=
{
	.readDir=DevFsReadDir,
};

struct VfsSuperBlock* DevFsReadSuper(struct StorageDevice* dev,int flags,char* data)
{
	struct VfsSuperBlock* retVal;

	/* Should not be mounted on a device */
	if (dev)
		return NULL;

	retVal=VfsAllocSuper(NULL,0);
	if (!retVal)
		return NULL;

	retVal->sbOps=&devFsSbOps;
	retVal->mount=VNodeGet(retVal,(DWORD)&rootEntry);
	
	retVal->mount->mode=VFS_ATTR_DIR | VFS_ATTR_READ;
	retVal->mount->size=KeFsDirSize(rootDir);
	retVal->mount->vNodeOps=&devFsVOps;
	retVal->mount->fileOps=&devFsFileOps;

	return retVal;
}

static struct FileSystem devFileSystem=
{
	.name = "DevFs",
	.readSuper = DevFsReadSuper,
};

int DevFsInit()
{
	VfsRegisterFileSystem(&devFileSystem);

	/* Set up the root directory here, as devices will be added before ReadSuper is called. */
	KeFsInitRoot(&rootEntry);
	rootDir=&rootEntry.dir;

	return 0;
}
