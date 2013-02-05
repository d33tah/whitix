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

#include <console.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <module.h>
#include <error.h>
#include <devices/sdevice.h>
#include <task.h>
#include <fs/devfs.h>
#include <fs/exports.h>
#include <slab.h>
#include <panic.h>
#include <sys.h>
#include <print.h>
#include <sections.h>

/* The root filesystem is mounted manually (i.e. not calling VfsMount) and the current->root=vnode. */

struct Cache* sbCache;
LIST_HEAD(fsList);
Spinlock fsListLock;
LIST_HEAD(sbList);

/***********************************************************************
 *
 * FUNCTION: VfsResolveDevName
 *
 * DESCRIPTION: Resolve a device name to a storage device.
 *
 * PARAMETERS: devName - name of device
 *			   sDevice - storage device to return information to.
 *			   deviceNode -vNode to return information to.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

static int VfsResolveDevName(char* devName,struct StorageDevice** sDevice,struct VNode** deviceNode)
{
	int err;

	/* 
	 * Not necessarily an error condition - for example, the device filesystem
	 * does not require a device to mount on
	 */
	if (!devName)
		return 0;

	/* Resolve deviceName to StorageDevice* here */
	err=NameToVNode(deviceNode, devName, FILE_READ | FILE_FORCE_OPEN, NULL);
	if (err || !*deviceNode)
	{
		if (*deviceNode)
			VNodeRelease(*deviceNode);

		return err;
	}

	/* Not supporting loop devices et al yet */
	if (!((*deviceNode)->mode & VFS_ATTR_BLOCK))
	{
		VNodeRelease(*deviceNode);
		return -ENOTIMPL;
	}
		
	*sDevice=(struct StorageDevice*)DeviceGetOps(*deviceNode);
	return 0;
}

/***********************************************************************
 *
 * FUNCTION: VfsMount
 *
 * DESCRIPTION: Mounts a filesystem at a given mount point.
 *
 * PARAMETERS: deviceName - filename of the device that is to be mounted.
 *			   mountPoint - directory name of the mount point.
 *			   fsName - name of the filesystem (FAT etc).
 *			   data - data needed by the filesystem driver.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int VfsMount(char* deviceName,char* mountPoint,char* fsName,void* data)
{
	struct VNode* mountNode=NULL,*deviceNode=NULL;
	struct StorageDevice* sDevice=NULL;
	struct FileSystem* curr, *curr2;
	int err;
	struct VfsSuperBlock* superBlock=NULL;

	if (!mountPoint)
		return -EFAULT;

	/* Is the root filesystem being remounted? */
	if (strlen(mountPoint) == 1 && mountPoint[0] == '/')
	{
		KernelPanic("VfsMount: Remounting the root filesystem not yet supported");
		return -ENOTIMPL;
	}

	/* Not really anything to mount */
	if (!deviceName && !fsName)
		return -EFAULT;

	err=NameToVNode(&mountNode, mountPoint, FILE_READ | FILE_FORCE_OPEN, NULL);

	if (err)
		return err;

	if (mountNode->refs > 1 || mountNode->mountNode)
	{
		VNodeRelease(mountNode);
		return -EBUSY;
	}

	/* Check if this is the mount node of it's superblock - if so, it's busy */
	if (mountNode == mountNode->superBlock->mount)
	{
		if (mountNode)
			VNodeRelease(mountNode);
		return -EBUSY;
	}

	err=VfsResolveDevName(deviceName,&sDevice,&deviceNode);
	
	if (err)
		return err;
		
	/* Now cycle through each superblock and read */
	ListForEachEntrySafe(curr,curr2, &fsList,list)
	{
		/* If we are given a filesystem name, check that the names match */
		if (fsName)
			if (strcmp(fsName,curr->name))
				continue;

		if ((superBlock=curr->readSuper(sDevice, 0, NULL)))
			break;
	}

	if (!superBlock)
	{
		VNodeRelease(mountNode);
		VNodeRelease(deviceNode);
		return -ENOENT;
	}

	superBlock->coveredId=mountNode->id;
	superBlock->parent=mountNode->superBlock;
	mountNode->mountNode=superBlock->mount;
//	superBlock->mount->refs++; /* FIXME: Needed? */

	VNodeRelease(deviceNode);

	return 0;
}

SYMBOL_EXPORT(VfsMount);

