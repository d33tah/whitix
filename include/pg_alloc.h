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

#ifndef PG_ALLOC_H
#define PG_ALLOC_H

#include <llist.h>
#include <typedefs.h>
#include <wait.h>

/* One per physical page in the system */
struct PhysPage
{
	DWORD physAddr;
	int refs;
	struct ListHead list;
};

int PageEarlyInit(DWORD endPfn);
int PageInit();
struct PhysPage* PageGetStruct(DWORD physAddr);
struct PhysPage* PageAlloc();
struct PhysPage* PageAllocLow();
void PageFree(struct PhysPage* page);

static inline void PageFreeAddr(DWORD physAddr)
{
	struct PhysPage* page = PageGetStruct(physAddr);

	if (page)
		PageFree(page);
}

void PageReserveArea(DWORD start,DWORD size);
WaitQueue* PageGetWaitQueue(struct PhysPage* page);

#define PageGet(page) ((page)->refs++)
#define PagePut(page) do { \
 (page)->refs--; if ((page)->refs <= 0) PageFree((page)); } while (0)
#define CopyPage(dest,src) memcpy((dest),(src),PAGE_SIZE)

#endif
