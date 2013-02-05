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

/* Adapted from the Linux NE2K PCI driver. */

#include <error.h>
#include <module.h>
#include <i386/ioports.h>
#include <i386/irq.h>
#include <net/network.h>
#include <net/socket.h>
#include <net/eth.h>
#include <pci.h>
#include <print.h>

int Ne2kIrq(void* data)
{
//	struct NetDevice* netDev=(struct NetDevice*)data;
	KePrint("Ne2kIrq\n");

	return 0;
}

int Ne2kSend(struct NetDevice* device, struct SocketBuffer* sockBuff)
{
	KePrint("Ne2kSend\n");
	return 0;
}

struct PciDeviceId ne2kIdTable[]=
{
	{0x10EC, 0x8029, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x1050, 0x0940, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x11F6, 0x1401, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x8E2E, 0x3000, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x4A14, 0x5000, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x1106, 0x0926, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x10BD, 0x0E34, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x1050, 0x5A5A, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x12C3, 0x0058, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x12C3, 0x5598, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0x8C4A, 0x1980, PCI_ID_ANY, PCI_ID_ANY, 0, 0, NULL},
	{0, 0, 0, 0, 0, 0, NULL}
};

struct NetDevOps ne2kOps=
{
	.send = Ne2kSend,
};

int Ne2kInitOne(struct PciDevice* device, struct PciDeviceId* devId)
{
	int ret;
	WORD ioAddr;
	struct NetDevice* netDev;
	struct EthDevice* ethDev;

	ret=PciEnableDevice(device);

	if (ret)
	{
		KePrint("NE2K: Could not enable device.\n");
		return -EIO;
	}

	ioAddr=PciResourceStart(device, 0);

	if (inb(ioAddr) == 0xFF)
		goto out;

	netDev=NetDeviceAlloc();

	if (!netDev)
	{
		KePrint("NE2K: Could not allocate network device.\n");
		return -EIO;
	}

	/* Fill in the network device structure. */
	netDev->ops=&ne2kOps;
	strncpy(netDev->name, "Ethernet", sizeof("Ethernet"));
	ethDev=EthDeviceAlloc(netDev);

	ret=IrqAdd(device->irq, Ne2kIrq, netDev);

	if (ret)
	{
		KePrint("NE2K: Could not enable device irq.\n");
		return -EIO;
	}

	KePrint("NE2K: Found device. I/O base = %#X, irq = %u\n", ioAddr, device->irq);

	/* Get the station (MAC) address. */

	NetDeviceRegister(netDev);
	
out:
	return 0;
}

struct PciDriver ne2kDriver=
{
	.name = "NE2K (PCI)",
	.idTable = ne2kIdTable,

	/* Functions */
	.initOne = Ne2kInitOne,
};

int Ne2kInit()
{
	return PciRegisterDriver(&ne2kDriver);
}

ModuleInit(Ne2kInit);
