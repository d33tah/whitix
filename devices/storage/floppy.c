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

#include <delay.h>
#include <error.h>
#include <console.h>
#include <i386/ioports.h>
#include <i386/idt.h>
#include <i386/pic.h>
#include <i386/pit.h>
#include <devices/sdevice.h>
#include <typedefs.h>
#include <fs/devfs.h>
#include <i386/physical.h>
#include <i386/irq.h>
#include <module.h>
#include <request.h>
#include <i386/virtual.h>
#include <string.h>
#include <timer.h>
#include <print.h>

/* 
 * A driver for your normal floppy drive. Only supports one floppy drive (and controller) at the moment,
 * since that's all that's needed for most people. The infrastructure for a second floppy drive is there though 
 *
 * TODO: Add support for multiple controllers.
 */

/* Floppy commands */

#define CMD_SPECIFY 0x03
#define CMD_RECAL 0x07
#define CMD_SENSEI 0x08
#define CMD_VERSION 0x10
#define CMD_DUMPREGS 0x0E
#define CMD_SEEK 0x0F
#define CMD_WRITE 0xC5
#define CMD_READ 0xE6

/* Floppy data registers */

#define FDC_DOR 0x3F2
#define FDC_MSR 0x3F4
#define FDC_DRS 0x3F4
#define FDC_DATA 0x3F5
#define FDC_DIR 0x3F7

/* Status register 0 (sr0 or st0) bits */
#define SEEK_SUCCESS 0x20
#define ERROR_MASK 0xC0

#define DG_144_GAP 0x1B

struct Fdc
{
	struct StorageDevice device;
	
	BYTE status[7];
	BYTE sr0,currTrack,dio,busy,mrq,motor,reset,isBusy;
	int idle,doReset; /* idle = able to shift a new request to the fdc */
	/* For a transfer */
	int rwHead,rwTrack,rwSector,rwReadWrite;
	void (*handler)(struct Fdc* fdc);
	void (*onHandler)(struct Fdc* fdc); /* Has to be a better way */
	struct Timer onTimer;
	BYTE* buffer; /* Physical address */
	BYTE* virt; /* Virtual address */
};

#define HANDLE_RESET(fdc) do {if ((fdc)->doReset) FdcHwReset(fdc);} while(0)

struct Fdc flop;

static BYTE FdcGetByte(struct Fdc* fdc);
static void FdcSendByte(struct Fdc* fdc,BYTE byte);
static void FdcRecal(struct Fdc* fdc);
static void FdcDoSeek();
static int FdcDoRequest(void* data);
static void FdcActivateMotor(struct Fdc* fdc,void (*onHandler)(struct Fdc* fdc));
//static void FdcDeactivateMotor(struct Floppy* floppy);
static void FdcHwReset(struct Fdc* fdc);
 
struct StorageDevice floppyDev={
	.request=FdcDoRequest,
	.blockSize=512,
	.heads=2,
	.cylinders=80,
	.sectors=18,
	.totalSectors=(2*80*18),
	.priv=&flop, /* Is a parameter to the Request function */
};

/***********************************************************************
 *
 * FUNCTION:	FdcDmaTransfer
 *
 * DESCRIPTION: Set up a DMA transfer from memory to disk or vice-versa.
 *
 * PARAMETERS: read - is the transfer from disk-to-memory?
 *			   trackBuffer - physical location to read/write.
 *			   length - length of buffer.
 *
 * RETURNS: 0 if successful, -EIO if request could not be serviced.
 *
 ***********************************************************************/

/* More of a utility function */
static void FdcDmaTransfer(int read,BYTE* trackBuffer,DWORD length)
{
	BYTE page = ((DWORD)trackBuffer) >> 16;
	WORD offset = ((DWORD)trackBuffer) & 0xFFFF;
	DWORD flags;

	IrqSaveFlags(flags);
	--length;

	outb(0x0A,2 | 4); /* Disable channel */
	outb(0x0C,0); /* Clear flip-flop */
	outb(0x0B,(read ? 0x44 : 0x48) + 2); /* Set control byte to read (or write) */
	outb(0x81,page); /* Set DMA page */	

	/* Set DMA offset */
	outb(0x04,offset & 0xFF); 
	outb(0x04,offset >> 8);

	/* Set length of transfer - always real length minus 1 */
	outb(0x05,length & 0xFF);
	outb(0x05,length >> 8);

	/* Enable channel again */
	outb(0x0A,2); 
	
	IrqRestoreFlags(flags);
}

