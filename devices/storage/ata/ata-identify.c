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

#include <llist.h>
#include <i386/pic.h>
#include <i386/pit.h>
#include <typedefs.h>
#include <i386/irq.h>
#include <malloc.h>
#include <fs/devfs.h>
#include <slab.h>
#include <request.h>
#include <timer.h>
#include <print.h>

/*** Library functions. ***/

#define AtaGetu64(p, n) \
	(((unsigned long long)(p)[(n) + 3] << 48) | \
	((unsigned long long)(p)[(n) + 2] << 32) | \
	((unsigned long long)(p)[(n) + 1] << 16) | \
	((unsigned long long)(p)[(n)]) )

static int AtaHasLba48(struct AtaIdentify* identify)
{
	WORD* p = (WORD*)identify;
	
	if ((p[83] & 0xC000) != 0x4000)
		return 0;
		
	if (!AtaGetu64(p, 100))
		return 0;
		
	return p[83] & (1 << 10);
}

static int AtaHasLba48Enabled(struct AtaIdentify* identify)
{
	WORD* p = (WORD*)identify;
	
	if (!AtaHasLba48(identify))
		return 0;

	if ((p[87] & 0xC000) !=	0x4000)
		return 0;
		
	return ((p[86]) & (1 << 10));
}

static int AtaHasLba(struct AtaIdentify* identify)
{
	WORD* p = (WORD*)identify;
	
	return (p[49] & (1 << 9));
}

static void AtaGetCapacity(struct AtaDrive* drive)
{
	if (AtaHasLba48Enabled(drive->identify))
	{
		WORD* p = (WORD*)(drive->identify);
		drive->totalSectors = AtaGetu64(p, 100);
	}else if (AtaHasLba(drive->identify))
	{
		drive->totalSectors = drive->identify->lbaCapacity;
	}else{
		KePrint("CHS: TODO\n");
	}	
}

/***********************************************************************
 *
 * FUNCTION:	AtaSwapBytes
 *
 * DESCRIPTION: Swap the bytes in a ATA description string and copy to
 *				another string.
 *
 * PARAMETERS: dest - string data is copied to.
 *			   src - string where unarranged data comes from.
 *			   count - length of string.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

static void AtaSwapBytes(char* dest,char* src,int count)
{
	char c;

	/* Make the count a multiple of 2. */
	count &= ~0x1;

	for(; count>0; count-=2, src+=2, dest+=2)
	{
		c=dest[0];
		dest[0]=src[1];
		dest[1]=c;
	}
}

/***********************************************************************
 *
 * FUNCTION:	AtaFormatString
 *
 * DESCRIPTION: Format a string from an identity block.
 *
 * PARAMETERS: dest - array to store transformed string to
 *			   src - source string (from identity block).
 *			   count - length of the string.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

static void AtaFormatString(char* dest,char* src,int count)
{
	char* c=NULL;
	int i;

	AtaSwapBytes(dest,src,count);

	for (i=0; i<count; i++)
	{
		if (dest[i] !=' ')
			c=dest+i+1;
	}

	if (c != NULL && c < dest+count)
		*c='\0';
}

/***********************************************************************
 *
 * FUNCTION:	AtaDoIdentify
 *
 * DESCRIPTION: Send off the command and wait for a response.
 *
 * PARAMETERS: drive - drive to send command to.
 *			   cmd - ATA_IDENTIFY or ATA_PIDENTIFY, command to send to
 *			   drive.
 *
 * RETURNS: 0 if success, 1 if no interface, -EIO if not present.
 *
 ***********************************************************************/

static int AtaDoIdentify(struct AtaDrive* drive,int cmd)
{
	DWORD timeout,flags;
	QWORD startTime;
	BYTE reg=0;
	int ret=0;

	uDelay(50);

	if (cmd == CMD_ATA_PIDENTIFY)
		outb(CTRL_REG(drive->parent,REG_FEATURE),0);

	outb(CTRL_REG(drive->parent,REG_COMMAND),cmd);

	timeout=((cmd == CMD_ATA_IDENTIFY) ? 3000 : 1000)/2;
	startTime=PitGetQuantums();

	SaveFlags(flags);
	sti();

	do
	{
		if ((PitGetQuantums()-startTime) > timeout)
		{
			IrqRestoreFlags(flags);
			KePrint("Identify timed out: reg = %#X\n",(DWORD)reg);
			return 2;
		}

		uDelay(50); /* Don't keep pestering the drive */

		reg=inb(CTRL_REG(drive->parent,REG_CONTROL));
	}while (reg & STATE_BUSY);

	uDelay(50);

	if ((inb(CTRL_REG(drive->parent,REG_STATUS)) & (STATE_DRQ | STATE_BUSY | STATE_ERR)) == STATE_DRQ)
		ret=0;
	else
		ret=-EIO;

	/* Now we can recieve data */

	RestoreFlags(flags);

	return ret;
}

