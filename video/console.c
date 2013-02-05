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
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <error.h>
#include <malloc.h>
#include <module.h>
#include <i386/ioports.h>
#include <task.h>
#include <fs/icfs.h>
#include <print.h>
#include <panic.h>
#include <keobject.h>
#include <devices/device.h>
#include <devices/class.h>

/* Device number */
#define CONSOLE_MAJOR 1

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

/* Read settings from a kernel command line soon */

#define CONSOLE_WIDTH 	80
#define CONSOLE_BWIDTH	(CONSOLE_WIDTH*2)
#define CONSOLE_HEIGHT 	25
#define CONSOLE_BHEIGHT	(CONSOLE_HEIGHT*2)
#define CONSOLE_SIZE	(CONSOLE_WIDTH * CONSOLE_HEIGHT *2)	/* Console size in bytes. */
#define TAB_WIDTH	8
#define CLEAR		((0x07 << 8) | (' '))

#define CONSOLE_BUF(i)	(scrBuf+(CONSOLE_SIZE*(i)))
#define CONSOLE_IND(i,j) ((j)*CONSOLE_BWIDTH + (i))

static int rows = CONSOLE_HEIGHT, cols = CONSOLE_WIDTH;

static void ConsoleUpdateCursorPos(int console);

static BYTE* screen=(BYTE*)0xC00B8000; /* VGA memory. */
static BYTE* scrBuf;int currConsole=0;
int numTextConsoles = MAX_CONSOLES;
struct Console consoles[MAX_CONSOLES];
Spinlock consoleLock;

/* Keymaps. */

BYTE scanCodes[]={
	0,EXT_ESC,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t', /* 0x00 - 0x0F */
	'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s', /* 0x10 - 0x1F */
	'd','f','g','h','j','k','l',';','\'',0,0,'#','z','x','c','v', /* 0x20 - 0x2F */
	'b','n','m',',','.','/',0,0,0,' ',0,0,0,0,0,0, /* 0x30 - 0x3F */
	0,0,0,0,0,0,0,'7','8','9',0,'4','5','6',0,'1', /* 0x40 - 0x4F */
	'2','3',0,0,0,0,'\\','4','5','6','7','8','9',0,0,0, /* 0x50 - 0x5F */
};

SYMBOL_EXPORT(scanCodes);

BYTE shiftScanCodes[]={
	0,0,'!','"',0xA3,'$','%','^','&','*','(',')','_','+','\b', /* 0x00 - 0x0F */
	0,'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A', /* 0x10 - 0x1F */
	'S','D','F','G','H','J','K','L',':','@',0,0,'~',
	'Z','X','C','V','B','N','M','<','>','?',0,
	0,0,
	' ',
	0,0,0,0,0,0,0,0,0,0, /* Function keys */
	0,0,0,0,0,0,0,0,0,0,0,'1','2','3',0,0,'4','5','|','7','8','9',
};

SYMBOL_EXPORT(shiftScanCodes);

/* Special characters such as the arrow keys */

