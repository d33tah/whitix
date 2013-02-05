#include <stdio.h>
#include <stdlib.h>

#include <syscalls.h>

#define PATH_MAX	2048

void UsagePrint()
{
	printf("moduleadd <module name>\n"
			"Inserts a module into the kernel.\n");

	exit(0);
}

int ModuleLoad(char* name)
{
	/* Support multiple paths. */
	int fd;
	unsigned int size;
	int ret;
	unsigned long address;

	fd = SysOpen(name, _SYS_FILE_FORCE_OPEN, 0);

	if (fd < 0)
	{
		printf("moduleadd: Could not open %s\n", name);
		return 1;
	}

	/* Get the file size. */
	size = SysSeek(fd, 0, SEEK_END);
	SysSeek(fd, 0, SEEK_SET);

	address=SysMemoryMap(0, size, 5, fd, 0, _SYS_MMAP_PRIVATE);

	if (!address)
	{
		printf("Could not read %s\n", name);
		return 1;
	}
	
	/* Get the module name (minus .sys) out of the pathname. */
	char* begin, *end;
	
	begin = strrchr(name, '/');
	if (!begin)
		begin = name;
	else
		begin++;
		
	end = strrchr(name, '.');
	*end = '\0';
	
	ret=SysModuleAdd(begin, address, size);

	SysMemoryUnmap(address, size);

	/* Check ret. */
	if (ret)
		printf("moduleadd: Error adding module %s to kernel: %s\n", name, strerror(ret));

	SysClose(fd);

	return ret;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
		UsagePrint();

	char* moduleName=argv[1];
	char buf[PATH_MAX+100];
	int ret;

	if (!strstr(moduleName, ".sys"))
		sprintf(buf, "%s.sys", moduleName);
	else
		strcpy(buf, moduleName);

	ret=ModuleLoad(buf);

#if 0
	if (ret == 1)
	{
		/* Could not load module file, try .ko for Linux kernel modules, and load in the Linux driver layer module. */
		strcpy(buf, "/System/Modules/Core/linux.sys");
		
		ret=ModuleLoad(buf);
		
		if (!ret)
		{
			sprintf(buf, "%s.ko", moduleName);
			ret=ModuleLoad(buf);
		}
	}
#endif

	if (ret == 1)
	{
		printf("moduleadd: Could not load %s. Exiting\n", buf);
	}

	return ret;
}
