#include <module.h>
#include <error.h>
#include <print.h>
#include <fs/kfs.h>
#include <fs/icfs.h>
#include <fs/vfs.h>
#include <sys.h>

#include "icfs.h"

extern struct KeFsEntry root;

struct KeFsEntry* ConfigResolvePath(char* name)
{
	/* Parse name. */
	struct KeFsDir* dir;
	struct KeFsEntry* entry;
	
	if (name[0] == '/')
		name++;
	
	dir=KeFsNameToDir(&root.dir, &name);
	
	if (!dir)
		return NULL;

	entry=KeFsLookupDir(dir, name, strlen(name));
	
	if (!entry)
		return NULL;
	
	/* Handle symlinks in KeFsLookupDir? */
	if (!(entry->type & VFS_ATTR_FILE))
		return NULL;
		
	return entry;
}

int SysConfRead(char* name, BYTE* data, int size)
{
	struct KeFsEntry* entry;
	DWORD position=0;
	
	entry=ConfigResolvePath(name);
	
	if (!entry)
		return -ENOENT;
	
	return IcRead(&position, entry->file, data, size);
}

int SysConfWrite(char* name, BYTE* data, int size)
{
	struct KeFsEntry* entry;
	DWORD position=0;
	
	entry=ConfigResolvePath(name);

	if (!entry)
		return -ENOENT;
	
	return IcWrite(&position, entry->file, data, size);
}

struct SysCall configSystemCalls[]=
{
	SysEntry(SysConfRead, 12),
	SysEntry(SysConfWrite, 12),
	SysEntryEnd()
};

int ConfigInit()
{
	/* Register the ReadConf and WriteConf system calls, for quick access to configuration variables. */
	SysRegisterRange(SYS_CONFIG_BASE, configSystemCalls);
	
	return 0;
}
