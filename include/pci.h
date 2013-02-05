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

#ifndef PCI_H
#define PCI_H

#include <console.h>
#include <typedefs.h>

struct PciBus
{
	int index;
	struct ListHead next;
	struct ListHead deviceList;
};

struct PciDriver;

struct PciDevice
{
	struct PciBus* bus;
	BYTE dev,func;
	WORD vendorId,deviceId;
	BYTE devClass,subClass,irq;
	WORD subVendor, subDevice;
	DWORD spaces[6]; /* I/O or memory space */
	
	struct PciDriver* driver;
	void* driverData;

	struct ListHead next;
};

/* PCI BIOS structure. */
struct PciBios
{
	DWORD magicNumber;
	DWORD physEntry;
	BYTE version;
	BYTE length;
	BYTE crc;
	BYTE reserved[5];
}PACKED;

/* PCI configuration space registers */
#define PCI_VENDOR_ID		0x00
#define PCI_DEVICE_ID		0x02
#define PCI_COMMAND			0x04
#define		PCI_COMMAND_IO		0x01
#define		PCI_COMMAND_MEM		0x02
#define		PCI_COMMAND_MASTER	0x04
#define PCI_SUBSYS_VENDOR_ID 0x2C
#define PCI_SUBSYS_ID		0x2E
#define PCI_INTERRUPT_PIN	0x3D

extern int (*PciRead)(int bus, int dev, int func, int reg, int bytes, DWORD* val);
extern int (*PciWrite)(int bus, int dev, int func, int reg, DWORD v, int bytes);

/* Internal functions */
int PciDetectBios();

/* Internal defines */
#define PCI_VENDOR_INVALID(vendorId) (((vendorId) == 0xFFFF) || ((vendorId) == 0x0000))
#define PCI_RES_BEGIN_MASK(addressReg) ((addressReg) & 0xFFFFFFF0)

/*** LIBRARY CODE ***/

/* PCI library structures. Used by drivers. */

#define PCI_ID_ANY	(DWORD)(~0)	/* Used if the driver doesn't care about a certain ID field. */

#define PciIdTableEnd() {0, 0, 0, 0, 0, 0, NULL}

struct PciDeviceId
{
	DWORD vendorId, deviceId;
	DWORD subVendor, subDevice;
	DWORD class, classMask;
	void* data;
};

struct PciDriver
{
	char* name;
	struct PciDeviceId* idTable;

	/* Function pointers. */
	int (*initOne)(struct PciDevice* device, struct PciDeviceId* devId);

	/* Internal data */
	struct ListHead next;
	void* data;
};

/* PCI library functions. Used by drivers. */
int PciEnableDevice(struct PciDevice* device);
int PciRegisterDriver(struct PciDriver* pciDriver);
unsigned long PciResourceStart(struct PciDevice* device, int index);
int PciWriteConfigByte(struct PciDevice* device, int reg, BYTE value);
int PciReadConfigByte(struct PciDevice* device, int reg, BYTE* value);
int PciReadConfigWord(struct PciDevice* device, int reg, WORD* value);

int PciReadConfigDword(struct PciDevice* device, int reg, DWORD* value);
int PciWriteConfigDword(struct PciDevice* device, int reg, DWORD value);

#endif
