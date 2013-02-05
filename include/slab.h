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

#ifndef SLAB_H
#define SLAB_H

#include <i386/virtual.h>
#include <typedefs.h>
#include <keobject.h>
#include <llist.h>
#include <types.h>

#define SLAB_SIZE PAGE_SIZE
#define SLAB_MIN_OBJECT_SIZE 16
/* Max objects is needed when allocating a bitmap of free objects in a slab */
#define MAX_OBJECTS_PER_SLAB (SLAB_SIZE/SLAB_MIN_OBJECT_SIZE)

/* Slab options */
#define SLAB_OFFSLAB 1 /* Store the slab info off-slab */

#define MALLOC_MAX_SIZE		131072

struct Slab
{
	DWORD page; /* Virtual address */
	struct ListHead list;
	BYTE isFull;
	BYTE bitmap[MAX_OBJECTS_PER_SLAB/8];
};

typedef void (*Constructor)(void* obj);
typedef void (*Destructor)(void* obj);

struct Cache
{
	struct ListHead list;
	Constructor ctor;
	Destructor dtor;
	struct KeObject object;
	const char* name;
	int options, numObjs, offset;
	size_t size;
	struct Slab* slabs; /* Each cache owns a number of slabs */
	int numPages; /* per slab */
};

struct Cache* MemCacheCreate(const char* name,size_t size,Constructor ctor,Destructor dtor,int options);
void* MemCacheAlloc(struct Cache* cache);
int MemCacheFree(struct Cache* cache,void* obj);
int MemCacheDestroy(struct Cache* cache);

#endif
