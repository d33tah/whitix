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
#include <fs/file_table.h>
#include <error.h>
#include <task.h>
#include <user_acc.h>
#include <malloc.h>
#include <vmm.h>
#include <print.h>
#include <module.h>
#include <devices/sdevice.h>
#include <spinlock.h>

/***********************************************************************
 *
 * FUNCTION: 	VfsFileAccessed
 *
 * DESCRIPTION: The vNode was accessed in some transaction, so update
 *				time-related information and dirty the vNode, so it
 *				gets updated to disk as soon as possible.
 *
 * PARAMETERS: 	vNode - vNode to update.
 *
 * RETURNS: 	Nothing.
 *
 ***********************************************************************/

void VfsFileAccessed(struct VNode* vNode)
{
	/* Don't dirty read-only vNodes, as they won't
	 * be written to disk anyway */
	if (!(vNode->mode & VFS_ATTR_WRITE))
		return;

	/* TODO: Check granularity of vNode times on disk. No point dirtying a vNode if
	 * the time isn't updated. */

	if (vNode->aTime.seconds < currTime.seconds)
	{
		vNode->aTime=currTime;
		SetVNodeDirty(vNode);
	}
}

/***********************************************************************
 *
 * FUNCTION: 	VfsFileModified
 *
 * DESCRIPTION: The vNode was modified in some transaction, so update
 *				time-related information and dirty the vNode, so it
 *				gets updated to disk as soon as possible.
 *
 * PARAMETERS: 	vNode - vNode to update.
 *			   	create - whether the vNode was just created or not.
 *
 * RETURNS: 	Nothing.
 *
 ***********************************************************************/

void VfsFileModified(struct VNode* vNode, int create)
{
	/* If the vNode can't be written to, it shouldn't have been modified in the
	 * first place. */
	if (!(vNode->mode & VFS_ATTR_WRITE))
		return;

	/* Has the vNode just been created? If so, record its
	creation time */
	if (create)
		vNode->cTime=currTime;

	/* See VfsFileAccessed comment. */
	if (vNode->mTime.seconds < currTime.seconds || create)
	{
		vNode->mTime=currTime;
		SetVNodeDirty(vNode);
	}
}

/***********************************************************************
 *
 * FUNCTION: FileGenericRead
 *
 * DESCRIPTION: Generic routine for reading from a storage device using
 *				the block map function.
 *
 * PARAMETERS: file - file to read from.
 *			   buffer - buffer to read into.
 *			   size - size of buffer to read into.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int FileGenericRead(struct File* file,BYTE* buffer,DWORD size)
{
	DWORD readSize,readOffset;
	int copyOffset=0;
	struct Buffer* buff;
	int res;
	int bytesPerSec=BYTES_PER_SECTOR(file->vNode->superBlock);
	struct VNode* vNode=file->vNode;

	/* Limit the read size to the end of the file */
	if (size > (vNode->size - file->position))
		size=vNode->size - file->position;

	/* Is it the EOF? */
	if (file->position >= vNode->size)
		return 0;

	while (size > 0)
	{
		readOffset=(file->position % bytesPerSec);
		readSize=MIN(bytesPerSec-readOffset,size);

		res=file->vNode->vNodeOps->blockMap(vNode,file->position / bytesPerSec, 0);

		if (res == -1)
		{
			KePrint("FileGenericRead: block map failed, file->position = %d, file->size = %d, file->id = %#X\n",
				file->position,file->vNode->size,file->vNode->id);
			return -EIO;
		}
		
		buff=BlockRead(vNode->superBlock->sDevice, res);

		if (!buff)
		{
			KePrint("Failed to read in FileGenericRead!\n");
			return -EIO;
		}

		memcpy((char*)(buffer+copyOffset),(char*)(buff->data+readOffset),readSize);
		size-=readSize;
		copyOffset+=readSize;
		file->position+=readSize;

//		BlockFree(buff);
	}

