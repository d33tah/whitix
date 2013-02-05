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

#include <devices/class.h>
#include <devices/input.h>
#include <fs/devfs.h>
#include <llist.h>
#include <module.h>
#include <error.h>
#include <request.h>
#include <panic.h>
#include <print.h>
#include <fs/vfs.h>
#include <slab.h>
#include <sections.h>
#include <malloc.h>

struct DevClass inputClass;

/* TODO: Sort out device ids, add vararg names. */

struct InputDevice* InputDeviceAdd(const char* name, struct FileOps* ops)
{
	struct InputDevice* device;
	
	device = (struct InputDevice*)MemAlloc(sizeof(struct InputDevice));
	
	KeDeviceCreate(&device->device, &inputClass.set, DEV_ID_MAKE(0, 0), ops, DEVICE_CHAR,
		name);
	
	return device;
}

SYMBOL_EXPORT(InputDeviceAdd);

initcode int InputInit()
{
	/* TODO: Create cache. */
	
	DevClassCreate(&inputClass, NULL, "Input");
	
	return 0;
}
