#ifndef LINUX_PCI_H
#define LINUX_PCI_H

#include <llist.h>
#include <typedefs.h>

struct device
{
	char data[200+112];
};

struct resource
{
	unsigned long start, end;
	const char* name;
	unsigned long flags;
	struct resource* parent, *sibling, *child;
};

struct pci_device
{
	struct ListHead bus_list;
	void* bus;
	void* subord;
	void* sysdata;
	void* proc_ent;
	void* slot;
	unsigned int devfn;
	unsigned short vendor, device, subsys_vendor, subsys_device;
	unsigned int class;
	BYTE revision, hdr_type, pcie_type, rom_base_reg, pin;
	struct pci_driver* driver;
	unsigned long long dma_mask;
	int dma_params;
	int current_state;
	int pm_cap;

	unsigned int pme_support:5;
	unsigned int d1_support:1;
	unsigned int d2_support:1;
	unsigned int no_d1d2:1;

	int error_state;
	struct device dev;
	int cfg_size;
	
	unsigned int irq;
	struct resource resources[12];
	void* data;
};

struct pci_driver
{
	struct ListHead list;
	char* name;
	struct PciDeviceId* id_table; /* Binary compatiable */
	__attribute__ ((regparm(3))) int (*probe)(struct pci_device* dev, const struct PciDeviceId* id);
	void* data;
};

#endif
