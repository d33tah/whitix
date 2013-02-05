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
#include <module.h>
#include <typedefs.h>
#include <i386/i386.h>
#include <slab.h>
#include <user_acc.h>
#include <sys.h>
#include <print.h>
#include <preempt.h>

extern struct Cache* areaCache,*mapCache;

/* FIXME: Move more functions into vmm.c */

int MmapHandleFault(struct Process* process, DWORD address, int error)
{
	struct VMArea* area;

	address=PAGE_ALIGN(address);

	/* A null pointer has been deferenced */
	if (!process || address < PAGE_SIZE)
		return -EFAULT;

	area=VmLookupAddress(process,address);
	
	/* Area of memory not currently mapped? */
	if (!area)
		return -EFAULT;

	/* A few sanity checks here */

	/* Can't write to a read-only page (although processor doesn't know that - page hasn't been mapped in yet */
	if (!(area->protection & PAGE_RW) && (error & VM_WRITE))
		return -EFAULT;

	/* Can't let the user write to a kernel region */
	if (!(area->protection & PAGE_USER) && (error & VM_USER))
		return -EFAULT;

	if (error & VM_PROTECTION)
		return VmProtFault(area,address);
	else
		return VmHandleNoPage(area,address);
}

/***********************************************************************
 *
 * FUNCTION:	MMapAddArea
 *
 * DESCRIPTION: Add an area to the process's area list. The list is
 *				sorted by start address, as this makes it much easier
 *				for MMapFindAddress.
 *
 * PARAMETERS:	process - process in question.
 *				area - area to be added.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

static void MMapAddArea(struct Process* process, struct VMArea* area)
{
	if (ListEmpty(&process->areaList))
		ListAdd(&area->list,&process->areaList);
	else
	{
		struct VMArea* curr=NULL;

		ListForEachEntry(curr,&process->areaList,list)
		{
			if (curr->start > area->start)
				break;
		}

		DoListAdd(&area->list,curr->list.prev,&curr->list);
	}
}

/***********************************************************************
 *
 * FUNCTION: MMapRemoveArea
 *
 * DESCRIPTION: Remove a memory mapped area.
 *
 * PARAMETERS: area - the memory area to be removed.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void MMapRemoveArea(struct VMArea* area)
{
	ListRemove(&area->list);
	VNodeRelease(area->vNode);
	MemCacheFree(areaCache,(void*)area);
}

/***********************************************************************
 *
 * FUNCTION: MMapFreePage
 *
 * DESCRIPTION: Free a virtual page. Get the virtual page in question (if
 *				it is indeed present) and PutPage it. It does not unmap the
 *				page however.
 *
 * PARAMETERS: area - memory mapped area containing page.
 *			   virt - virtual address of page.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/
	
void MMapFreePage(struct VMArea* area, DWORD virt)
{
	struct PhysPage* page;

	return;

	if (PAGE_IS_PRESENT(virt))
	{
		DWORD pageAddr;
		
		pageAddr=VirtToPhys(virt);
		page=PageGetStruct(pageAddr);

		if (page->refs == 1)
			VmFreeMappedPage(area->vNode, page->physAddr);

		if (page->refs <= 0)
		{
			KePrint("%#X, %#X: page->physAddr = %#X (%d) pid = %d\n", 
				virt, page, page->physAddr, page->refs, current->pid);
			cli(); hlt();
		}
	
		PagePut(page);
	}
}

/***********************************************************************
 *
 * FUNCTION: MMapProcessRemove
 *
 * DESCRIPTION: Remove all memory areas linked to a process.
 *
 * PARAMETERS: process - process in question.  No need to VirtUnmapPhysRange
 *			   - as the whole address space will be destroyed soon.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void MmapProcessRemove(struct Process* process)
{
	struct VMArea* curr, *curr2;
	DWORD i;
	DWORD flags;

	IrqSaveFlags(flags);

	ListForEachEntrySafe(curr, curr2, &process->areaList, list)
	{
		for (i=curr->start; i< curr->start+curr->length; i+=PAGE_SIZE)
			MMapFreePage(curr, i);

		MMapRemoveArea(curr);
	}

	IrqRestoreFlags(flags);
}

SYMBOL_EXPORT(MmapProcessRemove);

/***********************************************************************
 *
 * FUNCTION: MMapFindAddress
 *
 * DESCRIPTION: Find a space in memory for an area of a certain length.
 *
 * PARAMETERS: process - process in question.
 *			   length - length needed.
 *
 * RETURNS: Address of suitable area. 0 is returned on error.
 *
 ***********************************************************************/

