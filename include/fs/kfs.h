#ifndef KFS_H
#define KFS_H

#include <llist.h>

struct VNode;
struct File;
struct IcFsEntry;

/* Structures for kernel filesystems. */
struct KeFsDir
{
	struct ListHead entries; /* All DevFsEntrys */
	struct KeFsDir* parent;
};

struct KeFsEntry
{
	struct ListHead next;
	int type;
	
	union {
		struct KeFsDir dir;
		void* file;
		int (*followLink)(struct KeFsEntry** dest, struct KeFsEntry* src);
	};
	
	char* name;
};

/* Standard operation on kernel filesystems. */
void KeFsInitRoot(struct KeFsEntry* root);
int KeFsDirSize(struct KeFsDir* dir);
struct KeFsEntry* KeFsLookup(struct VNode* dir, char* name, int nameLength);
int KeFsReadDir(struct File* file, int (*fillDir)(void*, char*, int, DWORD), void* dirEntries);
struct KeFsEntry* KeFsAddDir(struct KeFsDir* dir, char* name);
struct KeFsEntry* KeFsAddEntry(struct KeFsDir* dir, char* name, DWORD permissions);
struct KeFsDir* KeFsDirLookup(char* name, int nameLength, struct KeFsDir* dir);
struct KeFsDir* KeFsNameToDir(struct KeFsDir* dir, char** name);
struct KeFsEntry* KeFsLookupDir(struct KeFsDir* dir, char* name, int nameLength);

#endif
