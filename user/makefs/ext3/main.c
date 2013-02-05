#include <stdio.h>
#include <syscalls.h>

int verbose=0;
char* deviceName;
int deviceFd;
unsigned long blockSize;
char confBuf[2048];

int GetDeviceInfo()
{
	struct Stat stat;
	int err;

	deviceFd=SysOpen(deviceName,_SYS_FILE_FORCE_OPEN | _SYS_FILE_READ | _SYS_FILE_WRITE,0);

	if (deviceFd < 0)
	{
		printf("Error: could not open file '%s'\n",deviceName);
		return deviceFd;
	}

	err=SysStatFd(deviceFd,&stat);

	if (err)
		goto statFailed;
	
	/* Does not make much sense */
	if (stat.mode & _SYS_STAT_CHAR)
	{
		printf("Error: device is a character device\n");
		return 1;
	}

	if (!(stat.mode & _SYS_STAT_WRITE))
	{
		printf("Error: device is read-only\n");
		return 1;
	}

	/* Copy device name. */
	
	if (stat.mode & _SYS_STAT_BLOCK)
	{
		sprintf(confBuf, "/Devices/Storage/%s/blockSize", deviceName);
		err=SysConfRead(confBuf, &blockSize, 4);
		if (err)
		{
			printf("Could not read block size.\n");
			goto statFailed;
		}
	}else
		blockSize=1024; /* A good default */
	
	printf("blockSize = %u\n", blockSize);
	
	return 0;

statFailed:
	SysClose(deviceFd);
	return -1;
}

void UsagePrint()
{
	printf("mkext3fs v0.1 built at %s on %s\n", __TIME__, __DATE__);
	printf("Format: mkext3fs [options] <device name>\n");
	printf("Options:\n");
	printf("-v - verbose execution\n");
	printf("-h - print this message\n");
}

int ParseArguments(int argc,char** argv)
{
	char o;
	char* str;

	if (argc < 2)
	{
		UsagePrint();
		return 1;
	}

	argv++;

	while (*argv)
	{
		/* Option */
		if ((*argv)[0] == '-' && strlen(*argv) > 1)
		{
			o=(*argv)[1];
			argv++;
			str=*argv;

			if (str)
				argv++;

			if (!str && o != 'v' && o != 'h')
			{
				printf("Error: -%c requires parameter.\n",o);
				return 1;
			}

			switch (o)
			{
			case 'v':
				verbose=1;
				if (str)
					argv--;
				break;

			case 'h':
				UsagePrint();
				return 1;

			default:
				//printf("Error: Unknown option -%c.\n",o);
				return 1;
			}
		}else{
			deviceName=*argv;
			/* Device name */
			argv++;
		}
	}

	/* Check that things are ok */
	if (!deviceName)
	{
		printf("Error: no device name given\n");
		return 1;
	}

	return 0;
}

int ConstructFs()
{
	printf("ConstructFs\n");
}

int main(int argc, char* argv[])
{
	if (ParseArguments(argc,argv))
		return 0;

	/* Arguments are okay, determine device size */
	if (GetDeviceInfo())
		return 0;
		
	if (ConstructFs())
		return 0;

#if 0
	WriteFs();
#endif

	return 0;
}
