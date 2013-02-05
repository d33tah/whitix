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
#include <malloc.h>
#include <string.h>
#include <typedefs.h>

#include "isofs.h"

int IsoBlockMap(struct VNode* vNode, DWORD block, int flags)
{
	/* Block mapping in ISO filesystems is fairly simple, as the blocks
	 * are stored as a linear array (essentially), starting at firstSector.
	 */

	struct IsoSbPriv* sbPriv=IsoGetSbPriv(vNode->superBlock);

	if (flags & VFS_MAP_CREATE)
		return -ENOENT;

	/* Is the block to be mapped past the end of the file? */
	if ((block*sbPriv->sectorSize) > vNode->size)
		return -1;

	return IsoGetPriv(vNode)->firstSector+block;
}

struct Buffer* IsoBlockRead(struct VNode* vNode,int block)
{
	int sector=IsoBlockMap(vNode, block, 0);

	if (sector == -1)
		return NULL;

	return BlockRead(vNode->superBlock->sDevice,sector);
}

static int IsoParseFileName(char* dest,char* src,int nameLength)
{
	int i;

	for (i=0; i<nameLength; i++)
	{
		dest[i]=tolower(src[i]);
		if (i+3 == nameLength && !strncmp(&src[i],".;1",3))
			goto out;

		if (i+2 == nameLength && !strncmp(&src[i],";1",2))
			goto out;
	}

out:
	dest[i+1]='\0';
	return i;
}

int IsoReadAcrossSector(struct IsoSbPriv* sbPriv, struct VNode* dir, struct Buffer** buffer, struct IsoDirEntry** dirEntry, struct IsoDirEntry** tmpDirEntry, int* sector, int offset)
{
	int firstFrag = sbPriv->sectorSize-offset;
	if (*tmpDirEntry)
		MemFree(*tmpDirEntry);

	*tmpDirEntry=(struct IsoDirEntry*)MemAlloc(sizeof(struct IsoDirEntry));

	if (!*tmpDirEntry)
		return -ENOMEM;
			
	memcpy(*tmpDirEntry, (*buffer)->data+offset, firstFrag);
	BlockFree(*buffer);
			
	(*sector)++;
	*buffer=IsoBlockRead(dir, *sector);
	if (!*buffer)
		return -EIO;
		
	memcpy((char*)((*tmpDirEntry)+firstFrag), (*buffer)->data, (*tmpDirEntry)->entryLength - firstFrag);	
	*dirEntry=*tmpDirEntry;

	return 0;
}

/* Used for just .. and . */

static int IsoRawLookup(struct VfsSuperBlock* superBlock,DWORD startSector,char* name,int nameLength,struct IsoDirEntry** retEntry,DWORD* vNum,DWORD* wantedStart)
{
	int sector=0,offset=0,found=0;
	struct IsoDirEntry* dirEntry;
	struct Buffer* buffer=NULL;
	DWORD start;

	while (1)
	{
		if (!buffer)
		{
			buffer=BlockRead(superBlock->sDevice,startSector+sector);
			if (!buffer)
				return -EIO;
		}

		dirEntry=(struct IsoDirEntry*)(buffer->data+offset);

		/* Both . and .. should be in the first block however */
		if (!dirEntry->entryLength)
		{
			offset=0;
			BlockFree(buffer);
			buffer=NULL;
			sector++;
			continue;
		}

		start=dirEntry->firstSector.native+dirEntry->extAddrLength;

		if (!name)
		{
			if (*wantedStart == start)
				found=1;
		}else if (dirEntry->nameLen == 1)
		{
			if (!dirEntry->name[0] && nameLength == 1 && name[0] == '.')
				found=1;
			else if (dirEntry->name[0] == 1 && nameLength == 2 && !strncmp(name,"..",2))
				found=1;
		}

		if (found)
		{
			if (vNum)
				*vNum=ISO_TO_VNUM(startSector+sector, offset);
			
			if (retEntry)
			{
				*retEntry=(struct IsoDirEntry*)MemAlloc(dirEntry->entryLength);
				memcpy(*retEntry,dirEntry,dirEntry->entryLength);
			}

			if (wantedStart)
				*wantedStart=start;

			return 0;
		}

		offset+=dirEntry->entryLength;
	}

	BlockFree(buffer);
	return -ENOENT;
}

int IsoGetParent(struct VNode* dir,DWORD* vNum)
{
	int err;
	DWORD startSector,gpSector;

	/* Search for the '..' record first in the current directory */
	err=IsoRawLookup(dir->superBlock,IsoGetPriv(dir)->firstSector,"..",2,NULL,NULL,&startSector);
	if (err)
		return err;

	err=IsoRawLookup(dir->superBlock,startSector,"..",2,NULL,NULL,&gpSector);
	if (err)
		return err;

	return IsoRawLookup(dir->superBlock,gpSector,NULL,0,NULL,vNum,&startSector);
}

int IsoLookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	DWORD i=0;
	int err=-ENOENT, sector=0, offset=0;
	struct Buffer* buffer=NULL;
	struct IsoDirEntry* dirEntry, *tmpDirEntry=NULL;
	DWORD vNum;
	struct IsoSbPriv* sbPriv=IsoGetSbPriv(dir->superBlock);
	char aName[256];
	int aNameLen;

	while (i < dir->size)
	{
		if (!buffer)
		{
			buffer=IsoBlockRead(dir,sector);
			if (!buffer)
			{
				err=-EIO;
				goto out;
			}
		}

		dirEntry=(struct IsoDirEntry*)(buffer->data+offset);

		/* TODO: Document. */
		if (!dirEntry->entryLength)
		{
			i+=(sbPriv->sectorSize-offset);
			offset=0;
			BlockFree(buffer);
			buffer=NULL;
			sector++;
			continue;
		}

		i+=dirEntry->entryLength;

		if (offset > sbPriv->sectorSize)
			if ((err=IsoReadAcrossSector(sbPriv, dir, &buffer, &dirEntry, &tmpDirEntry, &sector, offset)))
				goto out;

		/* .. and . cases */
		if (dirEntry->nameLen == 1)
		{
			/* '.' not covered as case is handled by generic filesystem layer */

			if (dirEntry->name[0] == 1 && nameLength == 2 && !strncmp(name,"..",2))
			{
				err=IsoGetParent(dir,&vNum);
				if (err)
					goto out;

				*retVal=VNodeGet(dir->superBlock,vNum);
				err=0;
				goto out;
			}
		}

		aNameLen=IsoParseFileName(aName,dirEntry->name,dirEntry->nameLen);

//		KePrint("%s (%d) %s (%d)\n", name, nameLength, dirEntry->name, aNameLen);

		if (nameLength == aNameLen && !strnicmp(name,dirEntry->name,aNameLen))
		{
			*retVal=VNodeGet(dir->superBlock, ISO_TO_VNUM(IsoBlockMap(dir,sector, 0), offset));
			err=0;
			goto out;
		}
		offset+=dirEntry->entryLength;
	}

out:
	if (tmpDirEntry)
		MemFree(tmpDirEntry);

	if (buffer)
		BlockFree(buffer);

	return err;
}

int IsoReadDir(struct File* file,void* dirEntries)
{
	struct IsoDirEntry* dirEntry, *tmpDirEntry=NULL;
	struct Buffer* buffer=NULL;
	int offset, len;
	struct IsoSbPriv* sbPriv=IsoGetSbPriv(file->vNode->superBlock);
	int sector;
	char name[256];
	int err;
	
	sector = file->position/sbPriv->sectorSize;
	offset = file->position % sbPriv->sectorSize;

	while (file->position < file->vNode->size)
	{
		if (!buffer)
		{
			buffer=IsoBlockRead(file->vNode, sector);
			if (!buffer)
				return -EIO;
		}

		dirEntry=(struct IsoDirEntry*)(buffer->data+offset);

		if (!dirEntry->entryLength)
		{
			file->position+=(sbPriv->sectorSize - offset);
			offset=0;
			BlockFree(buffer);
			buffer=NULL;
			sector++;
			continue;
		}

		offset+=dirEntry->entryLength;

		if (offset > sbPriv->sectorSize)
			if ((err=IsoReadAcrossSector(sbPriv, file->vNode, &buffer, &dirEntry, &tmpDirEntry, &sector, offset)))
				break;

		file->position += dirEntry->entryLength;
		
		if (dirEntry->flags & 0x80)
			continue;

		/* '.' directory */
		if (dirEntry->nameLen == 1 && !dirEntry->name[0])
		{
			if (FillDir(dirEntries,".",1,file->vNode->id))
				break;

			continue;
		}

		/* '..' directory */
		if (dirEntry->nameLen == 1 && dirEntry->name[0] == 1)
		{
			if (FillDir(dirEntries,"..",2,0))
				break;

			continue;
		}

		len=IsoParseFileName(name, dirEntry->name, dirEntry->nameLen);

		if (FillDir(dirEntries, name, len,
			ISO_TO_VNUM(IsoBlockMap(file->vNode,sector, 0),
				offset-dirEntry->entryLength)))
			break;
	}

	if (buffer)
		BlockFree(buffer);

	return 0;
}

struct VNodeOps isoVNodeOps=
{
	lookup: IsoLookup,
	blockMap: IsoBlockMap,
};

struct FileOps isoFileOps=
{
	readDir: IsoReadDir,
	read: FileGenericRead
};
