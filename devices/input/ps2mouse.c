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

#include <console.h>
#include <error.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <i386/ioports.h>
#include <i386/irq.h>
#include <typedefs.h>
#include <ps2.h>
#include <task.h>
#include <wait.h>
#include <print.h>
#include <devices/input.h>

#ifdef MODULE
#include <module.h>
#endif

#define PS2MOUSE_CMD_SETSCALE11			0xE6
#define PS2MOUSE_CMD_SETRES				0xE8
#define PS2MOUSE_CMD_GETID				0xF2
#define	PS2MOUSE_CMD_SETRATE			0xF3
#define PS2MOUSE_CMD_ENABLE				0xF4
#define PS2MOUSE_CMD_RESET_DISABLE		0xF5

#define INPUT_MAJOR		5

static void Ps2MouseActivate();
static void Ps2MouseDeactivate();

static BYTE mouseBytes[3];
static int currByte=0;
static WaitQueue mouseQueue;
static int mouseHead;
static int mouseTail;

struct Ps2MousePacket
{
	BYTE data[3];
}PACKED;

#define MOUSE_QUEUE_SIZE		100

static struct Ps2MousePacket packetQueue[MOUSE_QUEUE_SIZE];

static int Ps2TryRead(BYTE* data)
{
	DWORD flags;

	if (mouseHead != mouseTail)
	{
		IrqSaveFlags(flags);
		memcpy(data, &packetQueue[mouseTail], sizeof(struct Ps2MousePacket));
		mouseTail=(mouseTail+1) % MOUSE_QUEUE_SIZE;
		IrqRestoreFlags(flags);
		return 0;
	}else
		return -EIO;
}

static int Ps2MouseInterrupt(void* data)
{
	int head;
	
	/* Should be three bytes to read. */
	mouseBytes[currByte++] = inb(0x60);

	if (currByte == 3)
	{
		head=(mouseHead+1) % MOUSE_QUEUE_SIZE;

		if (head != mouseTail)
		{
			/* Add to data queue. */
			memcpy(&packetQueue[head], mouseBytes, sizeof(struct Ps2MousePacket));
			mouseHead=head;
			WakeUp(&mouseQueue);
		}

		currByte=0;
	}
	
	return 0;
}

#define MouseSendSimpleCmd(cmd) (Ps2SendCommand((cmd), NULL, 0, PS2_CMD_MOUSE))

static int Ps2Open(struct File* file)
{
	unsigned char param;

	/* Being opened for the first time means we should enable and configure the device. */
	if (file->vNode->refs == 1)
	{
		Ps2MouseActivate();
		IrqAdd(12, Ps2MouseInterrupt, NULL);

		param=3 | (1 << 6);
		/* Enable interrupt 1 and 12 (keyboard and mouse). */
		Ps2SendCommand(PS2_CMD_WRITECMD, &param, 1, PS2_CMD_INPUT);
	}

	return 0;
}

static int Ps2Close(struct File* file)
{
	/* Being closed for the last time means we should disable the device. */
	if (file->vNode->refs == 1)
	{
		KePrint("Close mouse\n");
		IrqRemove(12, Ps2MouseInterrupt, NULL);
	}

	return 0;
}

/* TODO: Convert to generic mouse data format, not a PS/2-specific one. */
static int Ps2Read(struct File* file, BYTE* data, DWORD size)
{
	DWORD origSize=size;

	if ((file->flags & FILE_NONBLOCK) && mouseHead == mouseTail)
		return -SIOPENDING; /* TODO: Fix. */

	while (size >= sizeof(struct Ps2MousePacket))
	{
		WAIT_ON(&mouseQueue, mouseHead != mouseTail);

		while (Ps2TryRead(data));

		size-=sizeof(struct Ps2MousePacket);
		data+=sizeof(struct Ps2MousePacket);
	}
	
	return (origSize-size);
}

static int Ps2Poll(struct File* file, struct PollItem* item, struct PollQueue* pollQueue)
{
	PollAddWait(pollQueue, &mouseQueue);
	
	if (mouseHead != mouseTail)
		item->revents |= POLL_IN;

	return 0;
}

static void Ps2MouseActivate()
{
	unsigned char params;

	MouseSendSimpleCmd(PS2MOUSE_CMD_ENABLE);

	/* Set the report rate. */
	params=100; /* 100 reports per second. */
	Ps2SendCommand(PS2MOUSE_CMD_SETRATE, &params, 1, PS2_CMD_MOUSE | PS2_CMD_INPUT);

	/* Set the resolution. */
	params=3;
	Ps2SendCommand(PS2MOUSE_CMD_SETRES, &params, 1, PS2_CMD_MOUSE | PS2_CMD_INPUT);

	MouseSendSimpleCmd(PS2MOUSE_CMD_SETSCALE11);
}

static void Ps2MouseDeactivate()
{
	/* Reset and disable the mouse. We'll enable it when a process opens the
	 * /Input/Ps2Mouse file. */
	MouseSendSimpleCmd(PS2MOUSE_CMD_RESET_DISABLE);
}

static int Ps2MouseProbe()
{
	unsigned char params[2];
	DWORD flags;
	int err=0;

	IrqSaveFlags(flags);

	Ps2GetData();

	/* Probe for the presence of a mouse. */
	if (Ps2SendCommand(PS2MOUSE_CMD_GETID, params, 1, PS2_CMD_MOUSE | PS2_CMD_OUTPUT))
	{
		err=-EIO;
		goto out;
	}
	
	if (params[0] != 0x00 && params[0] != 0x03 && params[0] != 0x04 &&
		params[0] != 0xFF)
	{
		err=-EIO;
		goto out;
	}

	Ps2MouseDeactivate();

out:
	IrqRestoreFlags(flags);
	return err;
}

struct FileOps mouseOps={
	.open = Ps2Open,
	.close = Ps2Close,
	.read = Ps2Read,
	.poll = Ps2Poll,
};

static struct InputDevice* mouseDev;

static int Ps2MouseInit()
{
	/* Not an error if a mouse doesn't exist. (Otherwise we end up with
 	 * moduleadd complaining). */
	if (Ps2MouseProbe())
	{
		KePrint("PS2: No PS/2 mouse present\n");
		return 0;
	}
	
	INIT_WAITQUEUE_HEAD(&mouseQueue);

	KePrint("PS2: Found PS/2 mouse\n");

	mouseDev = InputDeviceAdd("Ps2Mouse", &mouseOps);
	
	return 0;
}

ModuleInit(Ps2MouseInit);
