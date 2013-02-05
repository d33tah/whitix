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

#include <bitmap.h>
#include <print.h>

#include "ext3.h"

/*******************************************************************************
 *
 * FUNCTION: 	Ext3GetGroupDesc
 *
 * DESCRIPTION: Get the on-disk group descriptor with index 'groupNo'.
 *
 * PARAMETERS: 	superBlock 	- superblock of the filesystem that contains the
 *							  descriptors.
 *			   	groupNo 	- index of the group.
 *				buffer		- return the buffer containing the group on-disk,
 *							  in case the calling function wants to update the
 *							  buffer and call JournalDirtyMetadata. (OPTIONAL)
 *
 * RETURNS: 	A pointer to the group descriptor in the buffer.
 *
 ******************************************************************************/

struct Ext3GroupDesc* Ext3GetGroupDesc(struct VfsSuperBlock* superBlock, DWORD groupNo,
	struct Buffer** buffer)
{
	struct Ext3GroupDesc* descP;
	DWORD groupDesc, desc;
	struct Ext3SbInfo* sbInfo=EXT3_SUPERINFO(superBlock);
	
	/* 
	 * The group descriptors are located after the superblock on disk, and are
	 * read in when the superblock is mounted.
	 */
	 
	if (groupNo >= sbInfo->groupCount)
		return NULL;
	
	/* Sector index of the group */
	groupDesc=groupNo/DESCS_PER_BLOCK(superBlock);
	
	/* Offset within the group */
	desc = groupNo % DESCS_PER_BLOCK(superBlock);
	
	descP=(struct Ext3GroupDesc*)(EXT3_SUPERINFO(superBlock)->descs[groupDesc]->data);
	
	if (buffer)
		*buffer=EXT3_SUPERINFO(superBlock)->descs[groupDesc];
	
	return descP+desc;
}

/*******************************************************************************
 *
 * FUNCTION: 	Ext3BlockLoadBitmap
 *
 * DESCRIPTION: Load the block bitmap pointed to by the group descriptor at index
 *				blockGroup.
 *
 * PARAMETERS: 	superBlock 	- superblock of the filesystem that contains the
 *							  descriptor.
 *			   	groupNo 	- index of the group.
 *
 * RETURNS: 	A buffer pointer to the block bitmap.
 *
 ******************************************************************************/

struct Buffer* Ext3BlockLoadBitmap(struct VfsSuperBlock* superBlock, int blockGroup)
{
	struct Ext3GroupDesc* desc;
	struct Buffer* buffer;
	
	desc=Ext3GetGroupDesc(superBlock, blockGroup, NULL);
	
	buffer=BlockRead(superBlock->sDevice, desc->blockBitmap);

	/* TODO: Keep a cache? */
	
	return buffer;
}

DWORD Ext3BlockNew(struct JournalHandle* handle, struct VNode* vNode)
{
	/* TODO: Better algorithm. Goal block? */
	DWORD i, j=0;
	struct Ext3SbInfo* sbInfo=EXT3_SUPERINFO(vNode->superBlock);
	struct Buffer* groupBuffer, *bitmapBuffer;
	struct Ext3GroupDesc* desc=NULL;
	
	for (i=0; i<sbInfo->groupCount; i++)
	{
		desc=Ext3GetGroupDesc(vNode->superBlock, i, &groupBuffer);
		
		if (desc->freeBlocks)
		{
			/* Read the block bitmap. */
			bitmapBuffer=Ext3BlockLoadBitmap(vNode->superBlock, i);
			
			j=BitFindFirstZero((DWORD*)bitmapBuffer->data, sbInfo->blocksPerGrp);
			
			if (j < sbInfo->blocksPerGrp)
			{
				JournalGetWriteAccess(handle, bitmapBuffer); /* TODO: UndoAccess */
				BmapSetBit((DWORD*)bitmapBuffer->data, j, 1);
				JournalDirtyMetadata(handle, bitmapBuffer);
//				KePrint("Found free zero at %u %u, %#X\n", j, sbInfo->blocksPerGrp, bitmapBuffer);
				break;
			}
		}
	}
	
	if (i == sbInfo->groupCount)
		return 0;
		
	j = j + i*sbInfo->blocksPerGrp + sbInfo->firstDataBlock;
		
	/* Update the group descriptor. */
	JournalGetWriteAccess(handle, groupBuffer);
	desc->freeBlocks--;
	JournalDirtyMetadata(handle, groupBuffer);
	
	/* Update the superblock. */
	JournalGetWriteAccess(handle, sbInfo->sbBuffer);
	sbInfo->super->freeBlocks--;
	JournalDirtyMetadata(handle, sbInfo->sbBuffer);
	
	KePrint("Allocated %u\n", j);
	
	/* Zero the new block out. TODO: Create a "new block" flag for buffer? */
	struct Buffer* buffer=BlockRead(vNode->superBlock->sDevice, j);
	ZeroMemory(buffer->data, BYTES_PER_SECTOR(vNode->superBlock));
	
	return j;
}

int Ext3BlockCreateTable(struct VNode* vNode, struct JournalHandle* handle, DWORD* block)
{
	struct Buffer* buffer;
	
	*block=Ext3BlockNew(handle, vNode);
				
	/* Zero the new block out. */
	buffer=BlockRead(vNode->superBlock->sDevice, *block);
	memset(buffer->data, BYTES_PER_SECTOR(vNode->superBlock), 0);
	
	return 0;
}

