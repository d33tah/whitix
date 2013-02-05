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

#include "fat.h"

#include <error.h>
#include <typedefs.h>
#include <fs/vfs.h>
#include <slab.h>
#include <malloc.h>
#include <print.h>

static int FatDirScanSector(struct VfsSuperBlock* superBlock,DWORD sector,const char* name,
							struct FatDirEntry* retVal,DWORD* vNum,DWORD* wantedStart)
{
	DWORD i;
	int found=0;
	struct Buffer* buffer=BlockRead(superBlock->sDevice,sector);
	struct FatDirEntry* curr;
	DWORD startCluster;

	if (!buffer)
		return -EIO;

	for (i=0; i<(BYTES_PER_SECTOR(superBlock)/sizeof(struct FatDirEntry)); i++)
	{
		/* Get the i'th directory in the sector. */
		curr=(struct FatDirEntry*)(((DWORD)buffer->data)+FAT_DIR_POS(i));

		startCluster=((curr->startClusterHigh << 16) | curr->startClusterLow);

		/* Zero is a special mark in the filename. It means there are no more valid directory
		 * entries present in this sector.
		 */
		if (!curr->fileName[0])
		{
			BlockFree(buffer);
			return -EINVAL;
		}

		/* Several sanity checks, to established whether this is a valid directory entry.
		 * (Some directory entries are empty).
		 */
		if (!FAT_VALID_DE(curr))
			continue;

		/* Assume name has been formatted already and is short enough */
		if ((name && !strncmp((const char*)curr->fileName,name, 11))
			|| (!name && *wantedStart == startCluster))
			found=1;

		/* Did we find it? If so, return out of the function, and copy over information about the
		 * directory entry corresponding to the filename.
		 */
		if (found)
		{
			/* If the caller requested it, fill in attributes about the directory entry. */
			if (retVal)
				memcpy(retVal,curr,sizeof(struct FatDirEntry));

			if (vNum)
				*vNum=SS_TO_VNUM(sector,i);

			/* Used when finding out the start of the grandparent directory */
			if (wantedStart)
				*wantedStart=startCluster;

			BlockFree(buffer);
			return 0;
		}
	}
	
	BlockFree(buffer);
	return -ENOENT;
}

static int FatScanDirNonRootRaw(struct VfsSuperBlock* superBlock,int start,const char* name,
								struct FatDirEntry* retVal,DWORD* vNum,DWORD* wantedStart)
{
	DWORD cluster=start;
	int err;
	struct FatSbInfo* fatInfo=FatGetSbPriv(superBlock);
	DWORD i;

	while (1)
	{
		cluster=FatToSector(superBlock,cluster);

		for (i=0; i<fatInfo->secsPerClus; i++)
		{
			err=FatDirScanSector(superBlock,cluster+i,name,retVal,vNum,wantedStart);

			switch (err)
			{
			/* Don't need to do anything */
			case -ENOENT:
				break;

			/* Means that a directory entry with 0 as a beginning character has been reached */
			case -EINVAL:
				return -ENOENT;

			default:
				return err;
			}
		}

		cluster=FatAccess(superBlock,cluster,-1);
		if (cluster >= fatInfo->invalidCluster)
			break;
	}

	return -ENOENT;
}

static int FatScanDirRootRaw(struct VfsSuperBlock* superBlock,int start,const char* name,
							 struct FatDirEntry* retVal,DWORD* vNum,DWORD* wantedStart)
{
	/* Scanning the root directory (of fat12 and fat16) is a special case 
	 * because it is linear and it's not possible to GetNextCluster() on it */
	
	int err=0;
	DWORD i=0;
	struct FatSbInfo* sbInfo=FatGetSbPriv(superBlock);

	while ( i < sbInfo->rootDirLength)
	{
		DWORD cluster=0;
		cluster=start+i;
		err=FatDirScanSector(superBlock,cluster,name,retVal,vNum,wantedStart);

		switch (err)
		{
		/* Don't need to do anything */
		case -ENOENT:
			break;

		/* Means that a directory entry with 0 as a beginning character has been reached */
		case -EINVAL:
			return -ENOENT;

		default:
			return err;
		}

		i++;
	}

	return -ENOENT;
}

