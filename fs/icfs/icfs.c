#include <module.h>
#include <fs/vfs.h>
#include <module.h>
#include <error.h>
#include <print.h>
#include <fs/kfs.h>
#include <fs/icfs.h>

#include "icfs.h"

struct VfsSuperBlock* icFsSuperBlock;

int IcFsWrite(struct File* file, BYTE* data, DWORD size)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(file->vNode->id);
	struct IcFsEntry* iEnt=(struct IcFsEntry*)(entry->file);
	
	return IcWrite(&file->position, iEnt, data, size);
}

int IcFsRead(struct File* file, BYTE* data, DWORD size)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(file->vNode->id);
	struct IcFsEntry* iEnt=(struct IcFsEntry*)(entry->file);
	
	return IcRead(&file->position, iEnt, data, size);
}

int IcFsReadDir(struct File* file, void* dirEntries)
{
	return KeFsReadDir(file, FillDir, dirEntries);
}

struct FileOps icFsFileOps=
{
	.read = IcFsRead,
	.readDir=IcFsReadDir,
	.write = IcFsWrite,
};

int IcFsLookup(struct VNode** retVal, struct VNode* dir, char* name, int nameLength)
{
	struct KeFsEntry* entry;
	
	entry=KeFsLookup(dir, name, nameLength);
	
	if (!entry)
		return -ENOENT;
		
	*retVal=VNodeGet(dir->superBlock, (DWORD)entry);
	
	return 0;
}

int IcFsFollowLink(struct VNode** ret, struct VNode* vNode)
{
	struct KeFsEntry* src, *dest;
	int err;
	
	src=(struct KeFsEntry*)(vNode->id);
	
	/* FIXME: Optimize */
	err = src->followLink(&dest, src);

	if (err)
		return err;

	*ret = VNodeGet(icFsSuperBlock, (DWORD)dest);
	
	if (!*ret)
		return -ENOENT;
	
	return 0;
}

struct VNodeOps icFsVOps=
{
	.lookup=IcFsLookup,
	.followLink = IcFsFollowLink,
};

int IcFsReadVNode(struct VNode* vNode)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(vNode->id);
	struct IcFsEntry* iEnt;
	
	vNode->vNodeOps=&icFsVOps;
	vNode->fileOps=&icFsFileOps;

	vNode->mode = entry->type;
		
	if (vNode->mode & VFS_ATTR_DIR)
		vNode->size = KeFsDirSize(&entry->dir);
	else if (vNode->mode & VFS_ATTR_FILE)
	{
		/* Fill in size. */
		iEnt=entry->file;
		
		switch (iEnt->type)
		{
			case ICFS_TYPE_INT:
				vNode->size=sizeof(int);
				break;
				
			case ICFS_TYPE_STR:
				vNode->size=strlen(iEnt->sVal);
				break;
				
			case ICFS_TYPE_BYTE:
				vNode->size=iEnt->size;
				break;

			case ICFS_TYPE_FUNC:
				vNode->size = 0; /* Can't give a size. */
				break;
	
			default:
				KePrint("IcFsReadVNode: %d\n", iEnt->type);
		}
	}

	return 0;
}

struct SuperBlockOps icFsSbOps=
{
	.readVNode = IcFsReadVNode,
};

struct VfsSuperBlock* IcFsReadSuper(struct StorageDevice* dev, int flags, char* data)
{
	struct VfsSuperBlock* ret;
	
	if (dev)
		return NULL;
		
	ret=VfsAllocSuper(dev, flags);
	
	if (!ret)
		goto end;
		
	ret->privData=NULL;
	
	ret->sbOps=&icFsSbOps;	
	ret->mount=VNodeGet(ret, (DWORD)IcFsGetRootId());

	icFsSuperBlock = ret;

end:
	return ret;
}

VFS_FILESYSTEM(icFileSystem, "icfs", IcFsReadSuper);

int IcFsInit()
{
	return VfsRegisterFileSystem(&icFileSystem);
}

ModuleInit(IcFsInit);
