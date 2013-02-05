#ifndef REISER_FS_H
#define REISER_FS_H

#include <error.h>
#include <fs/vfs.h>
#include <typedefs.h>
#include <types.h>
#include <print.h>
#include <module.h>
#include <string.h>
#include <malloc.h>

/* Forward defines. */
struct ReCpuKey;
struct RePath;
struct ReKey;
struct ReDirHead;
struct ReItemHead;

/* Superblock */

struct ReFsSuperBlockV1
{
	DWORD blockCount;
	DWORD freeBlocks;
	DWORD rootBlockNo;

	/* Journal information */
	DWORD journFirstBlock;
	DWORD journDev;
	DWORD journSize;
	DWORD journMaxTrans;
	DWORD journMagic;
	DWORD journMaxBatch;
	DWORD journMaxCommitAge;
	DWORD journMaxTransAge;

	WORD blockSize;
	WORD oidMaxSize;
	WORD oidCurSize;
	WORD uMountState;
	char magic[10];
	WORD fsState;
	DWORD hashFuncCode;
	WORD treeHeight;
	WORD bMapNo;
	WORD version;
	WORD reservedForJournal;
}PACKED;

struct ReFsSuperBlock
{
	struct ReFsSuperBlockV1 sbVer1;

	DWORD vNodeGen;
	DWORD flags;
	BYTE uuid[16];
	BYTE label[16];
	char unUsed[88];
}PACKED;

struct ReSuperPriv
{
	struct ReFsSuperBlockV1 superBlock;
	DWORD (*hashFunction)(char* name, int nameLength);
};

#define HASH_GET_VALUE(hash) ((hash) & 0x7fffff80)

#define RE_PRIV_SB(superBlock) ((struct ReSuperPriv*)((superBlock)->privData))
#define RE_V1_DISK_SB(super) (RE_PRIV_SB(super)->superBlock)

#define RE_ROOT_BLOCK(super) (RE_V1_DISK_SB(super).rootBlockNo)
#define RE_TREE_HEIGHT(super) (RE_V1_DISK_SB(super)->treeHeight)

int ReFsReadVNode(struct VNode* vNode, void* data);

#define RFS_ROOT_OBJECTID	2
#define RFS_ROOT_PARENT_OBJECTID 1

/* On-disk inode structures. */

#define SD_OFFSET		0
#define SD_UNIQUENESS	0

#define V1_SD_UNIQUENESS	0
#define V1_INDIRECT_UNIQUENESS	0xFFFFFFFE
#define V1_DIRECT_UNIQUENESS	0xFFFFFFFF
#define V1_DIRENTRY_UNIQUENESS	500
#define V1_ANY_UNIQUENESS		555

struct ReOffsetVer1
{
	DWORD offset;
	DWORD uniqueness;
}PACKED;

#define TYPE_STAT_DATA		0
#define TYPE_INDIRECT		1
#define TYPE_DIRECT			2
#define TYPE_DIRENTRY		3
#define TYPE_MAX_TYPE		3
#define TYPE_ANY			15

struct ReOffsetVer2
{
	QWORD offset:60;
	QWORD type:4;
}PACKED;

#define KEY_FORMAT_3_5		0
#define KEY_FORMAT_3_6		1

#define STAT_DATA_V1		0
#define STAT_DATA_V2		1

#define RE_SHORT_KEY_LEN	2

struct ReKey
{
	DWORD dirId;
	DWORD objectId;

	union
	{
		struct ReOffsetVer1 offsetVer1;
		struct ReOffsetVer2 offsetVer2;
	}PACKED u;
}PACKED;

struct ReCpuKey
{
	struct ReKey onDiskKey;
	int version;
	int keyLength;
};

struct ReDirEntry
{
	struct Buffer* buffer;
	struct ReItemHead* itemHead;
	struct ReDirHead* dirHead;
	int itemNum;
};

/* S+ tree functions. stree.c */
int ReSearchByKey(struct VfsSuperBlock* superBlock, struct ReCpuKey* key, struct RePath* searchPath, int stopLevel);
void ReMakeCpuKey(struct ReCpuKey* key, struct VNode* vNode, DWORD offset, int type, int length);
int ReSearchByEntryKey(struct VfsSuperBlock* superBlock, struct ReCpuKey* key, struct RePath* path, struct ReDirEntry* entry);
int ReSearchPositionByKey(struct VfsSuperBlock* superBlock, struct ReCpuKey* key, struct RePath* path);
void ReSetKeyOffset(struct ReCpuKey* key, DWORD offset);
DWORD ReGetThirdComponent(struct VfsSuperBlock* superBlock, char* name, int nameLength);
int ReReadDir(struct File* file, void* dirEntries);
int ReLeSearchInDirItem(struct ReCpuKey* key, struct ReDirEntry* entry, const char* name, int nameLength);

/* Paths */

/* Defines */
#define MAX_HEIGHT_EXTENDED				7
#define PATH_ELEMENT_ILLEGAL_OFFSET		1
#define PATH_ELEMENT_FIRST_OFFSET		2

struct RePathElement
{
	struct Buffer* buffer;
	int position;
};

struct RePath
{
	int pathLength;
	struct RePathElement elements[MAX_HEIGHT_EXTENDED];
	int posInItem;
};

#define PATH_INITIALIZE(path)	struct RePath path={PATH_ELEMENT_ILLEGAL_OFFSET, { {NULL, 0}, }, 0}
#define PATH_OFFSET_ELEMENT(path, offset) ((path)->elements+(offset))
#define PATH_OFFSET_BUFFER(path, offset) (PATH_OFFSET_ELEMENT(path, offset)->buffer)
#define PATH_OFFSET_POSITION(path, offset) (PATH_OFFSET_ELEMENT(path, offset)->position)