/* FIXME: Shorten. */
int Ext3BlockMapHandle(struct JournalHandle* handle, struct VNode* vNode, DWORD block, int flags)
{
	struct Ext3INodeInfo* iNodeInfo=(struct Ext3INodeInfo*)vNode->extraInfo;
	struct Buffer* buffer;
	DWORD entriesPerBlock=(BYTES_PER_SECTOR(vNode->superBlock)/sizeof(unsigned long));
	
//	if (flags & VFS_MAP_CREATE)
//		KePrint("BlockMap(%u)\n", block);
	
	if (block < 12)
	{
		if (!iNodeInfo->blocks[block])
		{
			if (flags & VFS_MAP_CREATE)
			{
				/* Allocate one block. */
				iNodeInfo->blocks[block]=Ext3BlockNew(handle, vNode);
			}else
				return -1;
		}

		/* The first 12 blocks in the inode structure are direct blocks */
		return iNodeInfo->blocks[block];
	}

	block-=12;
	if (block < entriesPerBlock)
	{
		DWORD sector=iNodeInfo->blocks[12];
		
		if (!sector)
		{
			if (flags & VFS_MAP_CREATE)
			{
				Ext3BlockCreateTable(vNode, handle, &sector);
				iNodeInfo->blocks[12]=sector;
			}else
				return -1;
		}
		
		buffer=BlockRead(vNode->superBlock->sDevice, sector);
		if (!buffer)
			return -EIO;

		DWORD* data=(DWORD*)buffer->data;
		
		if (!data[block])
		{
			if (flags & VFS_MAP_CREATE)
			{
				data[block]=Ext3BlockNew(handle, vNode);
				BlockWrite(vNode->superBlock->sDevice, buffer);
			}else
				return -1;
		}

		return data[block];
	}
	
	block-=entriesPerBlock;
	
	if (block < entriesPerBlock*entriesPerBlock)
	{
		DWORD sector=iNodeInfo->blocks[13];
		int second=block/entriesPerBlock;
		int offset=block % entriesPerBlock;
		
		if (!sector)
		{
			if (flags & VFS_MAP_CREATE)
			{
				Ext3BlockCreateTable(vNode, handle, &sector);
				iNodeInfo->blocks[13]=sector;
			}else
				return -1;
		}
		
		buffer=BlockRead(vNode->superBlock->sDevice, sector);
		if (!buffer)
			return -EIO;

		DWORD* data=(DWORD*)buffer->data;
		
		if (!data[second])
		{
			if (flags & VFS_MAP_CREATE)
			{
				Ext3BlockCreateTable(vNode, handle, &data[second]);
			}else
				return -1;
		}
		
		buffer=BlockRead(vNode->superBlock->sDevice, data[second]);
		data=(DWORD*)buffer->data;
		
		if (!data[offset])
		{
			if (flags & VFS_MAP_CREATE)
			{
				data[offset]=Ext3BlockNew(handle, vNode);
				BlockWrite(vNode->superBlock->sDevice, buffer);
			}else
				return -1;
		}
		
		return data[offset];		
	}
	
	KePrint("BlockMap: Here\n");
	cli(); hlt();
	
	return 0;
}

int Ext3BlockFree(struct VfsSuperBlock* superBlock, struct JournalHandle* handle, int blockNo)
{
	DWORD group, offset;
	struct Ext3SbInfo* sbInfo=EXT3_SUPERINFO(superBlock);
	struct Ext3GroupDesc* desc;
	struct Buffer* groupBuffer, *bitmapBuffer;
	
	blockNo-=sbInfo->firstDataBlock;
	
	group=blockNo/sbInfo->blocksPerGrp;
	offset=blockNo % sbInfo->blocksPerGrp;
	
	if (group >= sbInfo->groupCount)
	{
		KePrint("Ext3BlockFree: blockNo = %d, %u %u", blockNo, group, sbInfo->groupCount);
		return -EIO;
	}
	
	desc=Ext3GetGroupDesc(superBlock, group, &groupBuffer);
	
	bitmapBuffer=Ext3BlockLoadBitmap(superBlock, group);
	
	JournalGetWriteAccess(handle, bitmapBuffer); /* TODO: UndoAccess */
	BmapSetBit((DWORD*)bitmapBuffer->data, offset, 0);
	JournalDirtyMetadata(handle, bitmapBuffer);
	
	/* Update the group descriptor. */
	JournalGetWriteAccess(handle, groupBuffer);
	desc->freeBlocks++;
	JournalDirtyMetadata(handle, groupBuffer);
	
	/* Update the superblock. */
	JournalGetWriteAccess(handle, sbInfo->sbBuffer);
	sbInfo->super->freeBlocks++;
	JournalDirtyMetadata(handle, sbInfo->sbBuffer);
	
	return 0;
}

int Ext3BlockTruncate(struct VNode* vNode, int size)
{
	KePrint("%d %d\n", vNode->size, size);
	int startBlock, endBlock;
	int bytesPerSec=BYTES_PER_SECTOR(vNode->superBlock);
	int i;
	struct JournalHandle* handle;
	struct Ext3INodeInfo* info=EXT3_INFO(vNode);
	
	/* TODO: Work out number of blocks needed! */
	handle=JournalStart(EXT3_JOURNAL(vNode), EXT3_RESERVE_TRANS_BLOCKS);
	
	startBlock=size/bytesPerSec;
	endBlock=(size+bytesPerSec+1)/bytesPerSec;
	
	for (i=endBlock-1; i>=startBlock; i--)
	{
		if (i < 12)
		{
			KePrint("Direct block: unmap %d\n", info->blocks[0]);
			Ext3BlockFree(vNode->superBlock, handle, i);
		}else{
			KePrint("Ext3BlockTruncate: todo\n");
			cli(); hlt();
		}
	}
	
	JournalStop(handle);
	
	return 0;
}
