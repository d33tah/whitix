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


/* Code used by PCI drivers. */

#include <i386/ioports.h>
#include <module.h>
#include <llist.h>
#include <module.h>
#include <pci.h>
#include <slab.h>

LIST_HEAD(pciDriverList);

int (*PciRead)(int bus, int dev, int func, int reg, int bytes, DWORD* val);
int (*PciWrite)(int bus, int dev, int func, int reg, DWORD v, int bytes);

/* These functions assume PciRead and PciWrite will be set, because the code calling
 * the functions won't be executed if no PCI support can be found.
 */

int PciWriteConfigByte(struct PciDevice* device, int reg, BYTE value)
{
	return PciWrite(device->bus->index, device->dev, device->func, reg, value, 1);
}

SYMBOL_EXPORT(PciWriteConfigByte);

int PciReadConfigByte(struct PciDevice* device, int reg, BYTE* value)
{
	DWORD val;
	int ret;

	ret=PciRead(device->bus->index, device->dev, device->func, reg, 1, &val);

	if (!ret)
		*value=(BYTE)val;

	return ret;
}

SYMBOL_EXPORT(PciReadConfigByte);

int PciWriteConfigWord(struct PciDevice* device, int reg, WORD value)
{
	return PciWrite(device->bus->index, device->dev, device->func, reg, value, 2);
}

SYMBOL_EXPORT(PciWriteConfigWord);

int PciReadConfigWord(struct PciDevice* device, int reg, WORD* value)
{
	DWORD val;
	int ret;

	ret=PciRead(device->bus->index, device->dev, device->func, reg, 1, &val);

	if (!ret)
		*value=(WORD)val;

	return ret;
}

SYMBOL_EXPORT(PciReadConfigWord);

int PciReadConfigDword(struct PciDevice* device, int reg, DWORD* value)
{
	return PciRead(device->bus->index, device->dev, device->func, reg, 4, value);
}

SYMBOL_EXPORT(PciReadConfigDword);

int PciWriteConfigDword(struct PciDevice* device, int reg, DWORD value)
{
	return PciWrite(device->bus->index, device->dev, device->func, reg, value, 4);
}

SYMBOL_EXPORT(PciWriteConfigDword);

int PciEnableDevice(struct PciDevice* device)
{
	/* Enable base address registers */
	WORD command;

	if (PciReadConfigWord(device, PCI_COMMAND, &command))
		return -EIO;

	/* For now, just enable I/O and memory. */
	command = command | PCI_COMMAND_IO | PCI_COMMAND_MEM;

	return PciWriteConfigWord(device, PCI_COMMAND, command);
}

SYMBOL_EXPORT(PciEnableDevice);

int PciSetMaster(struct PciDevice* device)
{
	WORD command;

	if (PciReadConfigWord(device, PCI_COMMAND, &command))
		return -EIO;

	command |= PCI_COMMAND_MASTER;

	return PciWriteConfigWord(device, PCI_COMMAND, command);
}

SYMBOL_EXPORT(PciSetMaster);

int PciRegisterDriver(struct PciDriver* pciDriver)
{
	/* Add it to the global list of PCI drivers, after checking that it
	 * hasn't been added already. */

	struct PciDriver* curr;
	struct PciBus* bus;

	ListForEachEntry(curr, &pciDriverList, next)
		if (pciDriver == curr)
			return -EEXIST;

	ListAddTail(&pciDriver->next, &pciDriverList);
	
	extern struct ListHead pciBusList;
	
	ListForEachEntry(bus, &pciBusList, next)
		PciAttachDriver(bus, pciDriver);
	
	return 0;
}

SYMBOL_EXPORT(PciRegisterDriver);

unsigned long PciResourceStart(struct PciDevice* device, int index)
{
	if (index < 0 || index >= 6)
		return -EINVAL;

	return PCI_RES_BEGIN_MASK(device->spaces[index]);
}

SYMBOL_EXPORT(PciResourceStart);