/***********************************************************************
 *
 * FUNCTION:	AtaIdentifyDrive
 *
 * DESCRIPTION: Fills drive structure with key information about the drive.
 *
 * PARAMETERS: drive - structure to fill in.
 *			   cmd - either ATA_IDENTIFY or ATA_PIDENTIFY.
 *
 * RETURNS: Usual error codes.
 *
 ***********************************************************************/

static int AtaIdentifyDrive(struct AtaDrive* drive,int cmd)
{	
	struct AtaIdentify* info;
	WORD* p;
	int i,ret;
	DWORD flags;
	QWORD startTime;
	BYTE reg;

	info=(struct AtaIdentify*)MemAlloc(sizeof(struct AtaIdentify));
	p=(WORD*)info;

	if (!info)
		return -ENOMEM;

	uDelay(50);
	AtaSelectDrive(drive);
	uDelay(50);

	reg=inb(CTRL_REG(drive->parent,REG_SELECT));

	if (reg != ((drive->drive << 4) | LDH_DEFAULT))
	{
		/* Select drive 0 and exit */
		outb(CTRL_REG(drive->parent,REG_STATUS),LDH_DEFAULT);
		uDelay(50);
		return 2;
	}

	if (cmd == CMD_ATA_IDENTIFY && AtaWaitForState(drive->parent, STATE_BUSY | STATE_DRDY, STATE_DRDY))
		return -EIO;

	ret=AtaDoIdentify(drive,cmd);

	if (ret == -EIO)
		ret=AtaDoIdentify(drive,cmd);

	reg=inb(CTRL_REG(drive->parent,REG_STATUS));

	if (reg == (STATE_BUSY | STATE_DRDY))
		return 1;

	/* React depending on what the drive did during identification */
	if (ret == 1)
		return 1;
	else if (ret == 2 && cmd == CMD_ATA_PIDENTIFY)
	{
		KePrint("No response (%#02X) - resetting drive interface\n",inb(CTRL_REG(drive->parent,REG_COMMAND)));
		uDelay(50);
		AtaSelectDrive(drive);
		uDelay(50);
		outb(CTRL_REG(drive->parent,REG_COMMAND),ATAPI_CMD_SRST);

		startTime=PitGetQuantums();

		IrqSaveFlags(flags);
		sti();

		do
		{
			uDelay(50);
			asm("nop");
		}while ((inb(CTRL_REG(drive->parent,REG_COMMAND)) & STATE_BUSY) && (PitGetQuantums()-startTime) > 3000);

		ret=AtaDoIdentify(drive,cmd);
	}

	if (ret)
		return ret;

	IrqSaveFlags(flags);

	for (i=0; i<256; i++)
		p[i]=inw(CTRL_REG(drive->parent,REG_DATA));

	/* Clear the drive's interrupt */
	inb(CTRL_REG(drive->parent,REG_STATUS));

	IrqRestoreFlags(flags);

	AtaFormatString(info->model,info->model,40);

	if (info->caps & 0x2)
		drive->type|=ATA_LBA;
	
	if (info->dwordIo == 1)
		drive->ioFlags |= ATA_IO_DWORD;

	drive->cyls = info->cylinders;
	drive->heads = info->heads;
	drive->sectors = info->sectors;
	
	drive->identify = info;
	
	/* Get the total capacity of the drive */
	AtaGetCapacity(drive);
		
	drive->type |= (cmd == CMD_ATA_IDENTIFY) ? ATA_DISK : ATA_REMOVABLE;

	return 0;
}

int AtaTryToIdentifyDrive(struct AtaDrive* drive)
{
	int err;
	
	err=AtaIdentifyDrive(drive,CMD_ATA_IDENTIFY);

	if (err == 1) /* No interface, so don't bother probing anymore */
		return -EIO;

	if (err)
	{
		if (AtaIdentifyDrive(drive,CMD_ATA_PIDENTIFY))
			return -EIO;
	}

	/* Exit with drive 0 selected */
	outb(CTRL_REG(drive->parent,REG_SELECT),LDH_DEFAULT);
	uDelay(50);
	inb(CTRL_REG(drive->parent,REG_STATUS));

	return 0;
}
