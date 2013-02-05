/* DiskFormat - formats a hard-drive of choice */

/* TODO: offer a choice between user, kernel and table's view of geometry */

#include <stdio.h>
#include <stdlib.h>

struct AtaPartition
{
	BYTE boot,startHead,startSector,startCyl,system,endHead,endSector,endCyl;
	DWORD startSectorAbs;
	DWORD sectorCount;
}PACKED;

/* Should go in some XML data file someday? It's pretty constant though.. */
char partTypes[]=
{

};

void ClearScreen()
{
	fputs("\33[H\33[2J",stdout);
}

void CursorSetColor(int color)
{
	printf("\33[%dm",color);
}

void CursorMove(int x,int y)
{
	printf("\33[%d;%dH",x,y);
}

void EraseCurrLine()
{
	/* Clear the whole of the current line */
	fputs("\33[2K",stdout);
}

void PrintHeader(char* message,int y)
{
	int x=40-(strlen(message)/2);

	CursorMove(0,y);
	CursorSetColor(7);
	EraseCurrLine();
	CursorMove(x,y);
	fputs(message,stdout);
	CursorSetColor(0);
}

struct AtaPartition partitions[4];
int zeroTable=0;

struct DiskGeo
{
	DWORD heads,cylinders,sectors;
}PACKED;

struct DiskGeo diskGeom;

int ReadInPartitions(FILE* file)
{
	int i;
	char data[512];

	/* Read the first sector - this contains the partition information */
	fseek(file,0,SEEK_SET);
	if (!fread(data,512,1,file))
	{
		printf("DiskFormat - failed to read first sector. Exiting\n");
		return 1;
	}

	/* Get geometry from kernel */
	if (SysIoCtl(file->fd,DISK_GET_GEOMETRY,&diskGeom))
	{
		printf("DiskFormat - failed to get geometry\n");
		return 1;
	}

	for (i=0; i<64; i++)
		if (data[0x1BE+i])
			break;

	if (i == 64)
	{
		/* Zero table */
		zeroTable=1;
		return 0;
	}

	memcpy(partitions,&data[0x1BE],64);

	return 0;
}

int main(int argc,char** argv)
{
	char devName[255]="/Devices/HardDriveA";
	int i;

	if (argc == 2)
		strcpy(devName,argv[1]);

	FILE* file=fopen(devName,"rb");
	if (!file)
	{
		printf("DiskFormat - failed to open device file\n");
		return 0;
	}

	if (ReadInPartitions(file))
	{
		fclose(file);
		return 0;
	}

	if (zeroTable)
	{
		/* Set up one partition entry, free and has the whole hard-drive */
	}

	/* Set up text */
	ClearScreen();

	PrintHeader("DiskFormat V0.01",0);
	char text[255];
	sprintf(text,"Disk drive: %s",devName);
	PrintHeader(text,1);

	printf("\nName\t\t\tPart Type\t\t\t\tFS Type\t\t\tSize (MB)\n");
	for (i=0; i<80; i++)
		putchar('-');

	for (i=0; i<4; i++)
		if (partitions[i].system)
			printf("%s%d\t\t%s\t\t0x%X\t\t\t\t%u\n",devName,partitions[i].boot == 0x80 ? "boot" : "",i,partitions[i].system,(partitions[i].sectorCount*512)/1024);

	fclose(file);
	return 0;
}