#define PATH_LAST_POSITION(path) (PATH_OFFSET_POSITION(path, path->pathLength))
#define PATH_LAST_BUFFER(path) (PATH_OFFSET_BUFFER((path), (path)->pathLength))
#define PATH_ITEM_HEAD(path) (RE_ITEM_OFFSET(PATH_LAST_BUFFER((path)), PATH_LAST_POSITION((path))))

/* Leafs */

#define DISK_LEAF_NODE_LEVEL		1

#define RE_ITEM_OFFSET(buff, offset) ((struct ReItemHead*)(buff->data+sizeof(struct ReBlockHead))+offset)
#define RE_ITEM_LOCATION(buffer, item) ((buffer)->data+item->itemLocation)
#define RE_DIR_HEAD(buffer, itemHead) ((struct ReDirHead*)(RE_ITEM_LOCATION(buffer, itemHead)))

#define RE_BLOCK_CHILD(buff, pos) ( (struct ReBlockChild*)((buff)->data+sizeof(struct ReBlockHead)+ \
	sizeof(struct ReKey)*BLOCK_NUM_ITEMS(buff)+sizeof(struct ReBlockChild)*pos) )

#define RE_BLOCK_CHILD_NUM(buff, pos) (RE_BLOCK_CHILD(buff, pos)->blockNum)

#define RE_ENTRY_NUM(itemHead) ((itemHead)->u.entryCount)

struct ReItemHead
{
	struct ReKey key;
	
	union
	{
		WORD freeSpaceReserved;
		WORD entryCount;
	}PACKED u;

	WORD itemLen;
	WORD itemLocation;
	WORD version;
}PACKED;

#define BLOCK_NUM_ITEMS(buff) (((struct ReBlockHead*)buff->data)->noItems)

struct ReBlockHead
{
	WORD level;
	WORD noItems;
	WORD freeSpace;
	WORD reserved;

	struct ReKey rightDelimKey;
}PACKED;

struct ReBlockChild
{
	DWORD blockNum;
	WORD size;
	WORD reserved;
}PACKED;

#define RE_DIR_VISIBLE		0x04

struct ReDirHead
{
	DWORD offset;
	DWORD dirId;
	DWORD objectId;
	WORD location;
	WORD state;	
};

static inline int ReEntryLength(struct Buffer* buffer, struct ReItemHead* itemHead, int entryNum)
{
	struct ReDirHead* dirHead = RE_DIR_HEAD(buffer, itemHead) + entryNum;

	if (entryNum)
		return (dirHead-1)->location - dirHead->location;

	return itemHead->itemLen - dirHead->location;
}

#define DOT_OFFSET		1

#define RE_MODE_DIR		1

#define RE2_MODE_DIR	0x4000
#define RE2_MODE_FILE	0x8000

struct ReStatV1
{
	WORD mode;
	WORD numLinks;
	WORD uid;
	WORD gid;
	DWORD size;
	DWORD aTime;
	DWORD mTime;
	DWORD cTime;

	union
	{
		DWORD rdev;
		DWORD blocks;
	}PACKED u;

	DWORD firstDirectByte;
}PACKED;

struct ReStatV2
{
	WORD mode;
	WORD attrs;
	DWORD numLinks;
	QWORD size;
	DWORD uid;
	DWORD gid;
	DWORD aTime;
	DWORD mTime;
	DWORD cTime;
	DWORD blocks;
	union
	{
		DWORD rdev;
		DWORD generation;
	}PACKED u;
}PACKED;

struct ReVNodeInfo
{
	struct ReKey key;
	int version;
};

#define RE_TEA_HASH		0x1
#define RE_YURA_HASH	0x2
#define RE_R5_HASH		0x3

typedef DWORD (*ReHash)(char* name, int nameLength);

static inline int ReTypeToUniqueness(int type)
{
	switch (type)
	{
		case TYPE_STAT_DATA:
			return V1_SD_UNIQUENESS;

		case TYPE_INDIRECT:
			return V1_INDIRECT_UNIQUENESS;

		case TYPE_DIRECT:
			return V1_DIRECT_UNIQUENESS;

		case TYPE_DIRENTRY:
			return V1_DIRENTRY_UNIQUENESS;

		default:
			KePrint("ReTypeToUniqueness: unknown type %d\n", type);

		case TYPE_ANY:
			return V1_ANY_UNIQUENESS;
	}

	return 0;
}

static inline int ReUniquenessToType(int uniqueness)
{
	KePrint("ReUniquenessToType\n");
	cli(); hlt();
	return 0;
}

/* Inline functions for ReKey retrieval */

static inline DWORD ReGetKeyType(struct ReKey* key, int version)
{
	if (version == KEY_FORMAT_3_5)
		return ReUniquenessToType(key->u.offsetVer1.uniqueness);
	else
		return key->u.offsetVer2.type;	
}

static inline int ReKeyGetVersion(struct ReKey* key)
{
	int type;

	type=ReGetKeyType(key, KEY_FORMAT_3_6);

	if (type != TYPE_DIRECT && type != TYPE_INDIRECT && type != TYPE_DIRENTRY)
		return KEY_FORMAT_3_5;

	return KEY_FORMAT_3_6;
}

static inline DWORD ReKeyGetOffset(struct ReKey* key, int version)
{
	if (version == KEY_FORMAT_3_5)
		return key->u.offsetVer1.offset;
	else
		return key->u.offsetVer2.offset;
}

#endif
