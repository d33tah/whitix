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
#include <error.h>
#include <typedefs.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <module.h>
#include <print.h>

#include "isofs.h"

static inline void IsoTranslateDirEnt(struct VNode* vNode,struct IsoDirEntry* dirEntry)
{
	vNode->mode=((dirEntry->flags & 2) ? VFS_ATTR_DIR : VFS_ATTR_FILE) | VFS_ATTR_READ;
	
	if (dirEntry->flags & 0x80)
	{
		KePrint("dirEntry->flags & 0x80!\n");
		cli(); hlt();
	}else
		vNode->size = dirEntry->length.native;

	/* Set times. */
	vNode->mTime.seconds=vNode->aTime.seconds=vNode->cTime.seconds=IsoConvertDate(dirEntry->date);
}

int IsoReadVNode(struct VNode* vNode)
{
	struct Buffer* buffer;
	DWORD offset=ISO_VNUM_OFFSET(vNode->id);
	struct IsoDirEntry* dirEntry, *tmpDirEntry=NULL;
	struct IsoVNodePriv* isoPriv;
	struct IsoSbPriv* sbPriv=IsoGetSbPriv(vNode->superBlock);
	int err=0;

	buffer=BlockRead(vNode->superBlock->sDevice, ISO_VNUM_SECTOR(vNode->id));

	if (!buffer)
	{
		KePrint("IsoReadVNode: failed to read buffer\n");
		return -EIO;
	}

	dirEntry=(struct IsoDirEntry*)(buffer->data+offset);

	/* Does the directory entry cross a sector boundary? If so,
	 * read in the next sector and piece the entry together. */
	if (offset+dirEntry->entryLength > sbPriv->sectorSize)
	{
		int firstFrag = sbPriv->sectorSize-offset;
		tmpDirEntry=(struct IsoDirEntry*)MemAlloc(sizeof(struct IsoDirEntry));
		
		if (!tmpDirEntry)
		{
			KePrint(KERN_ERROR "ISO: could not allocate directory entry.\n");
			err=-ENOMEM;
			goto out;
		}
			
		memcpy(tmpDirEntry, buffer->data+offset, firstFrag);
		BlockFree(buffer);
		
		buffer=BlockRead(vNode->superBlock->sDevice, ISO_VNUM_SECTOR(vNode->id)+1);
		
		if (!buffer)
		{
			KePrint(KERN_ERROR "ISO: could not read block %u\n", ISO_VNUM_SECTOR(vNode->id)+1);
			err=-EIO;
			goto out;
		}
		
		memcpy((char*)(tmpDirEntry+firstFrag), buffer->data, tmpDirEntry->entryLength - firstFrag);
		dirEntry=tmpDirEntry;
	}

	IsoTranslateDirEnt(vNode, dirEntry);

	vNode->extraInfo=(struct IsoVNodePriv*)MemAlloc(sizeof(struct IsoVNodePriv));

	if (!vNode->extraInfo)
	{
		err=-ENOMEM;
		goto out;
	}

	isoPriv=IsoGetPriv(vNode);
	isoPriv->firstSector=dirEntry->firstSector.native+dirEntry->extAddrLength;

	vNode->vNodeOps=&isoVNodeOps;
	vNode->fileOps=&isoFileOps;

out:
	if (tmpDirEntry)
		MemFree(tmpDirEntry);

	BlockFree(buffer);
	return err;
}

/* TODO: Get the multisession information */

DWORD IsoGetVolStart(struct VfsSuperBlock* superBlock)
{
	return 0;
}

/* ISO volume descriptor types */

#define ISO_VD_PRI	0x01
#define ISO_VD_END	0xFF

#define ISO_VD_IDS	"CD001"

struct SuperBlockOps isoSbOps=
{
	.readVNode = IsoReadVNode,
	.writeVNode = NULL,
};

struct VfsSuperBlock* IsoReadSuper(struct StorageDevice* dev,int flags,char* data)
{
	struct VfsSuperBlock* retVal;
	struct Buffer* buff,*priBuff=NULL;
	struct IsoVolumeDesc* volDesc;
	struct IsoPrimaryDesc* priDesc=NULL;
	struct IsoDirEntry* rootEntry;
	struct IsoSbPriv* sbPriv;
	DWORD currBlock;
	DWORD volStart;

	if (!dev || BlockSetSize(dev,2048))
		return NULL;

	retVal=VfsAllocSuper(dev,flags);

	if (!retVal)
		return NULL;

	retVal->sbOps=&isoSbOps;

	volStart=IsoGetVolStart(retVal);

	for (currBlock=volStart+16; currBlock<volStart+100; currBlock++)
	{
		buff=BlockRead(dev,currBlock);
		if (!buff)
			goto freeSuper;

		volDesc=(struct IsoVolumeDesc*)buff->data;

		if (!strncmp((char*)volDesc->id,ISO_VD_IDS,5))
		{
			if (volDesc->type == ISO_VD_END)
				break;

			if (volDesc->type == ISO_VD_PRI)
			{
				priDesc=(struct IsoPrimaryDesc*)volDesc;
				priBuff=buff;
				buff=NULL; /* So it isn't freed */
			}
		}

		if (buff)
			BlockFree(buff);
	}

	if (!priDesc)
		goto freeSuper;

	rootEntry=&priDesc->root.dirEntry;

	retVal->privData=MemAlloc(sizeof(struct IsoSbPriv));
	sbPriv=IsoGetSbPriv(retVal);
	sbPriv->sectorSize=priDesc->sectorSize.native;
	
	/* Is the sector size less than the hardware block size? If so, error out. */
	if (sbPriv->sectorSize < dev->blockSize)
		goto freeSuper;
		
	BlockSetSize(dev, sbPriv->sectorSize);

	if (sbPriv->sectorSize != 512 && sbPriv->sectorSize != 1024 && sbPriv->sectorSize != 2048)
	{
		KePrint(KERN_ERROR "ISO: Sector size is invalid.\n");
		/* Not a valid size, so pretend we never read it. */
		goto freeSuper;
	}

	retVal->mount=VNodeGet(retVal,ISO_TO_VNUM(rootEntry->firstSector.native,0));
	
	if (!retVal->mount)
	{
		KePrint(KERN_ERROR "ISO: Could not read root vNode.\n");
		goto freeSuper;
	}
	
	/* CDFS is always a read-only filesystem */
	retVal->flags |= SB_RDONLY;

freeBlock:
	if (priBuff)
		BlockFree(priBuff);
	return retVal;

	/* privData is freed in VfsFreeSuper */
freeSuper:
	VfsFreeSuper(retVal);
	retVal=NULL;
	goto freeBlock;
}

static struct FileSystem isoFileSystem={
	.name="CDFS",
	.readSuper=IsoReadSuper,
};

int IsoFsInit()
{
	return VfsRegisterFileSystem(&isoFileSystem);
}

void IsoFsExit()
{
	VfsDeregisterFileSystem(&isoFileSystem);
}

ModuleInit(IsoFsInit);
