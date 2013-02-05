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

#include <fs/devfs.h>
#include <fs/vfs.h>
#include <wait.h>
#include <task.h>
#include <i386/irq.h>
#include <i386/process.h>
#include <print.h>
#include <devices/input.h>
#include <llist.h>
#include <module.h>
#include <ps2.h>

/* Function defines for use in this file */
static void KeyboardAckScancode();
static int KeyboardIrq();

/* Should all the translating be in userspace? (i.e. should there be a raw option?)
   Windows and X11 do that already. It can't be in userspace until everything
   runs graphically */

#define EXT_BASE 0x80

/* Keypad keys */
#define EXT_UP EXT_BASE+1
#define EXT_DOWN EXT_BASE+2
#define EXT_LEFT EXT_BASE+3
#define EXT_RIGHT EXT_BASE+4

/* Escape key */
#define EXT_ESC EXT_BASE+5

/* Keyboard commands */
#define KBD_CMD_ENABLE	0xF4
#define KBD_CMD_DISABLE 0xAD
#define KBD_CMD_ENABLE_EX 0xAE

/* Ioctl defines */
#define KBD_IO_SETLEDS 0x01

static BYTE scanCodes[]={
	0,EXT_ESC,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t', /* 0x00 - 0x0F */
	'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s', /* 0x10 - 0x1F */
	'd','f','g','h','j','k','l',';','\'',0,0,'#','z','x','c','v', /* 0x20 - 0x2F */
	'b','n','m',',','.','/',0,0,0,' ',0,0,0,0,0,0, /* 0x30 - 0x3F */
	0,0,0,0,0,0,0,'7','8','9',0,'4','5','6',0,'1', /* 0x40 - 0x4F */
	'2','3',0,0,0,0,'\\','4','5','6','7','8','9',0,0,0, /* 0x50 - 0x5F */
};

static BYTE shiftScanCodes[]={
	0,0,'!','"',0xA3,'$','%','^','&','*','(',')','_','+','\b', /* 0x00 - 0x0F */
	0,'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A', /* 0x10 - 0x1F */
	'S','D','F','G','H','J','K','L',':','@',0,0,'~',
	'Z','X','C','V','B','N','M','<','>','?',0,
	0,0,
	' ',
	0,0,0,0,0,0,0,0,0,0, /* Function keys */
	0,0,0,0,0,0,0,0,0,0,0,'1','2','3',0,0,'4','5','|','7','8','9',
};

/* Special characters such as the arrow keys */

static char e0ScanCodes[]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x00 - 0x0F */
	0,0,0,0,0,0,0,0,0,0,0,0,'\n',0,0,0, /* 0x10 - 0x1F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x20 - 0x2F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x30 - 0x3F */
	0,0,0,0,0,0,0,0,EXT_UP,0,0,EXT_LEFT,0,EXT_RIGHT,0,0, /* 0x40 - 0x4F */
	EXT_DOWN,0,0,0x7F,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x50 - 0x5F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x60 - 0x6F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x70 - 0x7F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80 - 0x8F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int isShift=0;
int isCaps=0;
int isCtrl=0;

static void KeyboardAckScancode()
{
	BYTE a=inb(0x61);
	outb(0x61,a | 0x80);
	outb(0x61,a & 0x7F);
}

BYTE prevScancode=0;

static int KeyboardIrq(void* data)
{
	Ps2SendSimpleCmd(KBD_CMD_DISABLE);
	KeyboardAckScancode();
	Ps2Wait();
	BYTE key=inb(0x60);

	ConsoleAddKeyCode(key);

	if (prevScancode == 0xE0)
	{
		prevScancode=0;

		/* An extended key */
		if (key <= sizeof(e0ScanCodes) && e0ScanCodes[key])
			ConsoleAddKey(e0ScanCodes[key]);
			
		goto end;
	}

	switch (key)
	{
	case 0x1D:
		isCtrl=1;
		break;

	/* Shift keys */
	case 0x2A:
	case 0x36:
		isShift=true;
		break;

	/* Caps lock */
	case 0x3A:
		isCaps=isCaps ^ true;
		break;

	case 0x9D:
		isCtrl=0;
		break;

	/* Shift keys - key up */
	case 0xAA:
	case 0xB6:
		isShift=false;
		break;

	case 0xE0:
		prevScancode=key;
		KeyboardAckScancode();
		break;

	default:
		if (key & 0x80)
			break;

		if (isShift)
		{
			if (key >= 0x3B && key <= 0x3B+MAX_CONSOLES)
			{
				int index=key-0x3B;
				ChangeConsole(index);
			} else if (isCaps)
			{
				if((key >= 0x10 && key <= 0x19)		/* Q - P */
					|| (key >= 0x1E && key <= 0x26)	/* A - L */
					|| (key >= 0x2C && key <= 0x32))/* Z - M */
				{
					ConsoleAddKey(scanCodes[key]);
				} else {
					ConsoleAddKey(shiftScanCodes[key]);
				}
			}else{
				if (isCtrl)
					ConsoleAddKey(0x8F);
				ConsoleAddKey(shiftScanCodes[key]);
			}
		}
		else if (isCaps)
		{
			if (isCtrl)
				ConsoleAddKey(0x8F);
			
			if((key >= 0x10 && key <= 0x19)		/* Q - P */
				|| (key >= 0x1E && key <= 0x26)	/* A - L */
				|| (key >= 0x2C && key <= 0x32))/* Z - M */
			{
				ConsoleAddKey(shiftScanCodes[key]);
			} else {
				ConsoleAddKey(scanCodes[key]);
			}
		}else{
			if (isCtrl)
				ConsoleAddKey(0x8F);

			ConsoleAddKey(scanCodes[key]);
		}
	}

end:
	Ps2SendSimpleCmd(KBD_CMD_ENABLE_EX); /* Enable keyboard */
	return 0;
}

static struct InputDevice* keyBoardDev;

int KeyboardInit()
{
	IrqAdd(1,KeyboardIrq,NULL);
	KeyboardAckScancode();

	/* Do self test here */
	
	/* Clear the buffer. */
	Ps2SendSimpleCmd(KBD_CMD_ENABLE);

	KePrint(KERN_INFO "PS2: Found PS/2 keyboard\n");

	keyBoardDev = InputDeviceAdd("Keyboard", NULL);

	struct KeFsEntry* dir=KeDeviceGetConfDir(&keyBoardDev->device);
	
	IcFsAddArrayEntry(dir, "keyMap", (char*)scanCodes, sizeof(scanCodes), 
		sizeof(scanCodes), sizeof(scanCodes));
	IcFsAddArrayEntry(dir, "shiftKeyMap", (char*)shiftScanCodes,
		sizeof(shiftScanCodes), sizeof(shiftScanCodes), sizeof(shiftScanCodes)); 
	
	return 0;
}

ModuleInit(KeyboardInit);
