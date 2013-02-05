#include <i386/multiboot.h>
#include <i386/physical.h>
#include <module.h>
#include <i386/virtual.h>
#include <print.h>
#include <panic.h>
#include <module.h>
#include <sections.h>
#include <config.h>

#define ARCH_CLINE_SIZE		256

struct MbInfo* mbInfo = (struct MbInfo*)0xDEADBEEF;
initdata DWORD mbMagic = 0XDEADBEEF;

char mbCommandLine[ARCH_CLINE_SIZE];

initdata struct MbMemoryMap* memoryMap;
extern int numEntries;

initcode void i386CopyMultiBoot(DWORD magic, struct MbInfo* info)
{
	mbMagic = magic;
	mbInfo = info;
}

initcode void i386InitMultiBoot()
{
	/* Check the flags and magic number */
	if ((mbMagic != MULTIBOOT_MAGIC) || !(mbInfo->flags & MULTIBOOT_REQ_FLAGS))
		KernelPanic("Invalid bootloader.");

	memoryMap = mbInfo->mmapAddr;
	numEntries = mbInfo->mmapLength/sizeof(struct MbMemoryMap);

	/* Copy over the command line as well. */
	strncpy(mbCommandLine, mbInfo->commandLine, ARCH_CLINE_SIZE);
}

initcode void ArchGetSectionInfo(DWORD* sectionAddr, DWORD* numEntries)
{
	*sectionAddr = mbInfo->sectionAddr;
	*numEntries = mbInfo->sectionNum;
}

initcode void ArchModulesLoad()
{
	DWORD i;
	struct MbModule* curr = mbInfo->modAddr;
	
	for (i=0; i<mbInfo->modCount; i++)
	{
		void* kData;
		DWORD length;
		
		length = curr[i].modEnd - curr[i].modStart;
		kData=(void*)VirtMapPhysRange(MODULE_START, MODULE_END, PAGE_ALIGN_UP(length) >> PAGE_SHIFT, 3);

		/* We get the module as a pathname, like /System/Modules/Core/console.sys.
		 * Trim it to console.sys
		 */
		 
		char* begin, *end;
		
		begin = end = curr[i].string + strlen(curr[i].string);
		while (*begin != '/')
			begin--;
			
		begin++;
			
		while (*end != '.')
			end--;

		*end = '\0';

		if (ModuleAdd(begin, (void*)curr[i].modStart, kData, length, 0))
		{
			KePrint(KERN_ERROR "Could not load %s\n", curr[i].string);
			KernelPanic("Could not initialize the kernel");
		}
	}
}

char* ArchGetCommandLine()
{
	return mbCommandLine;
}