DWORD MMapFindAddress(struct Process* process,DWORD length)
{
	struct VMArea* curr;
	DWORD mapStart=MMAP_BASE,mapEnd;

	length=PAGE_ALIGN_UP(length);

	ListForEachEntry(curr,&process->areaList,list)
	{
		mapEnd=curr->start;

		if (mapStart <= mapEnd && (mapEnd-mapStart) >= length)
			return mapStart;

		mapStart=curr->start+PAGE_ALIGN_UP(curr->length);
	}

	/* Got to the end, and found no suitable area */
	if (UNLIKELY(mapStart == MMAP_END))
		return 0;

	return mapStart;
}

/***********************************************************************
 *
 * FUNCTION:	MMapMerge
 *
 * DESCRIPTION: Merge two memory areas together, if they can indeed be
 *				merged.
 *
 * PARAMETERS:	first - the first memory area.
 *				second - the second memory area.
 *				anon - is the area mapped to the Zero device?
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void MMapMerge(struct VMArea* first, struct VMArea* second)
{
	if (first->start+first->length != second->start)
		return;

	if (first->vNode != second->vNode)
		return;

	if (first->areaOps != second->areaOps)
		return;

	if (first->flags != second->flags)
		return;
		
	if (first->protection != second->protection)
		return;

	/* Check offsets */
	if (!(first->flags & MMAP_ANON) && second->offset != first->offset+first->length)
		return;

	/* Ok to merge now */
	first->length+=second->length;
	MMapRemoveArea(second);
}

void MMapMergeMappings(struct Process* process, struct VMArea* area)
{
	/* Get neighbours of this area (in terms of memory), and see whether they are 
	   compatible with each other */
	struct VMArea* prev,*next;

	prev=ListEntry(area->list.prev,struct VMArea,list);
	next=ListEntry(area->list.next,struct VMArea,list);

	if (area->list.next != &process->areaList)
		MMapMerge(area, next);

	if (area->list.prev != &process->areaList)
		MMapMerge(prev, area);
}

/***********************************************************************
 *
 * FUNCTION: MMapDo
 *
 * DESCRIPTION: Add a memory mapping to the process's map list.
 *
 * PARAMETERS: process - process in question.
 *			   vNode - VFS node to map to.
 *			   address - address to start mapping at.
 *			   length - length of mapping.
 *			   protection - protection of the mapping.
 *			   offset - offset in VFS node.
 *			   flags - see vmm.h
 *
 * RETURNS: Address if success, 0 if failure.
 *
 ***********************************************************************/

/* FIXME: Split this function up. Eight parameters is bad design. */

DWORD MMapDo(struct Process* process,struct VNode* vNode,DWORD address,DWORD length,int protection,DWORD offset,int flags, struct VmAreaOps* ops)
{
	struct VMArea* area=(struct VMArea*)MemCacheAlloc(areaCache);

	if (!process || !length || !area)
		return 0;
		
	length = PAGE_ALIGN_UP(length);
	
	if (!(flags & MMAP_FIXED))
		address=MMapFindAddress(process, length);

	address = PAGE_ALIGN(address);

	if (address < MMAP_BASE || address >= MMAP_END)
		return 0;

	if (vNode)
	{
		/* Can't allow a area mapped to a file to grow up or down */
		if (flags & (MMAP_GROWDOWN))
			return 0;

		if (!(vNode->mode & VFS_ATTR_FILE))
			return 0;

	}else if (!vNode)
	{
		/* Map to the zero file, so we can use the existing infrastructure. */
		if (NameToVNode(&vNode,DEVICES_PATH "Special/Zero",0, NULL))
			return 0;

		flags |= MMAP_ANON;
	}

	/* Fill in the new structure */
	vNode->refs++;
	area->vNode=vNode;
	area->start=address;
	area->length=length;
	area->offset=offset;
	area->process=process;
	area->protection=protection;
	area->flags=flags;
	area->areaOps=ops;

	MMapUnmap(process, address, length);	

	MMapAddArea(process,area);
	MMapMergeMappings(process, area);

	return address;
}

