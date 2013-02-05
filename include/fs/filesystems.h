/******************************************************************************
 *
 * Name: filesystems.h - Filesystem management.
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

#ifndef VFS_FILESYSTEMS_H
#define VFS_FILESYSTEMS_H

/* Includes */
#include <llist.h>

/* Structure declarations for filesystems. */
struct VfsSuperBlock;
struct StorageDevice;

/* The StorageDevice parameter can be ignored if, for example, the filesystem is a network one */
typedef struct VfsSuperBlock* (*VfsReadSuperFunc)(struct StorageDevice* dev, int flags, char* data);

struct FileSystem
{
	char* name;
	VfsReadSuperFunc readSuper;
	struct ListHead list;
};

/* For filesystem drivers */
int VfsRegisterFileSystem(struct FileSystem* fs);
int VfsDeregisterFileSystem(struct FileSystem* fs);

/* Helpful macros */
#define VFS_FILESYSTEM(id, fsName, fsReadSuper) \
	struct FileSystem id={ \
		.name = fsName, \
		.readSuper = fsReadSuper \
		};

#endif
