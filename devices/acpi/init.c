#include <acpi/acpi.h>
#include <print.h>

int AcpiInit()
{
	struct AcpiRsdp* rsdp=AcpiArchGetRsdp();
	
	/* Print out the RSDP */
	KePrint("ACPI: RSDP: revision = %u, RSDT = %#X\n", rsdp->revision, rsdp->rsdtPhysAddr);
	return 0;
}
