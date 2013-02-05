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
#include <malloc.h>
#include <error.h>
#include <print.h>

#include "ext3.h"

int Ext3INodeGetLoc(struct VNode* vNode, struct Buffer** buffer, struct Ext3INode** iNode)
{
	DWORD vNo=vNode->id-1;
	struct Ext3SbInfo* sbInfo=EXT3_SUPERINFO(vNode->superBlock);
	DWORD blockGroup,groupDesc,desc, block;
	struct Ext3GroupDesc* groupDescs;

	if (vNo > sbInfo->iNodesCount)
		return -EINVAL;

	blockGroup=vNo/sbInfo->iNodesPerGrp;

	if (blockGroup >= sbInfo->groupCount)
		return -EINVAL;

	/* Get the group descriptor for the inode */
	groupDesc=blockGroup/DESCS_PER_BLOCK(vNode->superBlock);
	desc=blockGroup & (DESCS_PER_BLOCK(vNode->superBlock)-1);

	groupDescs=(struct Ext3GroupDesc*)(sbInfo->descs[groupDesc]->data);

	if (!groupDescs)
		return -EFAULT;

	block=groupDescs[desc].iNodeTable+((vNo % sbInfo->iNodesPerGrp)/INODES_PER_BLOCK(vNode->superBlock));

	*buffer=BlockRead(vNode->superBlock->sDevice, block);

	if (!*buffer)
		return -EIO;

	*iNode=(((struct Ext3INode*)(*buffer)->data)+(vNo % INODES_PER_BLOCK(vNode->superBlock)));

	return 0;
}

int Ext3ReadVNode(struct VNode* vNode)
{
	struct Buffer* buffer;
	struct Ext3INode* iNode;
	struct Ext3INodeInfo* iNodeInfo;
	int err;

	err=Ext3INodeGetLoc(vNode, &buffer, &iNode);

	if (err)
		return err;

	iNodeInfo=(struct Ext3INodeInfo*)MemAlloc(sizeof(struct Ext3INodeInfo));

	if (!iNodeInfo)
	{
		BlockFree(buffer);
		return -ENOMEM;
	}
	
	/* Copy EXT3-specific data first. */
	iNodeInfo->blockGroup=(vNode->id-1)/EXT3_SUPERINFO(vNode->superBlock)->iNodesPerGrp;

	memcpy(iNodeInfo->blocks, iNode->blocks, 15*sizeof(DWORD));

	/* Copy the iNode times over */
	vNode->aTime.seconds=iNode->aTime;
	vNode->cTime.seconds=iNode->cTime;
	vNode->mTime.seconds=iNode->mTime;

	vNode->vNodeOps=&ext3VNodeOps;
	vNode->fileOps=&ext3FileOps;
	vNode->mode=VFS_ATTR_WRITE | VFS_ATTR_READ;
	vNode->extraInfo=iNodeInfo;

	if ((iNode->mode & 0xF000) == 0x4000)
		vNode->mode|=VFS_ATTR_DIR;
	else if ((iNode->mode & 0xF000) == 0x8000)
		vNode->mode|=VFS_ATTR_FILE;

	vNode->size=iNode->size;

	BlockFree(buffer);
	return 0;
}

int Ext3WriteVNode(struct VNode* vNode)
{
	int err;

	/* Force a commit to the journal, to make sure this vNode is written to disk. */
	err=JournalForceCommit(EXT3_JOURNAL(vNode));

	return err;
}

int Ext3INodeReserveWrite(struct JournalHandle* handle, struct VNode* vNode,
	struct Buffer** buffer, struct Ext3INode** iNode)
{
	int err;

	/* Fill in buffer and iNode first, before signalling to the journal that we're
	 * going to write at that disk location. */

	err=Ext3INodeGetLoc(vNode, buffer, iNode);

	if (!err)
		err=JournalGetWriteAccess(handle, *buffer);

	return err;
}

int Ext3INodeUpdate(struct JournalHandle* handle, struct VNode* vNode,
	struct Buffer* buffer, struct Ext3INode* iNode)
{
	struct Ext3INodeInfo* info=EXT3_INFO(vNode);
	
	if (handle)
	{
		/* Call GetWriteAccess? */
	}

	/* Fill in the raw inode structure. */
	iNode->aTime=vNode->aTime.seconds;
	iNode->cTime=vNode->cTime.seconds;
	iNode->mTime=vNode->mTime.seconds;
	iNode->size=vNode->size;
	
