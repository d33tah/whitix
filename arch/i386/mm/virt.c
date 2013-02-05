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
 */

#include <i386/virtual.h>
#include <i386/i386.h>
#include <task.h>
#include <fs/vfs.h>
#include <vmm.h>
#include <module.h>
#include <sections.h>
#include <malloc.h>
#include <slab.h>
#include <print.h>

DWORD* pageDir=(DWORD*)PAGE_DIR_VA;
DWORD* pageTable=(DWORD*)PAGE_TABLES_VA;

/* The kernel's memory space - used in start up and when a previous thread has been
 * destroyed.
 */
struct MemManager kernelMem;
LIST_HEAD(managerList);

#define invlpg(page) asm volatile("invlpg %0" :: "m"(*(char*)page))
struct Cache* managerCache;

/***********************************************************************
 *
 * FUNCTION: VirtMemManagerSetup
 *
 * DESCRIPTION: Sets up an address space for a user process.
 *
 * PARAMETERS: process - the process in question.
 *
 ***********************************************************************/

int VirtMemManagerSetup(struct Process* process)
{
	process->memManager=(struct MemManager*)MemCacheAlloc(managerCache);
	if (!process->memManager)
		return -EFAULT;

	return VirtMemManagerInit(process->memManager,false);
}

#define PA_TO_VA(addr) ((DWORD)addr + 0xC0000000)
#define VA_TO_PA(addr) ((DWORD)addr - 0xC0000000)

int VirtMemManagerInit(struct MemManager* manager,int kernel)
{
	DWORD i;

	ZeroMemory(manager,sizeof(struct MemManager));

	if (kernel)
	{
		extern DWORD bootPageDirectory;
		manager->pageDir = (DWORD*)(VA_TO_PA(&bootPageDirectory));
	}else{
		manager->pageDir=(DWORD*)(PageAlloc()->physAddr);

		if (!manager->pageDir)
			goto error;
			
		DWORD* vPageDir, *kPageDir;
	
		/* Map in the new page directory temporarily */
		vPageDir=(DWORD*)VirtAllocateTemp((DWORD)manager->pageDir);
		
		if (!vPageDir)
			goto error;

		ZeroMemory(vPageDir, PAGE_SIZE);

		kPageDir=kernelMem.pageDir;
			
		/* 0xC0000000 to 0xD0000000 is the slab's memory */
		for (i=(0xC0000000 >> 22); i<(0xFF000000 >> 22); i++)
			vPageDir[i]=(DWORD)kPageDir[i];
		
		vPageDir[0] = vPageDir[768];
		
		vPageDir[1023]=((DWORD)manager->pageDir) | PAGE_PRESENT | PAGE_RW | PAGE_KERNEL;
		VirtUnmapPhysPage((DWORD)vPageDir);
	}

	/* Add the new memory manager to the global list */
	if (currThread)
		PreemptDisable();

	ListAddTail(&manager->list, &managerList);

	if (currThread)
		PreemptEnable();

	return 0;

error:
	KePrint("VirtMemManagerInit failed\n");
	return -ENOMEM;
}

int VirtSetCurrent(struct MemManager* manager)
{
	DWORD flags;

	if (!manager)
		return -EFAULT;

	IrqSaveFlags(flags);

	/* Change the current address space */
	SetCr3((DWORD)(manager->pageDir));
	IrqRestoreFlags(flags);

	return 0;
}

SYMBOL_EXPORT(VirtSetCurrent);

int VirtDestroyMemManager(struct MemManager* manager)
{
	/* Go through all the page tables, releasing the memory if allocated. No need to release kernel page tables */
	int i;
	struct PhysPage* curr=NULL;
	DWORD* vPageDir;

	/* Is this the current address space? If so, switch to the kernel
	 * one for safety, as this one will be freed. */

	PreemptDisable();

	if (manager == current->memManager)
		VirtSetCurrent(&kernelMem);

	vPageDir=(DWORD*)VirtAllocateTemp((DWORD)manager->pageDir);

	/* Free all the private page directories */
	for (i=2; i<768; i++)
	{
		if (vPageDir[i] & PAGE_PRESENT)
		{
			curr=PageGetStruct(vPageDir[i]);
			PagePut(curr);
		}
	}

	VirtUnmapPhysPage((DWORD)vPageDir);
	ListRemove(&manager->list);

	PreemptEnable();

	MemCacheFree(managerCache,manager);

	return 0;
}

