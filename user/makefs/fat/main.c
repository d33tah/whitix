/* mkfatfs */

#include <stdio.h>
#include <stdlib.h>
#include <syscalls.h>

#include "../block.h"

#define ATTR_RDONLY		0x01 /* Read-only */
#define ATTR_HIDDEN		0x02 /* Hidden file */
#define ATTR_SYSTEM		0x04 /* System file */
#define ATTR_LABEL		0x08 /* Volume label */
#define ATTR_DIR		0x10 /* Directory */
#define ATTR_ARCHIVE	0x20 /* Archived */
#define ATTR_DEVICE		0x40 /* Device (never found on disk) */

#define PACKED		__attribute__((packed))

struct FatBootSector
{
	unsigned char jmp[3];
	unsigned char oemID[8];
	unsigned short bytesPerSec;
	unsigned char sectorsPerClus;
	unsigned short reservedSectorCnt;
	unsigned char numFats;
	unsigned short numRootDirEnts;
	unsigned short totalSecSmall;
	unsigned char mediaIDByte;
	unsigned short secsPerFat;
	unsigned short secsPerTrack;
	unsigned short heads;
	unsigned long hiddenSectors;
	unsigned long totalSectorsLarge;

	/* Only used by FAT32 */
	unsigned long secsPerFat32;
	
	unsigned short extFlags;
	unsigned short fsVer;
	unsigned long rootClus;
	unsigned short fsInfo;
	unsigned short bkBootSec;
	char reserved[12];
	unsigned char driveNum;
	unsigned char reserved1;
	unsigned char bootSig;
	unsigned long volID;
	char volLabel[11];
	char fileSysType[8];
}PACKED;

struct FatFsInfo
{
	unsigned long reserved1;
	unsigned long signature;
	unsigned long freeClusters;
	unsigned long nextCluster;
	unsigned long reserved2;
}PACKED;

struct FatDirEntry
{
	char fileName[11];
	unsigned char attribute;
	unsigned char reserved;
	unsigned char creationTimeMs;
	unsigned short creationTime;
	unsigned short creationDate;
	unsigned short accessDate;
	unsigned short startClusterHigh;
	unsigned short lastWriteTime;
	unsigned short lastWriteDate;
	unsigned short startClusterLow;
	unsigned long fileSize;
}PACKED;

#define FAT_MAX12	((1 << 12)-16)
#define FAT_MAX16	((1 << 16)-16)
#define FAT_MAX32	((unsigned long long)(((1 << 28)-16)))

int verbose=0;
char* deviceName=NULL,*volumeName=NULL;
int numFats=2,fatSize=0,secsPerClus=2,reservedSectors=1;
int numRootDirEnts=224;
unsigned long long totalSize;
struct FatBootSector bootSec;
int deviceFd;
int bkBootSec = 0;
unsigned long blockSize,aBlockSize=0;
char* infoSec;
char* emptySector;
unsigned long long numClusters,fatLength;
char* fat;

void GetConfigPath(char* path, char* out)
{
	char* p;
	
	p = strrchr(path, '/');
	
	if (p)
		p++;
	else
		p = path;
		
	strcpy(out, "/Devices/Storage/");
	strcat(out, p);
	strcat(out, "/");
}

int GetDeviceInfo()
{
	struct Stat stat;
	int err;
	char path[270];

	deviceFd=SysOpen(deviceName, _SYS_FILE_FORCE_OPEN | _SYS_FILE_READ | _SYS_FILE_WRITE,0);

	if (deviceFd < 0)
	{
		printf("Error: could not open file '%s'\n",deviceName);
		return deviceFd;
	}

	err=SysStatFd(deviceFd,&stat);

	if (err)
	{
		printf("Error: could not retrieve information about the file\n");
		goto statFailed;
	}
	
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

	if (stat.mode & _SYS_STAT_BLOCK)
	{
		GetConfigPath(deviceName, path);
		
		strcat(path, "blockSize");
				
		if ( SysConfRead(path, &blockSize, sizeof(blockSize)) != sizeof(blockSize))
		{
			printf("mkfatfs: error: could not read block size of device. Assuming a block size of 512.\n");
			blockSize = 512;
		}		
	}else
		blockSize=1024; /* A good default */

	totalSize = stat.size;

	if (verbose)
		printf("Info: device size = %llu (%lluMB, %lluGB), block size = %u\n",totalSize, totalSize/1024/1024, totalSize/1024/1024/1024, blockSize);

	if (aBlockSize)
	{
		if (aBlockSize < blockSize)
			printf("Error: Given block size is less than that of the device (%u < %u)\n",aBlockSize,blockSize);
		else
			blockSize=aBlockSize;
	}

	return 0;

statFailed:
	SysClose(deviceFd);
	return 1;
}

