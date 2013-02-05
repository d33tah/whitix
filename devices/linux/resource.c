#include <console.h>
#include <module.h>
#include <print.h>

void* __request_region(void* parent, unsigned long start, unsigned long n, char* name)
{
	KePrint("__request_region\n");
	return 0xDEADCAFE;
}

SYMBOL_EXPORT(__request_region);

void __release_region(void* parent, unsigned long start, unsigned long n)
{
	KePrint("__release_region\n");
}

SYMBOL_EXPORT(__release_region);

/* Wrong type. */
unsigned int ioport_resource;

SYMBOL_EXPORT(ioport_resource);

int per_cpu__cpu_number;

SYMBOL_EXPORT(per_cpu__cpu_number);

void* per_cpu__current_task;

SYMBOL_EXPORT(per_cpu__current_task);
