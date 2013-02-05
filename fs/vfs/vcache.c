/***************************************************************************
 *																		   *
 *   Whitix is free software; you can redistribute it and/or modify it     *
 *   under the terms of the GNU General Public License version 2 as        *
 *   published by the Free Software Foundation.                      	   *
 *																		   *
 ***************************************************************************/

/* 
 * vcache.c - functions for allocating, managing and freeing VFS nodes.
 *
 * Contains a library of functions for managing VFS nodes (shortened to vNodes).
 * It also handles the SysFilesystemSync system call, which synchronises all
 * (dirty) vNodes and blocks to disk.
 *
 * This module also manages the vNode cache and the per-superblock vNode list.
 * Note that it doesn't have anything to do with resolving names to vNodes
 * or superblocks. It merely manages the various lists and reference counts.
 */
 
/* General TODOs:
 *
 * o Use a more efficient structure than a linked list of vNodes for each
 *	 superblock?
 *
 * o Make more use of VNodeGetWithData? Perhaps it should be the default VNodeGet?
 */

#include <fs/vfs.h>
#include <malloc.h>
#include <module.h>
#include <slab.h>
#include <print.h>
#include <task.h>
#include <devices/sdevice.h>

struct Cache* vNodeCache;

/* Used when syncing all in-memory vNodes */
extern struct ListHead sbList;

static int VNodeReadWithData(struct VNode* vNode, void* data);

/***********************************************************************
 *
 * FUNCTION: VNodeFind
 *
 * DESCRIPTION: See if the vNode is already present in memory.
 *
 * PARAMETERS: superBlock - superblock which the filesystem node is 
 *							stored on.
 *			   id - the vNode's id - filesystem specific.
 *
 * RETURNS: vNode already stored in memory.
 *
 ***********************************************************************/

struct VNode* VNodeFind(struct VfsSuperBlock* superBlock, DWORD id)
{
	struct VNode* curr;

	SpinLock(&superBlock->nodeListLock);

	ListForEachEntry(curr,&superBlock->vNodeList,next)
		if (curr->id == id)
		{
			/* Found it */
			++curr->refs;
			SpinUnlock(&superBlock->nodeListLock);
			VNodeWaitOn(curr);
			return curr;
		}

	SpinUnlock(&superBlock->nodeListLock);

	/* Not found */
	return NULL;
}

/***********************************************************************
 *
 * FUNCTION: VNodeAlloc
 *
 * DESCRIPTION: Allocate a vNode and fill in data.
 *
 * PARAMETERS: superBlock - superblock which the filesystem node is 
 *							stored on.
 *			   id - the vNode's id - filesystem specific.
 *
 * RETURNS: the filesystem node allocated.
 *
 ***********************************************************************/

struct VNode* VNodeAlloc(struct VfsSuperBlock* superBlock, DWORD id)
{
	struct VNode* newNode;

	newNode=(struct VNode*)MemCacheAlloc(vNodeCache);
	
	if (newNode)
	{
		newNode->superBlock=superBlock;
		newNode->id=id;

		/* Unlocked at the end of VNodeRead. */
		VNodeLock(newNode);

		ListAddTail(&newNode->next,&superBlock->vNodeList);
	}

	return newNode;
}

SYMBOL_EXPORT(VNodeAlloc);

/***********************************************************************
 *
 * FUNCTION: VNodeGet
 *
 * DESCRIPTION: Get a reference to a VNode.
 *
 * PARAMETERS: superBlock - superblock which the filesystem node is 
 *							stored on.
 *			   id - the vNode's id - filesystem specific.
 *
 * RETURNS: vNode corresponding to node on disk.
 *
 ***********************************************************************/

struct VNode* VNodeGet(struct VfsSuperBlock* superBlock, DWORD id)
{
	struct VNode* curr;

	if (!superBlock)
		return NULL;