static int FatScanDirRaw(struct VfsSuperBlock* superBlock, DWORD start,
	const char* name, struct FatDirEntry* retVal, DWORD* vNum,
	DWORD* wantedStart)
{
	struct FatSbInfo* sbInfo=FatGetSbPriv(superBlock);
	
	if (sbInfo->fatType == 32 && start == 0)
		start = sbInfo->rootDirStart;
	
	if (sbInfo->fatType != 32 && start == sbInfo->rootDirStart)
		return FatScanDirRootRaw(superBlock,start,name,retVal,vNum,wantedStart);
	else
		return FatScanDirNonRootRaw(superBlock,start,name,retVal,vNum,wantedStart);
}

DWORD FatTranslate(struct FatDirEntry* dirEntry)
{
	DWORD retVal=0;
	
	if (FAT_ISREGULAR(dirEntry->attribute))
		retVal|=VFS_ATTR_FILE;
	else if (FAT_ISDIR(dirEntry->attribute))
		retVal |= VFS_ATTR_DIR;
	else
		KePrint("FAT: invalid directory entry attribute?\n");

	retVal |= VFS_ATTR_READ;

	if (!(FAT_ISRDONLY(dirEntry->attribute))
		retVal |= VFS_ATTR_WRITE;

	return retVal;
}

int FatParentVNo(struct VNode* dir, DWORD* vNum)
{
	int err;
	DWORD startCluster=0,gpCluster;
	struct FatSbInfo* info;
	
	info = FatGetSbPriv(dir->superBlock);

	/* Is this case ever reached? */
	if (dir->id == FAT_ROOT_ID)
		return FAT_ROOT_ID;

	/* Get the start of the parent directory entry */
	err=FatScanDirRaw(dir->superBlock, FatGetPriv(dir)->startCluster,
		FAT_DOTDOT, NULL, NULL, &startCluster);
		
	if (err)
		return err;
		
	/* Is the root directory then */
	if (!startCluster)
		return FAT_ROOT_ID;

	err=FatScanDirRaw(dir->superBlock,startCluster,FAT_DOTDOT,NULL,NULL,&gpCluster);
	
	if (err)
		return err;

	/* Now find the vNode id corresponding to the start cluster of the parent directory */
	err=FatScanDirRaw(dir->superBlock,gpCluster,NULL,NULL,vNum,&startCluster);

	return err;
}

int FatLookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	DWORD vNum=0;
	int err;

	if (nameLength == 2 && name[0] == '.' && name[1] == '.')
	{
		/* Find the (proper) parent vNode number, and jump ahead. */
		err=FatParentVNo(dir, &vNum);
		
		if (err)
			return err;

		goto getVNode;
	}
	
	/* Otherwise, for a normal entry, just scan the directory. */
	err=FatScanDir(dir, name, nameLength, NULL, &vNum, NULL);

	if (err)
		return err;

getVNode:
	*retVal=VNodeGet(dir->superBlock, vNum);
	return 0;
}

/* Convert from a short filename to a normal C-terminated string. */

int FatNameToStr(char* fat12Name,char* name)
{
	int i=0;

	ZeroMemory(name, 12);

	while (i < 8 && fat12Name[i] != ' ')
	{
		name[i]=tolower(fat12Name[i]);
		++i;
	}

	name[i]='\0';

	if (fat12Name[8] != ' ') /* Has extension */
	{
		name[i]='.';
		int j=i+1;
		i=8;

		while (i < 11 && fat12Name[i] != ' ')
		{
			name[j]=tolower(fat12Name[i]);
			++i;
			++j;
		}

		name[j]='\0';
	}
	
	return 0;
}

int FatCreateEntry(struct VNode* dir,char* name,int nameLength,int attr,DWORD* vNum)
{
	char fatName[11];
	int err, isShortName = 1;

	if (FatValidLongName(name,nameLength))
		return -EINVAL;
	
	/* Creates a VFAT-friendly short name (~1 etc.) */
	if (FatCreateShortName(dir,fatName,name,nameLength, &isShortName))
		return -EINVAL;

	err = FatAddName(dir, fatName, name, nameLength, vNum, attr, isShortName);

	return err;
}

/* Illegal characters in a FAT filename */
char* invalidChars="\"*+,/[]<=>:;?|\\";

int FatIsInvalidChar(int c)
{
	return (strchr(invalidChars, c) || (c < 0x20));
}

int FatSkipChar(int c)
{
	return (c == '.' || c == ' ');
}

