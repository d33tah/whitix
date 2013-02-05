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
#include <module.h>
#include <task.h>
#include <print.h>
#include <typedefs.h>

/***********************************************************************
 *
 * FUNCTION: VfsDoLookup
 *
 * DESCRIPTION: Wraps up looking up one element of a directory path.
 *
 * PARAMETERS: currName - name to lookup.
 *			   length - length of currName.
 *			   ret - vNode to return information into.
 *			   dir - directory that currName is in.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int VfsDoLookup(char* currName,int length,struct VNode** ret,struct VNode* dir)
{
	int err=0;
	struct VNode* oldNode;

	if (!dir)
		return -EFAULT;

	/* Not implemented. */
	if (!dir->vNodeOps || !dir->vNodeOps->lookup)
		return -ENOTIMPL;

	/* Check if the current node is a directory before we lookup ... */
	if (!(dir->mode & VFS_ATTR_DIR))
		return -ENOTDIR;

	/* If the directory name is just '.', return the current directory */
	if (length == 1 && currName[0] == '.')
	{
		*ret=dir;
		return 0;
	}

	/* 
	 * If the directory name is '..', return the parent directory in special cases, otherwise, every filesystem
	 *  has .. entries, so do a lookup there.
	 */
	if (length == 2 && currName[0] == '.' && currName[1] == '.')
	{
		if (dir == current->root)
		{
			*ret=dir;
			return 0;
		}else if (dir == dir->superBlock->mount)
		{
			oldNode=dir;
			dir=VNodeGet(dir->superBlock->parent, dir->superBlock->coveredId);
			VNodeRelease(oldNode);
		}
	}

	*ret=NULL;

	/* Do the lookup call. */
	err=dir->vNodeOps->lookup(ret, dir, currName, length);

	if (err)
		goto out;

	/* This is how mountpoints work. FIXME: Does every vNode need a mountNode? */
	if (*ret && (*ret)->mountNode)
	{
		oldNode=*ret;
		VNodeRelease(oldNode);
		*ret = (*ret)->mountNode;
		(*ret)->refs++;
	}
		
	/* Check if the new vNode is a link. TODO: Move. */
	if ((*ret)->mode & VFS_ATTR_SOFTLINK)
	{
//		KePrint("Following symlink, %#X\n", (*ret)->vNodeOps->followLink);
		err = (*ret)->vNodeOps->followLink(ret, *ret);
	}

out:
		
	VNodeRelease(dir);
	return err;
}

/***********************************************************************
 *
 * FUNCTION: 	DirNameVNode
 *
 * DESCRIPTION: Resolve a filesystem path to a parent directory and entry
 *				name.
 *
 * PARAMETERS: 	path
 *
 * RETURNS: 	Usual error codes in error.h
 *
 ***********************************************************************/

struct VNode* DirNameVNode(char* path, char** retPath, int* retLen, struct VNode* dir)
{
	int length;
	char* currName=NULL;
	struct VNode *currNode,*newNode;
	char c;

	/* Does the grunt work of NameToVNode. Just returns an vnode, regardless of the on-disk structure
	being a file or directory */

	if (!path)
		return NULL;

	if (retLen)
		*retLen=0;

	if (path[0] == '/')
	{
		currNode=current->root;
		++currNode->refs;
		if (strlen(path) == 1)
		{
			*retPath=NULL;
			if (retLen)
				*retLen=0;
			return currNode;
		}
		++path;
	}else{
		currNode = (dir != NULL) ? dir : current->cwd;
		++currNode->refs;
	}

	/* Parse each pathname */
	while (*path)
	{
		/* Skip past initial '/'s */
		while (*path == '/')
			path++;

		currName=path;

		/* Seperate each path element by '/' */
		for (length=0; (c=*(path++)) && (c != '/'); length++);

		if (!c)
			break;
			
		currNode->refs++;

		if (VfsDoLookup(currName, length, &newNode, currNode) || !(newNode->mode & VFS_ATTR_DIR))
			return NULL;

		currNode=newNode;
	}

	*retPath=currName;

	if (retLen)
		*retLen=strlen(currName);

