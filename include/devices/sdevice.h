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

#ifndef SDEVICE_H
#define SDEVICE_H

#include "device.h"

#include <typedefs.h>
#include <llist.h>
#include <wait.h>

/* Common interface to storage devices for the block cache code */
struct StorageDevice
{
	/* Has name and list */
	struct KeDevice device;
	
	int (*request)(void* data);
	DWORD blockSize,softBlockSize;
	struct ListHead requestList;
	struct ListHead* hashLists;
	void* priv;
	
	unsigned long long totalSectors;
	int partIndex; /* For partition code */

	/* Hardware information - may not apply for some storage devices */
	DWORD heads, sectors, cylinders;
	
	/* Flushing information */
	int syncing, numDirty;
	WaitQueue flushDone;
};

static inline struct StorageDevice* KeObjectToStorageDev(struct KeObject* object)
{
	return ContainerOf(KeObjectToDevice(object), struct StorageDevice, device);	
}

static inline const char* StorageDeviceGetName(struct StorageDevice* dev)
{
	return KeDeviceGetName(&dev->device);
}

int StorageDeviceInit(struct StorageDevice* dev, DevId devId);
int StorageDeviceAdd(struct StorageDevice* dev, const char* name, ...);
int StorageRemoveDevice(struct StorageDevice* dev);

struct Request* StorageGetCurrRequest(struct StorageDevice* dev);
int StorageDoRequest(struct StorageDevice* dev,struct Request* request);
int StorageQueueDestroy(struct StorageDevice* dev);
int StorageEndRequest(struct StorageDevice* device,int status);

#endif