int FatIsLong(const char* name,int nameLength)
{
	return (FatBaseNameLen(name,nameLength) > 8 || FatExtensionLen(name,nameLength) > 3);
}

int FatCreateShortName(struct VNode* vNode,char* fatName,char* name,int nameLength, int* isShortName)
{
	int i=0;
	int numTailBaseLen = 6, numTail2BaseLen = 2, baseLen = 0, extLen = 0;
	int baseValid = 1, extValid = 1;
	char base[8], ext[3];
	char* extStart, *end;
	int size = 0;
	
	/* Find the extension, and make a note of its place in the string. */
	extStart = end = &name[nameLength];
	
	while (--extStart >= name)
	{
		if (*extStart == '.')
		{
			/* Is there a full stop at the end of the name? If so, ignore it,
			 * and consider there to be no extension. */
			if (extStart == end - 1)
			{
				size = nameLength;
				extStart = NULL;
			}
			
			break;
		}
	}
	
	/* If we didn't find an extension in the name, the whole string will be the
	 * shortname. */
	if (extStart == name - 1)
	{
		size = nameLength;
		extStart = NULL;
	}else if (extStart)
	{
		char* nameStart;
		
		nameStart = name;
		
		/* Skip full stops and spaces at the start of the name, and use the rest
		 * of the name to build the shortname. */
		while (nameStart < extStart)
		{
			if (!FatSkipChar(*nameStart))
				break;
				
			nameStart++;
		}
		
		if (nameStart != extStart)
		{
			size = extStart - name;
			extStart++;
		}else{
			size = nameLength;
			extStart = NULL;
		}
	}
	
	for (baseLen = i = 0; i < size && i < 8; i++)
	{
		if (FatIsInvalidChar(name[i]))
			return -EINVAL;
			
		if (FatSkipChar(name[i]))
		{
			baseValid = 0;
			continue;
		}
			
		if (baseLen < 2 && (baseLen + 1) > 2)
			numTail2BaseLen = baseLen;
		
		if (baseLen < 6 && (baseLen + 1) > 6)
			numTailBaseLen = baseLen;
			
		base[baseLen] = name[i];
		baseLen++;
	}

	if (!baseLen)
		return -EINVAL;
	
	if (i < size)
		*isShortName = 0;
		
	if (extStart)
	{
		char* extension = extStart;
		
		for (i=0; i<3 && extension < end; i++)
		{
			if (FatIsInvalidChar(name[i]))
				return -EINVAL;
				
			if (FatSkipChar(name[i]))
			{
				extValid = 0;
				continue;
			}
			
			ext[extLen] = *extension++;
			extLen++;
		}
		
		if (extStart+extLen < end)
			*isShortName = 0;
	}
	
	memset(fatName, 11, ' ');
	memcpy(fatName, base, baseLen);
	memcpy(fatName+8, ext, extLen);

	if (*isShortName == 1 && baseValid && extValid)
		return 0;
	
	*isShortName = 0; /* TODO: Check. */
		
	if (baseLen > 6)
	{
		baseLen = numTailBaseLen;
		fatName[7] = ' ';
	}

	/* Add ~x to fatName */
	fatName[baseLen]='~';
	
	for (i=0; i<10; i++)
	{
		/* Try first few, and then go random */
		fatName[baseLen + 1]= '1' + i;
		if (FatScanDirRaw(vNode->superBlock,
				FatGetPriv(vNode)->startCluster,
				fatName, NULL, NULL, NULL) == -ENOENT)
			break;
	}
	
//	KePrint("fatName = '%.11s', from %s\n", fatName, name);

	return 0;
}

/* Perform various sanity checks on the given filename. */

int FatValidLongName(char* name, int nameLength)
{
	/* Is it too long? */
	if (name[nameLength-1] == ' ' || nameLength > 255)
		return -EINVAL;

	/* Is it the same name as some special DOS files? */
	if (nameLength == 3 || (nameLength > 3 && name[3] == '.'))
	{
		if (!strnicmp(name,"aux",3) || !strnicmp(name,"con",3)
			|| !strnicmp(name,"nul",3) || !strnicmp(name,"prn",3))
				return -EINVAL;
	}

	/* As above, but is the name com0, com1, ... or lpt0, lpt1, ..? */
	if (nameLength == 4 || (nameLength > 4 && name[4] == '.'))
	{
		if ('1' <= name[3] && name[3] <= '9')
		{
			if (!strnicmp(name,"com",3) ||
				!strnicmp(name,"lpt",3))
					return -EINVAL;
		}
	}

	return 0;
}

