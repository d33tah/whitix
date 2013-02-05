#include <module.h>
#include <error.h>
#include <init.h>
#include <print.h>
#include <fs/kfs.h>
#include <fs/icfs.h>
#include <fs/vfs.h>

DWORD ConfigGetRootId();
int IcFsRead(struct File* file, BYTE* data, DWORD size);
int IcFsReadDir(struct File* file, void* dirEntries);
int IcFsFollowLink(struct VNode** ret, struct VNode* vNode);

int ConfigLookup(struct VNode** retVal,struct VNode* dir,char* name,int nameLength)
{
	struct KeFsEntry* entry;
	
	entry=KeFsLookup(dir, name, nameLength);
	
	if (!entry)
		return -ENOENT;
		
	*retVal=VNodeGet(dir->superBlock, (DWORD)entry);
	
	return 0;
}

int ConfigWriteInt(struct File* file, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	int copied;
	
	copied=MIN(size-file->position, sizeof(int)-file->position);
	
	memcpy(iEnt->iVal+file->position, data, copied);
	
	file->position+=copied;
	
	return copied;
}

int ConfigWrite(struct File* file, BYTE* data, DWORD size)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(file->vNode->id);
	struct IcFsEntry* iEnt=(struct IcFsEntry*)(entry->file);
	
	switch (iEnt->type)
	{
		case ICFS_TYPE_INT:
			return ConfigWriteInt(file, iEnt, data, size);
			
		default:
			printf("ConfigWrite: TODO\n");
	}
	
	return 0;
}

struct VNodeOps configFsVOps=
{
	.lookup=ConfigLookup,
	.followLink=IcFsFollowLink,
};

struct FileOps configFsFileOps=
{
	.read = IcFsRead,
	.readDir=IcFsReadDir,
	.write = ConfigWrite
};

int ConfigReadVNode(struct VNode* vNode)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(vNode->id);

	IcFsReadVNode(vNode);
	
	vNode->vNodeOps=&configFsVOps;
	vNode->fileOps=&configFsFileOps;

	return 0;
}

struct SuperBlockOps configFsSbOps=
{
	.readVNode = ConfigReadVNode,
};

struct VfsSuperBlock* ConfigReadSuper(struct StorageDevice* dev, int flags, char* data)
{
	struct VfsSuperBlock* ret;
	
	if (dev)
		return NULL;
		
	ret=VfsAllocSuper(dev, flags);
	
	if (!ret)
		goto end;
		
	ret->privData=NULL;
	ret->sbOps=&configFsSbOps;	
	ret->mount=VNodeGet(ret, (DWORD)ConfigGetRootId());

end:
	return ret;
}

struct FileSystem configFileSystem={
	.name="configfs",
	.readSuper=ConfigReadSuper,
};
