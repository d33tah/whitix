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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <devices/device.h>
#include <typedefs.h>
#include <wait.h>

/* Defines */

/* TODO: Make it large, so the user is very unlikely to fill up the buffer. */
#define CONSOLE_KEYBUF_SIZE		100

int ConsoleEarlyInit();
void ConsoleClearCurrent();
void ConsoleSwitchToText();
void putc(int c);

int ChangeConsole(int index);
void ConsoleAddKey(BYTE n);
void ConsoleAddKeyCode(BYTE n);

#define MAX_CONSOLES 7
#define CONSOLE_KEYBUF_SIZE	100

extern int currConsole;

struct Console
{	
	struct KeDevice device;
	int currX, currY, currColor, foreColor, backColor;
	WaitQueue waitQueue;
	BYTE keyboardBuf[CONSOLE_KEYBUF_SIZE];
	BYTE head, tail;
	unsigned int params[16]; /* For ASCII control codes */
	int inCtrl,ctrlParams,cmdLetter,paramIndex;
	int flags;
};

extern struct Console consoles[MAX_CONSOLES];

/* Console ioctls */
#define CONSOLE_SET_OPTIONS 0x00000001
#define CONSOLE_GET_OPTIONS 0x00000002
#define CONSOLE_GET_INFO	0x00000003
#define CONSOLE_GET_POS		0x00000004
#define CONSOLE_GET_KEYMAP	0x00000005
#define CONSOLE_SET_KEYMAP	0x00000006

/* Console flags */
#define CONSOLE_NO_ECHO		0x00000001
#define CONSOLE_GRAPHICS	0x00000002
#define CONSOLE_SENDKEYC	0x00000004

/* and related structures */

struct ConsoleInfo
{
	int rows,cols;
};

#endif