int FatAddEntries(struct VNode* dir, int numEntries)
{
	struct FatDirEntry currDirEntry;
	int pos=0,currNum=0,offset=0;

	if (!numEntries)
		return -EINVAL;

	/* Get numEntries number of directory entries following each other. */
	while (!FatGetDirEntry(dir, &currDirEntry, FAT_DIR_POS(pos)))
	{
		pos ++;

		if (!FAT_VALID_DE(&currDirEntry))
		{
			if (++currNum == numEntries)
				return offset;
		}else{
			currNum=0;
			offset=pos;
		}
	}

	/* In FAT12 and FAT16, it's not possible to extend the root directory */
	if (!dir->id && (FatGetSbPriv(dir->superBlock)->fatType != 32))
		return -ENOSPC;

	/* numEntries*size of FAT dir entry must be less than cluster size. */
	DWORD clustersToCreate=(numEntries*sizeof(struct FatDirEntry))/BYTES_PER_SECTOR(dir->superBlock);

	while (clustersToCreate--)	
		FatAddCluster(dir, NULL);

	pos ++;

	return pos;
}

int FatAddShortName(struct VNode* dir,char* fatName,int offset,DWORD* vNum,int attr)
{
	struct Buffer* buffer;
	struct FatDirEntry* entry;
	int sector,sOffset;

	if (offset == -1)
	{
		offset=FatAddEntries(dir,1);
		if (offset < 0)
			return offset;
	}

	sector=FAT_DIR_POS(offset)/BYTES_PER_SECTOR(dir->superBlock);
	sOffset = FAT_DIR_POS(offset) % BYTES_PER_SECTOR(dir->superBlock);

	buffer=FatBlockRead(dir,sector);
	entry=(struct FatDirEntry*)((char*)buffer->data+sOffset);

	memcpy(entry->fileName, fatName, 11);
	entry->startClusterLow=entry->startClusterHigh=0;
	entry->fileSize=0;
	entry->attribute=attr;

	*vNum=SS_TO_VNUM(FatBlockMap(dir,sector, 0),sOffset/sizeof(struct FatDirEntry));

	BlockWrite(dir->superBlock->sDevice,buffer);
	BlockFree(buffer);

	return 0;
}

void FatCopyLongName(struct FatDirSlot* slot,char* src,int copyOffset,int nameLength)
{
	int i;

	for (i=0; i<5; i++)
	{
		if (copyOffset+i < nameLength)
			slot->name[i*2]=src[copyOffset+i];
	}

	for (i=0; i<6; i++)
	{
		if (copyOffset+i+5 < nameLength)
			slot->name2[i*2]=src[copyOffset+5+i];
	}

	for (i=0; i<2; i++)
	{
		if (copyOffset+i+11 < nameLength)
			slot->name3[i*2]=src[copyOffset+11+i];
	}
}

/* Function should be cleared up soon */

int FatAddName(struct VNode* dir, char* fatName, char* name, int nameLength,
	DWORD* vNum, int attr, int isShortName)
{
	/* Search for nameLength/13 entries to put name in, but round up */
	int numEntries=(nameLength+12)/13;
	int offset,i,currSlotNo=0;
	BYTE checkSum;
	struct FatDirSlot* slot;
	int lastOffset=-1;
	struct Buffer* buffer;
	int sector,sOffset;

	/* Build long file name entry */
	if (!isShortName)
	{
		offset=FatAddEntries(dir, numEntries);
		if (offset < 0)
			return offset;

		/* Checksum the name */
		checkSum=FatCheckSum(fatName);

		/* And write the entries to disk */
		for (i=numEntries; i>0; i--)
		{
			sector=FAT_DIR_POS(offset+currSlotNo)/BYTES_PER_SECTOR(dir->superBlock);
			sOffset=FAT_DIR_POS(offset+currSlotNo) % BYTES_PER_SECTOR(dir->superBlock);

			buffer=FatBlockRead(dir, sector);
			if (!buffer)
				return -EIO;

			slot=(struct FatDirSlot*)((char*)buffer->data+sOffset);
			ZeroMemory(slot,sizeof(struct FatDirSlot));

			slot->id=i;

			if (i == numEntries)
				slot->id |= 0x40;

			slot->checkSum=checkSum;
			slot->attr=ATTR_EXT;
			slot->start=0;

			FatCopyLongName(slot, name, (i-1)*13, nameLength);

			BlockWrite(dir->superBlock->sDevice,buffer);
			BlockFree(buffer);
			if (i == 1)
				lastOffset=currSlotNo+offset+1;
				
			currSlotNo++;
		}
	}

	return FatAddShortName(dir, fatName, lastOffset, vNum, attr);
}

