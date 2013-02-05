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

/* TODO: Support multi-write and multi-read */

#include <llist.h>
#include <i386/pic.h>
#include <i386/pit.h>
#include <typedefs.h>
#include <i386/irq.h>
#include <module.h>
#include <malloc.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <slab.h>
#include <request.h>
#include <timer.h>
#include <print.h>

#include "ata.h"

struct AtaBase ataBases[]={
	{0x1F0,14},
	{0x170,15},
	{0x1E8,11},
	{0x168,10},
};

struct Partition
{
	DWORD startSector;
	DWORD secLen; /* Length of partition in sectors */
	struct StorageDevice* sDev,*partDev;
};

/* Globals. */
struct KeSet ataSet;

KE_OBJECT_TYPE_SIMPLE(ataType, NULL);

/***********************************************************************
 *
 * FUNCTION:	AtaDataRead
 *
 * DESCRIPTION: Read data from the drive controller.
 *
 * PARAMETERS:	controller - controller of the drive.
 *				buffer - buffer to transfer data into.
 *				count - number of bytes available.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void AtaDataRead(struct AtaDrive* drive, BYTE* buffer, int count)
{
	if (!count)
	{
		KePrint(KERN_ERROR "AtaDataRead: no bytes to read.\n");
		return;
	}
	
	if (drive->ioFlags & ATA_IO_DWORD)
		insd(CTRL_REG(drive->parent, REG_DATA), buffer, count >> 2);
	else
		insw(CTRL_REG(drive->parent,REG_DATA), buffer, count >> 1);
}

/***********************************************************************
 *
 * FUNCTION:	AtaWaitForState
 *
 * DESCRIPTION: Wait for the controller to change to a wanted state, and
 *				return an error if it times out.
 *
 * PARAMETERS:	controller - structure containing controller information.
 *				mask - mask for controller register
 *				value - wanted value
 *
 * RETURNS:		0 on success, -EIO on failure
 *
 ***********************************************************************/

/* What to do if errors crop up? */
int AtaWaitForState(struct AtaController* controller,int mask,int value)
{
	QWORD startTime;
	BYTE status;
	DWORD flags;

	IrqSaveFlags(flags);
	sti();

	for (startTime=PitGetQuantums(); PitGetQuantums()-startTime<100;)
	{
		status=inb(CTRL_REG(controller,REG_CONTROL));
		if ((status & mask) == value)
		{
			IrqRestoreFlags(flags);
			return 0;
		}
	}

//	printf("WaitForState: status = %u, mask = %#X, value = %d, status & mask = %#X, i = %d\n", status, mask, value, status & mask);

	controller->needReset=1;
	IrqRestoreFlags(flags);
	return -EIO;
}

/***********************************************************************
 *
 * FUNCTION:	AtaSendCommand
 *
 * DESCRIPTION: Send an ATA command to the drive (through the controller).
 *
 * PARAMETERS:	drive - drive to send command to.
 *				command - command to be sent.
 *
 * RETURNS:		0 on success, -EIO on failure.
 *
 ***********************************************************************/

int AtaSendCommand(struct AtaDrive* drive,struct AtaCommand* command)
{
	DWORD sector=0,head=0,secsPerCyl=0,cylLo=0,cylHi=0,cylinder=0;
	DWORD flags;
	struct AtaController* controller=drive->parent;

	if (drive->type & ATA_LBA)
	{
		/* Drive handles conversion to CHS */
		command->ldh|=0x40;
		sector=(command->block) & 0xFF;
		cylLo=(command->block >> 8) & 0xFF;
		cylHi=(command->block >> 16) & 0xFF;
		command->ldh|=(command->block >> 24) & 0xF;
	}else{
		secsPerCyl=drive->heads*drive->sectors;
		cylinder=command->block / secsPerCyl;
		head=(command->block % secsPerCyl) / drive->sectors;
		sector=(command->block % drive->sectors)+1;
		cylLo=cylinder;
		cylHi=cylinder >> 8;
		command->ldh|=head;
	}

	if (controller->needReset)
		AtaReset(controller);
	
	if (AtaWaitForState(controller,STATE_BUSY,0))
		return -EIO;

	IrqSaveFlags(flags);

	outb(CTRL_REG(controller,REG_LDH),command->ldh);

	/* Wait for DRDY (drive ready) to be set to 1 */
	if (AtaWaitForState(controller,STATE_BUSY | STATE_DRDY,STATE_DRDY))
		return -EIO;

	outb(CTRL_REG(controller,REG_CONTROL),CTL_EIGHTHEADS);
	outb(CTRL_REG(controller,REG_PRECOMP),0);
	outb(CTRL_REG(controller,REG_COUNT),1);
	outb(CTRL_REG(controller,REG_SECTOR),sector);
	outb(CTRL_REG(controller,REG_CYL_LO),cylLo);
	outb(CTRL_REG(controller,REG_CYL_HI),cylHi);

	outb(CTRL_REG(controller,REG_COMMAND),command->command);

	IrqRestoreFlags(flags);

	return 0;
}

