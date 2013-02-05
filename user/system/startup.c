#include <stdio.h>

#include <syscalls.h>

/* TODO: Make more configurable. Create a thread for each directory. */

#define PATH_MAX	2048

int threadId = -1;

int ModuleLoad(char* name)
{
	char* args[]={name, NULL};

	return SysCreateProcess("moduleadd", NULL, args);
}

/* TODO: Finish. */
int ModuleLoadDirectory(char* dirName)
{
	char buf[PATH_MAX];
	char dirEntryBuf[4096];

	int fd=SysOpen(dirName, _SYS_FILE_READ, 0);
	int ret=0;

	while ((ret=SysGetDirEntries(fd, dirEntryBuf, 4096)) > 0)
	{
		printf("ret = %d\n", ret);
	}

	SysClose(fd);
}

int networkFinished=0;

void NetworkInit(void* args)
{
	int pid;

	pid=ModuleLoad("Core/net");

	SysWaitForProcessFinish(pid, NULL);

	ModuleLoad("Core/local");
	ModuleLoad("Core/inet");

	pid=ModuleLoad("Core/pci");

	SysWaitForProcessFinish(pid, NULL);

//	ModuleLoad("Video/nvidia");

	ModuleLoad("Network/ne2k_pci");
	
	int ret;
	
	/* Move to FS thread? */
	ModuleLoad("Filesystems/fatfs");
	pid=ModuleLoad("Filesystems/icfs");
	
	SysWaitForProcessFinish(pid, NULL);
	
	printf("Mounting the system filesystem..");
	
	ret=SysMount(NULL, "/System/Config", "icfs", NULL);
	
	if (!ret)
		printf("[OK]\n");
	else{
		printf("[Failed]: %s\n", strerror(-ret));
		SysExit(-1);
	}

	networkFinished=1;

	SysResumeThread(threadId);

	SysExitThread(-1);
}

int storageFinished=0;

void StorageInit(void* args)
{
	ModuleLoad("Storage/ramdisk");
	ModuleLoad("Storage/floppy");

	storageFinished=1;

	SysResumeThread(threadId);
	
	SysExitThread(-1);
}

int inputFinished=0;

void InputInit()
{
	int pid;

	pid=ModuleLoad("Input/Ps2");

	SysWaitForProcessFinish(pid, NULL);

	ModuleLoad("Input/keyboard");
	ModuleLoad("Input/Ps2Mouse");

	inputFinished=1;

	SysResumeThread(threadId);
	SysExitThread(-1);
}

int unixFinished=0;

void UnixInit()
{
//	ModuleLoad("Core/serial");
	ModuleLoad("Core/shmem");
	ModuleLoad("Video/virtual");

	ModuleLoad("Profile/profiler");

	unixFinished=1;

	SysResumeThread(threadId);
	SysExitThread(-1);
}

int ShellRun()
{
	int pid;
	int numTextConsoles, i;
	
	SysConfRead("/Devices/Consoles/numTextConsoles", &numTextConsoles, 4);	
	
	for (i = 0; i < numTextConsoles; i++)
	{
		char consName[255];
		int fd;
		int shellFds[3];

		sprintf(consName, "/System/Devices/Consoles/Console%d", i);

		fd = SysOpen(consName, _SYS_FILE_FORCE_OPEN, 0);

		if (fd < 0)
		{
			printf("startup: could not open Console%d\n", i);
			continue;
		}

		shellFds[0] = fd;
		shellFds[1] = fd;
		shellFds[2] = fd;

		pid = SysCreateProcess("/Applications/burn", shellFds, NULL);
		
		if (pid < 0)
			printf("startup: could not launch the shell on console %d\n", i);
	}

    SysWaitForProcessFinish(-1, NULL);

	/* Figure out which burn to restart on the console? */
}

int main(int argc, char* argv[])
{
	int pid;

	/* TODO: Mount device filesystem. */

	/* Load modules */
	SysChangeDir("/System/Modules/");

	SysCreateThread(NetworkInit, 0, NULL);
	SysCreateThread(InputInit, 0 , NULL);
	SysCreateThread(StorageInit, 0, NULL);
	SysCreateThread(UnixInit, 0, NULL);

	threadId = SysGetCurrentThreadId();

	while (!inputFinished || !networkFinished || !storageFinished || !unixFinished)
	{
//		printf("%#X %#X %#X %#X\n", inputFinished, networkFinished, storageFinished, unixFinished);
		SysSuspendThread(threadId);
	}

	SysChangeDir("/");

//	SysCreateProcess("/System/Startup/regserver", NULL, NULL);

	ShellRun();

//	SysShutdown(0);
	
	while (1)
		SysSuspendThread(threadId);
	
	return 0;
}
