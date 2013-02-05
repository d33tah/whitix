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
#include <typedefs.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <error.h>
#include <module.h>
#include <print.h>

#include "fat.h"

int FatWriteSuper(struct VfsSuperBlock* superBlock);

struct SuperBlockOps fatSbOps=
{
	.readVNode = FatReadVNode,
	.writeVNode = FatWriteVNode,
	.writeSuper = FatWriteSuper,
};

struct VNodeOps fatVNodeOps=
{
	.create = FatCreate,
	.remove = FatRemove,
	.lookup = FatLookup,
	.mkDir = FatMkDir,
	.rmDir = FatRmDir,
	.truncate = FatTruncate,
	.blockMap = FatBlockMap,
};

/* Update the freeClusters in the FsInfo sector on FAT32 */
int FatWriteSuper(struct VfsSuperBlock* superBlock)
{
	struct FatSbInfo* sbInfo;
	struct FatFsInfo* info;
	struct Buffer* buff;
	DWORD prevSize = superBlock->sDevice->softBlockSize;
	
	sbInfo = FatGetSbPriv(superBlock);

	if (sbInfo->fatType != 32)
		return 0;
	
	/* Need this to write the correct FsInfo sector. It will throw all blocks
	 * out of cache, but if we're calling this, we're either syncing or
	 * freeing the super typically.
	 */
	 
	BlockSetSize(superBlock->sDevice, superBlock->sDevice->blockSize);
	
	buff = BlockRead(superBlock->sDevice, sbInfo->fsInfoSector);
	
	info = (struct FatFsInfo*)(buff->data + 0x1E0);
	info->freeClusters = sbInfo->freeClusters;
	
	BlockWrite(buff->device, buff);
	
	BlockSetSize(superBlock->sDevice, prevSize);
	
	return 0;
}

int FatReadFsInfo(struct VfsSuperBlock* superBlock, unsigned long fsInfoSector)
{
	struct Buffer* buff;
	struct FatFsInfo* info;
	struct FatSbInfo* sbInfo;
	BYTE* data;
	int ret = 0;
		
	buff = BlockRead(superBlock->sDevice, fsInfoSector);
	
	if (!buff)
		return -EIO;

	data = (BYTE*)(buff->data);
	info = (struct FatFsInfo*)(data + 0x1E0);
		
	/* Four byte signature at start of fsInfo sector. */
	if (data[0] != 0x52 || data[1] != 0x52 || data[2] != 0x61 || data[3] != 0x41)
	{
		ret = -EINVAL;
		goto out;
	}
	
	/* TODO: Check info->signature? */
	
	/* Boot record signature at end of fsInfo sector. */
	if (data[510] != 0x55 || data[511] != 0xAA)
	{
		ret = -EINVAL;
		goto out;
	}
	
	/* Store the number of free clusters, and update it again in WriteSuper. */
	sbInfo = FatGetSbPriv(superBlock);
	
	sbInfo->freeClusters = info->freeClusters;
	sbInfo->fsInfoSector = fsInfoSector;
		
out:
	if (ret)
		KePrint(KERN_DEBUG "FAT: Invalid FAT32 filesystem.\n");
		
	BlockFree(buff);
	return ret;
}

struct VfsSuperBlock* FatReadSuper(struct StorageDevice* dev,int flags,char* data)
{
	struct FatBootSector* bootSec;
	struct Buffer* buff;
	struct VfsSuperBlock* retVal;
	struct FatSbInfo* fatSbInfo;
	DWORD clusterCount,totalSectors;

	if (!dev)
		return NULL;

	/* This fixes issue #110. The soft block size at the start should be the
	 * device's sector size as we may read the second sector of the disk in 
	 * FatReadFsInfo. */
	 
	if (BlockSetSize(dev, dev->blockSize))
		return NULL;

	buff = BlockRead(dev, 0);
	
	if (!buff)
	{
		KePrint("FAT: Failed to read superblock\n");
		return NULL;
	}

	bootSec=(struct FatBootSector*)(buff->data);