unsigned long long CeilDiv(unsigned long long a, unsigned long b)
{
	return (a+b-1)/b;
}

void FatMarkCluster(int index, unsigned long value)
{
	switch (fatSize)
	{
		case 12:
		{
			unsigned char* fat12=(unsigned char*)fat;
			value &= 0x0FFF;

			if (!((index * 3) & 0x1))
			{
				fat12[3*index/2]=value & 0xFF;
				fat12[(3*index/2)+1]=((fat[(3*index/2)+1] & 0xF0)
					| ((value & 0xF00) >> 8));
			}else{
				fat12[3*index/2] = ((fat[3*index/2] & 0xf) | ((value & 0xf) << 4));
				fat12[(3*index/2)+1] = ((value & 0xff0) >> 4);
			}
			break;
		}

		case 16:
		{
			unsigned short* fat16=(unsigned short*)fat;
			fat16[index] = (unsigned short)value;
			break;
		}
		
		case 32:
		{
			unsigned char* fat32=(unsigned char*)fat;
			value &= 0xFFFFFFF;
			fat32[4*index] = (unsigned char)(value & 0xFF); /* TODO: Check? */
			fat32[(4*index)+1] = (unsigned char)((value & 0xFF00) >> 8);
			fat32[(4*index)+2] = (unsigned char)((value & 0xFF0000) >> 16);
			fat32[(4*index)+3] = (unsigned char)((value & 0xFF000000) >> 24);
			break;
		}
	}
}

