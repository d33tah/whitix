/******************************************************************************
 *
 * Name: super.h - defines, structures and function prototypes for superblocks.
 *
 ******************************************************************************/

/* 
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

#ifndef VFS_SUPER_H
#define VFS_SUPER_H

#include <spinlock.h>
#include <typedefs.h>
#include <devices/sdevice.h>

struct VNode;

struct VfsSuperBlock
{
	struct SuperBlockOps* sbOps; /* A field so it is easier to set up a VfsSuperBlock */
	void* privData;
	DWORD coveredId; /* VNode id of the vNode covered by the mount. */
	struct VfsSuperBlock* parent;
	struct StorageDevice* sDevice; /* Can be null */
	struct VNode* mount;
	int flags;
	struct ListHead vNodeList,sbList;
	Spinlock nodeListLock;
};

struct SuperBlockOps
{
	/* General vNode functions */
	int (*allocVNode)(struct VNode* vNode);
	int (*freeVNode)(struct VNode* vNode);
	int (*readVNode)(struct VNode* vNode);
	int (*readVNodeWithData)(struct VNode* vNode, void* data);
	int (*writeVNode)(struct VNode* vNode);
	int (*dirtyVNode)(struct VNode* vNode);

	/* General superblock functions */
	int (*writeSuper)(struct VfsSuperBlock* superBlock);
	int (*freeSuper)(struct VfsSuperBlock* superBlock);
};

/* Prototypes */
int VfsMount(char* mountPoint,char* deviceName,char* fsName,void* mData);
int SysMount(char* mountPoint,char* deviceName,char* fsName,void* data);
int SysUnmount(char* mountPoint);
struct VfsSuperBlock* VfsAllocSuper(struct StorageDevice* sDev,int flags);
void VfsFreeSuper(struct VfsSuperBlock* superBlock);

/* Defines */
#define BYTES_PER_SECTOR(s)		((s)->sDevice->softBlockSize)

#define SuperSetDirty(sb) 		((sb)->flags |= SB_DIRTY)

/* Superblock flags. */
#define SB_DIRTY	0x1
#define SB_RDONLY	0x2

#endif
