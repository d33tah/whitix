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

#ifndef ATA_H
#define ATA_H

#include <request.h>
#include <devices/sdevice.h>
#include <i386/ioports.h>
#include <timer.h>
#include <typedefs.h>

/* Disk types */

#define ATA_DISK 1
#define ATA_REMOVABLE 2
#define ATA_LBA 0x80

/* ATA/ATAPI commands */

#define CMD_ATA_READ		0x20
#define CMD_ATA_WRITE		0x30
#define CMD_ATAPI_PACKET	0xA0
#define CMD_ATA_IDENTIFY	0xEC
#define CMD_ATA_PIDENTIFY	0xA1

/* ATA/ATAPI registers */
#define REG_DATA	0
#define REG_PRECOMP 1
#define REG_ERROR	1
#define REG_FEATURE	1
#define REG_COUNT	2
#define REG_REASON	2
#define REG_SECTOR	3
#define REG_CYL_LO	4
#define REG_CYL_HI	5
#define REG_COUNT_LO REG_CYL_LO
#define REG_COUNT_HI REG_CYL_HI
#define REG_LDH		6
#define REG_SELECT	6
#define LDH_DEFAULT 0xA0

#define REG_CONTROL 0x206
	#define CTL_EIGHTHEADS 0x08
	#define CTL_RESET 0x04
	#define CTL_DISABLEINT 0x02

#define REG_STATUS 0x7

#define STATE_BUSY 0x80
#define STATE_DRDY 0x40
#define STATE_DF 0x20
#define STATE_DRQ 0x08
#define STATE_ERR 0x01

#define REG_COMMAND 0x7

#define CTRL_REG(controller,reg) (ataBases[(controller)->index].base+(reg))

/* ATAPI specific commands */

#define ATAPI_CMD_SRST		0x08
#define ATAPI_CMD_READ10	0x28
#define ATAPI_READ_SIZE		32768
#define ATAPI_SECTOR_SIZE	2048
#define ATAPI_PACKET_SIZE	12

/* ATAPI phases */

#define ATAPI_PHASE_ABORT	0
#define ATAPI_PHASE_DONE	3
#define ATAPI_PHASE_DATAOUT	8
#define ATAPI_PHASE_CMDOUT	9
#define ATAPI_PHASE_DATAIN	10

/* ATA Identify structure */

struct AtaIdentify
{
	WORD config;
	WORD cylinders;
	WORD reserved;
	WORD heads;
	WORD trackBytes;
	WORD secBytes;
	WORD sectors;
	WORD vendor[3];
	BYTE serial[20];
	WORD bufType;
	WORD bufSize;
	WORD eccBytes;
	BYTE firmWareRev[8];
	char model[40];
	BYTE maxMultiSect;
	BYTE vendor3;
	WORD dwordIo;
	BYTE vendor4;
	BYTE caps;
	WORD reserved2;
	BYTE vendor5;
	BYTE tPIO;
	BYTE vendor6;
	BYTE tDMA;
	WORD fieldValid;
	WORD curCyls;
	WORD curHeads;
	WORD curSectors;
	WORD currCapacity0,currCapacity1;
	BYTE multiSect;
	BYTE multiSecValid;
	DWORD lbaCapacity;
	WORD dma1Word;
	WORD dmaMWord;
	WORD idePioModes;
	WORD ideDmaMin;
	WORD ideDmaTime;
	WORD idePioIoRdy;
	WORD moreReserved[10];
	
	WORD theRest[186];
}PACKED;

/* General ATA/ATAPI structures */

#define ATA_IO_DWORD	0x01

struct AtaDrive
{
	DWORD heads,cyls,sectors;
	unsigned long long totalSectors;
	struct AtaIdentify* identify;
	int drive; /* Either 0 or 1 */
	struct StorageDevice* sDev;
	DWORD sleepTime;
	void (*interrupt)(struct AtaDrive* drive,int status);
	struct AtaController* parent;
	struct ListHead next;
	int type;
	
	/* I/O flags */
	int ioFlags;
};

struct AtaController
{
	struct AtaDrive* currDrive;
	struct ListHead drives;
	int index;
	BYTE needReset;
	struct Timer timer;
	int busy;
};

struct AtaPartition
{
	BYTE boot,startHead,startSector,startCyl,system,endHead,endSector,endCyl;
	DWORD startSectorAbs;
	DWORD sectorCount;
}PACKED;

struct AtaCommand
{
	BYTE driveNum,ldh,command,precomp,count;
	int block;
};

struct AtaBase
{
	DWORD base,irq;
};

extern struct AtaBase ataBases[];

/* General utility functions */
#define AtaCtrlDisableInts(controller) outb(CTRL_REG(controller,REG_CONTROL),inb(CTRL_REG(controller,REG_CONTROL)) | CTL_DISABLEINT)
#define AtaCtrlEnableInts(controller) outb(CTRL_REG(controller,REG_CONTROL),CTL_EIGHTHEADS)
#define AtaSelectDrive(theDrive) outb(CTRL_REG((theDrive)->parent,REG_LDH),(((theDrive)->drive << 4) | LDH_DEFAULT))

int AtaRequest(void* data);
int AtaWaitForState(struct AtaController* controller,int mask,int value);
int AtaSendCommand(struct AtaDrive* drive,struct AtaCommand* command);
int AtaReset(struct AtaController* controller);
int AtaTryToIdentifyDrive(struct AtaDrive* drive);
int AtaFinishRequest(struct AtaDrive* drive, int status);

void AtaDataRead(struct AtaDrive* drive, BYTE* buffer, int count);

/* Drive-specific functions */
int AtaReadWrite(struct Request* request, struct AtaDrive* drive);
int AtapiReadWrite(struct Request* request, struct AtaDrive* drive);

#endif