int VirtEarlyInit()
{
	/* Init the kernel page tables */
	VirtMemManagerInit(&kernelMem, true);

	return 0;
}

int VirtInit()
{	
	managerCache=MemCacheCreate("VmManagers", sizeof(struct MemManager), NULL, NULL, 0);

	if (!managerCache)
		return -ENOMEM;

	return 0;
}

int VirtMemMapPage(DWORD virt,DWORD phys,int perms)
{
	DWORD virtEnt, flags;
	struct MemManager* manager;
	DWORD* virtDir;

	/* Can't map over the mapped page tables */
	if (virt >= (DWORD)PAGE_TABLES_VA)
		return -EFAULT;

	virtEnt=PGDIR_ENT(virt);

	IrqSaveFlags(flags);

	if (!pageDir[virtEnt])
	{
		pageDir[virtEnt]=PageAlloc()->physAddr | perms | PAGE_RW;
		/* Zero out the whole page table */
		ZeroMemory(&pageTable[virtEnt*1024], PAGE_SIZE);

		/* If in kernel memory space, map to all other page directories */
		if (virtEnt >= 768)
		{
			ListForEachEntry(manager,&managerList,list)
			{
				virtDir=(DWORD*)VirtAllocateTemp((DWORD)manager->pageDir);
				virtDir[virtEnt]=pageDir[virtEnt];
				VirtUnmapPhysPage((DWORD)virtDir);
			}
		}
	}

	pageTable[PGTABLE_ENT(virt)]=phys | perms;
	invlpg(virt);

	IrqRestoreFlags(flags);
	return 0;
}

SYMBOL_EXPORT(VirtMemMapPage);

void VirtUnmapPages(DWORD start,DWORD len)
{
	DWORD curr;

	len=PAGE_ALIGN_UP(len);
	start=PAGE_ALIGN(start);

	for (curr=start; curr<start+len; curr+=PAGE_SIZE)
		VirtUnmapPhysPage(curr);
}

/* 
 * Whitix currently doesn't map every physical page into kernel memory, because that allows just < 1gb on a system.
 * So the current way to map pages is to call this function, with a virtual address range that's acceptable 
 * Should it?
 */

DWORD VirtMapPhysPage(DWORD virt,DWORD endVirt,int perms)
{
	/* Allocate a page */
	struct PhysPage* physPage;
	DWORD page;
	DWORD i;
	DWORD flags;

	IrqSaveFlags(flags);

	physPage=PageAlloc();
	if (!physPage)
		return 0;

	page=physPage->physAddr;

	for (i=virt; i<endVirt; i+=PAGE_SIZE)
	{
		if (!(pageDir[PGDIR_ENT(i)]) || !(pageTable[PGTABLE_ENT(i)])) /* Definitely not used */
		{
			VirtMemMapPage(i,page,perms);
			IrqRestoreFlags(flags);
			return i;
		}
	}

	KePrint("VirtMapPhysPage failed\n");
	IrqRestoreFlags(flags);
	return 0;
}

SYMBOL_EXPORT(VirtMapPhysPage);

