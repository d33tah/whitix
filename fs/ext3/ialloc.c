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
#include <malloc.h>
#include <print.h>

#include "ext3.h"
 
struct Buffer* Ext3INodeLoadBitmap(struct VfsSuperBlock* superBlock, int iNodeGroup)
{
	struct Ext3GroupDesc* desc;
	struct Buffer* buffer;
	
	desc=Ext3GetGroupDesc(superBlock,iNodeGroup, NULL);
	
	buffer=BlockRead(superBlock->sDevice, desc->iNodeBitmap);
	
	/* TODO */
	
	return buffer;
}
 
struct VNode* Ext3CreateINode(struct JournalHandle* handle, struct VNode* dir, int isDir)
{
	DWORD groupNo=0;
	struct Ext3GroupDesc* desc=NULL;
	struct Buffer* groupBuffer=NULL;
	struct Buffer* bitmapBuffer;
	DWORD i;
	struct VfsSuperBlock* superBlock=dir->superBlock;
	struct Ext3SbInfo* sbInfo=EXT3_SUPERINFO(superBlock);
	struct Ext3INodeInfo* iNodeInfo;
	struct VNode* vNode;
	
	if (isDir)
	{
		/* If we're creating a directory inode, search for a inode group that has
		 * a low directory-to-inode ratio.
		 */
		int avgFree=sbInfo->super->freeInodes/sbInfo->groupCount;
		
		for (i=0; i<sbInfo->groupCount; i++)
		{
			desc=Ext3GetGroupDesc(superBlock, i, &groupBuffer);
			
			if (desc && desc->freeINodes && desc->freeINodes >= avgFree)
			{
				groupNo=i;
				break;
			}
		}
	}else{
		/* TODO: May have to search other groups. */
		groupNo=EXT3_INFO(dir)->blockGroup;
	
		desc=Ext3GetGroupDesc(superBlock, groupNo, &groupBuffer);

		if (!desc->freeINodes)
		{
			KePrint("Need to allocate!\n");
			cli(); hlt();
		}
	}

	if (!groupBuffer || !desc)
		return NULL;
	
	bitmapBuffer=Ext3INodeLoadBitmap(superBlock, groupNo);
	
	i=BitFindFirstZero((DWORD*)bitmapBuffer->data, sbInfo->iNodesPerGrp);
	
	if (i < sbInfo->iNodesPerGrp)
	{
		KePrint("Found free zero at %d\n", i);
		JournalGetWriteAccess(handle, bitmapBuffer);
		
		BmapSetBit((DWORD*)bitmapBuffer->data, i, 1);
		
		JournalDirtyMetadata(handle, bitmapBuffer);
	}else{
		KePrint("Ext3CreateINode: todo\n");
	}
	
	/* Update the group descriptor. */
	JournalGetWriteAccess(handle, groupBuffer);
	
	desc->freeINodes--;
	
	if (isDir)
		desc->usedDirs++;
	
	JournalDirtyMetadata(handle, groupBuffer);
	
	/* Update the superblock inode count. */
	JournalGetWriteAccess(handle, sbInfo->sbBuffer);
	
	sbInfo->super->freeInodes--;
	
	JournalDirtyMetadata(handle, sbInfo->sbBuffer);
	
	/* Now allocate and fill in the vNode structure, and let Ext3DirtyVNode sync
	 * it to disk.
	 */
	 
	/* Allocate beforehand? */
	vNode=VNodeAlloc(superBlock, i+groupNo*sbInfo->iNodesPerGrp+1);
	iNodeInfo=(struct Ext3INodeInfo*)MemAlloc(sizeof(struct Ext3INodeInfo));
	
	/* Create an empty vNode. */
	/* Copy EXT3-specific data first. */
	iNodeInfo->blockGroup=groupNo;

	/* Copy over the blocks */
	for (i=0; i<15; i++)
		iNodeInfo->blocks[i]=0;

	/* Copy the iNode times over */
	vNode->aTime.seconds=currTime.seconds;
	vNode->cTime.seconds=currTime.seconds;
	vNode->mTime.seconds=currTime.seconds;

	vNode->vNodeOps=&ext3VNodeOps;
	vNode->fileOps=&ext3FileOps;

	vNode->mode=VFS_ATTR_WRITE | VFS_ATTR_READ;
		
	if (isDir)
		vNode->mode |= VFS_ATTR_DIR;
	else
		vNode->mode |= VFS_ATTR_FILE;
		
	vNode->extraInfo=iNodeInfo;
	
	VNodeUnlock(vNode);
	
	KePrint("%u\n", i+groupNo*sbInfo->iNodesPerGrp+1);
	
//	Ext3DirtyVNode(vNode);
	
	return vNode;
}
