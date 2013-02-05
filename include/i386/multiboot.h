#ifndef I386_MULTIBOOT_H
#define I386_MULTIBOOT_H

#include <typedefs.h>

struct MbModule
{
	DWORD modStart;
	DWORD modEnd;
	char* string;
	DWORD reserved;
};

struct MbMemoryMap
{
	DWORD size;
	QWORD baseAddr;
	QWORD length;
	DWORD type;
};

/* Defines for the MbInfo structure */

/* Magic number passed by a multiboot-compliant bootloader */
#define MULTIBOOT_MAGIC 0x2BADB002

#define MBFLAGS_CLINE	(1 << 2)
#define MBFLAGS_MODULE	(1 << 3)
#define MBFLAGS_ELF		(1 << 5)
#define MBFLAGS_MMAP	(1 << 6)

#define MULTIBOOT_REQ_FLAGS (MBFLAGS_CLINE | MBFLAGS_MODULE | MBFLAGS_ELF | MBFLAGS_MMAP)

struct MbInfo
{
	DWORD flags;
	DWORD memLower;
	DWORD memUpper;
	DWORD bootDevice;
	char* commandLine;
	DWORD modCount;
	struct MbModule* modAddr;
	
	/* Elf section header table information. */
	DWORD sectionNum;
	DWORD sectionSize;
	DWORD sectionAddr;
	DWORD sectionIndex;
	
	DWORD mmapLength;
	struct MbMemoryMap* mmapAddr;
};

char* ArchGetCommandLine();

#endif
