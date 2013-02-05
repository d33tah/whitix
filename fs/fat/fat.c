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
#include <fs/vfs.h>
#include <panic.h>
#include <typedefs.h>
#include <print.h>

#include "fat.h"

/***********************************************************************
 *
 * FUNCTION: FatAccess
 *
 * DESCRIPTION: Get the FAT entry at clusterNum.
 *
 * PARAMETERS: sBlock - FAT superblock that has the FAT.
 *			   clusterNum - offset in the FAT
 *			   newVal - the value for the offset in the FAT.
 *
 * RETURNS: Value at clusterNum.
 *
 ***********************************************************************/

DWORD FatAccess(struct VfsSuperBlock* sBlock,DWORD clusterNum,int newVal)
{
	struct FatSbInfo* fatInfo=FatGetSbPriv(sBlock);
	DWORD first=0,last=0,retVal=0;
	BYTE *pFirst=NULL,*pLast=NULL;
	struct Buffer* buff,*buff2,*copyBuff,*copyBuff2;
	DWORD i;
	int block;

	if (clusterNum > fatInfo->totalDataSectors)
	{
		KePrint("PANIC: FatAccess : cluster number (%#X) > totalDataSectors from %#X\n",clusterNum,__builtin_return_address(0));
		KernelPanic("FatAccess: cluster number > total data sectors");
		cli(); hlt();
	}

	/* Due to the lovely FAT12 format, where entries can strech over blocks, 
	 * 2 blocks have to be read in in the worst case scenario */

	/* Get the index number into the FAT */
	if (fatInfo->fatType == 12)
	{
		first=clusterNum*3/2;
		last=first+1;
	}else if (fatInfo->fatType == 16)
		last=first=clusterNum*2; /* Never goes over a sector boundary */
	else
		/* FAT32 */
		last = first = clusterNum * 4;

	/* Read in the FAT sector(s) concerned */
	buff=BlockRead(sBlock->sDevice,fatInfo->fatStart+(first/BYTES_PER_SECTOR(sBlock)));
	if (!buff)
	{
		KePrint("Failed to read buffer in FatAccess\n");
		return 0;
	}

	/* Is the entry on the same sector? */
	if ((first/BYTES_PER_SECTOR(sBlock)) == (last/BYTES_PER_SECTOR(sBlock)))
		buff2=buff;
	else{
		buff2=BlockRead(sBlock->sDevice,(fatInfo->fatStart+(first/BYTES_PER_SECTOR(sBlock)))+1);
		if (!buff2)
		{
			BlockFree(buff);
			KePrint("Failed to read buffer 2 in FatAccess\n");
			return 0;
		}
	}

	if (fatInfo->fatType == 12)
	{
		/* Slightly confusing */
		pFirst=&((BYTE*)buff->data)[first % BYTES_PER_SECTOR(sBlock)];
		pLast=&((BYTE*)buff2->data)[(first+1) % BYTES_PER_SECTOR(sBlock)];
		if (clusterNum & 1)
			retVal=((*pFirst >> 4) | (*pLast << 4)) & 0xFFF;
		else
			retVal=(*pFirst+(*pLast << 8)) & 0xFFF;
	}else if (fatInfo->fatType == 16)
		retVal=*(WORD*)&buff->data[first % BYTES_PER_SECTOR(sBlock)];
	else if (fatInfo->fatType == 32)
		retVal=(*(DWORD*)&buff->data[first % BYTES_PER_SECTOR(sBlock)]) & 0xFFFFFFF;

	if (newVal != -1)
	{
		if (fatInfo->fatType == 12)
		{
			if (clusterNum & 1)
			{
				*pFirst=(*pFirst & 0xF) | (newVal << 4);
				*pLast=newVal >> 4;
			}else{
				*pFirst=newVal & 0xFF;
				*pLast=(*pLast & 0xF0) | (newVal >> 8);
			}
		}else if (fatInfo->fatType == 16)
		{
			WORD* addr=(WORD*)&buff->data[first % BYTES_PER_SECTOR(sBlock)];
			*addr=(WORD)newVal;
		}else if (fatInfo->fatType == 32)
		{
			DWORD* addr=(DWORD*)&buff->data[first % BYTES_PER_SECTOR(sBlock)];
			*addr=(DWORD)newVal;
		}

		BlockWrite(sBlock->sDevice,buff);
		if (buff != buff2)
			BlockWrite(sBlock->sDevice,buff2);

		/* Copy to the other backup FATs */
		for (i=1; i<fatInfo->numFats; i++)
		{
			block=fatInfo->fatStart+(first/BYTES_PER_SECTOR(sBlock))+fatInfo->fatLength*i;
			if (!(copyBuff=BlockRead(sBlock->sDevice,block)))
				break; /* Ok - doesn't matter too much */

			if (buff != buff2)
			{
				/* FAT12 only */
				if (!(copyBuff2=BlockRead(sBlock->sDevice,block+1)))
				{
					BlockFree(copyBuff);
					break;
				}

				memcpy(copyBuff2->data,buff2->data,BYTES_PER_SECTOR(sBlock));
				BlockWrite(sBlock->sDevice,copyBuff2);
				BlockFree(copyBuff2);
			}

			memcpy(copyBuff->data, buff->data, BYTES_PER_SECTOR(sBlock));
			BlockWrite(sBlock->sDevice,copyBuff);
			BlockFree(copyBuff);
		}
	}

	BlockFree(buff);

	if (buff != buff2)
		BlockFree(buff2);

	return retVal;
}