	return currNode;
}

int NameToVNode(struct VNode** retVal, char* path, int flags, struct VNode* dirAt)
{
	char* name;
	int nameLen,strLen,ret=0;
	struct VNode* dir;

	if (!path || !retVal)
		return -EFAULT;

	strLen=strlen(path)-1;

	if (path[strLen] == '/')
	{
		/* Looking up the root directory? */
		if (!strLen)
		{
			*retVal=current->root;
			current->root->refs++;
			return 0;
		}

		/* Remove the trailing "/" otherwise. */
		path[strLen]='\0';
	}

	dir=DirNameVNode(path, &name, &nameLen, dirAt);

	/* Could not find the directory leading to the vNode, so it must be an
	 * error */
	if (!dir)
		return -ENOENT;
	
	/* Now the directory vNode is known, create/open the file, according to what
	the caller wants to do. */
	
	if (flags & FILE_FORCE_CREATE)
	{
		++dir->refs;

		/* The file must be created, if it already exists, it's an error */
		if (VfsDoLookup(name, nameLen, retVal, dir))
		{
			if (dir->vNodeOps->create)
			{
				ret=dir->vNodeOps->create(retVal, dir, name, nameLen);

				if (*retVal)
				{
					VfsFileAccessed(*retVal);
					VfsFileModified(*retVal,1);
					SetVNodeDirty(*retVal);
				}
			}else
				ret=-EROFS;
		}else{
			VNodeRelease(dir);
			ret=-EEXIST;
		}
	}else if (flags & FILE_CREATE_OPEN)
	{
		dir->refs++;
		/* The file can either be created or opened */
		if (VfsDoLookup(name,nameLen,retVal,dir))
		{
			/* Obviously doesn't exist, so create it */
			if (dir->vNodeOps && dir->vNodeOps->create)
			{
				ret=dir->vNodeOps->create(retVal,dir,name,nameLen);

				if (*retVal)
				{
					VfsFileAccessed(*retVal);
					VfsFileModified(*retVal,1);
					SetVNodeDirty(*retVal);
				}
			}else
				ret=-EROFS;
		}else{
			VNodeRelease(dir);
		}

		/* 
		 * Otherwise, it already exists and DoLookup was a success
		 */
	}else{
		/* Normal lookup */
		ret=VfsDoLookup(name, nameLen, retVal, dir);
	}
	
	/* Check if the flags state that the file opened must be a directory.
	 * TODO: This check can't be hit in the create cases, so avoid it. */
	if (*retVal && (flags & FILE_DIRECTORY) && ((*retVal)->flags & VFS_ATTR_DIR))
	{
		VNodeRelease(*retVal);
		retVal=NULL;
		ret=-ENOTDIR;
	}

	/* On success, this code is reached */
	return ret;
}

SYMBOL_EXPORT(NameToVNode);

/* For demand-loading and memory mapping really */
int FileGenericReadPage(DWORD pageAddr,struct VNode* vNode,int offset)
{
	char* page=(char*)pageAddr;
	int i,res,blockSize;
	struct Buffer* buff;

	if (!vNode || !vNode->superBlock)
		return -EFAULT;

	if (!vNode->vNodeOps || !vNode->vNodeOps->blockMap)
		return -ENOTIMPL;

	blockSize=BYTES_PER_SECTOR(vNode->superBlock);

	/* Read in PAGE_SIZE/blockSize buffers to the page. */
	for (i=0; i<PAGE_SIZE/blockSize; i++)
	{
		res=vNode->vNodeOps->blockMap(vNode, (offset/blockSize)+i, 0);

		if (res < 0)
		{
			/* Quite unlikely, but I think this is the best way to deal with it */
			ZeroMemory(page+(i*blockSize), blockSize);
		}else{
			buff=BlockRead(vNode->superBlock->sDevice, res);
			if (!buff)
			{
				KePrint("FileGenericReadPage: failed to read\n");
				return -EIO;
			}
			
			memcpy(page+(i*blockSize), buff->data, blockSize);
			BlockFree(buff);
		}
	}

	return 0;
}

SYMBOL_EXPORT(FileGenericReadPage);