int ConstructFs()
{
	unsigned long long fatData;
	unsigned long long numClusts12,fatLength12,maxClusts12;
	unsigned long long numClusts16,fatLength16,maxClusts16;
	unsigned long long numClusts32, fatLength32, maxClusts32;

	fatData=totalSize/blockSize-CeilDiv(numRootDirEnts*32,blockSize)-reservedSectors;

	if (verbose)
		printf("%d sectors for FATs and data\n",fatData);

	/* If we're formatting a big disk that will use FAT32, increase the number of
	 * sectors per cluster.
	 */
	 
	if (totalSize >= 512*1024*1024)
		secsPerClus = 16;

	do
	{
		if (verbose)
			printf("Trying with %d sectors per cluster:\n",secsPerClus);

		numClusts12=2*(fatData*blockSize+numFats*3)/
			(2*(int)secsPerClus*blockSize+numFats*3);

		fatLength12=CeilDiv(((numClusts12+2)*3+1) >> 1, blockSize);
		numClusts12=(fatData-numFats*fatLength12)/secsPerClus;
		maxClusts12=(fatLength12*2*blockSize)/3;

		if (maxClusts12 > FAT_MAX12)
			maxClusts12=FAT_MAX12;

		if (verbose)
			printf("FAT12: number of clusters = %u, fat length = %u, max clusters = %u (limit = %u)\n",
				numClusts12,fatLength12,maxClusts12,FAT_MAX12);

		if (numClusts12 > maxClusts12-2)
		{
			numClusts12=0;
			if (verbose)
				printf("FAT12: too many clusters\n");
		}

		numClusts16=(fatData*blockSize+numFats*4)/
			(secsPerClus*blockSize+numFats*2);

		fatLength16=CeilDiv((numClusts16+2)*2,blockSize);
		numClusts16=(fatData-numFats*fatLength16)/secsPerClus;
		maxClusts16=(fatLength16*blockSize)/2;

		if (maxClusts16 > FAT_MAX16)
			maxClusts16 = FAT_MAX16;

		if (verbose)
			printf("FAT16: number of clusters = %u, fat length = %u, max clusters = %u (limit = %u)\n",
				numClusts16,fatLength16,maxClusts16,FAT_MAX16);

		if (numClusts16 > maxClusts16-2)
		{
			if (verbose)
				printf("FAT16: too many clusters\n");
			
			numClusts16 = 0;
		}
		
		if (numClusts16 < 4085)
		{
			if (verbose)
				printf("FAT16 : would be detected as FAT12\n");

			numClusts16=0;
		}
						
		numClusts32 = ( fatData*blockSize + numFats*8) /
			((int)secsPerClus*blockSize + numFats*4);
						
		fatLength32 = CeilDiv((numClusts32+2) * 4, blockSize);
		
		numClusts32 = (fatData - numFats*fatLength32)/secsPerClus;
		maxClusts32 = (fatLength32*blockSize)/4;

		if (verbose)
			printf("FAT32: number of clusters = %llu, fat length = %llu, max clusters = %llu (limit = %llu)\n",
				numClusts32,fatLength32,maxClusts32,FAT_MAX32);
		
		if (maxClusts32 > FAT_MAX32)
		{
			if (verbose)
				printf("FAT32: too many clusters\n");
				
			numClusts32 = 0;
		}
		
		if (numClusts32 < 65526)
		{
			if (verbose)
				printf("FAT32: too few clusters\n");
				
			numClusts32 = 0;
		}

		if (numClusts12 || numClusts16 || numClusts32)
			break;

		secsPerClus <<= 1;
	}while (secsPerClus && secsPerClus <= 128);

	switch (fatSize)
	{
		case 12:
			break;
		
		case 16:
			break;
		
		case 32:
			if (numClusts32 == 0)
			{
				printf("mkfatfs: error: FAT32 filesystem would be misdetected as FAT12 or FAT16\n");
				return 1;
			}
			break;
			
		case 0:
			fatSize=(numClusts16 > numClusts12) ? 16 : 12;
		
			if (numClusts32 > numClusts16)
				fatSize = 32;

			if (verbose)
				printf("Chose FAT%d\n",fatSize);
	}

	switch (fatSize)
	{
	case 12:
		numClusters=numClusts12;
		fatLength=fatLength12;
		break;

	case 16:
		if (numClusts16 <= FAT_MAX12)
		{
			printf("Error, FAT16: Filesystem will be identified as FAT12, which is incorrect.\n");
			return 1;
		}
		numClusters=numClusts16;
		fatLength=fatLength16;
		break;
		
	case 32:
		numClusters = numClusts32;
		fatLength = fatLength32;
		break;
	}
	
	/* FAT32 needs at least 32 reserved sectors. */
	if (fatSize == 32)
		reservedSectors = 32;

	if (verbose)
		printf("Number of reserved sectors: %u\n", reservedSectors);

	/* TODO: Fill in boot code? */

	strncpy((char*)bootSec.oemID, "mkfatfs", 8);
	bootSec.bytesPerSec=blockSize;
	bootSec.sectorsPerClus=secsPerClus;
	bootSec.reservedSectorCnt=reservedSectors;
	bootSec.numFats=numFats;
	
	if (fatSize != 32)
		bootSec.numRootDirEnts = numRootDirEnts;
	else
		bootSec.numRootDirEnts = numRootDirEnts = 0;
		
	bootSec.mediaIDByte=0xF8;
	bootSec.secsPerTrack=32;
	bootSec.heads=64;
	
	if (fatSize != 32)
		bootSec.secsPerFat = (unsigned short)fatLength;
	else
		bootSec.secsPerFat = 0;
		
	bootSec.hiddenSectors=0;
	
	if (fatSize != 32)
	{
		bootSec.totalSecSmall=totalSize/blockSize;
		bootSec.totalSectorsLarge = 0;
	}else{
		bootSec.totalSectorsLarge = totalSize / blockSize;
		bootSec.totalSecSmall = 0;
	}
		
	bootSec.volID=0xDEADBEEF;
	bootSec.bootSig=0x28;
	bootSec.driveNum=0x80;
	strncpy(bootSec.volLabel,(volumeName) ? volumeName : "whitix", 11);
	snprintf(bootSec.fileSysType, 8, "FAT%d", fatSize);
	
	if (fatSize == 32)
		bootSec.secsPerFat32 = fatLength;
	else
		bootSec.secsPerFat32 = 0;
		
	if (fatSize == 32)
	{
		struct FatFsInfo* info;
		
		/* FAT32 extends the boot structure with useful information. */
		bootSec.extFlags = 0;
		bootSec.fsVer = 0;
		bootSec.rootClus = 2;
		bootSec.fsInfo = 1;
		bkBootSec = bootSec.bkBootSec = 6; /* TODO: Should have backup boot sector? */
		memset(bootSec.reserved, 0, 12);

		/* What about the other members of the boot sector? */
		
		/* Set up the FSINFO structure */
		infoSec = (char*)malloc(blockSize);
		
		info = (struct FatFsInfo*)(infoSec + 0x1E0);
		
		infoSec[0] = 'R';
		infoSec[1] = 'R';
		infoSec[2] = 'a';
		infoSec[3] = 'A';
		
		info->signature = 0x61417272;
		info->freeClusters = numClusters - 2;
		info->nextCluster = 2;
		
		*(unsigned short*)(infoSec + 0x1FE) = 0xAA55;
	}

	/* Allocate the FAT and initiate special entries? */
	fat=(char*)malloc(fatLength * blockSize);
	if (!fat)
		return 1;

	memset(fat, 0, fatLength * blockSize);

	FatMarkCluster(0, 0xFFFFFFFF);
	FatMarkCluster(1, 0xFFFFFFFF);
	
	/* Put the media byte in the first byte. */
	fat[0] = 0xF8;
	
	if (fatSize == 32)
		FatMarkCluster(2, 0x0FFFFFF8);

	return 0;
}

