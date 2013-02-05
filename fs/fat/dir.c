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

#include <console.h>
#include <error.h>
#include <typedefs.h>

/***********************************************************************
 *
 * FUNCTION: FatGetDirEntry
 *
 * DESCRIPTION: Get the directory entry at currPos
 *
 * PARAMETERS: vNode - vNode to find directory entry in.
 *			   dirEntry - directory entry structure to fill in.
 *			   currPos - current position in the vNode.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int FatGetDirEntry(struct VNode* vNode,struct FatDirEntry* dirEntry, int currPos)
{
	struct Buffer* buff;
	int offset;

	if (!vNode || !vNode->superBlock)
		return -EFAULT;

	/* Read the sector that corresponds to our offset in the directory. */
	buff=FatBlockRead(vNode, currPos / BYTES_PER_SECTOR(vNode->superBlock));
	
	if (!buff)
		return -EIO;

	/* And calculate the sector offset that corresponds to our offset in the directory. */
	offset=currPos & (BYTES_PER_SECTOR(vNode->superBlock)-1);

	/* Using the sector offset, copy the directory entry over to the caller-supplied parameter. */
	memcpy((char*)dirEntry, (char*)((DWORD)buff->data+offset), sizeof(struct FatDirEntry));

	BlockFree(buff);

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: FatGetLongName
 *
 * DESCRIPTION: Get the long file name for a directory entry.
 *
 * PARAMETERS: vNode - directory vfs node.
 *			   pos - position in vNode.
 *			   longName - buffer to return name into.
 *			   currDirEntry - current directory entry to return info to.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int FatGetLongName(struct VNode* vNode, DWORD* pos,char* longName,struct FatDirEntry* currDirEntry)
{
	/* FatDirSlot is the alternative structure for a long filename entry (i.e. it also looks like a
	 * FatDirEntry to software that doesn't support long filenames. . A long filename's layout
	 * on-disk is essentially a linked list of FatDirSlots. The structure encodes useful
	 * information about LFNs, such as the number of slots remaining in the chain. */

	struct FatDirSlot* slot=(struct FatDirSlot*)currDirEntry;
	DWORD numSlots=FAT_LONG_ENTRIES(slot->id);
	BYTE checkSum=slot->checkSum,shortCheckSum;
	int offset,i;

	/* Is it actually a FatDirSlot? Several sanity checks here. */
	if (!(FAT_IS_LONG(slot->id)) || !numSlots || numSlots > 20)
		return -EINVAL;

	while (1)
	{
		numSlots--;
		offset=numSlots*13; /* 13 characters are stored per directory slot */

		/* There are three different names entries in the FatDirSlot structure. Copy
		 * each into the long name, taking account of offsets. */
		for (i=0; i<5; i++)
			longName[offset+i]=slot->name[i*2];

		for (i=0; i<6; i++)
			longName[offset+5+i]=slot->name2[i*2];

		for (i=0; i<2; i++)
			longName[offset+11+i]=slot->name3[i*2];

		/* End of name */
		if (slot->id & 0x40)
			longName[offset+13]='\0';

		/* Why GetDirEntry here? Document. */
		FatGetDirEntry(vNode, currDirEntry, FAT_DIR_POS(*pos));
		
		++(*pos);

		if (!numSlots)
			break;

		slot=(struct FatDirSlot*)currDirEntry;

		/* Several sanity checks */
		if (slot->attr != ATTR_EXT || (FAT_LONG_ENTRIES(slot->id)) != numSlots 
			|| (slot->checkSum != checkSum))
			return -EINVAL;
	}

	/* Check that the checksum of the current (short entry) is the same 
	   as all the checksums before. */
	shortCheckSum=FatCheckSum(currDirEntry->fileName);

	if (UNLIKELY(shortCheckSum != checkSum))
		return -EINVAL;
	else
		return 0;
}

