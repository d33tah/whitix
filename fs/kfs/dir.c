#include <llist.h>
#include <fs/kfs.h>
#include <fs/vfs.h>
#include <error.h>
#include <module.h>
#include <malloc.h>
#include <print.h>

/* TODO: Create KeFsEntry cache. */

/* Returns number of entries in the directory. */
int KeFsDirSize(struct KeFsDir* dir)
{
	struct KeFsEntry* entry;
	int i=0;

	ListForEachEntry(entry, &dir->entries, next)
		i++;

	return i;
}

SYMBOL_EXPORT(KeFsDirSize);

struct KeFsEntry* KeFsLookupDir(struct KeFsDir* dir, char* name, int nameLength)
{
	struct KeFsEntry* entry, *found = NULL;
	
	/* Handle '..'. '..' on the root directory will already have been handled by the VFS. */
	if (nameLength == 2 && name[0] == '.' && name[1] == '.')
	{
		dir=dir->parent;
		found = (struct KeFsEntry*)(((DWORD)dir) - OffsetOf(struct KeFsEntry, dir));
	}else{

		ListForEachEntry(entry,&dir->entries,next)
			if (!strnicmp(name, entry->name, nameLength) && strlen(entry->name) == nameLength)
			{
				found = entry;
				break;
			}
	}

	return found;	
}

SYMBOL_EXPORT(KeFsLookupDir);

struct KeFsEntry* KeFsLookup(struct VNode* dir, char* name, int nameLength)
{
	struct KeFsEntry* entry;
	
	entry=(struct KeFsEntry*)(dir->id);
	
	return KeFsLookupDir(&entry->dir, name, nameLength);
}

SYMBOL_EXPORT(KeFsLookup);

int KeFsReadDir(struct File* file, int (*fillDir)(void*, char*, int, DWORD), void* dirEntries)
{
	struct KeFsEntry* entry;
	struct KeFsEntry* entryDir=(struct KeFsEntry*)(file->vNode->id);
	struct KeFsDir* dir=&entryDir->dir;
	int ret;
	DWORD i = 0;
	
	/* Only updated when the vNode is first read. */
	file->vNode->size = KeFsDirSize(dir);
	
	if (!file->vNode->size)
		return 0;
		
	entry = ListEntry(dir->entries.next, struct KeFsEntry, next);
	
	/* Get to the current file position. */ 
	while (i < file->position)
	{
		entry = ListEntry(entry->next.next, struct KeFsEntry, next);
		i++;
	}

	while (file->position < file->vNode->size)
	{
		ret=fillDir(dirEntries, entry->name, strlen(entry->name), (DWORD)entry);
		
		if (ret < 0)
			return ret;

		entry = ListEntry(entry->next.next, struct KeFsEntry, next);

		file->position++;
	}

	return 0;
}

SYMBOL_EXPORT(KeFsReadDir);

struct KeFsDir* KeFsDirLookup(char* name, int nameLength, struct KeFsDir* dir)
{
	struct KeFsEntry* entry;
	
	/* Look it up */
	ListForEachEntry(entry,&dir->entries,next)
	{
		if ((entry->type & VFS_ATTR_SOFTLINK) && !strnicmp(entry->name, name, nameLength))
		{
			struct KeFsEntry* dest;
			
			if (entry->followLink(&dest, entry))
				return NULL;
			
			if ((dest->type & VFS_ATTR_DIR))
				return &dest->dir;
		}
		
		if ((entry->type & VFS_ATTR_DIR) && !strnicmp(entry->name, name, nameLength))
			return &entry->dir;
	}	
	
	return NULL;
}

SYMBOL_EXPORT(KeFsDirLookup);

struct KeFsDir* KeFsNameToDir(struct KeFsDir* dir, char** name)
{
	char* endName;
	DWORD offset;
	
	/* Get to the directory first. */
	while ((endName=strchr(*name,'/')))
	{
		offset=(DWORD)(endName-*name);
		dir=KeFsDirLookup(*name, offset, dir);
		if (!dir)
			return NULL;
			
		*name+=offset+1;
	}
	
	return dir;
}

SYMBOL_EXPORT(KeFsNameToDir);

struct KeFsEntry* KeFsAddDir(struct KeFsDir* dir, char* name)
{
	struct KeFsDir *newDir;
	struct KeFsEntry* entry;

	/* Check it doesn't exist already. */
	ListForEachEntry(entry, &dir->entries, next)
		if (!strnicmp(entry->name, name, strlen(name)) && strlen(entry->name) == strlen(name))
			return entry;
	
	/* Add entry. */
	entry=(struct KeFsEntry*)MemAlloc(sizeof(struct KeFsEntry)+strlen(name)+1);
	
	if (!entry)
		return NULL;
		
	entry->type = VFS_ATTR_DIR | VFS_ATTR_READ;
	entry->name = name;
	
	newDir=&entry->dir;
	INIT_LIST_HEAD(&newDir->entries);
	newDir->parent=dir;	

	ListAddTail(&entry->next, &dir->entries);
	
	return entry;
}

SYMBOL_EXPORT(KeFsAddDir);

struct KeFsEntry* KeFsAddEntry(struct KeFsDir* dir, char* name, DWORD permissions)
{
	struct KeFsEntry* entry;
	
	/* Check if the name exists already */
	ListForEachEntry(entry, &dir->entries, next)
		if (!strnicmp(entry->name, name, strlen(name)))
			return NULL;

	/* Allocate the KeFsEntry, and the name with it */
	entry=(struct KeFsEntry*)MemAlloc(sizeof(struct KeFsEntry));
	if (!entry)
		return NULL;

	entry->type = VFS_ATTR_FILE | permissions;
	entry->name = name;
	
	ListAddTail(&entry->next, &dir->entries);
	
	return entry;
}

SYMBOL_EXPORT(KeFsAddEntry);

void KeFsInitRoot(struct KeFsEntry* root)
{
	root->type = VFS_ATTR_DIR | VFS_ATTR_READ;
	root->name = NULL;
	INIT_LIST_HEAD(&root->dir.entries);
	root->dir.parent = NULL;
}

SYMBOL_EXPORT(KeFsInitRoot);

/* Recursively remove dir. */
int KeFsRemoveDir(struct KeFsEntry* entry)
{
	struct KeFsDir* dir;
	
	if (!entry || !entry->name)
		return -EFAULT;
	
	if (!(entry->type & VFS_ATTR_DIR))
		return -ENOTDIR;
		
	dir=&entry->dir;
	
	/* The only directory without a parent is the root directory, which we
	 * can't remove. */
	if (!dir->parent)
		return -EPERM;
		
	ListRemove(&entry->next);
	
	ListForEachEntry(entry,&dir->entries,next)
	{
		/* Remove. */
//		KePrint("Remove %s\n", entry->name);
	}
	
	return 0;
}