/***********************************************************************
 *
 * FUNCTION: FatToSector
 *
 * DESCRIPTION: Translate a cluster number to a sector index
 *
 * PARAMETERS: sBlock - FAT superblock in question.
 *			   cluster - cluster to translate.
 *
 * RETURNS: a disk sector index.
 *
 ***********************************************************************/

DWORD FatToSector(struct VfsSuperBlock* sBlock, DWORD cluster)
{
	struct FatSbInfo* fatInfo=FatGetSbPriv(sBlock);

	if (!cluster)
		/* Root directory */
		return fatInfo->rootDirStart;
	else
		return (cluster-2)*(fatInfo->secsPerClus)+fatInfo->firstDataSector;
}

/***********************************************************************
 *
 * FUNCTION: FatGetDirSize
 *
 * DESCRIPTION: Get the size of a directory in directory entries.
 *
 * PARAMETERS: superBlock - superBlock in question.
 *			   cluster - cluster to start at.
 *
 * RETURNS: Size in directory entries.
 *
 ***********************************************************************/

DWORD FatGetDirSize(struct VfsSuperBlock* sBlock, DWORD cluster)
{
	struct FatSbInfo* fatInfo=FatGetSbPriv(sBlock);
	DWORD retVal=0;

	if (fatInfo->fatType != 32 && !cluster)
		return fatInfo->rootDirLength * (BYTES_PER_SECTOR(sBlock)/sizeof(struct FatDirEntry));

	while (cluster < fatInfo->invalidCluster)
	{
		cluster = FatAccess(sBlock, cluster, -1);
		++retVal;
	}

	return retVal * (BYTES_PER_SECTOR(sBlock)/sizeof(struct FatDirEntry)) * fatInfo->secsPerClus;
}

/* Adds one cluster to vNode */
int FatAddCluster(struct VNode* vNode, DWORD* lastCluster)
{
	DWORD i;
	int ret=-1,last=0;
	struct FatVNodeInfo* fatVNodeInfo=FatGetPriv(vNode);
	struct FatSbInfo* sbInfo=FatGetSbPriv(vNode->superBlock);
	struct Buffer* buff;

	DWORD freeClusters = sbInfo->freeClusters;
	
	/* Cannot allocate clusters to the root directory in FAT12/16 */
	if (sbInfo->fatType != 32 && vNode->id == FAT_ROOT_ID)
		return -ENOSPC;

	if (fatVNodeInfo->startCluster)
	{
		/* Find last cluster for the FAT */
		last=fatVNodeInfo->startCluster;
		DWORD val;

		while (1)
		{
			/* This could possibly loop forever */
			val=FatAccess(vNode->superBlock,last,-1);

			if (val >= sbInfo->invalidCluster)
				break;
				
			last=val;
		}
	}

	/* Find a free FAT entry, first two are reserved */
	for (i=2; i<sbInfo->totalDataSectors; i++)
		if (!FatAccess(vNode->superBlock, i, -1))
		{
			ret=i;
			break;
		}

	if (ret == -1)
		return -ENOSPC;

	/* Zero out the new cluster. */
	for (i = 0; i < sbInfo->secsPerClus; i++)
	{
		buff=BlockRead(vNode->superBlock->sDevice, FatToSector(vNode->superBlock, ret) + i);
		ZeroMemory(buff->data, BYTES_PER_SECTOR(vNode->superBlock));
		BlockWrite(vNode->superBlock->sDevice, buff);
		BlockFree(buff);
	}

	/* Now add an EOF marker in that last cluster */
	FatAccess(vNode->superBlock, ret, sbInfo->invalidCluster);

	if (!fatVNodeInfo->startCluster)
		fatVNodeInfo->startCluster=ret;
	else if (last)
		/* Add the new cluster onto the end of last */
		FatAccess(vNode->superBlock, last, ret);

	if (lastCluster)
		*lastCluster=ret;

	freeClusters--;
	
	/* If we've got FAT32 and a valid number of free clusters, update it. */
	if (sbInfo->fatType == 32 && sbInfo->freeClusters != -1)
	{
		sbInfo->freeClusters = freeClusters;
		SuperSetDirty(vNode->superBlock);
	}	

	return 0;
}

/* Remove clusters after 'number' clusters */
int FatRemoveClusters(struct VNode* vNode, int number)
{
	DWORD cluster=0;
	struct FatVNodeInfo* fatVNodeInfo=FatGetPriv(vNode);
	struct FatSbInfo* sbInfo=FatGetSbPriv(vNode->superBlock);
	int fromNum = number;

	DWORD freeClusters = sbInfo->freeClusters;

	if (sbInfo->fatType != 32 && !vNode->id)
		return -EPERM;

	if (!fatVNodeInfo->startCluster)
		return 0;

	if (number)
	{
		cluster=fatVNodeInfo->startCluster;

		while (number)
		{
			cluster=FatAccess(vNode->superBlock, cluster,-1);
			
			freeClusters++;
			
			if (cluster >= sbInfo->invalidCluster)
				return -EINVAL;

			--number;
		}
	}else
		cluster=fatVNodeInfo->startCluster;

	/* And now go through all clusters, getting their next cluster and marking them free */
	while (1)
	{
		/* Mark this cluster to be free - and get the next one */
		cluster=FatAccess(vNode->superBlock, cluster, 0);

		if (cluster >= sbInfo->invalidCluster)
			break;
	}

	if (fatVNodeInfo->startCluster && !fromNum)
		fatVNodeInfo->startCluster=0; /* Since it doesn't have one anymore */

	/* If we've got FAT32 and a valid number of free clusters, update it. */
	if (sbInfo->fatType == 32 && sbInfo->freeClusters != -1)
	{
		sbInfo->freeClusters = freeClusters;
		SuperSetDirty(vNode->superBlock);
	}

	return 0;
}