/* Creates a file */

int FatCreate(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	int err;
	DWORD vNum;

	err=FatCreateEntry(dir, name, nameLength,ATTR_ARCHIVE,&vNum);
	
	if (err)
		return err;

	*retVal=VNodeGet(dir->superBlock,vNum);

	return 0;
}

/* Size of a FAT directory after MkDir. Two directory entries for . and .. */
#define FAT_MKDIR_SIZE		64

int FatMkDir(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	int err;
	DWORD vNum;
	struct FatDirEntry* dirEntry;
	struct Buffer* buffer;

	/* Create the directory and get a vNode reference to it */
	err=FatCreateEntry(dir, name, nameLength, ATTR_DIR, &vNum);
	if (err)
		return err;

	*retVal=VNodeGet(dir->superBlock,vNum);

	/* Add a starting cluster so directory entries can be added. */
	err=FatAddCluster(*retVal, NULL);
	if (err < 0)
		goto error;

	/* Add the '.' entry */
	err=FatAddShortName(*retVal,FAT_DOT,-1,&vNum,ATTR_DIR);
	
	if (err)
		goto error;

	buffer=BlockRead(dir->superBlock->sDevice,VNUM_TO_SECTOR(vNum));
	
	dirEntry=(struct FatDirEntry*)(buffer->data+FAT_DIR_POS(VNUM_TO_OFFSET(vNum)));

	dirEntry->startClusterHigh=FatGetPriv(*retVal)->startCluster >> 16;
	dirEntry->startClusterLow=FatGetPriv(*retVal)->startCluster & 0xFFFF; /* i.e. start cluster of the directory */
	dirEntry->fileSize=0;

	/* Add the '..' entry */
	err=FatAddShortName(*retVal, FAT_DOTDOT, -1, &vNum, ATTR_DIR);
	if (err)
		goto error;

	dirEntry=(struct FatDirEntry*)(buffer->data+FAT_DIR_POS(VNUM_TO_OFFSET(vNum)));
	
	if (dir->id != FAT_ROOT_ID)
	{
		dirEntry->startClusterHigh=FatGetPriv(dir)->startCluster >> 16;
		dirEntry->startClusterLow=FatGetPriv(dir)->startCluster & 0xFFFF;
	}else{
		/* If the parent is the root directory, the start cluster should
		 * always point to 0, even on FAT32, where the root directory has its
		 * own start cluster.
		 */
		dirEntry->startClusterHigh = 0;
		dirEntry->startClusterLow = 0;
	}
	
	dirEntry->fileSize=0;

	(*retVal)->size=FAT_MKDIR_SIZE;

	/* Write the directory entry out */
	BlockWrite(dir->superBlock->sDevice, buffer);
	BufferRelease(buffer);

	return 0;
	
error:
	KePrint(KERN_DEBUG "Could not add %s, %d\n", name, err);
	return err;
}

int FatRemoveEntry(struct VNode* dir,int sector,int offset)
{
	struct Buffer* buffer=BlockRead(dir->superBlock->sDevice,sector);
	if (!buffer)
		return -EIO;

	struct FatDirEntry* dirEntry=(struct FatDirEntry*)((char*)buffer->data+FAT_DIR_POS(offset));
	ZeroMemory(dirEntry,sizeof(struct FatDirEntry));
	dirEntry->fileName[0]=0xE5; /* Mark as invalid */

	BlockWrite(dir->superBlock->sDevice,buffer);
	BlockFree(buffer);

	return 0;
}

