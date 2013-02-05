#include <i386/cpuid.h>
#include <i386/i386.h>
#include <i386/virtual.h>
#include <print.h>
#include <i386/msr.h>
#include <i386/process.h>
#include <i386/sib.h>
#include <panic.h>

/* The system information block contains read-only information about the system
 * that can be read from userspace, saving a potential expensive system call.
 */

#define SIB_USER_ADDR		0xF8001000

#define SIB_CURR_VERSION	0x1

struct SystemInfoBlock* block;

/* SIB API */
void SibUpdateTime(unsigned long seconds, unsigned long useconds)
{
	block->seconds = seconds;
	block->uSeconds = useconds;
}

int SibInit()
{
	struct PhysPage* page;
	
	/* Map the system information block into userspace as read-only. To update
	 * the block, we have another map of the physical page somewhere in kernel
	 * memory.
	 */
	 
	/* Userspace map. */
	page = PageAlloc();
	
	if (!page)
		KernelPanic("SIB: Could not allocate a page for the system information block.");
		
	VirtMemMapPage(SIB_USER_ADDR, page->physAddr, PAGE_READONLY | PAGE_USER | PAGE_PRESENT);
	
	/* Kernel map. */
	block = (struct SystemInfoBlock*)VirtAllocateTemp(page->physAddr);
	
	if (!block)
		KernelPanic("SIB: Could not map system information block in kernel memory.");
	
	/* Set up structure. */
	block->version = SIB_CURR_VERSION;
	block->seconds = 0;
	block->uSeconds = 0;
	
	KePrint("SIB: Set up system information block\n");
	 
	return 0;
}