	if (vNode->mode & VFS_ATTR_DIR)
		iNode->mode=0x4000;
	else if (vNode->mode & VFS_ATTR_FILE)
		iNode->mode=0x8000;
		
	iNode->linksCount=1;
	iNode->blocksCount=((vNode->size+BYTES_PER_SECTOR(vNode->superBlock)-1)/BYTES_PER_SECTOR(vNode->superBlock));
	
	int i;
	for (i=0; i<15; i++)
		iNode->blocks[i]=info->blocks[i];
	
	/* TODO: Complete. */

//	KePrint("Update\n");

	JournalDirtyMetadata(handle, buffer);

//	BufferRelease(buffer);

	return 0;
}

int Ext3DirtyVNode(struct VNode* vNode)
{
	struct JournalHandle* handle;
	struct Buffer* buffer;
	struct Ext3INode* iNode;
	int err=0;

	handle=JournalStart(EXT3_JOURNAL(vNode), 2);

	if (handle)
	{
		err=Ext3INodeReserveWrite(handle, vNode, &buffer, &iNode);

		if (!err)
		{
			BufferGet(buffer);
			err=Ext3INodeUpdate(handle, vNode, buffer, iNode);
			BufferRelease(buffer);
		}
	}

	JournalStop(handle);

	return err;
}

int Ext3FindEntry(struct VNode* dir, char* name, int nameLength, struct Ext3DirEntry** ext3Entry,
	struct Buffer** buffer)
{
	DWORD i=0;
	struct Ext3DirEntry* entry;
	int bytesPerSec=BYTES_PER_SECTOR(dir->superBlock);
	
	while (i < dir->size)
	{
		int block=i/bytesPerSec;
		int offset=i % bytesPerSec;

		/* Read in the sector in question, and scan it for the directory entry */
		struct Buffer* buff=Ext3BlockRead(dir, block, 0);

		while (i < dir->size && offset < bytesPerSec)
		{
			entry=(struct Ext3DirEntry*)(buff->data+(i % bytesPerSec));

			if (entry->nameLen == nameLength)
			{
				/* No need to go through comparing it all just to find it's a different length */
				if (!strncmp(entry->name,name,nameLength))
				{
					*ext3Entry=entry;
					
					if (buffer)
						*buffer=buff;
						
					return 0;
				}
			}

			offset += entry->recLen;
			i += entry->recLen;
		}

	}
	
	return -ENOENT;
}

static int Ext3AddEntry(struct JournalHandle* handle, char* name, int nameLength,
	struct VNode* dir, struct VNode* vNode)
{
	struct Buffer* buffer;
	int offset=0, recLen;
	struct Ext3DirEntry* entry;
	
//	KePrint("Here\n");
	
	buffer=Ext3BlockRead(dir, 0, 1);
	
	if (!buffer)
		return -EIO;
	
	recLen=EXT3_DIR_REC_LEN(nameLength);
	
	entry=(struct Ext3DirEntry*)buffer->data;
	
	while (1)
	{
		if ((char*)entry >= (char*)(BYTES_PER_SECTOR(vNode->superBlock)+buffer->data))
		{
			KePrint("New block?\n");
			cli(); hlt();
		}
		
		KePrint("%u %u\n", entry->iNodeNum, entry->recLen);
		
		if ((entry->iNodeNum == 0 && entry->recLen >= recLen) ||
			(entry->recLen >= EXT3_DIR_REC_LEN(entry->nameLen) + recLen))
		{
			JournalGetWriteAccess(handle, buffer);
			
			if (entry->iNodeNum)
			{
				struct Ext3DirEntry* next;
				
				next=(struct Ext3DirEntry*)((char*)entry+EXT3_DIR_REC_LEN(entry->nameLen));
				next->recLen=entry->recLen-EXT3_DIR_REC_LEN(entry->nameLen);
				entry->recLen=EXT3_DIR_REC_LEN(entry->nameLen);
								
				KePrint("de: (%u %u) %u\n", entry->recLen, entry->nameLen, next->recLen);
				entry=next;
			}
			
			entry->fileType=EXT3_FT_UNKNOWN;
			
			if (vNode)
			{
				entry->iNodeNum=vNode->id;
				entry->fileType=Ext3SetFileType(vNode->mode);
			}
			
			entry->nameLen=nameLength;
			memcpy(entry->name, name, nameLength);
			
			JournalDirtyMetadata(handle, buffer);
			return 0;
		}
		
		offset+=entry->recLen;
		entry=(struct Ext3DirEntry*)(((char*)entry)+entry->recLen);
	}
	
