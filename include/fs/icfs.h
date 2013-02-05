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

#ifndef ICFS_H
#define ICFS_H

#include <llist.h>
#include <typedefs.h>

#define ICFS_TYPE_INT	0
#define ICFS_TYPE_STR	1
#define ICFS_TYPE_BYTE	2
#define ICFS_TYPE_FUNC	3

#define IC_READ		0
#define IC_RW		1

/* Type definitions */

struct IcAttribute
{
	int type, offset, perms;
	const char* name;
};

typedef int (*IcReadFunc)(BYTE* data, unsigned long size, DWORD position);
typedef int (*IcWriteFunc)(BYTE* data, unsigned long size, DWORD position);

#define IC_INT(structure, member, permissions) \
	{ \
	  .type = ICFS_TYPE_INT, \
	  .offset = OffsetOf(structure, member), \
	  .name = #member, \
	  .perms = permissions \
	}

#define IC_STRING(structure, member, permissions) \
	{ \
	  .type = ICFS_TYPE_STR, \
	  .offset = OffsetOf(structure, member), \
	  .name = #member, \
	  .perms = permissions \
	}
	
#define IC_STRING_NAME(objName, structure, member, permissions) \
	{ \
		.type = ICFS_TYPE_STR, \
		.offset = OffsetOf(structure, member), \
		.name = objName, \
		.perms = permissions, \
	}
	
#define IC_END() \
	{ -1, -1, -1, NULL }

struct IcFsEntry
{
	int type;
	union
	{
		int* iVal;
		char* sVal;
		
		struct
		{
			char* csVal;
			unsigned int minLen, maxLen, size;
		};
		
		struct
		{
			IcReadFunc readFunc;
			IcWriteFunc writeFunc;
		};
	};
};

struct KeFsEntry* IcFsCreateDir(struct KeFsEntry* dir, char* name);

/* Adding an entry to a IcFs directory. */
int IcFsAddIntEntry(struct KeFsEntry* dir, char* name, int* value, DWORD permissions);

struct IcFsEntry* IcFsAddStrEntry(struct KeFsEntry* dir, char* name, char* str, DWORD permissions);

int IcFsAddArrayEntry(struct KeFsEntry* dir, char* name, char* array, unsigned int minLen, unsigned int maxLen, unsigned int size);

int IcFsAddSoftLink(struct KeFsEntry* dir, char* name,
	int (*followLink)(struct KeFsEntry**, struct KeFsEntry*));
	
/* 
 * Permissions are calculated from the values of readFunc and writeFunc. That is,
 * a entry with no write function is read-only.
 */
int IcFsAddFuncEntry(struct KeFsEntry* dir, char* name, IcReadFunc readFunc, IcWriteFunc writeFunc);

int IcFsRemoveDir(struct KeFsEntry* dir);

#endif