void WriteBuffer(char* data,int size)
{
	SysWrite(deviceFd,(unsigned char*)data,size);
}

void WriteFs()
{
	int i;

	emptySector = (char*)malloc(blockSize);

	memset(emptySector, 0, blockSize);

	SysSeek(deviceFd,0,0);
	
	for (i=0; i<reservedSectors; i++)
		WriteBuffer(emptySector, blockSize);

	if (verbose)
		printf("Wrote reserved sectors\n");

	SysSeek(deviceFd,0,0);
	WriteBuffer((char*)&bootSec, 512);

	if (verbose)
		printf("Wrote boot sector\n");

	if (fatSize == 32)
	{
		SysSeek(deviceFd, blockSize, 0);
			
		WriteBuffer((char*)infoSec, 512);
		free(infoSec);
		
		if (verbose)
			printf("Wrote FSINFO sector\n");
		
		if (bkBootSec)
		{
			SysSeek(deviceFd, bkBootSec * blockSize, 0);
			WriteBuffer((char*)&bootSec, 512);
			
			if (verbose)
				printf("Wrote backup boot sector\n");
		}
	}

	/* Write fat */
	SysSeek(deviceFd,reservedSectors*blockSize, 0);

	if (verbose)
		printf("Writing first FAT\n");

	/* fatLength is length in sectors. */
	WriteBuffer(fat, fatLength * blockSize);
	
	/* Clear out special information at the beginning of the FAT, and write
	 * it as the backup FAT.
	 */
	 
	memset(fat, 0, blockSize);
	
	for (i = 0; i < numFats; i++)
	{
		if (verbose)
			printf("Writing backup FAT %d\n", i);
			
		WriteBuffer(fat, fatLength * blockSize);
	}
	
	/* Write root directory */
	
	if (numRootDirEnts)
	{
		for (i=0; i<((numRootDirEnts*32)/blockSize); i++)
			WriteBuffer(emptySector, blockSize);

		if (verbose)
			printf("Wrote root directory\n");
	}
	
	/* Fix #108 by zeroing out cluster 2. */
	if (fatSize == 32)
	{
		WriteBuffer(emptySector, blockSize);
		
		if (verbose)
			printf("Wrote (FAT32) root directory\n");
	}
	
	/* Free fat entries */
	free(fat);

	if (verbose)
		printf("Filesystem created successfully\n");

	free(emptySector);

	SysClose(deviceFd);
}

void UsagePrint()
{
	printf("mkfatfs v0.2 built at %s on %s\n", __TIME__, __DATE__);
	printf("Format: mkfatfs [options] <device name>\n");
	printf("Options:\n");
	printf("-b - bytes per sector (power of 2)\n"
			"-f <number> - number of FATs (default is 2)\n"
			"-h - print this help message\n"
			"-n <string> - volume name\n"
			"-s <number> - sectors per cluster\n"
			"-S <number> - the FAT entry size (12,16 or 32)\n"
			"-v - verbose execution\n");
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
			case 'b':
				/* atol soon? */
				aBlockSize=atoi(str);
				if (aBlockSize < 512 || aBlockSize > 4096 || (aBlockSize & (aBlockSize-1)))
				{
					printf("Error: Invalid bytes per sector (%u)\n",aBlockSize);
					return 1;
				}
				break;

			case 'f':
				numFats=atoi(str);
				if (numFats < 1 || numFats > 4)
				{
					printf("Error: too many file allocation tables (%d) Acceptable range is 1-4.\n",numFats);
					return 1;
				}
				break;

			case 'S':
				fatSize=atoi(str);
				if (fatSize != 12 && fatSize != 16 && fatSize != 32)
				{
					printf("Error: %d is an illegal FAT size.\n",fatSize);
					return 1;
				}
				break;

			case 'n':
				volumeName=str;
				break;

			case 's':
				secsPerClus=atoi(str);
				if (secsPerClus < 1 || secsPerClus > 128 || (secsPerClus & (secsPerClus-1)))
				{
					printf("Error: illegal number of sectors per cluster (%d).\n",secsPerClus);
					return 1;
				}
				break;

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
			deviceName=strdup(*argv);
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

int main(int argc,char** argv)
{
	if (ParseArguments(argc,argv))
		return 0;

	/* Arguments are ok, determine device size */
	if (GetDeviceInfo())
		return 0;

	if (ConstructFs())
		return 0;

	WriteFs();

	return 0;
}
