/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <fs/vfs.h>
#include <error.h>
#include <task.h>
#include <user_acc.h>
#include <malloc.h>
#include <vmm.h>
#include <print.h>
#include <module.h>
#include <devices/sdevice.h>
#include <spinlock.h>
#include <slab.h>
#include <sections.h>

/* Code to manage the per-process file tables. Split from file.c */

struct Cache* fileCache;

struct File* FileAllocate()
{
	return (struct File*)MemCacheAlloc(fileCache);
}

SYMBOL_EXPORT(FileAllocate);

int FdFree(struct Process* process, struct File** file)
{
	FileFree(*file);
	*file = NULL;
	
	return 0;
}

SYMBOL_EXPORT(FdFree);

/***********************************************************************
 *
 * FUNCTION: FileGet
 *
 * DESCRIPTION: Get the File* corresponding to a fd.
 *
 * PARAMETERS: fd - file descriptor of the file.
 *
 * RETURNS: The file on success, NULL on failure.
 *
 ***********************************************************************/

struct File* FileGet(int fd)
{
	if (fd < 0 || fd >= current->numFds || !current->files)
		return NULL;
	
	return current->files[fd];
}

SYMBOL_EXPORT(FileGet);

void FileFree(struct File* file)
{
	MemCacheFree(fileCache, file);
}

SYMBOL_EXPORT(FileFree);

void VfsFileDup(struct File** src, struct File** dest)
{
	*dest = *src;
	(*dest)->vNode->refs++;
	(*dest)->refs++;
}

#define FILE_COPY_SIZE sizeof(struct File*)

int VfsGetFreeFd(struct Process* process)
{
	int i;

	struct File** tempP;

	/* Write lock? */
	SpinLock(&process->fileListLock);

	/* Look through the current file array of the process, to see if we
	 * can find a free file pointer there.
	 */
	for (i=0; i<process->numFds; i++)
		if (!process->files[i])
			goto out;

	/* Too many file descriptors. */
	if (process->numFds >= 100)
	{
		i=-EMFILE;
		goto out;
	}

	/* Try allocating some more. */
	process->numFds+=10;
	tempP = process->files;
	process->files=(struct File**)MemAlloc(process->numFds * FILE_COPY_SIZE);
	memcpy(process->files, tempP, (process->numFds-10) * FILE_COPY_SIZE);
	memset(&process->files[process->numFds-10], 0, 10 * FILE_COPY_SIZE);
	
	MemFree(tempP);

out:
	SpinUnlock(&process->fileListLock);
	return i;
}

SYMBOL_EXPORT(VfsGetFreeFd);

initcode int FileTableInit()
{
	fileCache = MemCacheCreate("Files", sizeof(struct File), /* FileCtor*/ NULL, NULL, 0);
	return 0;
}
