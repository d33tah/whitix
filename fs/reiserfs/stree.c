#include "reiserfs.h"

/* TODO: Does offset need to be 64-bits? */

static DWORD ReGetKeyOffset(struct ReCpuKey* key)
{
	if (key->version == KEY_FORMAT_3_5)
		return key->onDiskKey.u.offsetVer1.offset;
	else
		return key->onDiskKey.u.offsetVer2.offset;	
}

void ReSetKeyOffset(struct ReCpuKey* key, DWORD offset)
{
	if (key->version == KEY_FORMAT_3_5)
		key->onDiskKey.u.offsetVer1.offset=offset;
	else
		key->onDiskKey.u.offsetVer2.offset=offset;
}

static void ReSetKeyType(struct ReCpuKey* key, int type)
{
	if (key->version == KEY_FORMAT_3_5)
		key->onDiskKey.u.offsetVer1.uniqueness=ReTypeToUniqueness(type);
	else
		key->onDiskKey.u.offsetVer2.type=type;
}

void ReMakeCpuKey(struct ReCpuKey* key, struct VNode* vNode, DWORD offset, int type, int length)
{
	struct ReVNodeInfo* info=(struct ReVNodeInfo*)(vNode->extraInfo);

	key->version=info->version;
	key->onDiskKey.objectId=info->key.objectId;
	key->onDiskKey.dirId=info->key.dirId;

	ReSetKeyOffset(key, offset);
	ReSetKeyType(key, type);

	key->keyLength=length;
}

static inline int ReCompareKeysShort(struct ReKey* leftKey, struct ReCpuKey* rightKey)
{
	DWORD* leftBits=(DWORD*)leftKey;
	DWORD* rightBits=(DWORD*)rightKey;
	int keyLength=RE_SHORT_KEY_LEN;

	for (; keyLength--;  ++leftBits, ++rightBits)
	{
		if (*leftBits < *rightBits)
			return -1;

		if (*leftBits > *rightBits)
			return 1;
	}

	return 0;
}

static inline int ReCompareKeys(struct ReKey* leftKey, struct ReCpuKey* rightKey)
{
	int result;

	result=ReCompareKeysShort(leftKey, rightKey);

	if (result)
		return result;

	if (ReKeyGetOffset(leftKey, ReKeyGetVersion(leftKey)) < ReGetKeyOffset(rightKey))
		return -1;

	if (ReKeyGetOffset(leftKey, ReKeyGetVersion(leftKey)) > ReGetKeyOffset(rightKey))
		return 1;

	if (rightKey->keyLength == 3)
		return 0;

	/* TODO */
	return 0;
}

struct ReKey reMinKey={0, 0, {{0, 0}, }};
struct ReKey reMaxKey={~0, ~0, {{~0, ~0}, }};

struct ReKey* ReGetLeftKey(struct VfsSuperBlock* superBlock, struct RePath* path)
{
	int pathLength = path->pathLength;

	while (pathLength -- > PATH_ELEMENT_FIRST_OFFSET)
	{
		/* TODO */
	}

	if (PATH_OFFSET_BUFFER(path, PATH_ELEMENT_FIRST_OFFSET)->blockNum == RE_ROOT_BLOCK(superBlock))
		return &reMinKey;
	else
		return &reMaxKey;
}

struct ReKey* ReGetRightKey(struct VfsSuperBlock* superBlock, struct RePath* path)
{
	/* TODO */
	return &reMaxKey;
}

int ReKeyInBuffer(struct VfsSuperBlock* superBlock, struct RePath* checkPath, struct ReCpuKey* key)
{
	if (ReCompareKeys(ReGetLeftKey(superBlock, checkPath), key) == 1)
		return 0;

	if (ReCompareKeys(ReGetRightKey(superBlock, checkPath), key) != 1)
		return 0;

	return 1;
}

int ReBinSearch(struct ReCpuKey* key, void* base, int totalNum, int width, int* pos)
{
	int lBound=0;
	int rBound=totalNum-1;
	int middle;

	for (middle=(lBound+rBound)/2; lBound <= rBound; middle=(lBound+rBound)/2)
	{
		int result=ReCompareKeys((struct ReKey*)((char*)base+middle*width), key);

		switch (result)
		{
			case -1:	lBound=middle+1; continue;
			case  1:	rBound=middle-1; continue;
			case  0:	*pos=middle;	 return 0;
		}
	}

	*pos=lBound;
	return -ENOENT;
}

