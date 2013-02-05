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

/* TODO: Needs to be tightened up and rewritten in parts. sprintf'ing on user buffers
 * is not a good idea. */

/* In the future, code that wants to use virtual terminals will do SysMakeDir
 * in Bus/Virtual. For now, use attributes on a VirtualMaster device. */
#include <console.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <module.h>
#include <error.h>
#include <malloc.h>
#include <i386/ioports.h>
#include <task.h>
#include <print.h>
#include <devices/device.h>
#include <devices/class.h>
#include <console.h>

#define VIRTUAL_MAX		10
#define VIRTUAL_BUFFER_SIZE		4096 /* TEST */

/* At most there will be two digits after Virtual, plus the null terminator. */
#define VIRTUAL_NAME_SIZE	sizeof("Virtual")+3
#define VIRTUAL_MASTER_INVALID (-1)
#define VIRTUAL_FILE_MASTER	1
#define VIRTUAL_FILE_SLAVE	2

/* Device number */
#define VIRTUAL_MAJOR	255

/* TEMP */
extern struct DevClass consoleClass;

struct VirtualMaster
{
	struct KeDevice device;
};

struct VirtualTerm
{
	struct KeDevice device;
	
	int masterFlag;
	int flags;
	int slaveCount;
	char* masterOutput;
	int masterHead, masterTail;
	WaitQueue masterQueue;
	char* slavesOutput;
	int slaveHead, slaveTail;
	WaitQueue slaveQueue;
};

KE_OBJECT_TYPE(vMasterType, NULL, NULL, KEDEV_OBJ_OFFSET(struct VirtualMaster, device));
KE_OBJECT_TYPE(vTermType, NULL, NULL, KEDEV_OBJ_OFFSET(struct VirtualTerm, device));

struct VirtualMaster vMaster;

struct FileOps vChildOps;
struct VirtualTerm virtualT[VIRTUAL_MAX];

static inline int VirtualIsMaster(struct File* file)
{
	return ((int)file->filePriv == VIRTUAL_FILE_MASTER);
}

struct VirtualTerm* VirtualTermGet(struct File* file)
{
	struct KeDevice* device;
	
	device = DevFsGetDevice(file->vNode);
	
	if (!device)
		return NULL;
		
	return ContainerOf(device, struct VirtualTerm, device);
}

int VirtualChildOpen(struct File* file)
{
	struct VirtualTerm* term;
	
	term = VirtualTermGet(file);
	
	if (!term)
		return -EINVAL;
	
	if (term->masterFlag == VIRTUAL_MASTER_INVALID)
	{
		term->masterFlag = VIRTUAL_FILE_MASTER; /* Now taken. */
		file->filePriv = (void*)VIRTUAL_FILE_MASTER;
		term->masterOutput=(char*)MemAlloc(VIRTUAL_BUFFER_SIZE);
		term->slavesOutput=(char*)MemAlloc(VIRTUAL_BUFFER_SIZE);
		term->masterHead = term->masterTail = 0;
		term->slaveHead = term->slaveTail = 0;

		INIT_WAITQUEUE_HEAD(&term->masterQueue);
		INIT_WAITQUEUE_HEAD(&term->slaveQueue);
	}else{
		file->filePriv = (void*)VIRTUAL_FILE_SLAVE;
		term->slaveCount++;
	}
		
	return 0;
}

int VirtualChildRead(struct File* file, BYTE* data, DWORD length)
{
	struct VirtualTerm* vTerm;
	DWORD size=0, oldSize;
	DWORD flags;

	vTerm=VirtualTermGet(file);
	
	if (!vTerm)
		return -ENOENT;
		
//	KePrint("%s: VirtualChildRead(%#X)\n", current->name, length);
			
	if (VirtualIsMaster(file))
	{
		WAIT_ON(&vTerm->slaveQueue, vTerm->slaveHead != vTerm->slaveTail);

		IrqSaveFlags(flags);
		size=MIN((DWORD)(vTerm->slaveHead-vTerm->slaveTail), length);
		oldSize=size;

		while (size--)
		{
			*data++=vTerm->slavesOutput[vTerm->slaveTail++];
			vTerm->slaveTail &= VIRTUAL_BUFFER_SIZE-1;
		}
	}else{
		WAIT_ON(&vTerm->masterQueue, vTerm->masterHead != vTerm->masterTail);

		IrqSaveFlags(flags);
		size=MIN((DWORD)(vTerm->masterHead-vTerm->masterTail), length);
		oldSize=size;

		while (size--)
		{
			*data++=vTerm->masterOutput[vTerm->masterTail++];
			vTerm->masterTail &= VIRTUAL_BUFFER_SIZE-1;
		}
	}

	IrqRestoreFlags(flags);

	return oldSize;
}