DWORD VirtMapPhysRange(DWORD virt,DWORD endVirt,DWORD numPages,int perms)
{
	DWORD currNum=numPages;
	DWORD start=0;
	DWORD i;
	DWORD flags;

	IrqSaveFlags(flags);

//	KePrint("VirtMapPhysRange(%#X,%#X,%u,%d)\n",virt,endVirt,numPages,perms);

	for (i=virt; i<endVirt; i+=PAGE_SIZE)
	{
		if (!pageDir[PGDIR_ENT(i)] || !pageTable[PGTABLE_ENT(i)]) /* Not used */
		{
			if (currNum == numPages)
				start=i;
			--currNum;
			if (!currNum) /* Got enough virtual pages in order */
				break;
		}else
			currNum=numPages; /* Hrmph :) */
	}

	IrqRestoreFlags(flags);

	if (i == endVirt) /* Failure */
		return 0;

	if (currNum)
		return 0;

	IrqSaveFlags(flags);

	/* Now map them in */
	for (i=start; i<start+(numPages*PAGE_SIZE); i+=PAGE_SIZE)
	{
		struct PhysPage* page=PageAlloc();

		if (!page)
		{
			/* Unmap the pages mapped so far. */
/*			for (i-=PAGE_SIZE; i>start; i-=PAGE_SIZE)
				VirtUnmapPhysPage(PageGetStruct(PAGE_ALIGN(VirtToPhys(i)))); */

			return 0;
		}

		VirtMemMapPage(i,page->physAddr,perms);
	}

	IrqRestoreFlags(flags);

	return start;
}

SYMBOL_EXPORT(VirtMapPhysRange);

int VirtChangeProtection(DWORD start, DWORD end, int protection)
{
	DWORD address;

	for (address=start; address<end; address+=PAGE_SIZE)
	{
		DWORD virtEnt=PGDIR_ENT(address);

		if (pageDir[virtEnt])
		{
			if (pageTable[PGTABLE_ENT(address)] & PAGE_PRESENT)
			{
				pageTable[PGTABLE_ENT(address)] &= ~(0xFFF);
				pageTable[PGTABLE_ENT(address)] |= protection;
//				KePrint("address = %#X\n", pageTable[PGTABLE_ENT(address)]);
			}
		}
	}

	return 0;
}

SYMBOL_EXPORT(VirtChangeProtection);

void VirtUnmapPhysPage(DWORD virt)
{
	DWORD flags;
	
	IrqSaveFlags(flags);
		
	if (!pageDir[PGDIR_ENT(virt)] || !pageTable[PGTABLE_ENT(virt)])
		/* Doesn't exist */
		goto out;

	pageTable[PGTABLE_ENT(virt)]=0;
	invlpg(virt);
	
out:
	IrqRestoreFlags(flags);
}

SYMBOL_EXPORT(VirtUnmapPhysPage);

DWORD VirtAllocateTemp(DWORD phys)
{
	DWORD i;

	/* 0xF0080000 to 0xF8000000 is the space for temporarily mapping physical memory */

	for (i=0xF0080000; i<0xF8000000; i+=PAGE_SIZE)
		if (!pageDir[PGDIR_ENT(i)] || !pageTable[PGTABLE_ENT(i)]) /* Definitely not used */
		{
			VirtMemMapPage(i,phys,PAGE_PRESENT | PAGE_KERNEL | PAGE_RW);
			return i;
		}

	/* Failure */
	return 0;
}

SYMBOL_EXPORT(VirtAllocateTemp);

void VirtShowFault(DWORD address,int error)
{
	KePrint("\nAddress = %#X (pageDir[%d] = %#X)",address,address >> 22,pageDir[address >> 22]);

	if (pageDir[address >> 22])
		KePrint(" pageTable[%d] = %#X (%d)",address >> 12,pageTable[address >> 12], pageTable[address >> 12] & 7);

	KePrint("\n");

	if (error & 1)
		KePrint("Protection violation, ");
	else
		KePrint("Page not present, ");
				
	if (error & 2)
		KePrint("writing, ");
	else
		KePrint("reading, ");
				
	if (error & 4)
		KePrint("in user mode\n");
	else
		KePrint("in kernel mode\n");	
}

DWORD VirtToPhys(DWORD virt)
{
	return PAGE_ALIGN(pageTable[PGTABLE_ENT(virt)])+PAGE_OFFSET(virt);
}

SYMBOL_EXPORT(VirtToPhys);

