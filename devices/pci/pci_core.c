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

#include <print.h>
#include <i386/ioports.h>
#include <module.h>
#include <llist.h>
#include <pci.h>
#include <slab.h>

LIST_HEAD(pciBusList);
struct Cache* pciDeviceCache;
struct Cache* pciBusCache;

extern struct ListHead pciDriverList;

struct PciIoData
{
	BYTE reg:8;
	BYTE func:3;
	BYTE dev:5;
	BYTE bus:8;
	BYTE resvd:7;
	BYTE enable:1;
};

int Type1Read(int bus, int dev, int func, int reg, int bytes, DWORD* val)
{
	WORD base;
		
	union
	{
		struct PciIoData c;
		DWORD n;
	}u;

	//Clear struct
	u.n=0;

	u.c.enable=1;
	u.c.resvd=0;
	u.c.bus=bus;
	u.c.dev=dev;
	u.c.func=func;
	u.c.reg=reg & 0xFC;

	outd(0xCF8,u.n);
	base=0xCFC+(reg & 0x03);

	switch (bytes)
	{
		case 1:	*val=(DWORD)inb(base); break;
		case 2: *val=(DWORD)inw(base); break;
		case 4: *val=(DWORD)ind(base); break;
		default: return -EINVAL;
	}
	
	return 0;
}

int Type1Write(int bus,int dev,int func,int reg,DWORD v,int bytes)
{
	WORD base;
	union
	{
		struct PciIoData c;
		DWORD n;
	}u;

	u.n=0;
	u.c.enable=1;
	u.c.resvd=0;
	u.c.bus=bus;
	u.c.dev=dev;
	u.c.func=func;
	u.c.reg=reg & 0xFC;

	base=0xCFC+(reg & 0x03);
	
	outd(0xCF8, u.n);
	
	switch (bytes)
	{
		case 1: outb(base,(BYTE)v); break;
		case 2: outw(base,(WORD)v); break;
		case 4: outd(base,v); break;
		default: return -EINVAL;
	}

	return 0;
}

char* classes[]=
{
	"Unclassified device",
	"Mass storage controller",
	"Network controller",
	"Display controller",
	"Multimedia controller",
	"Memory controller",
	"Bridge",
	"Communication controller",
	"Generic system peripheral",
	"Input device controller",
	"Docking station",
	"Processor",
	"Serial bus controller",
	"Wireless controller",
	"Intelligent controller",
	"Satellite communications controller",
	"Encryption controller",
	"Signal processing controller",
};

#define PCI_WRITE_TEST	0xFFFFFFFF
#define PCI_BAR_BASE	0x10

int PciDeviceProbe(struct PciDevice* device)
{
	/* Fill in device-specific information, such as IRQ, I/O and memory. */
	BYTE irq;
	DWORD v, size;
	int i;

	for (i=0; i<6; i++)
	{
		PciReadConfigDword(device, PCI_BAR_BASE + (i << 2), &v);
	
		PciWriteConfigDword(device, PCI_BAR_BASE + (i << 2), PCI_WRITE_TEST);
		PciReadConfigDword(device, PCI_BAR_BASE + (i << 2), &size);
		
		PciWriteConfigDword(device, PCI_BAR_BASE + (i << 2), v);
		
		if (!size || size == 0xFFFFFFFF)
			continue;
			
		if (v == PCI_WRITE_TEST)
			v = 0;
		
		size &= 0xFFFFFFF0;
							
		/* Parse the base address register. */
		device->spaces[i] = v;
	}

	PciReadConfigByte(device, 0x3D, &irq);

	if (irq)
	{
		PciReadConfigByte(device, 0x3C, &irq);
		device->irq = irq;
	}

	/* Read the subsystem vendor ID and device ID. Used in device identification. */
	PciReadConfigWord(device, PCI_SUBSYS_VENDOR_ID, &device->subVendor);
	PciReadConfigWord(device, PCI_SUBSYS_ID, &device->subDevice);

	return 0;
}

struct PciDevice* PciProbe(struct PciBus* bus, int dev, int func)
{
	DWORD v;
	WORD vendorId, deviceId;
	BYTE deviceType;
	struct PciDevice* device;

	/* Read in the device and vendor ID. It's the best way of detect whether
	 * a device actually exists at that location. */
	PciRead(bus->index, dev, func, PCI_VENDOR_ID, 2, &v);
	vendorId=(WORD)v;
	PciRead(bus->index, dev, func, PCI_DEVICE_ID, 2, &v);
	deviceId=(WORD)v;

	/* No device here if the vendor ID is invalid. */
	if (PCI_VENDOR_INVALID(vendorId))
		return NULL;

