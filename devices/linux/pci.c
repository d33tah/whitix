#include <console.h>
#include <module.h>
#include <pci.h>
#include <print.h>

#include "include/module.h"
#include "include/pci.h"

int pci_choose_state(void* dev, unsigned int state)
{
	KePrint("pci_choose_state(%#X)\n", dev);
	return 0;
}

SYMBOL_EXPORT(pci_choose_state);

int pci_set_power_state(void* dev, int num)
{
	KePrint("pci_set_power_state(%#X, %d)\n", dev, num);
	return 0;
}

SYMBOL_EXPORT(pci_set_power_state);

int pci_save_state(void* dev, void* buffer)
{
	KePrint("pci_save_state(%#X, %#X)\n", dev, buffer);
	return 0;
}

SYMBOL_EXPORT(pci_save_state);

int pci_restore_state(void* dev)
{
	KePrint("pci_restore_state(%#X)\n", dev);
	return 0;
}

SYMBOL_EXPORT(pci_restore_state);

int pci_enable_wake(void* dev, int state, int enable)
{
	KePrint("pci_enable_wake(%#X, %d, %d)\n", dev, state, enable);
	return 0;
}

SYMBOL_EXPORT(pci_enable_wake);

int quickcall pci_enable_device(struct pci_device* dev)
{
	return PciEnableDevice(dev->data);
}

SYMBOL_EXPORT(pci_enable_device);

int pci_disable_device(void* dev)
{
	return 0;
}

SYMBOL_EXPORT(pci_disable_device);

void quickcall pci_set_master(struct pci_device* dev)
{
	PciSetMaster(dev->data);
}

SYMBOL_EXPORT(pci_set_master);

int LinuxPciInitOne(struct PciDevice* device, struct PciDeviceId* devId)
{
	struct PciDriver* pciDriver = device->driver;
	struct pci_driver* driver = (struct pci_driver*)pciDriver->data;
	struct pci_device* dev = (struct pci_device*)MemAlloc(sizeof(struct pci_device));
	int i;
	
	for (i=0; i<6; i++)
	{
		dev->resources[i].start = device->spaces[i] & ~0xF;
		KePrint("start = %#X\n", dev->resources[i].start);
	}
	
	KePrint("resources offset = %#X\n", OffsetOf(struct pci_device, resources[0].start));
	KePrint("[0].start = %#X\n", dev->resources[0].start);
	
	KePrint("val = %#X\n", *(DWORD*)((char*)dev+0x194));
	
	dev->data = device;
	
	return driver->probe(dev, devId);
}

int quickcall __pci_register_driver(struct pci_driver* driver, struct module* owner, const char* modName)
{
	/* PciDriver cache? */
	struct PciDriver* pciDriver = (struct PciDriver*)MemAlloc(sizeof(struct PciDriver));
	
	pciDriver->name = driver->name;
	pciDriver->idTable = driver->id_table; /* Same format. */
	pciDriver->initOne = LinuxPciInitOne;
			
	driver->data = pciDriver;
	pciDriver->data = driver;
		
	return PciRegisterDriver(pciDriver);
}

SYMBOL_EXPORT(__pci_register_driver);

void pci_unregister_driver(void* dev)
{
	KePrint("pci_unregister_driver\n");
}

SYMBOL_EXPORT(pci_unregister_driver);
