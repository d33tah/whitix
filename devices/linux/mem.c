#include <console.h>
#include <module.h>
#include <print.h>

void kfree(void* addr)
{
	KePrint("kfree\n");
}

SYMBOL_EXPORT(kfree);

void kfree_skb(void* skb)
{
	KePrint("kfree_skb\n");
}

SYMBOL_EXPORT(kfree_skb);

void* __kmalloc(unsigned long size, int flags)
{
	KePrint("__kmalloc\n");
	return NULL;
}

SYMBOL_EXPORT(__kmalloc);

void printk(const char* message, ...)
{
	VaList args;
			
	VaStart(args, message);
	KeVaPrint(message, args);
	VaEnd(args);
}

SYMBOL_EXPORT(printk);
