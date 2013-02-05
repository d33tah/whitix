#include <error.h>
#include <fs/kfs.h>
#include <fs/icfs.h>
#include <fs/vfs.h>
#include <module.h>
#include <malloc.h>
#include <print.h>

#include "icfs.h"

struct KeFsEntry root;

int InfoReadInt(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	DWORD copied;
	
	copied=MIN(size-*position, sizeof(int)-*position);
	
	memcpy(data, iEnt->iVal+*position, copied);
	
	*position+=copied;
	
	return copied;
}

int InfoReadStr(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	DWORD copied;
	
	copied=MIN(size-*position, (DWORD)strlen(iEnt->sVal)-*position);
	
	memcpy(data, iEnt->sVal+*position, copied);
	
	*position+=copied;
	
	return copied;
}

int InfoReadArray(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	DWORD copied;
	
	copied=MIN(size-*position, (DWORD)iEnt->size-*position);
	
	memcpy(data, iEnt->csVal+*position, copied);
	
	*position+=copied;
	
	return copied;
}

int InfoReadFunc(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	int ret;

	if (iEnt->readFunc == NULL)
		return -EPERM;

	ret = iEnt->readFunc(data, size, *position);

	if (ret >= 0)
		*position += ret;

	return ret;
}

int IcRead(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	switch (iEnt->type)
	{
		case ICFS_TYPE_INT:
			return InfoReadInt(position, iEnt, data, size);
			
		case ICFS_TYPE_STR:
			return InfoReadStr(position, iEnt, data, size);
			
		case ICFS_TYPE_BYTE:
			return InfoReadArray(position, iEnt, data, size);
		
		case ICFS_TYPE_FUNC:
			return InfoReadFunc(position, iEnt, data, size);	
	
		default:	
			KePrint("IcRead: %d\n", iEnt->type);
	}
	
	return -EINVAL;
}

SYMBOL_EXPORT(IcRead);

int IcWriteInt(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	DWORD copied;
	
	copied=MIN(size-*position, sizeof(int)-*position);
	
	memcpy(iEnt->iVal+*position, data, copied);
	
	*position+=copied;
	
	return copied;
}

int IcWrite(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size)
{
	switch (iEnt->type)
	{
		case ICFS_TYPE_INT:
			return IcWriteInt(position, iEnt, data, size);
			
		default:
			KePrint("IcWrite: %d\n", iEnt->type);		
	}
	
	return -EINVAL;
}

SYMBOL_EXPORT(IcWrite);

int IcFsAddIntEntry(struct KeFsEntry* dir, char* name, int* value, DWORD permissions)
{
	struct KeFsEntry* entry;
	struct IcFsEntry* newEnt;
	
	if (!dir)
		dir=&root;
	
	entry=KeFsAddEntry(&dir->dir, name, permissions);
	
	newEnt = IcEntryAlloc();
	newEnt->type=ICFS_TYPE_INT;
	newEnt->iVal=value;
	
	entry->file=newEnt;
	
	return 0;
}

SYMBOL_EXPORT(IcFsAddIntEntry);

struct IcFsEntry* IcFsAddStrEntry(struct KeFsEntry* dir, char* name, char* str, DWORD permissions)
{
	struct KeFsEntry* entry;
	struct IcFsEntry* newEnt;
	
	if (!dir)
		dir=&root;
		
	entry=KeFsAddEntry(&dir->dir, name, permissions);
	
	newEnt = IcEntryAlloc();
	newEnt->type = ICFS_TYPE_STR;
	newEnt->sVal = str;
	
	entry->file=newEnt;
	
	return newEnt;
}

SYMBOL_EXPORT(IcFsAddStrEntry);

