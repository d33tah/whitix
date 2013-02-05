/******************************************************************************
 *
 * Name: device.h - device object management.
 *
 ******************************************************************************/

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

#ifndef KEDEVICE_H
#define KEDEVICE_H

#include <keobject.h>

typedef unsigned long DevId;

#define DEV_ID_MAJOR(devId) (devId >> 20)
#define DEV_ID_MINOR(devId) (devId & 0xFFFF)
#define DEV_ID_MAKE(major, minor) ((major << 20) | (minor & 0xFFFF))

#define KEDEV_OBJ_OFFSET(type, device) (OffsetOf(type, device) + \
	OffsetOf(struct KeDevice, object))

struct KeDevice
{
	struct KeObject object;
	DevId devId;
	DWORD type; /* Move to a KeDeviceClass structure soon. */
	
	union
	{
		struct FileOps* fileOps; /* for character devices. */
		struct StorageDevice* sDevice; /* for block devices. */
		void* devPriv;
	};
};

/* API for manging device objects. */
int KeDeviceInit(struct KeDevice* device, struct KeSet* type, DevId devId,
	void* devPriv, int devType);
int KeDeviceAttach(struct KeDevice* device, const char* name, ...);
int KeDeviceVaAttach(struct KeDevice* device, const char* name, VaList args);

/* TEMP. FIXME: Remove. Too many parameters to function well. */

static inline int KeDeviceCreate(struct KeDevice* device, struct KeSet* parent, DevId devId, 
	void* devPriv, int devType, const char* name, ...)
{
	int err;
	VaList args;
	
	err = KeDeviceInit(device, parent, devId, devPriv, devType);
	
	if (err)
		return err;
	
	VaStart(args, name);
	err = KeDeviceVaAttach(device, name, args);
	VaEnd(args);
	
	return err;
}

int KeDeviceDetach(struct KeDevice* device);

static inline const char* KeDeviceGetName(struct KeDevice* device)
{
	return KeObjGetName(&device->object);
}

static inline struct KeSet* KeDeviceGetSet(struct KeDevice* device)
{
	return KeObjGetSet(&device->object);
}

static inline struct KeDevice* KeObjectToDevice(struct KeObject* object)
{
	return ContainerOf(object, struct KeDevice, object);
}

static inline struct KeFsEntry* KeDeviceGetConfDir(struct KeDevice* device)
{
	return device->object.dir;
}

#endif
