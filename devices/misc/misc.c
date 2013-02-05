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

#include <fs/vfs.h>
#include <fs/devfs.h>
#include <typedefs.h>
#include <module.h>
#include <i386/i386.h>
#include <i386/virtual.h>
#include <keobject.h>
#include <devices/class.h>

#define MISC_MAJOR 2

static int ZeroRead(struct File* file,BYTE* data,DWORD size)
{
	ZeroMemory(data,size);
	return size;
}

static int ZeroMemoryMap(struct VNode* vNode, DWORD address, DWORD offset)
{
	ZeroMemory(address, PAGE_SIZE);
	return 0;
}

static int NullWrite(struct File* file,BYTE* data,DWORD size)
{
	return size;
}

static int MemMemoryMap(struct VNode* vNode, DWORD address, DWORD offset)
{
	VirtMemMapPage(address, offset, PAGE_USER | PAGE_RW | PAGE_PRESENT);
	return 0;
}

struct FileOps nullOps={
	.write = NullWrite,
};

struct FileOps zeroOps={
	.read = ZeroRead,
	.mMap = ZeroMemoryMap,
};

struct FileOps memOps={
	.mMap = MemMemoryMap,
};

struct DevClass specialClass;
static struct KeDevice nullDev, zeroDev, memDev;

int MiscInit()
{
	DevClassCreate(&specialClass, NULL, "Special");
	
	KeDeviceCreate(&nullDev, &specialClass.set, DEV_ID_MAKE(MISC_MAJOR, 0), &nullOps,
		DEVICE_CHAR, "Null");
	KeDeviceCreate(&zeroDev, &specialClass.set, DEV_ID_MAKE(MISC_MAJOR, 1), &zeroOps,
		DEVICE_CHAR, "Zero");
	KeDeviceCreate(&memDev, &specialClass.set, DEV_ID_MAKE(MISC_MAJOR, 2), &memOps,
		DEVICE_CHAR, "Memory");

	return 0;
}