	KePrint("Out?\n");
	cli(); hlt();
	
	return 0;
}

static int Ext3AddNonDirEntry(struct JournalHandle* handle, char* name, int nameLength,
	struct VNode* dir, struct VNode* vNode)
{
	int err;
	
	err=Ext3AddEntry(handle, name, nameLength, dir, vNode);
	
	if (!err)
	{
		err=Ext3DirtyVNode(vNode);
	}
	
	return err;
}

int Ext3Create(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	struct JournalHandle* handle;
	struct VNode* vNode;
	int err;
	
	handle=JournalStart(EXT3_JOURNAL(dir), EXT3_DATA_TRANS + 3);
	
	vNode=Ext3CreateINode(handle, dir, 0);
	
	/* Create directory entry. */
	err=Ext3AddNonDirEntry(handle, name, nameLength, dir, vNode);
	
	/* Add orphan entry. */
	
	JournalStop(handle);
	
	*retVal=vNode;
	
	return err;
}

int Ext3RemoveEntry(struct JournalHandle* handle, struct Buffer* buffer, struct Ext3DirEntry* entry)
{
	DWORD i=0;
	struct Ext3DirEntry* curr=(struct Ext3DirEntry*)buffer->data, *prev=NULL;
	
	while (i < buffer->device->softBlockSize)
	{
		if (entry == curr)
		{
			JournalGetWriteAccess(handle, buffer);
			
			if (prev)
				prev->recLen=prev->recLen+curr->recLen;
			else
				curr->iNodeNum=0;
				
			JournalDirtyMetadata(handle, buffer);
			return 0;
		}
		
		i+=curr->recLen;
		prev=curr;
		curr=(struct Ext3DirEntry*)(((DWORD)curr)+curr->recLen);
	}
	
	return -ENOENT;
}

int Ext3Remove(struct VNode* dir, char* name, int nameLength)
{
	struct JournalHandle* handle;
	struct Ext3DirEntry* ext3Entry;
	struct Buffer* buffer;
	int err;
	
	handle=JournalStart(EXT3_JOURNAL(dir), EXT3_DELETE_TRANS_BLOCKS);
	
	err=Ext3FindEntry(dir, name, nameLength, &ext3Entry, &buffer);
	
	if (err)
		goto fail;
		
	err=Ext3RemoveEntry(handle, buffer, ext3Entry);
	
	/* Add to orphan list. */
	
fail:
	JournalStop(handle);
	
	return err;
}

int Ext3Lookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	int ret;
	struct Ext3DirEntry* ext3Entry;
	
	ret=Ext3FindEntry(dir, name, nameLength, &ext3Entry, NULL);
	
	if (!ret)
		*retVal=VNodeGet(dir->superBlock, ext3Entry->iNodeNum);
	
	return ret;
}

#define EXT3_DIO_CREDITS (EXT3_RESERVE_TRANS_BLOCKS+32)

int Ext3BlockMap(struct VNode* vNode, DWORD block, int flags)
{
	struct JournalHandle* handle=JournalCurrHandle();
	int ret=0;
	
	if (flags & VFS_MAP_CREATE)
	{
		handle=JournalStart(EXT3_JOURNAL(vNode), EXT3_RESERVE_TRANS_BLOCKS);
#if 0
		if (handle->numBlocks <= EXT3_RESERVE_TRANS_BLOCKS)
		{
			ret=JournalExtend(handle, EXT3_DIO_CREDITS);
			
			if (ret)
			{
				ret=JournalRestart(handle, EXT3_DIO_CREDITS);
			}
		}
#endif
	}
	
	if (!ret)
	{
//		KePrint("Ext3BlockMap(%u, %d)\n", block, flags);
		ret=Ext3BlockMapHandle(handle, vNode, block, flags);
	}
	
	if (flags & VFS_MAP_CREATE)
	{
		JournalStop(handle);
	}

	return ret;
}