	if (bootSec->bytesPerSec != 512 && bootSec->bytesPerSec != 1024 && bootSec->bytesPerSec != 2048 && bootSec->bytesPerSec != 4096)
	{
		BlockFree(buff);
		return NULL;
	}

	retVal=VfsAllocSuper(dev,flags);
	if (!retVal)
		goto end;

	retVal->sbOps=&fatSbOps;

	/* Allocate the FAT-specific superblock structure */
	fatSbInfo=(struct FatSbInfo*)MemAlloc(sizeof(struct FatSbInfo));
	if (!fatSbInfo)
		goto fail;

	retVal->privData=(void*)fatSbInfo;

	/* Fill the structure with info */
	fatSbInfo->fatStart=bootSec->reservedSectorCnt;

	if (bootSec->secsPerFat == 0 && bootSec->secsPerFat32 > 0)
	{
		/* Must be FAT32 then */
		fatSbInfo->rootDirStart = bootSec->rootClus;
		fatSbInfo->fatLength = bootSec->secsPerFat32;
	}else{
		fatSbInfo->rootDirStart = bootSec->reservedSectorCnt+(bootSec->numFats*bootSec->secsPerFat);
		fatSbInfo->fatLength = bootSec->secsPerFat;
	}

	totalSectors=(bootSec->totalSecSmall) ? bootSec->totalSecSmall : bootSec->totalSectorsLarge;
	fatSbInfo->rootDirEnts = bootSec->numRootDirEnts;
	fatSbInfo->rootDirLength=(bootSec->numRootDirEnts*sizeof(struct FatDirEntry))/bootSec->bytesPerSec; /* FAT32 doesn't use this */
 	fatSbInfo->firstDataSector=bootSec->reservedSectorCnt+
 		(bootSec->numFats*fatSbInfo->fatLength)+fatSbInfo->rootDirLength;
	fatSbInfo->clusterLength=bootSec->bytesPerSec*bootSec->sectorsPerClus;
	fatSbInfo->secsPerClus=bootSec->sectorsPerClus;
	fatSbInfo->totalDataSectors=totalSectors-fatSbInfo->firstDataSector;
	fatSbInfo->numFats=bootSec->numFats;

	clusterCount=fatSbInfo->totalDataSectors/bootSec->sectorsPerClus;

	/* Set the FAT type and end of cluster marker, depending on the number of clusters */
	if (clusterCount < 4085)
	{
		fatSbInfo->fatType=12;
		fatSbInfo->invalidCluster=0xFF8;
	}else if (clusterCount < 65525)
	{
		fatSbInfo->fatType=16;
		fatSbInfo->invalidCluster=0xFFF8;
	}else{
		fatSbInfo->fatType = 32;
		fatSbInfo->invalidCluster=0x0FFFFFF8;
	}
	
	/* Read the FsInfo structure if this is a FAT32 filesystem, so we can keep
	 * track of the free clusters and update it when necessary. */
	if (fatSbInfo->fatType == 32)
	{
		if (FatReadFsInfo(retVal, bootSec->fsInfo))
			goto fail;
	}
	
	KePrint(KERN_DEBUG "FAT: drive is a FAT%u volume\n",(DWORD)fatSbInfo->fatType);

	if (BlockSetSize(dev,bootSec->bytesPerSec))
		goto fail;

	retVal->mount = VNodeGet(retVal, FAT_ROOT_ID);

	if (!retVal->mount)
		goto fail;

end:
	if (buff->refs)
		BlockFree(buff);
	return retVal;

fail:
	/* privData is freed in VfsFreeSuper */
	VfsFreeSuper(retVal);
	retVal=NULL;
	goto end;
}

static struct FileSystem fatFileSystem={
	.name="FAT",
	.readSuper=FatReadSuper,
};

int FatInit()
{
	return VfsRegisterFileSystem(&fatFileSystem);
}

void FatExit()
{
	VfsDeregisterFileSystem(&fatFileSystem);
}

ModuleInit(FatInit);
