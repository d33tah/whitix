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

#include <devices/device.h>
#include <fs/devfs.h>
#include <keobject.h>
#include <module.h>

/* Set up the structure. */
int KeDeviceInit(struct KeDevice* device, struct KeSet* type, DevId devId,
	void* devPriv, int devType)
{
	KeObjectInit(&device->object, type);
		
	device->devId = devId;
	device->devPriv = devPriv;
	device->type = devType;
	
	return 0;
}

SYMBOL_EXPORT(KeDeviceInit);

int KeDeviceVaAttach(struct KeDevice* device, const char* name, VaList args)
{
	int err;
	
	err = KeObjectVaAttach(&device->object, name, args);
	
	if (err)
		return err;
		
	/* Now add the device in the device filesystem. */
	return DevFsAddDevice(device, KeObjGetName(&device->object));	
}

SYMBOL_EXPORT(KeDeviceVaAttach);

int KeDeviceAttach(struct KeDevice* device, const char* name, ...)
{
	int err;
	VaList args;
	
	/* Bind the KeObject to the filesystem first. This creates a directory
	 * in IcFs, along with all the attributes of the device type.
	 */
	 
	VaStart(args, name);
	
	err = KeDeviceVaAttach(device, name, args);
	
	VaEnd(args);
	
	return err;
}

SYMBOL_EXPORT(KeDeviceAttach);
