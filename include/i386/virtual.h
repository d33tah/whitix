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

#ifndef VIRTUAL_H
#define VIRTUAL_H

#include <console.h>
#include <error.h>
#include <i386/physical.h>
#include <pg_alloc.h>
#include <typedefs.h>

/* i386 page permissions */

#define PAGE_NOTPRESENT 0x000
#define PAGE_PRESENT	0x001
#define PAGE_READONLY	0x000
#define PAGE_RW			0x002
#define PAGE_KERNEL		0x000
#define PAGE_USER		0x004
#define PAGE_ACCESSED	0x020
#define PAGE_DIRTY		0x040
#define PAGE_GLOBAL		0x080
#define PAGE_SHARED		0x100

#define PGDIR_SELF 1023 /* Entry in the page directory of the page directory itself */
#define PGDIR_ENT(addr) (addr >> 22)
#define PGTABLE_ENT(addr) (addr >> 12)
#define PAGE_TABLES_VA (PGDIR_SELF << 22)
#define PAGE_DIR_VA (PAGE_TABLES_VA | (PGDIR_SELF << 12))

extern DWORD* pageDir, *pageTable;

#define PAGE_IS_PRESENT(page) \
 (pageDir[PGDIR_ENT(page)] && (pageTable[PGTABLE_ENT(page)] & PAGE_PRESENT))

struct MemManager
{
	DWORD* pageDir;
	DWORD codeStart,codeEnd,dataStart,dataEnd,bssStart,bssEnd,start,end,
		argsStart,argsEnd;
	struct ListHead list;
};

extern struct MemManager kernelMem; /* Rarely needed */

/* Move to non-arch-specific header */
int VirtInit();
int VirtMemManagerInit(struct MemManager* manager,int isKernel);
int VirtMemManagerSetup(struct Process* process);

int VirtMemMapPage(DWORD virt,DWORD phys,int perms);
int VirtAltMemMap(struct MemManager* memManager,DWORD virt,DWORD physical,int perms);
DWORD VirtMapPhysPage(DWORD virt,DWORD endVirt,int perms);
void VirtUnmapPhysPage(DWORD virt);
DWORD VirtMapPhysRange(DWORD virt,DWORD endVirt,DWORD numPages,int perms);
DWORD VirtAllocateTemp(DWORD phys);
int VirtSetCurrent(struct MemManager* manager);
DWORD VirtToPhys(DWORD virt);
int VirtDestroyMemManager(struct MemManager* manager);
void VirtUnmapPages(DWORD start,DWORD len);
void VirtShowFault(DWORD address,int error);

#endif
