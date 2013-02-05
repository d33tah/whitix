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

#include <bitmap.h>
#include <module.h>
#include <error.h>
#include <i386/i386.h>
#include <print.h>
#include <slab.h>
#include <keobject.h>

/* 
 * This is the beginnings of a decent slab allocator. Inspired by the paper on
 * the Sun caching memory allocator.
 *
 * Each slab type has a bitmap, which has a size of PAGE_SIZE/MINIMUM_OBJECT_SIZE
 * /8. It sounds memory inefficent when you first think about it, but other 
 * methods of pushing addresses into a linked list could use a lot more memory.
 * However, the bitmap method requires more CPU resources. 
 *
 */

LIST_HEAD(cacheList);

/* KeObject types. */
struct KeSet cacheSet;

/* Cache attributes. Displayed on the IC filesystem. */
struct IcAttribute cacheAttributes[] =
{
	IC_INT(struct Cache, size, IC_READ),
	IC_INT(struct Cache, options, IC_READ),
	IC_INT(struct Cache, numObjs, IC_READ),
	IC_INT(struct Cache, offset, IC_READ),
	IC_INT(struct Cache, numPages, IC_READ),
	IC_END(),
};

KE_OBJECT_TYPE(cacheType, NULL, cacheAttributes, OffsetOf(struct Cache, object));

static int slabInfoSetup = 0;

/* General slab limits */
#define SLAB_START 0xC0000000
#define SLAB_END   0xD0000000

/* How many bytes can fit in a cache? */
#define SLAB_TOTAL_BYTES(cache) \
	(((cache)->numPages*PAGE_SIZE) - (cache)->offset)

/* Gets over the bootstrapping problem, and from there a slab cache is allocated */
static struct Cache cacheCache={
	.list=LIST_HEAD_INIT(cacheCache.list),
	.name = "Caches",
	.size=sizeof(struct Cache),
	.numPages = 1,
	.numObjs = (PAGE_SIZE-sizeof(struct Slab))/sizeof(struct Cache),
	.offset = sizeof(struct Slab),
};

struct CacheSize
{
	DWORD size;
	struct Cache* cache;
};

/* MemAlloc just cycles through these */

static struct CacheSize cacheSizes[]={
	{16,NULL},
	{32,NULL},
	{64,NULL},
	{128,NULL},
	{256,NULL},
	{512,NULL},
	{1024,NULL},
	{2048,NULL},
	{4096,NULL},
	{8192,NULL},
	{16384,NULL},
	{32768,NULL},
	{65536,NULL},
	{131072,NULL}, /* 128k */
};

const char* mallocNames[]=
{
	"Malloc16",
	"Malloc32",
	"Malloc64",
	"Malloc128",
	"Malloc256",
	"Malloc512",
	"Malloc1024",
	"Malloc2048",
	"Malloc4096",
	"Malloc8192",
	"Malloc16384",
	"Malloc32768",
	"Malloc65536",
	"Malloc131072",
};

/* Used globally if caller wishes to store bookkeeping information about
 * the slab off the slab itself. */
struct Cache* slabCache;

void* MemAlloc(size_t size)
{
	/* Should be serialized? */
	DWORD flags, i;
	void* obj;

	IrqSaveFlags(flags);

	for (i=0; i<count(cacheSizes); i++)
		if (size <= cacheSizes[i].size)
			/* Should be ok to use */
			break;

	if (i == count(cacheSizes)) /* Too big */
	{
		KePrint("malloc(%u) failed\n",size);
		IrqRestoreFlags(flags);
		return NULL;
	}

	obj=MemCacheAlloc(cacheSizes[i].cache);

	IrqRestoreFlags(flags);
	return obj;
}

SYMBOL_EXPORT(MemAlloc);

/***********************************************************************
 *
 * FUNCTION:	SlabFree
 *
 * DESCRIPTION: Mark a block of memory as free.
 *
 * PARAMETERS:	cache - the cache that contains the memory block.
 *		slab - the slab that contains the memory block.
 *		p - pointer to the memory block.
 *
 * RETURNS:	Nothing.
 *
 ***********************************************************************/

static void SlabFree(struct Cache* cache,struct Slab* slab,void* p)
{
	/* Just clear the bit in the bitmap */
	int i=((DWORD)p-(DWORD)(slab->page+cache->offset))/cache->size;

	/* And zero out the memory concerned */
	BmapSetBit(slab->bitmap, i, false);
	slab->isFull=0; /* At least 1 piece is free now */

	if (cache->ctor)
		cache->ctor(p);
	else
		/* Make sure it's zeroed for later use */
		ZeroMemory(p,cache->size);
}

