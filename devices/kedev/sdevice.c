/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <devices/class.h>
#include <devices/sdevice.h>
#include <llist.h>
#include <module.h>
#include <error.h>
#include <request.h>
#include <panic.h>
#include <console.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <print.h>
#include <fs/devfs.h>

/* Function prototypes */
int StorageQueueInit(struct StorageDevice* dev);

LIST_HEAD(sDeviceList);
Spinlock requestLock;
Spinlock deviceListLock;

struct KeSubsystem storage;
struct DevClass storageClass;

int StorageDeviceInit(struct StorageDevice* dev, DevId devId)
{
	dev->softBlockSize=dev->blockSize;	
	
	return KeDeviceInit(&dev->device, &storageClass.set, devId, dev, DEVICE_BLOCK);
}

SYMBOL_EXPORT(StorageDeviceInit);


/*******************************************************************************
 *
 * FUNCTION:	StorageDeviceAdd
 *
 * DESCRIPTION: Attach the device to the filesystem with the specified name.
 *
 * PARAMETERS:	dev - device to attach
 *				name - name to attach it under.
 *
 * RETURNS:		Standard return code. -EEXIST if the device already exists.
 *
 ******************************************************************************/

int StorageDeviceAdd(struct StorageDevice* dev, const char* name, ...)
{
	int err;
	VaList args;

	/* General storage device setup. */
	BlockCreateHashTable(dev);
	StorageQueueInit(dev);
	
	VaStart(args, name);
	err = KeDeviceVaAttach(&dev->device, name, args);
	VaEnd(args);

	/* TODO: Implement icfs attributes in struct StorageDevice and figure out how attribute inheritance
	 * might work.
	 */	
	struct KeFsEntry* dir=KeDeviceGetConfDir(&dev->device);
		
	IcFsAddIntEntry(dir, "blockSize", &dev->blockSize, VFS_ATTR_READ);
	IcFsAddIntEntry(dir, "heads", &dev->heads, VFS_ATTR_READ);
	IcFsAddIntEntry(dir, "cylinders", &dev->cylinders, VFS_ATTR_READ);
	IcFsAddIntEntry(dir, "sectors", &dev->sectors, VFS_ATTR_READ);
	
	return err;
}

SYMBOL_EXPORT(StorageDeviceAdd);

int StorageSetCreate(struct KeSet* set, struct KeObjType* type, const char* name)
{
	return KeSetCreate(set, &storageClass.set, type, name);
}

SYMBOL_EXPORT(StorageSetCreate);

int StorageRemoveDevice(struct StorageDevice* dev)
{
#if 0
	if (dev->name)
		MemFree(dev->name);

	SpinLock(&deviceListLock);
	ListRemove(&dev->list);
	SpinUnlock(&deviceListLock);
#endif
	return 0;
}

SYMBOL_EXPORT(StorageRemoveDevice);

/* Request queue handling */

int StorageQueueInit(struct StorageDevice* dev)
{
	/* Set up the request list head */
	INIT_LIST_HEAD(&dev->requestList);
	INIT_WAITQUEUE_HEAD(&dev->flushDone);
	return 0;
}

SYMBOL_EXPORT(StorageQueueInit);

struct Request* StorageBuildRequest(struct Buffer* buffer,int type)
{
	struct Request* request;

	if (!buffer->device->blockSize)
		return NULL;

	request=(struct Request*)MemAlloc(sizeof(struct Request));

	if (!request)
		return NULL;

	request->type=type;
	request->data=buffer->data;
	request->dataLength=buffer->device->softBlockSize; /* Should be block size per device */
	request->sector=(buffer->blockNum*buffer->device->softBlockSize)/buffer->device->blockSize;
	request->ioStatus=request->retries=0;
	request->buffer=buffer;
	request->numSectors=buffer->device->softBlockSize/buffer->device->blockSize;

	return request;
}

/*
 * Always hold a spinlock when dealing with the list, because an irq can alter the list
 * at the end of its request, causing havoc with ListEmpty etc.
 */

struct Request* StorageGetCurrRequest(struct StorageDevice* dev)
{
	struct Request* retVal=NULL;

	SpinLockIrq(&requestLock);

	if (!ListEmpty(&dev->requestList))
		retVal=ListEntry(dev->requestList.next,struct Request,list);

	SpinUnlockIrq(&requestLock);

	return retVal;
}

SYMBOL_EXPORT(StorageGetCurrRequest);

int StorageDoRequest(struct StorageDevice* dev,struct Request* request)
{
	int isEmpty;
	int err=-SIOPENDING;

	if (!dev || !request)
		return -EFAULT;

	SpinLockIrq(&requestLock);
	
	isEmpty = (StorageGetCurrRequest(dev) == NULL);
	ListAddTail(&request->list, &dev->requestList);

	SpinUnlockIrq(&requestLock);

	/* If the list was empty, it's worth calling the request function */
	if (isEmpty)
		err = dev->request(dev->priv);
				
	return err;
}

/* Ends the current request with the error code. This is called from an interrupt */

int StorageEndRequest(struct StorageDevice* device, int status)
{
	struct Request* request = StorageGetCurrRequest(device);

	if (!request)
		KePrint("StorageEndRequest: request == NULL?\n");
	
	request->ioStatus = status;
		
	SpinLockIrq(&requestLock);
	
	ListRemoveInit(&request->list);

	if (status > 0)
		KePrint("I/O error reading sector %u (%d)\n",request->sector, status);
		
	/* BufferUnlock also wakes up the queue that's waiting for it */
	
	BufferUnlock(request->buffer);
		
	SpinUnlockIrq(&requestLock);

	return 0;
}

SYMBOL_EXPORT(StorageEndRequest);

int StorageQueueDestroy(struct StorageDevice* dev)
{
	/* Free all requests */
	return 0;
}

char* StorageParseCommandLine(char* commandLine)
{
	char* root = commandLine;
	
	while (strncmp(root, "root=", sizeof("root=")-1) && *root != '\0')
		root++;
		
	if (*root == '\0')
		return NULL;
	
	root+=sizeof("root=")-1;
	
	return root;
}

char* ArchGetCommandLine();

void StorageMountRoot()
{
	struct KeObject* object;
	char* commandLine = ArchGetCommandLine();
	char* root;
	
	root = StorageParseCommandLine(commandLine);
	
	object = KeSetFind(&storageClass.set, root);
	
	if (!object)
		KernelPanic("Device is not a valid root device");

	VfsMountRoot( KeObjectToStorageDev(object) );
}

SYMBOL_EXPORT(StorageMountRoot);

int StorageInit()
{
	return DevClassCreate(&storageClass, NULL, "Storage");
}