char e0ScanCodes[]={
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

SYMBOL_EXPORT(e0ScanCodes);

static void ConsoleNewLine();
static void ConsoleUpdateCursorPos();
static void ConsoleClearScreen();
static void ConsoleWrite(char c,int index);
static int ConsoleWriteString(int console,char* str,int length);

int ChangeConsole(int index)
{
	if (consoles[index].flags & CONSOLE_GRAPHICS || (currConsole == index))
		return 0;

	SpinLockIrq(&consoleLock);

	/* Multiple screens are set up during the boot sequence. */
	if (!scrBuf || index < 0 || index >= MAX_CONSOLES)
		goto error;

	/* Copy current vga screen memory to scrBuf[currConsole] */
	memcpy(CONSOLE_BUF(currConsole),screen,CONSOLE_SIZE);

	currConsole=index;

	/* And copy scrBuf[currConsole] into VGA screen memory */
	memcpy(screen,CONSOLE_BUF(currConsole), CONSOLE_SIZE);

	ConsoleUpdateCursorPos(currConsole);

error:
	SpinUnlockIrq(&consoleLock);
	return 0;
}

SYMBOL_EXPORT(ChangeConsole);

static void ConsoleScrollScreen(int index)
{
	int i;

//	if (consoles[index].flags & CONSOLE_GRAPHICS)
	//	return;

	PreemptDisable();
 
	if (currConsole == index)
	{
		for (i=0; i < CONSOLE_HEIGHT-1 ; i++)
			memcpy ( &screen [ CONSOLE_IND(0,i) ] , &screen [ CONSOLE_IND(0,i+1) ] , CONSOLE_BWIDTH);
		
		/* Clear the last line of the console. */
		memsetw( & screen[ CONSOLE_IND(0,i) ] , CONSOLE_WIDTH, CLEAR);
	}else{
		for (i=0; i<CONSOLE_HEIGHT-1; i++)
			memcpy ( & CONSOLE_BUF(index) [ CONSOLE_IND(0,i) ] ,
				& CONSOLE_BUF(index) [ CONSOLE_IND(0,i) ], CONSOLE_BWIDTH);
			
		/* Clear last line */
		memsetw( & CONSOLE_BUF(index)[ CONSOLE_IND(0,i) ] , CONSOLE_WIDTH, CLEAR);
	}

	PreemptEnable();
}

void putc(int c)
{
	ConsoleWrite(c,currConsole);
}

unsigned char colorTable[] = {0, 4, 2, 6, 1, 5, 3, 7};

/* TODO: Should we use currCons? */

int ConsoleWriteString(int console, char* str, int length)
{
	struct Console* currCons=&consoles[console];
	int oldLength=length;

//	if (currCons->flags & CONSOLE_GRAPHICS)
//		return 0;

	while (length--)
	{
		if (*str == 0x1B)
		{
			currCons->inCtrl=1;
			goto end;
		}

		if (currCons->inCtrl)
		{
			if (*str == '[')
			{
				currCons->ctrlParams=1;
				ZeroMemory(currCons->params, sizeof(unsigned int)*16);
				currCons->paramIndex=0;
				goto end;
			}
			
			if (currCons->ctrlParams)
			{
				if (*str == ';')
				{
					++currCons->paramIndex;
					goto end;
				}else if (*str < '0' || *str > '9') /* Command */
				{
					currCons->cmdLetter=1;
					currCons->ctrlParams=0;
				}else{
					currCons->params[currCons->paramIndex]*=10;
					currCons->params[currCons->paramIndex]+=(*str)-'0';
					goto end;
				}
			}

			/* VT100 control characters are handled here. */
			if (currCons->cmdLetter)
			{
				switch (*str)
				{
					case 'C':
						currCons->currX+=currCons->params[0];
						ConsoleUpdateCursorPos(console);
						break;
						
					case 'D':
						currCons->currX-=currCons->params[0];
						ConsoleUpdateCursorPos(console);
						break;
						
					/* 'J'. Clear the screen. */
					case 'J':
						ConsoleClearScreen(console);
						break;

					/* 'H'. Move the cursor to the specified position */
					case 'H':
						/* First parameter is row, second is column. */

						/* Bound the given parameters. */
						if (currCons->params[0] >= CONSOLE_HEIGHT)
							currCons->params[0]=CONSOLE_HEIGHT;

						if (currCons->params[1] >= CONSOLE_WIDTH)
							currCons->params[1]=CONSOLE_WIDTH;

						/* Update the cursor position. */
						currCons->currY = currCons->params[0];
						currCons->currX = currCons->params[1];
						ConsoleUpdateCursorPos(console);

						break;

					/* 'K'. Erase characters from console. */
					case 'K':
					{
						switch (currCons->params[0])
						{

						/* From current cursor position to end of line */
						case 0:
							if (currConsole == console)
								memsetw((WORD*)(screen+(currCons->currY*(CONSOLE_WIDTH*2))+(currCons->currX*2)),CONSOLE_WIDTH-currCons->currX,(currCons->currColor << 8) | ' ');
							else
								memsetw((WORD*)(scrBuf+(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*console)+(currCons->currY*(CONSOLE_WIDTH*2))+currCons->currX),CONSOLE_WIDTH-currCons->currX,(currCons->currColor << 8) | ' ');
							break;

						/* From start of line to current cursor position */
						case 1:
							if (currConsole == console)
								memsetw((WORD*)(screen+(currCons->currY*(CONSOLE_WIDTH*2))), currCons->currX,(currCons->currColor << 8) | ' ');
							else
								memsetw((WORD*)(scrBuf+(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*console)+(currCons->currY*(CONSOLE_WIDTH*2))), currCons->currX ,(currCons->currColor << 8) | ' ');
							break;

						case 2:
							/* Erase whole of current line */
							if (currConsole == console)
								memsetw((WORD*)(screen+(currCons->currY*(CONSOLE_WIDTH*2))),CONSOLE_WIDTH,(currCons->currColor << 8) | ' ');
							else
								memsetw((WORD*)(scrBuf+(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*console)+(currCons->currY*(CONSOLE_WIDTH*2))),CONSOLE_WIDTH,(currCons->currColor << 8) | ' ');
							break;
						}
						break;
					}

					/* 'm'. Set graphic rendition. */
					case 'm':
						switch (currCons->params[0])
						{
							/* Reset the color */
							case 0:
								currCons->currColor=7;
								break;
								
							case 1:
							case 2:
								currCons->currColor ^= 0x08;
								break;
								
							/* Invert the color */
							case 7:
								currCons->currColor=(currCons->currColor & 0x88) | (((currCons->currColor >> 4) | (currCons->currColor << 4)) & 0x77);								
								break;

							case 30 ... 37:		
								currCons->foreColor = colorTable[currCons->params[0] % 10];
								currCons->currColor = currCons->foreColor | (currCons->backColor << 4);
 								break;
								
							/* Set the background color */
							case 40 ... 47:
								currCons->backColor = colorTable[currCons->params[0] % 10];
								currCons->currColor = currCons->foreColor | (currCons->backColor << 4);
								break;
						};

						break;
				}

				/* Reset the control variables, in case we get another iteration. */
				currCons->inCtrl= currCons->paramIndex = currCons->cmdLetter = 0;
				goto end;
			}
		}
		outb(0xE9, *str);
		ConsoleWrite(*str, console);
end:
		str++;
	}

	ConsoleUpdateCursorPos(console);

	return oldLength;
}

SYMBOL_EXPORT(ConsoleWriteString);

extern void KeyboardDoAlarm();

/* Writes an ASCII character to screen */

static void ConsoleWrite(char c,int index)
{
	struct Console* cons=&consoles[index];

	if (cons->flags & CONSOLE_GRAPHICS)
		return;

	switch (c)
	{
	case '\r':
		break;

	case '\n':
		ConsoleNewLine(index);
		break;
	
	case '\b':
		if (cons->currX > 0)
		{
			cons->currX--;
			
			if (currConsole == index)
			{
				screen[(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)]=' ';
				screen[(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)+1]=cons->currColor;
			}else{
				scrBuf[(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*index)+(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)]=' ';
				scrBuf[(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*index)+(cons->currY*(CONSOLE_WIDTH*2))+(cons->currY*2)+1]=cons->currColor;
			}
		}
		break;

	case '\t':
	{
		if (currConsole == index)
		{
			int i;
			for (i=0; i<TAB_WIDTH*2; i+=2)
			{
				screen[(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)+i]=' ';
				screen[(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)+1+i]=cons->currColor;
				++cons->currX;
				if (cons->currX >= CONSOLE_WIDTH)
					ConsoleNewLine(currConsole);
			}
		}else{
			int i;
			for (i=0; i<TAB_WIDTH*2; i+=2)
			{
				scrBuf[(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*index)+(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)+i]=' ';
				scrBuf[(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*index)+(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)+i+1]=cons->currColor;
				++cons->currX;
				if (cons->currX >= CONSOLE_WIDTH)
					ConsoleNewLine(index);
			}
		}
		break;
	}

	default:
		if (currConsole == index)
		{
			screen[(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)]=c;
			screen[(cons->currY*(CONSOLE_WIDTH*2))+(cons->currX*2)+1]=cons->currColor;
		}else{
			CONSOLE_BUF(index)[CONSOLE_IND(cons->currX*2, cons->currY)]=c;
			CONSOLE_BUF(index)[CONSOLE_IND(cons->currX*2, cons->currY)+1]=cons->currColor;
		}
		
		cons->currX++;
		
		if (cons->currX >= CONSOLE_WIDTH)
			ConsoleNewLine(index);
	}
}

void ConsoleWriteOutput(char* message, int length)
{
	ConsoleWriteString(currConsole, message, length);
}

static void ConsoleClearScreen(int index)
{
	if (consoles[index].flags & CONSOLE_GRAPHICS)
		return;

	int i;
	if (currConsole == index)
	{
		for (i=0; i<(CONSOLE_WIDTH*CONSOLE_HEIGHT*2); i+=2)
		{
			screen[i]=' ';
			screen[i+1]=7;
		}
	}else{
		for (i=0; i<(CONSOLE_WIDTH*CONSOLE_HEIGHT*2); i+=2)
		{
			scrBuf[(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*index)+i]=' ';
			scrBuf[(CONSOLE_WIDTH*CONSOLE_HEIGHT*2*index)+i+1]=7;
		}
	}
	
	consoles[index].currX=0;
	consoles[index].currY=0;
	
	ConsoleUpdateCursorPos(index);
}

void ConsoleClearCurrent()
{
	ConsoleClearScreen(currConsole);
}

static void ConsoleUpdateCursorPos(int console)
{
	WORD position;
	struct Console* cons=&consoles[currConsole];

	if (console != currConsole)
		return;

	position=(cons->currY*(CONSOLE_WIDTH))+cons->currX;

	PreemptDisable();
	outb(0x3D4,14);
	outb(0x3D5,(BYTE)(position >> 8));
	outb(0x3D4,15);
	outb(0x3D5,(BYTE)(position & 0xFF));
	PreemptFastEnable();
}

static void ConsoleGetCursorPos()
{
	DWORD flags;
	WORD position;

	IrqSaveFlags(flags);

	outb(0x3D4, 14);
	position=inb(0x3D5) << 8;
	outb(0x3D4, 15);
	position |= inb(0x3D5);

	IrqRestoreFlags(flags);

	consoles[currConsole].currX=position % 80;
	consoles[currConsole].currY=position/80;
}

static void ConsoleNewLine(int index)
{
	struct Console* console=&consoles[index];
	
	console->currX=0;
	console->currY++;
	
	if (console->currY == CONSOLE_HEIGHT)
	{
		console->currY=CONSOLE_HEIGHT-1;
		ConsoleScrollScreen(index);
	}
	
	ConsoleUpdateCursorPos(index);
}

static void ConsoleAddToQueue(BYTE n)
{
	BYTE head;
	DWORD flags;
	struct Console* currCons;

	/* May not press keys before console array has been set up */
	if (!scrBuf)
		return;

	currCons=&consoles[currConsole];

	if (!currCons)
		return;

	IrqSaveFlags(flags);

	head=(currCons->head+1) % CONSOLE_KEYBUF_SIZE;
	
//	KePrint("ConsoleAddToQueue(%u), %u %u\n", n, head, currCons->tail);
	
	if (head != currCons->tail)
	{
		currCons->keyboardBuf[currCons->head]=n;
		currCons->head=head;
		WakeUp(&currCons->waitQueue);
	}else
		KePrint("ConsoleAddToQueue: Keyboard buffer full!\n");

	IrqRestoreFlags(flags);
}

void ConsoleAddKey(BYTE key)
{
	if (!(consoles[currConsole].flags & CONSOLE_SENDKEYC))
		ConsoleAddToQueue(key);
}

SYMBOL_EXPORT(ConsoleAddKey);

void ConsoleAddKeyCode(BYTE key)
{	
	if (consoles[currConsole].flags & CONSOLE_SENDKEYC)
		ConsoleAddToQueue(key);
}

SYMBOL_EXPORT(ConsoleAddKeyCode);

int ConsoleEarlyInit()
{
	ConsoleClearScreen(0);
	consoles[0].currColor=7;
	consoles[0].backColor=0;
	return 0;
}

BYTE ConsoleGetCharacter()
{
	BYTE retVal=0;
	struct Console* currCons=&consoles[currConsole];

	if (currCons->head != currCons->tail)
	{
		retVal=currCons->keyboardBuf[currCons->tail] & 0xFF;
		currCons->tail=(currCons->tail+1) % CONSOLE_KEYBUF_SIZE; /* Wraps around */
		if (!(currCons->flags & CONSOLE_NO_ECHO) && retVal < 0x80 && current && retVal != '\b') DoWriteFile(current->files[0],&retVal,1);
	}

	return retVal;
}

int ConsoleDevRead(struct File* file,BYTE* data,DWORD size)
{
	int index=DEV_ID_MINOR(file->vNode->devId); /* TODO: Change. */
	struct Console* currCons=&consoles[index];
	DWORD i=0;
	DWORD flags;

	IrqSaveFlags(flags);

	do
	{
		WAIT_ON(&consoles[index].waitQueue, currCons->head != currCons->tail);
		data[i++]=ConsoleGetCharacter();
	}while (i < size);

	IrqRestoreFlags(flags);

	return size;
}

int ConsoleDevWrite(struct File* file,BYTE* data,DWORD size)
{		
	return ConsoleWriteString(DEV_ID_MINOR(file->vNode->devId),(char*)data,size);
}

int ConsolePoll(struct File* file, struct PollItem* item, struct PollQueue* pollQueue)
{
	struct Console* cons=&consoles[DEV_ID_MINOR(file->vNode->devId)];

	if ((item->events & POLL_IN) && cons->head != cons->tail)
		item->revents |= POLL_IN;

	/* The console can always be written to. */
	if (item->events & POLL_OUT)
		item->revents |= POLL_OUT;

	PollAddWait(pollQueue, &cons->waitQueue);

	return 0;
}

struct FileOps consoleOps={
	.read = ConsoleDevRead, /* Read the keyboard buffer */
	.write = ConsoleDevWrite,
	.poll = ConsolePoll,
};

/* ConsoleGetCurrent also works for virtual terminals */

int ConsoleGetCurrent(struct KeFsEntry** dest, struct KeFsEntry* src)
{
	struct VNode* vNode = current->files[0]->vNode;
	struct KeFsEntry* entry = (struct KeFsEntry*)vNode->id;
	struct KeObject* object = (struct KeObject*)entry->file;
	
	*dest = object->dir;
		
	if (!*dest)
		return -ENOENT;
	
	return 0;
}

struct IcAttribute consAttributes[]=
{
	IC_END(),
};

KE_OBJECT_TYPE_SIMPLE(consType, NULL);

struct DevClass consoleClass;

SYMBOL_EXPORT(consoleClass);

static void ConsoleAddTextCons()
{
	int i;
	
	DevClassCreate(&consoleClass, &consType, "Consoles");

	/* Register all the consoles */
	for (i=0; i<MAX_CONSOLES; i++)
	{
		struct Console* curr=&consoles[i];
		
		KeDeviceCreate(&curr->device, &consoleClass.set, DEV_ID_MAKE(CONSOLE_MAJOR, i), &consoleOps, DEVICE_CHAR, "Console%d", i);
		
		INIT_LIST_HEAD(&curr->waitQueue.list);
		consoles[i].currColor=7;
		consoles[i].backColor=0;
		consoles[i].currX=consoles[i].currY=0;
		consoles[i].head=consoles[i].tail=0;
	
		/* Register each console's configuration options. TODO: Change to ICFS attributes. */
		struct KeFsEntry* dir=KeDeviceGetConfDir(&curr->device);
				
		IcFsAddIntEntry(dir, "flags", &curr->flags, VFS_ATTR_RW);
		IcFsAddIntEntry(dir, "rows", &rows, VFS_ATTR_READ);
		IcFsAddIntEntry(dir, "cols", &cols, VFS_ATTR_READ);
		IcFsAddIntEntry(dir, "cursorRow", &curr->currY, VFS_ATTR_RW);
		IcFsAddIntEntry(dir, "cursorCol", &curr->currX, VFS_ATTR_RW);
	}
	
	/* Add a symbolic link for the current console used by the process. */
	IcFsAddSoftLink(DevClassGetDir(&consoleClass), "currConsole", ConsoleGetCurrent);
	
	/* And the total number of text consoles. */
	IcFsAddIntEntry(DevClassGetDir(&consoleClass), "numTextConsoles", &numTextConsoles, VFS_ATTR_READ);
}

int ConsoleInit()
{
	/* Now allocate all the other consoles, since dynamic memory allocation can be performed. */
	scrBuf=(BYTE*)MemAlloc(MAX_CONSOLES*(CONSOLE_WIDTH*CONSOLE_HEIGHT*2));
	if (!scrBuf)
	{
		KernelPanic("Failed to allocate memory for multiple consoles");
		return -ENOMEM; /* Never reached. */
	}

	ZeroMemory(scrBuf,(MAX_CONSOLES*(CONSOLE_WIDTH*CONSOLE_HEIGHT*2)));
	
	ConsoleAddTextCons();

	/* Get the current cursor position, and take over from the early console driver. */
	ConsoleGetCursorPos();

	KeSetOutput(ConsoleWriteOutput);
	
	KePrint("CONS: Registered %u text consoles.\n", MAX_CONSOLES);
	
	return 0;
}

ModuleInit(ConsoleInit);