#if 0
	/* Read in the next few sectors, as part of read-ahead */
	readOffset=((file->position + bytesPerSec-1) & ~(bytesPerSec-1));
	readOffset /= bytesPerSec;

	res=file->vNode->vNodeOps->blockMap(vNode, readOffset, 0);

	if (res > 0)
	{
		BlockReadAhead(vNode->superBlock->sDevice, res);

		res=file->vNode->vNodeOps->blockMap(vNode, readOffset+1, 0);

		if (res > 0)
			BlockReadAhead(vNode->superBlock->sDevice, res);
	}
#endif

	return copyOffset;
}

SYMBOL_EXPORT(FileGenericRead);

int FileGenericWrite(struct File* file,BYTE* buffer,DWORD size)
{
	DWORD writeSize,writeOffset,copyOffset=0;
	struct Buffer* buff;
	int res;

	while (size > 0)
	{
		res=file->vNode->vNodeOps->blockMap(file->vNode, 
			file->position/BYTES_PER_SECTOR(file->vNode->superBlock), VFS_MAP_CREATE);

		/* Could not find block? Deal with this better. */
		if (res < 0)
			return -EIO;

		writeOffset=(file->position % BYTES_PER_SECTOR(file->vNode->superBlock));
		writeSize=MIN(BYTES_PER_SECTOR(file->vNode->superBlock)-writeOffset,size);

		/* Probably read in already, but we must get the data */
		buff=BlockRead(file->vNode->superBlock->sDevice, res);

		if (!buff)
		{
			KePrint("FileGenericWrite: failed to read sector %d\n",res);
			return -EIO;
		}

		memcpy((char*)(buff->data+writeOffset),(char*)(buffer+copyOffset),writeSize);
		BlockWrite(file->vNode->superBlock->sDevice, buff);

		copyOffset+=writeSize;
		size-=writeSize;
		file->position+=writeSize;

		BlockFree(buff);

		if (file->position > file->vNode->size)
			file->vNode->size=file->position;
	}

	/* The file's size needs to be updated */
	SetVNodeDirty(file->vNode);

	return copyOffset;
}

SYMBOL_EXPORT(FileGenericWrite);

/***********************************************************************
 *
 * FUNCTION: DoOpenFile
 *
 * DESCRIPTION: Return a file corresponding to a filename on the FS.
 *
 * PARAMETERS: file - file structure to put information into.
 *			   path - path of file on filesystem.
 *			   flags - flags to pass to NameToVNode.
 *			   perms - permissions for file opening.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int DoOpenFile(struct File* file,char* path,int flags,int perms)
{
	struct VNode* vNode;
	int err;

	if (!file)
		return -EFAULT;

	err=NameToVNode(&vNode, path, flags, NULL);
	
	if (err || !vNode)
	{
		/* Could not find the file */
		file->vNode=NULL;
		return err;
	}

	file->vNode = vNode;
	file->position = 0;
	file->refs = 1;
	file->fileOps = file->vNode->fileOps;

	if (file->fileOps && file->fileOps->open)
	{
		err = file->fileOps->open(file);
		
		if (err)
		{
			VNodeRelease(vNode);
			return err;
		}
	}

	return 0;
}

SYMBOL_EXPORT(DoOpenFile);

int DoReadFile(struct File* file,BYTE* buffer,DWORD size)
{
	int err;

	if (!size)
		return 0;

	/* Only files can be read */
	if (!(file->vNode->mode & VFS_ATTR_FILE))
		return -EINVAL;

	if (file->fileOps && file->fileOps->read)
	{
		err=file->fileOps->read(file, buffer, size);
		
		VfsFileAccessed(file->vNode);
		
		return err;
	}

	return -ENOTIMPL;
}

SYMBOL_EXPORT(DoReadFile);