	/* Read in the header type to find out if it is a multi-function device.
	 * If it isn't, there's no point allocating a device structure for it. */
	PciRead(bus->index, dev, func, 0x0E, 1, &v);
	deviceType=(BYTE)v;

	if (func && !(deviceType & 0x80))
		return NULL;

	/* Allocate the device. */
	device=(struct PciDevice*)MemCacheAlloc(pciDeviceCache);

	if (!device)
		return NULL;

	/* Start populating the structure. */
	device->vendorId=vendorId;
	device->deviceId=deviceId;
	device->bus=bus;
	device->dev=dev;
	device->func=func;

	PciReadConfigByte(device, 0x0B, &device->devClass);
	PciReadConfigByte(device, 0x0A, &device->subClass);
	
	switch (deviceType & 0x7F)
	{
		case 0:
			PciDeviceProbe(device);
			break;

		case 1:
			/* TODO: Scan bridges. */
//			KePrint("PCI to PCI bridge\n");
			break;

		default:
			break;
	}

	return device;
}

int PciScanBus(int index)
{
	int dev, func;
	struct PciBus* bus;
	struct PciDevice* device;

	/* Allocate the bus. */
	bus=(struct PciBus*)MemCacheAlloc(pciBusCache);

	if (!bus)
		return -ENOMEM;

	INIT_LIST_HEAD(&bus->deviceList);
	ListAddTail(&bus->next, &pciBusList);
	bus->index=index;

	/* Populate its device list. */
	for (dev=0; dev<32; dev++)
		for (func=0; func<8; func++)
		{
			device=PciProbe(bus, dev, func);

			if (device)
				ListAddTail(&device->next, &bus->deviceList);
		}

	return 0;
}

int PciAttachDriverOne(struct PciBus* bus, struct PciDevice* device, struct PciDriver* driver)
{
	struct PciDeviceId* currId;
	int ret;

	/* Search through each device id entry, looking for a match. */
	currId = driver->idTable;

	while (currId->vendorId)
	{
		if ((currId->vendorId == device->vendorId || currId->vendorId == PCI_ID_ANY) &&
			(currId->deviceId == device->deviceId || currId->deviceId == PCI_ID_ANY) &&
			(currId->subVendor == device->subVendor || currId->subVendor == PCI_ID_ANY) &&
			(currId->subDevice == device->subDevice || currId->subDevice == PCI_ID_ANY) /*&&
			((currId->class ^ device->devClass) & currId->classMask)*/)
		{
			device->driver = driver;
			
			/* Matched! Start up the device. */
			ret = driver->initOne(device, currId);
			
			if (ret)
				device->driver = NULL;
				
			return ret;
		}

		currId++;
	}

	return 0;
}

int PciAttachDriver(struct PciBus* bus, struct PciDriver* driver)
{
	struct PciDevice* curr;

	ListForEachEntry(curr, &bus->deviceList, next)
	{
		int ret;

		ret=PciAttachDriverOne(bus, curr, driver);
		if (ret)
			return ret;
	}

	return 0;
}

int PciCheckConfig()
{
	DWORD temp;
	
	outb(0xCFB,0x01);
	temp=ind(0xCF8);
	outd(0xCF8,0x80000000);
	if (ind(0xCF8) == 0x80000000)
	{
		outd(0xCF8,temp);
		return 1;
	}

	outd(0xCF8,temp);
		
	outb(0xCFB,0x00);
	outb(0xCF8,0x00);
	outb(0xCFA,0x00);

	if (inb(0xCF8) == 0x00 && inb(0xCFA) == 0x00)
	{
		return 2;
	}

	return 0;
}

int PciInit()
{
	int type=PciCheckConfig();	

	if (!type)
	{
		PciDetectBios();
		return 0;
	}else if (type == 2)
	{
		KePrint("Type 2 PCI buses are not supported\n");
		return 0;
	}

	PciRead=Type1Read;
	PciWrite=Type1Write;

	/* Allocate a cache since there might be a lot of PCI devices */
	pciDeviceCache=MemCacheCreate("PciDevice",sizeof(struct PciDevice),NULL,NULL,0);
	if (!pciDeviceCache)
	{
		KePrint("PCI: Could not allocate device cache\n");
		return -ENOMEM;
	}

	pciBusCache=MemCacheCreate("PciBus", sizeof(struct PciBus), NULL, NULL, 0);
	if (!pciBusCache)
	{
		KePrint("PCI: Could not allocate bus cache\n");
		return -ENOMEM;
	}

	/* Scan the first bus initially. We may scan other buses as a result. */
	PciScanBus(0);

	/* Add devices to IcFs */

	return 0;
}

ModuleInit(PciInit);
