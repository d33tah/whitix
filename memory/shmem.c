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
#include <vmm.h>
#include <llist.h>
#include <malloc.h>
#include <typedefs.h>
#include <i386/i386.h>
#include <slab.h>
#include <user_acc.h>
#include <sys.h>
#include <module.h>
#include <print.h>

#define SHMEM_MAX_DESCS		10

struct SharedMemoryDesc
{
	int creatorPid;
	DWORD size;
	DWORD* pages;
};

struct SharedMemoryDesc* shMemDescs[SHMEM_MAX_DESCS];

int ShMemNoPage(struct VMArea* area, DWORD address, DWORD offset);
int ShMemAddPage(struct VMArea* area, DWORD address);

struct VmAreaOps shMemOps=
{
	.handleNoPage = ShMemNoPage,
	.addPage = ShMemAddPage,
};

static struct SharedMemoryDesc* ShMemGetDesc(int i)
{
	if (i < 0 || i >= SHMEM_MAX_DESCS)
		return NULL;

	return shMemDescs[i];
}

int SharedMemCreate(size_t size, int flags)
{
	int numPages=PAGE_ALIGN_UP(size) >> PAGE_SHIFT;
	int i, id;
	struct SharedMemoryDesc* shDesc;

	if (!numPages)
		return -EINVAL;

	/* Find an empty shared memory description slot. */
	for (i=0; i<SHMEM_MAX_DESCS; i++)
		/* Descriptor slot not occupied? */
		if (!shMemDescs[i])
			break;

	if (i == SHMEM_MAX_DESCS)
		return -EMFILE;

	shMemDescs[i]=shDesc=(struct SharedMemoryDesc*)MemAlloc(sizeof(struct SharedMemoryDesc));

	id=i;

	if (!shDesc)
		return -ENOMEM;

	/* Fill in the shared memory descriptor */
	shDesc->size=PAGE_ALIGN_UP(size);
	shDesc->pages=(DWORD*)MemAlloc(sizeof(DWORD)*numPages);

	if (!shDesc->pages)
		return -ENOMEM;

	for (i=0; i<numPages; i++)
		shDesc->pages[i]=0;

	return id+1;
}

int SysSharedMemoryGet(unsigned int key, size_t size, int flags)
{
//	KePrint("SysSharedMemoryGet(%d, %u)\n", key, size);
	if (key == 0)
		return SharedMemCreate(size, flags);
	else{
		KePrint("SysSharedMemoryGet: key > 0\n");
		return -EINVAL;
	}

	return 0;
}

DWORD SysSharedMemoryAttach(int id, DWORD address, int flags)
{
	struct SharedMemoryDesc* desc=ShMemGetDesc(id-1);

//	KePrint("%s: SysSharedMemoryAttach(%d, %#X), %#X, size = %#X\n", current->name, id, address, desc, desc->size);

	if (!desc)
		return 0;

	return MMapDo(current, NULL, address, desc->size, 7, 0, MMAP_PRIVATE, &shMemOps);
}

/* FIXME: Rewrite. */
struct SharedMemoryDesc* ShMemFind(DWORD length)
{
	/* FIXME: Have private field for VMArea. */
	int i=0;

	for (i=0; i<SHMEM_MAX_DESCS; i++)
	{
		/* FIXME: Hack! */
		if (shMemDescs[i] && shMemDescs[i]->size == length)
			break;
	}

	if (i == SHMEM_MAX_DESCS)
		return NULL;

	return shMemDescs[i];
}

int ShMemAddPage(struct VMArea* area, DWORD address)
{
	struct SharedMemoryDesc* shMem=ShMemFind(area->length);
	int pageIndex=(address-area->start) >> PAGE_SHIFT;

//	KePrint("pageIndex = %d\n", pageIndex);

	if (!shMem)
		return -EFAULT;

	while (1);

	return 0;
}

/* TODO: Try to merge with the general mmap code perhaps? */
int ShMemNoPage(struct VMArea* area, DWORD address, DWORD offset)
{
	int pageIndex=offset >> PAGE_SHIFT;
	struct SharedMemoryDesc* shMem=ShMemFind(area->length);

	if (!shMem)
		return -EFAULT;

	if (!shMem->pages[pageIndex])
		shMem->pages[pageIndex]=PageAlloc()->physAddr;

//	KePrint("%s: ShMemNoPage(%d) = %#X\n", current->name, pageIndex, shMem->pages[pageIndex]);

	/* HACK: Need to have flag and reconsider whether should map here. */
	struct VNode* vNode;

	NameToVNode(&vNode,DEVICES_PATH "Special/Zero",0, NULL);

	if (area->vNode->id == vNode->id)
	{
		VirtMemMapPage(address, shMem->pages[pageIndex], 7);
		return 0;
	}

	return -EFAULT;
}

int SysSharedMemoryControl(int id, int command, void* buffer)
{
	KePrint("SysSharedMemoryControl(%d)\n", id);
	return 0;
}

int SysSharedMemoryDetach(const void* address)
{
	KePrint("SysSharedMemoryDetach(%#X)\n", address);
	return 0;
}

struct SysCall shMemSysCalls[]=
{
	SysEntry(SysSharedMemoryGet, 12),
	SysEntry(SysSharedMemoryAttach, 12),
	SysEntry(SysSharedMemoryControl, 12),
	SysEntry(SysSharedMemoryDetach, 4),
	SysEntryEnd(),
};

int ShMemInit()
{
	SysRegisterRange(SYS_SHMEM_BASE, shMemSysCalls);
	return 0;
}

ModuleInit(ShMemInit);
