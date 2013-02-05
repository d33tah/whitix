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

#include "ata.h"

#include <console.h>
#include <error.h>
#include <i386/ioports.h>
#include <request.h>
#include <typedefs.h>
#include <print.h>

/***********************************************************************
 *
 * FUNCTION:	AtapiSetupPacket
 *
 * DESCRIPTION: Construct the packet to send to the ATAPI device.
 *
 * PARAMETERS: packet - packet to construct information within
 *			   request - the request with information for the packet.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

static void AtapiSetupPacket(BYTE* packet,struct Request* request)
{
	DWORD sectorCount=request->dataLength/ATAPI_SECTOR_SIZE;

	ZeroMemory(packet,ATAPI_PACKET_SIZE);

	packet[0]=ATAPI_CMD_READ10;
	packet[2]=request->sector >> 24;
	packet[3]=request->sector >> 16;
	packet[4]=request->sector >> 8;
	packet[5]=request->sector & 0xFF;

	packet[7]=sectorCount >> 8;
	packet[8]=sectorCount & 0xFF;
}

/***********************************************************************
 *
 * FUNCTION:	AtapiSendPacket
 *
 * DESCRIPTION: Actually send the packet to the ATAPI device.
 *
 * PARAMETERS: controller - controller that controls the drive.
 *			   packet - the ATAPI packet.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

static void AtapiSendPacket(struct AtaController* controller,BYTE* packet)
{
	WORD* curr=(WORD*)packet;
	int i;

	for (i=0; i<6; i++)
		outw(CTRL_REG(controller,REG_DATA),curr[i]);
}

/***********************************************************************
 *
 * FUNCTION:	AtapiTransferCount
 *
 * DESCRIPTION: Return the number of bytes transferred in a phase.
 *
 * PARAMETERS:	controller - controller of the drive.
 *
 * RETURNS:		Number of bytes transferred.
 *
 ***********************************************************************/

static DWORD AtapiTransferCount(struct AtaController* controller)
{
	DWORD retVal;

	retVal=inb(CTRL_REG(controller,REG_COUNT_HI)) << 8;
	retVal|=inb(CTRL_REG(controller,REG_COUNT_LO));

	return retVal;
}

/***********************************************************************
 *
 * FUNCTION:	AtapiError
 *
 * DESCRIPTION: A common error routine for handling a I/O read failure for
 *				an ATAPI device.
 *
 * PARAMETERS:	drive - ATAPI drive structure
 *				request - current request.
 *				status - the drive's status register.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

static void AtapiError(struct AtaDrive* drive,struct Request* request,int status)
{
	if (request->retries > 5)
	{
		KePrint("ATA-CD: failed to read, error = %#X, count = %u, status = %d\n",inb(CTRL_REG(drive->parent,REG_ERROR)),
			AtapiTransferCount(drive->parent),status);
		StorageEndRequest(drive->sDev,EIO);
		return;
	}

	request->retries++;
	drive->interrupt=NULL;
	AtaRequest(drive);
}

/***********************************************************************
 *
 * FUNCTION:	AtapiRwInterrupt
 *
 * DESCRIPTION: Function to be called in the event of a sucessful I/O
 *				operation.
 *
 * PARAMETERS:	drive - ATAPI drive structure
 *				status - result of the status register read.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

static void AtapiRwInterrupt(struct AtaDrive* drive, int status)
{
	BYTE phase;
	WORD count;
	struct Request* request=StorageGetCurrRequest(drive->sDev);

	phase = status & STATE_DRQ;
	phase |= inb(CTRL_REG(drive->parent,REG_REASON)) & 0x3;

	if (status & STATE_ERR)
	{
		AtapiError(drive, request, status);
		return;
	}

	switch (phase)
	{
	case ATAPI_PHASE_ABORT:
		KePrint("Drive aborted the command.\n");
		AtapiError(drive,request,status);
		break;

	case ATAPI_PHASE_CMDOUT:
		break;

	case ATAPI_PHASE_DONE:
		if (!request->numSectors)
			goto finishedRead;

	/* Fall-through to get more data */

	case ATAPI_PHASE_DATAIN:
		count=AtapiTransferCount(drive->parent);
		
		AtaDataRead(drive, (BYTE*)request->data, count);
		
		request->data+=ATAPI_SECTOR_SIZE;
		request->sector++;
		if (!request->numSectors)
			goto finishedRead;
		else
			request->numSectors--;
		break;

	default:
		KePrint("Bad phase, %d\n",phase);
	};

	return;

finishedRead:
	AtaFinishRequest(drive, 0);
}

int AtapiReadWrite(struct Request* request, struct AtaDrive* drive)
{
	struct AtaController* control=drive->parent;
	DWORD flags;
	BYTE packet[12];
	
	if (control->needReset)
		AtaReset(control);

	IrqSaveFlags(flags);

	outb(CTRL_REG(control,REG_LDH),(drive->drive << 4) | LDH_DEFAULT);

	if (AtaWaitForState(control,STATE_BUSY,0))
	{
		IrqRestoreFlags(flags);
		return -EIO;
	}
	
	outb(CTRL_REG(control,REG_FEATURE),0);
	outb(CTRL_REG(control,REG_COUNT),0);
	outb(CTRL_REG(control,REG_SECTOR),0);

	outb(CTRL_REG(control,REG_CYL_LO),ATAPI_READ_SIZE & 0xFF);
	outb(CTRL_REG(control,REG_CYL_HI),ATAPI_READ_SIZE >> 8);

	outb(CTRL_REG(control,REG_COMMAND),CMD_ATAPI_PACKET);

	if (AtaWaitForState(control,STATE_BUSY | STATE_DRQ,STATE_DRQ))
	{
		IrqRestoreFlags(flags);
		return -EIO;
	}

	drive->interrupt=AtapiRwInterrupt;

	AtapiSetupPacket(packet,request);
	AtapiSendPacket(control,packet);

	IrqRestoreFlags(flags);

//	printf("AtapiRead(%u)\n", request->sector);

	return -SIOPENDING;
}
