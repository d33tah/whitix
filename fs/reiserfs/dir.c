#include "reiserfs.h"

int ReLookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength);
int ReReadDir(struct File* file, void* dirEntries);
int ReBlockMap(struct VNode* vNode, DWORD offset, int flags);

struct VNodeOps reVNodeOps={
	.blockMap = ReBlockMap,
	.lookup = ReLookup,
};

int ReBlockMap(struct VNode* vNode, DWORD offset, int flags)
{
	struct ReCpuKey key;
	struct Buffer* buffer;
	struct ReItemHead* itemHead;
	DWORD keyType;
	PATH_INITIALIZE(path);

	ReMakeCpuKey(&key, vNode, (offset*BYTES_PER_SECTOR(vNode->superBlock))+1, TYPE_ANY, 3);

	ReSearchPositionByKey(vNode->superBlock, &key, &path);

	buffer=PATH_LAST_BUFFER(&path);
	itemHead=PATH_ITEM_HEAD(&path);

	keyType=ReGetKeyType(&itemHead->key, itemHead->version);
	
	if (keyType == 1 /* INDIRECT */)
	{
		DWORD* item=(DWORD*)RE_ITEM_LOCATION(buffer, itemHead);
		KePrint("%#X %#X, %#X\n", item[0], item[1], item[2]);
		return item[path.posInItem];
	}

	if (keyType == 2)
		return -EINVAL;
	
	return 0;
}

int ReLookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	struct ReCpuKey key;
	struct ReDirEntry entry;
	PATH_INITIALIZE(path);
	int ret;
	DWORD dirId;
	
	ReMakeCpuKey(&key, dir, ReGetThirdComponent(dir->superBlock, name, nameLength), TYPE_DIRENTRY, 3);

//	KePrint("ReLookup(%s), %#X\n", name, ReGetThirdComponent(dir->superBlock, name, nameLength));

	while (1)
	{
		ReSearchByEntryKey(dir->superBlock, &key, &path, &entry);

		ret=ReLeSearchInDirItem(&key, &entry, name, nameLength);

		if (!ret)
			break;
		else
			return ret;

		cli();
		hlt();
	}

	dirId = entry.dirHead->dirId;
	*retVal=VNodeGetWithData(dir->superBlock, entry.dirHead->objectId, &dirId);

	return 0;
}

int ReReadDir(struct File* file, void* dirEntries)
{
	PATH_INITIALIZE(path);
	struct ReCpuKey key;
	struct ReDirEntry dirEntry;
	int result;
	int nextPos;

	ReMakeCpuKey(&key, file->vNode, (file->position) ? (file->position) : DOT_OFFSET, TYPE_DIRENTRY, 3);
	
	while (file->position < file->vNode->size)
	{
		result=ReSearchByEntryKey(file->vNode->superBlock, &key, &path, &dirEntry);

		if (!result || dirEntry.itemNum < RE_ENTRY_NUM(dirEntry.itemHead))
		{
			struct ReDirHead* dirHead=RE_DIR_HEAD(dirEntry.buffer, dirEntry.itemHead)+dirEntry.itemNum;
			int entryNum=dirEntry.itemNum;

			for (; entryNum < RE_ENTRY_NUM(dirEntry.itemHead); entryNum++, dirHead++)
			{
				if (!(dirHead->state & RE_DIR_VISIBLE))
					continue;

				/* Add to listing */
				char* name=(char*)(RE_ITEM_LOCATION(dirEntry.buffer, dirEntry.itemHead)+dirHead->location);
				int length=ReEntryLength(dirEntry.buffer, dirEntry.itemHead, entryNum);

				if (!name[length-1])
					length=strlen(name);

				if (FillDir(dirEntries, name, length, dirHead->objectId))
					break;

				nextPos=dirHead->offset+1;
				file->position=nextPos;
			}

/*			ReSetKeyOffset(&key.onDiskKey, nextPos);

			KePrint("Loop?, %d\n", nextPos);
			cli(); hlt();*/
		}

		/* Get rkey */
//		KePrint("Rkey\n");
//		cli();
//		hlt();
	}

	file->position=file->vNode->size;
	return 0;
}
