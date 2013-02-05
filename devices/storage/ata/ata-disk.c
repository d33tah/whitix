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
#include <typedefs.h>
#include <print.h>

static void AtaRwInterrupt(struct AtaDrive* drive,int status)
{
	struct Request* request=StorageGetCurrRequest(drive->sDev);
	BYTE* p=(BYTE*)request->data;

	if (!request)
		return;

	if ( (status & STATE_ERR) || (status & STATE_DF))
	{
		KePrint("AtaRwInterrupt: failure reading sector %u\n",request->sector);
		/* Failure? If so, do it again */
		AtaRequest(drive);
		return;
	}

	if (request->type == REQUEST_READ)
	{
		if (AtaWaitForState(drive->parent,STATE_BUSY | STATE_DRQ, STATE_DRQ))
		{
			KePrint("Timed out waiting for read\n");
			AtaRequest(drive);
			return;
		}
		
		AtaDataRead(drive, p, 512);
	}

	/* I/O completed ok */
	request->data += 512;
	++request->sector;
	--request->numSectors;

	if (request->numSectors > 0)
		AtaReadWrite(request, drive);
	else
		AtaFinishRequest(drive, 0);
}

int AtaReadWrite(struct Request* request, struct AtaDrive* drive)
{
	struct AtaCommand command;
	int err,i;
	WORD* p=(WORD*)request->data;

	/* If the request is out of bounds, notify the request code and process
 	 * another request. */
	if (request->sector > drive->totalSectors)
		return AtaFinishRequest(drive, -EIO);
	
	command.driveNum=drive->drive;
	command.ldh=(command.driveNum << 4) | LDH_DEFAULT;
	command.command=(request->type == REQUEST_WRITE ? CMD_ATA_WRITE : CMD_ATA_READ);
	command.block=request->sector;
	command.count=1; /* For now */

	drive->interrupt = AtaRwInterrupt;
	err=AtaSendCommand(drive,&command);

	if (err)
		return AtaFinishRequest(drive, -EIO);

	if (request->type == REQUEST_WRITE)
	{
		if (AtaWaitForState(drive->parent,STATE_BUSY | STATE_DRQ,STATE_DRQ))
			return AtaFinishRequest(drive, -EIO);
		
		for (i=0; i<256; i++)
			outw(CTRL_REG(drive->parent,REG_DATA),p[i]);
	}

	return -SIOPENDING;
}