static int FdcGetStatuses(struct Fdc* fdc)
{
	int statusIndex=0;
	int i;
	int status;

	if (fdc->doReset)
		return -1;

	for (i=0; i<10000; i++)
	{
		status=inb(FDC_MSR) & 0xD0;
		if (status == 0x80)
			return statusIndex;

		if (status == 0xD0)
		{
			if (statusIndex >= 7)
				break;

			fdc->status[statusIndex++]=inb(FDC_DATA);
		}
	}

	fdc->reset=1;
	return -1;
}

static void FdcDoSenseInterrupt(struct Fdc* fdc)
{
	FdcSendByte(fdc,CMD_SENSEI);
	fdc->sr0=FdcGetByte(fdc);
	fdc->currTrack=FdcGetByte(fdc);
}

static void FdcResetInterrupt(struct Fdc* fdc)
{
	int i;
	for (i=0; i<4; i++)
		FdcDoSenseInterrupt(fdc);

	/* Now recalibrate the floppy to make sure everything is in working order */
	/* Must issue a specify command it seems */
	FdcSendByte(fdc,0x03);
	FdcSendByte(fdc,0xDF); /* Step rate */
	FdcSendByte(fdc,6);

	FdcActivateMotor(fdc,FdcRecal);
}

static void FdcRecalInterrupt(struct Fdc* fdc)
{
	FdcDoSenseInterrupt(fdc);

	if ((fdc->sr0 & 0xE0) == 0x60)
	{
		FdcHwReset(fdc);
		return;
	}

	if (fdc->sr0 & 0x10)
	{
		KePrint("Recal failed, trying again\n");
		FdcRecal(fdc);
		return;
	}

	fdc->isBusy=0;

	/* Now ready to accept any I/O requests */
	FdcDoRequest(fdc);
}

static int FdcIrq(void* data)
{
	struct Fdc* fdc=(struct Fdc*)data;

	if (!(fdc->handler))
	{
		KePrint("floppy: No handler for interrupt, %#X\n",fdc->busy);
		return 0;
	}

	(*fdc->handler)(fdc);
	return 0;
}

static void FdcRwInterrupt(struct Fdc* fdc)
{
	/* No sensing of interrupt on an I/O interrupt */
	int err=(fdc->status[0] & 0xC0) >> 6;
	int num=FdcGetStatuses(fdc);
	struct Request* currRequest=StorageGetCurrRequest(&floppyDev);

	if (UNLIKELY(err))
	{
		KePrint("Floppy: I/O error, starting again\n");

		KePrint("Floppy registers: \n");
		int i;
		for (i=0; i<num; i++)
			KePrint("%d: %#X\n",i,(DWORD)fdc->status[i]);

		KePrint("err = %d,num = %d\n",err,num);

		currRequest->retries++;

		if (currRequest->retries >= 4)
		{
			/* Request failed */
			StorageEndRequest(&floppyDev,EIO);
			return;
		}
		if (err == 1)
		{
			/* Error occured during command execution */
			if (fdc->status[1] & 0x2)
			{
				/* Write protected */
				KePrint("Floppy is write-protected\n");
			}else if (fdc->status[1] & 0x4)
			{
				KePrint("No data. Unreadable\n");
				KePrint("Floppy thinks chs is %d,%d,%d\n",fdc->status[3],fdc->status[4],fdc->status[5]);
			}
		}

		/* Error, just go back, seek, and do it again */
		fdc->doReset=1;
	}else{
		/* Successful, end the request */

		/* Copy over the data from buffer */
		if (currRequest->type == REQUEST_READ)
			memcpy((char*)currRequest->data,(void*)fdc->virt,512);

		--currRequest->numSectors;
		++currRequest->sector;
		currRequest->data+=512;

		/*
		 * Once the floppy has succeeded with the I/O request, indicate so to the storage layer so another request
		 * can be handled
		 */

		if (!currRequest->numSectors)
			StorageEndRequest(&floppyDev,0);
			/* Now schedule the motor off timer, which will be cancelled if any requests come through */

		fdc->handler=NULL; /* Deal with any stray interrupts */
	}

	fdc->isBusy=0;
	FdcDoRequest(fdc);
}

static void FdcDiskChange(struct Fdc* fdc)
{
	/* Disk change */
	if (inb(FDC_DIR) & 0x80)
		/* Force a seek, and then we'll find out whether a disk now exists or not */
		fdc->currTrack=-1;
}