/***********************************************************************
 *
 * FUNCTION:	SysMount
 *
 * DESCRIPTION: System call for mounting a filesystem at a given mount 
 *				point.
 *
 * PARAMETERS:	deviceName - filename of the device that is to be mounted.
 *				mountPoint - directory name of the mount point.
 *				fsName - name of the filesystem (FAT etc).
 *				data - data needed by the filesystem driver.
 *
 * RETURNS:		Usual error codes in error.h
 *
 ***********************************************************************/

int SysMount(char* deviceName,char* mountPoint,char* fsName,void* data)
{
	char* cDevName=NULL,*cMountPoint,*cFsName=NULL;
	int err=0;

	/* Copy over the pathnames */
	cMountPoint=GetName(mountPoint);

	if (deviceName)
		cDevName=GetName(deviceName);

	if (fsName)
		cFsName=GetName(fsName);

	err=PTR_ERROR(cMountPoint);

	if (cMountPoint)
	{
		err=VfsMount(cDevName,cMountPoint,cFsName,data);

		if (cFsName)
			MemFree(cFsName);

		if (cDevName)
			MemFree(cDevName);

		MemFree(cMountPoint);
	}

	return err;
}

/***********************************************************************
 *
 * FUNCTION: VfsCanUnmount
 *
 * DESCRIPTION: Can the superblock be unmounted?
 *
 * PARAMETERS: superBlock - the superblock in question.
 *
 * RETURNS: -EBUSY if it can't, 0 if it can.
 *
 ***********************************************************************/

int VfsCanUnmount(struct VfsSuperBlock* superBlock)
{
	struct VNode* vNode;

	if (superBlock->mount->refs > 2)
		return -EBUSY;


	ListForEachEntry(vNode, &superBlock->vNodeList, next)
	{
		/* Only the superblock's 'root' is allowed to have a reference */
		if (vNode->refs && vNode != superBlock->mount)
			return -EBUSY;
	}

	return 0;
}

int VfsUnmount(char* mountPoint)
{
	struct VNode* vNode,*coveredNode;
	struct VfsSuperBlock* superBlock;
	int err;

	err=NameToVNode(&vNode, mountPoint, FILE_READ | FILE_FORCE_OPEN, NULL);

	if (err)
		return err;

	/* You can't unmount the root filesystem - you can remount it read-only however */
	if (vNode == current->root)
		return -EPERM;

	superBlock=vNode->superBlock;

	if (VfsCanUnmount(superBlock))
		return -EBUSY;

	/* The mount point given is not the path of the mount point of the super block */
	if (superBlock->mount != vNode)
		return -EPERM;

	/* Get the covered vNode */
	coveredNode=VNodeGet(superBlock->parent,superBlock->coveredId);
	if (!coveredNode)
		return -EFAULT;

	coveredNode->mountNode=NULL;
	
	VNodeRelease(coveredNode);
	VNodeRelease(coveredNode); /* Reference from the mount node needs to be freed. */
	
	VNodeRelease(superBlock->mount);

	/* Write the superblock here */
	if (superBlock->flags & SB_DIRTY)
		superBlock->sbOps->writeSuper(superBlock);

	VfsFreeSuper(superBlock);
	return 0;
}

int SysUnmount(char* mountPoint)
{
	int err=0;
	char* cMountPoint;
	
	cMountPoint=GetName(mountPoint);
	err=PTR_ERROR(cMountPoint);

	if (cMountPoint)
	{
		err=VfsUnmount(cMountPoint);
		MemFree(cMountPoint);
	}

	return err;
}

int VfsRegisterFileSystem(struct FileSystem* fs)
{
	struct FileSystem* curr;
	
	SpinLock(&fsListLock);
	
	ListForEachEntry(curr,&fsList,list)
		if (curr == fs)
		{
			SpinUnlock(&fsListLock);
			return -EEXIST;
		}
		
	/* Just add to the main filesystem list */
	ListAddTail(&fs->list, &fsList);

	SpinUnlock(&fsListLock);

	return 0;
}

SYMBOL_EXPORT(VfsRegisterFileSystem);

int VfsDeregisterFileSystem(struct FileSystem* fs)
{
	ListRemove(&fs->list);
	return 0;
}

SYMBOL_EXPORT(VfsDeregisterFileSystem);

/***********************************************************************
 *
 * FUNCTION: VfsMountRoot
 *
 * DESCRIPTION: Mount the root filesystem.
 *
 * PARAMETERS: storageDev - the storage device of the root filesystem.
 *
 * RETURNS: Error codes in error.h.
 *
 ***********************************************************************/

