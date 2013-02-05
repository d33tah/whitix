#include <acpi/acpi.h>
#include <print.h>
#include <string.h>

DWORD AcpiScanRsdp(DWORD start, DWORD length)
{
	DWORD offset = 0;
	DWORD sigLen = sizeof("RSD PTR ") - 1;
	
	for (offset = 0; offset < length; offset += 16)
	{
		if (strncmp((char*)(start+offset), "RSD PTR ", sigLen) == 0)
		{
			DWORD i;
			BYTE checkSum=0;
			BYTE* table=(BYTE*)(start+offset);
			
			for (i = 0; i < 20; i++)
				checkSum += table[i];
				
			KePrint(KERN_INFO "ACPI: RSDP found at %#X (checksum = %u)\n", table, checkSum);
			
			if (checkSum == 0)
				return (DWORD)table;
		}
	}
	
	return 0;
}