	/* Find if there is already a vnode with the given id and superblock by
	iterating through the vNode list */
	if ((curr=VNodeFind(superBlock,id)))
		return curr;

	/* If not, allocate it. VNodeAlloc calls the superblock's ReadVNode function,
	 * which fills in the vNode's information. */
	curr=VNodeAlloc(superBlock,id);
	
	if (VNodeRead(curr))
	{
		MemCacheFree(vNodeCache, curr);
		return NULL;
	}
	
	return curr;
}

SYMBOL_EXPORT(VNodeGet);

struct VNode* VNodeGetWithData(struct VfsSuperBlock* superBlock, vid_t id, void* data)
{
	struct VNode* curr;

	if (!superBlock)
		return NULL;

	/* Find if there is already a vnode with the given id and superblock by
	iterating through the vNode list */
	if ((curr=VNodeFind(superBlock, id)))
		return curr;

	/* If not, allocate it. VNodeAlloc calls the superblock's ReadVNode function,
	 * which fills in the vNode's information. */
	curr=VNodeAlloc(superBlock, id);
	
	if (VNodeReadWithData(curr, data))
	{
		MemCacheFree(vNodeCache, curr);
		return NULL;
	}
	
	return curr;
}

SYMBOL_EXPORT(VNodeGetWithData);

/***********************************************************************
 *
 * FUNCTION: VNodeGetEmpty
 *
 * DESCRIPTION: Create an empty vNode.
 *
 * PARAMETERS: None
 *
 * RETURNS: A new vNode, created using MemCacheAlloc.
 *
 ***********************************************************************/

struct VNode* VNodeGetEmpty()
{
	return MemCacheAlloc(vNodeCache);
}

SYMBOL_EXPORT(VNodeGetEmpty);

/***********************************************************************
 *
 * FUNCTION: VNodeRead
 *
 * DESCRIPTION: Read data into a vNode. Calls the filesystem-specific
 *				function.
 *
 * PARAMETERS: vNode - vNode to read into.
 *
 * RETURNS: Result of readVNode.
 *
 ***********************************************************************/

int VNodeRead(struct VNode* vNode)
{
	int ret=-ENOTIMPL;

	if (!vNode)
		return -EFAULT;

	if (vNode->superBlock && vNode->superBlock->sbOps && vNode->superBlock->sbOps->readVNode)
		ret=vNode->superBlock->sbOps->readVNode(vNode);

	VNodeUnlock(vNode);

	return ret;
}

static int VNodeReadWithData(struct VNode* vNode, void* data)
{
	int ret=-ENOTIMPL;
	
	if (!vNode)
		return -EFAULT;
	
	if (vNode->superBlock && vNode->superBlock->sbOps && vNode->superBlock->sbOps->readVNodeWithData)
		ret=vNode->superBlock->sbOps->readVNodeWithData(vNode, data);

	VNodeUnlock(vNode);
	
	return ret;
}

/***********************************************************************
 *
 * FUNCTION: VNodeWrite
 *
 * DESCRIPTION: Write data from a VNode onto disk. Calls the filesystem-
 *				specific function.
 *
 * PARAMETERS: vNode - vNode to write from.
 *
 * RETURNS: Result of writeVNode.
 *
 ***********************************************************************/

void VNodeWrite(struct VNode* vNode)
{
	if (!vNode)
		return;

	/* No need to write to disk if the vNode is not dirty. */
	if (!BitTest(&vNode->flags, VNODE_DIRTY))
		return;

	VNodeLock(vNode);

	if (vNode->superBlock && vNode->superBlock->sbOps && vNode->superBlock->sbOps->writeVNode)
			vNode->superBlock->sbOps->writeVNode(vNode);

	BitClear(&vNode->flags, VNODE_DIRTY);
	VNodeUnlock(vNode);
}

/***********************************************************************
 *
 * FUNCTION: VNodeSyncAll
 *
 * DESCRIPTION: Synchronize all vNodes in the filesystem.
 *
 * PARAMETERS: None.
 *
 * RETURNS: None.
 *
 ***********************************************************************/

