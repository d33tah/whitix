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
#include <error.h>
#include <task.h>
#include <malloc.h>
#include <user_acc.h>
#include <module.h>
#include <print.h>

/***********************************************************************
 *
 * FUNCTION: DoCreateDir
 *
 * DESCRIPTION: Create a directory at 'pathName'.
 *
 * PARAMETERS: pathName - name of the directory.
 *			   perms - permissions of the directory.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

int DoCreateDir(char* pathName,int perms)
{
	struct VNode* dir,*retVal;
	char* path;
	int pathLen,err;

	if (!NameToVNode(&retVal,pathName, 0, NULL) && retVal)
	{
		VNodeRelease(retVal);
		return -EEXIST;
	}

	/* Get the directory node */
	dir=DirNameVNode(pathName,&path,&pathLen, NULL);
	if (!dir)
		return -ENOENT;

	/* Can't SysCreateDir the root directory. */
	if (!path || !pathLen)
	{
		err=-EPERM;
		goto fail;
	}
	
	/* Can't create a directory on a read-only filesystem. */
	if (dir->superBlock->flags & SB_RDONLY)
	{
		err=-EROFS;
		goto fail;
	}

	/* Check if the mkDir function actually exists. */
	if (!dir->vNodeOps || !dir->vNodeOps->mkDir)
	{
		err=-ENOTIMPL;
		goto fail;
	}

	err=dir->vNodeOps->mkDir(&retVal,dir,path,pathLen);

	if (!err && retVal)
	{
		/* Make sure the new directory node gets written to disk */
		VfsFileAccessed(retVal);
		VfsFileModified(retVal,1);
		SetVNodeDirty(retVal);

		VNodeRelease(retVal);
	}

fail:
	VNodeRelease(dir);
	return err;
}

/***********************************************************************
 *
 * FUNCTION: SysCreateDir
 *
 * DESCRIPTION: Create a directory at 'pathName' - system call interface.
 *
 * PARAMETERS: pathName - name of the directory.
 *			   perms - permissions of the directory.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

int SysCreateDir(char* pathName,int perms)
{
	char* copiedPath;
	int err;

	copiedPath=GetName(pathName);
	err=PTR_ERROR(copiedPath);

	if (copiedPath)
	{
		err=DoCreateDir(pathName,perms);
		MemFree(copiedPath);
	}

	return err;
}

/***********************************************************************
 *
 * FUNCTION:	DoRemoveDir
 *
 * DESCRIPTION: Remove a directory on the filesystem.
 *
 * PARAMETERS:	path - path of the directory.
 *
 * RETURNS:		Usual error codes in error.h
 *
 ***********************************************************************/