int FatRemoveLongEntry(struct VNode* dir,char* name,int nameLength)
{
	int err,extLen;
	int sec,offset;
	DWORD currOffset,vNum;
	struct FatDirEntry dirEntry;

	/* Get the offset of the directory entry */
	err=FatScanDir(dir,name,nameLength,NULL,&vNum,&currOffset);
	if (err)
		return err;

	err=FatRemoveEntry(dir, VNUM_TO_SECTOR(vNum), VNUM_TO_OFFSET(vNum));
	if (err)
		return err;

	/* Remove ATTR_EXT entries if need be. Short names may have them. */
	extLen=(nameLength+12)/13;

	currOffset--;

	while (extLen)
	{
		if (FatGetDirEntry(dir,&dirEntry, FAT_DIR_POS(currOffset)))
			break;

		/* May not even have a long file name entry */
		if (dirEntry.attribute != ATTR_EXT)
			break;

		FatPosToSecOffset(dir,currOffset,&sec,&offset);

		FatRemoveEntry(dir, sec, offset);

		currOffset--;
		extLen--;
	}

	return 0;
}

int FatRemove(struct VNode* dir,char* name,int nameLength)
{
	struct VNode* vNode;
	int err;

	err=FatLookup(&vNode,dir,name,nameLength);

	if (err)
		return err;

	if (vNode->refs > 1)
	{
		VNodeRelease(vNode);
		return -EACCES;
	}

	FatRemoveClusters(vNode,0);
	vNode->size=0;
	SetVNodeDirty(vNode);
	VNodeRelease(vNode); /* Update to disk */

	return FatRemoveLongEntry(dir,name,nameLength);
}

/***********************************************************************
 *
 * FUNCTION: FatDirEmpty
 *
 * DESCRIPTION: Is the directory empty? Used when removing a directory.
 *
 * PARAMETERS: dir - directory to check.
 *
 * RETURNS: 0 if empty, 1 if not empty.
 *
 ***********************************************************************/

static int FatDirEmpty(struct VNode* dir)
{
	struct FatDirEntry dirEntry;
	int dirPos=0;

	while (!FatGetDirEntry(dir, &dirEntry, FAT_DIR_POS(dirPos++)))
	{
		if (!FAT_VALID_DE(&dirEntry))
			continue;

		if (strncmp(dirEntry.fileName,FAT_DOT,11) &&
			strncmp(dirEntry.fileName,FAT_DOTDOT,11))
			return 1;
	}

	return 0;
}

int FatRmDir(struct VNode* dir,char* name,int nameLength)
{
	struct VNode* vNode;
	int err;

	err=FatLookup(&vNode,dir,name,nameLength);

	if (err)
		return err;

	/* If the directory is empty, it cannot be removed */
	if (FatDirEmpty(vNode) || vNode->refs > 1)
	{
		VNodeRelease(vNode);
		return -EACCES;
	}

	FatRemoveClusters(vNode,0);
	vNode->size=0;
	SetVNodeDirty(vNode);
	VNodeRelease(vNode); /* Update to disk */

	return FatRemoveLongEntry(dir,name,nameLength);
}

/* Inspiration taken from Linux - since it's about the only way to do it anyway.
   Tidy up. */

static int dayTotal[]={
	0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0
};