void VNodeSyncAll()
{
	struct VNode* curr, *curr2;
	struct VfsSuperBlock* superBlock;

	ListForEachEntry(superBlock,&sbList,sbList)
		/* Cycle through all vNodes. */
		ListForEachEntrySafe(curr, curr2, &superBlock->vNodeList, next)
			if (BitTest(&curr->flags, VNODE_DIRTY))
				VNodeWrite(curr);
}

/***********************************************************************
 *
 * FUNCTION: VNodeRelease
 *
 * DESCRIPTION: Release a reference to a vNode, and free if there are no
 *				references left.
 *
 * PARAMETERS: vNode - vNode to release reference to.
 *
 * RETURNS: None.
 *
 ***********************************************************************/

void VNodeRelease(struct VNode* vNode)
{
	struct VfsSuperBlock* superBlock;
	
	if (!vNode)
		return;
		
	superBlock = vNode->superBlock;

	VNodeWaitOn(vNode);

	if (!vNode->refs)
	{
		KePrint("VNodeRelease: Trying to free free vNode (%#X on %s)\n",
			vNode->id, StorageDeviceGetName(superBlock->sDevice));
		return;
	}

repeat:
	if (vNode->refs > 1)
	{
		vNode->refs--;
		return;
	}

	if (BitTest(&vNode->flags, VNODE_DIRTY))
	{
		VNodeWrite(vNode);
		VNodeWaitOn(vNode);
		goto repeat;
	}

	vNode->refs--;

	/* May still have references as VNodeWrite may read in a buffer
	 * and sleep. Writing a buffer won't however. */
	if (vNode->refs)
		return;

	if (vNode->superBlock)
		SpinLock(&vNode->superBlock->nodeListLock);

	if (vNode->superBlock && vNode->superBlock->sbOps &&
		 vNode->superBlock->sbOps->freeVNode)
		vNode->superBlock->sbOps->freeVNode(vNode);
	else if (vNode->extraInfo)
		MemFree(vNode->extraInfo);

	ListRemove(&vNode->next);

	if (vNode->superBlock)
		SpinUnlock(&vNode->superBlock->nodeListLock);

	MemCacheFree(vNodeCache,vNode);
}

SYMBOL_EXPORT(VNodeRelease);

/* Should be a spinlock for SMP */

void VNodeWaitOn(struct VNode* vNode)
{
	vNode->refs++;
	WAIT_ON(&vNode->waitQueue, !BitTest(&vNode->flags, VNODE_LOCKED));
	vNode->refs--;
}

void VNodeLock(struct VNode* vNode)
{
	while (BitTestAndSet(&vNode->flags, VNODE_LOCKED))
		VNodeWaitOn(vNode);
}

SYMBOL_EXPORT(VNodeLock);

void VNodeUnlock(struct VNode* vNode)
{
	BitClear(&vNode->flags, VNODE_LOCKED);
	WakeUp(&vNode->waitQueue);
}

SYMBOL_EXPORT(VNodeUnlock);

void VNodeCtor(void* obj)
{
	struct VNode* vNode=(struct VNode*)obj;

	ZeroMemory(vNode, sizeof(struct VNode));
	vNode->refs=1;
	vNode->extraInfo=NULL;
	vNode->id=~0; /* To catch unsuitable accesses perhaps */
	INIT_WAITQUEUE_HEAD(&vNode->waitQueue);

	INIT_LIST_HEAD(&vNode->sharedList);
	INIT_LIST_HEAD(&vNode->next);
}

int SysFileSystemSync()
{
	VNodeSyncAll();
	BlockSyncAll();

	return 0;
}

int VNodeInitCache()
{
	vNodeCache=MemCacheCreate("VNodes",sizeof(struct VNode),VNodeCtor,NULL,0);
	return 0;
}