/* If this is called to free a non-malloc'ed piece of memory, it's your fault (tm) */
void MemFree(void* p)
{
	struct Cache* currCache;
	DWORD flags, i;

	if (!p)
		return;

	IrqSaveFlags(flags);

	for (i=0; i<count(cacheSizes); i++)
	{
		currCache=cacheSizes[i].cache;
		if (!MemCacheFree(currCache,p))
		{
			IrqRestoreFlags(flags);
			return;
		}
	}

/* FIXME! */
	KePrint("free did not free %#X (from %#X)\n",p,__builtin_return_address(0));
	cli(); hlt();
}

SYMBOL_EXPORT(MemFree);

/***********************************************************************
 *
 * FUNCTION: MemCacheAllocSlab
 *
 * DESCRIPTION: Allocate a slab (well, the memory for it).
 *
 * PARAMETERS: cache - the cache to allocate a slab to.
 *
 * RETURNS: 	The uninitialized slab structure.
 *
 ***********************************************************************/

static inline struct Slab* MemCacheAllocSlab(struct Cache* cache)
{
	DWORD virtAddr;
	struct Slab* slab;

	/* Allocate multiple pages. Don't want to get in the way of quick code though */
	if (cache->size > PAGE_SIZE)
		virtAddr=VirtMapPhysRange(SLAB_START,SLAB_END,PAGE_ALIGN_UP(cache->size) >> PAGE_SHIFT,PAGE_KERNEL | PAGE_RW | PAGE_PRESENT);
	else
		virtAddr=VirtMapPhysPage(SLAB_START,SLAB_END,PAGE_KERNEL | PAGE_RW | PAGE_PRESENT);

	/* Failed to allocate the memory for the slab */
	if (!virtAddr)
		return NULL;

	if (cache->options & SLAB_OFFSLAB)
	{
		/* Allocate from a slab cache, which does keep it's info on slab */
		slab=(struct Slab*)MemCacheAlloc(slabCache);
	}else
		slab=(struct Slab*)virtAddr;

	/* Slab setup is performed here. ZeroMemory call also zeros the bitmap. */
	ZeroMemory(slab,sizeof(struct Slab));
	slab->page=virtAddr;

	return slab;
}

static inline void MemCacheConstructSlab(struct Cache* cache,struct Slab* slab)
{
	int i;
	void* obj;

	/* If the slab has a constructor, call it */
	if (cache->ctor)
	{
		for (i=0; i<cache->numObjs; i++)
		{
			obj=(void*)(slab->page+(i*cache->size)+cache->offset);
			cache->ctor(obj);
		}
	}else
		/* Zero out the slab */
		ZeroMemory(slab->page+cache->offset,(cache->numPages*PAGE_SIZE)-cache->offset);

	if (!cache->slabs)
	{
		/* This is now the first slab in the list */
		INIT_LIST_HEAD(&slab->list);
		cache->slabs=slab;
	}else
		/* Adds to the head of the list = less time traversing for a free slab */
		ListAdd(&slab->list, &cache->slabs->list);
}

/***********************************************************************
 *
 * FUNCTION: MemCacheAddSlab
 *
 * DESCRIPTION: Add a slab of memory to a cache.
 *
 * PARAMETERS: cache - the cache to add a slab to. 
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

/* Note: In future, calculate the amount of memory needed for the bitmap */

static int MemCacheAddSlab(struct Cache* cache)
{
	struct Slab* slab;

	slab=MemCacheAllocSlab(cache);

	if (!slab)
		return -ENOMEM;

	MemCacheConstructSlab(cache,slab);
	return 0;
}

/*******************************************************************************
 *
 * FUNCTION:	MemCacheAlloc
 *
 * DESCRIPTION: Allocate a block of memory from a cache.
 *
 * PARAMETERS:	cache - the cache to allocate memory from. 
 *
 * RETURNS:		Pointer to the block of memory allocated if it is a 
 *				success, NULL otherwise.
 *
 ******************************************************************************/
 