int IcFsAddArrayEntry(struct KeFsEntry* dir, char* name, char* array, unsigned int minLen, unsigned int size, unsigned int maxLen)
{
	struct KeFsEntry* entry;
	struct IcFsEntry* newEnt;
	
	if (!dir)
		dir = &root;
		
	entry = KeFsAddEntry ( &dir->dir, name, VFS_ATTR_RW	);
	
	newEnt = IcEntryAlloc();
	newEnt->type = ICFS_TYPE_BYTE;
	newEnt->csVal = array;
	newEnt->minLen = minLen;
	newEnt->maxLen = maxLen;
	newEnt->size = size;
	
	entry->file = newEnt;	
	return 0;
}

SYMBOL_EXPORT(IcFsAddArrayEntry);

int IcFsAddFuncEntry(struct KeFsEntry* dir, char* name, IcReadFunc readFunc, IcWriteFunc writeFunc)
{
	struct KeFsEntry* entry;
	struct IcFsEntry* newEnt;
	int permissions = 0;
	
	if (!dir)
		dir = &root;
		
	if (readFunc)
		permissions |= VFS_ATTR_READ;
		
	if (writeFunc)
		permissions |= VFS_ATTR_WRITE;
		
	entry = KeFsAddEntry ( &dir->dir, name, permissions );
	
	newEnt = IcEntryAlloc();
	newEnt->type = ICFS_TYPE_FUNC;
	newEnt->readFunc = readFunc;
	newEnt->writeFunc = writeFunc;
	
	entry->file = newEnt;
	
	return 0;
}

SYMBOL_EXPORT(IcFsAddFuncEntry);

int IcFsAddSoftLink(struct KeFsEntry* dir, char* name,
	int (*followLink)(struct KeFsEntry**, struct KeFsEntry*))
{
	struct KeFsEntry* entry;
	
	if (!dir)
		dir=&root;
		
	entry=KeFsAddEntry(&dir->dir, name, VFS_ATTR_READ);
	
	entry->type &= ~VFS_ATTR_FILE;
	entry->type |= VFS_ATTR_SOFTLINK;
	
	entry->followLink=followLink;
	
	return 0;
}

SYMBOL_EXPORT(IcFsAddSoftLink);

struct KeFsEntry* IcFsCreateDir(struct KeFsEntry* dir, char* name /*, DWORD permissions */)
{
	if (!dir)
		dir=&root;
		
	return KeFsAddDir(&dir->dir, name);
}

int IcFsRemoveDir(struct KeFsEntry* dir)
{
	return KeFsRemoveDir(dir);
}

/* Process an array of IC attributes and add them to a particular directory. */
int IcAddAttributes(struct KeObject* object, struct IcAttribute* attributes)
{
	char* base;
	int objOffset;
	
	if (!object->parent || !object->parent->type || !object->dir)
		return -EFAULT;
		
	objOffset = object->parent->type->offset;
		
	if (objOffset < 0)
	{
		KePrint(KERN_DEBUG "icfs: invalid object offset in type.\n");
		return -EFAULT;
	}
	
	base = ((char*)object - objOffset);
	
	/* Go through the array and register each element.*/
	while (attributes->type != -1)
	{
		int permissions = 0;
		
		/* Convert IC permissions to VFS permissions. */
		if (attributes->perms == IC_READ)
			permissions = VFS_ATTR_READ;
		else if (attributes->perms == IC_RW)
			permissions = VFS_ATTR_WRITE;
		
		switch (attributes->type)
		{
			case ICFS_TYPE_INT:
				IcFsAddIntEntry(object->dir, attributes->name, (int*)(base+attributes->offset),
					permissions);
				break;
				
			case ICFS_TYPE_STR:
				IcFsAddStrEntry(object->dir, attributes->name, *(char**)(base+attributes->offset),
					permissions);
				break;
				
			default:
				KePrint(KERN_WARNING "icfs: unhandled object type %d\n", attributes->type);
				break;
				
		}

		attributes++;
	}
	
	return 0;
}


DWORD IcFsGetRootId()
{
	return (DWORD)&root;
}

SYMBOL_EXPORT(IcFsGetRootId);

int InfoInit()
{
	KeFsInitRoot(&root);
	
	return 0;
}
