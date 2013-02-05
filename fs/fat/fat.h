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

/* Common defines for the FAT filesystem */

#ifndef FAT_H
#define FAT_H

#include <console.h>
#include <fs/vfs.h>
#include <typedefs.h>
#include <devices/sdevice.h>

#define FAT_DELETED 0xE5

#define ATTR_RDONLY		0x01 /* Read-only */
#define ATTR_HIDDEN		0x02 /* Hidden file */
#define ATTR_SYSTEM		0x04 /* System file */
#define ATTR_LABEL		0x08 /* Volume label */
#define ATTR_DIR		0x10 /* Directory */
#define ATTR_ARCHIVE	0x20 /* Archived */
#define ATTR_DEVICE		0x40 /* Device (never found on disk) */

#define ATTR_UNUSED		(ATTR_LABEL | ATTR_ARCHIVE | ATTR_SYSTEM | ATTR_HIDDEN)
#define ATTR_EXT		(ATTR_RDONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_LABEL)

#define FAT_DOT 	".          "
#define FAT_DOTDOT 	"..         "

#define FAT_ISREGULAR(attr) (!((attr) & ATTR_LABEL) && !((attr) & ATTR_DIR))
#define FAT_ISDIR(attr) (((attr) & ATTR_DIR) && !((attr) & ATTR_LABEL))
#define FAT_ISRDONLY(attr) ((attr) & ATTR_RDONLY))
#define FAT_VALID_DE(de) (((de)->fileName[0]) && ((BYTE)(de)->fileName[0] != 0xE5))
#define FatGetPriv(v) ((struct FatVNodeInfo*)((v)->extraInfo))
#define FatGetSbPriv(sb) ((struct FatSbInfo*)((sb)->privData))
#define	FAT_LONG_ENTRIES(id) ((id) & ~0x40)
#define FAT_DIR_POS(pos) ((pos)*sizeof(struct FatDirEntry))
#define FAT_IS_LONG(id) ((id) & 0x40)

#define FAT_ROOT_ID		0

/* VNode 'hashing' functions */
#define SS_TO_VNUM(sec,subSec) (((sec) << 8) | (subSec & 0xFF))
#define VNUM_TO_SECTOR(vNum) ((vNum) >> 8)
#define VNUM_TO_OFFSET(vNum) ((vNum) & 0xFF)

static inline int FatExtensionLen(const char* name,int nameLength)
{
	int len=0;
	
	while (name[--nameLength] != '.' && nameLength) ++len;
	
	if (!nameLength)
		len=0; /* Found no . , so no extension */
		
	return len;
}

static inline int FatBaseNameLen(const char* name,int nameLength)
{
	int len=FatExtensionLen(name,nameLength);
	if (len)
		++len; /* Will have a . */

	return nameLength-len;
}

static inline BYTE FatCheckSum(char* fatName)
{
	BYTE checkSum=0;
	int i;

	for (i=0; i<11; i++)
		checkSum=(((checkSum & 1) << 7)|((checkSum & 0xFE) >> 1))+fatName[i];

	return checkSum;
}

struct FatBootSector
{
	BYTE jmp[3];
	BYTE oemID[8];
	WORD bytesPerSec;
	BYTE sectorsPerClus;
	WORD reservedSectorCnt;
	BYTE numFats;
	WORD numRootDirEnts;
	WORD totalSecSmall;
	BYTE mediaIDByte;
	WORD secsPerFat;
	WORD secsPerTrack;
	WORD heads;
	DWORD hiddenSectors;
	DWORD totalSectorsLarge;

	/* Only used by FAT32 */
	DWORD secsPerFat32;
	WORD extFlags;
	WORD fsVer;
	DWORD rootClus;
	WORD fsInfo;
	WORD bkBootSec;
	char reserved[12];
	BYTE driveNum;
	BYTE reserved1;
	BYTE bootSig;
	DWORD volID;
	char volLabel[11];
	char fileSysType[8];
}PACKED;

struct FatFsInfo
{
	DWORD reserved1;
	DWORD signature;
	DWORD freeClusters;
	DWORD nextCluster;
	DWORD reserved2;
};

struct FatDirEntry
{
	char fileName[11];
	BYTE attribute;
	BYTE reserved;
	BYTE creationTimeMs;
	WORD creationTime;
	WORD creationDate;
	WORD accessDate;
	WORD startClusterHigh;
	WORD lastWriteTime;
	WORD lastWriteDate;
	WORD startClusterLow;
	DWORD fileSize;
}PACKED;

/* For use with long filenames */

struct FatDirSlot
{
	BYTE id;
	BYTE name[10];
	BYTE attr,reserved,checkSum;
	BYTE name2[12];
	WORD start;
	BYTE name3[4];
}PACKED;

struct FatSbInfo
{
	/* FAT information */
	DWORD fatStart, fatLength, fatType, invalidCluster, numFats;
	
	/* Root directory information */
	DWORD rootDirStart, rootDirLength, rootDirEnts;
	
	/* Data sector and cluster layout */
	DWORD firstDataSector,totalDataSectors, clusterLength, secsPerClus;
	
	/* Values that are only updated on FAT32 */
	DWORD freeClusters, fsInfoSector;
};

struct FatVNodeInfo
{
	DWORD startCluster;
};

int FatCreate(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
int FatRemove(struct VNode* dir,char* name,int nameLength);
int FatLookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
int FatRmDir(struct VNode* dir,char* name,int nameLength);
int FatWriteVNode(struct VNode* vNode);
int FatReadVNode(struct VNode* vNode);
int FatNameToStr(char* fat12Name,char* name);
int FatMkDir(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
int FatTruncate(struct VNode* vNode, DWORD size);
int FatScanDir(struct VNode* vNode,const char* name,int nameLength,struct FatDirEntry* retVal,DWORD* vNum,DWORD* dirPos);
int FatCreateShortName(struct VNode* dir,char* fatName,char* name,int nameLength, int* isShortName);
int FatIsLong(const char* name,int nameLength);

/* File Allocation Table (FAT) functions */
DWORD FatToSector(struct VfsSuperBlock* sBlock,DWORD cluster);
DWORD FatGetDirSize(struct VfsSuperBlock* sBlock,DWORD cluster);
int FatAddCluster(struct VNode* vNode, DWORD* lastCluster);
int FatRemoveClusters(struct VNode* vNode,int number);
DWORD FatAccess(struct VfsSuperBlock* sBlock,DWORD clusterNum,int newVal);

/* Name functions */
int FatValidLongName(char* name,int nameLength);

/* Directory functions */
int FatAddEntries(struct VNode* dir,int numEntries);
int FatAddName(struct VNode* dir,char* fatName,char* name,int nameLength,DWORD* vNum,int attr, int isShortName);
int FatReadDir(struct File* file,void* dirEntries);
int FatGetDirEntry(struct VNode* vNode,struct FatDirEntry* dirEntry,int currPos);

/* Block mapping and reading functions */
struct Buffer* FatBlockRead(struct VNode* vNode,int offset);
int FatBlockMap(struct VNode* vNode, DWORD offset, int flags);

static inline void FatPosToSecOffset(struct VNode* vNode,int pos,int* sec,int* offset)
{
	/* Is this the right way to do things? */
	*sec=FatBlockMap(vNode, FAT_DIR_POS(pos)/BYTES_PER_SECTOR(vNode->superBlock), 0);
	*offset=pos % (BYTES_PER_SECTOR(vNode->superBlock)/sizeof(struct FatDirEntry));
}

extern struct VNodeOps fatVNodeOps;
extern struct SuperBlockOps fatSbOps;
extern struct FileOps fatFileOps;

#endif
