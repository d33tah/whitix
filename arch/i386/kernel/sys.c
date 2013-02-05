#include <i386/cpuid.h>
#include <i386/i386.h>
#include <i386/virtual.h>
#include <print.h>
#include <i386/msr.h>
#include <i386/process.h>

DWORD* CpuIdFeatures();
void SysEnterEntry();

extern char* intSysCall;
extern char* endIntSysCall;

extern char* sysEnterSysCall;
extern char* endSysEnterSysCall;

DWORD sysEnterStack[16];

#define SYSCALL_STUB_ADDR		0xF8000000

int SysArchInit()
{
	DWORD features = *(CpuIdFeatures());
	char vendorId[12];
	void* intAddr, *intEndAddr;	

	CpuIdVendorId(vendorId);

	if ((features & CPU_FEATURE_SEP) && !strncmp(vendorId, "GenuineIntel",11))
	{
		KePrint(KERN_DEBUG "CPU: Using SYSENTER for system calls.\n");

		intAddr = &sysEnterSysCall;
		intEndAddr = &endSysEnterSysCall;

		/* Write to the MSRs. The processor reads them when it executes the call. */
		MsrWrite(MSR_SYSENTER_CS, GDT_KCODE_SELECTOR);
		MsrWrite(MSR_SYSENTER_EIP, (unsigned long)SysEnterEntry);
		MsrWrite(MSR_SYSENTER_ESP, TSS_ADDR+4);
	}else{
		KePrint(KERN_DEBUG "CPU: Using interrupt for system calls.\n");
			
		intAddr = &intSysCall;
		intEndAddr = &endIntSysCall;
	}
	
	/* Copy the exit code over to a user accessable page of memory. */
	VirtMemMapPage(SYSCALL_STUB_ADDR, PageAlloc()->physAddr, PAGE_PRESENT | PAGE_USER);
	memcpy((void*)SYSCALL_STUB_ADDR, intAddr, intEndAddr - intAddr);

	return 0;
}