int ReBinSearchDirItem(struct ReDirEntry* entry, DWORD offset)
{
	struct ReItemHead* itemHead=entry->itemHead;
	struct ReDirHead* dirHead=RE_DIR_HEAD(entry->buffer, itemHead);

	int lBound=0;
	int rBound=itemHead->u.entryCount;
	int middle;

	for (middle=(lBound+rBound)/2; lBound <= rBound; middle=(lBound+rBound)/2)
	{
		if (offset < dirHead[middle].offset)
		{
			rBound=middle-1;
			continue;
		}

		if (offset > dirHead[middle].offset)
		{
			lBound=middle+1;
			continue;
		}

		entry->itemNum=middle;
		return 0;
	}

	entry->itemNum=lBound;
	return -ENOENT;
}

static void ReSetDirEntryInfo(struct ReDirEntry* entry, struct RePath* path)
{
	entry->buffer=PATH_LAST_BUFFER(path);
	entry->itemHead=PATH_ITEM_HEAD(path);
	entry->itemNum=PATH_LAST_POSITION(path);
}

int ReLeSearchInDirItem(struct ReCpuKey* key, struct ReDirEntry* entry, const char* name, int nameLength)
{
	int i=entry->itemNum;
	struct ReDirHead* dirHead=RE_DIR_HEAD(entry->buffer, entry->itemHead)+i;

	for (; i>=0; i--, dirHead--)
	{
		if (HASH_GET_VALUE(dirHead->offset) != HASH_GET_VALUE(ReGetKeyOffset(key)))
			return -ENOENT;

		char* deName=(char*)(RE_ITEM_LOCATION(entry->buffer, entry->itemHead)+dirHead->location);
		int deLength=ReEntryLength(entry->buffer, entry->itemHead, i);

		if (!deName[deLength-1])
			deLength=strlen(deName);

		if (!memcmp(name, deName, deLength))
		{
			entry->dirHead=dirHead;
			return 0;
		}
	}

	return -ENOENT;
}

int ReSearchPositionByKey(struct VfsSuperBlock* superBlock, struct ReCpuKey* key, struct RePath* path)
{
	/* TODO: Add more cases. */
	return ReSearchByKey(superBlock, key, path, DISK_LEAF_NODE_LEVEL);
}

int ReSearchByEntryKey(struct VfsSuperBlock* superBlock, struct ReCpuKey* key, struct RePath* path, struct ReDirEntry* entry)
{
	int result;

	result=ReSearchByKey(superBlock, key, path, DISK_LEAF_NODE_LEVEL);

	switch (result)
	{
		case -ENOENT:
			if (!PATH_LAST_POSITION(path))
				KePrint("position == 0?!\n");

			PATH_LAST_POSITION(path)--;
		case 0:
			break;

		default:
			return result;
	}

	ReSetDirEntryInfo(entry, path);

	result=ReBinSearchDirItem(entry, ReGetKeyOffset(key));
	path->posInItem=entry->itemNum;

	return result;
}

int ReSearchByKey(struct VfsSuperBlock* superBlock, struct ReCpuKey* key, struct RePath* searchPath, int stopLevel)
{
	struct RePathElement* lastElement;
	DWORD blockNo=RE_ROOT_BLOCK(superBlock);
//	int expectedLevel=RE_TREE_HEIGHT(superBlock);
	struct Buffer* currBuff;

	/* TODO: Decrement counts of all buffers in searchPath */

	while (1)
	{
		struct ReBlockHead* bHead;
		int result;

		/* Path will now have another element added to it */
		lastElement=PATH_OFFSET_ELEMENT(searchPath, ++searchPath->pathLength);
		currBuff=lastElement->buffer=BlockRead(superBlock->sDevice, blockNo);

		if (!currBuff)
		{
			/* Tidy up */
			return -EIO;
		}

		/* TODO: Compare to max key? */

		/* Assert here. */
//		ReKeyInBuffer(superBlock, searchPath, key);

		/* Another check. */
//		ReIsTreeNode(

		bHead=(struct ReBlockHead*)currBuff->data;

		result=ReBinSearch(key, RE_ITEM_OFFSET(currBuff, 0), bHead->noItems,
			bHead->level == DISK_LEAF_NODE_LEVEL ? sizeof(struct ReItemHead) : sizeof(struct ReKey), &lastElement->position);

		if (bHead->level == stopLevel)
				return result;

		if (!result)
			lastElement->position++;

		KePrint("%d %u %u\n", result, bHead->level, stopLevel);
        KePrint("%u, %#X, %#X, %u\n", BLOCK_NUM_ITEMS(currBuff), RE_BLOCK_CHILD(currBuff, lastElement->position), (DWORD)currBuff->data, bHead->level);

		blockNo=RE_BLOCK_CHILD_NUM(currBuff, lastElement->position);

		if (!blockNo)
		{
			KePrint("lastElement->position = %u, result = %d\n", lastElement->position, result);
			cli();
			hlt();
		}

		KePrint("blockNo = %u\n", blockNo);
	}

	return 0;
}
