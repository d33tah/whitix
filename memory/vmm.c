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
#include <locks.h>
#include <llist.h>
#include <malloc.h>
#include <module.h>
#include <typedefs.h>
#include <i386/i386.h>
#include <slab.h>
#include <user_acc.h>

struct Cache* areaCache,*mapCache;
SYMBOL_EXPORT(areaCache);
SYMBOL_EXPORT(mapCache);

/***********************************************************************
 *
 * FUNCTION: VmLookupAddress
 *
 * DESCRIPTION: Look up an address in the process's region list.
 *
 * PARAMETERS: process - the process in question.
 *			   address - the address in question.
 *
 * RETURNS: area that the address is contained in.
 *
 ***********************************************************************/

struct VMArea* VmLookupAddress(struct Process* process,DWORD address)
{
	struct VMArea* curr, *curr2;

	ListForEachEntrySafe(curr, curr2, &process->areaList, list)
	{ 
		/* Ordered by vm area start address, so don't have to search through
		the whole list if the address has already been passed. */
		if (curr->start > address)
			break;
		
		/* Is the address in the virtual mapping? */
		if (address >= curr->start && address < (curr->start+curr->length))
			return curr;
	}

	return NULL;
}

SYMBOL_EXPORT(VmLookupAddress);

/***********************************************************************
 *
 * FUNCTION: VmLookupPage
 *
 * DESCRIPTION: Looks up a page in a vNode by file offset to see if anyone
 *				has mapped the page already.
 *
 * PARAMETERS: vNode - vNode that contains the shared list.
 *			   offset - file offset needed.
 *
 * RETURNS: the virtual page containing the file data.
 *
 ***********************************************************************/

struct VMMapPage* VmLookupPage(struct VNode* vNode, DWORD offset)
{
	struct VMMapPage* page = NULL, *curr;

	/* Has anyone else mapped it in? */
	if (!vNode || vNode->refs == 1 || ListEmpty(&vNode->sharedList))
		return NULL;
		
	PreemptDisable();

	/* Loop through the shared list and return the one with the offset we need. */
	ListForEachEntry(curr, &vNode->sharedList, list)	
		if (curr->offset == offset)
		{
			page = curr;
			break;
		}
		
	PreemptFastEnable();

	if (page)
	{
		PageGet(page->page);
		VmWaitForPage(page);
	}

	return page;
}

SYMBOL_EXPORT(VmLookupPage);

/***********************************************************************
 *
 * FUNCTION: VmCreateMappedPage
 *
 * DESCRIPTION: Looks up a page in a vNode by file offset.
 *
 * PARAMETERS: vNode - vNode that contains the shared list.
 *			   offset - file offset needed.
 *
 * RETURNS: the virtual page containing the file data.
 *
 ***********************************************************************/

struct VMMapPage* VmCreateMappedPage(DWORD offset, struct PhysPage* page)
{
	struct VMMapPage* mappedPage=(struct VMMapPage*)MemCacheAlloc(mapCache);

	if (!mappedPage)
		return NULL;

	mappedPage->offset=offset;
	mappedPage->page=page;

	return mappedPage;
}

SYMBOL_EXPORT(VmCreateMappedPage);

/***********************************************************************
 *
 * FUNCTION: VmFreeMappedPage
 *
 * DESCRIPTION: Free a mapped page by physical address.
 *
 * PARAMETERS: vNode - VFS node containing the reference to the page.
 *			   physAddr - physical address of the page.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

int VmFreeMappedPage(struct VNode* vNode, DWORD physAddr)
{
	struct VMMapPage* page=NULL, *page2;

	if (!vNode)
		return -EFAULT;

	/* Remove a physical page from the list, because there are no references and it will be freed */

	ListForEachEntrySafe(page, page2, &vNode->sharedList, list)
		if (page->page->physAddr == physAddr)
		{
			VmWaitForPage(page);
			ListRemove(&page->list);
			return 0;
		}
	
	return -ENOENT;
}

SYMBOL_EXPORT(VmFreeMappedPage);

/***********************************************************************
 *
 * FUNCTION:	VmWaitOnPage
 *
 * DESCRIPTION: Wait for another thread to stop altering the page. 
 *				A single waitqueue is shared between a number of pages 
 *				(one per 4mb+ or so) since there are not many conflicts 
 *				for a single page - it is very unlikely that a page will 
 *				be locked and someone else try to access it.
 *
 * PARAMETERS:	page - page in question.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

static void _VmWaitOnPage(struct VMMapPage* page)
{
	WaitQueue* waitQueue;

	PageGet(page->page);

	waitQueue=PageGetWaitQueue(page->page);

	WAIT_ON(waitQueue, !PageLocked(page));
	
	PagePut(page->page);
}

void VmWaitForPage(struct VMMapPage* page)
{
	if (PageLocked(page))
		_VmWaitOnPage(page);
}

SYMBOL_EXPORT(VmWaitForPage);

/***********************************************************************
 *
 * FUNCTION: VmLockPage
 *
 * DESCRIPTION: Locks a page to alter data.
 *
 * PARAMETERS: page - page in question.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void VmLockPage(struct VMMapPage* page)
{
	while (BitTestAndSet(&page->flags, PAGE_LOCKED))
		_VmWaitOnPage(page);
}

SYMBOL_EXPORT(VmLockPage);

/***********************************************************************
 *
 * FUNCTION: VmUnlockPage
 *
 * DESCRIPTION: Unlock the page, let any thread access it.
 *
 * PARAMETERS: page - page in question.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void VmUnlockPage(struct VMMapPage* page)
{
	BitClear(&page->flags, PAGE_LOCKED);	
	
	/* Wake up one? */
	
	WakeUpAll(PageGetWaitQueue(page->page));
}

