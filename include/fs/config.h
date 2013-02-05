#ifndef FS_CONFIG_H
#define FS_CONFIG_H

#include <fs/icfs.h>

/* ConfigFs API, to be used by drivers. */
int ConfigAddIntEntry(struct KeFsEntry* dir, char* name, void* value);
struct KeFsEntry* ConfigCreateDir(struct KeFsEntry* dir, char* name);
int ConfigAddStringEntry(struct KeFsEntry* dir, char* name, char* value, int minLen, int maxLen);
int ConfigAddSoftLink(struct KeFsEntry* dir, char* name, int (*followLink)(char* buffer, int size));
int ConfigAddArrayEntry(struct KeFsEntry* dir, char* name, BYTE* value, int minLen, int maxLen, int size);

typedef struct KeFsEntry ConfigDir;

#endif
