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

#ifndef EXT3_H
#define EXT3_H

#include <fs/journal.h>
#include <fs/vfs.h>
#include <module.h>

/* EXT3 features */
#define EXT3_FEATURE_COMPAT_JOURNAL		0x4

#define EXT3_FEATURE_COMPAT(sb, feature) ((sb)->featureCompat & (feature))

/* Journal block defines. */
#define EXT3_SINGLE_DATA_TRANS		8
#define EXT3_DATA_TRANS				(3*EXT3_SINGLE_DATA_TRANS-2)
#define EXT3_RESERVE_TRANS_BLOCKS 	12
#define EXT3_DELETE_TRANS_BLOCKS	(2*EXT3_DATA_TRANS+64)

struct Ext3SuperBlock
{
	DWORD iNodesCount; 		/* Inodes Count */
	DWORD blocksCount; 		/* Blocks Count */
	DWORD reservedBlocks; 	/* Reserved Blocks Count */
	DWORD freeBlocks; 		/* Free Blocks Count */
	DWORD freeInodes; 		/* Free inode Count */
	DWORD firstDataBlock; 	/* First Data Block */
	DWORD logBlockSize; 	/* Block Size as a power of 2. The unit is 1024. Thus 
	0 == 1024 , 1 = 2048 , 2 = 4096*/
	DWORD logFragSize; 		/* Fragments are not yet implemented int ext3. So 
	logFragSize .equal. logBlockSize */
	DWORD blocksPerGrp; 	/* Number of Blocks in each block group */
	DWORD fragsPerGrp; /* Number of Fragments in each block group */
	DWORD iNodesPerGrp; /* Number of Inodes in each block group */
	DWORD mountTime; /* Mount Time */
	DWORD writeTime; /* Write Time */
	WORD mntCount; /* Mount count */
	WORD maxMntCount; /* Maximal mount count */
	WORD magic; /* Magic signature */
	WORD state; /* File system state. 0 = Currently Mounted or not clenaly mounted, 
	1 = clenaly unmounted , 2 = It has errors  */
	WORD errors; /* Behaviour when detecting errors */
	WORD minorRevLevel; /* minor revision level */
	DWORD lastCheck; /* time of last check */
	DWORD checkInterval; /* max. time between checks */
	DWORD creatorOs; /*  OS where FS was created */
	DWORD revLevel; /* Revision of the filesystem */
	WORD defResUid,defResGid; /* Default uid/gid for reserved blocks */
	DWORD firstINode; /* First non reserved Inode */
	WORD iNodeSize; /* size of inode structure */
	WORD blockGroupNr; /* block group # of this superblock */
	DWORD featureCompat,featureInCompat,featureRoCompat; /* feature set */
	BYTE uuid[16]; /* 128 bit uuid for the volume */
	char volumeName[16]; /* volume name */
	char lastMounted[64]; /* directory where last mounted */
	DWORD algorithmUsageBitmap; /* algorithm used for compression */
 
 	/*
	* Performance hints.  Directory preallocation should only
	* happen if the EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	*/
	BYTE preallocBlocks; /* # of blks try to preallocate */
	BYTE preallocDirBlocks; /* # of blks try to preallocate for dir */
	WORD reservedGDescBlocks; /* Per group table for online growth */
	/*
	 * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	BYTE journalUuid[16];/* uuid of journal superblock  */
	DWORD journalINum; /* inode number of journal file */
	DWORD journalDev; /* device number of journal file */
	DWORD lastOrphan;/* start of list of inodes to delete  */
	DWORD hashSeed[4]; /* HTREE hash seed */
	BYTE defHasVersion; /* Default hash version to use */
	BYTE reservedCharPad;  
	WORD reservedWordPad;
	DWORD defaultMountOpts;
	DWORD firstMetaBg; /* First metablock group */
	DWORD reserved[190]; /* Padding to the end of the block */
}PACKED;

struct Ext3SbInfo
{
	DWORD iNodesCount,groupCount,iNodesPerGrp;
	DWORD firstDataBlock, blocksPerGrp;
	struct Buffer** descs;
	
	DWORD sbSector;
	struct Buffer* sbBuffer;
	struct Ext3SuperBlock* super;
	
	struct Journal* journal;
};

struct Ext3INodeInfo
{
	DWORD blocks[15];
	DWORD blockGroup;
};

/* On disk structures */

/*  
 * Each block group has its own group descriptor
 */
