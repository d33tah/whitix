#include <console.h>
#include <module.h>
#include <print.h>
#include <typedefs.h>

struct device
{
};

void* dma_ops;

SYMBOL_EXPORT(dma_ops);

void* dma_alloc_coherent(struct device* dev, unsigned long size, void* handle, int gfp)
{
	KePrint("dma_alloc_coherent\n");
	return NULL;
}

SYMBOL_EXPORT(dma_alloc_coherent);

void* dma_free_coherent(struct device* dev, unsigned long size, void* cpu_addr, unsigned int handle)
{
	KePrint("dma_free_coherent\n");
	return NULL;
}

SYMBOL_EXPORT(dma_free_coherent);

int dma_supported(struct device* dev, unsigned long long mask)
{
	KePrint("dma_supported\n");
	return 1;
}

SYMBOL_EXPORT(dma_supported);