static int FdcDoRwTransfer(struct Fdc* fdc)
{
	FdcDmaTransfer(fdc->rwReadWrite == REQUEST_READ,fdc->buffer,512);
	fdc->handler=FdcRwInterrupt;

	FdcSendByte(fdc,(fdc->rwReadWrite == REQUEST_READ ? CMD_READ : CMD_WRITE));
	FdcSendByte(fdc,fdc->rwHead << 2);
	FdcSendByte(fdc,fdc->rwTrack);
	FdcSendByte(fdc,fdc->rwHead);
	FdcSendByte(fdc,fdc->rwSector);
	FdcSendByte(fdc,2);
	FdcSendByte(fdc,18);
	FdcSendByte(fdc,DG_144_GAP);
	FdcSendByte(fdc,0xFF);

	HANDLE_RESET(fdc);

	return 0;
}

static void FdcSeekInterrupt(struct Fdc* fdc)
{
	FdcDoSenseInterrupt(fdc);
	struct Request* request=StorageGetCurrRequest(&floppyDev);

	if ((fdc->sr0 & 0xF8) != SEEK_SUCCESS ||  fdc->currTrack != fdc->rwTrack)
	{
		KePrint("Floppy seek failed\n");

		/* If a seek is not possible, a disk may not even be in the drive */
		if (request)
			StorageEndRequest(&floppyDev,EIO);
	}else{
		fdc->currTrack=fdc->rwTrack;
		FdcDoRwTransfer(fdc);
	}
}

/* 
 * Seek to the appropriate track for a data transfer. If it's already at that track, just call the FdcTransfer function
 */

static void FdcDoSeek(struct Fdc* fdc)
{
	if (fdc->currTrack != fdc->rwTrack)
	{
		fdc->handler=FdcSeekInterrupt;
		FdcSendByte(fdc,CMD_SEEK);
		FdcSendByte(fdc,fdc->rwHead << 2);
		FdcSendByte(fdc,fdc->rwTrack);
	}else
		FdcDoRwTransfer(fdc);

	HANDLE_RESET(fdc);
}

/* Well, it's a callback */

static void FdcOnInterrupt(void* data)
{
	struct Fdc* fdc=(struct Fdc*)data;

	/* Select current drive here */
	fdc->motor=1;
	fdc->onHandler(data);
}

/* Only called in event of a read/write request */

static void FdcActivateMotor(struct Fdc* fdc,void (*onHandler)(struct Fdc* fdc))
{
	fdc->onHandler=onHandler;

	if (fdc->motor)
	{
		FdcOnInterrupt(fdc);
		return;
	}

	outb(FDC_DOR,0x1C);
	fdc->onTimer.expires=300; /* 300 ms */
	fdc->onTimer.data=fdc;
	fdc->onTimer.func=FdcOnInterrupt;
	TimerAdd(&fdc->onTimer);
}

#if 0

/* Work this out */

static void FdcDeactivateMotor(struct Fdc* fdc)
{
	if (!fdc->motor)
		return;
		
	TimerRemove(&fdc->onTimer);
	fdc->offTimer.expires=3000; /* 10 seconds */
	fdc->offTimer.data=fdc;
	fdc->offTimer.func=FdcOffInterrupt;
	TimerAdd(&fdc->offTimer);
}

#endif

static void FdcGetStatus(struct Fdc* fdc)
{
	BYTE msr=inb(FDC_MSR);
 
 	fdc->mrq=(msr & 0x80);
 	fdc->dio=(msr & 0x40);
 	fdc->busy=(msr & 0x10);
}

static int FdcMrq(struct Fdc* fdc)
{
	FdcGetStatus(fdc);
	return fdc->mrq;
}

static void FdcSendByte(struct Fdc* fdc,BYTE byte)
{
	while (!FdcMrq(fdc));

	int i;

	for (i=0; i<0x10000; i++)
	{
		FdcGetStatus(fdc);
		if (fdc->mrq && !fdc->dio)
		{
			outb(FDC_DATA,byte);
			return;
		}
		
		/* A delaying thing */
		inb(0x80);
	}

	fdc->doReset=1;
}

static BYTE FdcGetByte(struct Fdc* fdc)
{
	int i;
	for (i=0; i<0x10000; i++)
	{
		FdcGetStatus(fdc);
		if (fdc->mrq && fdc->dio)
			return inb(FDC_DATA);

		/* A delaying thing */
		inb(0x80);
	}

	fdc->doReset=1;
	return 0xFF;
}

