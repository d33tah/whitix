#include <devices/sdevice.h>
#include <llist.h>
#include <error.h>
#include <request.h>
#include <print.h>
#include <malloc.h>
#include <fs/vfs.h>
#include <fs/devfs.h>
#include <module.h>
#include <i386/virtual.h>
#include <devices/class.h>

/* Globals */

struct DevClass ramDiskClass;

struct RamDisk
{
	char* data;
	struct StorageDevice* sDev;
};

/* Useful defines */

#define MAX_RAMDISKS 2
#define RAMDISK_SIZE (4*1024*1024) /* In bytes */
#define RAMDISK_SSIZE 1024 /* Sector size, in bytes */

static struct RamDisk disks[MAX_RAMDISKS];
struct StorageDevice rdDevices[MAX_RAMDISKS];

/***********************************************************************
 *
 * FUNCTION:	RamDiskRequest
 *
 * DESCRIPTION: Service a read or write request to a ramdisk.
 *
 * PARAMETERS: data - pointer to a RamDisk structure.
 *
 * RETURNS: 0 if successful, -EIO if request could not be serviced.
 *
 ***********************************************************************/

int RamDiskRequest(void* data)
{
	struct RamDisk* disk=(struct RamDisk*)data;
	struct StorageDevice* sDev=disk->sDev;
	struct Request* request=StorageGetCurrRequest(sDev);
	int reqSize=request->numSectors*RAMDISK_SSIZE;
	int memOffset=request->sector*RAMDISK_SSIZE;
	if (!request)
		return 0;

	if (!disk->data)
	{
		/* Allocate from the malloc range - maybe should choose somewhere else? */
		disk->data=(char*)VirtMapPhysRange(0xC0000000,0xD0000000,RAMDISK_SIZE >> PAGE_SHIFT,3);

		if (!disk->data)
		{
			StorageEndRequest(sDev,ENOMEM);
			return -ENOMEM;
		}
	}

	/* Past the end of the ramdisk? */
	if (memOffset >= (RAMDISK_SIZE-reqSize))
	{
		StorageEndRequest(sDev,EIO);
		return -EIO;
	}

	if (request->type == REQUEST_WRITE)
		memcpy((char*)(disk->data+memOffset),(char*)request->data,reqSize);
	else if (request->type == REQUEST_READ)
		memcpy((char*)request->data,(char*)(disk->data+memOffset),reqSize);

	StorageEndRequest(sDev,0);
	return 0;
}

#define RAMDISK_MAJOR	10

KE_OBJECT_TYPE_SIMPLE(ramDiskType, /* RamDiskFree */ NULL);

int RamDiskInit()
{
	int i;
	int err;
	
	/* Create the ramdisk set */
	err = DevClassCreate(&ramDiskClass, &ramDiskType, "Ram");
	
	if (err)
		return err;

	for (i=0; i<MAX_RAMDISKS; i++)
	{
		disks[i].sDev=&rdDevices[i];
		disks[i].data=NULL;

		/* Set up the storage device structure */
		rdDevices[i].request=RamDiskRequest;
		rdDevices[i].blockSize=RAMDISK_SSIZE;
		rdDevices[i].priv=&disks[i];
		rdDevices[i].totalSectors=RAMDISK_SIZE/RAMDISK_SSIZE;
		
		StorageDeviceInit(&rdDevices[i], DEV_ID_MAKE(RAMDISK_MAJOR,  i));
		
		StorageDeviceAdd(&rdDevices[i], "RamDisk%d", i);
	}

	KePrint("RAM: %dMB %d ramdisks loaded\n",RAMDISK_SIZE/1024/1024, MAX_RAMDISKS);

	return 0;
}

ModuleInit(RamDiskInit);