struct AtaDrive* AtaChooseDrive(struct AtaController* controller)
{
	struct AtaDrive* drive, *best = NULL;
	
	ListForEachEntry(drive, &controller->drives, next)
	{		
		if (StorageGetCurrRequest(drive->sDev) /* This drive has pending requests. */
			/* Has this drive slept for longer than the current best choice? */
			&& (!best || (best && (PitGetQuantums() - drive->sleepTime) > 
				(PitGetQuantums() - best->sleepTime))
			))
		{
			best = drive;
		}
	}
	
	controller->currDrive = best;
	
	return best;
}

int AtaControllerRequest(struct AtaController* controller)
{
	struct AtaDrive* drive;
	struct Request* request;
	int err;
	
	if (controller->busy > 0)
		return -SIOPENDING;
	
	controller->busy = 1;
	
	drive = AtaChooseDrive(controller);

	/* No drives have requests pending. */
	if (!drive)
	{
		controller->busy = 0;
		return 0;
	}
	 	
	request = StorageGetCurrRequest(drive->sDev);

	if (drive->type & ATA_DISK)
		err = AtaReadWrite(request, drive);
	else
		err = AtapiReadWrite(request, drive);
		
	/* Set up the timer, which will expire if we happen to miss an interrupt
	 * on the controller.
	 */
	 
	 if (err == -SIOPENDING)
	 {
	 	controller->timer.expires = 1000; /* 1 second? */
	 	
//	 	if (!ListEmpty(&controller->timer.list))
//	 		KernelPanic("Timer already attached?\n");
	 	
	 	TimerAdd(&controller->timer);
	 }
	 
	 return err;
}

int AtaFinishRequest(struct AtaDrive* drive, int status)
{
	struct AtaController* controller = drive->parent;

	StorageEndRequest(drive->sDev, status);	

	controller->busy = 0;
	drive->interrupt = NULL;
	drive->sleepTime = PitGetQuantums();

	/* Remove the timer. We'll set it up again once we've issued a command to
	 * the drive.
	 */

	TimerRemove(&controller->timer);
	
	/* See if there any pending requests that another drive was waiting for. */
	AtaControllerRequest(controller);

	return status;
}

int AtaRequest(void* data)
{
	struct AtaDrive* drive=(struct AtaDrive*)data;
	
	return AtaControllerRequest(drive->parent);
}

void AtaControllerExpiry(void* data)
{
	struct AtaController* controller = (struct AtaController*)data;
	
	KePrint("data = %#X\n", data);
	
	if (!controller->busy)
	{
		KePrint("AtaControllerExpiry(), %d, %#X\n", controller->busy, VirtToPhys(&controller->busy));
		cli(); hlt();
		return;
	}
	
	KePrint("needReset = %#X, %#X\n", controller->needReset, controller->currDrive);
	
	cli(); hlt();
	
//	TimerRemove(&controller->timer);
	
	DWORD flags;
	IrqSaveFlags(flags);
	
//	KePrint("AtaController: missed interrupt?\n");
//	cli(); hlt();
	
	controller->busy = 0;
		
	AtaControllerRequest(controller);
	
	IrqRestoreFlags(flags);
}

/***********************************************************************
 *
 * FUNCTION:	AtaWaitNotBusy
 *
 * DESCRIPTION: Wait for a controller to unbusy itself
 *
 * PARAMETERS:	controller - controller to wait for.
 *				timeOut - number of seconds*100 to wait for.
 *
 * RETURNS:		-EIO or -EBUSY if hardware fault, 0 on success.
 *
 ***********************************************************************/