int DoWriteFile(struct File* file, BYTE* buffer, DWORD size)
{
	int err;

	if (!size)
		return 0;

	if (!file || !file->vNode || !buffer)
		return -EFAULT;

	if (!(file->vNode->mode & VFS_ATTR_WRITE))
		return -EPERM;

	/* Only files can be written */
	if (file->vNode->mode & VFS_ATTR_DIR)
		return -EISDIR;

	if (file->vNode->superBlock && file->vNode->superBlock->flags & SB_RDONLY)
		return -EROFS;

	if (file->fileOps->write)
	{
		err=file->fileOps->write(file, buffer, size);
		VfsFileAccessed(file->vNode);
		VfsFileModified(file->vNode, 0);

		return err;
	}

	return -ENOTIMPL;
}

SYMBOL_EXPORT(DoWriteFile);

int DoSeekFile(struct File* file,int distance,int whence)
{
	int temp=-1;

	if (file->fileOps->seek)
		return file->fileOps->seek(file,distance,whence);
	else{
		switch (whence)
		{
			case 0: /* SEEK_SET */
				temp=distance;
				break;

			case 1: /* SEEK_CUR */
				temp = file->position+distance;
				break;

			case 2: /* SEEK_END */
				if (!file->vNode)
					return -EINVAL;
				temp =file->vNode->size - distance;
				break;
				
			default:
				KePrint("Invalid, %d\n", whence);
				return -EINVAL;
		}
	}
	
	if (temp < 0)
		return -EINVAL;
		
	if ((DWORD)temp != file->position)
		file->position = (DWORD)temp; /* May want to adjust readahead at this point */

	return file->position;
}

SYMBOL_EXPORT(DoSeekFile);

/***********************************************************************
 *
 * FUNCTION: DoCloseFile
 *
 * DESCRIPTION: Close the file pointer, release the VFS node.
 *
 * PARAMETERS: file - file to close.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int DoCloseFile(struct File* file)
{
	int ret = 0;
	
	if (!file || !file->vNode)
		return -EFAULT;

	if (file->refs <= 0)
		return -EINVAL;
	
	if (--file->refs)
		return 0;

	if (file->fileOps && file->fileOps->close)
		ret = file->fileOps->close(file);

	VNodeRelease(file->vNode);

	return ret;
}

SYMBOL_EXPORT(DoCloseFile);

int FileNameMaxLength(char* path)
{
	struct VMArea* area,*next;
	int len;

	area=VmLookupAddress(current, (DWORD)path);
	if (!area)
		return -EFAULT;

	len=(area->start+area->length)-(DWORD)path;
	if (len > PATH_MAX)
		len=PATH_MAX;

	next=ListEntry(area->list.next,struct VMArea,list);

	if (area->list.next != &current->areaList && next->start == area->start+area->length)
		len=PATH_MAX;

	return len;
}

/***********************************************************************
 *
 * FUNCTION: 	GetName
 *
 * DESCRIPTION: Copy the filename from userspace to kernel space, to avoid
 *				race conditions (i.e. if another userspace thread modifies
 *				the path).
 *
 * PARAMETERS: 	path - pointer to userspace string
 *
 * RETURNS:		Allocated and copied string.
 *
 ***********************************************************************/

char* GetName(char* path)
{
	int i=0;
	char* ret;
	int len;

	/* Handle zero-length paths. Is there a better place for this? */
	if (!path[0])
		return NULL;

	len=FileNameMaxLength(path);

	if (len <= 0)
		return NULL;

	ret=(char*)MemAlloc(len);

	if (!ret)
		return NULL;

	/* Copy one by one */
	while (i < len)
	{
		ret[i]=path[i];
		if (!ret[i])
			return ret;

		++i;
	}

	/* Null terminate the name - so it's actually PATH_MAX-1 characters.
	Is this correct? */

	ret[i]='\0';

	return ret;
}

SYMBOL_EXPORT(GetName);

