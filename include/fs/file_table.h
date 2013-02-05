#ifndef VFS_FILE_TABLE_H
#define VFS_FILE_TABLE_H

struct Process;
struct File;

struct File* FileAllocate();
int FileTableInit();
void FileFree(struct File* file);
int FdFree(struct Process* process, struct File** file);
void VfsFileDup(struct File** src, struct File** dest);

#endif