int VfsMountRoot(struct StorageDevice* storageDev)
{
	struct VfsSuperBlock* superBlock=NULL;
	struct FileSystem* curr;

	KePrint("VFS: Mounting the root filesystem\n");

	if (!storageDev)
		return -EFAULT;
		
	/* We won't be registering filesystems at the same time, so no need to lock. */
	
	/* No clue what filesystem the storage device is, so try all filesystems */
	ListForEachEntry(curr,&fsList,list)
	{
		if ((superBlock=curr->readSuper(storageDev,0,NULL)))
			break;
	}

	if (!superBlock)
		KernelPanic("Failed to mount the root filesystem");

	/* Allocate some basic filesystem variables. */
	current->cwd=superBlock->mount;
	current->root=superBlock->mount;
	superBlock->mount->refs+=2;
	current->files=(struct File**)MemAlloc(sizeof(struct File*)*MAX_FILES);
	superBlock->sDevice=storageDev;

	/* Allocate a current directory string for the kernel loading process */
	current->sCwd=(char*)MemAlloc(PATH_MAX);
	strncpy(current->sCwd, "/", 1);

	KePrint("VFS: Sucessfully mounted the root filesystem (%s)\n",curr->name);

	return 0;
}

SYMBOL_EXPORT(VfsMountRoot);

/***********************************************************************
 *
 * FUNCTION: VfsAllocSuper
 *
 * DESCRIPTION: Allocate a superblock.
 *
 * PARAMETERS: dev - device of the superblock.
 *			   flags - flags given to it's ReadSuper.
 *
 ***********************************************************************/

struct VfsSuperBlock* VfsAllocSuper(struct StorageDevice* dev,int flags)
{
	struct VfsSuperBlock* retVal=(struct VfsSuperBlock*)MemCacheAlloc(sbCache);

	if (!retVal)
		return NULL;

	ListAdd(&retVal->sbList,&sbList);

	/* Allow vNodes to be allocated to it */
	INIT_LIST_HEAD(&retVal->vNodeList);

	retVal->sDevice=dev;
	retVal->flags=flags;

	return retVal;
}

SYMBOL_EXPORT(VfsAllocSuper);

void VfsFreeSuper(struct VfsSuperBlock* superBlock)
{
	/* Don't free all vNodes - that should have been freed by the Unmount function,
	or it may have no vNodes except the mount vNode (in it's ReadSuper) */

	if (superBlock->sbOps->freeSuper)
	{
		superBlock->sbOps->freeSuper(superBlock);
	}else if (superBlock->privData)
		MemFree(superBlock->privData);

	ListRemove(&superBlock->sbList);
	MemCacheFree(sbCache,superBlock);
}

SYMBOL_EXPORT(VfsFreeSuper);

struct SysCall fsSystemCalls[]=
{
	SysEntry(SysOpen, 12),
	SysEntry(SysCreateDir, 8),
	SysEntry(SysClose, 4),
	SysEntry(SysRemove, 4),
	SysEntry(SysRemoveDir, 4),
	SysEntry(SysFileAccess, 8),
	SysEntry(SysFileDup, 4),
	SysEntry(SysFileSync, 4),
	SysEntry(SysFileSystemSync, 0),
	SysEntry(SysTruncate, 8),
	SysEntry(SysMove, 8),
	SysEntry(SysWrite, 12),
	SysEntry(SysRead, 12),
	SysEntry(SysSeek, 12),
	SysEntry(SysChangeDir, 4),
	SysEntry(SysChangeRoot, 4),
	SysEntry(SysMount, 16),
	SysEntry(SysUnmount, 4),
	SysEntry(SysGetDirEntries, 12),
	SysEntry(SysGetCurrDir, 8),
	SysEntry(SysStat, 12),
	SysEntry(SysStatFd, 8),
	SysEntry(SysIoCtl, 12),
	SysEntry(SysPoll, 12),
	SysEntry(SysPipe, 4),
	SysEntryEnd(),
};

initcode int VfsInit()
{
	VNodeInitCache();
	BlockInit();
	FileTableInit();

	/* Allocate a cache for superblocks */
	sbCache=MemCacheCreate("Superblocks",sizeof(struct VfsSuperBlock),NULL,NULL,0);
	if (!sbCache)
		return -ENOMEM;

	/* Register system calls for the whole virtual filesystem layer. */
	SysRegisterRange(SYS_FS_BASE, fsSystemCalls);

	return 0;
}