DWORD FatDtToUnix(WORD date,WORD time)
{
	int month,year,secs;
	
	month=((date >> 5)-1) & 15;
	year=date >> 9;
	secs=(time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600+86400*
		((date & 31)-1+dayTotal[month]+(year/4)+year*365-((year & 3) == 0
		&& month < 2 ? 1 : 0)+3653);

	return secs;
}

int FatReadVNode(struct VNode* vNode)
{
	struct FatSbInfo* sbInfo;
	struct FatVNodeInfo* info;
	DWORD sector,subSec;
	struct Buffer* buffer;
	struct FatDirEntry* dirEntry;

	if (!vNode)
		return -EFAULT;

	sbInfo=(struct FatSbInfo*)vNode->superBlock->privData;

	if (vNode->id == FAT_ROOT_ID)
	{
		vNode->mode=VFS_ATTR_DIR | VFS_ATTR_WRITE | VFS_ATTR_READ;
		vNode->extraInfo=MemAlloc(sizeof(struct FatVNodeInfo));
		info=(struct FatVNodeInfo*)vNode->extraInfo;
		if (sbInfo->fatType == 32)
		{
			info->startCluster = sbInfo->rootDirStart;
			vNode->size = sbInfo->rootDirLength = FatGetDirSize(vNode->superBlock,info->startCluster);
		}else{
			info->startCluster=0;
			vNode->size=sbInfo->rootDirEnts; /* FIXME: Check. */
		}

		/* No real information anywhere for times in the root directory */
	}else{
		/* A normal node */

		/* Read in directory entry */
		sector=VNUM_TO_SECTOR(vNode->id);
		subSec=VNUM_TO_OFFSET(vNode->id)*sizeof(struct FatDirEntry);

		buffer=BlockRead(vNode->superBlock->sDevice,sector);
		if (!buffer)
			return -EIO;

		dirEntry=(struct FatDirEntry*)((char*)buffer->data+subSec);

		vNode->extraInfo=MemAlloc(sizeof(struct FatVNodeInfo));
		info=FatGetPriv(vNode);
		
		info->startCluster=(dirEntry->startClusterHigh << 16) | dirEntry->startClusterLow;
		vNode->mode=FatTranslate(dirEntry);

		/* If it is a file, set the size to the size of the file, but, if it is a directory, 
		calculate the whole cluster length of the directory */
		if (vNode->mode & VFS_ATTR_DIR)
			vNode->size=FatGetDirSize(vNode->superBlock,info->startCluster);
		else if (vNode->mode & VFS_ATTR_FILE)
			vNode->size=dirEntry->fileSize;
		
		vNode->mTime.seconds=vNode->aTime.seconds=FatDtToUnix(dirEntry->lastWriteDate,dirEntry->lastWriteTime);
		vNode->cTime.seconds=FatDtToUnix(dirEntry->creationDate,dirEntry->creationTime);
		vNode->cTime.useconds=dirEntry->creationTimeMs;

		BlockFree(buffer);
	}

	vNode->vNodeOps=&fatVNodeOps;
	vNode->fileOps=&fatFileOps;

	return 0;
}

void FatUnixToDt(DWORD unixTime,WORD* fatDate,WORD* fatTime)
{
	int day,year,nlDay,month;

	if (unixTime < 315532800)
		unixTime=315532800;

	*fatTime=(unixTime % 60)/2+(((unixTime/60) % 60) << 5)+
		(((unixTime/3600) % 24) << 11);
	day=unixTime/86400-3652;
	year=day/365;

	if ((year+3)/4+365*year > day)
		year--;

	day-=(year+3)/4+365*year;

	if (day == 59 && !(year & 3))
	{
		nlDay=day;
		month=2;
	}else{
		nlDay=(year & 3) || day <= 59 ? day : day-1;
		for (month=0; month<12; month++)
			if (dayTotal[month] > nlDay)
				break;
	}

	*fatDate=nlDay-dayTotal[month-1]+1+(month << 5)+(year << 9);
}

int FatWriteVNode(struct VNode* vNode)
{
	int err;
	struct Buffer* buffer;
	struct FatDirEntry* dirEntry;
	struct FatVNodeInfo* info=FatGetPriv(vNode);

	/* Can't really update the root directory */
	if (vNode->id == FAT_ROOT_ID)
		return 0;

	buffer=BlockRead(vNode->superBlock->sDevice, VNUM_TO_SECTOR(vNode->id));

	if (!buffer)
		return -EIO;

	dirEntry=(struct FatDirEntry*)((char*)buffer->data+
		FAT_DIR_POS(VNUM_TO_OFFSET(vNode->id)));

	/* Update the size and start cluster */
	if (vNode->mode & VFS_ATTR_DIR)
		dirEntry->fileSize=0;
	else
		dirEntry->fileSize=vNode->size;
		
	dirEntry->startClusterLow=info->startCluster & 0xFFFF;
	dirEntry->startClusterHigh=info->startCluster >> 16;

	/* Update the times */
	FatUnixToDt(vNode->mTime.seconds,&dirEntry->lastWriteDate,&dirEntry->lastWriteTime);
	FatUnixToDt(vNode->cTime.seconds,&dirEntry->creationDate,&dirEntry->creationTime);
	dirEntry->accessDate=dirEntry->lastWriteDate;

	err=BlockWrite(vNode->superBlock->sDevice,buffer);
	BlockFree(buffer);
	return err;
}

int FatTruncate(struct VNode* vNode, DWORD size)
{
	struct FatSbInfo* sbInfo=FatGetSbPriv(vNode->superBlock);
	DWORD clusterNum=(size+(sbInfo->clusterLength-1))/sbInfo->clusterLength;
	/* Truncate vNode to it's new size, round up to the nearest cluster */

	FatRemoveClusters(vNode, clusterNum);

	return 0;
}
