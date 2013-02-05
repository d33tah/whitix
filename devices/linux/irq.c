#include <console.h>
#include <module.h>
#include <pci.h>
#include <print.h>

int synchronize_irq(unsigned int irq)
{
	KePrint("synchronize_irq(%u)\n", irq);
	return 0;
}

SYMBOL_EXPORT(synchronize_irq);

void free_irq(unsigned int irq, void* dev_id)
{
	KePrint("free_irq(%u)\n", irq);
}

SYMBOL_EXPORT(free_irq);

void disable_irq(unsigned int irq)
{
	KePrint("disable_irq(%u)\n", irq);
}

SYMBOL_EXPORT(disable_irq);

void enable_irq(unsigned int irq)
{
	KePrint("enable_irq(%u)\n", irq);
}

SYMBOL_EXPORT(enable_irq);

int probe_irq_off(unsigned long val)
{
	KePrint("probe_irq_off(%u)\n", val);
	return 0;
}

SYMBOL_EXPORT(probe_irq_off);

int probe_irq_on(void)
{
	KePrint("probe_irq_on\n");
	return 0;
}

SYMBOL_EXPORT(probe_irq_on);

int request_irq(unsigned int irq, void (*handler)(int, void*, void*),
	unsigned long irqflags, const char* devname, void* devid)
{
	KePrint("request_irq(%u)\n", irq);
	return 0;
}

SYMBOL_EXPORT(request_irq);

void local_bh_disable()
{
	KePrint("local_bh_disable\n");
}

SYMBOL_EXPORT(local_bh_disable);

void local_bh_enable()
{
	KePrint("local_bh_enable\n");
}

SYMBOL_EXPORT(local_bh_enable);

void _spin_lock(void* lock)
{
	KePrint("_spin_lock\n");
}

SYMBOL_EXPORT(_spin_lock);

void _spin_lock_irqsave(void* lock)
{
	KePrint("_spin_lock_irqsave\n");
}

SYMBOL_EXPORT(_spin_lock_irqsave);

void _spin_unlock_irqrestore(void* lock)
{
	KePrint("_spin_unlock_irqrestore\n");
}

SYMBOL_EXPORT(_spin_unlock_irqrestore);

/* TODO: CHECK */
unsigned int pv_lock_ops;

SYMBOL_EXPORT(pv_lock_ops);
