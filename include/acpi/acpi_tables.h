#ifndef ACPI_TABLES_H
#define ACPI_TABLES_H

#include <typedefs.h>

/* TODO: Document. */

#define ACPI_OEM_ID_SIZE		6

struct AcpiRsdp
{
	char signature[8];
	BYTE checkSum;
	char oemId[ACPI_OEM_ID_SIZE];
	BYTE revision;
	DWORD rsdtPhysAddr;
	DWORD length;
	QWORD xsdtPhysAddr;
	BYTE extendedSum;
	BYTE reserved[3];
};

#if 0
struct AcpiTableHeader
{
	char signature[ACPI_NAME_SIZE];
	DWORD length;
	BYTE revision;
	BYTE checkSum;
	char oemId[ACPI_OEM_ID_SIZE];
	char oemTableId[ACPI_OEM_TABLE_ID_SIZE];
	DWORD oemRevision;
	char aslCompilerId[ACPI_NAME_SIZE];
	DWORD aslCompilerRevision;
};
#endif

/* Functions */
DWORD AcpiScanRsdp(DWORD start, DWORD length);
struct AcpiRsdp* AcpiArchGetRsdp();

int AcpiInit();

#endif