static void FdcHwReset(struct Fdc* fdc)
{
	DWORD flags;
	IrqSaveFlags(flags);

	KePrint("FLOP: Resetting floppy\n");

	fdc->handler=FdcResetInterrupt;
	fdc->motor=0;

	outb(FDC_DOR,0x0C & ~0x04); /* Turn the floppy motor off */
	outb(FDC_DOR,0); /* Initiate a software reset */

	uDelay(20);

	outb(FDC_DOR,0x0C);
	fdc->doReset=0;
	IrqRestoreFlags(flags);
}

static void FdcRecal(struct Fdc* fdc)
{
	fdc->handler=FdcRecalInterrupt;
	FdcSendByte(fdc,CMD_RECAL);
	FdcSendByte(fdc,fdc->rwHead << 2); /* rwHead usually is 0 (at the beginning anyway) */
	return;
}

/* Should be more general? */

static void LbaToChs(int block,int* head,int* track,int* sector)
{
	*head=(block % (18 * 2)) / 18;
	*track=block / (18*2);
	*sector=block % 18 + 1;
}

/*
 * DoFdRequest
 * -----------
 */

static int FdcDoRequest(void* data)
{
	/* Might be called from interrupt, so no sleeping, and request can be null */
	struct Fdc* fdc=(struct Fdc*)data;
	struct Request* request=StorageGetCurrRequest(&floppyDev);
	DWORD flags;

	if (!request)
		return 0; /* No requests pending */

	if (request->sector >= 2880)
		return -EIO;

	/* If it's busy, there's no point */
	if (fdc->isBusy)
		return -SIOPENDING;

	IrqSaveFlags(flags);
	fdc->isBusy=1;
	IrqRestoreFlags(flags);

	LbaToChs(request->sector,&fdc->rwHead,&fdc->rwTrack,&fdc->rwSector);
	fdc->rwReadWrite=request->type;

	if (request->type == REQUEST_WRITE)
		memcpy((char*)fdc->virt,(char*)request->data,512);

	/* Check there is a disk in there before I/O, or if it's changed */
	FdcDiskChange(fdc);

	FdcActivateMotor(fdc,FdcDoSeek);
	return -SIOPENDING; /* Zero requests get fufilled immediately in this floppy driver */
}

static int FdcGetConfig()
{
	BYTE c;

	/* Retrieve floppy data from CMOS */
	outb(0x70,0x10);

	/* High nibble contains first floppy data */
	c=(inb(0x71) >> 4);

	/* If c is 0, there is no floppy */
	return (c == 0);
}

void FdcRelease(struct KeObject* object)
{
}

KE_OBJECT_TYPE_SIMPLE(floppyObjType, FdcRelease);

#define FLOPPY_MAJOR		5

struct KeSet floppySet;

int FdcRegisterDevice()
{
#if 0
	int err;
	
	/* Create a device type for floppies */
	err = KeSetCreate(&floppySet, NULL, &floppyObjType, "Floppy");
	
	if (err)
		return err;
	
	err = StorageDeviceCreate(&floppyDev, DEV_ID_MAKE(FLOPPY_MAJOR, 0));
	
	if (err)
		return err;
	
	return StorageAddDevice(&floppyDev, "Floppy0");
#endif
	return 0;
}

int FdcInit()
{
	DWORD page;
	
	/* Inspect CMOS data for basic floppy information. */
	if (FdcGetConfig())
		return 0; /* No problem if a floppy drive does not exist. */

	ZeroMemory(&flop,sizeof(struct Fdc));
	flop.isBusy=1;
	FdcHwReset(&flop);

	FdcSendByte(&flop, CMD_DUMPREGS);

	/* If it doesn't respond to DUMPREGS, there's no floppy drive here. */
	if (flop.doReset)
		return 0;

	/* Clear out the result registers. */
	if (FdcGetStatuses(&flop) < 0)
		return 0;

	IrqAdd(6,FdcIrq,&flop);

	page = PageAllocLow()->physAddr;
	if (!page)
		return -ENOMEM;

	flop.buffer=(BYTE*)page;
	flop.currTrack=255; /* Force a recalibration. */

	/* Map a (not-so-temporary) buffer */
	flop.virt=(BYTE*)VirtAllocateTemp((DWORD)flop.buffer);
	if (!flop.virt)
		return -ENOMEM;

	return FdcRegisterDevice();
}

/*int FdcExit()
{
	StorageQueueDestroy(&floppyDev);
}*/

ModuleInit(FdcInit);