int AtaWaitNotBusy(struct AtaController* controller, int timeOut)
{
	BYTE reg;
	unsigned int startTime = PitGetQuantums(), currTime = 0;
	
	timeOut *= 100;

	while (currTime < timeOut)
	{
		reg=inb(CTRL_REG(controller,REG_STATUS));
		if (!(reg & STATE_BUSY))
			return 0;

		if (reg == 0xFF)
			return -EIO;
			
		currTime = PitGetQuantums() - startTime;
	}

	return -EBUSY;
}

/***********************************************************************
 *
 * FUNCTION:	AtaReset
 *
 * DESCRIPTION: Reset the controller at 'index' and wait for it to be
 *				ready.
 *
 * PARAMETERS:	controller - structure containing controller information.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

int AtaReset(struct AtaController* controller)
{
	int err;

	/* Reset the controller */
	outb(CTRL_REG(controller,REG_CONTROL),CTL_RESET | CTL_EIGHTHEADS);

	/* Needs a small delay loop */
	uDelay(10);

	outb(CTRL_REG(controller,REG_CONTROL),CTL_EIGHTHEADS);
	
	uDelay(10);

	/* Wait at most 35 (!) seconds for it to unbusy itself */
	err=AtaWaitNotBusy(controller, 35);
	if (err)
		return err;

	/* Select the first drive (manually) */
	outb(CTRL_REG(controller,REG_SELECT),LDH_DEFAULT);

	outb(CTRL_REG(controller,REG_CONTROL),CTL_EIGHTHEADS);
	
	uDelay(10);
	
	AtaWaitNotBusy(controller, 10);

	outb(CTRL_REG(controller,REG_SELECT),LDH_DEFAULT | 1 << 4);
	outb(CTRL_REG(controller,REG_CONTROL),CTL_EIGHTHEADS);
	
	uDelay(10);
	
	AtaWaitNotBusy(controller, 10);

	/* And end with drive 0 being selected */
	outb(CTRL_REG(controller,REG_SELECT),LDH_DEFAULT);

	controller->needReset=0;

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:	AtaInterrupt
 *
 * DESCRIPTION: Handle a general ATA interrupt, sending it off to the
 *				correct subsystem.
 *
 * PARAMETERS:	data - controller interrupt was sent to.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

int AtaInterrupt(void* data)
{
	struct AtaController* controller=(struct AtaController*)data;
	struct AtaDrive* drive=controller->currDrive;
	int status;

	/* It may be a spurious interrupt */
	if (!drive || !controller)
		return 1;

	status=inb(CTRL_REG(controller,REG_STATUS));

	/* The read of the status register clears the interrupt */
	if (drive->interrupt)
		drive->interrupt(drive,status);

	return 0;
}

/* Fairly hacky. TODO: Replace with a better data structure. */

static int AtaPartitionRequest(void* data)
{
	struct Partition* part=(struct Partition*)data;
	struct StorageDevice* partDev=part->partDev;
	struct Request* request=StorageGetCurrRequest(partDev);
	struct Request newRequest;
	struct Buffer newBuffer;
	int err;

	if (!request)
		return 0;
	
	do
	{
		if (request->sector > part->secLen)
			break;
	
		memcpy(&newRequest, request, sizeof(struct Request));

		newRequest.sector += part->startSector;
	
		memcpy(&newBuffer, newRequest.buffer, sizeof(struct Buffer));
		INIT_WAITQUEUE_HEAD(&newBuffer.waitQueue);
		newRequest.buffer = &newBuffer;
		newRequest.buffer->flags = (1 << BUFFER_LOCKED);
	
		err = BlockSendRequestRaw(part->sDev, &newRequest);

		StorageEndRequest(partDev, -err);
	
	} while (( request = StorageGetCurrRequest(partDev) ));
	
	return 0;
}

static int cdRomIndex=0, hdIndex=0;

static int AtaAddPartition(int i,int base,struct StorageDevice* parent,struct AtaPartition* part)
{
	struct Partition* newPart;
	struct StorageDevice* sDevice;

	newPart=(struct Partition*)MemAlloc(sizeof(struct Partition));

	if (!newPart)
		return -ENOMEM;

	sDevice=(struct StorageDevice*)MemAlloc(sizeof(struct StorageDevice));

	if (!sDevice)
	{
		MemFree(newPart);
		return -ENOMEM;
	}

	newPart->startSector=part->startSectorAbs;
	newPart->secLen=part->sectorCount;
	newPart->sDev=parent;
	newPart->partDev=sDevice;

	sDevice->request=AtaPartitionRequest;
	sDevice->blockSize=sDevice->softBlockSize=parent->blockSize;
	sDevice->priv=newPart;
	sDevice->totalSectors=newPart->secLen;

	StorageDeviceInit(sDevice, DEV_ID_MAKE(4, ((base*64)+i+1)));	
	StorageDeviceAdd(sDevice, "AtaDisk%d%c", base, 'a'+i);
	
	KePrint("%s: %#X, %u, %u\n", KeDeviceGetName(&sDevice->device),
		(DWORD)part[i].system,part[i].startSectorAbs,part[i].sectorCount);

	return 0;
}

