#include <error.h>
#include <module.h>
#include <i386/ioports.h>
#include <i386/irq.h>
#include <pci.h>
#include <print.h>

#define PCI_ID_NVIDIA	0x10DE

/* PCI ID table. */
struct PciDeviceId nvidiaIdTable[]=
{
	{PCI_ID_NVIDIA, PCI_ID_ANY, PCI_ID_ANY, PCI_ID_ANY, PCI_ID_ANY, PCI_ID_ANY, NULL},
	PciIdTableEnd(),
};

int NvidiaInitOne(struct PciDevice* device, struct PciDeviceId* devId)
{
	if (PciEnableDevice(device))
	{
		KePrint(KERN_ERROR "NV: could not enable device.\n");
		return -EIO;
	}
	
	KePrint("first: %#X\n", PciResourceStart(device, 0));
	KePrint("second: %#X\n", PciResourceStart(device, 1));
	
	return 0;
}

struct PciDriver nvidiaDriver =
{
	.name = "NVidia",
	.idTable = nvidiaIdTable,
	.initOne = NvidiaInitOne,
};

int NvidiaInit()
{
	return PciRegisterDriver(&nvidiaDriver);
}

ModuleInit(NvidiaInit);