int DoRemoveDir(char* path)
{
	struct VNode* dir;
	char* pathName;
	int pathLen,err;

	/* Check that pathName does exist and is a directory */
	err=NameToVNode(&dir, path, FILE_FORCE_OPEN, NULL);
	if (err)
		return err;

	if (dir->mode & VFS_ATTR_FILE)
	{
		VNodeRelease(dir);
		return -ENOTDIR;
	}

	VNodeRelease(dir);

	/* Now actually remove the directory */
	dir=DirNameVNode(path,&pathName,&pathLen, NULL);

	if (!dir)
		return -ENOENT;

	/* Don't try to remove the root directory. */
	if (!pathName || !pathLen)
	{
		err=-EPERM;
		goto fail;
	}

	if (dir->superBlock->flags & SB_RDONLY)
	{
		err=-EROFS;
		goto fail;
	}

	if (!dir->vNodeOps || !dir->vNodeOps->rmDir)
	{
		err=-ENOTIMPL;
		goto fail;
	}
	
	err=dir->vNodeOps->rmDir(dir,pathName,pathLen);

fail:
	VNodeRelease(dir);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:	SysRemoveDir
 *
 * DESCRIPTION: System call interface to remove a directory.
 *
 * PARAMETERS:	path - path of the directory to remove.
 *
 * RETURNS:		Usual error codes in error.h
 *
 ***********************************************************************/

int SysRemoveDir(char* path) 
{
	char* copiedPath;
	int err;

	copiedPath=GetName(path);
	err=PTR_ERROR(copiedPath);

	if (copiedPath)
	{
		err=DoRemoveDir(path);
		MemFree(copiedPath);
	}

	return err;
}

/***********************************************************************
 *
 * FUNCTION:	DoChangeRoot
 *
 * DESCRIPTION: Change the filesystem root for a particular process.
 *
 * PARAMETERS:	dirName - name of the directory that is the new root.
 *
 * RETURNS:		Usual error codes in error.h
 *
 ***********************************************************************/

int DoChangeRoot(char* dirName)
{
	struct VNode* dirNode;
	int err;

	err=NameToVNode(&dirNode, dirName, FILE_READ | FILE_FORCE_OPEN, NULL);

	if (err)
		return err;

	/* New root must be a directory */
	if (!(dirNode->mode & VFS_ATTR_DIR))
	{
		VNodeRelease(dirNode);
		return -ENOTDIR;
	}

	/* Change the current directory (and root) to the new root vNode,
	 * because we can't be sure the current directory doesn't point to a
	 * location outside that accessible by the root directory.
	 */
	VNodeRelease(current->cwd);
	VNodeRelease(current->root);

	current->cwd=current->root=dirNode;
	dirNode->refs+=2;

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:	SysChangeRoot
 *
 * DESCRIPTION: Change the filesystem root for a particular process.
 *
 * PARAMETERS:	dirName - name of the directory that is the new root.
 *
 * RETURNS:		Usual error codes in error.h
 *
 ***********************************************************************/

int SysChangeRoot(char* dirName)
{
	int err;
	char* copiedPath;

	copiedPath=GetName(dirName);
	err=PTR_ERROR(copiedPath);

	if (copiedPath)
	{
		err=DoChangeRoot(dirName);
		MemFree(copiedPath);
	}

	return err;
}

struct FillDirInfo
{
	struct DirEntry* curr,*prev;
	int count,error;
};

/* Round up the length to 4 bytes as memory access is quicker. */
#define ROUND_UP(x) (((x)+sizeof(long)-1) & ~(sizeof(long)-1))

/***********************************************************************
 *
 * FUNCTION:	FillDir
 *
 * DESCRIPTION: Used by filesystem drivers to transfer a directory entry
 *				to userspace.
 *
 * PARAMETERS:	entries - pointer given to the ->ReadDir function.
 *				name - name of the directory entry.
 *				nameLen - length of the directory's name entry.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int FillDir(void* entries,char* name,int nameLen,DWORD vNum)
{
	struct FillDirInfo* dirEntries=(struct FillDirInfo*)entries;
	int recordLen=ROUND_UP( OffsetOf(struct DirEntry, name) + nameLen + 1);

	if (dirEntries->count < recordLen) /* No space */
		return -EFAULT;

	/* Fill in the current directory entry. */
	dirEntries->curr->vNodeNum=vNum;
	dirEntries->curr->length=recordLen;
	dirEntries->curr->offset=0;
	memcpy(dirEntries->curr->name, name, nameLen);
	dirEntries->curr->name[nameLen]='\0';

	/* And update the GetDirEntries state. */
	dirEntries->count-=recordLen;
	dirEntries->prev=dirEntries->curr;
	dirEntries->curr=(void*)dirEntries->curr+recordLen;
	dirEntries->error=0;

	return 0;
}

SYMBOL_EXPORT(FillDir);

int SysGetDirEntries(int fd,void* entries,DWORD count)
{
	struct File* file;
	struct FillDirInfo fDirInfo;
	struct VNode* vNode;

	ZeroMemory(&fDirInfo,sizeof(struct FillDirInfo));

	fDirInfo.count=count;
	fDirInfo.curr=(struct DirEntry*)entries;

	/* Do usual sanity checks */

	file=FileGet(fd);
	if (!file)
		return -EBADF;

	/* Must be able to write the directory entries to the given area of memory. */
	if (VirtCheckArea(entries,count,VER_WRITE))
		return -EFAULT;

	vNode=file->vNode;

	if (!(vNode->mode & VFS_ATTR_DIR))
	{
		KePrint("SysGetDirEntries : not a directory\n");
		return -ENOTDIR;
	}

	if (file->position >= vNode->size) /* Past the end of the directory */
		return 0;

	if (vNode->fileOps && vNode->fileOps->readDir)
		vNode->fileOps->readDir(file,&fDirInfo);
	else
	{
		KePrint("GetDirEntries is not yet implemented for fs\n");
		return -ENOTIMPL;
	}

	VfsFileAccessed(file->vNode);

	/* Check if an error occured during ReadDir. If not, return the total byte-count of
	 * the ReadDir entries.
	 */
	if (fDirInfo.error)
		return fDirInfo.error;

	return (count-fDirInfo.count);
}

/* Adds the path string to the process->sCwd. TODO: Check that MAX_PATH length is not exceeded.
   This function is fairly hacky and not well put together. FIXME. */

void AddStringPath(struct Process* process,char* dirName)
{
	int i,j;

	/* Start from root if need be */
	if (dirName[0] == '/')
	{
		ZeroMemory(process->sCwd,PATH_MAX);
		process->sCwd[0]='/';
		i=1; j=1;
	}else{
		i=0; j=strlen(process->sCwd);
	}

	while (dirName[i])
	{
		while (dirName[i] == '/')
			i++;

		if (!strncmp(&dirName[i],"..",2))
		{
			/* Go back - currently at end of string */
			--j;

			/* Skip over '/' */
			while (process->sCwd[j] == '/')
				--j;

			/* Go back and zero the string */
			while (process->sCwd[j] != '/')
			{
				process->sCwd[j]='\0';
				--j;
			}

			i+=2; /* Skip the .. part */
			
			while (dirName[i] == '/')
				++i;

			while (process->sCwd[j] == '/')
				++j;

		}else if (dirName[i] == '.')
		{
			/* Just skip the . */
			i+=2;
		}else{
			do
			{
				process->sCwd[j]=dirName[i];
				i++; j++;
			}while ((dirName[i] != '/') && dirName[i]);

			process->sCwd[j++]='/';

			if (dirName[i] == '/')
				i++;
		}
	}

	process->sCwd[j]='\0';
}

/* Changes the current directory of a process */

int VfsChangeDir(char* dirName)
{
	int len=strlen(dirName);
	struct VNode* dirNode;
	int err;

	/* Check for .. at root directory */
	if (strlen(current->sCwd) == 1 && !strncmp(dirName,"..",2))
		return 0;

	/* No point looking up */
	if (len == 1 && dirName[0] == '.')
		return 0;

	if (len == 1 && dirName[0] == '/')
	{
		/* Special case, why search for the root directory when it's known? */
		strncpy(current->sCwd, dirName, PATH_MAX);

		if (current->cwd != current->root)
		{
			VNodeRelease(current->cwd);
			current->cwd=current->root;
			current->cwd->refs++;
		}

		return 0;
	}

	/* Look up the directory vNode. */
	err=NameToVNode(&dirNode, dirName, FILE_READ | FILE_FORCE_OPEN, NULL);

	if (err)
		return err;

	if (!(dirNode->mode & VFS_ATTR_DIR))
	{
		VNodeRelease(dirNode);
		return -ENOTDIR;
	}

	/* Update the directory path string. Used in SysGetCurrDir. */
	AddStringPath(current, dirName);

	/* And finally, update the current directory vNode for the process. */
	VNodeRelease(current->cwd);
	current->cwd=dirNode;

	return 0;
}

int SysChangeDir(char* dirName)
{
	char* copiedPath;
	int err;

	copiedPath=GetName(dirName);
	err=PTR_ERROR(copiedPath);

	if (copiedPath)
	{
		err=VfsChangeDir(copiedPath);
		MemFree(copiedPath);
	}

	return err;
}

int SysGetCurrDir(char* str,int size)
{
	if (strlen(current->sCwd) > size)
		return -EFAULT; /* -ETOOLONG */

	if (VirtCheckArea((void*)str,(DWORD)size,VER_WRITE))
		return -EFAULT;

	strncpy(str,current->sCwd,size);

	return strlen(current->sCwd);
}