static int AtaPartitionDevice(struct AtaDrive* drive, int base)
{
	/* Generalize function to include all hard-drive device partitions (i.e not just ATA) */
	struct StorageDevice* device=drive->sDev;
	struct Buffer* buff=BlockRead(device,0);
	BYTE* data=buff->data;
	struct AtaPartition* part=(struct AtaPartition*)(data+0x1BE);
	int i;

	if (!buff)
		return -EIO;

	if (data[510] != 0x55 || data[511] != 0xAA)
		return -EINVAL;

	/* Now investigate the partitions */
	for (i=0; i<4; i++)
		if (part[i].system)
			AtaAddPartition(i,base,device,part);

	return 0;
}

#define ATA_MAJOR	4

static void AtaRegisterDrive(struct AtaDrive* drive)
{
	struct StorageDevice* sDevice;
	char* format;
	int index;	

	sDevice=(struct StorageDevice*)MemAlloc(sizeof(struct StorageDevice));
	drive->sDev=sDevice;
	sDevice->request=AtaRequest;
	sDevice->blockSize=(drive->type & ATA_REMOVABLE) ? 2048 : 512;
	sDevice->priv=drive;
	
	sDevice->totalSectors = drive->totalSectors;

	/* Fill out hardware information */
	if (drive->type & ATA_DISK)
	{
		sDevice->cylinders=drive->cyls;
		sDevice->heads=drive->heads;
		sDevice->sectors=drive->sectors;
		
		format = "AtaDisk%u";
		index = hdIndex;
	}else{
		sDevice->cylinders=sDevice->heads=sDevice->sectors=0;
		format = "AtaCd%u";
		index = cdRomIndex++;
	}

	/* And add the device */
	StorageDeviceInit(sDevice, DEV_ID_MAKE(ATA_MAJOR, 0));
	
	StorageDeviceAdd(sDevice, format, index);

	KePrint("ATA: Found %s\n", StorageDeviceGetName(sDevice));

	if (drive->type & ATA_DISK)
	{
		AtaPartitionDevice(drive, hdIndex);
		hdIndex++;
	}
}

int AtaInit()
{
	int i, j, err;
	struct AtaController* controller;
	struct AtaDrive* drive;
	DWORD flags;

	/* Create the ATA device set. */
//	err = StoralgeSetCreate(&ataSet, &ataType, "Ata");
	
//	if (err)
//		return err;

	for (i=0; i<2; i++)
	{		
		controller=(struct AtaController*)MemAlloc(sizeof(struct AtaController));
		
		/* Set up the controller data structure. */
		controller->index=i;
		INIT_LIST_HEAD(&controller->drives);
		controller->timer.func = AtaControllerExpiry;
		controller->timer.data = controller;
		controller->timer.expires = 0;
		INIT_LIST_HEAD(&controller->timer.list);
				
		AtaReset(controller);
		
		/* Register the interrupts and detect the drives */
		IrqAdd(ataBases[i].irq, AtaInterrupt, controller);

		SaveFlags(flags);
		sti();

		for (j=0; j<2; j++)
		{
			drive=(struct AtaDrive*)MemAlloc(sizeof(struct AtaDrive));
			controller->currDrive=drive;
			drive->parent=controller;
			drive->drive=j;
			drive->sleepTime = PitGetQuantums();

			AtaCtrlDisableInts(controller);
			err=AtaTryToIdentifyDrive(drive);
			AtaCtrlEnableInts(controller);
			
			if (err)
			{
				controller->currDrive=NULL;
				MemFree(drive);
				continue;
			}

			/* Register it as a valid drive with the controller. */
			ListAddTail(&drive->next, &controller->drives);

			AtaRegisterDrive(drive);
		}

		RestoreFlags(flags);
	}

	return 0;
}

ModuleInit(AtaInit);