int Ext3Truncate(struct VNode* vNode, DWORD size)
{
	if (size > vNode->size)
	{
		KePrint("Ext3Truncate, %d, %d\n", size, vNode->size);
	}else if (size < vNode->size)
	{
		return Ext3BlockTruncate(vNode, size);
	}
	
	return 0;
}

/***********************************************************************
 *
 * FUNCTION:	Ext3BlockRead
 *
 * DESCRIPTION: Map a block index to a physical block, and read in the
 *				buffer
 *
 * PARAMETERS:	vNode - ext3 vNode to read.
 *				block - block index to read at.
 *
 * RETURNS:		Buffer structure that has been read in.
 *
 ***********************************************************************/

struct Buffer* Ext3BlockRead(struct VNode* vNode, int block, int flags)
{
	int realBlock=Ext3BlockMap(vNode,block, flags);
	if (realBlock < 0)
		return NULL;
	
	return BlockRead(vNode->superBlock->sDevice, realBlock);
}

int Ext3ReadDir(struct File* file,void* dirEntries)
{
	int offset,block,bytesPerSec=BYTES_PER_SECTOR(file->vNode->superBlock);
	struct Ext3DirEntry* ext3Entry;
	struct Buffer* buff;

	if (file->position >= file->vNode->size)
		return 0;
	
	while (file->position < file->vNode->size)
	{
		offset=(file->position % bytesPerSec);
		block=file->position/bytesPerSec;
		buff=Ext3BlockRead(file->vNode,block, 0);
		if (!buff)
			return -EIO;

		while (file->position < file->vNode->size && offset < bytesPerSec)
		{
			ext3Entry=(struct Ext3DirEntry*)(buff->data+offset);
			
//			KePrint("%u %u, %s (%u %u)\n", file->position, file->vNode->size, ext3Entry->name, ext3Entry->nameLen, ext3Entry->recLen);

			if (LIKELY(ext3Entry->iNodeNum))
				if (FillDir(dirEntries,ext3Entry->name,ext3Entry->nameLen,ext3Entry->iNodeNum))
					goto out;

			offset+=ext3Entry->recLen;
			file->position+=ext3Entry->recLen;
		}

		BlockFree(buff);
	}

out:
	return 0;
}

/*******************************************************************************
 *
 * FUNCTION:	Ext3MkDir
 *
 * DESCRIPTION: Make a directory in dir with name 'name'.
 *
 * PARAMETERS:	retVal - new vNode that points to the new directory.
 *				dir - the directory to create the new directory in
 *				name - name of the new directory
 *				nameLength - length in bytes of the new name.
 *
 * RETURNS:		Various errors.
 *
 ******************************************************************************/
 
int Ext3MkDir(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	struct VNode* vNode;
	struct JournalHandle* handle;
	struct Buffer* buffer;
	struct Ext3DirEntry* entry;
	
	handle=JournalStart(EXT3_JOURNAL(dir), EXT3_DATA_TRANS);
	
	if (!handle)
		return -EIO;
	
	vNode=Ext3CreateINode(handle, dir, 1);
	vNode->size=BYTES_PER_SECTOR(dir->superBlock);
	
	buffer=Ext3BlockRead(vNode, 0, 1);
	
	/* Add . and .. entries. */
	entry=(struct Ext3DirEntry*)(buffer->data);
	
	/* . */
	entry->iNodeNum=vNode->id;
	entry->nameLen=1;
	entry->recLen=EXT3_DIR_REC_LEN(entry->nameLen);
	entry->name[0] = '.';
	entry->fileType=EXT3_FT_DIR;
	
	/* .. */
	entry=(struct Ext3DirEntry*)((char*)entry+entry->recLen);
	entry->iNodeNum=dir->id;
	entry->nameLen=2;
	entry->recLen=BYTES_PER_SECTOR(vNode->superBlock)-EXT3_DIR_REC_LEN(1);
	entry->name[0] = '.';
	entry->name[1] = '.';
	entry->fileType=EXT3_FT_DIR;
	
	JournalGetWriteAccess(handle, buffer);
	JournalDirtyMetadata(handle, buffer);
	
	Ext3DirtyVNode(vNode);
	
	Ext3AddEntry(handle, name, nameLength, dir, vNode);
	
	JournalStop(handle);
	BufferRelease(buffer);
	
	*retVal=vNode;
	
	return 0;
}

int Ext3RemoveDir(struct VNode* dir, char* name, int nameLength)
{
	return -ENOTIMPL;
}
