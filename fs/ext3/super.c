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
#include "ext3.h"
#include <malloc.h>
#include <error.h>
#include <print.h>

int Ext3FreeSuper(struct VfsSuperBlock* superBlock);

struct SuperBlockOps ext3SbOps={
	.readVNode = Ext3ReadVNode,
	.writeVNode = Ext3WriteVNode,
	.dirtyVNode = Ext3DirtyVNode,
	.freeSuper = Ext3FreeSuper,
};

struct VNodeOps ext3VNodeOps={
	.lookup = Ext3Lookup,
	.blockMap = Ext3BlockMap,
	.create = Ext3Create,
	.remove = Ext3Remove,
	.truncate = Ext3Truncate,
	.mkDir = Ext3MkDir,
	.rmDir = Ext3RemoveDir,
};

struct FileOps ext3FileOps={
	.readDir = Ext3ReadDir,
	.read = FileGenericRead,
	.write = Ext3FileWrite,
};

int Ext3FreeSuper(struct VfsSuperBlock* superBlock)
{
	struct Ext3SbInfo* sb=EXT3_SUPERINFO(superBlock);
	
	if (sb && sb->journal)
		JournalDestroy(sb->journal);
	
	if (sb)
		MemFree(sb);
		
	return 0;
}

int Ext3JournalSetup(struct VfsSuperBlock* superBlock, struct Ext3SuperBlock* sb, DWORD iNo)
{
	struct VNode* journal;

	journal=VNodeGet(superBlock, iNo);

	if (!journal)
	{
		KePrint("EXT3: no journal found\n");
		return 0;
	}
	
	if (!journal->size)
	{
		KePrint(KERN_ERROR "EXT3: Zero length journal present\n");
		return -EINVAL;
	}

	/* The journal node must be used (i.e. linked to somewhere). Check! */

	KePrint("EXT3: Found journal at %#X: size = %#X\n", iNo, journal->size);

	/* Let the journal layer handle the replaying. */
	EXT3_SUPERINFO(superBlock)->journal=JournalCreate(journal);
	
	if (!journal)
		return -EIO;
	
	JournalSetUuid(EXT3_SUPERINFO(superBlock)->journal, sb->journalUuid);

	return 0;
}

int Ext3JournalInit(struct VfsSuperBlock* superBlock, struct Ext3SuperBlock* sb)
{
	/* TODO: Recovery? */

	if (sb->journalINum)
	{
		return Ext3JournalSetup(superBlock, sb, sb->journalINum);
	}else
		KePrint("Ext3JournalInit: get dev journal\n");

	return -ENOTIMPL;
}

DWORD Ext3GetDescriptorLoc(struct VfsSuperBlock* superBlock, int i)
{
	struct Ext3SbInfo* sb=EXT3_SUPERINFO(superBlock);
//	unsigned long block;

	return sb->sbSector+i+1;

	/* Check features for meta block group */
#if 0
	block=DESCS_PER_BLOCK(superBlock)*i;

	return sb->firstDataBlock+(block*sb->blocksPerGrp);
#endif
}

int Ext3ReadSuperBlock(struct VfsSuperBlock* superBlock, struct Ext3SbInfo* sbInfo)
{
	int sector, offset;
	
	sector=1*1024/BYTES_PER_SECTOR(superBlock);
	offset=(1*1024) % BYTES_PER_SECTOR(superBlock);
	
	sbInfo->sbBuffer=BlockRead(superBlock->sDevice, sector);

	if (!sbInfo->sbBuffer)
		return -EIO;

	sbInfo->super=(struct Ext3SuperBlock*)((sbInfo->sbBuffer->data)+offset);
	sbInfo->sbSector=sector;

	return 0;	
}

struct VfsSuperBlock* Ext3ReadSuper(struct StorageDevice* dev,int flags,char* data)
{
	struct VfsSuperBlock* retVal;
	struct Buffer* buff;
	struct Ext3SuperBlock sb;
	struct Ext3SbInfo* sbInfo;
	int descCount,i;

	/* Must have a storage device */
	if (!dev || BlockSetSize(dev,1024))
		return NULL;

	/* The Ext3 superblock is located after the first block, which is reserved for boot-
	 * related data. */
	buff=BlockRead(dev, 1);
	if (!buff)
		goto error;

	/* The buffer will be freed if we set the block size to something other than
	 * 1024, so save the data off now.
	 */

	memcpy(&sb, buff->data, sizeof(struct Ext3SuperBlock));

	/* Sanity check. */
	if (sb.magic != EXT3_SB_MAGIC)
		goto blockReadError;

	retVal=VfsAllocSuper(dev,flags);
	if (!retVal)
		goto blockReadError;

	/* Ext3 block sizes are a multiple of 1024. */
	BlockSetSize(dev,1024 << sb.logBlockSize);

	/* Copy some useful information over into the private superblock structure. */
	sbInfo=(struct Ext3SbInfo*)MemAlloc(sizeof(struct Ext3SbInfo));
	if (!sbInfo)
		goto superFreeError;

	sbInfo->iNodesCount=sb.iNodesCount;
	sbInfo->blocksPerGrp=sb.blocksPerGrp;
	sbInfo->groupCount=(sb.blocksCount-sb.firstDataBlock+sb.blocksPerGrp-1)/sb.blocksPerGrp;
	sbInfo->iNodesPerGrp=sb.iNodesPerGrp;

	sbInfo->firstDataBlock=sb.firstDataBlock;
	
	/* Read in the superblock again. */
	Ext3ReadSuperBlock(retVal, sbInfo);

	/* And set the Ext3-specific parts of the superblock structure. */
	retVal->privData=sbInfo;
	retVal->sbOps=&ext3SbOps;

	/* Read in the block descriptors as pointers to buffers. */
	descCount=(sbInfo->groupCount+DESCS_PER_BLOCK(retVal)-1)/DESCS_PER_BLOCK(retVal);
	sbInfo->descs=(struct Buffer**)MemAlloc(descCount*sizeof(struct Buffer*));

	if (!descCount)
		goto superFreeError;

	for (i=0; i<descCount; i++)
		sbInfo->descs[i]=BlockRead(dev, Ext3GetDescriptorLoc(retVal, i));

	/* If we have a journal, read it in. We read the journal in before the root
	 * node, because it may have been modified in the journal. */

	if (EXT3_FEATURE_COMPAT(&sb, EXT3_FEATURE_COMPAT_JOURNAL))
	{
		if (Ext3JournalInit(retVal, &sb))
			goto descsFree;
		
		/* Write journal superblock. */
	}

	retVal->mount=VNodeGet(retVal, EXT3_ROOT_VNO);
	if (!retVal->mount)
		goto descsFree;

	BlockFree(buff);
	return retVal;

descsFree:
	for (i=0; i<descCount; i++)
		BlockFree(sbInfo->descs[i]);

	MemFree(sbInfo->descs);
	/* sbInfo is freed in VfsFreeSuper */
superFreeError:
	VfsFreeSuper(retVal);
blockReadError:
	BlockFree(buff);
error:
	return NULL;
}

VFS_FILESYSTEM(ext3FileSystem, "ext3", Ext3ReadSuper);

int Ext3Init()
{
	return VfsRegisterFileSystem(&ext3FileSystem);
}

ModuleInit(Ext3Init);
