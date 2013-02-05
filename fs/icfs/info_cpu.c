#include <module.h>
#include <error.h>
#include <module.h>
#include <print.h>
#include <fs/kfs.h>
#include <fs/vfs.h>

int numCpus=1;
char cpuMan[13];
char cpuName[49];

int InfoCpuInit()
{
#if 0
	InfoDir* cpusDir, *cpuDir;
	
	cpusDir=InfoCreateDir(NULL, "Devices/Cpus");
	
	InfoAddIntEntry(cpusDir, "numCpus", &numCpus);
	
	/* Create one CPU directory for now. */
	cpuDir=InfoCreateDir(cpusDir, "0");
	
	/* Add manufacturer data. */
	CpuIdVendorId(cpuMan);
	cpuMan[12]='\0';
	
	CpuIdName(cpuName);
	cpuName[37]='\0';
	
	InfoAddStrEntry(cpuDir, "vendorId", cpuMan);
	InfoAddStrEntry(cpuDir, "name", cpuName);
	InfoAddIntEntry(cpuDir, "features", (int*)CpuIdFeatures());
#endif
	
	return 0;
}
