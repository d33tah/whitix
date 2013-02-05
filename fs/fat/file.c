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
#include <fs/vfs.h>
#include <error.h>
#include <panic.h>
#include <print.h>

#include "fat.h"

/***********************************************************************
 *
 * FUNCTION: FatBlockRead
 *
 * DESCRIPTION: Take the offset in clusters of a vNode and return a buffer
 *				for a storage device block.
 *
 * PARAMETERS: vNode - vNode to read buffer of.
 *			   offset - offset in clusters.
 *
 * RETURNS: buffer with data on success, -1 on failure.
 *
 ***********************************************************************/

struct Buffer* FatBlockRead(struct VNode* vNode,int offset)
{
	int sector;

	sector=FatBlockMap(vNode, offset, 0);
	
	if (sector == -1) /* Couldn't map the sector */
		return NULL;

	return BlockRead(vNode->superBlock->sDevice, sector);
}

/***********************************************************************
 *
 * FUNCTION: FatBlockMap
 *
 * DESCRIPTION: Take the offset in blocks of a vNode and return a sector
 *				number for a storage device.
 *
 * PARAMETERS: vNode - vNode to find block of.
 *			   offset - offset in blocks.
 *
 * RETURNS: block number on success, -1 on failure.
 *
 ***********************************************************************/

/* FIXME: Needs simplify, especially the main while case. */

int FatBlockMap(struct VNode* vNode, DWORD offset, int flags)
{
	struct FatSbInfo* fatSbInfo=FatGetSbPriv(vNode->superBlock);
	DWORD retVal = FatGetPriv(vNode)->startCluster;
	DWORD origOffset=offset;

	/* 
	 * In FAT12 and FAT16, the root directory is a constant length (the length
	 * is given in the FAT's superblock), so make sure the offset is within limits.
	 * In FAT32, this is not a problem as a root directory operates like a normal
	 * directory.
	 */

	if (vNode->id == FAT_ROOT_ID && fatSbInfo->fatType != 32)
	{
		if (offset >= fatSbInfo->rootDirLength)
			return -1;

		return fatSbInfo->rootDirStart+offset;
	}
	
	offset/=fatSbInfo->secsPerClus;

	/* The vNode might not have a start cluster because it has just been created. */
	if (!retVal)
	{
		if (flags & VFS_MAP_CREATE)
		{
			if (FatAddCluster(vNode, NULL))
				return -1;
				
			return FatToSector(vNode->superBlock, FatGetPriv(vNode)->startCluster) + offset;
		}else
			return -1;
	}

	while (offset--)
	{
		retVal=FatAccess(vNode->superBlock, retVal, -1);
		if (retVal >= fatSbInfo->invalidCluster)
		{
			DWORD lastCluster;

			if (!(flags & VFS_MAP_CREATE))
				return -1;

			offset++;

			/* Add a new cluster if the block requested is one more than needed. */
			while (offset--)
			{
				if (FatAddCluster(vNode, &lastCluster))
					return -1;
			}
			
			return FatToSector(vNode->superBlock, lastCluster)+
				(origOffset % fatSbInfo->secsPerClus);
		}
	}

	return FatToSector(vNode->superBlock, retVal)+(origOffset % fatSbInfo->secsPerClus);
}

struct FileOps fatFileOps=
{
	.read = FileGenericRead,
	.write = FileGenericWrite,
	.readDir = FatReadDir,
};
