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

#include "reiserfs.h"

/* Document soon */

struct SuperBlockOps reFsSbOps=
{
	.readVNodeWithData = ReFsReadVNode,
};

DWORD ReR5Hash(char* name, int nameLength)
{
	DWORD hash=0;
	int i=0;

	for (i=0; i<nameLength; i++)
	{
		hash += name[i] << 4;
		hash += name[i] >> 4;
		hash *= 11;
	}

	return hash;
}

ReHash ReGetHashFunction(struct ReFsSuperBlock* reFsSuper)
{
	KePrint("hash = %u\n", reFsSuper->sbVer1.hashFuncCode);
	switch (reFsSuper->sbVer1.hashFuncCode)
	{
		case RE_R5_HASH:
			return ReR5Hash;
	}

	return NULL;
}

int ReFsSuperRead(struct VfsSuperBlock* super)
{
	/*
	 * Read the 64th block. (New format.) ReiserFs leaves
 	 * a lot of space for the bootloader.
 	 */

	struct Buffer* buffer;
	struct ReFsSuperBlock* reFsSuper;
	struct ReSuperPriv* superPriv;

	buffer=BlockRead(super->sDevice, 64);

	reFsSuper=(struct ReFsSuperBlock*)(buffer->data);

	/* Check for any ReiserFs magic string. */
	if (strncmp(reFsSuper->sbVer1.magic, "ReIsErFs", 8)
	&& strncmp(reFsSuper->sbVer1.magic, "ReIsEr2Fs", 9)
	&& strncmp(reFsSuper->sbVer1.magic, "ReIsEr3Fs", 9))
		goto freeBuffer;

	/* Okay, there's a ReiserFs block here. Fill in the privData field of SuperBlock. */
	superPriv=(struct ReSuperPriv*)MemAlloc(sizeof(struct ReSuperPriv));
	superPriv->hashFunction=ReGetHashFunction(reFsSuper);
	memcpy(&superPriv->superBlock, &reFsSuper->sbVer1, sizeof(struct ReFsSuperBlockV1));
	super->privData=superPriv;

	/* Set the block size to what the filesystem wants. */
	BlockSetSize(super->sDevice, superPriv->superBlock.blockSize);
	
	return 0;

freeBuffer:
	BlockFree(buffer);
	return -EINVAL;
}

struct VfsSuperBlock* ReFsReadSuper(struct StorageDevice* dev,int flags,char* data)
{
	if (!dev || BlockSetSize(dev, 1024))
		return NULL;

	struct VfsSuperBlock* super=VfsAllocSuper(dev, flags);
	DWORD dirId;

	/* Support the old format soon. */

	if (ReFsSuperRead(super))
		goto err;

	super->sbOps=&reFsSbOps;
	dirId=RFS_ROOT_PARENT_OBJECTID;
	super->mount=VNodeGetWithData(super, 2, &dirId);

	return super;

err:
	VfsFreeSuper(super);
	return NULL;
}

static struct FileSystem reFs={
	.name = "ReiserFs",
	.readSuper = ReFsReadSuper,
};

int ReFsInit()
{
	return VfsRegisterFileSystem(&reFs);
}

ModuleInit(ReFsInit);

