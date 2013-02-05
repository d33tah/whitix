#include <stdio.h>
#include <syscalls.h>

char wbuf[150000];

int FileCopy(char* src, char* dest)
{
	int fd, fd2;
	int err;

	printf("%s -> %s\n", src, dest);
	
	fd = SysOpen(src, _SYS_FILE_READ | _SYS_FILE_FORCE_OPEN, 0);
	fd2 = SysOpen(dest, _SYS_FILE_CREATE_OPEN | _SYS_FILE_WRITE, 0);

	if (fd < 0)
	{
		printf("textinstall: could not open %s. Exiting\n", src);
		exit(0);
	}else if (fd2 < 0) {
		printf("textinstall: could not open %s. Exiting\n", dest);
		exit(0);
	}
	
	do
	{
		err = SysRead(fd, wbuf, 150000);
	
		if (err <= 0)
			break;
	
		err = SysWrite(fd2, wbuf, err);
	}while (err > 0);
	
	SysClose(fd2);
	SysClose(fd);
}

int main(int argc, char* argv[])
{
	char dev[2048], drive[2048], *p;
	char* devName;
	char* driveName;
	struct Stat stat;

	if (argc < 2)
	{
		printf("textinstall <device name>\n");
		return 0;
	}

	devName = argv[1];
	
	driveName = strndup(devName, strlen(devName)-1);
		
	/* Partition devices end in a number. */
	if (!isalpha(devName[strlen(devName)-1]))
	{
		printf("textinstall: only partition installations are supported.\n");
		return 0;
	}
	
	p = strrchr(driveName, '/');
	
	if (p)
		p++;
	else
		p = driveName;
	
	/* If the path given is not absolute, prepend /System/Devices/Storage. */	
	if (devName[0] != '/')
		sprintf(dev, "/System/Devices/Storage/%s", devName);
	else
		strcpy(dev, devName);
		
	sprintf(drive, "/System/Devices/Storage/%s", p);
	free(driveName);

	/* Check the device actually exists before we do anything more. */
	if (SysStat(dev, &stat, -1))
	{
		printf("textinstall: could not find the specified drive.\n");
		return 0;
	}
		
	printf("WARNING: textinstall is alpha software. Do not use on production systems\n");
	printf("textinstall will format %s as a FAT filesystem.", devName);

	char c;

	do
	{
		printf("\nAre you sure you want to continue? ");

		c = getchar();

		if (tolower(c) == 'n')
		{
			putchar('\n');
			return 0;
		}
	}while (tolower(c) != 'y');

	putchar('\n');
	
	char* args[]={dev, NULL};
	
	printf("textinstall: formatting %s as a FAT filesystem.\n", devName);
	
	int pid=SysCreateProcess("/Applications/mkfatfs", NULL, args); 

	SysWaitForProcessFinish(pid, NULL);

	printf("textinstall: formatted %s\n", devName);
	printf("textinstall: installing system files\n");
	
	int err = SysMount(dev, "/mount", NULL, NULL);
	
	if (err)
	{
		printf("textinstall: failed to mount %s on %s. Error: %s\n", dev, "/mount", strerror(-err));
		return 0;
	}
	
	printf("Creating directories\n");
	
	/* Install files! */
	err = SysCreateDir("/mount/Applications", 0);
	err = SysCreateDir("/mount/Boot", 0);
	err = SysCreateDir("/mount/Boot/Grub", 0);
	err = SysCreateDir("/mount/System", 0);
	err = SysCreateDir("/mount/System/Devices", 0);
	err = SysCreateDir("/mount/System/Modules", 0);
	err = SysCreateDir("/mount/System/Modules/Core", 0);
	err = SysCreateDir("/mount/System/Modules/Filesystems", 0);
	err = SysCreateDir("/mount/System/Modules/Input", 0);
	err = SysCreateDir("/mount/System/Startup", 0);
	err = SysCreateDir("/mount/System/Config", 0);
	err = SysCreateDir("/mount/System/Runtime", 0);
	err = SysCreateDir("/mount/Users", 0);
	
	printf("textinstall: copied system files. Installing MBR and bootloader.\n");
	
	int fdDisk = SysOpen(drive, _SYS_FILE_READ | _SYS_FILE_WRITE, 0);
	
	if (fdDisk < 0)
	{
		printf("Could not open disk to copy over MBR and bootloader. drive = '%s', err = %d\n", drive, fdDisk);
		return 0;
	}
	
	int fd2=SysOpen("/Boot/stage1", _SYS_FILE_READ | _SYS_FILE_FORCE_OPEN, 0);

	char buf[512];
	SysRead(fd2, buf, 512);
	
	/* Copy the first 446 bytes */
	SysWrite(fdDisk, buf, 0x1BE);
	SysSeek(fdDisk, 510, 0);
	SysSeek(fd2, 510, 0);
	
	buf[510]=0x55;
	buf[511]=0xAA;
	SysWrite(fdDisk, &buf[510], 2);
	
	SysClose(fd2);
	
	/* Store FAT stage 1.5 */
	
	fd2=SysOpen("/Boot/Grub/fat_stage1_5", _SYS_FILE_READ | _SYS_FILE_FORCE_OPEN, 0);

	if (fd2 < 0)
	{
		printf("textinstall: could not open FAT stage 1.5\n");
		return 0;
	}
	
	err = SysRead(fd2, wbuf, 150000);

	/* Round up. */
	int totalSectors = (err+511)/512;

	/* Write the number of sectors to load. */
	*(unsigned short*)(wbuf+512-8+4)=(err+511)/512;

/*
	if (totalSectors + 1 >= firstPartStart)
	{
		// Not enough space. Let user know and exit.
	}
*/

	err = SysWrite(fdDisk, wbuf, err);
	
	SysClose(fd2);
	SysClose(fdDisk);
	
	FileCopy("/Boot/Grub/stage2", "/Mount/Boot/Grub/stage2");
	FileCopy("/Boot/Kernel", "/Mount/Boot/Kernel");
	FileCopy("/Boot/Grub/hdgrub.conf", "/Mount/Boot/Grub/menu.lst");

	FileCopy("/System/Startup/startup", "/Mount/System/Startup/startup");

#define SameCopy(src) \
	FileCopy(src, "/Mount" src)
	
	SameCopy("/System/Runtime/liblinker.so");
	SameCopy("/System/Runtime/libstdc.so");
	SameCopy("/System/Runtime/libpthread.so");
	SameCopy("/System/Runtime/libconsole.so");
	SameCopy("/System/Runtime/libregistry.so");
	SameCopy("/System/Runtime/libnetwork.so");
	SameCopy("/System/Runtime/libfile.so");
	SameCopy("/System/Modules/moduleadd");
	SameCopy("/Applications/burn");

#define ModuleCopy(module ) \
	FileCopy("/System/Modules/" module, "/Mount/System/Modules/" module)

	ModuleCopy("Core/console.sys");
	FileCopy("/System/Modules/Filesystems/fatfs.sys", "/Mount/System/Modules/Core/fatfs.sys");
	ModuleCopy("Filesystems/icfs.sys");
	ModuleCopy("Core/ata_ide.sys");
	ModuleCopy("Input/Ps2.sys");
	ModuleCopy("Input/keyboard.sys");

	err = SysUnmount("/mount");
	
	if (err)
	{
		printf("textinstall: failed to unmount. Error %s\n", strerror(-err));
		return 0;
	}
	
	SysFileSystemSync();
	printf("textinstall: installed Whitix to %s\n", devName);

	return 0;
}