/***********************************************************************
 *
 * FUNCTION: FatScanDir
 *
 * DESCRIPTION: Scan a directory for a name or a free entry.
 *
 * PARAMETERS: vNode - directory vfs node.
 *			   name - name of entry to be found.
 *			   nameLength - length of name.
 *			   retVal - directory entry to be returned.
 *			   vNum - vfs node number to be returned.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int FatScanDir(struct VNode* vNode,const char* name,int nameLength,struct FatDirEntry* retVal,DWORD* vNum,DWORD* dirPos)
{
	DWORD pos;
	int sec,offset,found=0,isLongName=0;
	struct FatDirEntry currDirEntry;
	char longName[255];
	char fatName[13];

	pos = 0;

	while (pos < vNode->size)
	{
		/* No more directory entries */
		if (FatGetDirEntry(vNode, &currDirEntry, FAT_DIR_POS(pos)))
			break;

		FatPosToSecOffset(vNode, pos, &sec, &offset);
		
		pos++;

		if (!currDirEntry.fileName[0])
			break;

		if (!FAT_VALID_DE(&currDirEntry))
			continue;

		if (currDirEntry.attribute == ATTR_EXT)
		{
			/* Get the long filename corresponding to this entry. */
			if (FatGetLongName(vNode,&pos,longName,&currDirEntry))
				continue;

			FatPosToSecOffset(vNode,pos - 1, &sec, &offset);
			isLongName=1;
		}

		if (isLongName)
		{
			/* Compare normal (or long) name */
			if (nameLength == strlen(longName) && !strnicmp(name,longName,strlen(longName)))
				found=1;

			isLongName=0;
		}else{
			FatNameToStr(currDirEntry.fileName, fatName);
		
			/* Otherwise compare the 8.3 name */
			if (strlen(fatName) == nameLength && !strnicmp(name, fatName, strlen(fatName)))
				found=1;
		}
		
		if (found)
		{
			if (retVal)
				memcpy(retVal,&currDirEntry, sizeof(struct FatDirEntry));

			if (vNum)
				*vNum=SS_TO_VNUM(sec, offset);

			if (dirPos)
				*dirPos=pos - 1;

			return 0;
		}
	}
	
	return -ENOENT;
}

/***********************************************************************
 *
 * FUNCTION: FatReadDir
 *
 * DESCRIPTION: Read directory entries into a buffer.
 *
 * PARAMETERS: file - directory in question.
 *			   dirEntries - parameter to VfsFillDir.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int FatReadDir(struct File* file, void* dirEntries)
{
	struct FatDirEntry currDirEntry;
	int sec,offset;
	char* fillName;
	char newName[12]="";
	char longName[256]; /* Used as a buffer. */
	int isLongName=0;

	while (file->position < file->vNode->size)
	{
		/* Get the directory entry, and quit ReadDir if an I/O error occured. */
		if (FatGetDirEntry(file->vNode, &currDirEntry, FAT_DIR_POS(file->position)))
			break;
		
		FatPosToSecOffset(file->vNode, file->position, &sec, &offset);

		file->position++;

		/* Means there are no new entries */
		if (!currDirEntry.fileName[0])
			break;

		if (!FAT_VALID_DE(&currDirEntry))
			continue;

		if ((currDirEntry.attribute & ATTR_LABEL) && currDirEntry.attribute != ATTR_EXT)
			continue;

		if (currDirEntry.attribute == ATTR_EXT)
		{
			if (FatGetLongName(file->vNode, &file->position, longName, &currDirEntry))
				continue;

			FatPosToSecOffset(file->vNode,file->position - 1,&sec,&offset);
			isLongName=1;
		}

		if (isLongName)
		{
			fillName=longName;
			isLongName=0;
		}else{
			FatNameToStr(currDirEntry.fileName, newName);
			fillName=newName;
		}

		if (FillDir(dirEntries, fillName, strlen(fillName), SS_TO_VNUM(sec,offset)))
		{
			/* May have run out of buffer space. Go back to the previous entry
			 * and try again next time.
			 */
			if (file->position)
				file->position--; /* FIXME: Move back more when long name entry? */
				
			break;
		}
	}

	return 0;
}
