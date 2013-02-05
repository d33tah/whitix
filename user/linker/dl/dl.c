#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>

#define DL /* The linker code sets dlErrno then. */

#include "../load_lib.c"
#include "../hash.c"
#include "../relocate.c"

extern int dlErrno;
extern struct ElfResolve* loadedModules;

/* TODO: Add list to keep track of resolves. */

void* dlopen(const char* file,int mode)
{
	struct ElfResolve* resolve, *entry;

	resolve=DlLoadElfLibrary(file);

	if (!resolve)
		return NULL;

	for (entry=loadedModules; entry; entry=entry->next)
	{
		struct ElfDyn* dynEntry;
		for (dynEntry=(struct ElfDyn*)entry->dynamicAddr; dynEntry->tag; dynEntry++)
		{
			if (dynEntry->tag == DT_NEEDED)
			{
				struct ElfResolve* newEnt;

				char* name=(char*)(entry->dynamicInfo[DT_STRTAB]+dynEntry->val);
				newEnt=DlLoadElfLibrary(name);

				if (!newEnt)
				{
					/* Cleanup */	
					printf("Failed to load %s\n", name);
					SysExit(0);
				}
			}
		}
	}

	if (symbolTables)
		DlFixupSymbols(symbolTables);

	return resolve;
}

int dlclose(void* handle)
{
	struct ElfResolve* resolve;
	struct ElfSymResolve* symResolve = NULL, * curr;

	resolve = (struct ElfResolve*)handle;

	for (curr = symbolTables; curr; curr = curr->next)
	{
		if (curr->resolveEntry == resolve)
			break;

		symResolve = curr;
	}

	printf("dlclose: TODO.\nsymResolve = %#X, curr = %#X\n", symResolve, curr);

	return 0;
}

char* dlerror()
{
	if (!dlErrno)
		return NULL;

	return strerror(dlErrno);
}

void* dlsym(void* handle,const char* name)
{
	struct ElfResolve* resolve=(struct ElfResolve*)handle;
	unsigned long symHash = DlElfGnuHash(name);
	unsigned long oldHash = 0xFFFFFFFF;

	if (!handle)
	{
		dlErrno=EINVAL;
		return NULL;
	}

	return DlFindHashResolve(name, resolve, symHash, &oldHash, 0);
}