SYMBOL_EXPORT(MMapDo);

void MMapUnmapArea(struct Process* process,struct VMArea* area,DWORD start,DWORD len)
{
	DWORD end=start+len;
	struct VMArea* newArea;

	/* Just unmapping the whole area */
	if (area->start == start && area->length == len)
		return;

	if (start >= area->start && end == area->start+area->length)
		area->length-=((area->start+area->length)-start);
	else if (start == area->start && end <= area->start+area->length)
	{
		area->offset+=(end-area->start);
		area->start=end;
	}else if (start > area->start && end < area->start+area->length)
	{
		newArea=(struct VMArea*)MemCacheAlloc(areaCache);
		if (!newArea)
			return;

		memcpy(newArea,area,sizeof(struct VMArea));
		newArea->offset+=(end-area->start);
		newArea->start=end;
		newArea->vNode->refs++;
		area->length=end-area->start;
		MMapAddArea(process,newArea);
	}

	newArea=(struct VMArea*)MemCacheAlloc(areaCache);
	if (!newArea)
		return;

	memcpy(newArea,area,sizeof(struct VMArea));

	MMapAddArea(process,newArea);
}

/***********************************************************************
 *
 * FUNCTION: MMapUnmap
 *
 * DESCRIPTION: Unmap a whole or part of a mapping given an address and
 *				length.
 *
 * PARAMETERS: process - process in question.
 *			   start - start address of unmapping.
 *			   len - length of the mapping.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

int MMapUnmap(struct Process* process,DWORD start,DWORD len)
{
	struct VMArea* area,*next;
	DWORD currLen=len;
	DWORD end;
	int err=0;
	
	/* Several sanity checks */
	if (PAGE_OFFSET(start) || start > MMAP_END || len > MMAP_END-start || !len)
		return -EINVAL;

	area=VmLookupAddress(process, start);

	if (!area)
		return -EFAULT;
		
	end = start + len;
	
	if (area->start >= end)
		return 0;

	while (currLen)
	{
		DWORD currStart=(start < area->start) ? area->start : start;
		end=start+len;
		end=(end > area->start+area->length) ? area->start+area->length : end;

		next=ListEntry(area->list.next, struct VMArea, list);
		ListRemove(&area->list);

		MMapUnmapArea(process, area, currStart, end-currStart);

		if (area->list.next == &process->areaList && currLen > 0)
		{
			MemCacheFree(areaCache,(void*)area);
			err=-EINVAL;
			goto error;
		}

		MemCacheFree(areaCache,(void*)area);
		area=next;

		currLen-=(end-start);
	}

error:
	return err;
}

/***********************************************************************
 *
 * FUNCTION: SysMemoryMap
 *
 * DESCRIPTION: The system call to map memory.
 *
 * PARAMETERS: address - address to map to.
 *			   length - length of mapping.
 *			   protection - protection for each page of memory.
 *			   fd - file descriptor to map to.
 *			   offset - offset in file.
 *			   flags - general SysMemoryMap flags.
 *
 * RETURNS : address that memory was mapped to.
 *
 ***********************************************************************/

DWORD SysMemoryMap(DWORD address,DWORD length,int protection,int fd,DWORD offset,int flags)
{
	struct File* file=NULL;
	struct VNode* vNode=NULL;
	
	if (fd != -1 && !(file=FileGet(fd)))
		return 0;

	if (file)
		vNode=file->vNode;
			
	return MMapDo(current, vNode, PAGE_ALIGN(address),
	PAGE_ALIGN_UP(length), protection | PAGE_USER, offset, flags, NULL);
}

