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

/* FIXME: Rework VmAreaOps struct and design. */

#ifndef VMM_H
#define VMM_H

#include <typedefs.h>
#include <task.h>
#include <wait.h>
#include <locks.h>

struct File;
struct VmAreaOps;

struct VMArea
{
	DWORD start,length,offset;
	int flags;
	int protection;
	struct VNode* vNode;
	struct VmAreaOps* areaOps;
	struct ListHead list;
	struct Process* process;
};

struct VMMapPage
{
	DWORD offset;
	int flags;
	struct ListHead list;
	struct PhysPage* page;
};

/* Will be useful in paging. */
struct VmAreaOps
{
	int (*handleNoPage)(struct VMArea* area, DWORD address, DWORD offset);
	int (*addPage)(struct VMArea* area, DWORD address);
};

DWORD MMapDo(struct Process* process,struct VNode* vNode,DWORD address,DWORD length,int protection,DWORD offset,int flags, struct VmAreaOps* ops);
int MmapHandleFault(struct Process* process,DWORD address,int error);
void MmapProcessRemove(struct Process* process);
struct VMArea* VmLookupAddress(struct Process* process,DWORD address);
DWORD MMapFindAddress(struct Process* process,DWORD length);
int MMapInit();

/* Area management. */
struct VMMapPage* VmLookupPage(struct VNode* vNode, DWORD offset);
struct VMMapPage* VmCreateMappedPage(DWORD offset, struct PhysPage* page);
void VmLockPage(struct VMMapPage* page);
void VmUnlockPage(struct VMMapPage* page);
void VmWaitOnPage(struct VMMapPage* page);
void VmWaitForPage(struct VMMapPage* page);
int VmFreeMappedPage(struct VNode* vNode, DWORD physAddr);

/* General page fault errors */

#define VM_PROTECTION 0x00000001
#define VM_WRITE	  0x00000002
#define VM_USER		  0x00000004

/* MMapDo flags */
#define MMAP_PRIVATE	0x00000000
#define MMAP_SHARED		0x00000001
#define MMAP_FIXED		0x00000002
#define MMAP_GROWDOWN	0x00000004
#define MMAP_ANON		0x00000008

#define PAGE_LOCKED 0x0

#define PageLocked(page) (BitTest(&page->flags, PAGE_LOCKED))

/* General mmap defines */
#define MMAP_BASE	(0x800000)
#define MMAP_END	(0xC0000000)

#endif
