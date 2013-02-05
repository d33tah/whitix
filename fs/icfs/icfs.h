#ifndef _ICFS_H
#define _ICFS_H

#include <fs/icfs.h>
#include <typedefs.h>
#include <malloc.h>

/* Internal function prototypes. */
int IcRead(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size);
int IcWrite(DWORD* position, struct IcFsEntry* iEnt, char* data, DWORD size);

DWORD IcFsGetRootId();

/* Internal utility functions. */

static inline struct IcFsEntry* IcEntryAlloc()
{
	return (struct IcFsEntry*)MemAlloc(sizeof(struct IcFsEntry));
}

#endif