struct Ext3GroupDesc
{
	DWORD blockBitmap; // Block number of blkbitmap
	DWORD iNodeBitmap; // Blk number of inode bitmap
	DWORD iNodeTable; // Blk Number of first inode table block
	WORD freeBlocks; // Number of free blocks in the group
	WORD freeINodes; // Number of free inodes in the group
	WORD usedDirs; // Number of directories in the group
	WORD pad; //  Alignment to WORD
	DWORD reserved[3]; // Null to pad out 24 bytes.
}PACKED;

struct Ext3INode
{
	WORD mode; // file type and access rights.
	WORD uid; // Owner identifier
	DWORD size;// file length in bytes 
	DWORD aTime;// time of last file access 
	DWORD cTime;// Time that inode last changed 
	DWORD mTime;// Time that file contents last changed 
	DWORD dTime;// Time of file deletion
	WORD gid;// User group identifier 
	WORD linksCount; // Hard links count
	DWORD blocksCount;// Number of data blocks in a file 
	DWORD flags;// File flags 
	DWORD reserved; // Reserved 
	DWORD blocks[15];// Pointer to data blocks 
	DWORD version;// File version when used by network filesystem
	DWORD fileAcl;// file access control list 
	DWORD dirAcl;// directory access control list 
	DWORD fAddr; // Fragment address
	BYTE frag;// fragment number 
	BYTE fsize; // fragment size
	WORD pad;
	DWORD reserved2[2];
}PACKED;

#define EXT3_DIR_REC_LEN(nameLen)	(((nameLen) + 8 + 3) & ~3)

#define EXT3_FT_UNKNOWN		0
#define EXT3_FT_FILE		1
#define EXT3_FT_DIR			2

static inline DWORD Ext3SetFileType(DWORD mode)
{
	if (mode & VFS_ATTR_FILE)
		return EXT3_FT_FILE;
	else if (mode & VFS_ATTR_DIR)
		return EXT3_FT_DIR;
	else
		return EXT3_FT_UNKNOWN;
}

/*
 * DirEntry describes Directory, which are special kind of file
 * which stores file names along with its inodes.
 */
struct Ext3DirEntry
{
	DWORD iNodeNum; // Inode number
	WORD recLen; // Directory entry length
	BYTE nameLen;// filename length
	BYTE fileType; // file type
	char name[255]; /* file name. In practice, it's a lot less */
}PACKED;

/* Common functions */
int Ext3ReadVNode(struct VNode* vNode);
int Ext3WriteVNode(struct VNode* vNode);
int Ext3DirtyVNode(struct VNode* vNode);
int Ext3Lookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
int Ext3ReadDir(struct File* file,void* dirEntries);
int Ext3BlockMap(struct VNode* vNode, DWORD block, int flags);
struct Buffer* Ext3BlockRead(struct VNode* vNode,int block, int create);
int Ext3Create(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
int Ext3Truncate(struct VNode* vNode, DWORD size);
int Ext3MkDir(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
struct VNode* Ext3CreateINode(struct JournalHandle* handle, struct VNode* dir, int isDir);
int Ext3Remove(struct VNode* dir, char* name, int nameLength);
int Ext3RemoveDir(struct VNode* dir, char* name, int nameLength);

int Ext3FileWrite(struct File* file,BYTE* buffer,DWORD len);

/* balloc.c */
struct Ext3GroupDesc* Ext3GetGroupDesc(struct VfsSuperBlock* superBlock, DWORD groupNo,
	struct Buffer** buffer);
int Ext3BlockMapHandle(struct JournalHandle* handle, struct VNode* vNode, DWORD block, int flags);
int Ext3BlockTruncate(struct VNode* vNode, int size);

extern struct VNodeOps ext3VNodeOps;
extern struct FileOps ext3FileOps;

/* 
 * Defines for superblock stuff. At least the ReadVNode code doesn't look like lisp 
 * code anymore 
 */
#define DESCS_PER_BLOCK(s) (BYTES_PER_SECTOR(s)/sizeof(struct Ext3GroupDesc))
#define INODES_PER_BLOCK(s) (BYTES_PER_SECTOR(s)/sizeof(struct Ext3INode))
#define EXT3_SUPERINFO(s) ((struct Ext3SbInfo*)((s)->privData))
#define EXT3_JOURNAL(vNode) (EXT3_SUPERINFO((vNode)->superBlock)->journal)

#define EXT3_INFO(vNode) ((struct Ext3INodeInfo*)(vNode->extraInfo))

#define EXT3_SB_MAGIC	0xEF53
#define EXT3_ROOT_VNO	2

#endif
