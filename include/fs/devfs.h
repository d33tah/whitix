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

#ifndef DEVFS_H
#define DEVFS_H

#include <fs/kfs.h>
#include <fs/vfs.h>
#include <llist.h>
#include <typedefs.h>

/* Devices in Whitix are either block or character devices */
#define DEVICE_BLOCK 0x80000000
#define DEVICE_CHAR	 0x40000000

struct StorageDevice;

int DevFsAddDir(struct KeSet* set, char* name);
int DevFsAddDevice(struct KeDevice* device, const char* name);

static inline struct KeDevice* DevFsGetDevice(struct VNode* vNode)
{
	struct KeFsEntry* entry=(struct KeFsEntry*)(vNode->id);

	/* DevFs directories don't have specific operations. */	
	if (entry->type & VFS_ATTR_DIR)
		return NULL;
		
	return entry->file;
}

static inline void* DeviceGetOps(struct VNode* devNode)
{
	struct KeDevice* device;
	
	device = DevFsGetDevice(devNode);
	
	if (device == NULL)
		return NULL;
	
	return device->devPriv;
}

#endif