int VirtualChildWrite(struct File* file, BYTE* data, DWORD length)
{
	struct VirtualTerm* vTerm;
	unsigned int tempLength;
	vTerm=VirtualTermGet(file);

	if (!vTerm)
		return -ENOENT;

	PreemptDisable();
	
	tempLength=length;
	
//	KePrint("%s: VirtualChildWrite(%s, %d), %#X\n", current->name, data, length, VirtualIsMaster(file));
		
	if (VirtualIsMaster(file))
	{
		/* TODO: Limit length */
		
		memcpy(&vTerm->masterOutput[vTerm->masterHead], data, length);
		
		vTerm->masterHead+=length;
		vTerm->masterHead &= VIRTUAL_BUFFER_SIZE-1;

		PreemptEnable();

		WakeUp(&vTerm->masterQueue);
	}else{	
		while (length--)
		{
			vTerm->slavesOutput[vTerm->slaveHead++]=*data++;
			vTerm->slaveHead &= VIRTUAL_BUFFER_SIZE-1;
		}
		
		PreemptEnable();

		WakeUp(&vTerm->slaveQueue);
	}

	return tempLength;
}

int VirtualChildPoll(struct File* file, struct PollItem* item, struct PollQueue* pollQueue)
{
	struct VirtualTerm* vTerm;

	vTerm = VirtualTermGet(file);
	
	if (VirtualIsMaster(file))
	{
		if (vTerm->slaveHead != vTerm->slaveTail)
			item->revents |= POLL_IN;

		if (vTerm->masterTail != ((vTerm->masterHead - 1) % VIRTUAL_BUFFER_SIZE))
			item->revents |= POLL_OUT;
			
		if (vTerm->slaveCount <= 0)
			item->revents |= POLL_ERR;

		PollAddWait(pollQueue, &vTerm->slaveQueue);
	}else{
		if (vTerm->masterHead != vTerm->masterTail)
			item->revents |= POLL_IN;
			
		if (vTerm->slaveTail != ((vTerm->slaveHead - 1) %
			VIRTUAL_BUFFER_SIZE))
			item->revents |= POLL_OUT;
			
		if (vTerm->masterFlag == VIRTUAL_MASTER_INVALID)
			item->revents |= POLL_ERR;

		PollAddWait(pollQueue, &vTerm->masterQueue);
	}

	return 0;
}

int VirtualChildClose(struct File* file)
{
	struct VirtualTerm* vTerm;
	
	vTerm = VirtualTermGet(file);
	
	if (VirtualIsMaster(file))
	{
		vTerm->masterFlag = VIRTUAL_MASTER_INVALID;
	}else{
		vTerm->slaveCount--;
	}
	
	if (vTerm->masterFlag == VIRTUAL_MASTER_INVALID && vTerm->slaveCount == 0)
	{
		/* FIXME: Free terminal. */
	}
	
	return 0;
}

struct FileOps vChildOps=
{
	.read = VirtualChildRead,
	.write = VirtualChildWrite,
	.poll = VirtualChildPoll,
	.open = VirtualChildOpen,
	.close = VirtualChildClose,
};

int VirtualNewTerminal(BYTE* data, unsigned long size, DWORD position)
{
	int i, total, err;
	struct VirtualTerm* term;
	
	if (size < VIRTUAL_NAME_SIZE)
		return -EFAULT;
		
	/* Can only do one read to find out the terminal name. */
	if (position != 0)
		return 0;

	PreemptDisable();
	
	/* Find a free virtual terminal. */
	for (i = 0; i < VIRTUAL_MAX; i++)
		if (!virtualT[i].masterFlag)
			break;
			
	if (i == VIRTUAL_MAX)
	{
		PreemptEnable();
		return -EMFILE;
	}
	
	term = &virtualT[i];
	total = snprintf(data, VIRTUAL_NAME_SIZE, "Virtual%d", i);
	
	/* Create the master device */
	KeDeviceInit(&term->device, &consoleClass.set, DEV_ID_MAKE(0, 0), &vChildOps, DEVICE_CHAR);
	err = KeDeviceAttach(&term->device, "Virtual%d", i);
	
	if (err)
	{
		PreemptEnable();
		return err;
	}
	
	term->masterFlag = VIRTUAL_MASTER_INVALID;
	term->flags = 0;
	term->slaveCount = 0;

	/* TODO: Change to ICFS attributes with a common Console structure. */
	struct KeFsEntry* dir=KeDeviceGetConfDir(&term->device);
	IcFsAddIntEntry(dir, "flags", &term->flags, VFS_ATTR_RW);
	
	PreemptEnable();
	
	return total;
}

/* FIXME: Check if Close can handle removing a terminal. */
int VirtualRemoveTerminal(BYTE* data, unsigned long size, DWORD position)
{
	KePrint("VirtualRemoveTerminal\n");
	return 0;
}

int VirtualInit()
{
	ZeroMemory(virtualT, VIRTUAL_MAX * sizeof(struct VirtualTerm));
	
	KeDeviceInit(&vMaster.device, &consoleClass.set, DEV_ID_MAKE(0, 0), NULL, DEVICE_CHAR);
	KeDeviceAttach(&vMaster.device, "VirtualMaster");
	
	/* Register the master's configuration options. TODO: Change to ICFS attributes. */
	struct KeFsEntry* dir=KeDeviceGetConfDir(&vMaster.device);
	
	/* newTerminal returns a string to the reader with the name of the new terminal.
	 * It doesn't have a write function. */
	IcFsAddFuncEntry(dir, "newTerminal", VirtualNewTerminal, NULL);
	IcFsAddFuncEntry(dir, "delTerminal", NULL, VirtualRemoveTerminal);
	
	return 0;
}

ModuleInit(VirtualInit);
