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

#include <module.h>
#include <net/network.h>
#include <slab.h>

struct Cache* netDeviceCache;

/* Only one device for now! TODO: Add support for multiple devices. */
struct NetDevice* currDevice;

struct NetDevice* NetDeviceAlloc()
{
	return (struct NetDevice*)MemCacheAlloc(netDeviceCache);
}

SYMBOL_EXPORT(NetDeviceAlloc);

int NetDeviceRegister(struct NetDevice* device)
{
	currDevice=device;
	return 0;
}

SYMBOL_EXPORT(NetDeviceRegister);

int NetDeviceInit()
{
	netDeviceCache=MemCacheCreate("Network device cache", sizeof(struct NetDevice), NULL, NULL, 0);
	return 0;
}