int MMapProtectionFixStart(struct VMArea* area, DWORD end, int protection)
{
	struct VMArea* areaNext;
	
	areaNext = (struct VMArea*)MemCacheAlloc(areaCache);
	
	memcpy(areaNext, area, sizeof(struct VMArea));
	
	area->length = end - area->start;
	area->protection = protection;
	
	/* VNode refs? */
	
	areaNext->offset += (end - areaNext->start) >> PAGE_SHIFT;
	areaNext->start = end;
	MMapAddArea(current, areaNext);
	
	return 0;
}

int MMapProtectionFixAll(struct VMArea* area, int protection)
{
	area->protection=protection;
	return 0;
}

int MMapProtectionChange(struct VMArea* area, DWORD start, DWORD end, int protection)
{
	int error=-ENOTIMPL;

	if (protection ==  area->protection)
		return 0;

	if (start == area->start)
	{
		if (end == area->start+area->length)
			error=MMapProtectionFixAll(area, protection);
		else
			error = MMapProtectionFixStart(area, end, protection);
	}else{
		if (protection != 4)
		{
		KePrint("TODO: after start, before end, %#X, %#X, %d\n", start, end, protection);
		cli(); hlt();
		}
	}

	if (error)
		return error;

	VirtChangeProtection(start, end, protection);

	return 0;
}

int SysMemoryProtect(DWORD start, size_t length, int protection)
{
	int error=-EINVAL;
	size_t end;
	size_t address;
	struct VMArea* area;
	
	protection |= PAGE_USER;

	PreemptDisable();

	/* Can only deal with changing the protection of whole pages. */
	if (PAGE_OFFSET(start))
		goto out;

	length=PAGE_ALIGN_UP(length);
	end=start+length;

	if (end < start)
		goto out;

	/* Check protection bits. */

	error=0;

	/* A zero length is valid, but useless. */
	if (start == end)
		goto out;

	area=VmLookupAddress(current, start);

	error=-EFAULT;

	if (!area)
		goto out;

	/* The change of protection may extend over one or more mappings. */
	address=start;

	while (1)
	{
		if ((area->start + area->length) >= end)
		{
			error=MMapProtectionChange(area, address, end, protection);
			break;
		}

		KePrint("SysMemoryProtect: todo\n");
	}

out:
	PreemptEnable();
	return error;
}

/***********************************************************************
 *
 * FUNCTION: SysMemoryUnmap
 *
 * DESCRIPTION: The system call to unmap memory.
 *
 * PARAMETERS: address - base address
 *			   length - amount of memory to unmap.
 *
 * RETURNS: Usual error codes in error.h
 *
 ***********************************************************************/

int SysMemoryUnmap(DWORD address,DWORD length)
{
	return MMapUnmap(current,address, PAGE_ALIGN_UP(length));
}

/* FIXME: Legacy system call. Will be removed soon. */
void* SysMoreCore(int len)
{
	len=PAGE_ALIGN(len);

	KePrint("SysMoreCore: legacy. No longer supported\n");
	return NULL;

	DWORD flags;
	IrqSaveFlags(flags);

	if (len < 0)
	{
		KePrint("TODO: len = %d\n",len);
		/* TODO: Actually release the memory. Use memoryMap */
		current->memManager->end=PAGE_ALIGN(current->memManager->end);
		goto end;
	}

	if (len > 0)
		/* MMapDo merges suitable mappings, so it's ok to map multiple times */
		if (!MMapDo(current, NULL, current->memManager->end, len, PAGE_RW | PAGE_USER | PAGE_PRESENT, 0, MMAP_FIXED | MMAP_PRIVATE, NULL))
			return NULL;

end:
	current->memManager->end+=len;
	IrqRestoreFlags(flags);
	return (void*)(current->memManager->end-len);
}

struct SysCall mmapSysCalls[]=
{
	SysEntry(SysMoreCore, sizeof(int)),
	SysEntry(SysMemoryMap, 24),
	SysEntry(SysMemoryProtect, 12),
	SysEntry(SysMemoryUnmap, 8),
	SysEntryEnd(),
};

int VmInit();

int MMapInit()
{
	VmInit();
	SysRegisterRange(SYS_MMAP_BASE, mmapSysCalls);
	return 0;
}