int SysOpen(char* path,int flags,int perms)
{
	char* copiedPath;
	int err, fd;
	struct File* file;

	fd = VfsGetFreeFd(current);

	if (fd < 0)
	{
		err = fd;
		goto out;
	}

	copiedPath=GetName(path);
	err = PTR_ERROR(copiedPath);

	if (copiedPath)
	{
		file = FileAllocate();
		
		err = DoOpenFile(file, copiedPath, flags, perms);

		if (err)
		{
			FileFree(file);
			goto freePath;
		}

		/*
		 * Avoid race conditions with VfsGetFreeFd by copying into a temporary
		 * file structure.
		 */

		SpinLock(&current->fileListLock);
		current->files[fd] = file;
		SpinUnlock(&current->fileListLock);

freePath:
		MemFree(copiedPath);
	}
	
out:
	return (err < 0) ? err : fd;
}

/* SysRead
 * --------
 * Returns the number of bytes read - or a negative error on failure
 */

int SysRead(int fd, BYTE* buffer, DWORD amount)
{
	struct File* file=FileGet(fd);
	
	if (UNLIKELY(!file))
		return -EBADF;

	if (VirtCheckArea((void*)buffer, amount, VER_WRITE))
		return -EFAULT;

	return DoReadFile(file,buffer,amount);
}

/* SysWrite
 * --------
 * Returns the number of bytes written, or a negative error on failure
 */

int SysWrite(int fd,BYTE* data,DWORD amount)
{
	struct File* file = FileGet(fd);
	
	if (UNLIKELY(!file))
		return -EBADF;

	if (VirtCheckArea((void*)data, amount, VER_READ))
		return -EFAULT;

	return DoWriteFile(file,data,amount);
}

int SysClose(int fd)
{
	struct File* file = FileGet(fd);
	int ret;
	
	if (UNLIKELY(!file))
		return -EBADF;

	ret = DoCloseFile(file);
	
	if (ret)
		return ret;
		
	return FdFree(current, &current->files[fd]);
}

int DoRemove(char* pathName)
{
	struct VNode* vNode;
	char* name;
	int nameLen;
	int err;

	/* Check that pathName does exist and is not a directory */
	err=NameToVNode(&vNode, pathName, FILE_FORCE_OPEN, NULL);
	if (err)
		return err;

	if (vNode->mode & VFS_ATTR_DIR)
	{
		VNodeRelease(vNode);
		return -EISDIR;
	}

	if (vNode->superBlock->flags & SB_RDONLY)
	{
		VNodeRelease(vNode);
		return -EROFS;
	}

	VNodeRelease(vNode);

	/* If it does exist, remove */
	vNode=DirNameVNode(pathName, &name, &nameLen, NULL);
	if (!vNode)
		return -ENOENT;

	/* Probably trying to remove the root directory - not a good idea. */
	if (!name || !nameLen)
	{
		VNodeRelease(vNode);
		return -EPERM;
	}

	if (vNode->vNodeOps && vNode->vNodeOps->remove)
		err=vNode->vNodeOps->remove(vNode,name,nameLen);
	else
		err=-ENOTIMPL;

	/* Directory vNode */
	VNodeRelease(vNode);

	return err;
}

int SysRemove(char* pathName)
{
	int err;
	char* copiedName;

	copiedName=GetName(pathName);
	err=PTR_ERROR(copiedName);

	if (copiedName)
	{
		err=DoRemove(copiedName);
		MemFree(copiedName);
	}

	return err;
}

int SysIoCtl(int fd,unsigned long code,char* data)
{
	struct File* file=FileGet(fd);
	if (UNLIKELY(!file))
		return -EBADF;

	/* FIXME: Temporary hack. */
	if (code >= 0x80000000)
	{
		file->flags |= FILE_NONBLOCK;
		return 0;
	}

	if (file->fileOps && file->fileOps->ioctl)
		return file->fileOps->ioctl(file,code,data);
	else
		return -ENOTIMPL;
}