SYMBOL_EXPORT(VmUnlockPage);

int VirtCheckArea(void* beginAddr,DWORD size,int mask)
{
	DWORD begin=(DWORD)beginAddr;
	struct VMArea* area,*next;

	/* Don't have to check */
	if (!size)
		return 0;

	area=VmLookupAddress(current,begin);

	if (!area)
		return -EFAULT;

	/* Cannot write to a read-only area */
	if (!(area->protection & PAGE_RW) && (mask & VER_WRITE))
		return -EFAULT;

	/* Doesn't overrun */
	if ((area->start+area->length)-begin >= size)
		return 0;

	/* Check when size overruns the area */
	while (1)
	{
		/* Assumes all areas are at least readable */
		if (!(area->protection & PAGE_RW) && (mask & VER_WRITE))
			return -EFAULT;

		/* User cannot access kernel memory. */
		if (!(area->protection & PAGE_USER))
			return -EFAULT;

		if ((area->start+area->length)-begin >= size)
			break;

		next=ListEntry(area->list.next,struct VMArea,list);
		if (area->list.next == &current->areaList || next->start != area->start+area->length) /* Does next even exist? */
			return -EFAULT;

		area=next;
	}

	return 0;
}

SYMBOL_EXPORT(VirtCheckArea);

/***********************************************************************
 *
 * FUNCTION: VmHandleNoPage
 *
 * DESCRIPTION: Handle a page not present error, map in a page to satisfy
 *				it.
 *
 * PARAMETERS: area - memory area that processor faulted in.
 *			   address - memory address that processor faulted at.
 *
 * RETURNS: -EFAULT if page could not be mapped in, 0 otherwise.
 *
 ***********************************************************************/

int VmHandleNoPage(struct VMArea* area, DWORD address)
{
	DWORD offset=(address-area->start)+area->offset;
	struct VMMapPage* mappedPage;
	struct PhysPage* newPage;
	int ret;

	if (area->areaOps && area->areaOps->handleNoPage)
		if (!area->areaOps->handleNoPage(area, address, offset))
			return 0;

	if (area->protection & PAGE_RW)
	{
		/* Create private page, unless area is shared */

		if (area->flags & MMAP_SHARED)
		{
			KePrint("address = %#X\n", address);
			MachineHalt();
		}else{
			PreemptDisable(); /* Needed? */
			newPage=PageAlloc();

			if (!newPage)
			{
				PreemptFastEnable();
				return -ENOMEM;
			}

			VirtMemMapPage(address, newPage->physAddr, area->protection);

			PreemptFastEnable();

			if (area->vNode->fileOps->mMap)
				ret=area->vNode->fileOps->mMap(area->vNode, address, offset);
			else
				ret=FileGenericReadPage(address, area->vNode, offset);

			if (ret)
				goto fail;
		}
	}else{
		mappedPage=VmLookupPage(area->vNode, offset);

		if (!mappedPage)
		{
			/* So no luck finding it in the VNode's mapping list - read it in */
			newPage=PageAlloc();

			if (!newPage)
				return -ENOMEM;
				
			VirtMemMapPage(address, newPage->physAddr,
				area->protection | PAGE_ACCESSED | PAGE_SHARED);

			/* Add to shared list so other read-only pages can map this page */
			mappedPage=VmCreateMappedPage(offset, newPage);

			if (!mappedPage)
				return -EFAULT;

			VmLockPage(mappedPage);
			
			ListAdd(&mappedPage->list, &area->vNode->sharedList);
			
			if (area->vNode->fileOps->mMap)
				ret=area->vNode->fileOps->mMap(area->vNode, address, offset);
			else
				ret=FileGenericReadPage(address,area->vNode,offset);

			VmUnlockPage(mappedPage);

			if (ret)
				return ret;
		}else{
			/* We don't wait or get the page here, because VmLookupPage already
			 * did that. */
			VirtMemMapPage(address, mappedPage->page->physAddr,
				area->protection | PAGE_ACCESSED | PAGE_SHARED);
		}
	}

	return 0;

fail:
	/* Failed to read in the page, and cannot continue */
	KePrint("VmHandleNoPage failed: killing process\n");
	/* Not reached */
	return -EIO;
}

/***********************************************************************
 *
 * FUNCTION: VmProtFault
 *
 * DESCRIPTION: A protection err (writing to a read-only area for example).
 *				Fatal error for the thread in question, print some debug
 *				information.
 *
 * PARAMETERS: area - memory area that processor faulted in.
 *			   address - memory address that processor faulted at.
 *
 * RETURNS: -EFAULT always - picked up by upper layers and thread/process
 *			killed.
 *
 ***********************************************************************/

int VmProtFault(struct VMArea* area,DWORD address)
{
	KePrint("Protection fault at %#X, area = %#X, area->start = %#X, area->end = %#X (protection = %d)",
		address,area,area->start,area->start+area->length, area->protection);

	return -EFAULT;
}

int VmInit()
{
	/* Create caches */
	areaCache=MemCacheCreate("VmAreas", sizeof(struct VMArea), NULL, NULL, 0);
	mapCache=MemCacheCreate("MappedPages", sizeof(struct VMMapPage), NULL, NULL, 0);

	if (!areaCache || !mapCache)
		return -ENOMEM; /* TODO: Panic. */

	return 0;
}
