#include <fs/vfs.h>
#include <module.h>
#include <panic.h>
#include <sched.h>
#include <typedefs.h>
#include <sections.h>
#include <task.h>
#include <print.h>
#include <addresses.h>
#include <fs/devfs.h>

static void KernelFreeSections()
{
	DWORD addr;

	KePrint(KERN_INFO "KERN: Freeing startup sections of kernel\n");

	for (addr = (DWORD)initcode_start; addr < (DWORD)initcode_end; addr+=PAGE_SIZE)
		PageFreeAddr(VA_TO_PA(addr));
	
	for (addr = (DWORD)initdata_start; addr < (DWORD)initdata_end; addr+=PAGE_SIZE)
		PageFreeAddr(VA_TO_PA(addr));
}

void Start()
{
	int fds[]={
		0, 0, 0,
	};

	ModulesBootLoad();
	
	extern void StorageMountRoot();
	StorageMountRoot();
	
	KernelFreeSections();
	
	KePrint("DEVFS: Mounting device filesystem at %s\n", DEVICES_PATH);

	if (VfsMount(NULL, DEVICES_PATH, "DevFs", NULL))
		KernelPanic("Failed to mount the device filesystem");

	KePrint("Mounted the filesystems. Starting the shell\n");

	current->files[0] = FileAllocate();

	if (DoOpenFile(current->files[0], DEVICES_PATH "Consoles/Console0",FILE_READ | FILE_FORCE_OPEN,0))
		KernelPanic("Failed to open the first console. Halting");

	extern int Exec(char* pathName,int* fds,char** argv);
	if (Exec("/System/Startup/startup", fds, NULL) < 0)
		KernelPanic("Could not launch the startup program. Halting");

	ThrProcessExit(0);
	ThrSchedule();

	/* Shouldn't hit here. */
	while (1)
		hlt();
}

int StartInit()
{
	struct Process* start;
	struct Thread* firstThread;

	start=ThrCreateProcess("KernelLoading");
	firstThread=ThrCreateThread(start, (DWORD)Start, false, 0, NULL);

	if (!start || !firstThread)
		KernelPanic("Could not create the starting thread.\n");

	ThrStartThread(firstThread);

	return 0;
}
