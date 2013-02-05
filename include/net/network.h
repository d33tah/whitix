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

#ifndef NETWORK_H
#define NETWORK_H

#include <pci.h>

#define NET_FAMILIES_MAX	5

#define PF_LOCAL	0

int NetworkInit();

/* Forward declarations. */
struct NetDevice;
struct SocketBuffer;

/* Device management. */
struct NetDevOps
{
	int (*send)(struct NetDevice* device, struct SocketBuffer* sockBuff);
};

struct NetDevice
{
	char name[12];
	int type; /* Ethernet, wireless etc. */
	struct PciDevice* pciDev;
	struct NetDevOps* ops;
	void* procPriv;
	void* priv;
};

struct NetDevice* NetDeviceAlloc();
int NetDeviceRegister(struct NetDevice* device);

#endif
