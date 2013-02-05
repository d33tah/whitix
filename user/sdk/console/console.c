#include <console.h>
#include <stdio.h>
#include <syscalls.h>

/* TODO: error checking. */

void ConsClearScreen()
{
	fputs("\33[H\33[2J",stdout);
}

/* TODO: Unsigned here. */
void ConsSetForeColor(int color)
{
	printf("\33[3%dm", color);
}

void ConsSetBackColor(int color)
{
	printf("\33[4%dm", color);
}

/* TODO: Convert to unsigned integers. */
void ConsCursorMove(int x, int y)
{
	/* Check before printing */
	printf("\33[%d;%dH", y, x);
}

void ConsCursorGet(struct ConsoleInfo* console)
{
	SysConfRead("/Devices/Consoles/currConsole/cursorRow", &console->rows, 4);
	SysConfRead("/Devices/Consoles/currConsole/cursorCol", &console->cols, 4);
}

void ConsCursorMoveRel(int x, int y)
{
	if (x > 0)
		printf("\33[%dC", x);
	else if (x < 0)
		printf("\33[%dD", -x);
		
	if (y > 0)
		printf("\33[%dA", y);
	else if (y < 0)
		printf("\33[%dB", -y);
}

void ConsCursorMoveCol(int col)
{
	printf("ConsCursorMoveCol\n");
}

void ConsEraseLine()
{
	/* Clear the whole of the current line */
	fputs("\33[2K",stdout);
}

void ConsInvert()
{
	fputs("\33[7m", stdout);
}

void ConsColorReset()
{
	fputs("\33[0m", stdout);
}

void ConsColorSetBold(int bold)
{
	printf("\33[%dm", 2-bold);
}

int ConsGetDimensions(struct ConsoleInfo* consInfo)
{
	int ret;
	
	ret = SysConfRead("/Devices/Consoles/currConsole/rows", &consInfo->rows, 4);
	
	if (ret <= 0)
		return -1;
	
	ret = SysConfRead("/Devices/Consoles/currConsole/cols", &consInfo->cols, 4);
	
	if (ret <= 0)
		return -1;
		
	return 0;
}

int ConsSetEcho(int echoOn)
{
	int flags;

	SysConfRead("/Devices/Consoles/currConsole/flags", &flags, 4);

	if (echoOn)
		flags &=~ CONSOLE_NO_ECHO;
	else
		flags |= CONSOLE_NO_ECHO;

	SysConfWrite("/Devices/Consoles/currConsole/flags", &flags, 4);
	
	return 0;
}

