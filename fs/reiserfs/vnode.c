#include "reiserfs.h"

extern struct VNodeOps reVNodeOps;
extern struct FileOps reFileOps;

static WORD ReToVNodeModeV1(WORD rMode)
{
	if (rMode & RE_MODE_DIR)
		return VFS_ATTR_DIR;

	KePrint("ReToVNodeMode\n");
	cli(); hlt();

	return 0;
}

static WORD ReToVNodeModeV2(WORD rMode)
{
	if (rMode & RE2_MODE_DIR)
		return VFS_ATTR_DIR;

	if (rMode & RE2_MODE_FILE)
		return VFS_ATTR_FILE;

	KePrint("ReToVNodeModeV2\n");
	cli(); hlt();

	return 0;
}

DWORD ReGetThirdComponent(struct VfsSuperBlock* superBlock, char* name, int nameLength)
{
	/* TODO: Handle dot offsets. */
	DWORD hash;
	struct ReSuperPriv* superPriv=RE_PRIV_SB(superBlock);

	hash = superPriv->hashFunction(name, nameLength);
	hash = HASH_GET_VALUE(hash);

	if (!hash)
		hash=128;

	return hash;
}

static int ReInitVNode(struct VNode* vNode, struct RePath* path)
{
	struct Buffer* buff=PATH_LAST_BUFFER(path);
	struct ReItemHead* head=PATH_ITEM_HEAD(path);
	struct ReVNodeInfo* info;

	info=(struct ReVNodeInfo*)MemAlloc(sizeof(struct ReVNodeInfo));
	vNode->extraInfo=info;

	memcpy(&info->key, &head->key, sizeof(struct ReKey));
	info->version=head->version;

	if (head->version == KEY_FORMAT_3_5)
	{
		struct ReStatV1* stat=(struct ReStatV1*)RE_ITEM_LOCATION(buff, head);

		KePrint("mode = %#X, offset = %#X, %#X\n", stat->mode, (DWORD)stat-(DWORD)buff->data, head->itemLocation);
		KePrint("size = %u\n", stat->size);
		KePrint("%u\n", stat->numLinks);

		vNode->mode=ReToVNodeModeV1(stat->mode) | VFS_ATTR_READ;
		vNode->size=stat->size;
	}else{
		struct ReStatV2* stat=(struct ReStatV2*)RE_ITEM_LOCATION(buff, head);
		
		KePrint("%#X\n", stat->mode);

		vNode->size=(DWORD)stat->size;
		vNode->mode=ReToVNodeModeV2(stat->mode) | VFS_ATTR_READ; /* FIXME: for now. */
	}

	vNode->vNodeOps=&reVNodeOps;
	vNode->fileOps=&reFileOps;
	
	return 0;
}

int ReFsReadVNode(struct VNode* vNode, void* data)
{
	DWORD* dirId=(DWORD*)data;
	struct ReCpuKey key;
	PATH_INITIALIZE(path);

	if (!dirId)
		return -EFAULT;

	key.version=KEY_FORMAT_3_5;
	key.onDiskKey.dirId=*dirId;
	key.onDiskKey.objectId=vNode->id;
	key.onDiskKey.u.offsetVer1.offset=SD_OFFSET;
	key.onDiskKey.u.offsetVer1.uniqueness=SD_UNIQUENESS;

	if (ReSearchByKey(vNode->superBlock, &key, &path, DISK_LEAF_NODE_LEVEL))
	{
		KePrint("Could not find key\n");
		return -ENOENT;
	}

	ReInitVNode(vNode, &path);

	return 0;
}