void* MemCacheAlloc(struct Cache* cache)
{
	DWORD flags;
	struct Slab* currSlab;
	int i;

	if (!cache)
		return NULL;

	IrqSaveFlags(flags);

	/* Go through all the slabs */
	if (!cache->slabs)
		/* If MemCacheAddSlab fails, it'll fail straight through to the
		the bottom of this function, as currSlab will equal NULL. */
		MemCacheAddSlab(cache);

	currSlab=cache->slabs;

	/* Now iterate through each slab, seeing if any are full, and if not, allocate from them */

	while (currSlab)
	{
		if (!currSlab->isFull)
		{
			/* Search the bitmap */
			for (i=0; i<cache->numObjs; i++)
			{
				if (!BmapGetBit(currSlab->bitmap,i))
				{
					BmapSetBit(currSlab->bitmap,i,true);
					if (i == cache->numObjs-1)
						currSlab->isFull=1;
					
					if (!cache->ctor)	
						ZeroMemory(currSlab->page+cache->offset+(i*cache->size), cache->size);

					IrqRestoreFlags(flags);
					return (void*)(currSlab->page+cache->offset+(i*cache->size));
				}
			}
		}
		
		/* Only one in the list */
		if (currSlab->list.prev == &cache->slabs->list)
			MemCacheAddSlab(cache);

		currSlab=ListEntry(currSlab->list.prev,struct Slab,list);
	}

	KePrint("MemCacheAlloc(%#X) - failed to allocate memory\n",cache);
	IrqRestoreFlags(flags);
	/* Shouldn't get here, as all memory requests should be satifisted by adding slabs */
	return NULL;
}

SYMBOL_EXPORT(MemCacheAlloc);

int MemCacheFree(struct Cache* cache, void* obj)
{
	DWORD flags;
	DWORD addr;
	struct Slab* currSlab;
	int ret=-EFAULT;

	if (!obj)
		return -EFAULT;

	IrqSaveFlags(flags);

	addr=(DWORD)obj;
	currSlab=cache->slabs;

	while (currSlab)
	{
		if (addr >= (currSlab->page+cache->offset) && addr < currSlab->page+(cache->numPages*PAGE_SIZE))
		{
			/* In this slab then */
			SlabFree(cache,currSlab,obj);
			ret=0;
			goto out;
		}
		
		/* End of list really */
		if (currSlab->list.prev == &cache->slabs->list)
			goto out;

		currSlab=ListEntry(currSlab->list.prev,struct Slab,list);
	}

out:
//	printf("Done, %d\n",ret);
	IrqRestoreFlags(flags);
	return ret;
}

SYMBOL_EXPORT(MemCacheFree);

struct Cache* MemCacheCreate(const char* name, size_t size, Constructor ctor,
	Destructor dtor,int options)
{
	struct Cache* retVal;

	if (!size)
		return NULL;

	if (size < SLAB_MIN_OBJECT_SIZE)
		size=SLAB_MIN_OBJECT_SIZE;

	/* Allocate descriptor from cacheCache */
	retVal=(struct Cache*)MemCacheAlloc(&cacheCache);

	/* Round up the size to the nearest 4-byte boundary. */
	size = (size + 3) & ~0x3;

	retVal->size=size;

	if (size > PAGE_SIZE)
		retVal->numPages=size >> PAGE_SHIFT;
	else
		retVal->numPages=1;

	retVal->ctor=ctor;
	retVal->dtor=dtor;
	retVal->name = name;

	if (size >= PAGE_SIZE/4)
		options |= SLAB_OFFSLAB;

	retVal->offset=(options & SLAB_OFFSLAB) ? 0 : (sizeof(struct Slab));
	retVal->numObjs = SLAB_TOTAL_BYTES(retVal) / retVal->size;
	retVal->options = options;
	
	if (slabInfoSetup)
		/* Create the IcFs entry. */
		KeObjectCreate(&retVal->object, &cacheSet, name);

	ListAddTail(&retVal->list, &cacheList);

	return retVal;
}

SYMBOL_EXPORT(MemCacheCreate);

int SlabInfoInit()
{
	KeSetCreate(&cacheSet, NULL, &cacheType, "Memory");
	
	/* Register all the caches created so far. */
	struct Cache* cache;
	
	ListForEachEntry(cache, &cacheList, list)
	{
		KeObjectCreate(&cache->object, &cacheSet, cache->name);
	}
	
	slabInfoSetup = 1;
	
	return 0;
}

int SlabInit()
{
	DWORD i;

	/* Set up the cache cache */
	ListAddTail(&cacheCache.list, &cacheList);

	/* Now start allocating caches */

	/* Set up a slab cache if the caller wishes to store slab information off slab */
	slabCache=MemCacheCreate("Slabs", sizeof(struct Slab), NULL, NULL, 0);

	for (i=0; i<count(cacheSizes); i++)
	{
		cacheSizes[i].cache=MemCacheCreate(mallocNames[i], cacheSizes[i].size, NULL, NULL, 0);
		
		if (!cacheSizes[i].cache)
		{
			KePrint("Malloc slab allocation failed!\n");
			return -ENOMEM;
		}
	}

	return 0;
}
