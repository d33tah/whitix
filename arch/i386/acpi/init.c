#include <acpi/acpi.h>
#include <print.h>
#include <typedefs.h>
#include <string.h>

struct AcpiRsdp* AcpiArchGetRsdp()
{
	unsigned long rsdpPhys = 0;
	
//	rsdpPhys = AcpiScanRsdp(0, 0x400);
	
	/* TODO: Search EBDA */
	
//	if (!rsdpPhys)
		rsdpPhys = AcpiScanRsdp(0xE0000, 0x20000);
		
	if (!rsdpPhys)
	{
		KePrint("ACPI: No ACPI support found.\n");
		return 0;
	}
		
	return (struct AcpiRsdp*)rsdpPhys;	
}
