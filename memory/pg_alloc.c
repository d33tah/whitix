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

/* 
 * Should be updated soon to be more efficent. Perhaps a mixture of stack and
 * bitmaps? The Linux idea of zones for ISA DMA and normal memory also sounds
 * like a good idea. i386 specific at the moment.
 */

#include <bitmap.h>
#include <pg_alloc.h>
#include <i386/i386.h>
#include <i386/virtual.h>
#include <vmm.h>
#include <string.h>
#include <console.h>
#include <sections.h>
#include <module.h>
#include <panic.h>
#include <print.h>
#include <addresses.h>

static BYTE* allocatedBitmap;
DWORD maxPfn;
WaitQueue* waitQueues;

/* Each consists of all pages, although some allocated ones are not added to the list in the first place */
struct PageStack
{
	DWORD numPages; /* that are available */
	DWORD maxPages;
	struct ListHead list; /* Use singly-linked list! */
	WaitQueue* waitQueues;
};

static struct PageStack normal;
static struct PageStack isa;

/* TODO: Better address? */
#define BITMAP_START_ADDRESS 0xC0180000
#define PSTACK_START_ADDRESS 0xC0200000

struct PhysPage* pageArrayPtr=(struct PhysPage*)(PSTACK_START_ADDRESS);

/* Called by PhysInit so that it can reserve pages */
int PageEarlyInit(DWORD endPfn)
{
	maxPfn = endPfn;

	allocatedBitmap=(BYTE*)BITMAP_START_ADDRESS;
		
	/* Set up bitmap for first 20mb of memory. */
	ZeroMemory(allocatedBitmap, (2 << 20)/8);

	/* Reserve the first page of memory. Important structures may be stored there. */
	PageReserveArea(0, PAGE_SIZE);

	/* Reserve pages like the kernel's code and data. */
	PageReserveArea(VA_TO_PA(code), (DWORD)end-(DWORD)code);

	/* Reserve the page stack - each page has it's own structure */
	PageReserveArea(VA_TO_PA(PSTACK_START_ADDRESS), maxPfn*sizeof(struct PhysPage));
	
	/* And the waitqueues */
	PageReserveArea(VA_TO_PA((DWORD)PAGE_ALIGN_UP(
		(PSTACK_START_ADDRESS+(maxPfn*sizeof(struct PhysPage))))),
			((maxPfn+1023) >> 10)*sizeof(WaitQueue));

	return 0;
}

int PageInit()
{
	/* 
	 * Create 2 stacks of pages. The disadvantage is that it uses quite a lot of memory,
	 * maximum 16mb for a system with 4gb. But it's the speediest method possible.
	 */

	DWORD i;
	struct PhysPage* pageArray=pageArrayPtr;

	isa.maxPages=(maxPfn > 4096 ? 4096 : maxPfn); /* If there's more than 16mb, cut it down to 16mb for the ISA page stack */
	isa.numPages=0;
	INIT_LIST_HEAD(&isa.list);
	
	for (i=0; i<isa.maxPages; i++)
	{
		/* Free, can push onto stack */
		pageArray[i].physAddr=i << 12;

		if (!BmapGetBit(allocatedBitmap, i))
		{
			ListAdd(&pageArray[i].list, &isa.list);
			++isa.numPages;
		}
	}

	normal.numPages=0;
	INIT_LIST_HEAD(&normal.list);

	if (isa.maxPages < maxPfn) /* i.e. less than 16mb of memory */
	{
		normal.maxPages=maxPfn-4096;
		/* Go through normal memory and push onto stack */
		for (i=4096; i<maxPfn; i++)
		{
			pageArray[i].physAddr=i << 12;

			if (i >= 5120 || (i < 5120 && !BmapGetBit(allocatedBitmap, i)))
			{
				ListAdd(&pageArray[i].list, &normal.list);
				++normal.numPages;
			}
		}
	}

	KePrint(KERN_INFO "PAGE: ISA pages (%u/%u), normal pages (%u/%u)\n",
		isa.numPages, isa.maxPages, normal.numPages, normal.maxPages);
		
	allocatedBitmap=NULL; /* Can't reserve anymore */

	/* Allocate the waitqueues */
	waitQueues=(WaitQueue*)PAGE_ALIGN_UP((PSTACK_START_ADDRESS+(maxPfn*sizeof(struct PhysPage))));

	for (i=0; i<((maxPfn+1023) >> 10); i++)
		INIT_WAITQUEUE_HEAD(&waitQueues[i]);

	return 0;
}

struct PhysPage* PageListGetHead(struct PageStack* stack)
{
	struct PhysPage* page;
	DWORD flags;
	
	IrqSaveFlags(flags);

	page=ListEntry(stack->list.next,struct PhysPage,list);	
		
	--stack->numPages;
	ListRemove(stack->list.next);
	
	IrqRestoreFlags(flags);

	return page;
}

struct PhysPage* PageGetStruct(DWORD address)
{
	return &pageArrayPtr[address >> 12];
}

SYMBOL_EXPORT(PageGetStruct);

struct PhysPage* PageAlloc()
{
	struct PhysPage* page;
	
	/* Allocate from the normal page stack first, and then the ISA stack */
	if (normal.numPages)
		page=PageListGetHead(&normal);
	else
		page=PageAllocLow();

	return page;
}

SYMBOL_EXPORT(PageAlloc);

struct PhysPage* PageAllocLow()
{
	/* Just allocate from the ISA stack */
	if (isa.numPages < 5)
		KernelPanic("System out of memory");

	return PageListGetHead(&isa);
}

SYMBOL_EXPORT(PageAllocLow);

void PageListPutPage(struct PageStack* stack,struct PhysPage* page)
{
	DWORD flags;
	IrqSaveFlags(flags);

	++stack->numPages;

	ListAdd(&page->list, &stack->list);

	IrqRestoreFlags(flags);
}

void PageFree(struct PhysPage* page)
{
	DWORD pagePfn=page->physAddr >> 12;

	if (pagePfn > 4096) /* Normal */
	{
		if (pagePfn >= (normal.maxPages+4096))
		{
			KePrint("PageFree : illegal free\n");
			return;
		}

		PageListPutPage(&normal, page);
	}else{ /* ISA */
		if (pagePfn >= isa.maxPages)
		{
			KePrint("PageFree : isa illegal free\n");
			return;
		}

		PageListPutPage(&isa, page);
	}
}

SYMBOL_EXPORT(PageFree);

/* Used by bootup code to reserve an area of memory so it isn't added to the page stack */

void PageReserveArea(DWORD start,DWORD size)
{
	start=PAGE_ALIGN(start);
	size=PAGE_ALIGN_UP(size);

	if (!allocatedBitmap) /* Err, someone's reserving memory after the bitmap's gone */
		KernelPanic("PageReserveArea: can't reserve memory!");

	/* Set all the pages as allocated */
	while (size)
	{
		BmapSetBit(allocatedBitmap, (start >> 12), true);
		start+=PAGE_SIZE;
		size-=PAGE_SIZE;
	}
}


WaitQueue* PageGetWaitQueue(struct PhysPage* page)
{
	return &waitQueues[page->physAddr >> 22];
}

SYMBOL_EXPORT(PageGetWaitQueue);