int SysFileDup(int fd)
{
	int i;
	struct File* file;

	PreemptDisable();

	/* Duplicate file information in fd. Find a new file descriptor */
	for (i=0; i<MAX_FILES; i++)
		if (!current->files[i])
			break;

	if (i == MAX_FILES)
		return -EMFILE;

	file=FileGet(fd);
	
	if (UNLIKELY(!file))
		return -EBADF;

	/* FIXME. */

	PreemptEnable();

	return i;
}

/* TODO: Create sync function for file operations. */
int SysFileSync(int fd)
{
	struct File* file=FileGet(fd);

	if (!file)
		return -EBADF;

	VNodeWrite(file->vNode);
	
	return BlockSyncDevice(file->vNode->superBlock->sDevice);
}

int SysTruncate(int fd,DWORD length)
{
	struct File* file=FileGet(fd);
	int err=-ENOTIMPL;

	if (!file)
		return -EBADF;

	if (file->vNode->superBlock->flags & SB_RDONLY)
		return -EROFS;

	if (!(file->vNode->mode & VFS_ATTR_WRITE))
		return -EPERM;

	if (file->vNode->mode & VFS_ATTR_DIR)
		return -EISDIR;

	if (file->vNode->vNodeOps && file->vNode->vNodeOps->truncate)
	{
		err=file->vNode->vNodeOps->truncate(file->vNode,length);
		if (!err)
		{
			file->vNode->size=length;
			
			if (file->position > length)
				file->position = length;
		}
	}else
		err=-ENOTIMPL;

	return err;
}

int SysMove(char* src,char* dest)
{
	return -ENOTIMPL;
}

int SysFileAccess(char* path,int mode)
{
	return -ENOTIMPL;
}

int SysSeek(int fd,int distance,int whence)
{
	struct File* file=FileGet(fd);
	if (UNLIKELY(!file))
		return -EBADF;
	
	return DoSeekFile(file,distance,whence);
}

/*******************************************************************************
 *
 * FUNCTION: 	VfsStat
 *
 * DESCRIPTION: Fill in the Stat structure with information from the vNode
 *
 * PARAMETERS: 	vNode - vNode to update.
 *			   	buf - address of the stat structure to fill in/
 *
 * RETURNS: 	0 for now.
 *
 ******************************************************************************/

int VfsStat(struct VNode* vNode, void* buf)
{
	struct Stat* stat=(struct Stat*)buf;

	stat->size=vNode->size;
	stat->vNum=vNode->id;
	stat->mode=vNode->mode;

	stat->aTime=vNode->aTime.seconds;
	stat->mTime=vNode->mTime.seconds;
	stat->cTime=vNode->cTime.seconds;

	return 0;
}

int SysStat(char* name, void* buf, int fdAt)
{
	struct VNode* vNode=NULL, *dir=NULL;
	char* copiedName;
	int err;

	/* Make sure stat structure is fine to write to */
	if (VirtCheckArea(buf,sizeof(struct Stat),VER_WRITE))
		return -EFAULT;
	
	if (fdAt >= 0)
	{
		struct File* file;
		
		file=FileGet(fdAt);
		
		if (UNLIKELY(!file) || UNLIKELY(!file->vNode))
			return -EBADF;
			
		dir=file->vNode;
	}

	copiedName=GetName(name);
	err=PTR_ERROR(copiedName);
	
	if (copiedName)
	{
		err=NameToVNode(&vNode, copiedName, FILE_FORCE_OPEN, dir);

		if (vNode)
		{
			err=VfsStat(vNode, buf);
			VNodeRelease(vNode);
		}

		MemFree(copiedName);
	}

	return err;
}

int SysStatFd(int fd, void* buf)
{
	struct File* file=FileGet(fd);
	if (UNLIKELY(!file))
		return -EBADF;

	if (VirtCheckArea(buf, sizeof(struct Stat), VER_WRITE))
		return -EFAULT;

	return VfsStat(current->files[fd]->vNode, buf);
}
